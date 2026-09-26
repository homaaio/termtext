#ifndef BRACKETS_H
#define BRACKETS_H

/*
 * brackets.h/.c — smart brackets (FEATURE_BRACKETS in config.h):
 * auto-closing pairs and matching-bracket highlighting. Compiled only
 * when FEATURE_BRACKETS=1.
 *
 * Like selection.c, this module works directly on the editor's line
 * buffer (lines/count) and the cursor position, declared in main.c.
 */

/* Returns the partner character for an auto-pairable opening character
 * ('(' -> ')', '"' -> '"', etc.), or 0 if c doesn't start a pair. */
char bracket_partner(char c);

/* True for the 3 bracket-pair characters used for match highlighting:
 * ( ) { } [ ]. Quotes are excluded — "the matching quote" isn't a
 * well-defined nesting concept the way bracket depth is. */
int bracket_is_bracket(char c);

/* Handles typing character c at the cursor (*cy,*cx):
 *  - if c is a closing character and the character right after the
 *    cursor already is that same character (presumably the partner we
 *    auto-inserted earlier), just steps over it instead of inserting a
 *    duplicate;
 *  - if c opens a pair ( ( [ { " ' ) and the cursor is at the end of the
 *    line or before a non-word character, inserts c followed by its
 *    partner and leaves the cursor between them;
 *  - otherwise does nothing.
 * Returns 1 if it handled the character itself (the buffer was
 * modified), 0 if the caller should still do its own plain insert. */
int brackets_smart_insert(int c, int *cy, int *cx);

/* Finds the bracket matching the one at (cy,cx) — which must contain one
 * of ( ) { } [ ] — by scanning forward or backward and tracking the
 * nesting depth of that bracket type. Brackets inside strings/comments
 * aren't special-cased (same "good enough, not a full parser" tradeoff
 * as the highlighter). Returns 1 and fills out_y/out_x (dereferenced) if
 * a match was found, 0 otherwise. */
int brackets_find_match(int cy, int cx, int *out_y, int *out_x);

#endif
