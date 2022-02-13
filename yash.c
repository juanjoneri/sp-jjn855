#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "parser.h"
#include "prompt.h"

void execute(struct command* c) {
    int output_file_descriptor;
    int input_file_descriptor;

    int cpid = fork();
    if (cpid == 0) {
        if (hasOutputRedirection(c)) {
            output_file_descriptor = creat(c->outfile, 0644);
            dup2(output_file_descriptor, 1);  // Override stdout with outfile
        }
        if (hasInputRedirection(c)) {
            input_file_descriptor = creat(c->infile, 0644);
            dup2(output_file_descriptor, 0);  // Override stdin with infile
        }
        execvp(c->program, c->arguments);
    } else if (isBackgroundJob(c)) {
        printf("%s", "Child running in background");
    } else {
        wait((int*)NULL);
        // TODO: handle signals (lecture 29 hr 2)
    }
}

int main() {
    char* line;
    struct command* c;

    while (line = prompt()) {
        c = parseCommand(line);
        execute(c);
        free(line);
        freeCommand(c);
    }

    return 0;
}
