#include <unistd.h>
#include <stdio.h>

#include "project6.h"

int main(int argc, char argv[]) {
    int childfds[TOTAL_CHILD_PROCESSES];

    for (int i = 1; i <= TOTAL_CHILD_PROCESSES; i++) {
        int pipefds[2];
	int cpid;

	if (pipe(pipefds) == -1) {
	    fprintf(stderr, "pipe failed\n");
	    return -1;
	}
	
	cpid = fork();
	if (cpid < 0) {
	    /* error */
	    fprintf(stderr, "fork failed\n");
	    return -1;
	} else if (cpid == 0) {
	    /* child */
	    close(pipefds[READ_END]);
	    run_child_process(i, pipefds[WRITE_END]);
	} else {
	    /* parent */
	    close(pipefds[WRITE_END]);
	    childfds[i-1] = pipefds[READ_END];
	}
    }
}
