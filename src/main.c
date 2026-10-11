#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "version.h"
#include "settings.h"
#include "platform.h"
#if FEATURE_HIGHLIGHT
#include "highlight.h"
#include "settings_highlight.h"
#endif
#if FEATURE_SELECTION
#include "selection.h"
#endif
#if FEATURE_BRACKETS
#include "brackets.h"
#include "settings_brackets.h"
#endif
#if FEATURE_FIND
#include "find.h"
#endif
#if FEATURE_UNDO
#include "undo.h"
#endif
#if FEATURE_GIT
#include "gitdiff.h"
#include "settings_git.h"
#endif
#include <ctype.h>

char *lines[MAX_LINES];
int count = 0;
int cx = 0; // Left/Right (cx++/cx-- x+1 or x-1)
int cy = 0; // Down/Up (cy++/cy-- y+1 or y-1)
int modified = 0; // for settings (0 - saved, 1 - unsaved)
int term_rows = 24;
int term_cols = 80;

char current_file[256];

char status[96] = "";

#if FEATURE_HIGHLIGHT
static Lang cur_lang = LANG_NONE;
#endif

void set_status(const char *s) {
    snprintf(status, sizeof(status), "%s", s);
}

/* Print a string through the platform's plat_putc, one character at a
 * time. Not part of platform.h — just a convenience wrapper over the
 * primitive. */
static void put_str(const char *s) {
    while (*s) plat_putc(*s++);
}

int prompt(const char *label, char *out, int max) {
    int len = 0;
    out[0] = 0;
    while (1) {
        plat_clear_line(term_rows);
        plat_move_cursor(term_rows, 1);
        put_str(label);
        put_str(out);
        plat_flush();

        int c = plat_read_key();
        if (c == '\r' || c == '\n') return 1;
        if (c == PLAT_KEY_ESC) return 0;
        if ((c == 127 || c == 8) && len > 0) {
            out[--len] = 0;
        } else if (c >= 32 && c != 127 && len < max - 1) {
            out[len++] = c;
            out[len] = 0;
        }
    }
}

void insert_line(int at, char *s) {
    if (count >= MAX_LINES) return;
    for (int i = count; i > at; i--) {
        lines[i] = lines[i - 1];
    }
    lines[at] = s;
    count++;
}

void delete_line(int at) {
    if (at < 0 || at >= count) return;
    free(lines[at]);
    for (int i = at; i < count - 1; i++) {
        lines[i] = lines[i + 1];
    }
    count--;
}

void insert_char(int c) {
    int len = strlen(lines[cy]);
    if (len + 1 >= MAX_LEN) return;
#if FEATURE_UNDO
    undo_record_line_changed(cy, lines[cy], cy, cx);
#endif
    char *newl = malloc(len + 2);
    if (newl == NULL) return;
    memcpy(newl, lines[cy], cx);
    newl[cx] = c;
    strcpy(newl + cx + 1, lines[cy] + cx);
    free(lines[cy]);
    lines[cy] = newl;
    modified = 1;
    cx++;
}

#if FEATURE_SELECTION
/* Inserts (possibly multi-line) text at the cursor position — used for
 * Ctrl+V. Reuses insert_char for characters and repeats the line-split
 * logic from the Enter handler for newlines. */
static void paste_text(const char *text) {
    if (text == NULL) return;
    const char *p = text;
    while (*p) {
        const char *nl = strchr(p, '\n');
        int seglen = nl ? (int)(nl - p) : (int)strlen(p);
        for (int i = 0; i < seglen; i++) insert_char((unsigned char)p[i]);
        if (nl == NULL) break;
        if (count < MAX_LINES) {
            char *rest = strdup(lines[cy] + cx);
            if (rest != NULL) {
#if FEATURE_UNDO
                undo_record_line_changed(cy, lines[cy], cy, cx);
#endif
                lines[cy][cx] = 0;
                insert_line(cy + 1, rest);
#if FEATURE_UNDO
                undo_record_line_inserted(cy + 1, cy, cx);
#endif
                cy++;
                cx = 0;
            }
        }
        p = nl + 1;
    }
    modified = 1;
}
#endif

