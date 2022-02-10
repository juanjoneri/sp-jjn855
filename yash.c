#include <unistd.h>
#include <sys/wait.h>
#include "prompt.h"
#include "parser.h"

void executeAndWait(struct command* c) {
    printCommand(c);
    int cpid = fork();
    if (cpid == 0){
        setpgid(0, 0); // create a new group for the child
        execvp(c->program, c->arguments);
    } else if (!c->background) {
        // If the child ran in the foreground, we wait for its completion
        wait((int *)NULL);
        // TODO: handle signals (lecture 29 hr 2)
    } else {
        // If the child ran in the background, save it to the jobs list
        printf("%s", "Child running in background");
    }
}

int main() {
    char *line;
    struct command* c;
     
    while(line = prompt()) {
        c = parseCommand(line);
        executeAndWait(c);        
    }

    freeCommand(c);
    free(line);
    return 0;
}

