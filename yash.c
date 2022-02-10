#include <unistd.h>
#include <sys/wait.h>
#include "prompt.h"
#include "parser.h"

void executeAndWait(struct command* c) {
    printCommand(c);
    int cpid = fork();
    if (cpid == 0){
        execvp(c->program, c->arguments);
    }else{
        wait((int *)NULL);
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

