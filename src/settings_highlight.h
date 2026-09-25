#ifndef SETTINGS_HIGHLIGHT_H
#define SETTINGS_HIGHLIGHT_H

#include <stdio.h>

/*
 * Настройки модуля подсветки синтаксиса — отдельный файл настроек,
 * компилируемый только при FEATURE_HIGHLIGHT=1 (см. config.h и Makefile),
 * чтобы при отключении модуля из бинарника пропадал и код его настроек.
 */

typedef struct {
    int enabled; /* вкл/выкл подсветку синтаксиса */
} HlSettings;

extern HlSettings hl_settings;

int  hl_settings_set(const char *key, const char *value);
int  hl_settings_get(const char *key);
void hl_settings_print_all(void);
void hl_settings_write(FILE *f);

#endif