/* Word-jump helpers for Ctrl+Left/Right (and Ctrl+Shift+Left/Right to
 * extend selection by word). Crossing a line boundary lands at the end
 * of the previous line / start of the next one, same as most editors. */
static int is_word_char(int c) {
    return isalnum(c) || c == '_';
}

static void word_left(int *cy, int *cx) {
    if (*cx == 0) {
        if (*cy > 0) { (*cy)--; *cx = (int)strlen(lines[*cy]); }
        return;
    }
    char *s = lines[*cy];
    int x = *cx;
    while (x > 0 && !is_word_char((unsigned char)s[x - 1])) x--;
    while (x > 0 && is_word_char((unsigned char)s[x - 1])) x--;
    *cx = x;
}

static void word_right(int *cy, int *cx) {
    char *s = lines[*cy];
    int len = (int)strlen(s);
    if (*cx >= len) {
        if (*cy < count - 1) { (*cy)++; *cx = 0; }
        return;
    }
    int x = *cx;
    while (x < len && is_word_char((unsigned char)s[x])) x++;
    while (x < len && !is_word_char((unsigned char)s[x])) x++;
    *cx = x;
}

#if FEATURE_FIND
/* Marks, in mask[0..len), every column that's part of an occurrence of
 * the last search query on this line — used by draw() to highlight all
 * matches, not just the one the cursor happens to be on. */
static void mark_query_matches(const char *line, int len, char *mask) {
    memset(mask, 0, len);
    const char *q = find_get_last();
    int qlen = q != NULL ? (int)strlen(q) : 0;
    if (qlen == 0) return;
    const char *p = line;
    while ((p = strstr(p, q)) != NULL) {
        int start = (int)(p - line);
        for (int i = 0; i < qlen && start + i < len; i++) mask[start + i] = 1;
        p += qlen;
    }
}
#endif

