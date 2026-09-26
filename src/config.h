#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h — shared build settings and module switches.
 *
 * Each module can be disabled right at build time to shrink the
 * binary: the matching .c files simply don't make it into the Makefile's
 * source list, and all of their code (keyword tables, clipboard buffer,
 * etc.) never gets linked into the final file.
 *
 *   make tt FEATURE_HIGHLIGHT=0                                       # no syntax highlighting
 *   make tt FEATURE_SELECTION=0                                       # no selection / clipboard
 *   make tt FEATURE_BRACKETS=0                                        # no smart brackets
 *   make tt FEATURE_FIND=0                                            # no find / find & replace
 *   make tt FEATURE_UNDO=0                                            # no undo
 *   make tt FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 FEATURE_BRACKETS=0 FEATURE_FIND=0 FEATURE_UNDO=0 # minimal build (see `make minimal`)
 *
 * The values below are what you get building with no parameters
 * (make tt), or opening the project in an IDE that doesn't know about
 * the Makefile. The real value always comes from outside via -D (see
 * Makefile).
 */

#ifndef FEATURE_HIGHLIGHT
#define FEATURE_HIGHLIGHT 1   /* syntax highlighting: src/highlight.c, src/settings_highlight.c */
#endif

#ifndef FEATURE_SELECTION
#define FEATURE_SELECTION 1   /* text selection + internal clipboard: src/selection.c */
#endif

#ifndef FEATURE_BRACKETS
#define FEATURE_BRACKETS 1    /* smart brackets: auto-close pairs + matching-bracket highlight: src/brackets.c, src/settings_brackets.c */
#endif

#ifndef FEATURE_FIND
#define FEATURE_FIND 1        /* find (Ctrl+F) and find & replace (Ctrl+H): src/find.c */
#endif

#ifndef FEATURE_UNDO
#define FEATURE_UNDO 1        /* undo (Ctrl+Z): src/undo.c */
#endif

/*
 * Editor buffer limits. On PC (unix/win) the default values aren't
 * critical. When porting to a microcontroller (see plat_mcu.c and the
 * README's "Running without an OS" section) they almost certainly need
 * shrinking to fit available RAM — overridden the same way, via -D:
 *
 *   make tt PLAT=mcu FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 FEATURE_BRACKETS=0 \
 *           EXTRA_CFLAGS="-DMAX_LINES=200 -DMAX_LEN=128"
 */
#ifndef MAX_LINES
#define MAX_LINES 1000
#endif
#ifndef MAX_LEN
#define MAX_LEN 1024
#endif

/*
 * Undo history depth (src/undo.c, only present if FEATURE_UNDO=1): a
 * fixed-size ring buffer of this many line-edit steps, so undo depth is
 * bounded rather than growing with the file. Overridden the same way as
 * MAX_LINES/MAX_LEN, via -D — shrink it on a memory-constrained target.
 */
#ifndef UNDO_MAX
#define UNDO_MAX 300
#endif

#endif
