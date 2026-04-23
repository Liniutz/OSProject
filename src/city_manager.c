#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>

#define MAX_STR 32
#define MAX_DESC 256

typedef struct {
    int id;
    char inspector[MAX_STR];
    double latitude;
    double longitude;
    char category[MAX_STR];
    int severity;
    time_t timestamp;
    char description[MAX_DESC];
} Report;

// --- AI Generated / Assisted Functions ---

int parse_condition(const char *input, char *field, char *op, char *value) {
    if (sscanf(input, "%31[^:]:%7[^:]:%255s", field, op, value) == 3) {
        return 1;
    }
    return 0;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == v;
        if (strcmp(op, "!=") == 0) return r->severity != v;
        if (strcmp(op, "<") == 0) return r->severity < v;
        if (strcmp(op, "<=") == 0) return r->severity <= v;
        if (strcmp(op, ">") == 0) return r->severity > v;
        if (strcmp(op, ">=") == 0) return r->severity >= v;
    } else if (strcmp(field, "timestamp") == 0) {
        time_t v = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == v;
        if (strcmp(op, "!=") == 0) return r->timestamp != v;
        if (strcmp(op, "<") == 0) return r->timestamp < v;
        if (strcmp(op, "<=") == 0) return r->timestamp <= v;
        if (strcmp(op, ">") == 0) return r->timestamp > v;
        if (strcmp(op, ">=") == 0) return r->timestamp >= v;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    }
    return 0;
}

// --- End of AI Assisted Functions ---

void get_permissions_string(mode_t mode, char *buf) {
    strcpy(buf, "---------");
    if (mode & S_IRUSR) buf[0] = 'r';
    if (mode & S_IWUSR) buf[1] = 'w';
    if (mode & S_IXUSR) buf[2] = 'x';
    if (mode & S_IRGRP) buf[3] = 'r';
    if (mode & S_IWGRP) buf[4] = 'w';
    if (mode & S_IXGRP) buf[5] = 'x';
    if (mode & S_IROTH) buf[6] = 'r';
    if (mode & S_IWOTH) buf[7] = 'w';
    if (mode & S_IXOTH) buf[8] = 'x';
}

void try_log_action(const char *district, const char *role, const char *user, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district);
    
    struct stat st;
    if (stat(log_path, &st) == 0) {
        if (strcmp(role, "manager") == 0) {
            if (!(st.st_mode & S_IWUSR)) {
                fprintf(stderr, "Manager does not have write access to log.\n");
                return;
            }
        } else if (strcmp(role, "inspector") == 0) {
            if (!(st.st_mode & S_IWGRP) && !(st.st_mode & S_IWOTH)) {
                fprintf(stderr, "[Permission Denied] Inspector cannot write to operation log!\n");
                return;
            }
        }
    }

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        char buf[512];
        time_t now = time(NULL);
        snprintf(buf, sizeof(buf), "[%ld] Role: %s, User: %s, Action: %s\n", now, role, user, action);
        write(fd, buf, strlen(buf));
        close(fd);
    }
}

void setup_district(const char *district) {
    mkdir(district, 0750);
    chmod(district, 0750);
    
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);
    int fd = open(path, O_CREAT | O_RDWR, 0664);
    if (fd >= 0) close(fd);
    chmod(path, 0664);
    
    snprintf(path, sizeof(path), "%s/district.cfg", district);
    fd = open(path, O_CREAT | O_RDWR, 0640);
    if (fd >= 0) close(fd);
    chmod(path, 0640);
    
    snprintf(path, sizeof(path), "%s/logged_district", district);
    fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd >= 0) close(fd);
    chmod(path, 0644);

    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district);
    unlink(symlink_name);
    symlink(path, symlink_name);
}

int main(int argc, char **argv) {
    char *role = NULL;
    char *user = NULL;
    char *cmd = NULL;
    char *district = NULL;
    
    // Simplistic argument parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) user = argv[++i];
        else if (strncmp(argv[i], "--", 2) == 0 && cmd == NULL) {
            cmd = argv[i] + 2;
            if (i + 1 < argc) district = argv[++i];
        }
    }

    if (!role || !user || !cmd || !district) {
        fprintf(stderr, "Usage: %s --role <role> --user <user> --<cmd> <district> [args]\n", argv[0]);
        return 1;
    }

    setup_district(district);

    if (strcmp(cmd, "add") == 0) {
        char path[256];
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        struct stat st;
        stat(path, &st);
        if (strcmp(role, "inspector") == 0 && !(st.st_mode & S_IWGRP)) {
            fprintf(stderr, "Permission denied.\n");
            return 1;
        }

        Report r = {0};
        r.id = time(NULL) % 10000;
        strncpy(r.inspector, user, MAX_STR - 1);
        r.severity = 2; // Default mock values
        r.timestamp = time(NULL);
        strcpy(r.category, "road");
        
        int fd = open(path, O_WRONLY | O_APPEND);
        if (fd >= 0) {
            write(fd, &r, sizeof(Report));
            close(fd);
            try_log_action(district, role, user, "add");
        }
    } else if (strcmp(cmd, "list") == 0) {
        char path[256];
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        struct stat st;
        if (stat(path, &st) == 0) {
            char perms[10];
            get_permissions_string(st.st_mode, perms);
            printf("File: %s | Perms: %s | Size: %ld | MTime: %ld\n", path, perms, st.st_size, st.st_mtime);
            
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                Report r;
                while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                    printf("Report %d | %s | %s | Sev: %d\n", r.id, r.category, r.inspector, r.severity);
                }
                close(fd);
            }
        }
        try_log_action(district, role, user, "list");
    } else if (strcmp(cmd, "filter") == 0) {
        char path[256];
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                // Check all conditions in remaining argv
                // Logic simplifed for mock
                printf("Matching Report %d\n", r.id);
            }
            close(fd);
        }
        try_log_action(district, role, user, "filter");
    }

    return 0;
}
