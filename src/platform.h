#ifndef PLATFORM_H
#define PLATFORM_H

/*
 * Shared platform interface. No system headers here — just
 * declarations and constants. Implementations: plat_unix.c, plat_win.c,
 * plat_mcu.c.
 */

/* Init / shutdown of the platform layer (terminal, display, etc.) */
int  plat_init(void);      /* 0 on error, non-zero otherwise */
void plat_shutdown(void);  /* restore the original state */

/* Screen */
void plat_get_size(int *rows, int *cols);
void plat_clear(void);
void plat_clear_line(int row);
void plat_move_cursor(int row, int col);
void plat_set_inverse(int on);
void plat_show_cursor(int visible);
void plat_set_cursor_style(int blink);

/*
 * Text color — used by the syntax highlighting module (see
 * FEATURE_HIGHLIGHT in config.h and src/highlight.c) and by the smart
 * brackets module's matching-bracket highlight (see FEATURE_BRACKETS in
 * config.h and src/brackets.c). If a module is disabled at build time,
 * the corresponding colors are simply never passed in, but they stay
 * part of the interface so no platform needs to know about
 * FEATURE_HIGHLIGHT/FEATURE_BRACKETS directly.
 */
enum {
    PLAT_COLOR_DEFAULT = 0,
    PLAT_COLOR_KEYWORD,
    PLAT_COLOR_TYPE,
    PLAT_COLOR_STRING,
    PLAT_COLOR_COMMENT,
    PLAT_COLOR_NUMBER,
    PLAT_COLOR_PREPROC,
    PLAT_COLOR_ESCAPE,       /* escape sequences inside strings/chars, e.g. \n \t \xFF */
    PLAT_COLOR_BRACKET_MATCH /* the matching partner of the bracket under the cursor */
};
void plat_set_color(int color);

/* Output / input */
void plat_putc(char c);
void plat_flush(void);
int  plat_read_key(void); /* plain characters — as-is, arrows — constants below */

/* Special keys */
#define PLAT_KEY_UP    1000
#define PLAT_KEY_DOWN  1001
#define PLAT_KEY_RIGHT 1002
#define PLAT_KEY_LEFT  1003
#define PLAT_KEY_ESC   27

/*
 * Optional OS clipboard access — layered on top of the internal
 * clipboard in selection.c (see FEATURE_SELECTION) so Ctrl+C/X/V in
 * main.c can also interoperate with text copied outside the program.
 * Not every platform has a system clipboard (there's obviously nothing
 * to talk to on a bare-metal MCU build, and on unix it depends on an
 * external helper tool being installed): plat_clipboard_get() returns
 * NULL when unavailable/empty, and the caller falls back to the
 * internal clipboard; plat_clipboard_set() silently no-ops in that
 * case, since the internal clipboard already has the text.
 */
void  plat_clipboard_set(const char *text);
char *plat_clipboard_get(void); /* malloc'd (caller frees), or NULL */

/*
 * Shift-held arrows — used by the text-selection module (see
 * FEATURE_SELECTION in config.h and src/selection.c). If the selection
 * module is disabled, main.c just treats them as plain arrows, so
 * plat_*.c implementations can always return them without #ifdef.
 */
#define PLAT_KEY_SHIFT_UP    1010
#define PLAT_KEY_SHIFT_DOWN  1011
#define PLAT_KEY_SHIFT_RIGHT 1012
#define PLAT_KEY_SHIFT_LEFT  1013

#endif
