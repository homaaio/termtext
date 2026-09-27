#ifndef GITDIFF_H
#define GITDIFF_H

/*
 * gitdiff.c/.h — "git gutter": tracks which lines of the file being
 * edited differ from the last commit (HEAD), so draw() in main.c can
 * mark them in the line-number column with an orange "M" (as in `git
 * status --short`) instead of the usual "~" separator, the way VS
 * Code's colored gutter bar flags changed lines. Compiled only when
 * FEATURE_GIT=1 (see config.h).
 *
 * Shells out to the system's `git` — there's no bundled git
 * implementation, and none is needed: `git show`/`git diff --no-index`
 * do all the actual diffing. Silently does nothing (no lines marked) if
 * `git` isn't installed or the file isn't inside a git repo.
 *
 * gitdiff_refresh() diffs the current in-memory buffer (lines/count in
 * main.c), not just what's on disk, so the gutter reflects unsaved
 * edits too — but it isn't recomputed on every keystroke (spawning
 * `git` that often would make typing feel laggy). Call it after loading
 * a file and after every save; that's enough to stay accurate for how
 * this editor is used, at the cost of not updating on every single
 * keystroke like an IDE plugin might.
 */

/* Re-runs the diff for `path` against HEAD and rebuilds the changed-line
 * map from the editor's current buffer. Cheap to call occasionally
 * (file load, after save) but spawns a couple of `git` subprocesses, so
 * avoid calling it on every keystroke. */
void gitdiff_refresh(const char *path);

/* Was line `line_1based` (1-based, matching the on-screen line number)
 * added or changed since HEAD, as of the last gitdiff_refresh() call?
 * Always 0 before the first refresh, outside a git repo, or if git
 * isn't installed. */
int gitdiff_line_changed(int line_1based);

#endif
