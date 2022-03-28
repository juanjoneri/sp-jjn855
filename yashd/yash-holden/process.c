#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h>

#include "process.h"
#include "job.h"
#include "parse.h"

Process *init_process(int argc){
    Process *p = malloc(sizeof(*p));

    p->argv = (char **)calloc(argc, sizeof(char*));
    for(int i=0; i < argc; i++){
        p->argv[i] = (char *)calloc(1, sizeof(char[MAXLINE]));
    }

    p->argc = argc;
    p->in = -1;
    p->out = -1;
    p->p_next = NULL;
    return p;
};

void free_process(Process *p){
    if(p == NULL) return;

    for(int i=0; i < p->argc; i++){
        if(p->argv[i] != NULL) free(p->argv[i]);
        p->argv[i] = NULL;
    }
    if(p->argv != NULL)free(p->argv);
    p->argv = NULL;
    free(p);
    p = NULL;
}

// We will do a seperate one of these for each process
/* Strategy:
Open and dup2 accordingly if there is a pipe 
Check for file redirection and set input and output accordingly
End argv at the value before the first < >
Call execvp
*/
int exec_process(Process *p, int sockfd){
    int exec_status;

    // fprintf(logfd, "Child: exec_process\n");
    fflush(logfd);

    if(p->argc <= 0) exit(0);

    // loop through and set input and ouput streams according to <, >
    file_redirection(p);

    if(p->in != -1){
        // fprintf(logfd, "Input to: %d\n", p->in);
        dup2(p->in,STDIN_FILENO);
        close(p->in);
    }
    if(p->out != -1){
        // fprintf(logfd, "Output to: %d\n", p->out);
        dup2(p->out,STDOUT_FILENO);
        dup2(p->out,STDERR_FILENO);
        close(p->out);
    }
    fflush(logfd);

    if(!strcmp(p->argv[0], "jobs")){
        print_jobs(sockfd);
    }

    // print_process(p);
    fflush(logfd);

    exec_status = execvp(p->argv[0], p->argv);
    // perror("execvp: ");
    exit(0);
}

void print_process(Process * p){
    fprintf(logfd, "argv[0]:%s\nargv:", p->argv[0]);
    for(int i = 0; i < p->argc; i++){
        fprintf(logfd, "%s ", p->argv[i]);
    }
    fprintf(logfd, "\n");
}