#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RECEIVE_BUFFER_SIZE 2048

unsigned long rand_next = 1503;

// Pseudo-RNG Taken from https://en.wikipedia.org/wiki/Xorshift
int do_rand(unsigned long *ctx)
{
    /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
    long x = *ctx;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *ctx = x;
}

// Keep calling this after setting seed to generate random 32-bit numbers.
int rand(void)
{
    return (do_rand(&rand_next));
}

// Wait for child and exit with given code if successful or exit code 1 if wait fails.
void wait_for_child_and_exit(int child_pid, int exit_code)
{
    printf("[Parent]: Waiting for child to exit.\n");
    if (wait(&child_pid) < 0)
    {
        printf("[Parent]: ERROR: Wait for child failed.\n");
        exit(1);
    }
    printf("[Parent]: Goodbye world.\n");
    exit(exit_code);
}

int main(int argc, char **argv)
{
    int send_size = 0, temp = 0, fail = 0;
    int proc_id, pipe_fd[2], counter, start_time;
    char *receive_buffer;
    char *send_buffer;
    counter = 0;

    if (argc < 2)
    {
        printf("Assuming a default send size of 10,000,000 bytes.\n");
        send_size = 10000000;
    }
    else if (argc == 2)
    {
        for (int i = 0; i < strlen(argv[1]); i++)
        {
            if (argv[1][i] > '9' || argv[1][i] < '0')
            {
                printf("Usage ./pipetest <number of bytes>\n");
                exit(1);
            }
        }
        send_size = atoi(argv[1]);
        printf("Sending %d bytes.\n", send_size);
    }
    else
    {
        printf("Usage ./pipetest <number of bytes>\n");
        exit(1);
    }
    rand_next = uptime() + getpid(); // Pseudo-random seed for the pseudo RNG;

    printf("Generating %d bytes data.\n", send_size);
    send_buffer = (char *)malloc(send_size);
    start_time = uptime();
    for (int i = 0; i < send_size; i++)
    {
        if (i % 4 == 0)
            temp = rand(); // Generate next 4 random bytes.
        send_buffer[i] = temp >> (8 * (i % 4));
    }
    printf("Data generated. Time taken: %d ticks\n", uptime() - start_time);
    ;

    // Uncomment this block to print some data to verify that it is random.
    // printf("Data samples:\n");
    // for(int i=0; i < send_size / 1000 ; i += send_size / 1000)
    // {
    //     for(int j=i; j < (send_size / 1000) * (i+1) ;j+=100)
    //     {
    //         printf("%d ",send_buffer[j]);
    //     }
    //     printf("\n");
    // }

    printf("Creating a pipe...\n");
    if (pipe(pipe_fd) < 0)
    {
        printf("Failed to create a pipe.\n");
        exit(1);
    }

    printf("Forking...\n");
    if ((proc_id = fork()) > 0)
    {
        // Parent
        if (close(pipe_fd[1]) < 0)
        {
            printf("[Parent]: ERROR. Failed to close write end of the pipe after fork.");
            wait_for_child_and_exit(proc_id, 1);
        }
        receive_buffer = (char *)malloc(RECEIVE_BUFFER_SIZE); // receive in chunks of RECEIVE_BUFFER_SIZE
        start_time = uptime();
        while ((temp = read(pipe_fd[0], receive_buffer, RECEIVE_BUFFER_SIZE)) != 0)
        {
            if (temp < 0)
            {
                printf("[Parent]: ERROR. Failed to read from pipe.\n");
                wait_for_child_and_exit(proc_id, 1);
            }
            // Verify chunk
            for (int i = 0; i < temp; i++)
            {
                if (receive_buffer[i] != send_buffer[counter + i])
                {
                    fail = 1;
                    printf("Byte %d is %d, expected %d\n", counter + i, receive_buffer[i], send_buffer[counter + i]);
                }
            }
            counter += temp;
        }
        start_time = uptime() - start_time;
        if (fail)
        {
            printf("[Parent]: ERROR. Wrong data received!\n");
            wait_for_child_and_exit(proc_id, 1);
        }
        printf("[Parent]: Received and verified %d bytes.\n", counter);
        if (counter != send_size)
        {
            printf("[Parent]: ERROR. Pipe dropped some bytes!\n");
            wait_for_child_and_exit(proc_id, 1);
        }
        printf("[Parent]: Number of ticks : %d\n", start_time);
        wait_for_child_and_exit(proc_id, 0);
    }
    else if (proc_id == 0)
    {
        // Child
        // Uncomment below line to intentionally change a byte to check if data verification is working.
        // send_buffer[send_size/2] = send_buffer[send_size/2] ^ send_buffer[send_size/2] >> 2;
        if (close(pipe_fd[0]) < 0)
        {
            printf("[Child]: Failed to close read end of the pipe after fork.");
            exit(1);
        }
        while (counter < send_size)
        {
            if ((temp = write(pipe_fd[1], &send_buffer[counter], (send_size - counter))) < 0)
            {
                printf("[Child]: Failed to write to pipe!");
                exit(1);
            }
            counter += temp;
        }
        if (close(pipe_fd[1]) < 0)
        {
            printf("[Child]: ERROR. Failed to close write end of the pipe after writing.");
            exit(1);
        }
    }
    else
    {
        //Fork Failure.
        printf("Fork failed.\n");
        exit(1);
    }
    exit(0);
}