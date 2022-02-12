#include "parser.h"
#include "prompt.h"

int main() {
    char* line;
    struct command* c;

    while (line = prompt()) {
        c = parsePipe(line);
        printCommand(c);
        freeCommand(c); 
    }

    free(line);
    return 0;
}