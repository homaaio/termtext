#include <string.h>
#include "config.h"
#include "brackets.h"

extern char *lines[MAX_LINES];
extern int   count;
extern void  insert_char(int c);

char bracket_partner(char c) {
    switch (c) {
        case '(':  return ')';
        case '[':  return ']';
        case '{':  return '}';
        case '"':  return '"';
        case '\'': return '\'';
        default:   return 0;
    }
}

static int is_close_char(char c) {
    return c == ')' || c == ']' || c == '}' || c == '"' || c == '\'';
}

static int is_word_char_local(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

int bracket_is_bracket(char c) {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']';
}

int brackets_smart_insert(int c, int *cy, int *cx) {
    const char *line = lines[*cy];
    int len = (int)strlen(line);

    /* Typing a closing character right before that same character —
     * assume it's the partner we auto-inserted earlier and just step
     * over it instead of inserting a duplicate. */
    if (is_close_char((char)c) && *cx < len && line[*cx] == c) {
        (*cx)++;
        return 1;
    }

    char partner = bracket_partner((char)c);
    if (partner) {
        /* Only auto-pair at the end of the line or before a non-word
         * character, so typing in the middle of an existing
         * word/string isn't disrupted. */
        int next_is_word = *cx < len && is_word_char_local(line[*cx]);
        if (!next_is_word) {
            insert_char(c);       /* advances the global cx, aliased by *cx */
            insert_char(partner); /* inserted right after, also advances it */
            (*cx)--;               /* leave the cursor between the pair */
            return 1;
        }
    }

    return 0;
}

int brackets_find_match(int cy, int cx, int *out_y, int *out_x) {
    char c = lines[cy][cx];
    char open, close;
    int forward;

    switch (c) {
        case '(': open = '('; close = ')'; forward = 1; break;
        case ')': open = '('; close = ')'; forward = 0; break;
        case '{': open = '{'; close = '}'; forward = 1; break;
        case '}': open = '{'; close = '}'; forward = 0; break;
        case '[': open = '['; close = ']'; forward = 1; break;
        case ']': open = '['; close = ']'; forward = 0; break;
        default:  return 0;
    }

    int depth = 0;
    int y = cy, x = cx;

    if (forward) {
        while (y < count) {
            const char *ln = lines[y];
            int len = (int)strlen(ln);
            for (; x < len; x++) {
                if (ln[x] == open) depth++;
                else if (ln[x] == close) {
                    depth--;
                    if (depth == 0) { *out_y = y; *out_x = x; return 1; }
                }
            }
            y++;
            x = 0;
        }
    } else {
        while (y >= 0) {
            const char *ln = lines[y];
            for (; x >= 0; x--) {
                if (ln[x] == close) depth++;
                else if (ln[x] == open) {
                    depth--;
                    if (depth == 0) { *out_y = y; *out_x = x; return 1; }
                }
            }
            y--;
            if (y >= 0) x = (int)strlen(lines[y]) - 1;
        }
    }

    return 0;
}
