#include <unistd.h>
#include <sys/wait.h>
#include "prompt.h"
#include "parser.h"

int main() {
    int cpid;
    char *line;
    struct command* c;
     
    while(line = prompt()) {
        c = parseCommand(line);
        printf("Executing %s with %d arguments\n\n", c->program, c->argument_count);
        cpid = fork();
        if (cpid == 0){
            execvp(c->program, c->arguments);
        }else{
            wait((int *)NULL);
        }
    }
    freeCommand(c);
    free(line);
    return 0;
}