void draw(int cx, int cy, int offset, const char *filename) {
    char buf[MAX_LEN + 32];
#if FEATURE_HIGHLIGHT
    static int colors[MAX_LEN];
    int hl_on = hl_settings.enabled && cur_lang != LANG_NONE;
#endif
#if FEATURE_FIND
    static char qmask[MAX_LEN];
    const char *active_query = find_get_last();
    int query_on = active_query != NULL && active_query[0] != 0;
#endif
#if FEATURE_BRACKETS
    int br_match_found = 0, br_match_y = -1, br_match_x = -1;
    if (br_settings.enabled) {
        int cur_line_len = (int)strlen(lines[cy]);
        if (cx < cur_line_len && bracket_is_bracket(lines[cy][cx])) {
            br_match_found = brackets_find_match(cy, cx, &br_match_y, &br_match_x);
        }
    }
#endif

    plat_show_cursor(0);

    snprintf(buf, sizeof(buf), "-- %s%s -- %d lines", filename, modified ? "*" : "", count);
    plat_clear_line(1);
    put_str(buf);

    int w = 1;
    int tmp = count > 0 ? count : 1;
    while (tmp >= 10) { w++; tmp /= 10; }

    for (int i = offset; i < offset + term_rows - 2; i++) {
        plat_clear_line(i - offset + 2);

        if (i >= count) {
            put_str("~");
            continue;
        }

        int len = strlen(lines[i]);

        if (settings.show_lines) {
            snprintf(buf, sizeof(buf), "%*d ", w, i + 1);
            put_str(buf);
#if FEATURE_GIT
            int git_changed = git_settings.enabled && gitdiff_line_changed(i + 1);
#else
            int git_changed = 0;
#endif
            if (git_changed) {
                /* "M" as in `git status --short` — a line changed since
                 * HEAD. Distinct from the plain "~" separator on other
                 * lines so it doesn't just blend into the margin. */
                plat_set_color(PLAT_COLOR_GIT_CHANGED);
                put_str("M");
                plat_set_color(PLAT_COLOR_DEFAULT);
            } else {
                put_str("~");
            }
            put_str(" ");
        }

#if FEATURE_SELECTION
        int line_has_sel = sel.active;
#else
        int line_has_sel = 0;
#endif
#if FEATURE_BRACKETS
        int line_has_match = br_match_found && i == br_match_y;
#else
        int line_has_match = 0;
#endif
#if FEATURE_FIND
        int line_has_query_match = query_on && strstr(lines[i], active_query) != NULL;
#else
        int line_has_query_match = 0;
#endif

        if (i != cy && !line_has_sel && !line_has_match && !line_has_query_match
#if FEATURE_HIGHLIGHT
            && !hl_on
#endif
        ) {
            /* Fast path: no cursor, no selection, no bracket match, no
             * search-match highlighting and no syntax highlighting on
             * this line — just print it whole. */
            put_str(lines[i]);
            continue;
        }

#if FEATURE_HIGHLIGHT
        if (hl_on) highlight_line(cur_lang, lines[i], colors, len);
#endif
#if FEATURE_FIND
        if (line_has_query_match) mark_query_matches(lines[i], len, qmask);
#endif

        for (int k = 0; k < len; k++) {
            int is_cursor = (i == cy && k == cx);
#if FEATURE_SELECTION
            int is_sel = sel_contains(i, k, cy, cx);
#else
            int is_sel = 0;
#endif
#if FEATURE_HIGHLIGHT
            int color = hl_on ? colors[k] : PLAT_COLOR_DEFAULT;
#else
            int color = PLAT_COLOR_DEFAULT;
#endif
#if FEATURE_FIND
            if (line_has_query_match && qmask[k] && !is_sel && !is_cursor) {
                color = PLAT_COLOR_FIND_MATCH;
            }
#endif
#if FEATURE_BRACKETS
            if (br_match_found && i == br_match_y && k == br_match_x) {
                color = PLAT_COLOR_BRACKET_MATCH;
            }
#endif
            if (color != PLAT_COLOR_DEFAULT) plat_set_color(color);
            if (is_cursor || is_sel) plat_set_inverse(1);
            plat_putc(lines[i][k]);
            if (is_cursor || is_sel) plat_set_inverse(0);
            if (color != PLAT_COLOR_DEFAULT) plat_set_color(PLAT_COLOR_DEFAULT);
        }

        if (i == cy && cx >= len) {
            plat_set_inverse(1);
            plat_putc(' ');
            plat_set_inverse(0);
        }
    }

    plat_clear_line(term_rows);
    put_str(status);
    char posbuf[32];
    snprintf(posbuf, sizeof(posbuf), "Ln %d, Col %d", cy + 1, cx + 1);
    int poslen = (int)strlen(posbuf);
    if (term_cols > poslen) {
        plat_move_cursor(term_rows, term_cols - poslen + 1);
        put_str(posbuf);
    }

    /* if line numbers are on — w + 3 ("<number> ~ ") + 1 (1-based) */
    int prefix = settings.show_lines ? w + 3 : 0;
    plat_move_cursor(cy - offset + 2, cx + prefix + 1);
    plat_show_cursor(1);
    plat_flush();
}

/* Recomputes the scroll offset for the current cursor position (cy/cx
 * are the file-scope cursor globals) and re-reads the terminal size.
 * Shared by the main loop and the interactive Find prompt below, so
 * jumping to a match that's off-screen scrolls the view the same way a
 * normal cursor move would. */
static void sync_view(int *offset) {
    plat_get_size(&term_rows, &term_cols);
    int len = strlen(lines[cy]);
    if (cx > len) cx = len;
    if (cy < *offset) *offset = cy;
    if (cy >= *offset + term_rows - 2) *offset = cy - term_rows + 3;
}

int save_current() {
    FILE *out = fopen(current_file, "w");
    if (out == NULL) {
        perror("tt");
        set_status("Save failed!");
        return 0;
    }
    for (int i = 0; i < count; i++) {
        fprintf(out, "%s\n", lines[i]);
    }
    fclose(out);
    modified = 0;
#if FEATURE_GIT
    if (git_settings.enabled) gitdiff_refresh(current_file);
#endif
    return 1;
}

