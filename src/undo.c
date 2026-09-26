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

void undo_next_step(void) {
    cur_step++;
}

static void push(OpType type, int line, const char *old_text, int cy, int cx) {
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

    /* Ops are undone most-recent-first (LIFO) — the exact reverse of
     * the order they were made in, which is what keeps line indices
     * valid throughout (e.g. an insert must come back out before an
     * earlier change to the line it split off from is restored). */
    while (n > 0 && hist[n - 1].step == step) {
        UndoOp *op = &hist[n - 1];
        switch (op->type) {
            case OP_CHANGED:
                free(lines[op->line]);
                lines[op->line] = op->old_text ? op->old_text : strdup("");
                op->old_text = NULL; /* ownership moved into lines[] */
                break;
            case OP_INSERTED:
                delete_line(op->line);
                break;
            case OP_DELETED:
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
