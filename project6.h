#define RUNTIME 30
#define BUFFER_SIZE 256
#define TOTAL_CHILD_PROCESSES 5
#define SLEEP_TIME 3

#define WRITE_END 1
#define READ_END 0

void run_child_process(int process_num, int write_fd);
