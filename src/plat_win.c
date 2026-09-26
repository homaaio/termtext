#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "settings.h"

static HANDLE hin, hout;
static DWORD orig_in_mode, orig_out_mode;
static UINT orig_out_cp;

int plat_init(void) {
    hin = GetStdHandle(STD_INPUT_HANDLE);
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hin == INVALID_HANDLE_VALUE || hout == INVALID_HANDLE_VALUE) return 0;

    if (!GetConsoleMode(hin, &orig_in_mode)) return 0;
    if (!GetConsoleMode(hout, &orig_out_mode)) return 0;

    /* Switch the console's OUTPUT code page to UTF-8 so non-ASCII bytes
     * (Cyrillic and other multi-byte UTF-8 text in the edited file)
     * render correctly instead of turning into the "unknown character"
     * diamond glyph. Left as-is: the INPUT code page (SetConsoleCP) —
     * ReadConsoleInputA below reads raw bytes regardless of it, and
     * changing it isn't needed to fix display. Restored in
     * plat_shutdown(). */
    orig_out_cp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);

    DWORD in_mode = orig_in_mode;
    in_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    /* Turn off line-buffered input/echo and system Ctrl+C handling —
     * otherwise the console would "eat" Ctrl+C as a signal instead of
     * passing it through as a normal keypress (used for "Copy"). */
    in_mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    if (!SetConsoleMode(hin, in_mode)) return 0;

    DWORD out_mode = orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hout, out_mode)) return 0;

    plat_set_cursor_style(settings.cursor_blink);
    return 1;
}

void plat_shutdown(void) {
    plat_show_cursor(1);
    plat_set_color(PLAT_COLOR_DEFAULT);
    printf("\033[2J\033[H");
    fflush(stdout);

    SetConsoleMode(hin, orig_in_mode);
    SetConsoleMode(hout, orig_out_mode);
    SetConsoleOutputCP(orig_out_cp);
}

void plat_get_size(int *rows, int *cols) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hout, &info)) {
        *rows = 24;
        *cols = 80;
        return;
    }
    *cols = info.srWindow.Right - info.srWindow.Left + 1;
    *rows = info.srWindow.Bottom - info.srWindow.Top + 1;
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

int plat_read_key(void) {
    /* Read input events directly (instead of _getch()) so we can see
     * modifier state (Shift) for the arrow keys — needed for text
     * selection. Plain characters, including control characters
     * (Ctrl+letter), still come through uChar.AsciiChar as before.
     * UTF-8 (Cyrillic letters) is read byte-by-byte — main.c already
     * accounts for this. */
    INPUT_RECORD ir;
    DWORD read;
    for (;;) {
        if (!ReadConsoleInputA(hin, &ir, 1, &read) || read == 0) return EOF;
        if (ir.EventType != KEY_EVENT) continue;
        KEY_EVENT_RECORD *k = &ir.Event.KeyEvent;
        if (!k->bKeyDown) continue;

        int shift = (k->dwControlKeyState & SHIFT_PRESSED) != 0;
        switch (k->wVirtualKeyCode) {
            case VK_UP:    return shift ? PLAT_KEY_SHIFT_UP    : PLAT_KEY_UP;
            case VK_DOWN:  return shift ? PLAT_KEY_SHIFT_DOWN  : PLAT_KEY_DOWN;
            case VK_LEFT:  return shift ? PLAT_KEY_SHIFT_LEFT  : PLAT_KEY_LEFT;
            case VK_RIGHT: return shift ? PLAT_KEY_SHIFT_RIGHT : PLAT_KEY_RIGHT;
            default: break;
        }
        if (k->uChar.AsciiChar != 0) return (unsigned char)k->uChar.AsciiChar;
    }
}

/*
 * OS clipboard via the Win32 Clipboard API. Text is exchanged as UTF-8
 * with the rest of the program and converted to/from CF_UNICODETEXT at
 * the boundary — the same UTF-8 handling this file already applies to
 * console output (see the CP_UTF8 note in plat_init).
 */
void plat_clipboard_set(const char *text) {
    if (text == NULL) return;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (wlen <= 0) return;

    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();

    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(WCHAR));
    if (hmem != NULL) {
        WCHAR *dst = (WCHAR *)GlobalLock(hmem);
        if (dst != NULL) {
            MultiByteToWideChar(CP_UTF8, 0, text, -1, dst, wlen);
            GlobalUnlock(hmem);
            /* Ownership of hmem passes to the clipboard on success — do
             * not free it ourselves either way. */
            SetClipboardData(CF_UNICODETEXT, hmem);
        }
    }
    CloseClipboard();
}

char *plat_clipboard_get(void) {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return NULL;
    if (!OpenClipboard(NULL)) return NULL;

    char *out = NULL;
    HGLOBAL hmem = GetClipboardData(CF_UNICODETEXT);
    if (hmem != NULL) {
        WCHAR *wtext = (WCHAR *)GlobalLock(hmem);
        if (wtext != NULL) {
            int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                out = malloc(len);
                if (out != NULL) {
                    WideCharToMultiByte(CP_UTF8, 0, wtext, -1, out, len, NULL, NULL);
                }
            }
            GlobalUnlock(hmem);
        }
    }
    CloseClipboard();
    return out;
}
