#ifndef SETTINGS_BRACKETS_H
#define SETTINGS_BRACKETS_H

#include <stdio.h>

/*
 * Settings for the smart-brackets module (auto-closing pairs and
 * matching-bracket highlighting) — a separate settings file, compiled
 * only when FEATURE_BRACKETS=1 (see config.h and Makefile), so that
 * disabling the module also drops its settings code from the binary.
 */

typedef struct {
    int enabled; /* on/off for both auto-close and match highlighting */
} BracketSettings;

extern BracketSettings br_settings;

int  br_settings_set(const char *key, const char *value);
int  br_settings_get(const char *key);
void br_settings_print_all(void);
void br_settings_write(FILE *f);

#endif
