#include <string.h>
#include "settings_highlight.h"

HlSettings hl_settings = {
    .enabled = 1,
};

static int parse_bool(const char *value) {
    return strcmp(value, "true") == 0 ||
           strcmp(value, "on")   == 0 ||
           strcmp(value, "1")    == 0;
}

int hl_settings_set(const char *key, const char *value) {
    if (strcmp(key, "highlight") == 0) {
        hl_settings.enabled = parse_bool(value);
        return 1;
    }
    return 0;
}

int hl_settings_get(const char *key) {
    if (strcmp(key, "highlight") == 0) {
        printf("%s\n", hl_settings.enabled ? "true" : "false");
        return 1;
    }
    return 0;
}

void hl_settings_print_all(void) {
    printf("highlight=%s\n", hl_settings.enabled ? "true" : "false");
}

void hl_settings_write(FILE *f) {
    fprintf(f, "highlight=%s\n", hl_settings.enabled ? "true" : "false");
}
