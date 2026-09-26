#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "find.h"
#if FEATURE_UNDO
#include "undo.h"
#endif

/* The editor's line buffer — defined in main.c. */
extern char *lines[MAX_LINES];
extern int   count;
extern int   modified;
extern int   cx;
extern int   cy;

static char last_query[128] = "";

void find_set_last(const char *query) {
    if (query != NULL) snprintf(last_query, sizeof(last_query), "%s", query);
}

const char *find_get_last(void) {
    return last_query;
}

int find_next(const char *query, int *cy, int *cx, int wrap) {
    if (query == NULL || query[0] == 0 || count == 0) return 0;

    int y = *cy;
    int x = *cx + 1; /* search past the cursor so repeating Ctrl+F advances */

    /* At most count+1 lines are ever visited: the starting line first
     * (from x), then every other line in order, then — if wrap is set —
     * back to the starting line one more time (from x=0), which covers
     * the whole document exactly once without looping forever. */
    for (int scanned = 0; scanned <= count; scanned++) {
        if (y >= count) {
            if (!wrap) return 0;
            y = 0;
        }
        char *line = lines[y];
        int len = (int)strlen(line);
        if (x <= len) {
            char *hit = strstr(line + x, query);
            if (hit != NULL) {
                *cy = y;
                *cx = (int)(hit - line);
                return 1;
            }
        }
        y++;
        x = 0;
    }
    return 0;
}

int find_replace_all(const char *query, const char *replacement) {
    if (query == NULL || query[0] == 0 || replacement == NULL) return 0;

    int qlen = (int)strlen(query);
    int rlen = (int)strlen(replacement);
    int total = 0;

    for (int i = 0; i < count; i++) {
        char *line = lines[i];
        int occ = 0;
        for (const char *p = line; (p = strstr(p, query)) != NULL; p += qlen) occ++;
        if (occ == 0) continue;

        int len = (int)strlen(line);
        int new_len = len + occ * (rlen - qlen);
        if (new_len < 0) new_len = 0;
        if (new_len >= MAX_LEN) new_len = MAX_LEN - 1; /* clamp to the line-length limit */

        char *out = malloc(new_len + 1);
        if (out == NULL) continue;

        char *dst = out;
        const char *src = line;
        int remaining = new_len;
        while (remaining > 0 || *src) {
            const char *hit = strstr(src, query);
            int seg = hit != NULL ? (int)(hit - src) : (int)strlen(src);
            if (seg > remaining) seg = remaining;
            memcpy(dst, src, seg);
            dst += seg;
            remaining -= seg;
            if (hit == NULL) break;
            int rcopy = rlen < remaining ? rlen : remaining;
            if (rcopy > 0) {
                memcpy(dst, replacement, rcopy);
                dst += rcopy;
                remaining -= rcopy;
            }
            src = hit + qlen;
        }
        *dst = 0;

#if FEATURE_UNDO
        undo_record_line_changed(i, lines[i], cy, cx);
#endif
        free(lines[i]);
        lines[i] = out;
        total += occ;
    }

    if (total > 0) modified = 1;
    return total;
}
