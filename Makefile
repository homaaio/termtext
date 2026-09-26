CC = gcc
CFLAGS = -Wall -O2
PREFIX = /usr/local

PLAT ?= unix

# --- Modules --------------------------------------------------------
# Disable modules you don't need to shrink the binary:
#   make tt FEATURE_HIGHLIGHT=0
#   make tt FEATURE_SELECTION=0
#   make tt FEATURE_BRACKETS=0
#   make minimal          # all three modules off in one command
FEATURE_HIGHLIGHT ?= 1
FEATURE_SELECTION ?= 1
FEATURE_BRACKETS ?= 1

# Extra compiler flags for fine-tuning (buffer limits, flags for a
# specific MCU, etc.), see config.h and the README ("Running without an
# OS"):
#   make tt PLAT=mcu EXTRA_CFLAGS="-DMAX_LINES=200 -DMAX_LEN=128 -Os"
EXTRA_CFLAGS ?=

FEATURE_FLAGS = -DFEATURE_HIGHLIGHT=$(FEATURE_HIGHLIGHT) -DFEATURE_SELECTION=$(FEATURE_SELECTION) -DFEATURE_BRACKETS=$(FEATURE_BRACKETS)
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

ifeq ($(FEATURE_BRACKETS),1)
SRC += src/brackets.c src/settings_brackets.c
COMMON_DEPS += src/brackets.h src/settings_brackets.h
endif

tt: $(SRC) $(COMMON_DEPS)
	$(CC) $(ALL_CFLAGS) -o tt $(SRC)

unix: ; $(MAKE) tt PLAT=unix
macos: ; $(MAKE) tt PLAT=unix CC=clang
win: ; $(MAKE) tt PLAT=win CC=x86_64-w64-mingw32-gcc

# Template for a microcontroller port: see src/plat_mcu.c and the
# README ("Running without an OS") — doesn't give you working firmware
# on its own until the TODOs are filled in for the actual hardware. The
# highlight/selection/brackets modules are off by default since most
# MCUs are better off with a compact binary anyway; enable them
# explicitly if needed (FEATURE_HIGHLIGHT=1 FEATURE_SELECTION=1
# FEATURE_BRACKETS=1).
mcu: ; $(MAKE) tt PLAT=mcu FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 FEATURE_BRACKETS=0

# Minimal PC build: all three optional modules off — the smallest
# binary that still edits text.
minimal: ; $(MAKE) tt FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 FEATURE_BRACKETS=0

# Same, but with extra size optimization and dead-code removal at link
# time — usually a noticeable win on top of just disabling the modules.
tiny:
	$(MAKE) tt EXTRA_CFLAGS="-Os -ffunction-sections -fdata-sections" \
	           FEATURE_HIGHLIGHT=0 FEATURE_SELECTION=0 FEATURE_BRACKETS=0 \
	           CFLAGS="-Wall -Wl,--gc-sections -s"

install: tt
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 tt $(DESTDIR)$(PREFIX)/bin/tt

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/tt

# Show the current binary size — handy for comparing builds with
# different module sets (make size, make minimal size, make tiny size, ...).
size: tt
	@ls -la tt
	@size tt 2>/dev/null || true

clean:
	rm -f tt tt.exe

.PHONY: unix macos win mcu minimal tiny install uninstall clean size
