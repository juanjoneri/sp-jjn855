#ifndef PROCESS_H
#define PROCESS_H

typedef struct Process Process;

struct Process{
    char ** argv;
    int argc;
    int pid;
    int pgid;
    int in;
    int out;
    Process *p_next;
};

Process *init_process(int argc);
void free_process(Process *p);
int exec_process(Process *p, int sockfd);
void print_process(Process * p);

#endif