#ifndef PARSE_H
#define PARSE_H

#include "job.h"
#include "../socket.h"
#include "../thread.h"


Process *malloc_argv(char cmd[MAXLINE]);
Job *parse_command(char cmd[MAXLINE]);
int file_redirection(Process *p);
int word_count(char cmd[MAXLINE]);
void free_argv(int nchild, int argc1, char **argv1, int argc2, char **argv2);

#endif