#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h>

#include "parse.h"
#include "process.h"

/* Strategy:
Gets the arg count for child 1, and child 2 (if there is a pipe)
and mallocs argv for processes
Return 1 if pipe, or 0 if no
*/
Process *malloc_argv(char cmd[MAXLINE]){
    
    int word_count = 0;
    int curr_word = 0;
    int argc = 0;
    int n1 = 0;
    Process *p1 = NULL;
    Process *p_last = NULL;
    Process *p = NULL;

    // dynamically allocate memory for argv
    for(int i = 0; i < strlen(cmd); i++){
        if(cmd[i] == '|'){
            p_last = p;
            p = init_process(word_count);
            if(!n1){
                p1 = p;
                n1++;
            }
            if(p_last != NULL) p_last->p_next = p;
            word_count = 0;
            curr_word = 0;
        }else if(cmd[i] == ' '){
            if(curr_word != 0) word_count++;
            curr_word = 0;
        }else{
            curr_word++;
        }
    }

    if(cmd[strlen(cmd) - 1] != ' ') word_count++;

    p_last = p;
    p = init_process(word_count);
    if(!n1){
        p1 = p;
        n1++;
    }
    if(p_last != NULL) p_last->p_next = p;

    return p1;
}

/* Strategy:
    * Check for & at end of cmd, set bg = 1 if there is
    * Split the command by spaces
    * returns 0 if no pipe, and 1 if there is a pipe
*/
Job *parse_command(char cmd[MAXLINE]){
    int arg1 = 1;
    int i = 0;
    char *s;
    const char space = ' ';
    const char* pipe = "|";

    Job *j = init_job(cmd);
    Process *p;


    // Check if background job
    if(cmd[strlen(cmd)-1] == '&') {
        j->bg = 1;
        cmd[strlen(cmd)-1] = 0;
        j->cmd[strlen(cmd)-1] = 0;
    }

    
    // Malloc for all processes and assign first process to job
    p = malloc_argv(cmd);
    j->p1 = p;


    // Split cmd by spaces
    s = strtok(cmd, &space);

    // https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
    while(s != NULL){
        if(!strcmp(s, pipe)){
            // arg1 = 0;
            i = 0;
            j->nchild++;
            p = p->p_next;
        }else if(arg1){
            strcpy(p->argv[i++], s);
        }else{
            strcpy(p->argv[i++], s);
        }
        s = strtok(NULL, &space);
    }

    return j;
}

int fileExists(char* file) {
    int file_descriptor = open(file, O_RDONLY);
    if (file_descriptor == -1) {
        return 0;
    }
    close(file_descriptor);
    return 1;
}

/* Strategy:
    * Loop through argv and look for < or >
        * if >:
            * Open next value to argv
            * Dup2 to output
        * if >:
            * Open next value to argv
            * Dup2 to input
    * 
*/
int file_redirection(Process *p){
    int argc=-1;

    for (int i = 0; i < p->argc; i++){
        if(strcmp(p->argv[i], "<")==0) {
            // if(p->in != -1)close(p->in);
            // printf("Input from: %s\n", p->argv[i+1]);

            if((p->in = open(p->argv[i+1], O_RDONLY)) == -1){ // O_RDONLY = read only
                perror("Redirecting input");
            }

            if(argc == -1) argc = i;
        }
        else if(strcmp(p->argv[i], ">")==0) {
            close(p->out);

            // printf("Output to: %s\n", p->argv[i+1]);
            if (fileExists(p->argv[i+1])) {
                if((p->out = open(p->argv[i+1], O_CREAT | O_WRONLY | O_TRUNC)) == -1){ // O_WRONLY = write only, O_TRUNC overwrites file we open
                    perror("Redirecting output");
                }
            } else {
                p->out = creat(p->argv[i+1], 0644);
            }

            if(argc == -1) argc = i;
        }
    }


    if(argc != -1) {
        for(int i = argc; i < p->argc; i++){
            p->argv[i] = NULL;
        }

        p->argc = argc;
    }
    return 0;
}