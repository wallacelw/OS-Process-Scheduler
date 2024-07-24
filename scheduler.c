#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == -1) { // Error
        perror("fork");

        exit(EXIT_FAILURE);
    }
    else if (pid == 0) { // Child Process
        
        char *args[] = {"./process30", NULL}; // Arguments for the new program
        execvp(args[0], args);

        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else { // Parent Process
        int status;
        printf("Process with pid %d forked another process with pid %d \n", getpid(), pid);
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Child process (%d) exited with status %d\n", pid, WEXITSTATUS(status));
        } 
        else {
            printf("Child process did not exit normally\n");
        }
    }
}