#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include "platform.h"
#include "settings.h"

static struct termios orig_termios;
static int have_orig = 0;
static int use_altscreen = 0;

int plat_init(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return 0;
    have_orig = 1;

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return 0;

    /* The Linux console (TERM=linux) doesn't support the alt screen */
    const char *term = getenv("TERM");
    use_altscreen = (term != NULL && strcmp(term, "linux") != 0);
    if (use_altscreen) printf("\033[?1049h");

    printf("\033]12;#ffffff\033\\"); /* cursor color, if the terminal supports it */
    plat_set_cursor_style(settings.cursor_blink);
    fflush(stdout);
    return 1;
}

void plat_shutdown(void) {
    plat_show_cursor(1);
    plat_set_color(PLAT_COLOR_DEFAULT);
    if (use_altscreen) {
        printf("\033[?1049l");
    } else {
        printf("\033[2J\033[H");
    }
    fflush(stdout);

    if (have_orig) tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void plat_get_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0 || ws.ws_col == 0) {
        *rows = 24;
        *cols = 80;
        return;
    }
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

void plat_clear(void) {
    printf("\033[2J\033[H");
}

void plat_clear_line(int row) {
    printf("\033[%d;1H\033[K", row);
}

void plat_move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

void plat_set_inverse(int on) {
    printf(on ? "\033[7m" : "\033[27m");
}

void plat_set_color(int color) {
    switch (color) {
        case PLAT_COLOR_KEYWORD:       printf("\033[94m"); break;
        case PLAT_COLOR_TYPE:          printf("\033[96m"); break;
        case PLAT_COLOR_STRING:        printf("\033[92m"); break;
        case PLAT_COLOR_COMMENT:       printf("\033[32m"); break;
        case PLAT_COLOR_NUMBER:        printf("\033[93m"); break;
        case PLAT_COLOR_PREPROC:       printf("\033[95m"); break;
        case PLAT_COLOR_ESCAPE:        printf("\033[36m"); break;
        case PLAT_COLOR_BRACKET_MATCH: printf("\033[33m"); break;
        case PLAT_COLOR_GIT_CHANGED:   printf("\033[38;5;208m"); break; /* 256-color orange */
        default:                       printf("\033[39m"); break;
    }
}

void plat_show_cursor(int visible) {
    printf(visible ? "\033[?25h" : "\033[?25l");
}

void plat_set_cursor_style(int blink) {
    printf(blink ? "\033[1 q" : "\033[2 q");
}

void plat_putc(char c) {
    putchar(c);
}

void plat_flush(void) {
    fflush(stdout);
}

/* Raw read() on the fd, NOT getchar(): stdio would slurp a whole
 * escape sequence into its own buffer on the first call, and select()
 * on the fd would then wrongly report "nothing pending". Blocks until
 * a byte arrives; returns EOF on end of input. */
static int read_byte(void) {
    unsigned char b;
    ssize_t n = read(STDIN_FILENO, &b, 1);
    return n == 1 ? (int)b : EOF;
}

/* Reads one more byte of a possible escape sequence, but gives up after
 * ms milliseconds instead of blocking forever. A real arrow/function-key
 * sequence arrives as one burst (all its bytes already queued by the
 * time we get here), while a lone press of the Escape key sends just
 * the single 0x1b byte and nothing else — so "nothing shows up within
 * ms" reliably means "that really was just Escape". Returns -1 on
 * timeout. */
static int read_byte_timeout(int ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    int r = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (r <= 0) return -1;
    return read_byte();
}

