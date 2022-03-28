#include "yash_processor.h"

int main() {
    // Ignore ^Z in yash
    signal(SIGTSTP, SIG_IGN);

    char* line;
    struct job* job_chain = NULL;

    while (line = prompt()) {
        job_chain = executeNext(line, job_chain);
        free(line);
    }

    freeJobChain(job_chain);
    return 0;
}
