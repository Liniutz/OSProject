#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

const char *PID_FILE_NAME = ".monitor_pid";

void handle_sigint(int sig) {
    if (unlink(PID_FILE_NAME) == 0) {
        printf("END: Received SIGINT. Deleted %s and terminating successfully.\n", PID_FILE_NAME);
    } else {
        printf("END: Received SIGINT, but failed to delete %s.\n", PID_FILE_NAME);
    }
    fflush(stdout);
    exit(0);
}

void handle_sigusr1(int sig) {
    printf("MSG: A new report has been added.\n");
    fflush(stdout);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    // Check if monitor is already running
    int fd = open(PID_FILE_NAME, O_RDONLY);
    if (fd >= 0) {
        char pid_str[32];
        int bytes = read(fd, pid_str, sizeof(pid_str) - 1);
        if (bytes > 0) {
            pid_str[bytes] = '\0';
            printf("ERR: Monitor already running with PID %d\n", atoi(pid_str));
            fflush(stdout);
        }
        close(fd);
        exit(1);
    }

    pid_t pid = getpid();
    
    fd = open(PID_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error creating .monitor_pid");
        exit(EXIT_FAILURE);
    }
    
    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", pid);
    write(fd, pid_str, len);
    close(fd);
    
    printf("START: monitor_reports started with PID %d.\n", pid);
    printf("MSG: Waiting for signals (SIGUSR1 to notify, SIGINT to quit)...\n");
    fflush(stdout);

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
