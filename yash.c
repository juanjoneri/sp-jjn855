#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"
#include "prompt.h"


void closeFileDescriptor(int file_descriptor) {
    if (file_descriptor != -1) {
        close(file_descriptor);
    }
}

void doExecute(struct command* c, int input, int output, int to_close) {
    int child_pid = fork();
    if (child_pid == 0) {
        closeFileDescriptor(to_close);
        if (input != -1) {
            dup2(input, STDIN_FILENO);  // Override stdin with infile
        }
        if (output != -1) {
            dup2(output, STDOUT_FILENO);  // Override stdout with outfile
        }
        execvp(c->program, c->arguments);
    }
}

int getInputFileDescriptor(struct command* c) {
    if (hasInputRedirection(c)) {
        return open(c->infile, O_RDONLY);
    }
    return -1;
}

int getOutputFileDescriptor(struct command* c) {
    if (hasOutputRedirection(c)) {
        return creat(c->outfile, 0644);
    }
    return -1;

}

void execute(struct command* c) {
    int input = getInputFileDescriptor(c);
    int output = getOutputFileDescriptor(c);

    if (hasPipe(c)) {
        int pipe_left, pipe_right, pipe_file_descriptor[2];
        pipe(pipe_file_descriptor);
        pipe_left = pipe_file_descriptor[1];
        pipe_right = pipe_file_descriptor[0];
        
        if (output == -1) {
            // Only use pipe for output when no explicit output redirection
            output = pipe_left;
        } else {
            // Otherwise close it because it won't be used
            closeFileDescriptor(pipe_left);
        }
        
        int child_input = getInputFileDescriptor(c->pipe);
        int child_output = getOutputFileDescriptor(c->pipe);

        if (child_input == -1) {
            // Only use pipe for input when no explicit input redirection
            child_input = pipe_right;
        } else {
            // Otherwise close it because it won't be used
            closeFileDescriptor(pipe_right);
        }

        doExecute(c, input, output, pipe_right);
        doExecute(c->pipe, child_input, child_output, pipe_left);

        closeFileDescriptor(pipe_left);
        closeFileDescriptor(pipe_right);

        waitpid(-1, NULL, 0);
        waitpid(-1, NULL, 0);

    } else {
        doExecute(c, input, output, -1);
        waitpid(-1, NULL, 0);
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
