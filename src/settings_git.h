#ifndef SETTINGS_GIT_H
#define SETTINGS_GIT_H

#include <stdio.h>

/*
 * Settings for the git-gutter module (src/gitdiff.c) — a separate
 * settings file, compiled only when FEATURE_GIT=1 (see config.h and
 * Makefile), so that disabling the module also drops its settings code
 * from the binary.
 */

typedef struct {
    int enabled; /* on/off for the changed-line gutter color */
} GitSettings;

extern GitSettings git_settings;

int  git_settings_set(const char *key, const char *value);
int  git_settings_get(const char *key);
void git_settings_print_all(void);
void git_settings_write(FILE *f);

#endif
