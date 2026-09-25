# termtext

A minimalist terminal text editor written in C, with a platform abstraction
layer (`platform.h`) so the core editor logic is portable — including,
with some extra work described below, to targets that have no operating
system at all.

## Features

- Open and edit text files
- Move the cursor with arrow keys
- Insert, delete, split and join lines
- **Text selection** — hold `Shift` and use the arrow keys to select a
  range, then cut/copy/paste it with an internal clipboard (`Ctrl+X`,
  `Ctrl+C`, `Ctrl+V`) that works even on platforms with no OS clipboard
- **Syntax highlighting** — a lightweight, keywords-and-literals-only
  highlighter for C/C++, Rust, Python and assembly; toggle it on/off in
  settings
- Save (`Ctrl+S`) and "save as" (`Ctrl+O`)
- Status line for messages
- Alternate screen — the terminal returns to its previous state on exit
- Feature modules that can be compiled out to shrink the binary (see
  "Modules and binary size" below)

## Project layout

    src/main.c               editor logic, platform-independent
    src/config.h              feature flags (FEATURE_HIGHLIGHT, FEATURE_SELECTION) and buffer limits
    src/settings.c/h          core settings storage (~/.config/tt/settings.conf)
    src/settings_highlight.c/h  highlight on/off setting (only built if FEATURE_HIGHLIGHT=1)
    src/highlight.c/h         syntax highlighting (only built if FEATURE_HIGHLIGHT=1)
    src/selection.c/h         text selection + internal clipboard (only built if FEATURE_SELECTION=1)
    src/version.h             version string
    src/platform.h            platform interface (plat_*)
    src/plat_unix.c           Linux/macOS implementation (termios + ANSI)
    src/plat_win.c            Windows implementation (Console API + VT sequences)
    src/plat_mcu.c            skeleton for microcontrollers (display + buttons)

## Build

    make            # same as `make unix`
    make unix       # Linux/macOS, gcc
    make macos      # Linux/macOS, clang
    make win        # cross-compile for Windows via MinGW (x86_64-w64-mingw32-gcc)

Or pick the platform layer directly: `make tt PLAT=win CC=x86_64-w64-mingw32-gcc`.
`PLAT=mcu` is a template only — it won't produce a runnable firmware until
the TODOs in `src/plat_mcu.c` (display driver, button GPIO) are filled in
for your specific chip. See "Running without an OS" below.

## Modules and binary size

Syntax highlighting and text selection are each a self-contained module
that can be left out of the build entirely — not just disabled at
runtime, but not compiled or linked in at all, so its keyword tables,
buffer-copy logic and clipboard don't take up space in the binary:

    make tt FEATURE_HIGHLIGHT=0                       # no syntax highlighting
    make tt FEATURE_SELECTION=0                       # no selection / clipboard
    make minimal                                      # both off — same as above combined
    make tiny                                         # minimal + -Os + dead-code stripping

Feature flags default to `1` (see `src/config.h`) and are passed to the
compiler with `-D`, so `#if FEATURE_HIGHLIGHT` / `#if FEATURE_SELECTION`
blocks compile away cleanly on either side. On this machine, a release
build of `tt` came out at roughly:

| Build             | Size (unix, gcc -O2) |
|-------------------|-----------------------|
| `make unix` (full)| ~46 KB |
| `make minimal`    | ~27 KB |
| `make tiny`        | ~18 KB |

Exact numbers depend on your compiler, libc and platform — use
`make size` after any build to check yours (it runs `ls -la` and `size`
on the resulting binary).

`EXTRA_CFLAGS` lets you pass anything else through, which is mainly
useful for the buffer limits described next:

    make tt EXTRA_CFLAGS="-DMAX_LINES=200 -DMAX_LEN=128"

`MAX_LINES` and `MAX_LEN` (also in `src/config.h`) bound the editor's
static line buffer; the PC defaults (1000 lines × 1024 bytes) are
generous, but on a memory-constrained target you'll want to shrink them
(see below).

## Install

    sudo make install

After that, `tt` is available from any directory.

## Uninstall

    sudo make uninstall

## Usage

    tt <file>

For example:

    tt notes.txt

If the file does not exist, it will be created on first save.

## Keybindings

| Keys | Action |
|---|---|
| Arrows | Move cursor |
| `Shift` + Arrows | Extend selection (`FEATURE_SELECTION`) |
| Regular characters | Insert (replaces selection, if any) |
| Backspace | Delete character / join with previous line / delete selection |
| Enter | Split line (replaces selection, if any) |
| `Ctrl+C` | Copy selection to the internal clipboard |
| `Ctrl+X` | Cut selection to the internal clipboard |
| `Ctrl+V` | Paste from the internal clipboard |
| `Ctrl+S` | Save |
| `Ctrl+O` | Save as (prompts for name) |
| `Ctrl+E` | Scroll down |
| `Ctrl+Y` | Scroll up |
| `Ctrl+Q` | Quit |

Selection is anchored where you first press `Shift`+Arrow and follows the
cursor from there; any other key (besides `Ctrl+C`/`Ctrl+X`, which act on
it) clears it. The clipboard is internal to termtext, not the OS
clipboard — this is what lets cut/copy/paste work identically on unix,
Windows, and a bare-metal MCU build alike.

## Settings

Settings are stored in `~/.config/tt/settings.conf` (or
`$XDG_CONFIG_HOME/tt/settings.conf`) and managed from the command line:

    tt --settings                    # print all settings
    tt --settings <key>               # print one setting
    tt --settings <key> <value>       # change and save a setting

