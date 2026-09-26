#ifndef SELECTION_H
#define SELECTION_H

/*
 * selection.h/.c — text selection (Shift+arrows) and the internal
 * clipboard (Ctrl+C/X/V). Compiled only when FEATURE_SELECTION=1
 * (see config.h).
 *
 * The "internal" clipboard deliberately doesn't use the OS system
 * clipboard — this keeps copy/cut/paste working on platforms with no OS
 * too (see the README's "Running without an OS" section), and it also
 * means every plat_*.c doesn't need to implement yet another system API.
 *
 * The module works on top of the editor's line buffer (lines/count),
 * which is declared in main.c — insert_line/delete_line come from there
 * too.
 */

typedef struct {
    int active;             /* whether a selection "anchor" exists */
    int anchor_y, anchor_x; /* the point where selection started */
} Selection;

extern Selection sel;

void sel_start_if_needed(int cy, int cx);

void sel_clear(void);

int sel_has_range(int cy, int cx);

void sel_range(int cy, int cx, int *y0, int *x0, int *y1, int *x1);

int sel_contains(int y, int x, int cy, int cx);

void sel_delete(int *cy, int *cx);

char *sel_copy_text(int cy, int cx);

void        sel_clipboard_set(char *text);
const char *sel_clipboard_get(void);

#endif
