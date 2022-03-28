#include "yash_cmd.h"
#include "time.h"

void send_prompt(int socket_fd) {
    char* prompt = "\n# ";
    send(socket_fd, prompt, 3, 0);
}

char* current_time() {
    time_t now;
    time(&now);
    char* time_name = ctime(&now);
    time_name += 4; // Remove day
    time_name[strlen(time_name) - 6] = 0; // Remove year
    return time_name;

}

// --------------------------------------------------------Shell loop--------------------------------------------------------
/* Strategy:
    * Initizialize necessary variables;
    * recv from socket
        * TODO eventually we will need to print correct logging format: ex: "Sep 18 10:02:55 yashd[127.0.0.1:7815]: echo $PATH"
    * TODO: Check all children for SIGCHLD (check_jobs)
    * Remove "CMD " from beginning and \n and white space at end of command
    * If msg == "CTL {s}" we can just continue because there is no foreground process
    * Check cmd:
        * TODO fg: 
            * fg_job
            * Set job to foreground
        * TODO bg:
            * Just send kill to last stopped job
        * else:
            * execute command
*/
void shell_loop(struct Connection *conn){
    int status, pid, pipefd[2], next_job, n;
    Job *j;
    struct Job *job_table[MAXJOBS];
    static char orig_cmd[MAXLINE];
    char msg[MAXMSG];
    char * cmd = &msg[0];

    send_prompt(conn->connfd);

    while(1){
        memset(msg, 0, MAXMSG);
        cmd = &msg[0];

        // fprintf(logfd, "Recieving:\n");
        if((n = recv(conn->connfd, msg, MAXMSG, 0)) < 0){
            // fprintf(logfd,"Error recieving from connection\n");
            fflush(logfd);
            close(conn->connfd);
            pthread_exit(NULL);
        } else if (n == 0) {
            // fprintf(logfd,"Detected disconnect\n");
            fflush(logfd);
            close(conn->connfd);
            pthread_exit(NULL);
        } else {
            // fprintf(logfd, "Recieved msg:%s\n", msg);
            fflush(stdout);
        }


        // Check for SIGCHLDS
        //----------------------------------TODO----------------------------------
        check_jobs(job_table);

        // Remove "CMD " from beginning
        if(!strncmp(msg, "CMD ", 4)) cmd = &msg[4];
        else if(!strncmp(msg, "CTL d\n", 6)){
            // fprintf(logfd,"Detected disconnect\n");
            fflush(logfd);
            close(conn->connfd);
            pthread_exit(NULL);
        }
        else continue;

        //----------------------------------TODO----------------------------------
        // Check if CTL

        // Remove \n and white space at end of command
        while(strlen(cmd) >= 1 && (msg[strlen(msg)-1] == ' ' || msg[strlen(msg)-1] == '\n')) msg[strlen(msg)-1] = 0;
        if(!strlen(cmd)) {
            continue;
        }
        strcpy(orig_cmd, cmd);

        // Log the command in pretty format to log file
        fprintf(logfd, "%s yashd", current_time());
        fprintf(logfd, "[%s:%d]: ", inet_ntoa(conn->connaddr.sin_addr), ntohs(conn->connaddr.sin_port));
        fprintf(logfd, "%s\n", cmd);


        // Parse cmd
        if(!strcmp(cmd, "fg")){
            if((j = fg_job()) != NULL){
                pid = waitpid(-j->pgid, &status, WUNTRACED);
                update_status(pid, status);
            }
        }else if(!strcmp(cmd, "bg")){
            bg_job();
        }else{
            exec_cmd(cmd, conn->connfd);
        }

        // Semd a mew prompt when done with this command
        send_prompt(conn->connfd);
        

        fflush(logfd);
        // fflush(conn->connfd);
    }
}


