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

int doExecute(struct command* c, int input, int output, int to_close, int pgid) {
    int child_pid = fork();
    if (child_pid == 0) {
        closeFileDescriptor(to_close);
        if (input != -1) {
            dup2(input, STDIN_FILENO);  // Override stdin with infile
        }
        if (output != -1) {
            dup2(output, STDOUT_FILENO);  // Override stdout with outfile
        }
        setpgid(0, pgid);
        execvp(c->program, c->arguments);
    }
    return child_pid;
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
    if (c->program == NULL) {
        printf("no program\n");
        return;
    }

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
        }
        
        int child_input = getInputFileDescriptor(c->pipe);
        int child_output = getOutputFileDescriptor(c->pipe);

        if (child_input == -1) {
            // Only use pipe for input when no explicit input redirection
            child_input = pipe_right;
        }

        int left_child_pid, right_child_pid; 
        
        left_child_pid = doExecute(c, input, output, pipe_right, /*pgid*/ 0);
        int pipe_pgid = left_child_pid;
        if (left_child_pid != 0) {
            // Only execute right child from parent
            right_child_pid = doExecute(c->pipe, child_input, child_output, pipe_left, pipe_pgid);
        }

        if ((left_child_pid != 0) && (right_child_pid != 0)) {
            // Parent
            tcsetpgrp(0, pipe_pgid); // Give terminal control to the group

            closeFileDescriptor(pipe_right);
            closeFileDescriptor(pipe_left);
            
            int status;
            do { 
                waitpid(left_child_pid, &status, WUNTRACED);
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));
            do { 
                waitpid(right_child_pid, &status, WUNTRACED);
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));

            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpgid(getpid())); // Claim terminal control
        }

    } else {
        int child_pid = doExecute(c, input, output, -1, 0);
        if (child_pid != 0) {
            tcsetpgrp(0, child_pid); // Give terminal control to child
            int status, w;
            do { 
                w = waitpid(child_pid, &status, WUNTRACED);
                if (w == -1) {
                    exit(EXIT_FAILURE);
                }
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));
            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(0, getpgid(getpid())); // Claim terminal control
        }
    }
}

int main() {
    char* line;
    struct command* c;

    while (line = prompt()) {
        c = parsePipe(line);
        execute(c);
        free(line);
        freeCommand(c);
    }

    return 0;
}
