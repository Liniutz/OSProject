#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

const char *PID_FILE_NAME = ".monitor_pid";

void handle_sigint(int sig) {
    if (unlink(PID_FILE_NAME) == 0) {
        printf("\nReceived SIGINT. Deleted %s and terminating successfully.\n", PID_FILE_NAME);
    } else {
        printf("\nReceived SIGINT, but failed to delete %s.\n", PID_FILE_NAME);
    }
    exit(0);
}


void handle_sigusr1(int sig) {
    printf("A new report has been added.\n");
}

int main(void) {
    pid_t pid = getpid();
    
    int fd = open(PID_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating .monitor_pid");
        exit(EXIT_FAILURE);
    }
    
    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", pid);
    write(fd, pid_str, len);
    close(fd);
    
    printf("monitor_reports started with PID %d.\n", pid);
    printf("Waiting for signals (SIGUSR1 to notify, SIGINT to quit)...\n");

    struct sigaction sa_int, sa_usr1;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0; 
    sigaction(SIGUSR1, &sa_usr1, NULL);

    while(1) {
        pause();
    }

    return 0;
}
