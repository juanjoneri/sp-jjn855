#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

enum JobState { STOPPED, RUNNING, TERMINATED };

struct job {
    struct command* command;
    enum JobState state;
    int id;
    int pgid;
    struct job* next;
};

struct job* allocJob(struct command* c, int next_id) {
    struct job* j = malloc(sizeof(struct job));
    j->command = c;
    j->state = RUNNING;
    j->id = next_id;
    j->next = NULL;
    return j;
}

void freeJobChain(struct job* job_chain) {
    if (job_chain->next != NULL) {
        freeJobChain(job_chain->next);
    }
    free(job_chain);
}

void printJob(struct job* j) {
    printf("[%d]", j->id);
    if (j->next != NULL) {
        printf("-");
    } else {
        printf("+");
    }
    if (j->state == STOPPED) {
        printf(" Stopped");
    } else if (j->state == TERMINATED) {
        printf(" Done");
    } else {
        printf(" Running");
    }
    printf("\t");
    printCommand(j->command);
    printf("\n");
}

void printJobCommand(struct job* j) {
    printCommand(j->command);
    printf("\n");
}

void printJobChain(struct job* job_chain) {
    if (job_chain == NULL) {
        return;
    }

    struct job* current = job_chain;
    while (current != NULL) {
        printJob(current);
        current = current->next;
    }
}

int getNextJobId(struct job* job_chain) {
    if (job_chain == NULL) {
        return 1;
    }
    struct job* current = job_chain;
    while (current->next != NULL) {
        current = current->next;
    }
    return current->id + 1;
}

struct job* addToJobChain(struct job* job_chain, struct command* c) {
    if (job_chain == NULL) {
        struct job* job = allocJob(c, 1);
        return job;
    }
    struct job* current = job_chain;
    struct job* next = allocJob(c, getNextJobId(job_chain));

    while (current->next != NULL) {
        current = current->next;
    }
    current->next = next;
    return job_chain;
}

struct job* getLastJob(struct job* job_chain) {
    if (job_chain == NULL) {
        return NULL;
    }
    struct job* current = job_chain;
    while (current->next != NULL) {
        current = current->next;
    }
    return current;
}

struct job* getLastStoppedJob(struct job* job_chain) {
    if (job_chain == NULL) {
        return NULL;
    }
    struct job* current = job_chain;
    struct job* last_stopped = current->state == STOPPED ? current : NULL;
    while (current->next != NULL) {
        current = current->next;
        if (current->state == STOPPED) {
            last_stopped = current;
        }
    }
    return last_stopped;
}

void removeNext(struct job* job_chain) {
    if (job_chain->next == NULL) {
        return;
    }
    struct job* next = job_chain->next;
    job_chain->next = next->next;
    free(next);
}

struct job* removeTerminated(struct job* job_chain) {
    struct job* current = job_chain;
    while (current->next != NULL) {
        while (current->next != NULL && current->next->state == TERMINATED) {
            removeNext(current);
        }
        if (current->next != NULL) {
            current = current->next;
        }
    }
    if (job_chain->state == TERMINATED) {
        struct job* next = current->next;
        free(job_chain);
        return next;
    }
    return job_chain;
}

void updateJobStates(struct job* job_chain) {
    int status;
    struct job* current = job_chain;
    while (current != NULL) {
        if (current->state == RUNNING) {
            if (waitpid(current->pgid, &status, WNOHANG) != 0) {
                current->state = TERMINATED;
            }
        }
        current = current->next;
    }
}