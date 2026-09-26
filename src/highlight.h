#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

/*
 * highlight.h/.c — syntax highlighting module (FEATURE_HIGHLIGHT in
 * config.h). A simple line-by-line tokenizer: multi-line comments
 * (e.g. C/C++/Rust block comments) aren't tracked across lines — a
 * deliberate "20% effort -> 80% nice-looking" tradeoff the client
 * asked for, not a full language parser.
 */

typedef enum {
    LANG_NONE = 0,
    LANG_C,       /* C / C++ */
    LANG_RUST,
    LANG_PYTHON,
    LANG_ASM
} Lang;

/* Detects the language from the file name's extension. LANG_NONE if not
 * recognized or there's no extension — in that case highlighting is
 * simply not applied. */
Lang highlight_detect_lang(const char *filename);

/* Fills out_colors[0..len-1] with PLAT_COLOR_* values for each
 * character of line (len must equal strlen(line)). */
void highlight_line(Lang lang, const char *line, int *out_colors, int len);

#endif
