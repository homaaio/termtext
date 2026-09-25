#ifndef SELECTION_H
#define SELECTION_H

/*
 * selection.h/.c — выделение текста (Shift+стрелки) и внутренний буфер
 * обмена (Ctrl+C/X/V). Компилируется только при FEATURE_SELECTION=1
 * (см. config.h).
 *
 * "Внутренний" буфер обмена намеренно не использует системный clipboard
 * ОС — это делает copy/cut/paste рабочими и на платформах без ОС
 * (см. README, раздел "Работа без ОС"), а заодно не требует от каждого
 * plat_*.c реализовывать ещё один системный API.
 *
 * Модуль работает поверх буфера строк редактора (lines/count), который
 * объявлен в main.c — оттуда же берутся insert_line/delete_line.
 */

typedef struct {
    int active;             /* есть ли "якорь" выделения */
    int anchor_y, anchor_x; /* точка, с которой начали выделять */
} Selection;

extern Selection sel;

/* Вызывается при первом Shift+стрелка: если выделение ещё не начато,
 * ставит якорь в текущей позиции курсора. */
void sel_start_if_needed(int cy, int cx);

/* Снять выделение (не трогая текст). */
void sel_clear(void);

/* Есть ли непустое выделение относительно текущей позиции курсора (cy,cx). */
int sel_has_range(int cy, int cx);

/* Нормализует выделение между якорем и текущим курсором (cy,cx). */
void sel_range(int cy, int cx, int *y0, int *x0, int *y1, int *x1);

/* Входит ли символ в позиции (y,x) в выделение относительно курсора (cy,cx). */
int sel_contains(int y, int x, int cy, int cx);

/* Удаляет выделённый текст из буфера строк, двигает курсор (*cy,*cx) в
 * начало бывшего выделения и снимает выделение. */
void sel_delete(int *cy, int *cx);

/* Возвращает malloc'нутую копию выделенного текста (строки разделены '\n'). */
char *sel_copy_text(int cy, int cx);

/* Внутренний буфер обмена: sel_clipboard_set забирает владение строкой. */
void        sel_clipboard_set(char *text);
const char *sel_clipboard_get(void);

#endif
