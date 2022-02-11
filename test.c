#include "parser.h"

int main() {
    char* source = "debajo del mar&";
    struct command* c;
    c = parsePipe(source);
    printCommand(c);
    freeCommand(c);
    return 0;
}   