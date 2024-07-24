#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

enum Type { T15, T30 };

struct Process {
    int id;
    enum Type type;
};

#define cores 2
#define num_processes 6

// variable used as semaphore, if available_cores == 0 => block
int available_cores = cores;

pid_t child_pid[cores];
int child_id[cores];

void print_time() {
    double time = clock();
    time /= CLOCKS_PER_SEC;
    printf("[%010.6lf] ", time);
}

void busy_waiting() {
    for(int j=0; j<cores; j++) if (child_pid[j]) {

        int status;
        pid_t result = waitpid(child_pid[j], &status, WNOHANG);

        if (result > 0) { // sucessful exit
            print_time();
            printf("+ (pid %d) {id %d} Exited with status %d\n", child_pid[j], child_id[j], WEXITSTATUS(status));
            
            // release a core
            available_cores += 1;
            child_pid[j] = 0;
            break;
        }

        else if (result == -1) { // error in exit
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        // else, the process is still running
    }
}

int main() {
    
    // initialize array to store children pids with value 0 (not used)
    memset(child_pid, 0, sizeof(child_pid));

    struct Process queue[num_processes] = {
        // hard-coded testcase
        {0, T15}, {1, T30}, {2, T15}, {3, T15}, {4, T30}, {5, T15}
    };

    // get start time in seconds
    double start_time = clock();
    start_time /= CLOCKS_PER_SEC; 

    // execute processes
    for(int i = 0; i < num_processes; i++) {
        
        // if there are no cores available, block and wait for any child_pid to terminate
        // Block using busy waiting!
        while(available_cores == 0) {
            busy_waiting();
        }

        // allocate a core
        available_cores -= 1;

        // fork main process into two
        pid_t pid = fork();

        enum Type type = queue[i].type;
        int id = queue[i].id;

        if (pid == 0) { // child_pid process

            if (type == T15) {
                char *args[] = {"./process15", NULL};
                execvp(args[0], args);
            }

            else if (type == T30) {
                char *args[] = {"./process30", NULL};
                execvp(args[0], args);
            }
            
            // exec usually doesn't return, if it does, raise error:
            perror("execvp");
            exit(EXIT_FAILURE);
        }

        else if (pid > 0) { // parent process
            child_pid[i % cores] = pid; 
            child_id[i % cores] = id;

            print_time();
            printf("/ (pid %d) forked (pid %d)\n", getpid(), pid);

            print_time();
            printf("- (pid %d) {id %d} Execute {type %d}\n", pid, id, type);
        }

        else { // (pid < 0) => error
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // block main process until all children exit and cores are released
    while(available_cores < cores) {
        busy_waiting();
    }

    // get end time in seconds
    double end_time = clock();
    end_time /= CLOCKS_PER_SEC;

    // compute makespan and print it
    double run_time = end_time - start_time;
    printf("Makespan time: %.6lf seconds \n", run_time);
}