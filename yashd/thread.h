#ifndef THREAD_H
#define THREAD_H

#include <semaphore.h>

#include "socket.h"

extern FILE *logfd;

void *thread_exec(void *arg);

#endif