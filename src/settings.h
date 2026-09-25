#ifndef SETTINGS_H
#define SETTINGS_H

/*
 * Настройки "ядра" — всегда компилируются, независимо от того, какие
 * необязательные модули включены (см. config.h). Настройки необязательных
 * модулей живут в своих файлах (например settings_highlight.h) и
 * подключаются к общему хранилищу через settings_set/get/save только если
 * соответствующий модуль включён — см. settings.c.
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