| Key            | Values          | Default | Meaning |
|----------------|-----------------|---------|---------|
| `tab_width`    | `1`–`16`        | `4`     | Spaces inserted per Tab |
| `smart_tab`    | `true`/`false`  | `true`  | Tab fills to the next tab stop instead of always inserting `tab_width` spaces |
| `lines`        | `true`/`false`  | `true`  | Show line numbers in the left margin |
| `cursor_blink` | `true`/`false`  | `true`  | Blinking terminal cursor |
| `highlight`    | `true`/`false`  | `true`  | Syntax highlighting on/off (only present if built with `FEATURE_HIGHLIGHT=1`) |

Syntax highlighting is picked per file by its extension (`.c/.h/.cpp/.hpp`
→ C/C++, `.rs` → Rust, `.py` → Python, `.s/.asm` → assembly); anything
else is shown unhighlighted. It recognizes keywords, a handful of basic
types, string/char literals, numbers, line comments and (for C/C++)
preprocessor lines — enough to break up a wall of monochrome text without
being a real parser. It does not track constructs across lines (e.g. a
C-style block comment spanning several lines will not be colored
correctly past its first line) — a deliberate "20% effort, 80% of the
visual benefit" tradeoff rather than a full-blown syntax engine.

## Running without an OS

termtext's core (`main.c`, `settings.c`, `highlight.c`, `selection.c`) is
plain, freestanding-friendly C: it never talks to the terminal, a
filesystem driver, or any OS service directly. Every place it needs to
touch actual hardware — the screen, the keyboard/buttons, colors — it
calls one of the `plat_*` functions declared in `platform.h`. Swapping
`plat_unix.c`/`plat_win.c` for `plat_mcu.c` is the entire porting story;
nothing else in the codebase needs to change.

**What "without an OS" means here, precisely:** no kernel, no scheduler,
no filesystem, no terminal driver. It does *not* mean "without a C
library" — `main.c` still uses `malloc`/`free`, `strdup`, `snprintf` and
friends, and `settings.c`/`save_current()` still call `fopen`/`fprintf`,
which need *some* implementation to back them (a small heap, and either a
real filesystem or a stub that simply reports failure). This is the
normal embedded meaning of "runs without an OS": a freestanding target
with a minimal C runtime (e.g. newlib/newlib-nano cross-compiled for your
MCU), not literally zero library code. If your target has no filesystem
at all, you can stub `save_current()`/the initial `fopen` to talk to
whatever storage you do have (SPI flash, an SD card driver, etc.) instead
— that's a change local to `main.c`'s two `fopen` call sites, not to the
platform layer.

`src/plat_mcu.c` is the starting point for a hardware port, but it is a
**skeleton, not a working driver** — this repository has no specific chip
or display to target, so the actual register-level/SDK code has to come
from you. What it already gives you:

- Every function `platform.h` requires, with a `TODO` at each point where
  real hardware access belongs (SPI/display init, GPIO setup, character
  drawing, button debouncing).
- `plat_set_color()` wired in as a hook: fill in a `PLAT_COLOR_*` → RGB
  color table and use it in `plat_putc()` if you want highlighting on the
  device (or leave it a no-op and build with `FEATURE_HIGHLIGHT=0`).
- A comment at `BTN_ENTER` sketching how to add a "Shift" equivalent
  (e.g. a long-press) if you want selection on a buttons-only device —
  otherwise build with `FEATURE_SELECTION=0` and skip it.

To bring up a real port:

1. Pick your SDK/toolchain (e.g. an ARM `gcc` cross-compiler + vendor
   SDK, or Arduino-style build for ESP32) and get *any* "blink an LED" or
   "print to display" program building and flashing — that's outside
   this project's scope, but is a hard prerequisite.
2. Fill in `plat_init`/`plat_shutdown` (display + GPIO bring-up),
   `plat_get_size` (your display's character grid), `plat_clear`/
   `plat_clear_line`/`plat_move_cursor`/`plat_putc` (draw a character
   cell, respecting `plat_set_inverse`/`plat_set_color` if used), and
   `plat_read_key` (debounce your buttons and return `PLAT_KEY_*`).
3. Shrink `MAX_LINES`/`MAX_LEN` in `config.h` (or via `-D`) to fit your
   RAM budget — the default 1000×1024 is sized for a PC, not a
   microcontroller. If that's still too much (each open line costs a
   separate heap allocation, `strlen(line)+1` bytes, plus a `char*` slot
   in the `MAX_LINES`-sized pointer array), the next step is replacing
   the flat `lines[]` array with a bounded linked list — a bigger change,
   left as future work.
4. Decide what "save" means on your device (flash page write? SD card
   file? nothing, RAM-only?) and adjust the two `fopen` call sites in
   `main.c` accordingly if you don't have a real filesystem.
5. Build with the modules you actually want (`FEATURE_HIGHLIGHT`,
   `FEATURE_SELECTION`) — see "Modules and binary size" — since flash
   space is usually at more of a premium on a microcontroller than on a
   PC. `make mcu` builds with both off as a sane starting point.

None of this is hidden behind abstractions you'd need to reverse-engineer
first: `platform.h` is under 60 lines, and everything a port needs to
implement is listed there with a one-line comment each.

## Known limitations

- Syntax highlighting is per-line and stateless — see "Settings" above.
- Selection extends by character/line, not by word; there's no
  select-all shortcut.
- The internal clipboard holds one item and is not shared with the
  system clipboard or between separate `tt` processes — by design, so it
  keeps working with `FEATURE_SELECTION=1` on a target with no OS
  clipboard to share with.
- `plat_mcu.c` is a template, not a tested driver for any specific board.
