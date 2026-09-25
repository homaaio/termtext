/*
 * plat_mcu.c — платформенный слой для микроконтроллеров (ESP32, STM32 и т.п.)
 *
 * Это не порт терминала, а отдельная реализация: драйвер дисплея (например,
 * ILI9341 через SPI) + опрос кнопок вместо termios/ANSI. Замените заглушки
 * ниже на вызовы вашей библиотеки дисплея, GPIO и таймеров. Ни один из
 * остальных файлов (main.c, settings.c, highlight.c, selection.c) не
 * знает про конкретное железо — они видят только эти функции, объявленные
 * в platform.h. Подробный чек-лист для доведения этого шаблона до
 * работающей прошивки — в README, раздел "Работа без ОС".
 *
 * ВАЖНО (см. README, раздел "Работа без ОС"): статические массивы
 * lines[MAX_LINES][*] в main.c рассчитаны на ПК. Для реального порта
 * уменьшите MAX_LINES/MAX_LEN через config.h (или -D при сборке) под
 * доступную RAM, а если этого мало — замените хранение строк на связный
 * список с ограничением общего буфера. Это отдельная задача, не решаемая
 * одной лишь платформенной заглушкой.
 */

#include "platform.h"

/* TODO: подключить SDK и библиотеку дисплея, например:
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
/* TODO (FEATURE_SELECTION): если нужна поддержка выделения текста на
 * устройстве без клавиатуры — заведите отдельную кнопку/жест как аналог
 * Shift, например долгое удержание BTN_ENTER во время навигации стрелками,
 * и возвращайте из plat_read_key() PLAT_KEY_SHIFT_* вместо PLAT_KEY_*. */

static int cur_row = 0;
static int cur_col = 0;
static int inverse = 0;
static int cur_color = PLAT_COLOR_DEFAULT;

int plat_init(void) {
    /* TODO: инициализировать SPI/дисплей, например tft.begin();
     * TODO: настроить пины кнопок как входы с подтяжкой (pull-up) */
    return 1;
}

void plat_shutdown(void) {
    /* TODO: выключить дисплей, если нужно */
}

void plat_get_size(int *rows, int *cols) {
    *rows = MCU_ROWS;
    *cols = MCU_COLS;
}

void plat_clear(void) {
    /* TODO: очистить буфер/экран дисплея, например tft.fillScreen(BLACK); */
}

void plat_clear_line(int row) {
    /* TODO: закрасить одну строку на дисплее прямоугольником фона */
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
    /* TODO (FEATURE_HIGHLIGHT): сохранить color и учитывать его в
     * plat_putc при выборе цвета символа, например через таблицу
     * PLAT_COLOR_* -> RGB565. Если подсветка отключена сборкой
     * (FEATURE_HIGHLIGHT=0), эта функция не вызывается вовсе. */
    cur_color = color;
}

void plat_show_cursor(int visible) {
    /* На дисплее курсор обычно рисуется как часть символа (инверсией) —
     * отдельная сущность "видимость курсора" не нужна, можно оставить пустым. */
    (void)visible;
}

void plat_set_cursor_style(int blink) {
    /* Мигание курсора на дисплее реализуется таймером в plat_flush/основном
     * цикле, если потребуется. Пока не используется. */
    (void)blink;
}

void plat_putc(char c) {
    /* TODO: нарисовать символ c в позиции (cur_row, cur_col) с учётом
     * inverse и cur_color, например:
     * uint16_t fg = color_table[cur_color];
     * uint16_t bg = COLOR_BG;
     * if (inverse) { uint16_t t = fg; fg = bg; bg = t; }
     * tft.drawChar(cur_col * CHAR_W, cur_row * CHAR_H, c, fg, bg, 1);
     */
    (void)c;
    cur_col++;
}

void plat_flush(void) {
    /* TODO: если используется буферизация — отправить буфер на экран */
}

static int debounce(int pin) {
    /* TODO: простой антидребезг — считать пин несколько раз подряд с
     * небольшой задержкой между чтениями и вернуть 1, если кнопка
     * стабильно нажата всё это время. */
    (void)pin;
    return 0;
}

int plat_read_key(void) {
    if (debounce(BTN_UP))    return PLAT_KEY_UP;
    if (debounce(BTN_DOWN))  return PLAT_KEY_DOWN;
    if (debounce(BTN_LEFT))  return PLAT_KEY_LEFT;
    if (debounce(BTN_RIGHT)) return PLAT_KEY_RIGHT;
    if (debounce(BTN_ENTER)) return PLAT_KEY_ESC;
    return 0; /* ничего не нажато — основной цикл перерисует экран и продолжит опрос */
}
