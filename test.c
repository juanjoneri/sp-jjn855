#include "job.h"

int main() {
    struct command* c1 = parseCommand("./infloop 1 &");
    struct command* c2 = parseCommand("./infloop 2 &");

    struct job* job_chain = addToJobChain(NULL, c1);
    job_chain = removeTerminated(job_chain);
    job_chain = addToJobChain(job_chain, c2);
    job_chain = removeTerminated(job_chain);
    
    printJobChain(job_chain);

    // getLastJob(job_chain)->state = TERMINATED;
    // getLastJob(job_chain)->state = TERMINATED;
    // job_chain = removeTerminated(job_chain);
    // job_chain = addToJobChain(job_chain, c2);
    // printJobChain(job_chain);

    return 0;
}