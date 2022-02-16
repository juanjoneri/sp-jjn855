#include "parser.h"
#include "prompt.h"

int main() {
    char* line;
    struct command* c;

    while (line = prompt()) {
        printf("'%s'\n", line);
        c = parsePipe(line);
        printCommand(c);
        freeCommand(c); 
        // parseCommandTest(line);
        free(line);
    }

    return 0;
}