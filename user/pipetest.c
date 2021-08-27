#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUFFER_SIZE 512  // MUST BE MULTIPLE OF 4 FOR NOW.

// salvaged from grind.c from xv6 source
// from FreeBSD
int do_rand(unsigned long *ctx)
{
    /*
 * Compute x = (7^5 * x) mod (2^31 - 1)
 * without overflowing 31 bits:
 *      (2^31 - 1) = 127773 * (7^5) + 2836
 * From "Random number generators: good ones are hard to find",
 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
 * October 1988, p. 1195.
 */
    long hi, lo, x;

    /* Transform to [1, 0x7ffffffe] range. */
    x = (*ctx % 0x7ffffffe) + 1;
    hi = x / 127773;
    lo = x % 127773;
    x = 16807 * lo - 2836 * hi;
    if (x < 0)
        x += 0x7fffffff;
    /* Transform to [0, 0x7ffffffd] range. */
    x--;
    *ctx = x;
    return (x);
}

unsigned long rand_next = 1503;

int rand(void)
{
    return (do_rand(&rand_next));
}

// End of code copied from grind.c
int main()
{
    int id, pipes[2], counter, temp, start_time;
    counter = 0;
    char buffer[BUFFER_SIZE] /* = "The quick brown fox jumps over the lazy dog." */;

    printf("Creating a pipe...\n");
    if (pipe(pipes) < 0)
    {
        printf("Failed to create a pipe.\n");
        exit(1);
    }
    printf("Forking...\n");
    if ((id = fork()) > 0)
    {
        // Parent
        start_time = uptime();
        while (counter < 10 * 1024 * 1024)
        {
            if ((temp = read(pipes[0], buffer, BUFFER_SIZE)) < 0){
                printf("Failed to read from pipe.\n");
                exit(1);
            }
            // printf("Read %d bytes\n", temp);
            counter += temp;
            // Verify data
            for (int i = 0; i < BUFFER_SIZE; i += 4)
            {
                temp = rand();
                if((temp ^ (buffer[i] | buffer[i+1] << 8 | buffer[i+2] << 16 | buffer[i+3] << 24)) != 0)
                {
                    printf("Read wrong data!\n");
                    exit(1);
                }
            }
            // printf("Verified a %d bytes\n",BUFFER_SIZE);
            // printf("Counter is %d\n", counter);
        }
        start_time = uptime() - start_time;
        printf("Done\n");
        printf("Number of ticks : %d\n", start_time);
        // if (read(pipes[0], buffer, 45) != 45)
        // {
        //     printf("Failed to read 45 bytes\n");
        //     exit(1);
        // }
        // printf("DATA READ: %s\n", buffer);
    }
    else if (id == 0)
    {
        // Child
        while (counter < 10 * 1024 * 1024)
        {
            for (int i = 0; i < BUFFER_SIZE; i += 4)
            {
                temp = rand();
                buffer[i] = temp;
                buffer[i + 1] = temp >> 8;
                buffer[i + 2] = temp >> 16;
                buffer[i + 3] = temp >> 24;
            }
            if (write(pipes[1], buffer, BUFFER_SIZE) != BUFFER_SIZE)
            {
                printf("Failed to write %d bytes!\n", BUFFER_SIZE);
                exit(1);
            }
        }
    }
    else
    {
        printf("Fork failed.\n");
        exit(1);
    }
    // printf("My Id is %d\n", id);

    // printf("Size of int is %d\n", sizeof(int));
    // printf("Measuring time for 10 Mb random number generation\n");
    // printf("Start ticks %d\n", uptime());
    // for (int i = 0; i < 256 * 1024 * 10; i++)
    // {
    //     rand();
    // }
    // printf("End ticks %d\n", uptime());
    exit(0);
}