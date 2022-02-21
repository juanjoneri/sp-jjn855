#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    while (1) {
        printf("running\n");
        fflush(stdout);
        sleep(2);
    }
}