#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "parse.h"

// struct Job *job_table[MAXJOBS];
// static char orig_cmd[MAXLINE], cmd[MAXLINE];

void shell_loop();
void parent_fork(Job *j);
void child_fork(Process *p);
void exec_cmd();


int main(int argc, char const *argv[])
{
    // if(argc > 1) verbose = 1; // for debugging

    // We want to ignore all signals within the shell process
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    shell_loop();

    return 0;
}

// --------------------------------------------------------Shell loop--------------------------------------------------------
/* Strategy:
    * Initizialize necessary variables;
    * Ensure shell has TC
    * Get next command
    * Check all children for SIGCHLD (check_jobs)
    * Remove \n and white space at end of command
    * Parse cmd:
        * fg: 
            * fg_job
            * Set job to foreground
        *  bg:
            * Just send kill to last stopped job
        *  else:
            * execute command
*/
void shell_loop(){
    int status, pid;
    Job *j;

    while(1){
        // Ensure shell has control of terminal
        if (tcsetpgrp(STDIN_FILENO, getpid()) != 0){
            perror("tcsetpgrp() error");
        }


        // Get command
        printf("# ");
        if(fgets(cmd, MAXLINE, stdin) == NULL){
            // If CTRL-D is encountered we enter this if statement: https://stackoverflow.com/a/19228847
            printf("CTRL-D received, quitting shell\n");
            exit(0);
        }


        // Check for SIGCHLDS
        check_jobs();


        // Remove \n and white space at end of command
        while(strlen(cmd) >= 1 && (cmd[strlen(cmd)-1] == ' ' || cmd[strlen(cmd)-1] == '\n')) cmd[strlen(cmd)-1] = 0;
        if(!strlen(cmd)) continue;
        strcpy(orig_cmd, cmd);

        // printf("%s\n", cmd);


        // Parse cmd
        if(!strcmp(cmd, "fg")){
            if((j = fg_job()) != NULL){
                pid = waitpid(-j->pgid, &status, WUNTRACED);
                update_status(pid, status);
            }
        }else if(!strcmp(cmd, "bg")){
            bg_job();
        }else{
            exec_cmd();
        }
    }
}


/*Strategy:
    * Parse command to get pipes, background, and split argv
    * fork:
        * Parent:
            * if nchild > 1, fork again
                * Parent:
                    * Set pgid of children and TC in both parent and child to avoid race condition
                    * parent_fork
                * Child 2:
                    * Set pgid of children and TC in both parent and child to avoid race condition
                    * child_fork(2)
        * Child 1:
            * Set pgid of children and TC in both parent and child to avoid race condition
            * child_fork(1)
*/
void exec_cmd(){
    int cpid;
    int cpgid = 0;
    int pipefd[2] = {-1, -1};


    // Returns 1 if there is a pipeline
    Job *j = parse_command(cmd);

    // for(Process *p = j->p1; p!=NULL; p = p->p_next){
    //     printf("cmd:");
    //     for(int i = 0; i < p->argc; i++){
    //         printf("%s ", p->argv[i]);
    //     }
    //     printf("\n");
    // }

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

        if ((cpid = fork()) < 0){
            perror("Fork");
            exit(0);
        }else if (cpid > 0){ 
            // Parent
            close(pipefd[1]);

            if(!cpgid) cpgid = cpid;

            // Set pgid of children and TC in both parent and child to avoid race condition
            setpgid(cpid, cpgid);
            if (!j->bg && tcsetpgrp(STDIN_FILENO, cpgid) != 0) perror("tcsetpgrp() error in parent fg");
            
        }else{ // Child 
            p->pid = getpid();
            p->pgid = (cpgid == 0) ? p->pid : cpgid;

            // Set pgid of children and TC in both parent and child to avoid race condition
            setpgid(0, p->pgid);

            if (!j->bg && tcsetpgrp(STDIN_FILENO, p->pgid) != 0){
                perror("tcsetpgrp() error in child fg");
            }

            child_fork(p);

        }
    }
    close(pipefd[0]);

    j->pgid = cpgid;
    parent_fork(j);

    return;
}


// // --------------------------------------------------------PARENT--------------------------------------------------------
/*Strategy:
    * Close pipes
    * initialize job struct and add to table
    * wait for all process group = cpid1 to stop or terminate
*/
void parent_fork(Job *j){
    int status;
    int pid = getpid();    
    
    // Add job to table
    // Job *j = init_job(cpid1, 1, orig_cmd, bg, pid);
    add_job(j);


    // if it is a background job, add to the job table
    if(!j->bg){
        pid = waitpid(-j->pgid, &status, WUNTRACED);
        update_status(j->pgid, status);

        // Need to make sure job wasn't sent to background
        while(!j->bg &&(pid = waitpid(-j->pgid, &status, WUNTRACED)) > 0){
            update_status(getpgid(j->pgid), status);
        }
    }
}



// // --------------------------------------------------------CHILD--------------------------------------------------------
// /*Strategy:
//     * Reset signals to default
//     * Call exec_process
// */
void child_fork(Process *p){
    // Reset signals to the defaults
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    exec_process(p);
}