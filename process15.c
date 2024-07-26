#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    
    // get start time in seconds
    double start_time = clock();
    start_time /= CLOCKS_PER_SEC; 

    // simulate a real process
    long int i = 0;
    long limit = 6.95e10;
    for(i=0;i<limit;i++) {}
    
    // get end time in seconds
    double end_time = clock();
    end_time /= CLOCKS_PER_SEC;

    // compute runtime and print it
    double run_time = end_time - start_time;
    printf("Time Elapsed for Process (pid %d): %.6lf \n", getpid(), run_time);
}