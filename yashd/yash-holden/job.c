#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "job.h"
#include "parse.h"


int next_job = 0;
Job *job_table1[MAXJOBS];
const char *status_types[] ={
    "Stopped",
    "Running",
    "Done"
}; 

Job *init_job(char cmd[MAXLINE]){
    Job *j = malloc(sizeof(*j) );
    j->index = -1;
    j->status = 1;
    j->bg = 0;
    j->nchild = 1;
    strcpy(j->cmd, cmd);
    return j;
}

// Add job to table and incriment next_job
void add_job(Job *j){
    j->index = next_job;
    job_table1[next_job++] = j;
}

// Free job and remove from job table
void free_job(Job *j){
    // If job is in table, we want to free it
    if(get_job(j->pgid) != NULL){
        if(next_job-1 == j->index){
            next_job--;
        }
        job_table1[j->index] = NULL;
    }else{
        return;
    }


    // Free processes within job
    Process *p = j->p1;
    Process *p_next;
    while(p!=NULL){
        p_next = p->p_next;
        free_process(p);
        p = p_next;
    }

    free(j);
}

// Get job by pgid
Job *get_job(int pgid){
    Job *j;

    for(int i = 0; i < next_job; i++){
        if((j = job_table1[i]) != NULL && j->pgid == pgid) return j;
    }
    return NULL;
}

// Set status of job with given pgid
Job *set_job(int pgid, int status){
    Job *j;
    if((j = get_job(pgid)) != NULL) {
        j->status = status;
        return j;
    }
    return NULL;
}

// Free job and print DONE if job in background
void job_exited(int pgid){
    Job *j = get_job(pgid);
    if(j!=NULL){
        if(j->bg)printf("[%d] + Done\t%s\n", j->index + 1, j->cmd);
        free_job(j);
    }
}

// Free job, and don't print done
void job_terminated(int pgid){
    Job *j = get_job(pgid);
    if(j!=NULL){
        // printf("[%d] + Done\t%s\n", j->index + 1, j->cmd);
        free_job(j);
    }
}

// Set status to 0 and bg  to 1
void job_stopped(int pgid){
    Job *j = set_job(pgid, 0);
    if(j!=NULL) j->bg = 1;
}

// Set status to 1
void job_cont(int pgid){
    Job *j = set_job(pgid, 1);
}


// Print all jobs up to second to last with -
// Last job print with +
void print_jobs(int sockfd){
    Job *j;

    // if no jobs, just exit
    if(next_job == 0) exit(0);

    for(int i = 0; i < next_job-1; i++){
        if((j = job_table1[i]) != NULL && j->status != 2){
            char* string;
            asprintf(&string, "[%d] - %s\t%s\n", i+1, status_types[j->status], j->cmd);
            send(sockfd, string, strlen(string), 0);
            free(string);
        }
    }
    if((j = job_table1[next_job-1]) != NULL && j->status != 2){
        char* string;
        asprintf(&string, "[%d] + %s\t%s\n", next_job, status_types[j->status], j->cmd);
        send(sockfd, string, strlen(string), 0); 
        free(string);
    }

    exit(0);
}

// Call correct function based on status
void update_status(int pgid, int status){
    if (WIFEXITED(status)) {
        job_exited(pgid);
    } else if (WIFSIGNALED(status)) {
        job_terminated(pgid);
    } else if (WIFSTOPPED(status)) {
        job_stopped(pgid);
    } else if (WIFCONTINUED(status)) {
        job_cont(pgid);
    }
}

// Check to see if any children have changed status
void check_jobs(){
    int status;
    int pid;

    // https://stackoverflow.com/a/2596788
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        update_status(pid, status);
    }
}

/*
Find the last job  that is stopped or in the background
Print command
Send SIGCONT to pgid
Update bg and next_job
*/
Job * fg_job(){
    Job *j;

    for(int i = next_job - 1; i >= 0; i --){
        if((j = job_table1[i]) != NULL && (j->status == 0 || j->bg)){
            printf("%s\n", j->cmd);

            if (tcsetpgrp(STDIN_FILENO, j->pgid) != 0){
                perror("tcsetpgrp() error in fg_job");
            }

            kill(-j->pgid, SIGCONT);

            j->bg = 0;
            return j;
        }
    }
    return NULL;
}

/*
Find the last job that is stopped
Print command
Send SIGCONT to pgid
Update bg and next_job
*/
Job * bg_job(){
    Job *j;

    for(int i = next_job - 1; i >= 0; i --){
        if((j = job_table1[i]) != NULL && j->status == 0){
            printf("[%d]+ %s &\n", i+1, j->cmd);

            if (tcsetpgrp(STDIN_FILENO, getppid()) != 0){
                perror("tcsetpgrp() error in fg_job");
            }

            kill(-j->pgid, SIGCONT);

            j->status=1;
            return j;
        }
    }
    return NULL;
}

/* Used for debugging
Go through table and print the processes in it
*/
void print_table(){
    Job *j;

    printf("print_table, next_job:%d\n", next_job);

    if(next_job == 0) return;

    for(int i = 0; i < next_job; i++){
        if((j = job_table1[i]) != NULL){
            // printf("[%d] - %s\t%s\n", i+1, status_types[j->status], j->cmd);
            // last_printed = i;
            printf("%d:%s: pid: %d, status: %d, bg: %d\n", i, j->cmd, j->pgid, j->status, j->bg);
        }
    }
}
