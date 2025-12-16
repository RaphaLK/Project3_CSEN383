#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#include "project6.h"

int main(int argc, char argv[])
{
	int childfds[TOTAL_CHILD_PROCESSES];
	pid_t child_pids[TOTAL_CHILD_PROCESSES];
	struct timeval start, now;
	gettimeofday(&start, NULL);

	FILE *output = fopen("output.txt", "w");

	if (output == NULL)
	{
		fprintf(stderr, "Failed to open output.txt\n");
		return -1;
	}

	for (int i = 1; i <= TOTAL_CHILD_PROCESSES; i++)
	{
		int pipefds[2];
		int cpid;

		if (pipe(pipefds) == -1)
		{
			fprintf(stderr, "pipe failed\n");
			return -1;
		}

		cpid = fork();
		if (cpid < 0)
		{
			/* error */
			fprintf(stderr, "fork failed\n");
			return -1;
		}
		else if (cpid == 0)
		{
			/* child */
			close(pipefds[READ_END]);
			run_child_process(i, pipefds[WRITE_END]);
		}
		else
		{
			/* parent */
			close(pipefds[WRITE_END]);
			childfds[i - 1] = pipefds[READ_END];
			child_pids[i - 1] = cpid; 
		}
	}

	/* read from all pipes using select() */
	int active_processes = TOTAL_CHILD_PROCESSES;
	char buffer[BUFFER_SIZE];

	while (active_processes > 0)
	{
		gettimeofday(&now, NULL);
		double parent_lifetime = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000000.0;

		// check runtime
		if (parent_lifetime >= RUNTIME)
		{
			fprintf(stderr, "Terminate process after 30s\n");
			// Kill all child processes at 30s
			for (int i = 0; i < TOTAL_CHILD_PROCESSES; i++)
			{
				kill(child_pids[i], SIGTERM);
			}
			break;
		}

		fd_set readfds;
		FD_ZERO(&readfds);

		int max_fd = -1;
		for (int i = 0; i < TOTAL_CHILD_PROCESSES; i++)
		{
			if (childfds[i] != -1)
			{
				FD_SET(childfds[i], &readfds);
				if (childfds[i] > max_fd)
				{
					max_fd = childfds[i];
				}
			}
		}

		if (max_fd == -1)
		{
			break;
		}

		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int ready = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

		if (ready < 0)
		{
			fprintf(stderr, "select failed\n");
			break;
		}
		else if (ready == 0)
		{
			continue;
		}

		for (int i = 0; i < TOTAL_CHILD_PROCESSES; i++)
		{
			if (childfds[i] != -1 && FD_ISSET(childfds[i], &readfds))
			{
				ssize_t bytes_read = read(childfds[i], buffer, BUFFER_SIZE - 1);

				if (bytes_read > 0)
				{
					buffer[bytes_read] = '\0';

					gettimeofday(&now, NULL);
					int sec = (int)(now.tv_sec - start.tv_sec);
					double msec = (now.tv_usec - start.tv_usec) / 1000.0;

					fprintf(output, "%d:%06.3f: %s", sec, msec, buffer);
					fflush(output);
				}
				else if (bytes_read == 0)
				{
					/* pipe closed */
					close(childfds[i]);
					childfds[i] = -1;
					active_processes--;
				}
			}
		}
	}

	fclose(output);

	/* Wait for all child processes to terminate */
	for (int i = 0; i < TOTAL_CHILD_PROCESSES; i++)
	{
		wait(NULL);
	}

	return 0;
}