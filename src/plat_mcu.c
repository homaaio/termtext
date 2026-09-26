/*
 * plat_mcu.c — platform layer for microcontrollers (ESP32, STM32, etc.)
 *
 * This isn't a terminal port, it's a separate implementation: a display
 * driver (e.g. ILI9341 over SPI) + button polling instead of
 * termios/ANSI. Replace the stubs below with calls into your display
 * library, GPIO and timers. None of the other files (main.c, settings.c,
 * highlight.c, selection.c) know anything about the specific hardware —
 * they only see these functions, declared in platform.h. A detailed
 * checklist for turning this template into working firmware is in the
 * README, "Running without an OS" section.
 *
 * IMPORTANT (see README, "Running without an OS"): the static arrays
 * lines[MAX_LINES][*] in main.c are sized for a PC. For a real port,
 * shrink MAX_LINES/MAX_LEN via config.h (or -D at build time) to fit
 * available RAM, and if that's not enough, replace the line storage with
 * a linked list bounded by a total buffer size. That's a separate task,
 * not something a platform stub alone can solve.
 */

#include "platform.h"

/* TODO: pull in the SDK and display library, e.g.:
 * #include "driver/spi_master.h"
 * #include "ili9341.h"
 */

#define MCU_ROWS 20
#define MCU_COLS 40

#define BTN_UP     0
#define BTN_DOWN   1
#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_ENTER  4
/* TODO (FEATURE_SELECTION): if you need text-selection support on a
 * device with no keyboard, add a dedicated button/gesture as a Shift
 * analog — e.g. holding BTN_ENTER while navigating with the arrows —
 * and return PLAT_KEY_SHIFT_* instead of PLAT_KEY_* from
 * plat_read_key(). */

static int cur_row = 0;
static int cur_col = 0;
static int inverse = 0;
static int cur_color = PLAT_COLOR_DEFAULT;

int plat_init(void) {
    /* TODO: initialize SPI/display, e.g. tft.begin();
     * TODO: configure the button pins as pulled-up inputs */
    return 1;
}

void plat_shutdown(void) {
    /* TODO: turn off the display, if needed */
}

void plat_get_size(int *rows, int *cols) {
    *rows = MCU_ROWS;
    *cols = MCU_COLS;
}

void plat_clear(void) {
    /* TODO: clear the display buffer/screen, e.g. tft.fillScreen(BLACK); */
}

void plat_clear_line(int row) {
    /* TODO: paint over one display row with a background rectangle */
    (void)row;
}

void plat_move_cursor(int row, int col) {
    cur_row = row;
    cur_col = col;
}

void plat_set_inverse(int on) {
    inverse = on;
}

void plat_set_color(int color) {
    /* TODO (FEATURE_HIGHLIGHT / FEATURE_BRACKETS): store color and use it
     * in plat_putc when choosing the glyph color, e.g. via a
     * PLAT_COLOR_* -> RGB565 table. Remember to also cover
     * PLAT_COLOR_ESCAPE and PLAT_COLOR_BRACKET_MATCH there if those
     * modules are enabled for this build. If a module is disabled at
     * build time (FEATURE_HIGHLIGHT=0 / FEATURE_BRACKETS=0), this
     * function is simply never called with its colors. */
    cur_color = color;
}

void plat_show_cursor(int visible) {
    /* On a display the cursor is usually drawn as part of the glyph
     * (via inversion) — a separate "cursor visibility" concept isn't
     * needed, this can stay empty. */
    (void)visible;
}

void plat_set_cursor_style(int blink) {
    /* Cursor blinking on the display would be implemented with a timer
     * in plat_flush/the main loop, if needed. Not used for now. */
    (void)blink;
}

void plat_putc(char c) {
    /* TODO: draw character c at (cur_row, cur_col), honoring inverse and
     * cur_color, e.g.:
     * uint16_t fg = color_table[cur_color];
     * uint16_t bg = COLOR_BG;
     * if (inverse) { uint16_t t = fg; fg = bg; bg = t; }
     * tft.drawChar(cur_col * CHAR_W, cur_row * CHAR_H, c, fg, bg, 1);
     */
    (void)c;
    cur_col++;
}

void plat_flush(void) {
    /* TODO: if buffering is used, push the buffer to the screen */
}

static int debounce(int pin) {
    /* TODO: simple debouncing — read the pin several times in a row with
     * a short delay between reads and return 1 if the button was held
     * down steadily the whole time. */
    (void)pin;
    return 0;
}

int plat_read_key(void) {
    if (debounce(BTN_UP))    return PLAT_KEY_UP;
    if (debounce(BTN_DOWN))  return PLAT_KEY_DOWN;
    if (debounce(BTN_LEFT))  return PLAT_KEY_LEFT;
    if (debounce(BTN_RIGHT)) return PLAT_KEY_RIGHT;
    if (debounce(BTN_ENTER)) return PLAT_KEY_ESC;
    return 0; /* nothing pressed — the main loop will redraw and keep polling */
}

/* No system clipboard on bare metal — Ctrl+C/X/V (FEATURE_SELECTION)
 * fall back to the internal clipboard in selection.c automatically. */
void plat_clipboard_set(const char *text) {
    (void)text;
}

char *plat_clipboard_get(void) {
    return NULL;
}
