#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

/*
 * highlight.h/.c — модуль подсветки синтаксиса (FEATURE_HIGHLIGHT в
 * config.h). Простой построчный токенайзер: многострочные комментарии
 * (например, блочные комментарии C/C++/Rust) не отслеживаются между
 * строками — сознательный компромисс "20% усилий -> 80% красоты",
 * которого просил заказчик, а не полноценный парсер языка.
 */

typedef enum {
    LANG_NONE = 0,
    LANG_C,       /* C / C++ */
    LANG_RUST,
    LANG_PYTHON,
    LANG_ASM
} Lang;

/* Определяет язык по расширению имени файла. LANG_NONE, если не распознано
 * или расширения нет — в этом случае подсветка просто не применяется. */
Lang highlight_detect_lang(const char *filename);

/* Заполняет out_colors[0..len-1] значениями PLAT_COLOR_* для каждого
 * символа строки line (len должен равняться strlen(line)). */
void highlight_line(Lang lang, const char *line, int *out_colors, int len);

#endif
