#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <semaphore.h>

enum Type { T15, T30 };

struct Process {
    int id;
    enum Type type;
};

#define cores 2
#define num_processes 6

int main() {
    
    // array to store children pids
    pid_t child[cores];

    struct Process queue[num_processes] = {
        // hard-coded testcase
        {0, T15}, {1, T30}, {2, T15}, {3, T15}, {4, T30}, {5, T15}
    };

    // semaphore used to know if I can run a process now
    sem_t available_cores;
    sem_init(&available_cores, 1, cores);

    // execute processes
    int sem_val = cores;
    for(int i = 0; i < num_processes; i++) {
        
        // if there are no cores available, block and wait for any child to terminate
        sem_getvalue(&available_cores, &sem_val);
        while(sem_val == 0) {
            for(int j=0; j<cores; j++) {
                int status;
                pid_t result = waitpid(child[j], &status, WNOHANG);

                if (result == -1) {
                    perror("waitpid");
                    exit(EXIT_FAILURE);
                }
                else if (result > 0) { // sucessful exit
                    printf("Child process with (pid %d) exited with status %d\n", child[j], WEXITSTATUS(status));
                    // release a core
                    sem_post(&available_cores);
                    child[j] = 0;
                    break;
                }
                // else, the process is still running
            }
            sem_getvalue(&available_cores, &sem_val);
        }
        
        // allocate a core, if none are available, block
        sem_wait(&available_cores);

        // fork main process into two
        pid_t pid = fork();

        if (pid == 0) { // child process
            enum Type type = queue[i].type;

            if (type == T15) {
                char *args[] = {"./process15", NULL};
                execvp(args[0], args);
            }
            else if (type == T30) {
                char *args[] = {"./process30", NULL};
                execvp(args[0], args);
            }
            
            // exec doesn't return, if it does, error:
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0) { // parent process
            child[i % cores] = pid; 
            int id = queue[i].id;

            printf("Process with (pid %d) forked another process with (pid %d) and {id %d} \n", getpid(), pid, id);
        }
        else { // (pid < 0) => error
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // block main process until all children are released
    sem_getvalue(&available_cores, &sem_val);
    while(sem_val < cores) {
        for(int j=0; j<cores; j++) {
            int status;
            pid_t result = waitpid(child[j], &status, WNOHANG);

            if (result == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            else if (result > 0) { // sucessful exit
                printf("Child process with (pid %d) exited with status %d\n", child[j], WEXITSTATUS(status));
                // release a core
                sem_post(&available_cores);
                child[j] = 0;
                break;
            }
            // else, the process is still running
        }
        sem_getvalue(&available_cores, &sem_val);
    }
}