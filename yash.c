#include "prompt.h"
#include "parser.h"

int main() {
    char *line;
    struct command* c;
     
    while(line = prompt()) {
        c = parseCommand(line);
        printf("You entered %s", c->program);
    }
    freeCommand(c);
    free(line);
    return 0;
}

