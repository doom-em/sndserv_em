#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PIPE_NAME "/tmp/pipetestlol"

void* producer(void* arg) {
    int pipe_fd;
    char buffer[10];

    // Create the pipe (only if it doesn't exist)
    mkfifo(PIPE_NAME, 0666);


    // Open the pipe for writing
    pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd == -1) {
        perror("open");
        return NULL;
    }
    while (1) {
        if(buffer[0] == 'q') {
            printf("producer quit\n");
            break;
        }
        memset(buffer, 0, sizeof(buffer));

        printf("Enter a message (q for quit): ");
        fflush(stdout); // Ensure the prompt is shown before reading
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("fgets");
            close(pipe_fd);
            return NULL;
        }


        // Write data to the pipe
        write(pipe_fd, buffer, strlen(buffer) + 1);
        printf("Producer wrote: %s\n", buffer);
    }
    // Close the pipe
    close(pipe_fd);

    return NULL;
}
