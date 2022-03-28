#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>

#include "socket.h"
#include "thread.h"

FILE *create_daemon();

/* Daemon Example script
Using this article: http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html

Important Notes:
    * IMPORTANT: When writing Daemons it is important to write very defensive code
        * This means a majority of our code should be error checking, and logging accordingly

1. Create Daemon: create_daemon()

2. Daemon Code:
    1. Initialization:
        1. Set up TCP/IP Server
            Create Socket:
            1. Create: socket()
            2. Bound Attributes: bind()
            3. Listen: listen()

            To Do Later:
            4. Accept connection: accept()
            5. Read and Write data: write() read()
            6. Close: close()

    2. Big Loop
        1. Accept connection: accept()
        2. Create thread

*/
FILE *logfd;
int main(int argc, char const *argv[])
{
    int sockfd, connfd, errno;
    socklen_t addrlen;
    struct sockaddr_in connaddr;
    pthread_attr_t tattr;
    pthread_t ptid;

    //---------------------------------------------------- Create Daemon ----------------------------------------------------
    
    logfd = create_daemon();

    //---------------------------------------------------- Initialization ----------------------------------------------------

    // Pthread Attributes
    /* Set threads to DETACHED
    We want all of the threads to be detached because we don't care about their termination and 
    we don't want to worry about freeing resouces
    https://docs.oracle.com/cd/E19455-01/806-5257/attrib-69011/index.html
    */
    if(pthread_attr_init(&tattr) != 0 || pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) !=0){
        exit(EXIT_FAILURE);
    }
    //---------------------------------------------------- Set Up Server ----------------------------------------------------
    
    sockfd = create_server_socket(logfd);

    //---------------------------------------------------- The Big Loop ----------------------------------------------------
    fflush(logfd);


    while(1){

        Connection *conn = init_connection();
        addrlen = sizeof(conn->connaddr);
        if((conn->connfd = accept(sockfd, (struct sockaddr *)&conn->connaddr, &addrlen)) == -1){
            exit(EXIT_FAILURE);
        }

        if((errno = pthread_create(&ptid, &tattr, thread_exec, conn))){
            exit(EXIT_FAILURE);
        }
        fflush(logfd);

    }


    //---------------------------------------------------- Close Socket and Logfd----------------------------------------------------
    if(close(sockfd) == -1){
        exit(EXIT_FAILURE);
    }
    fclose(logfd);

    exit(EXIT_SUCCESS);
    return 0;
}

void sig_pipe(int n)  {
    // https://cis.temple.edu/~ingargio/cis307/readings/unixechoserver.html#ten
    perror("Broken pipe signal");
}


/* Handler for SIGCHLD signal */
void sig_chld(int n) {
    // https://cis.temple.edu/~ingargio/cis307/readings/unixechoserver.html#ten
    int status;
    // fprintf(stderr, "Child terminated\n");
    // wait(&status); // TODO no zombies
    wait3(&status, WNOHANG, NULL);
}

/* Setup Daemon
 *
 * Steps to creating a Daemon:
 *    1. Fork off parent process
 *    2. Change file mode to umask
 *        * Basically gives 777 permission to process
 * (https://stackoverflow.com/questions/6198623/when-is-umask-useful)
 *    3. Open a log file
 *        * Not necessary, but because daemons do not have access to stdout, it
 * can be only option for debugging
 *     4. Create unique session ID
 *         * So child does not become an orphan in the system
 *         * setsid()
 *     5. Change Working Directory
 *         * Change to root because it is always guranteed to exist on linux
 * filesystem
 *        * chdir()
 *    6. Close File Descriptors
 *        * STDIN, STDOUT, STDERR
 *        * Potential security hazard
 *
 * return FILE* to log file
 */
FILE *create_daemon() {
    pid_t pid;
    int k, fd;
    char buff[256];
    FILE *log;
    FILE *pid_file;

    // Step 1: Operate the server as a background processes with as parent the init process
    if ((pid = fork()) < 0) {
        perror("daemon_init: cannot fork");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Exit parent
        printf("process_id of child process %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    // Step 2: Close all open descriptors of the parent
    for (k = getdtablesize()-1; k > 0; k--) {
      close(k);
    }


    // Step 6: Reset the standard files to /dev/null
    if ((fd = open("/dev/null", O_RDWR)) < 0) {
        perror("Open");
        exit(0);
    }
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    // Step 10: Ignore signals
    if (signal(SIGCHLD, sig_chld) < 0) {
        perror("Signal SIGCHLD");
        exit(1);
    }
    if (signal(SIGPIPE, sig_pipe) < 0) {
        perror("Signal SIGPIPE");
        exit(1);
    }

    // Step 4: Move to a Known Safe Directory
    if (chdir("/tmp") < 0) {
        printf("Could not cd tmp");
        exit(EXIT_FAILURE);
    }

    // Step 9: Umask
    umask(0);

    // Step 3: Detach from controlling terminal
    setsid();

    // Step 5: Put self in a new process group */
    // pid = getpid();
    // setpgid(0, pid);
    setpgrp();

    // Step 7: Make sure yashd is not already running
    if ((k = open("yashd.pid", O_RDWR | O_CREAT, 0666)) < 0) {
        exit(EXIT_FAILURE);
    }
    if (lockf(k, F_TLOCK, 0) != 0) {
        perror("daemon_init: yash already running");
        exit(EXIT_SUCCESS);
    }

    // Step 11: Logging
    log = fopen("yashd.log", "w");
    // Send stderr to log file
    fd = fileno(log);
    dup2(fd, STDERR_FILENO);
    // close (fd);

    // Step 8: Save pid of daemon to file
    pid = getpid();
    sprintf(buff, "%d", pid);
    write(k, buff, strlen(buff));

    return log;
}