int plat_read_key(void) {
    int c = read_byte();
    if (c == PLAT_KEY_ESC) {
        /* Raw mode blocks on getchar() with no timeout (VMIN=1, VTIME=0
         * from cfmakeraw), so reading the rest of an escape sequence
         * with plain getchar() used to hang here on a lone Escape
         * keypress — waiting for a second byte that would never come,
         * then misreading whatever key came next as part of the
         * sequence instead of handling it normally (e.g. Escape not
         * cancelling the Ctrl+O "Save as" prompt). read_byte_timeout()
         * fixes that: no more bytes within 50ms means it really was
         * just Escape. */
        int c1 = read_byte_timeout(50);
        if (c1 == -1 || c1 != '[') return PLAT_KEY_ESC;
        int c2 = read_byte_timeout(50);
        if (c2 == -1) return PLAT_KEY_ESC;
        if (c2 == 'A') return PLAT_KEY_UP;
        if (c2 == 'B') return PLAT_KEY_DOWN;
        if (c2 == 'C') return PLAT_KEY_RIGHT;
        if (c2 == 'D') return PLAT_KEY_LEFT;
        /* rxvt/urxvt-style shift-arrows: CSI a/b/c/d (lowercase, no
         * leading "1;2" modifier form) — some non-xterm terminals send
         * this instead of the CSI-1;2-<letter> form below. Accepting
         * both means Shift+arrow selection works on more terminals. */
        if (c2 == 'a') return PLAT_KEY_SHIFT_UP;
        if (c2 == 'b') return PLAT_KEY_SHIFT_DOWN;
        if (c2 == 'c') return PLAT_KEY_SHIFT_RIGHT;
        if (c2 == 'd') return PLAT_KEY_SHIFT_LEFT;
        if (c2 == '1') {
            /* CSI 1 ; <mod> <letter> — arrow with a modifier (Shift/Ctrl/...) */
            int c3 = read_byte_timeout(50); /* ';' */
            if (c3 == ';') {
                int mod = read_byte_timeout(50);   /* '2' = Shift */
                int c5  = read_byte_timeout(50);   /* A/B/C/D */
                if (mod == '2') {
                    if (c5 == 'A') return PLAT_KEY_SHIFT_UP;
                    if (c5 == 'B') return PLAT_KEY_SHIFT_DOWN;
                    if (c5 == 'C') return PLAT_KEY_SHIFT_RIGHT;
                    if (c5 == 'D') return PLAT_KEY_SHIFT_LEFT;
                }
            }
        }
        return PLAT_KEY_ESC;
    }
    return c; /* Cyrillic letters arrive as two UTF-8 bytes — main.c accounts for this */
}

/*
 * OS clipboard, via whichever external helper is installed — there's no
 * portable libc/X11-free way to reach it otherwise. Each list is tried
 * once (first that exists wins) and the result is cached, so normal
 * Ctrl+C/X/V use costs one extra fork+exec, not a fresh search of every
 * tool on the list.
 */
static const char *clip_copy_cmds[] = {
    "wl-copy", "xclip -selection clipboard -in", "xsel --clipboard --input", "pbcopy", NULL
};
static const char *clip_paste_cmds[] = {
    "wl-paste -n", "xclip -selection clipboard -out", "xsel --clipboard --output", "pbpaste", NULL
};

static int clip_tool_exists(const char *cmd) {
    char prog[64], check[128];
    if (sscanf(cmd, "%63s", prog) != 1) return 0;
    snprintf(check, sizeof(check), "command -v %s >/dev/null 2>&1", prog);
    return system(check) == 0;
}

void plat_clipboard_set(const char *text) {
    static int idx = -2; /* -2 = not probed yet, -1 = no tool found */
    if (idx == -2) {
        idx = -1;
        for (int i = 0; clip_copy_cmds[i] != NULL; i++) {
            if (clip_tool_exists(clip_copy_cmds[i])) { idx = i; break; }
        }
    }
    if (idx < 0 || text == NULL) return;
    FILE *p = popen(clip_copy_cmds[idx], "w");
    if (p == NULL) return;
    fwrite(text, 1, strlen(text), p);
    pclose(p);
}

char *plat_clipboard_get(void) {
    static int idx = -2;
    if (idx == -2) {
        idx = -1;
        for (int i = 0; clip_paste_cmds[i] != NULL; i++) {
            if (clip_tool_exists(clip_paste_cmds[i])) { idx = i; break; }
        }
    }
    if (idx < 0) return NULL;

    FILE *p = popen(clip_paste_cmds[idx], "r");
    if (p == NULL) return NULL;

    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (buf == NULL) { pclose(p); return NULL; }

    int c;
    while ((c = fgetc(p)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *nb = realloc(buf, cap);
            if (nb == NULL) { free(buf); pclose(p); return NULL; }
            buf = nb;
        }
        buf[len++] = (char)c;
    }
    buf[len] = 0;
    pclose(p);

    if (len == 0) { free(buf); return NULL; }
    return buf;
}
