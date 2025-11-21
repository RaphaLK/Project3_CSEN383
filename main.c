#include <unistd.h>

#include "project6.h"

int main(int argc, char argv[]) {
    int childfds[TOTAL_CHILD_PROCESSES];

    for (int i = 1; i <= TOTAL_CHILD_PROCESSES; i++) {
        int pipefds[2];
	int cpid;

	pipe(pipefd);
	cpid = fork();
	
	if (cpid == 0) {
	    /* child */
	    close(pipefd[READ_END]);
	    run_child_process(i, pipefd[WRITE_END]);
	} else {
	    /* parent */
	    close(pipefd[WRITE_END]);
	    childfds[i-1] = pipefd[READ_END];
	}
    }
}
