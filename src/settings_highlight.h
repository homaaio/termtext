#ifndef SETTINGS_HIGHLIGHT_H
#define SETTINGS_HIGHLIGHT_H

#include <stdio.h>

/*
 * Settings for the syntax highlighting module — a separate settings
 * file, compiled only when FEATURE_HIGHLIGHT=1 (see config.h and
 * Makefile), so that disabling the module also drops its settings code
 * from the binary.
 */

typedef struct {
    int enabled; /* on/off for syntax highlighting */
} HlSettings;

extern HlSettings hl_settings;

int  hl_settings_set(const char *key, const char *value);
int  hl_settings_get(const char *key);
void hl_settings_print_all(void);
void hl_settings_write(FILE *f);

#endif
