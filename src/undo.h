#ifndef UNDO_H
#define UNDO_H

/*
 * undo.c/.h — line-level undo history. Compiled only when FEATURE_UNDO=1
 * (see config.h).
 *
 * Every edit that touches the line buffer (lines/count, declared in
 * main.c) is expressed as one of three primitive ops, recorded by the
 * call site right *before* it makes the change (so the "old" text can
 * still be read):
 *
 *   - a line's text changes in place  -> undo_record_line_changed()
 *   - a new line is inserted          -> undo_record_line_inserted()
 *   - an existing line is removed     -> undo_record_line_deleted()
 *
 * One user-visible action (one keypress) can produce several of these
 * back to back (e.g. Enter changes the current line and inserts a new
 * one; a paste can insert several). Call undo_next_step() once per
 * handled key, before dispatching it, so however many ops that key
 * produces, they all revert together on a single Ctrl+Z.
 *
 * History is a fixed-size ring buffer (UNDO_MAX steps worth of ops, see
 * config.h) — the oldest ops are dropped once it's full, so undo depth
 * is bounded rather than growing with the file.
 */

void undo_record_line_changed(int line, const char *old_text, int cy, int cx);
void undo_record_line_inserted(int line, int cy, int cx);
void undo_record_line_deleted(int line, const char *old_text, int cy, int cx);

/* Call once per handled key, before acting on it, so its ops form their
 * own step (see above). */
void undo_next_step(void);

/* Reverts every op from the most recent step, restoring cy/cx (via the
 * out params) to what they were right before that step began. Returns
 * 1 if something was undone, 0 if there's no history left. */
int undo_perform(int *cy, int *cx);

#endif
