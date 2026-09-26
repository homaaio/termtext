#ifndef FIND_H
#define FIND_H

/*
 * find.c/.h — plain-text search and search-and-replace over the editor's
 * line buffer. Compiled only when FEATURE_FIND=1 (see config.h).
 *
 * Ctrl+H (find & replace, see main.c) is built on top of the very same
 * find_next()/find_replace_all() used by Ctrl+F (find) — there's
 * deliberately no second, separate search implementation.
 *
 * Works on the editor's line buffer (lines/count), declared in main.c.
 */

/* Searches for the next occurrence of query, starting just after the
 * cursor position (*cy, *cx). On a match, updates the cursor position
 * to the match's start and returns 1. If wrap is non-zero and no match
 * turns up before the end of the document, wraps around to the top and
 * keeps looking until every line has been checked once. Returns 0 if
 * nothing matches. */
int find_next(const char *query, int *cy, int *cx, int wrap);

/* Replaces every occurrence of query with replacement, across the whole
 * document. Returns the number of replacements made. */
int find_replace_all(const char *query, const char *replacement);

/* Remembers/returns the last search query typed via Ctrl+F or Ctrl+H,
 * so answering either prompt with an empty line repeats/reuses it. */
void        find_set_last(const char *query);
const char *find_get_last(void);

#endif
