#include "thread.h"
#include "yash-holden/yash_cmd.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/* Strategy:
    * Ignore signals
    * get connection info from void *arg
    * log new connection
    * call shell_loop()
        * all within yash-holden yash_cmd.c
*/
void *thread_exec(void *arg) {
    // job_table, orig_cmd, cmd, pipefd, next_job
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    struct Connection *conn = arg;

    // fprintf(logfd, "Thread connected to %s:%d\n",
    //     inet_ntoa(conn->connaddr.sin_addr), ntohs(conn->connaddr.sin_port));
    fflush(logfd);


    shell_loop(conn);

    return NULL;
}