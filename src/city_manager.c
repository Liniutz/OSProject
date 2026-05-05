#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
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


int parse_condition(const char *input, char *field, char *op, char *value) {
    if (sscanf(input, "%31[^:]:%7[^:]:%255[^:]", field, op, value) == 3) {
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
    char *extra_arg = NULL;
    int filter_start = -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (strncmp(argv[i], "--", 2) == 0 && cmd == NULL) {
            cmd = argv[i] + 2; 
            if (i + 1 < argc) {
                district = argv[++i];
            }
            
            if (strcmp(cmd, "view") == 0 || strcmp(cmd, "remove_report") == 0 || strcmp(cmd, "update_threshold") == 0) {
                if (i + 1 < argc) {
                    extra_arg = argv[++i];
                }
            } else if (strcmp(cmd, "filter") == 0) {
                if (i + 1 < argc) {
                    filter_start = i + 1;
                }
                break;
            }
        }
    }

    if (!role || !user || !cmd || !district) {
        fprintf(stderr, "Usage: %s --role <role> --user <user> --<cmd> <district> [args]\n", argv[0]);
        return 1;
    }

    setup_district(district);

    char path[256];
    struct stat st;

    if (strcmp(cmd, "add") == 0) {
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        stat(path, &st);
        
        if (strcmp(role, "inspector") == 0 && !(st.st_mode & S_IWGRP)) {
            fprintf(stderr, "Permission denied: Inspector cannot write to reports.dat\n");
            return 1;
        } else if (strcmp(role, "manager") == 0 && !(st.st_mode & S_IWUSR)) {
            fprintf(stderr, "Permission denied: Manager cannot write to reports.dat\n");
            return 1;
        }

        Report r;
        memset(&r, 0, sizeof(Report));
        r.id = (int)(time(NULL) % 10000);
        strncpy(r.inspector, user, MAX_STR - 1);
        r.severity = 2; 
        r.timestamp = time(NULL);
        strcpy(r.category, "road");
        strcpy(r.description, "Pothole detected on main street.");
        
        int fd = open(path, O_WRONLY | O_APPEND);
        if (fd >= 0) {
            write(fd, &r, sizeof(Report));
            close(fd);
            printf("Successfully added report %d to %s\n", r.id, district);
            try_log_action(district, role, user, "add");
        } else {
            perror("Failed to open reports.dat for writing");
        }

    } else if (strcmp(cmd, "list") == 0) {
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        
        if (stat(path, &st) == 0) {
            if (strcmp(role, "inspector") == 0 && !(st.st_mode & S_IRGRP)) {
                fprintf(stderr, "Permission denied.\n");
                return 1;
            }
            
            char perms[10];
            get_permissions_string(st.st_mode, perms);
            printf("File: %s | Perms: %s | Size: %ld bytes | Last Modified: %ld\n", path, perms, st.st_size, st.st_mtime);
            
            int fd = open(path, O_RDONLY);
            if (fd >= 0) {
                Report r;
                int count = 0;
                while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                    printf("[%d] ID: %d | Category: %s | Inspector: %s | Severity: %d\n", ++count, r.id, r.category, r.inspector, r.severity);
                }
                close(fd);
                if (count == 0) printf("No reports found.\n");
            }
        } else {
            perror("stat failed");
        }
        try_log_action(district, role, user, "list");

    } else if (strcmp(cmd, "view") == 0) {
        if (!extra_arg) {
            fprintf(stderr, "Error: view requires a report ID.\n");
            return 1;
        }
        int target_id = atoi(extra_arg);
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            int found = 0;
            while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                if (r.id == target_id) {
                    printf("--- Report Details ---\n");
                    printf("ID:          %d\n", r.id);
                    printf("Inspector:   %s\n", r.inspector);
                    printf("Category:    %s\n", r.category);
                    printf("Severity:    %d\n", r.severity);
                    printf("Timestamp:   %ld\n", r.timestamp);
                    printf("Description: %s\n", r.description);
                    printf("----------------------\n");
                    found = 1;
                    break;
                }
            }
            close(fd);
            if (!found) printf("Report %d not found.\n", target_id);
        }
        try_log_action(district, role, user, "view");

    } else if (strcmp(cmd, "remove_report") == 0) {
        if (strcmp(role, "manager") != 0) {
            fprintf(stderr, "Permission denied: Only managers can remove reports.\n");
            return 1;
        }
        if (!extra_arg) {
            fprintf(stderr, "Error: remove_report requires a report ID.\n");
            return 1;
        }
        
        int target_id = atoi(extra_arg);
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        
        int fd = open(path, O_RDWR);
        if (fd >= 0) {
            Report r;
            off_t pos = 0;
            int found = 0;
            
            while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                if (r.id == target_id) {
                    found = 1;
                    break;
                }
                pos += sizeof(Report); 
            }
            
            if (found) {
                off_t read_pos = pos + sizeof(Report);
                off_t write_pos = pos;
                
                while (1) {
                    lseek(fd, read_pos, SEEK_SET);
                    int bytes_read = read(fd, &r, sizeof(Report));
                    if (bytes_read <= 0) break; 
                    
                    lseek(fd, write_pos, SEEK_SET);
                    write(fd, &r, sizeof(Report));
                    
                    read_pos += sizeof(Report);
                    write_pos += sizeof(Report);
                }
                
                fstat(fd, &st);
                ftruncate(fd, st.st_size - sizeof(Report));
                printf("Successfully removed report ID %d.\n", target_id);
            } else {
                printf("Report ID %d not found.\n", target_id);
            }
            close(fd);
        }
        try_log_action(district, role, user, "remove_report");

    } else if (strcmp(cmd, "update_threshold") == 0) {
        if (strcmp(role, "manager") != 0) {
            fprintf(stderr, "Permission denied: Manager role required.\n");
            return 1;
        }
        if (!extra_arg) {
            fprintf(stderr, "Error: Missing threshold value.\n");
            return 1;
        }
        
        snprintf(path, sizeof(path), "%s/district.cfg", district);
        stat(path, &st);
        
        if ((st.st_mode & 0777) != 0640) {
            fprintf(stderr, "Diagnostic Warning: Permission bits on district.cfg are not 640! Refusing to write.\n");
            return 1;
        }
        
        int fd = open(path, O_WRONLY | O_TRUNC);
        if (fd >= 0) {
            char buf[32];
            snprintf(buf, sizeof(buf), "THRESHOLD=%s\n", extra_arg);
            write(fd, buf, strlen(buf));
            close(fd);
            printf("Threshold updated to %s.\n", extra_arg);
        }
        try_log_action(district, role, user, "update_threshold");

    } else if (strcmp(cmd, "filter") == 0) {
        snprintf(path, sizeof(path), "%s/reports.dat", district);
        
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            Report r;
            int found_any = 0;
            
            while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
                int matches_all = 1;
                
                if (filter_start != -1) {
                    for (int i = filter_start; i < argc; i++) {
                        char field[32] = {0}, op[8] = {0}, val[256] = {0};
                        
                        if (parse_condition(argv[i], field, op, val)) {
                            if (!match_condition(&r, field, op, val)) {
                                matches_all = 0; 
                                break;
                            }
                        } else {
                            fprintf(stderr, "Warning: failed to parse condition '%s'\n", argv[i]);
                            matches_all = 0;
                            break;
                        }
                    }
                }
                
                if (matches_all) {
                    printf("-> Report %d matches filter! (Category: %s, Sev: %d, Inspector: %s)\n", r.id, r.category, r.severity, r.inspector);
                    found_any = 1;
                }
            }
            close(fd);
            if (!found_any) printf("No reports matched the given conditions.\n");
        }
        try_log_action(district, role, user, "filter");

    } else if (strcmp(cmd, "remove_district") == 0) {
        if (strcmp(role, "manager") != 0) {
            fprintf(stderr, "Permission denied: Only managers can remove districts.\n");
            return 1;
        }

        char symlink_name[256];
        snprintf(symlink_name, sizeof(symlink_name), "active_reports-%s", district);
        unlink(symlink_name);

        pid_t pid = fork();
        if (pid == 0) {
            execlp("rm", "rm", "-rf", district, NULL);
            perror("execlp");
            exit(1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);
            printf("Successfully removed district %s.\n", district);
        } else {
            perror("fork");
        }

    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
    }

    return 0;
}
