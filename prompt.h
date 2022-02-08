#include <stdio.h>
#include <stdlib.h>

char* prompt() {
    char *line;
    long unused = 0;

    printf("# ");
    getline(&line, &unused, stdin);
    return line;
}
