#include "parser.h"

int main() {
    char* source = "debajo < in.txt > out.txt &";
    struct command* c;
    c = parsePipe(source);
    printCommand(c);
    freeCommand(c);
    return 0;
}   