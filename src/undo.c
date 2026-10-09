#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "undo.h"

/* The editor's line buffer — defined in main.c. */
extern char *lines[MAX_LINES];
extern int   count;
extern int   modified;
extern void  insert_line(int at, char *s);
extern void  delete_line(int at);

typedef enum { OP_CHANGED, OP_INSERTED, OP_DELETED } OpType;

typedef struct {
    int    step;
    OpType type;
    int    line;
    char  *old_text; /* owned copy; unused (NULL) for OP_INSERTED */
    int    cy, cx;    /* cursor to restore to if this op's step is undone */
} UndoOp;

static UndoOp hist[UNDO_MAX];
static int n = 0;        /* ops currently stored, oldest at index 0 */
static int cur_step = 0;

/* Redo history: filled by undo_perform() with the inverse of every op it
 * reverts, in the order it reverted them — so popping from the end
 * replays the original edit order. */
typedef enum { RE_SET, RE_INSERT, RE_DELETE } RedoType;

typedef struct {
    int      step;
    RedoType type;
    int      line;
    char    *text;   /* owned; NULL for RE_DELETE */
    int      cy, cx; /* cursor to restore after redoing this op's step */
} RedoOp;

static RedoOp rhist[UNDO_MAX];
static int rn = 0;
static int replaying = 0; /* set while redo_perform() pushes undo ops, so they don't wipe the redo history */

static void redo_clear(void) {
    for (int i = 0; i < rn; i++) free(rhist[i].text);
    rn = 0;
}

static void redo_push(int step, RedoType type, int line, char *text, int cy, int cx) {
    if (rn >= UNDO_MAX) return; /* can't happen (see undo_perform), but never overflow */
    rhist[rn].step = step;
    rhist[rn].type = type;
    rhist[rn].line = line;
    rhist[rn].text = text;
    rhist[rn].cy = cy;
    rhist[rn].cx = cx;
    rn++;
}

void undo_next_step(void) {
    cur_step++;
}

static void push(OpType type, int line, const char *old_text, int cy, int cx) {
    if (!replaying) redo_clear(); /* a fresh edit makes the old redo history meaningless */
    if (n >= UNDO_MAX) {
        /* History full — drop the oldest op to make room. Dropping from
         * the middle of a step would leave it half-undoable, but the
         * oldest op is always the start of the oldest step, so this
         * only ever removes a whole step at a time, one op at a time. */
        free(hist[0].old_text);
        memmove(&hist[0], &hist[1], (UNDO_MAX - 1) * sizeof(UndoOp));
        n--;
    }
    hist[n].step = cur_step;
    hist[n].type = type;
    hist[n].line = line;
    hist[n].old_text = old_text ? strdup(old_text) : NULL;
    hist[n].cy = cy;
    hist[n].cx = cx;
    n++;
}

void undo_record_line_changed(int line, const char *old_text, int cy, int cx) {
    push(OP_CHANGED, line, old_text, cy, cx);
}

void undo_record_line_inserted(int line, int cy, int cx) {
    push(OP_INSERTED, line, NULL, cy, cx);
}

void undo_record_line_deleted(int line, const char *old_text, int cy, int cx) {
    push(OP_DELETED, line, old_text, cy, cx);
}

int undo_perform(int *cy, int *cx) {
    if (n == 0) return 0;

    int step = hist[n - 1].step;
    int restore_cy = 0, restore_cx = 0;
    int redo_cy = *cy, redo_cx = *cx; /* where the cursor is now = where redo should leave it */

    /* Ops are undone most-recent-first (LIFO) — the exact reverse of
     * the order they were made in, which is what keeps line indices
     * valid throughout (e.g. an insert must come back out before an
     * earlier change to the line it split off from is restored). */
    while (n > 0 && hist[n - 1].step == step) {
        UndoOp *op = &hist[n - 1];
        switch (op->type) {
            case OP_CHANGED:
                /* the text being replaced is what redo must put back */
                redo_push(step, RE_SET, op->line, lines[op->line], redo_cy, redo_cx);
                lines[op->line] = op->old_text ? op->old_text : strdup("");
                op->old_text = NULL; /* ownership moved into lines[] */
                break;
            case OP_INSERTED:
                redo_push(step, RE_INSERT, op->line, strdup(lines[op->line]), redo_cy, redo_cx);
                delete_line(op->line);
                break;
            case OP_DELETED:
                redo_push(step, RE_DELETE, op->line, NULL, redo_cy, redo_cx);
                insert_line(op->line, op->old_text ? op->old_text : strdup(""));
                op->old_text = NULL; /* ownership moved into lines[] */
                break;
        }
        /* The earliest op in the step (reached last here) carries the
         * cursor position from right before the step began. */
        restore_cy = op->cy;
        restore_cx = op->cx;
        free(op->old_text); /* NULL when ownership moved above; harmless otherwise */
        n--;
    }

    *cy = restore_cy;
    *cx = restore_cx;
    modified = 1;
    return 1;
}

int redo_perform(int *cy, int *cx) {
    if (rn == 0) return 0;

    int step = rhist[rn - 1].step;
    int restore_cy = *cy, restore_cx = *cx;

    /* The undo ops pushed below all belong to the current step (the
     * Ctrl+R keypress), so one Ctrl+Z reverts the whole redo again. */
    replaying = 1;
    while (rn > 0 && rhist[rn - 1].step == step) {
        RedoOp *op = &rhist[rn - 1];
        switch (op->type) {
            case RE_SET: {
                char *cur = lines[op->line];
                push(OP_CHANGED, op->line, cur, *cy, *cx);
                lines[op->line] = op->text ? op->text : strdup("");
                op->text = NULL; /* ownership moved into lines[] */
                free(cur);
                break;
            }
            case RE_INSERT:
                insert_line(op->line, op->text ? op->text : strdup(""));
                op->text = NULL;
                push(OP_INSERTED, op->line, NULL, *cy, *cx);
                break;
            case RE_DELETE:
                push(OP_DELETED, op->line, lines[op->line], *cy, *cx);
                delete_line(op->line);
                break;
        }
        restore_cy = op->cy;
        restore_cx = op->cx;
        free(op->text);
        rn--;
    }
    replaying = 0;

    *cy = restore_cy;
    *cx = restore_cx;
    modified = 1;
    return 1;
}
