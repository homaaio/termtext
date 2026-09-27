#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "gitdiff.h"

#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#define NULL_DEV "NUL"
#else
#define NULL_DEV "/dev/null"
#endif

/* The editor's line buffer — defined in main.c. Read-only here. */
extern char *lines[MAX_LINES];
extern int   count;

static unsigned char changed[MAX_LINES];
static int have_any = 0;

/* Splits "some/dir/file.c" into dir="some/dir" and base="file.c" (or
 * dir="." and base="file.c" if there's no directory component), so the
 * git commands below can run with -C dir and a plain filename — avoids
 * needing to resolve the file's path relative to the repo root. */
static void split_path(const char *path, char *dir, size_t dirsz, char *base, size_t basesz) {
    const char *slash = strrchr(path, '/');
#ifdef _WIN32
    const char *bslash = strrchr(path, '\\');
    if (!slash || (bslash && bslash > slash)) slash = bslash;
#endif
    if (slash) {
        size_t dlen = (size_t)(slash - path);
        if (dlen >= dirsz) dlen = dirsz - 1;
        memcpy(dir, path, dlen);
        dir[dlen] = 0;
        snprintf(base, basesz, "%s", slash + 1);
    } else {
        snprintf(dir, dirsz, ".");
        snprintf(base, basesz, "%s", path);
    }
}

static int in_git_repo(const char *dir) {
    char cmd[700];
    snprintf(cmd, sizeof(cmd),
             "git -C \"%s\" rev-parse --is-inside-work-tree 2>%s",
             dir, NULL_DEV);

    FILE *p = popen(cmd, "r");
    if (!p) return 0;
    char buf[16] = "";
    if (!fgets(buf, sizeof(buf), p)) buf[0] = 0;
    pclose(p);
    return strncmp(buf, "true", 4) == 0;
}

void gitdiff_refresh(const char *path) {
    memset(changed, 0, sizeof(changed));
    have_any = 0;

    char dir[512], base[256];
    split_path(path, dir, sizeof(dir), base, sizeof(base));

    if (!in_git_repo(dir)) return;

    char old_tmp[600], new_tmp[600];
    snprintf(old_tmp, sizeof(old_tmp), "%s/.tt.gitdiff.old.tmp", dir);
    snprintf(new_tmp, sizeof(new_tmp), "%s/.tt.gitdiff.new.tmp", dir);

    /* "Old" side: the file's content at HEAD, via git's cwd-relative
     * ":./name" object syntax (paired with -C dir) — no need to work out
     * the path relative to the repo root ourselves. If the file is new
     * or untracked this fails and old_tmp stays empty, which correctly
     * makes every current line show up as added below. */
    char ref[320], cmd[2048];
    snprintf(ref, sizeof(ref), "HEAD:./%s", base);
    snprintf(cmd, sizeof(cmd), "git -C \"%s\" show \"%s\" > \"%s\" 2>%s",
             dir, ref, old_tmp, NULL_DEV);
    if (system(cmd) == -1) { /* couldn't spawn a shell at all — leave old_tmp missing/empty */ }

    /* "New" side: the editor's current in-memory buffer, not just
     * what's on disk — so the gutter reflects unsaved edits too. */
    FILE *nf = fopen(new_tmp, "w");
    if (nf) {
        for (int i = 0; i < count; i++) fprintf(nf, "%s\n", lines[i]);
        fclose(nf);
    }

    /* --no-index compares two plain files directly, no repo/staging
     * involved; -U0 skips context lines, so every hunk's "+" side is
     * exactly the lines that were added or changed. */
    snprintf(cmd, sizeof(cmd),
             "git diff --no-color --no-index -U0 -- \"%s\" \"%s\" 2>%s",
             old_tmp, new_tmp, NULL_DEV);

    FILE *p = popen(cmd, "r");
    if (p) {
        char line[512];
        while (fgets(line, sizeof(line), p)) {
            if (line[0] != '@' || line[1] != '@') continue;

            /* "@@ -a,b +c,d @@ ..." — only the "+c,d" side matters; d
             * defaults to 1 when omitted (a single-line hunk). A hunk
             * with d==0 is a pure deletion with nothing to mark. */
            char *plus = strchr(line, '+');
            if (!plus) continue;
            int start = 0, cnt = 1;
            int n = sscanf(plus + 1, "%d,%d", &start, &cnt);
            if (n < 1) continue;
            if (n == 1) cnt = 1;

            for (int ln = start; ln < start + cnt && ln > 0; ln++) {
                if (ln - 1 < MAX_LINES) {
                    changed[ln - 1] = 1;
                    have_any = 1;
                }
            }
        }
        pclose(p);
    }

    remove(old_tmp);
    remove(new_tmp);
}

int gitdiff_line_changed(int line_1based) {
    if (!have_any) return 0;
    if (line_1based < 1 || line_1based > MAX_LINES) return 0;
    return changed[line_1based - 1];
}
