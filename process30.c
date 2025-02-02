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
    long limit = 13.8e9;
    for(i=0;i<limit;i++) {}

    // get end time in seconds
    double end_time = clock();
    end_time /= CLOCKS_PER_SEC;

    // compute runtime and print it in the STDOUT/PIPE
    double run_time = end_time - start_time;
    printf("%010.5lf", run_time);
    fflush(NULL);
}