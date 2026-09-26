#include <string.h>
#include "settings_brackets.h"

BracketSettings br_settings = {
    .enabled = 1,
};

static int parse_bool(const char *value) {
    return strcmp(value, "true") == 0 ||
           strcmp(value, "on")   == 0 ||
           strcmp(value, "1")    == 0;
}

int br_settings_set(const char *key, const char *value) {
    if (strcmp(key, "smart_brackets") == 0) {
        br_settings.enabled = parse_bool(value);
        return 1;
    }
    return 0;
}

int br_settings_get(const char *key) {
    if (strcmp(key, "smart_brackets") == 0) {
        printf("%s\n", br_settings.enabled ? "true" : "false");
        return 1;
    }
    return 0;
}

void br_settings_print_all(void) {
    printf("smart_brackets=%s\n", br_settings.enabled ? "true" : "false");
}

void br_settings_write(FILE *f) {
    fprintf(f, "smart_brackets=%s\n", br_settings.enabled ? "true" : "false");
}
