#ifndef JOB_H
#define JOB_H

#include "process.h"

#define MAXJOBS 100
#define MAXLINE 200
#define MAXCMD 100

// extern int pipefd[];
// extern int next_job;

typedef struct Job{
    int pgid;
    int status; // 1 = running, 0 = stopped, 2 = done
    int index;
    int bg;
    int nchild; // number of processes
    char cmd[MAXLINE];
    Process *p1;
} Job;
extern struct Job *job_table1[MAXJOBS];

Job *init_job(char cmd[MAXLINE]);
void free_job(Job *j);
void add_job(Job *j);
Job *get_job(int pgid);
Job *set_job(int pgid, int status);
void print_dones();
void print_jobs(int sockfd);
Job *fg_job();
Job *bg_job();
void update_status(int pgid, int status);
void check_jobs();
void job_stopped(int cpid);
void job_terminated(int pgid);
void job_exited(int pgid);
void job_cont(int pid);
void print_table();

#endif