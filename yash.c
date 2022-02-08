#include <stdio.h>
#include <stdlib.h>
#include "prompt.h"

int main() {
    char *line;
    while(line = prompt()) {
        printf("You entered %s", line);
    }
    free(line);
    return 0;
}

