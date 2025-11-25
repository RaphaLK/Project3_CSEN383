#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#include "project6.h"

void run_child_process(int process_num, int write_fd) {
    struct timeval start, now;
    gettimeofday(&start, NULL);

    int total_messages = 0;
    
    if (process_num != 5) {
        srand(getpid());
    }

    char line[BUFFER_SIZE];

    while (1) {
      gettimeofday(&now, NULL);
      double elapsed = (now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec) / 1000000.0;

      if (elapsed >= RUNTIME) {
        break;
      }

      if (process_num == 5) {
        printf("Enter input: ");
        fflush(stdout);

        if (fgets(line, BUFFER_SIZE, stdin) == NULL) {
          break;
        }
        // trailing newline convert to null terminator
        line[strcspn(line, "\n")] = '\0';
      } else {
        // sleep between 0-2s
        sleep(rand() % SLEEP_TIME);
      }

      gettimeofday(&now, NULL);
      int sec = (int)(now.tv_sec - start.tv_sec);
      double msec = (now.tv_usec - start.tv_usec) / 1000.0;

      char message[BUFFER_SIZE * 2]; // 2x, message can be 256 + 1 byte (null term)
      ++total_messages;
      if (process_num == 5) {
        snprintf(message, sizeof(message), 
                "%d:%06.3f: Child %d: #%d text msg from the terminal: %s\n", 
                sec, msec, process_num, total_messages, line);
      } else {
        snprintf(message, sizeof(message),
                "%d:%06.3f: Child %d message %d\n",
                sec, msec, process_num, total_messages);
      }

      write(write_fd, message, strlen(message));
    }

    close(write_fd);
    exit(0);
}
