#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "config.h"
#include "settings.h"
#if FEATURE_HIGHLIGHT
#include "settings_highlight.h"
#endif

#ifdef _WIN32
#define MKDIR(path) mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0755)
#endif

Settings settings = {
    .tab_width = 4,
    .smart_tab = 1,
    .show_lines = 1,
    .cursor_blink = 1,
};

static void trim(char *s) {
    int n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' ||
                     s[n-1] == ' '  || s[n-1] == '\t')) {
        s[--n] = 0;
    }
}

static int build_config_path(char *out, int max) {
    const char *base = getenv("XDG_CONFIG_HOME");
    if (base == NULL || base[0] == 0) {
        const char *home = getenv("HOME");
#ifdef _WIN32
        if (home == NULL) home = getenv("USERPROFILE");
#endif
        if (home == NULL) return 0;
        snprintf(out, max, "%s/.config/tt", home);
    } else {
        snprintf(out, max, "%s/tt", base);
    }
    return 1;
}

static int mkdir_p(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    int len = strlen(tmp);
    if (len == 0) return 0;
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            MKDIR(tmp);
            *p = '/';
        }
    }
    if (MKDIR(tmp) != 0 && errno != EEXIST) return 0;
    return 1;
}

int settings_load(void) {
    char dir[512];
    if (!build_config_path(dir, sizeof(dir))) return 0;

    char path[600];
    snprintf(path, sizeof(path), "%s/settings.conf", dir);

    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (eq == NULL) continue;
        *eq = 0;
        char *key = line;
        char *val = eq + 1;
        trim(key);
        trim(val);
        settings_set(key, val);
    }
    fclose(f);
    return 1;
}

int settings_save(void) {
    char dir[512];
    if (!build_config_path(dir, sizeof(dir))) return 0;

    mkdir_p(dir);

    char path[600];
    snprintf(path, sizeof(path), "%s/settings.conf", dir);

    FILE *f = fopen(path, "w");
    if (f == NULL) return 0;
    fprintf(f, "tab_width=%d\n", settings.tab_width);
    fprintf(f, "smart_tab=%s\n", settings.smart_tab ? "true" : "false");
    fprintf(f, "lines=%s\n", settings.show_lines ? "true" : "false");
    fprintf(f, "cursor_blink=%s\n", settings.cursor_blink ? "true" : "false");
#if FEATURE_HIGHLIGHT
    hl_settings_write(f);
#endif
    fclose(f);
    return 1;
}

int settings_set(const char *key, const char *value) {
    if (strcmp(key, "tab_width") == 0) {
        int n = atoi(value);
        if (n < 1) n = 1;
        if (n > 16) n = 16;
        settings.tab_width = n;
        return 1;
    }
    if (strcmp(key, "smart_tab") == 0) {
        settings.smart_tab = (strcmp(value, "true") == 0 ||
                              strcmp(value, "on")   == 0 ||
                              strcmp(value, "1")    == 0);
        return 1;
    }
    if (strcmp(key, "lines") == 0) {
        settings.show_lines = (strcmp(value, "true") == 0 ||
                               strcmp(value, "on")   == 0 ||
                               strcmp(value, "1")    == 0);
        return 1;
    }
    if (strcmp(key, "cursor_blink") == 0) {
        settings.cursor_blink = (strcmp(value, "true") == 0 ||
                                 strcmp(value, "on")   == 0 ||
                                 strcmp(value, "1")    == 0);
        return 1;
    }
#if FEATURE_HIGHLIGHT
    if (hl_settings_set(key, value)) return 1;
#endif
    return 0;
}

int settings_get(const char *key) {
    if (strcmp(key, "tab_width") == 0) {
        printf("%d\n", settings.tab_width);
        return 1;
    }
    if (strcmp(key, "smart_tab") == 0) {
        printf("%s\n", settings.smart_tab ? "true" : "false");
        return 1;
    }
    if (strcmp(key, "lines") == 0) {
        printf("%s\n", settings.show_lines ? "true" : "false");
        return 1;
    }
    if (strcmp(key, "cursor_blink") == 0) {
        printf("%s\n", settings.cursor_blink ? "true" : "false");
        return 1;
    }
#if FEATURE_HIGHLIGHT
    if (hl_settings_get(key)) return 1;
#endif
    return 0;
}

void settings_print_all(void) {
    printf("tab_width=%d\n", settings.tab_width);
    printf("smart_tab=%s\n", settings.smart_tab ? "true" : "false");
    printf("lines=%s\n", settings.show_lines ? "true" : "false");
    printf("cursor_blink=%s\n", settings.cursor_blink ? "true" : "false");
#if FEATURE_HIGHLIGHT
    hl_settings_print_all();
#endif
}
