#include <string.h>
#include "settings_git.h"

GitSettings git_settings = {
    .enabled = 1,
};

static int parse_bool(const char *value) {
    return strcmp(value, "true") == 0 ||
           strcmp(value, "on")   == 0 ||
           strcmp(value, "1")    == 0;
}

int git_settings_set(const char *key, const char *value) {
    if (strcmp(key, "git_gutter") == 0) {
        git_settings.enabled = parse_bool(value);
        return 1;
    }
    return 0;
}

int git_settings_get(const char *key) {
    if (strcmp(key, "git_gutter") == 0) {
        printf("%s\n", git_settings.enabled ? "true" : "false");
        return 1;
    }
    return 0;
}

void git_settings_print_all(void) {
    printf("git_gutter=%s\n", git_settings.enabled ? "true" : "false");
}

void git_settings_write(FILE *f) {
    fprintf(f, "git_gutter=%s\n", git_settings.enabled ? "true" : "false");
}