int main(int argc, char *argv[]) {
    // --settings <key>
    settings_load();
    if (argc >= 2 && strcmp(argv[1], "--settings") == 0) {
        if (argc == 2) {
            settings_print_all();
        } else if (argc == 3) {
            if (!settings_get(argv[2])) {
                fprintf(stderr, "unknown setting: %s\n", argv[2]);
                return 1;
            }
        } else if (argc == 4) {
            if (!settings_set(argv[2], argv[3])) {
                fprintf(stderr, "unknown setting: %s\n", argv[2]);
                return 1;
            }
            settings_save();
        } else {
            fprintf(stderr, "usage: tt --settings [key [value]]\n");
            return 1;
        }
        return 0;
    }

    // --version
    if (argc >= 2 && strcmp(argv[1], "--version") == 0) {
        printf("termtext v%s\n", TT_VERSION);
        printf("Copyright (C) 2026 homaaio\n");
        return 0;
    }
    /* tt <file> [-c <cmd...>] — cmd (all remaining args joined by
     * spaces) is run through the shell after the editor closes. */
    char *after_cmd = NULL;
    if (argc < 2) {
        fprintf(stderr, "usage: tt <file> [-c <cmd>]\n");
        return 1;
    } else if (argc >= 3) {
        if (strcmp(argv[2], "-c") != 0 || argc < 4) {
            fprintf(stderr, argc >= 3 && strcmp(argv[2], "-c") == 0
                    ? "tt: -c needs a command\n"
                    : "can't open 2 and more files (usage: tt <file> [-c <cmd>])\n");
            return 1;
        }
        size_t total = 1;
        for (int i = 3; i < argc; i++) total += strlen(argv[i]) + 1;
        after_cmd = malloc(total);
        if (after_cmd == NULL) { perror("malloc"); return 1; }
        after_cmd[0] = 0;
        for (int i = 3; i < argc; i++) {
            if (i > 3) strcat(after_cmd, " ");
            strcat(after_cmd, argv[i]);
        }
    }

    snprintf(current_file, sizeof(current_file), "%s", argv[1]);

#if FEATURE_HIGHLIGHT
    cur_lang = highlight_detect_lang(current_file);
#endif

    FILE *file = fopen(current_file, "r");
    if (file != NULL) {
        char buf[MAX_LEN];
        while (fgets(buf, sizeof(buf), file)) {
            if (count >= MAX_LINES) {
                set_status("File too long: truncated at MAX_LINES (saving would cut it!)");
                break;
            }
            buf[strcspn(buf, "\n")] = 0;
            lines[count] = strdup(buf);
            if (lines[count] == NULL) {
                perror("strdup");
                break;
            }
            count++;
        }
        fclose(file);
    }
    if (count == 0) {
        lines[0] = strdup("");
        if (lines[0] == NULL) {
            perror("strdup");
            return 1;
        }
        count = 1;
    }

#if FEATURE_GIT
    if (git_settings.enabled) gitdiff_refresh(current_file);
#endif

    if (!plat_init()) {
        fprintf(stderr, "tt: failed to init platform\n");
        return 1;
    }

    int offset = 0;
    int running = 1;
    int quit_pending = 0; /* set after a first Ctrl+Q on an unsaved file; needs a second, immediate Ctrl+Q to actually quit */

    while (running) {
        sync_view(&offset);
        int len = strlen(lines[cy]);
        draw(cx, cy, offset, current_file);

        int c = plat_read_key();
        if (c == 0) {
            /* nothing pressed (relevant for MCU builds) — just redraw */
            plat_flush();
            continue;
        }
        if (status[0]) status[0] = 0;
        if (c != 17) quit_pending = 0; /* anything other than Ctrl+Q cancels a pending quit confirmation */
        len = strlen(lines[cy]);

#if FEATURE_UNDO
        /* One step per handled key — however many line ops this key
         * goes on to record (if any), they'll all undo together. */
        undo_next_step();
#endif

#if FEATURE_SELECTION
        int is_shift_move = (c == PLAT_KEY_SHIFT_UP || c == PLAT_KEY_SHIFT_DOWN ||
                              c == PLAT_KEY_SHIFT_RIGHT || c == PLAT_KEY_SHIFT_LEFT ||
                              c == PLAT_KEY_SHIFT_HOME || c == PLAT_KEY_SHIFT_END ||
                              c == PLAT_KEY_SHIFT_CTRL_LEFT || c == PLAT_KEY_SHIFT_CTRL_RIGHT);
        /* Must be computed BEFORE the selection is cleared below —
         * Backspace/Enter/typing/paste replace the selected text, and
         * they only see it through has_sel. */
        int has_sel = sel_has_range(cy, cx);
        if (is_shift_move) {
            sel_start_if_needed(cy, cx);
        } else if (c != 3 && c != 24 && c != 1 && c != 4 && c != 9 && c != PLAT_KEY_BACKTAB) {
            /* Ctrl+C/Ctrl+X/Ctrl+D act on the current selection, Ctrl+A
             * sets its own selection right below, and Tab/Shift+Tab
             * indent the selected lines — all of them need to see it
             * still active. Any other key clears it. */
            sel_clear();
        }
#endif

        if (c == EOF) {
            running = 0;
        } else if (c == 17) {
            /* Ctrl+Q — quit; if there are unsaved changes, require a
             * second, immediate Ctrl+Q instead of quitting right away. */
            if (modified && !quit_pending) {
                quit_pending = 1;
                set_status("Unsaved changes! Press Ctrl+Q again to quit without saving");
            } else {
                running = 0;
            }
        } else if (c == 19) {
            /* Ctrl+S — just save file */
            if (save_current()) set_status("Saved!");
        } else if (c == 15) {
            /* Ctrl+O — save as (ask name) */
            char newname[256];
            if (prompt("Save as: ", newname, sizeof(newname)) && newname[0]) {
                snprintf(current_file, sizeof(current_file), "%s", newname);
#if FEATURE_HIGHLIGHT
                cur_lang = highlight_detect_lang(current_file);
#endif
                if (save_current()) set_status("Saved as new file");
            }
        } else if (c == 7) {
            /* Ctrl+G — go to line */
            char buf[32];
            if (prompt("Go to line: ", buf, sizeof(buf)) && buf[0]) {
                int target = atoi(buf);
                if (target >= 1 && target <= count) {
                    cy = target - 1;
                    cx = 0;
                } else {
                    set_status("Line out of range");
                }
            }
#if FEATURE_UNDO
        } else if (c == 26) {
            /* Ctrl+Z — undo the most recent step */
            set_status(undo_perform(&cy, &cx) ? "Undo" : "Nothing to undo");
        } else if (c == 18) {
            /* Ctrl+R — redo the most recently undone step */
            set_status(redo_perform(&cy, &cx) ? "Redo" : "Nothing to redo");
#endif
        } else if (c == 4) {
            /* Ctrl+D — duplicate the current line, or every line the
             * selection touches, right below it. */
            int y0 = cy, y1 = cy;
#if FEATURE_SELECTION
            int x0_unused, x1_unused;
            if (has_sel) sel_range(cy, cx, &y0, &x0_unused, &y1, &x1_unused);
#endif
            int n = y1 - y0 + 1;
            if (count + n <= MAX_LINES) {
                for (int i = 0; i < n; i++) {
                    char *copy = strdup(lines[y0 + i]);
                    if (copy == NULL) break;
                    int at = y1 + 1 + i;
                    insert_line(at, copy);
#if FEATURE_UNDO
                    undo_record_line_inserted(at, cy, cx);
#endif
                }
                cy = y1 + 1 + (cy - y0);
                modified = 1;
#if FEATURE_SELECTION
                sel_clear();
#endif
            }
#if FEATURE_SELECTION
        } else if (c == 1) {
            /* Ctrl+A — select the whole file */
            sel.active = 1;
            sel.anchor_y = 0;
            sel.anchor_x = 0;
            cy = count - 1;
            cx = (int)strlen(lines[cy]);
        } else if (c == 3) {
            /* Ctrl+C — copy selection to the OS clipboard, so it's
             * available outside termtext too, with the internal
             * clipboard (selection.c) mirroring it as a fallback for
             * platforms with no OS clipboard to reach. */
            if (has_sel) {
                char *txt = sel_copy_text(cy, cx);
                if (txt != NULL) {
                    plat_clipboard_set(txt);
                    sel_clipboard_set(txt); /* takes ownership of txt */
                    set_status("Copied");
                }
            }
        } else if (c == 24) {
            /* Ctrl+X — same as Ctrl+C, but removes the selection too */
            if (has_sel) {
                char *txt = sel_copy_text(cy, cx);
                sel_delete(&cy, &cx);
                modified = 1;
                if (txt != NULL) {
                    plat_clipboard_set(txt);
                    sel_clipboard_set(txt); /* takes ownership of txt */
                    set_status("Cut");
                }
            }
        } else if (c == 22) {
            /* Ctrl+V — paste. Prefers the OS clipboard, so text copied
             * from outside termtext pastes too; falls back to the
             * internal clipboard where there's no OS clipboard to read
             * (or it's empty). */
            if (has_sel) sel_delete(&cy, &cx);
            char *os_clip = plat_clipboard_get();
            if (os_clip != NULL) {
                paste_text(os_clip);
                sel_clipboard_set(os_clip); /* keep both clipboards in sync; takes ownership */
                set_status("Pasted");
            } else {
                const char *clip = sel_clipboard_get();
                if (clip != NULL) {
                    paste_text(clip);
                    set_status("Pasted");
                }
            }
#endif
        } else if (c == PLAT_KEY_UP
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_UP
#endif
        ) {
            if (cy > 0) cy--;
        } else if (c == PLAT_KEY_DOWN
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_DOWN
#endif
        ) {
            if (cy < count - 1) cy++;
        } else if (c == PLAT_KEY_RIGHT
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_RIGHT
#endif
        ) {
            if (cx < len) cx++;
        } else if (c == PLAT_KEY_LEFT
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_LEFT
#endif
        ) {
            if (cx > 0) cx--;
        } else if (c == PLAT_KEY_HOME
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_HOME
#endif
        ) {
            cx = 0;
        } else if (c == PLAT_KEY_END
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_END
#endif
        ) {
            cx = len;
        } else if (c == PLAT_KEY_PGUP) {
            int page = term_rows > 2 ? term_rows - 2 : 1;
            cy -= page;
            if (cy < 0) cy = 0;
        } else if (c == PLAT_KEY_PGDN) {
            int page = term_rows > 2 ? term_rows - 2 : 1;
            cy += page;
            if (cy > count - 1) cy = count - 1;
        } else if (c == PLAT_KEY_CTRL_LEFT
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_CTRL_LEFT
#endif
        ) {
            word_left(&cy, &cx);
        } else if (c == PLAT_KEY_CTRL_RIGHT
#if FEATURE_SELECTION
                   || c == PLAT_KEY_SHIFT_CTRL_RIGHT
#endif
        ) {
            word_right(&cy, &cx);
        } else if (c == PLAT_KEY_ALT_UP) {
            /* Alt+Up — move the current line up, swapping it with the one above */
            if (cy > 0) {
#if FEATURE_UNDO
                undo_record_line_changed(cy - 1, lines[cy - 1], cy, cx);
                undo_record_line_changed(cy, lines[cy], cy, cx);
#endif
                char *tmp = lines[cy - 1];
                lines[cy - 1] = lines[cy];
                lines[cy] = tmp;
                cy--;
                modified = 1;
            }
        } else if (c == PLAT_KEY_ALT_DOWN) {
            /* Alt+Down — move the current line down, swapping it with the one below */
            if (cy < count - 1) {
#if FEATURE_UNDO
                undo_record_line_changed(cy, lines[cy], cy, cx);
                undo_record_line_changed(cy + 1, lines[cy + 1], cy, cx);
#endif
                char *tmp = lines[cy];
                lines[cy] = lines[cy + 1];
                lines[cy + 1] = tmp;
                cy++;
                modified = 1;
            }
        } else if (c == 5) {
            if (offset + term_rows - 2 < count) offset++;
        } else if (c == 25) {
            if (offset > 0) offset--;
#if FEATURE_FIND
        } else if (c == 6) {
            /* Ctrl+F — interactive find. The query prompt stays open
             * while you search: Up/Down jump to the previous/next match
             * right away, without closing the prompt and pressing
             * Ctrl+F again to repeat the same query. Opens pre-filled
             * with the last search so Up/Down works immediately. Enter
             * or Esc closes the prompt; the cursor stays wherever the
             * last match left it. */
            char query[128];
            const char *seed = find_get_last();
            snprintf(query, sizeof(query), "%s", seed != NULL ? seed : "");
            int qlen = (int)strlen(query);
            int have_match = 0;
            while (1) {
                char label[48];
                snprintf(label, sizeof(label), "Find (up/down: next/prev): ");
                plat_clear_line(term_rows);
                plat_move_cursor(term_rows, 1);
                put_str(label);
                put_str(query);
                plat_flush();

                int k = plat_read_key();
                if (k == '\r' || k == '\n') {
                    /* Enter: same as the old Ctrl+F — jump to the next
                     * match from here, same as pressing Down would. */
                    if (query[0]) have_match = find_next(query, &cy, &cx, 1);
                    break;
                }
                if (k == PLAT_KEY_ESC) break;
                if ((k == 127 || k == 8) && qlen > 0) {
                    query[--qlen] = 0;
                } else if (k == PLAT_KEY_DOWN && query[0]) {
                    have_match = find_next(query, &cy, &cx, 1);
                    sync_view(&offset);
                    draw(cx, cy, offset, current_file);
                } else if (k == PLAT_KEY_UP && query[0]) {
                    have_match = find_prev(query, &cy, &cx, 1);
                    sync_view(&offset);
                    draw(cx, cy, offset, current_file);
                } else if (k >= 32 && k != 127 && qlen < (int)sizeof(query) - 1) {
                    query[qlen++] = k;
                    query[qlen] = 0;
                }
            }
            if (query[0]) {
                find_set_last(query);
                set_status(have_match ? "Found" : "Not found");
            }
        } else if (c == 8) {
            /* Ctrl+H — find & replace, built on the same find_next() /
             * find_replace_all() as Ctrl+F above rather than a second
             * search implementation. Replaces every match in the file.
             *
             * Note: code 8 is also the traditional ASCII code for
             * Backspace, but modern terminals (in raw mode, as used
             * here) send DEL (127) for the Backspace key instead — see
             * the Backspace handling below, which now only reacts to
             * 127. If your terminal's Backspace key turns out to still
             * send 8, tell me and I'll move find & replace to a
             * different key. */
            char query[128];
            if (prompt("Find: ", query, sizeof(query))) {
                const char *q = query[0] ? query : find_get_last();
                if (q != NULL && q[0]) {
                    char repl[128];
                    if (prompt("Replace with: ", repl, sizeof(repl))) {
                        find_set_last(q);
                        int n = find_replace_all(q, repl);
                        char msg[64];
                        snprintf(msg, sizeof(msg), "Replaced %d occurrence(s)", n);
                        set_status(msg);
                    }
                }
            }
#endif
        } else if (c == 127) {
#if FEATURE_SELECTION
            if (has_sel) {
                sel_delete(&cy, &cx);
                modified = 1;
            } else
#endif
            if (cx > 0) {
#if FEATURE_UNDO
                undo_record_line_changed(cy, lines[cy], cy, cx);
#endif
                memmove(&lines[cy][cx - 1], &lines[cy][cx], len - cx + 1);
                cx--;
                modified = 1;
            } else if (cy > 0) {
                int prev_len = strlen(lines[cy - 1]);
                if (prev_len + len + 1 > MAX_LEN) continue;
                char *merged = malloc(prev_len + len + 1);
                if (merged == NULL) continue;
                strcpy(merged, lines[cy - 1]);
                strcat(merged, lines[cy]);
#if FEATURE_UNDO
                undo_record_line_changed(cy - 1, lines[cy - 1], cy, cx);
                undo_record_line_deleted(cy, lines[cy], cy, cx);
#endif
                free(lines[cy - 1]);
                lines[cy - 1] = merged;
                delete_line(cy);
                cy--;
                cx = prev_len;
                modified = 1;
            }
        } else if (c == '\r' || c == '\n') {
#if FEATURE_SELECTION
            if (has_sel) sel_delete(&cy, &cx);
#endif
            if (count >= MAX_LINES) continue;
            /* Auto-indent: the new line starts with the same leading
             * whitespace as the one being split, capped at the split
             * point itself (so splitting inside the indentation doesn't
             * indent past where Enter was pressed). Folded into the same
             * "old text" the OP_INSERTED undo step already captures, so
             * this doesn't need an undo op of its own. */
            int indent = 0;
            if (settings.auto_indent) {
                while (lines[cy][indent] == ' ' || lines[cy][indent] == '\t') indent++;
                if (indent > cx) indent = cx;
            }
            char *tail = lines[cy] + cx;
            char *rest = malloc(indent + strlen(tail) + 1);
            if (rest == NULL) continue;
            memset(rest, ' ', indent);
            strcpy(rest + indent, tail);
#if FEATURE_UNDO
            undo_record_line_changed(cy, lines[cy], cy, cx);
#endif
            lines[cy][cx] = 0;
            insert_line(cy + 1, rest);
#if FEATURE_UNDO
            undo_record_line_inserted(cy + 1, cy, cx);
#endif
            cy++;
            cx = indent;
            modified = 1;
        } else if (c == 9 || c == PLAT_KEY_BACKTAB) {
#if FEATURE_SELECTION
            if (has_sel) {
                /* Tab/Shift+Tab with an active selection indents or
                 * unindents every line it touches, instead of typing a
                 * tab — the usual editor convention for block indent. */
                int y0, x0, y1, x1;
                sel_range(cy, cx, &y0, &x0, &y1, &x1);
                for (int i = y0; i <= y1; i++) {
                    char *line = lines[i];
                    int llen = (int)strlen(line);
                    if (c == 9) {
                        if (llen + settings.tab_width + 1 > MAX_LEN) continue;
#if FEATURE_UNDO
                        undo_record_line_changed(i, line, cy, cx);
#endif
                        char *nl = malloc(llen + settings.tab_width + 1);
                        if (nl != NULL) {
                            memset(nl, ' ', settings.tab_width);
                            memcpy(nl + settings.tab_width, line, llen + 1);
                            free(lines[i]);
                            lines[i] = nl;
                        }
                    } else {
                        int rm = 0;
                        while (rm < settings.tab_width && line[rm] == ' ') rm++;
                        if (rm > 0) {
#if FEATURE_UNDO
                            undo_record_line_changed(i, line, cy, cx);
#endif
                            memmove(line, line + rm, llen - rm + 1);
                        }
                    }
                }
                cy = y1;
                cx = 0;
                sel_clear();
                modified = 1;
            } else
#endif
            if (c == 9) {
                int spaces = settings.smart_tab
                           ? settings.tab_width - (cx % settings.tab_width)
                           : settings.tab_width;
                for (int k = 0; k < spaces; k++) {
                    insert_char(' ');
                }
            } else {
                /* Shift+Tab with no selection: unindent the current line */
                char *line = lines[cy];
                int rm = 0;
                while (rm < settings.tab_width && line[rm] == ' ') rm++;
                if (rm > 0) {
#if FEATURE_UNDO
                    undo_record_line_changed(cy, line, cy, cx);
#endif
                    int llen = (int)strlen(line);
                    memmove(line, line + rm, llen - rm + 1);
                    cx = cx >= rm ? cx - rm : 0;
                    modified = 1;
                }
            }
        } else if (c >= 32 && c != 127) {
#if FEATURE_SELECTION
            if (has_sel) sel_delete(&cy, &cx);
#endif
#if FEATURE_BRACKETS
            if (!br_settings.enabled || !brackets_smart_insert(c, &cy, &cx)) {
                insert_char(c);
            }
#else
            insert_char(c);
#endif
        }
    }

    plat_shutdown();

    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }

    if (after_cmd != NULL) {
        int rc = system(after_cmd);
        (void)rc;
        free(after_cmd);
    }

    return 0;
}
