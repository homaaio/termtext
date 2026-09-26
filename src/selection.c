#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "selection.h"

/* The editor's line buffer — defined in main.c. */
extern char *lines[MAX_LINES];
extern int   count;
extern void insert_line(int at, char *s);
extern void delete_line(int at);

Selection sel = { 0, 0, 0 };

static char *clipboard = NULL;

void sel_start_if_needed(int cy, int cx) {
    if (!sel.active) {
        sel.active = 1;
        sel.anchor_y = cy;
        sel.anchor_x = cx;
    }
}

void sel_clear(void) {
    sel.active = 0;
}

void sel_range(int cy, int cx, int *y0, int *x0, int *y1, int *x1) {
    int ay = sel.anchor_y, ax = sel.anchor_x;
    if (ay < cy || (ay == cy && ax <= cx)) {
        *y0 = ay; *x0 = ax; *y1 = cy; *x1 = cx;
    } else {
        *y0 = cy; *x0 = cx; *y1 = ay; *x1 = ax;
    }
}

int sel_has_range(int cy, int cx) {
    return sel.active && (sel.anchor_y != cy || sel.anchor_x != cx);
}

int sel_contains(int y, int x, int cy, int cx) {
    if (!sel_has_range(cy, cx)) return 0;
    int y0, x0, y1, x1;
    sel_range(cy, cx, &y0, &x0, &y1, &x1);
    if (y < y0 || y > y1) return 0;
    if (y0 == y1) return x >= x0 && x < x1;
    if (y == y0) return x >= x0;
    if (y == y1) return x < x1;
    return 1;
}

void sel_delete(int *cy, int *cx) {
    int y0, x0, y1, x1;
    sel_range(*cy, *cx, &y0, &x0, &y1, &x1);

    if (y0 == y1) {
        char *line = lines[y0];
        int len = strlen(line);
        memmove(line + x0, line + x1, len - x1 + 1);
    } else {
        char *first = lines[y0];
        char *last = lines[y1];
        int first_keep = x0;
        int last_len = strlen(last);
        int new_len = first_keep + (last_len - x1);
        char *merged = malloc(new_len + 1);
        if (merged != NULL) {
            memcpy(merged, first, first_keep);
            memcpy(merged + first_keep, last + x1, last_len - x1 + 1); /* + '\0' */
            free(lines[y0]);
            lines[y0] = merged;
        }
        for (int i = y1; i > y0; i--) {
            delete_line(i);
        }
    }

    *cy = y0;
    *cx = x0;
    sel_clear();
}

char *sel_copy_text(int cy, int cx) {
    int y0, x0, y1, x1;
    sel_range(cy, cx, &y0, &x0, &y1, &x1);

    if (y0 == y1) {
        int len = x1 - x0;
        char *out = malloc(len + 1);
        if (out == NULL) return NULL;
        memcpy(out, lines[y0] + x0, len);
        out[len] = 0;
        return out;
    }

    size_t total = strlen(lines[y0]) - x0 + 1; /* + '\n' */
    for (int i = y0 + 1; i < y1; i++) total += strlen(lines[i]) + 1;
    total += (size_t)x1;

    char *out = malloc(total + 1);
    if (out == NULL) return NULL;
    char *p = out;

    size_t l0 = strlen(lines[y0]) - x0;
    memcpy(p, lines[y0] + x0, l0); p += l0;
    *p++ = '\n';
    for (int i = y0 + 1; i < y1; i++) {
        size_t l = strlen(lines[i]);
        memcpy(p, lines[i], l); p += l;
        *p++ = '\n';
    }
    memcpy(p, lines[y1], x1); p += x1;
    *p = 0;
    return out;
}

void sel_clipboard_set(char *text) {
    free(clipboard);
    clipboard = text;
}

const char *sel_clipboard_get(void) {
    return clipboard;
}
