#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_INSPECTORS 100
#define MAX_NAME_LEN 256

typedef struct {
    char name[MAX_NAME_LEN];
    int score;
} InspectorScore;

InspectorScore scores[MAX_INSPECTORS];
int num_inspectors = 0;

void add_score(const char *name, int severity) {
    for (int i = 0; i < num_inspectors; i++) {
        if (strcmp(scores[i].name, name) == 0) {
            scores[i].score += severity;
            return;
        }
    }
    if (num_inspectors < MAX_INSPECTORS) {
        strncpy(scores[num_inspectors].name, name, MAX_NAME_LEN - 1);
        scores[num_inspectors].name[MAX_NAME_LEN - 1] = '\0';
        scores[num_inspectors].score = severity;
        num_inspectors++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <district>\n", argv[0]);
        return 1;
    }

    const char *district = argv[1];
    DIR *dir = opendir(district);
    if (!dir) {
        fprintf(stderr, "District %s not found.\n", district);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (strcmp(entry->d_name, "district.cfg") == 0 || strcmp(entry->d_name, "logged_district") == 0) continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", district, entry->d_name);

        FILE *f = fopen(filepath, "r");
        if (!f) continue;

        char line1[512];
        char line2[512];
        if (fgets(line1, sizeof(line1), f) && fgets(line2, sizeof(line2), f)) {
            char *id = strtok(line2, ",");
            char *inspector = strtok(NULL, ",");
            char *severity_str = strtok(NULL, ",");

            if (inspector && severity_str) {
                add_score(inspector, atoi(severity_str));
            }
        }
        fclose(f);
    }
    closedir(dir);

    for (int i = 0; i < num_inspectors; i++) {
        printf("%s - Inspector %s: %d\n", district, scores[i].name, scores[i].score);
    }

    return 0;
}