/*Strategy:
    * Create a Parent to Child pipe (recvpipe)
        * We are doing this because we need the parent to handle recv
        * This is because if there is a "CTL {s}" message, we need to handle this
    * Parse command to get pipes, background, and split argv
    * For loop through all processes in job:
        * if p has a p_next:
            * create a pipe and set p->out and p->p_next->in accordingly
        * fork:
            * Parent:
                * Set pgid of children in both parent and child to avoid race condition
                * parent_fork()
            * Child 1:
                * Set pgid of children in both parent and child to avoid race condition
                * child_fork()
*/
void exec_cmd(char *cmd, int sockfd){
    int cpid;
    int cpgid = 0;
    int pipefd[2] = {-1, -1};
    int recvpipe[2] = {-1, -1};


    // Returns 1 if there is a pipeline
    Job *j = parse_command(cmd);


    // Need to create a pipe to communicate between the parent recv and sending to child process
    if (pipe(recvpipe) == -1) {
        perror("pipe");
        exit(-1);
    }

    for(Process *p = j->p1; p!=NULL; p = p->p_next){

        if(p->p_next!=NULL){
            //create pipe
            // printf("pipe\n");
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(-1);
            }
            p->out = pipefd[1];
            p->p_next->in = pipefd[0];
        }

        // fprintf(logfd, "forking\n");
        fflush(logfd);
        if ((cpid = fork()) < 0){
            perror("Fork");
            exit(0);
        }else if (cpid > 0){ 
            // Parent
            close(pipefd[1]);

            if(!cpgid) cpgid = cpid;
            j->pgid = cpgid;

            // Set pgid of children and TC in both parent and child to avoid race condition
            setpgid(cpid, cpgid);
            // if (!j->bg && tcsetpgrp(STDIN_FILENO, cpgid) != 0) perror("tcsetpgrp() error in parent fg");
        }else{ // Child 
            // fprintf(logfd, "child");
            p->pid = getpid();
            p->pgid = (cpgid == 0) ? p->pid : cpgid;

            // Set pgid of children and TC in both parent and child to avoid race condition
            setpgid(0, p->pgid);
            child_fork(p, sockfd, recvpipe[0]);

        }
    }
    close(pipefd[0]);

    parent_fork(j, sockfd, recvpipe[1]);

    close(recvpipe[0]);
    close(recvpipe[1]);

    return;
}


// // --------------------------------------------------------PARENT--------------------------------------------------------
/*Strategy:
    * Close pipes
    * TODO: initialize job struct and add to table
    * Fork:
        * Parent: Used to wait on executing children
            * wait for all process group = j->pgid to stop or terminate
            * send kill to cpid
        * Child (cpid): Used for handling recv
            * default handler for sigint (just so parent can kill when all children exit)
            * while(1):
                * recv from socket
                * if msg == CTL {signal}
                    * send signal to process group
                * else:
                    * send to pipe for executing children
*/
void parent_fork(Job *j, int sockfd, int recvpipe){
    int status, cpid;
    int pid = getpid();
    char msg[MAXMSG];
    
    // Add job to table
    // Job *j = init_job(cpid1, 1, orig_cmd, bg, pid);
    //----------------------------------TODO----------------------------------
    if (strcmp(j->p1->argv[0], "jobs")) {
        add_job(j);
    }


    // if it is a background job, add to the job table
    // fprintf(logfd, "Parent: waiting\n");
    fflush(logfd);
    if ((cpid = fork()) < 0){
        perror("Fork");
        exit(0);
    }else if (cpid > 0){  // parent which waits on processes

        close(recvpipe);

        if(!j->bg){
            // fprintf(logfd, "Parent: forground\n");
            fflush(logfd);
            pid = waitpid(-j->pgid, &status, WUNTRACED);

            //----------------------------------TODO----------------------------------
            update_status(j->pgid, status);

            // Need to make sure job wasn't sent to background
            //----------------------------------TODO----------------------------------
            while(!j->bg &&(pid = waitpid(-j->pgid, &status, WUNTRACED)) > 0){
                update_status(getpgid(j->pgid), status);
                // fprintf(logfd, "waiting");
            }
        }


        kill(cpid, SIGINT);

        // fprintf(logfd, "Parent: exiting\n");
        fflush(logfd);

        return;

    }else{ // Child for recv within parent

        signal(SIGINT, SIG_DFL);

        while(1){
            memset(msg, 0, MAXMSG);
            int n = recv(sockfd, msg, MAXMSG, 0);
            if (n < 0) {
                // fprintf(logfd,"Error recieving from connection\n");
                exit(-1);
            } else if (n == 0) {
                // fprintf(logfd,"Detected disconnect\n");
                exit(-1);
            } else {
                // fprintf(logfd,"Recieved msg: %s", msg);
                fflush(stdout);

                if(!strncmp(msg, "CTL ", 4)){
                    if(msg[4] == 'c'){
                        kill(-j->pgid, SIGINT);
                    }else if(msg[4] == 'z'){
                        kill(-j->pgid, SIGTSTP);
                    }
                }else{
                    // fprintf(logfd,"Sending through pipe to child: %s", msg);
                    fflush(stdout);
                    write(recvpipe, msg, n);
                }
            }
        }
    }
}



// --------------------------------------------------------CHILD--------------------------------------------------------
/*Strategy:
    * Reset signals to default
    * Dup input to pipe that comes from parent
    * Dup output to socket
    * Call exec_process
*/
void child_fork(Process *p, int sockfd, int recvpipe){

    // fprintf(logfd, "Child: child_fork\n");
    fflush(logfd);

    // Reset signals to the defaults
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    dup2(sockfd, STDOUT_FILENO);
    dup2(recvpipe, STDIN_FILENO);
    dup2(sockfd, STDERR_FILENO);

    // print_process(p);
    // fflush(logfd);

    exec_process(p, sockfd);
}