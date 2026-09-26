#ifndef SETTINGS_H
#define SETTINGS_H

/*
 * "Core" settings — always compiled, regardless of which optional
 * modules are enabled (see config.h). Settings for optional modules live
 * in their own files (e.g. settings_highlight.h) and are wired into the
 * shared store via settings_set/get/save only if the corresponding
 * module is enabled — see settings.c.
 */

typedef struct {
    int tab_width;
    int smart_tab;
    int show_lines;
    int cursor_blink;
} Settings;

extern Settings settings;

// simple <key>=value

int  settings_load(void);
int  settings_save(void);
void settings_print_all(void);
int  settings_get(const char *key);
int  settings_set(const char *key, const char *value);


#endif
