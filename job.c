#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* copyString(char* str) {
    char* copy = malloc(strlen(str) + 1);
    strcpy(copy, str);
    return copy;
}

enum JobState { STOPPED, RUNNING };

struct job {
    char* source;
    enum JobState state;  // 0 stopped, 1 running
    int id;
    int pgid;  // process group id
    struct job* next;
};

struct job* allocJob() {
    struct job* j = malloc(sizeof(struct job));

    j->source = copyString("ls -la > out.txt");
    j->state = STOPPED;
    j->id = 1;
    j->next = NULL;

    return j;
}

void freeJobChain(struct job* j) {
    free(j->source);
    if (j->next != NULL) {
        freeJobChain(j->next);
    }
    free(j);
}

void addToJobChain(struct job* root, struct job* next) {
    struct job* current = root;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = next;
}

void printJobChain(struct job* root) {
    struct job* current = root;
    while (current != NULL) {
        printf("[%d]", current->id);
        if (current->next != NULL) {
            printf("-");
        } else {
            printf("+");
        }
        if (current->state != STOPPED) {
            printf(" Stopped");
        } else {
            printf(" Running");
        }
        printf("\t%s\n", current->source);
        current = current->next;
    }
}

int main() {
    struct job* a = allocJob();
    struct job* b = allocJob();
    struct job* c = allocJob();
    addToJobChain(a, b);
    addToJobChain(a, c);

    printJobChain(a);
    freeJobChain(a);

    return 0;
}