#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h — общие настройки сборки и переключатели модулей.
 *
 * Каждый модуль можно отключить прямо при сборке, чтобы уменьшить размер
 * бинарника: соответствующие .c-файлы просто не попадут в список исходников
 * в Makefile, и весь их код (таблицы ключевых слов, буфер обмена и т.д.)
 * не будет слинкован в итоговый файл.
 *
 *   make tt FEATURE_HIGHLIGHT=0                       # без подсветки синтаксиса
 *   make tt FEATURE_SELECTION=0                       # без выделения и буфера обмена
 *   make tt FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0   # минимальная сборка (см. `make minimal`)
 *
 * Значения ниже — это то, что получится, если собрать без параметров
 * (make tt) или открыть проект в IDE, которая не знает про Makefile.
 * Реальное значение всегда приходит снаружи через -D (см. Makefile).
 */

#ifndef FEATURE_HIGHLIGHT
#define FEATURE_HIGHLIGHT 1   /* подсветка синтаксиса: src/highlight.c, src/settings_highlight.c */
#endif

#ifndef FEATURE_SELECTION
#define FEATURE_SELECTION 1   /* выделение текста + внутренний буфер обмена: src/selection.c */
#endif

/*
 * Лимиты буфера редактора. На ПК (unix/win) значения по умолчанию не
 * критичны. При портировании на микроконтроллер (см. plat_mcu.c и раздел
 * README "Работа без ОС") их почти наверняка нужно уменьшить под доступную
 * RAM — переопределяются точно так же, через -D:
 *
 *   make tt PLAT=mcu FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 \
 *           EXTRA_CFLAGS="-DMAX_LINES=200 -DMAX_LEN=128"
 */
#ifndef MAX_LINES
#define MAX_LINES 1000
#endif
#ifndef MAX_LEN
#define MAX_LEN 1024
#endif

#endif
