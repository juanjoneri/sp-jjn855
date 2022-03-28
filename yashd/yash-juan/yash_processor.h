#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#include "job.h"
#include "prompt.h"
#include "../socket.h"

FILE *log_fd;
int fgpgid;


struct thread_args {
    char *line;
    struct job *job_chain;
    int socket_fd;
    FILE *logfd;
};

void send_signal(char c){

    // int fgpgid = tcgetpgrp(0);

    if (getpgid(0) == fgpgid || fgpgid == 0) return; // don't want to send signal if no fg job

    switch(c){

        case 'c':
            killpg(fgpgid, SIGINT);
            return;
        
        case 'z':
            killpg(fgpgid, SIGTSTP);
            return;

    }
}

void closeFileDescriptor(int file_descriptor) {
    if (file_descriptor != -1) {
        close(file_descriptor);
    }
}

void giveTerminalControl(int pgid) { tcsetpgrp(0, pgid); }

void claimTerminalControl() {
    signal(SIGTTOU, SIG_IGN);  // Prevent IO from breaking the program
    // tcsetpgrp(0, getpgid(getpid()));
}

int doExecute(struct command* c, int input, int output, int to_close,
              int pgid, int socket_fd) {
    int child_pid = fork();
    if (child_pid == 0) {
        setpgid(0, pgid);
        closeFileDescriptor(to_close);
        // dup2(socket_fd, STDERR_FILENO);
        if (input != -1) {
            dup2(input, STDIN_FILENO);  // Override stdin with infile
        } else {
            printf("%s: %d\n", "Overriding stdin with", socket_fd);
            dup2(socket_fd, STDIN_FILENO);  // Override stdin with socket
        }
        if (output != -1) {
            dup2(output, STDOUT_FILENO);  // Override stdout with outfile
        } else {
            printf("%s\n", "Overriding stdout also");
            dup2(socket_fd, STDOUT_FILENO);  // Override stdout with socket
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

void execute(struct job* job, int socket) {
    struct command* c = job->command;
    if (c->program == NULL) {
        return;
    }

    if (hasInputRedirection(c) && !fileExists(c->infile)) {
        printf("%s not such file or directory\n", c->infile);
        return;
    }

    if (hasPipe(c) && hasInputRedirection(c->pipe) &&
        !fileExists(c->pipe->infile)) {
        printf("%s not such file or directory\n", c->pipe->infile);
        return;
    }

    int input = getInputFileDescriptor(c);
    int output = getOutputFileDescriptor(c);

    if (hasPipe(c)) {
        if (isBackgroundJob(c) || isBackgroundJob(c->pipe)) {
            printf("Cannot background a pipeline\n");
            return;
        }

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
            doExecute(c, input, output, pipe_right, /*pgid*/ 0, socket);
        int pipe_pgid = left_child_pid;
        fgpgid = pipe_pgid;
        int right_child_pid =
            doExecute(c->pipe, child_input, child_output, pipe_left, pipe_pgid, socket);


        closeFileDescriptor(pipe_right);
        closeFileDescriptor(pipe_left);

        giveTerminalControl(pipe_pgid);

        waitForChild(right_child_pid);
        waitForChild(left_child_pid);

        claimTerminalControl();

    } else {
        int child_pid = doExecute(c, input, output, -1, 0, socket);
        job->pgid = child_pid;
        if (!isBackgroundJob(c)) {
            fgpgid = child_pid;
            fg(job);
        }else{
            fgpgid = 0;
        }
    }
}

void *executeNext(void *arguments) {
    // Override global socket for default input and output redirection
    struct thread_args *args = arguments;
    // socket_fd = args->socket_fd;
    log_fd = args->logfd;

    // dup2(args->socket_fd, STDIN_FILENO);
    dup2(args->socket_fd, STDOUT_FILENO);

    fprintf(log_fd, "Executing next command\n");

    if (args->job_chain != NULL) {
        updateJobStates(args->job_chain);
        args->job_chain = removeTerminated(args->job_chain);
    }
    if (strEquals(args->line, "jobs\n")) {
        fprintf(log_fd, "printJobChain\n");
        printJobChain(args->job_chain);
        return NULL;
    }
    if (strEquals(args->line, "fg\n")) {
        struct job* last_job = getLastJob(args->job_chain);
        if (last_job != NULL) {
            last_job->state = RUNNING;
            printJobCommand(last_job);
            fg(last_job);
        }
        return NULL;
    }
    if (strEquals(args->line, "bg\n")) {
        struct job* last_stopped = getLastStoppedJob(args->job_chain);
        if (last_stopped != NULL) {
            last_stopped->state = RUNNING;
            printJob(last_stopped);
            bg(last_stopped);
        }
        return NULL;
    }

    struct command* c = parsePipe(args->line);
    printCommand(c, log_fd);
    args->job_chain = addToJobChain(args->job_chain, c);
    struct job* last_job = getLastJob(args->job_chain);

    if (!hasPipe(last_job->command)) {
        // Allow ^z while child owns the terminal if child is not a pipeline
        signal(SIGTSTP, SIG_DFL);
    }
    execute(last_job, args->socket_fd);
    signal(SIGTSTP, SIG_IGN);  // Ignore ^z when yash owns the terminal
    return NULL;
}


void *executeNextWrapper(void *arguments) {
    executeNext(arguments);
    pthread_exit(NULL);
}