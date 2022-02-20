#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "job.h"
#include "prompt.h"

void closeFileDescriptor(int file_descriptor) {
    if (file_descriptor != -1) {
        close(file_descriptor);
    }
}

void giveTerminalControl(int pgid) {
    tcsetpgrp(0, pgid);
}

void claimTerminalControl() {
    signal(SIGTTOU, SIG_IGN); // Prevent IO from breaking the program
    tcsetpgrp(0, getpgid(getpid()));
}

int doExecute(struct command* c, int input, int output, int to_close,
              int pgid) {
    int child_pid = fork();
    if (child_pid == 0) {
        setpgid(0, pgid);
        closeFileDescriptor(to_close);
        if (input != -1) {
            dup2(input, STDIN_FILENO);  // Override stdin with infile
        }
        if (output != -1) {
            dup2(output, STDOUT_FILENO);  // Override stdout with outfile
        }
        execvp(c->program, c->arguments);
    }
    return child_pid;
}

int fileExists(char* file) {
    int file_descriptor = open(file, O_RDONLY);
    if (file_descriptor == -1) {
        return 0;
    }
    closeFileDescriptor(file_descriptor);
    return 1;
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

enum JobState waitForChild(int child_pid) {
    int status, w;
    while (1) {
        w = waitpid(child_pid, &status, WUNTRACED);
        if (w == -1) {
            // Terminated with a failure
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            // Terminated normally
            return TERMINATED;
        }
        if (WIFSTOPPED(status)) {
            // Terminated by signal SIGSTP (^z)
            return STOPPED;
        }
        if (WIFSIGNALED(status)) {
            // Terminated by a signal SIGINT (^c)
            return TERMINATED;
        }
    }
}

void fg(struct job* job) {
    giveTerminalControl(job->pgid);
    kill(job->pgid, SIGCONT);
    enum JobState state = waitForChild(job->pgid);
    claimTerminalControl();
    job->state = state;
}

void bg(struct job* job) { kill(job->pgid, SIGCONT); }

void printHistory(struct command* history[], int history_size) {
    for (int h = 0; h < history_size; h++) {
        printf(" %d ", h);
        printCommand(history[h]);
        printf("\n");
    }
}

void execute(struct job* job) {
    struct command* c = job->command;
    if (c->program == NULL) {
        return;
    }

    if (hasInputRedirection(c) && !fileExists(c->infile)) {
        printf("%s not such file or directory\n", c->infile);
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

        int left_child_pid =
            doExecute(c, input, output, pipe_right, /*pgid*/ 0);
        int pipe_pgid = left_child_pid;
        int right_child_pid =
            doExecute(c->pipe, child_input, child_output, pipe_left, pipe_pgid);

        closeFileDescriptor(pipe_right);
        closeFileDescriptor(pipe_left);

        giveTerminalControl(pipe_pgid);

        waitForChild(right_child_pid);
        waitForChild(left_child_pid);

        claimTerminalControl();

    } else {
        int child_pid = doExecute(c, input, output, -1, 0);
        job->pgid = child_pid;
        if (!c->background) {
            fg(job);
        }
    }
}

int main() {
    // Ignore ^Z in yash
    signal(SIGTSTP, SIG_IGN);

    char* line;
    int history_size = 100;
    struct command* history[history_size];
    struct job* job_chain = NULL;
    int i = 0;

    while (line = prompt()) {
        if (job_chain != NULL) {
            updateJobStates(job_chain);
            job_chain = removeTerminated(job_chain);
        }
        if (strEquals(line, "history")) {
            printHistory(history, i);
            continue;
        }
        if (i >= history_size) {
            for (int h = 0; h < i; h++) {
                freeCommand(history[h]);
            }
            i = -1;
        }
        if (strEquals(line, "jobs")) {
            printJobChain(job_chain);
            continue;
        }
        if (strEquals(line, "fg")) {
            struct job* last_job = getLastJob(job_chain);
            if (last_job != NULL) {
                last_job->state = RUNNING;
                printJob(last_job);
                fg(last_job);
            }
            continue;
        }
        if (strEquals(line, "bg")) {
            struct job* last_stopped = getLastStoppedJob(job_chain);
            if (last_stopped != NULL) {
                last_stopped->state = RUNNING;
                printJob(last_stopped);
                bg(last_stopped);
            }
            continue;
        }

        struct command* c = parsePipe(line);
        job_chain = addToJobChain(job_chain, c);

        signal(SIGTSTP, SIG_DFL);  // Allow ^z while child owns the terminal
        execute(getLastJob(job_chain));
        signal(SIGTSTP, SIG_IGN); // Ignore ^z when yash owns the terminal

        free(line);
        history[i++] = c;
    }

    freeJobChain(job_chain);
    for (int h = 0; h < i; h++) {
        freeCommand(history[h]);
    }
    return 0;
}
