#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

// yjr ept;f djpi; hp d[sdyov
// Try decode that lol
#include "producer.h"

#define PIPE_NAME "/tmp/pipetestlol"

int main() {
    int pipe_fd;
    char buffer[128];

    pthread_t thread;

    mkfifo(PIPE_NAME, 0666);

    pthread_create(&thread, NULL, producer, NULL);


    // Open the pipe for reading
    pipe_fd = open(PIPE_NAME, O_RDONLY);
    if (pipe_fd == -1) {
        perror("open");
        return 1;
    }

    // Read data from the pipe
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        read(pipe_fd, buffer, sizeof(buffer));
        if(buffer[0] == 'q') {
            printf("consumer quit\n");
            break;
        }
        printf("Consumer read: %s\n", buffer);
    }
    pthread_join(thread, NULL);
    // Close the pipe
    close(pipe_fd);

    return 0;
}
