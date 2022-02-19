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

void printJobChain(struct job* job_chain) {
    if (job_chain == NULL) {
        return;
    }

    struct job* current = job_chain;
    while (current != NULL) {
        printf("[%d]", current->id);
        if (current->next != NULL) {
            printf("-");
        } else {
            printf("+");
        }
        if (current->state == STOPPED) {
            printf(" Stopped");
        } else if (current->state == TERMINATED) {
            printf(" Done");
        } else {
            printf(" Running");
        }
        printf("\t");
        printCommand(current->command);
        printf("\n");
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
    struct job* current = job_chain;
    while (current->next != NULL) {
        current = current->next;
    }
    return current;
}

void remove_next(struct job* job_chain) {
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
            remove_next(current);
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