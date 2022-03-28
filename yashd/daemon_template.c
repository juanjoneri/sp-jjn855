#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

/* Daemon Example script
Using this article: http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html

Important Notes:
    * IMPORTANT: When writing Daemons it is important to write very defensive code
        * This means a majority of our code should be error checking, and logging accordingly

Steps to creating a Daemon:
    1. Fork off parent process
    2. Change file mode to umask
        * Basically gives 777 permission to process (https://stackoverflow.com/questions/6198623/when-is-umask-useful)
    3. Open a log file
        * Not necessary, but because daemons do not have access to stdout, it can be only option for debugging
    4. Create unique session ID
        * So child does not become an orphan in the system
        * setsid()
    5. Change Working Directory
        * Change to root because it is always guranteed to exist on linux filesystem
        * chdir()
    6. Close File Descriptors
        * STDIN, STDOUT, STDERR
        * Potential security hazard

Initialization:
    * Typically Daemon runs within in an infinite loop (while(1))
        * often times there is a sleep() function at the end of the while loop, so that the program runs at specific time intervals

*/

int main(int argc, char const *argv[])
{
    pid_t pid, sid;
    FILE *log;

    //---------------------------------------------------- Step 1: Fork Parent ----------------------------------------------------
    /* Fork off the parent process */       
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
        we can exit the parent process. */
    if (pid > 0) {
        printf("process_id of child process %d \n", pid);
        exit(EXIT_SUCCESS);
    }

    //---------------------------------------------------- Step 2: Umask ----------------------------------------------------
    umask(0);

    //---------------------------------------------------- Step 3: Logging ----------------------------------------------------
    log = fopen("/tmp/yashd.log", "w"); // https://stackoverflow.com/a/23856430
    if (!log){
        exit(EXIT_FAILURE);
    }

    fprintf(log, "Log Opened Successfully\n");

    //---------------------------------------------------- Step 4: Create Unique Session ID ----------------------------------------------------
    if (!(sid = setsid())){
        fprintf(log, "Error creating unique session ID\n");
        exit(EXIT_FAILURE);
    }

    fprintf(log, "Created Session Successfully\n");

    //---------------------------------------------------- Step 5: Change Working Directory ----------------------------------------------------
    if ((chdir("/")) < 0) {
        fprintf(log, "Error changing to root directory\n");
        exit(EXIT_FAILURE);
    }

    fprintf(log, "Changed Directory Successfully\n");

    //---------------------------------------------------- Step 6: Close STD File Descriptors ----------------------------------------------------
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //---------------------------------------------------- Initialization ----------------------------------------------------

    //---------------------------------------------------- The Big Loop ----------------------------------------------------
    int i = 0;
    while(1){
        fprintf(log, "Daemon loop #%d\n", i++);
        fflush(log);

        sleep(5);
    }

    fclose(log);
    exit(EXIT_SUCCESS);
    return 0;
}
