#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"
#include "prompt.h"


void closeFileDescriptor(int file_descriptor) {
    if (file_descriptor != 0) {
        close(file_descriptor);
    }
}

void doExecute(struct command* c, int input, int output, int to_close[]) {
    int child_pid = fork();
    if (child_pid == 0) {
        closeFileDescriptor(to_close[0]);
        closeFileDescriptor(to_close[1]);
        if (input != 0) {
            dup2(input, STDIN_FILENO);  // Override stdin with infile
        }
        if (output != 0) {
            dup2(output, STDOUT_FILENO);  // Override stdout with outfile
        }
        execvp(c->program, c->arguments);
    }
}


void executeChild(struct command* c, int parent_output, int unused_pipe) {
    int input = parent_output, output, pipe_left, pipe_right;

    if (hasInputRedirection(c)) {
        // Input redirection overrides pipe
        input = creat(c->infile, 0644);
    }
    if (hasPipe(c)) {
        int pipe_file_descriptor[2];
        pipe(pipe_file_descriptor);
        pipe_left = pipe_file_descriptor[1];
        pipe_right = pipe_file_descriptor[0];
        output = pipe_left;
    }
    if (hasOutputRedirection(c)) {
        // Output redirection overrides pipe
        output = creat(c->outfile, 0644);
    }

    int to_close[2] = {unused_pipe, pipe_right};
    doExecute(c, input, output, to_close);
    if (hasPipe(c)) {
        executeChild(c->pipe, pipe_right, /* unused_pipe */ pipe_left);
    }
    closeFileDescriptor(input);
    closeFileDescriptor(output);
    closeFileDescriptor(pipe_left);
    closeFileDescriptor(pipe_right);

    int unused_status;
    if (parent_output == 0) {
        waitpid(-1, &unused_status, 0);
    }
    if (hasPipe(c)) {
        waitpid(-1, &unused_status, 0);
    }
}

void execute(struct command* c) {
    return executeChild(c, 0, 0);
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
