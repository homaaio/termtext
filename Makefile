CC = gcc
CFLAGS = -Wall -O2
PREFIX = /usr/local

PLAT ?= unix

# --- Модули -------------------------------------------------------------
# Отключайте модули, которые не нужны, чтобы уменьшить бинарник:
#   make tt FEATURE_HIGHLIGHT=0
#   make tt FEATURE_SELECTION=0
#   make minimal          # оба модуля выключены одной командой
FEATURE_HIGHLIGHT ?= 1
FEATURE_SELECTION ?= 1

# Дополнительные флаги компилятора для тонкой настройки (лимиты буфера,
# флаги под конкретный МК и т.п.), см. config.h и README ("Работа без ОС"):
#   make tt PLAT=mcu EXTRA_CFLAGS="-DMAX_LINES=200 -DMAX_LEN=128 -Os"
EXTRA_CFLAGS ?=

FEATURE_FLAGS = -DFEATURE_HIGHLIGHT=$(FEATURE_HIGHLIGHT) -DFEATURE_SELECTION=$(FEATURE_SELECTION)
ALL_CFLAGS = $(CFLAGS) $(FEATURE_FLAGS) $(EXTRA_CFLAGS)

SRC = src/main.c src/settings.c src/plat_$(PLAT).c
COMMON_DEPS = src/settings.h src/version.h src/platform.h src/config.h

ifeq ($(FEATURE_HIGHLIGHT),1)
SRC += src/highlight.c src/settings_highlight.c
COMMON_DEPS += src/highlight.h src/settings_highlight.h
endif

ifeq ($(FEATURE_SELECTION),1)
SRC += src/selection.c
COMMON_DEPS += src/selection.h
endif

tt: $(SRC) $(COMMON_DEPS)
	$(CC) $(ALL_CFLAGS) -o tt $(SRC)

unix: ; $(MAKE) tt PLAT=unix
macos: ; $(MAKE) tt PLAT=unix CC=clang
win: ; $(MAKE) tt PLAT=win CC=x86_64-w64-mingw32-gcc

# Заготовка под микроконтроллер: см. src/plat_mcu.c и README ("Работа без
# ОС") — сама по себе не даёт рабочую прошивку, пока не заполнены TODO под
# конкретное железо. Модули подсветки/выделения выключены по умолчанию,
# т.к. на большинстве МК бинарник и так стоит держать компактным; при
# необходимости включите их явно (FEATURE_HIGHLIGHT=1 FEATURE_SELECTION=1).
mcu: ; $(MAKE) tt PLAT=mcu FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0

# Минимальная сборка ПК-версии: оба необязательных модуля выключены —
# самый маленький бинарник, который ещё редактирует текст.
minimal: ; $(MAKE) tt FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0

# То же самое, но с дополнительной оптимизацией под размер и удалением
# неиспользуемого кода на этапе линковки — обычно даёт заметный выигрыш
# сверх одного лишь выключения модулей.
tiny:
	$(MAKE) tt EXTRA_CFLAGS="-Os -ffunction-sections -fdata-sections" \
	           FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 \
	           CFLAGS="-Wall -Wl,--gc-sections -s"

install: tt
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 tt $(DESTDIR)$(PREFIX)/bin/tt

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/tt

# Показать текущий размер бинарника — удобно сравнивать сборки с разными
# наборами модулей (make size, make minimal size, make tiny size, ...).
size: tt
	@ls -la tt
	@size tt 2>/dev/null || true

clean:
	rm -f tt tt.exe

.PHONY: unix macos win mcu minimal tiny install uninstall clean size
