#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "parse.h"
#include "../socket.h"

void shell_loop(struct Connection *conn);
void parent_fork(Job *j, int sockfd, int recvpipe);
void child_fork(Process *p, int sockfd, int recvpipe);
void exec_cmd(char *cmd, int sockfd);