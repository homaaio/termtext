#ifndef PLATFORM_H
#define PLATFORM_H

/*
 * Общий платформенный интерфейс. Никаких системных заголовков здесь —
 * только объявления и константы. Реализации: plat_unix.c, plat_win.c,
 * plat_mcu.c.
 */

/* Инициализация / завершение работы платформы (терминал, дисплей и т.п.) */
int  plat_init(void);      /* 0 при ошибке, иначе не 0 */
void plat_shutdown(void);  /* вернуть исходное состояние */

/* Экран */
void plat_get_size(int *rows, int *cols);
void plat_clear(void);
void plat_clear_line(int row);
void plat_move_cursor(int row, int col);
void plat_set_inverse(int on);
void plat_show_cursor(int visible);
void plat_set_cursor_style(int blink);

/*
 * Цвет текста — используется модулем подсветки синтаксиса (см.
 * FEATURE_HIGHLIGHT в config.h и src/highlight.c). Если модуль подсветки
 * отключён при сборке, эта функция просто не вызывается, но остаётся
 * частью интерфейса, чтобы каждой платформе не нужно было знать о
 * FEATURE_HIGHLIGHT напрямую.
 */
enum {
    PLAT_COLOR_DEFAULT = 0,
    PLAT_COLOR_KEYWORD,
    PLAT_COLOR_TYPE,
    PLAT_COLOR_STRING,
    PLAT_COLOR_COMMENT,
    PLAT_COLOR_NUMBER,
    PLAT_COLOR_PREPROC
};
void plat_set_color(int color);

/* Вывод / ввод */
void plat_putc(char c);
void plat_flush(void);
int  plat_read_key(void); /* обычные символы — как есть, стрелки — константы ниже */

/* Специальные клавиши */
#define PLAT_KEY_UP    1000
#define PLAT_KEY_DOWN  1001
#define PLAT_KEY_RIGHT 1002
#define PLAT_KEY_LEFT  1003
#define PLAT_KEY_ESC   27

/*
 * Стрелки с зажатым Shift — используются модулем выделения текста (см.
 * FEATURE_SELECTION в config.h и src/selection.c). Если модуль выделения
 * отключён, main.c просто обрабатывает их как обычные стрелки, поэтому
 * реализации plat_*.c всегда могут их возвращать без #ifdef.
 */
#define PLAT_KEY_SHIFT_UP    1010
#define PLAT_KEY_SHIFT_DOWN  1011
#define PLAT_KEY_SHIFT_RIGHT 1012
#define PLAT_KEY_SHIFT_LEFT  1013

#endif
