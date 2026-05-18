#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

pid_t hub_mon_pid = -1;

void handle_start_monitor() {
    if (hub_mon_pid > 0) {
        printf("City Hub: Monitor is already started (PID %d)\n", hub_mon_pid);
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(pipefd[1]);

        execl("./monitor_reports_output", "monitor_reports_output", (char*)NULL);
        perror("execl monitor_reports_output");
        exit(1);
    } else {
        close(pipefd[1]);
        
        hub_mon_pid = pid;
        pid_t reader_pid = fork();
        if (reader_pid == 0) {
            FILE *f = fdopen(pipefd[0], "r");
            if (!f) {
                perror("fdopen");
                exit(1);
            }
            char line[MAX_LINE];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "ERR:", 4) == 0) {
                    printf("\n[HUB_MON ERROR] %s", line + 4);
                    break;
                } else if (strncmp(line, "END:", 4) == 0) {
                    printf("\n[HUB_MON END] %s", line + 4);
                    break;
                } else if (strncmp(line, "START:", 6) == 0) {
                    printf("\n[HUB_MON START] %s", line + 6);
                } else if (strncmp(line, "MSG:", 4) == 0) {
                    printf("\n[HUB_MON MSG] %s", line + 4);
                } else {
                    printf("\n[HUB_MON] %s", line);
                }
            }
            fclose(f);
            exit(0);
        } else {
            close(pipefd[0]);
        }
    }
}

void handle_calculate_scores(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: calculate_scores <district1> <district2> ...\n");
        return;
    }

    int num_districts = argc - 1;
    int **pipes = malloc(num_districts * sizeof(int *));
    pid_t *pids = malloc(num_districts * sizeof(pid_t));

    for (int i = 0; i < num_districts; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            continue;
        }

        pids[i] = fork();
        if (pids[i] == 0) {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);

            execl("./scorer_output", "scorer_output", argv[i + 1], (char *)NULL);
            perror("execl scorer_output");
            exit(1);
        } else {
            close(pipes[i][1]);
        }
    }

    printf("\n--- Workload Scores ---\n");
    for (int i = 0; i < num_districts; i++) {
        char buf[MAX_LINE];
        ssize_t bytes;
        while ((bytes = read(pipes[i][0], buf, sizeof(buf) - 1)) > 0) {
            buf[bytes] = '\0';
            printf("%s", buf);
        }
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);
        free(pipes[i]);
    }
    printf("------------------------\n\n");
    
    free(pipes);
    free(pids);
}

int main() {
    char input[MAX_LINE];
    char *args[MAX_ARGS];

    printf("City Hub Interactive CLI\n");
    printf("Available commands: start_monitor, calculate_scores <districts...>, exit\n");

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        int argc = 0;
        char *token = strtok(input, " \t\n");
        while (token && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[argc] = NULL;

        if (argc == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            if (hub_mon_pid > 0) {
                kill(hub_mon_pid, SIGINT);
            }
            break;
        } else if (strcmp(args[0], "start_monitor") == 0) {
            handle_start_monitor();
        } else if (strcmp(args[0], "calculate_scores") == 0) {
            handle_calculate_scores(argc, args);
        } else {
            printf("Unknown command: %s\n", args[0]);
        }
    }

    return 0;
}
