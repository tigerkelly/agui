# AGUI — ASCII GUI Library for C

**AGUI** is a lightweight C library for building terminal-based UIs using ANSI/VT100 escape codes and Unicode block/box-drawing characters. It is designed for *display-oriented* applications — dashboards, status monitors, and information panels — rather than interactive widget toolkits. The only keyboard event it captures is `Ctrl-C` to stop the program.

The project also ships **adraw**, a companion ncurses-based drawing tool for visually designing `.agui` screen layout files that can be loaded at runtime.

---

## Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [API Reference](#api-reference)
- [The adraw Tool](#the-adraw-tool)
- [The .agui File Format](#the-agui-file-format)
- [Examples](#examples)
- [License](#license)

---

## Features

- **Cursor control** — move, save, restore, hide/show, blink
- **Colors** — 17 foreground and background color constants (standard + bright variants + default)
- **Text effects** — bold, dim, italic, underline, blink, reverse, hidden, strikethrough, and their negations
- **Box drawing** — single-line, double-line, and ASCII-art (`+-|`) borders; full, partial, and block-character boxes
- **Unicode symbols** — geometric shapes, arrows, card suits, music notes, math symbols, currency signs
- **Screen management** — clear screen/line/EOS/BOS, scroll up/down, window title, screen-size query
- **Character insert/delete** — insert or delete characters and lines at arbitrary positions
- **Screen files** — load pre-designed layouts from `.agui` files at runtime
- **Debug logging** — `aguiDebug()` writes timestamped messages to a log file
- **adraw** — interactive ncurses designer for creating `.agui` layout files

---

## Project Structure

```
agui/
├── agui.c              # Core library implementation
├── agui.h              # Public API header
├── chartable.c         # Unicode character lookup tables (included by agui.c)
├── linkedList.c/.h     # Internal linked list used for screen-object management
├── parse.c             # Whitespace-delimited token parser
├── qparse.c            # Quoted-string-aware token parser
├── strqtok.c           # Quote-preserving strtok replacement
├── trimstr.c           # String whitespace trimmer
├── Makefile            # Builds libagui.a static library
│
├── agui_test/
│   ├── test_agui.c     # Demo program: exercises most library functions
│   └── Makefile
│
├── adraw_prg/
│   ├── adraw.c         # ncurses-based interactive screen designer
│   ├── parse.c
│   ├── qparse.c
│   ├── strqtok.c
│   └── Makefile        # Builds the `adraw` binary (links ncursesw)
│
└── adraw_test/
    ├── test_adraw.c    # Demo: loads a .agui file and renders it
    ├── test.agui       # Sample layout file
    └── Makefile
```

---

## Requirements

- C compiler with C99 support (`gcc` or `cc`)
- A terminal emulator that supports ANSI/VT100 escape codes (e.g., PuTTY, xterm, GNOME Terminal, iTerm2)
- `ncursesw` — only required to build the **adraw** designer tool

---

## Building

### Build the static library (`libagui.a`)

```sh
cd agui
make
```

This compiles `agui.c` and its supporting files into `libagui.a`.

Optional profiling build:

```sh
PROFILE=yes make
```

### Build the test program

```sh
cd agui_test
make
gcc test_agui.c -o test_agui -L../ -lagui
./test_agui
```

### Build the adraw designer

```sh
cd adraw_prg
make
# Installs binary to ~/bin/adraw
```

### Build the adraw screen-file test

```sh
cd adraw_test
make
gcc test_adraw.c -o test_adraw -L../ -lagui
./test_adraw
```

---

## Usage

1. Include `agui.h` in your source file.
2. Link against `libagui.a` when compiling.
3. Call `aguiBegin()` before any drawing calls and `aguiEnd()` when finished.

```c
#include "agui.h"

int main() {
    aguiBegin();
    aguiClearScreen();
    aguiHideCursor(true);

    aguiSetAll(C_BLUE, C_DEFAULT, TEXT_BOLD);
    aguiMvText(2, 10, "Hello, AGUI!");

    aguiBox(4, 10, 30, 8, SINGLE_BORDER);

    getchar();   // wait for keypress

    aguiClearScreen();
    aguiHideCursor(false);
    aguiEnd();
    return 0;
}
```

Compile:

```sh
gcc myapp.c -o myapp -L/path/to/agui -lagui
```

---

## API Reference

### Lifecycle

| Function | Description |
|---|---|
| `aguiBegin()` | Initialize the library (saves terminal state) |
| `aguiEnd()` | Restore terminal state and clean up |
| `char *aguiVersion()` | Returns the library version string (currently `"1.0.0"`) |

### Screen & Cursor

| Function | Description |
|---|---|
| `aguiClearScreen()` | Clear the entire screen |
| `aguiHomeCursor()` | Move cursor to top-left (row 1, col 1) |
| `aguiHideCursor(bool)` | Hide (`true`) or show (`false`) the cursor |
| `aguiBlinkCursor(bool)` | Enable or disable cursor blinking |
| `aguiSaveCursor()` | Save current cursor position |
| `aguiRestoreCursor()` | Restore previously saved cursor position |
| `aguiMvCursor(row, col)` | Move cursor to absolute position |
| `aguiForward(n)` | Move cursor forward n columns |
| `aguiBackward(n)` | Move cursor backward n columns |
| `aguiMvColumn(col)` | Move cursor to specific column on current row |
| `aguiMvUp(n)` | Move cursor up n rows |
| `aguiMvDown(n)` | Move cursor down n rows |
| `aguiCurrentPosition(RowCol *)` | Read current cursor row/col into a struct |
| `aguiScreenSize(WidthHeight *)` | Read terminal dimensions into a struct |
| `aguiWindowTitle(char *)` | Set the terminal window title bar text |

### Clearing

| Function | Description |
|---|---|
| `aguiClearEol()` | Clear from cursor to end of line |
| `aguiMvClearEol(row, col)` | Move then clear to end of line |
| `aguiClearEoS()` | Clear from cursor to end of screen |
| `aguiMvClearEoS(row, col)` | Move then clear to end of screen |
| `aguiClearBoS()` | Clear from cursor to beginning of screen |
| `aguiMvClearBoS(row, col)` | Move then clear to beginning of screen |
| `aguiScrollUp(n)` | Scroll screen up n lines |
| `aguiScrollDown(n)` | Scroll screen down n lines |

### Text & Color

| Function | Description |
|---|---|
| `aguiMvText(row, col, fmt, ...)` | Print formatted text at position (printf-style) |
| `aguiSetColor(fg, bg)` | Set foreground and background colors |
| `aguiSetEffect(effect)` | Apply a text effect |
| `aguiSetAll(fg, bg, effect)` | Set color and effect in one call |
| `aguiResetAllAttrib()` | Reset all colors and effects to defaults |

**Color constants** (`C_` prefix): `C_BLACK`, `C_RED`, `C_GREEN`, `C_YELLOW`, `C_BLUE`, `C_MAGENTA`, `C_CYAN`, `C_WHITE`, plus `C_BRIGHT_*` variants and `C_DEFAULT`.

**Effect constants** (`TEXT_` prefix): `TEXT_BOLD`, `TEXT_DIM`, `TEXT_ITALIC`, `TEXT_UNDERLINE`, `TEXT_BLINKING`, `TEXT_REVERSE`, `TEXT_HIDDEN`, `TEXT_STRIKE`, `TEXT_DOUBLE`, `TEXT_NORMAL`, and `TEXT_NOT*` negation variants.

### Box Drawing

Border type enum: `ASCII_BORDER`, `SINGLE_BORDER`, `DOUBLE_BORDER`

| Function | Description |
|---|---|
| `aguiBox(row, col, width, height, BorderType)` | Draw a complete box |
| `aguiBoxTop(row, col, width, BorderType)` | Draw only the top edge with corners |
| `aguiBoxBottom(row, col, width, BorderType)` | Draw only the bottom edge with corners |
| `aguiBoxLeft(row, col, height, BorderType)` | Draw only the left edge |
| `aguiBoxRight(row, col, height, BorderType)` | Draw only the right edge |
| `aguiHorizLine(row, col, width, BorderType)` | Draw a horizontal line |
| `aguiVertLine(row, col, height, BorderType)` | Draw a vertical line |
| `aguiBlockBox(row, col, width, height, BlockType)` | Draw a box using block characters |

Block type enum: `BLOCK_NONE`, `BLOCK_FULL`, `BLOCK_HALF`

### Symbols

Each symbol function has two forms: `aguiFoo(symbol)` draws at the current cursor position; `aguiMvFoo(row, col, symbol)` moves first.

| Category | Function pair | Constants |
|---|---|---|
| Geometric shapes | `aguiShape` / `aguiMvShape` | `B_SQUARE`, `W_CIRCLE`, `B_DIAMOND`, `BULLSEYE`, etc. |
| Arrows | `aguiArrow` / `aguiMvArrow` | `LEFT_ARROW`, `RIGHT_ARROW`, `UP_ARROW`, `DOWN_ARROW`, etc. |
| Card suits | `aguiCard` / `aguiMvCard` | `SPADE`, `CLUB`, `HEART`, `DIAMOND` |
| Music notes | `aguiMusic` / `aguiMvMusic` | `EIGHT_NOTE`, `BEAMED_EIGHT`, `FLAT_SIGN`, `SHARP_SIGN` |
| Math symbols | `aguiMath` / `aguiMvMath` | `M_INFINITY`, `M_SQUARE_ROOT`, `M_NOT_EQUAL`, etc. |
| Currency signs | `aguiCurrency` / `aguiMvCurrency` | `C_EURO_SIGN`, `C_BITCOIN_SIGN`, `C_YEN_SIGN`, etc. |

### Character Insert/Delete

| Function | Description |
|---|---|
| `aguiMvInsertChar(row, col, n)` | Insert n blank characters at position |
| `aguiMvDeleteChar(row, col, n)` | Delete n characters at position |
| `aguiMvInsertLine(row, col, n)` | Insert n blank lines at position |
| `aguiMvDeleteLine(row, col, n)` | Delete n lines at position |

### Screen Files & Debugging

| Function | Description |
|---|---|
| `aguiLoadScreen(char *filename)` | Parse and render a `.agui` layout file |
| `aguiDebug(char *fmt, ...)` | Write a printf-style message to the debug log |

---

## The adraw Tool

![adraw screenshot](adraw.png)

`adraw` is an interactive, ncurses-based screen designer for composing `.agui` layout files visually without writing code.

### Launch

```sh
adraw              # start with an empty canvas
adraw myscreen.agui  # open an existing layout file
```

### Key Bindings

| Key | Action |
|---|---|
| Arrow keys | Move cursor (also drags selected object) |
| `b` / `B` | Place single / double-line box (press twice to confirm) |
| `a` | Place ASCII box (`+-|`) |
| `h` / `H` | Single / double horizontal line |
| `\` | ASCII horizontal line |
| `v` / `V` | Single / double vertical line |
| `\|` | ASCII vertical line |
| `t` | Place text at cursor |
| `g` | Open glyph picker popup (arrows, PgUp/Dn, Enter to select) |
| `s` | Select object under cursor / confirm move |
| `d` | Delete object under cursor |
| `c` | Open color picker |
| `C` | Center selected object (`H`=horizontal, `V`=vertical, `B`=both) |
| `p` | Cycle panel background color |
| `w` | Save to file |
| `l` | Load from file |
| `q` | Quit (warns on unsaved changes) |
| `?` | Help screen |
| `Esc` | Cancel current action |

---

## The .agui File Format

`.agui` files are human-readable text files. Lines beginning with `##` are comments and are ignored on load. Each object occupies one directive line:

```
## row col width height color dir style "text"
## color: 0=Red 1=Green 2=Yellow 3=Blue 4=Magenta 5=Cyan 6=White 7=White(bright)
## dir:   0=vertical  1=horizontal
## style: 0=ascii  1=single  2=double

#@B  col  row  width  height  color  style    ## Box
#@L  col  row  length color   dir    style    ## Line
#@T  col  row  0      0       color  0  "text" ## Text or glyph
```

> **Note:** ncurses (used by adraw) is zero-relative, but the AGUI library is one-relative. `aguiLoadScreen()` adjusts coordinates automatically.

### Example `.agui` file

```
## Sample layout
#@B  16   5   6   8   7   0 ## Box: single-line, white
#@B  24   5   7   8   7   1 ## Box: double-line, white
#@L   9   2  16   7   0   0 ## Horizontal single line
#@L   9   5   8   7   1   0 ## Vertical single line
#@T  27   2   0   0   7   0 "Hello World"
```

Load it in your program:

```c
aguiBegin();
aguiClearScreen();
aguiLoadScreen("myscreen.agui");
getchar();
aguiEnd();
```

---

## Examples

### Colored text with a double-line border

```c
aguiBegin();
aguiClearScreen();
aguiHideCursor(true);

aguiSetAll(C_CYAN, C_DEFAULT, TEXT_BOLD);
aguiMvText(1, 2, "System Status");
aguiResetAllAttrib();

aguiBox(3, 2, 40, 10, DOUBLE_BORDER);
aguiMvText(5, 4, "CPU:    %3d%%", 42);
aguiMvText(6, 4, "Memory: %3d%%", 71);

getchar();
aguiClearScreen();
aguiHideCursor(false);
aguiEnd();
```

### Displaying symbols

```c
aguiMvText(10, 2, "Currency: ");
aguiCurrency(C_DOLLAR_SIGN);
aguiCurrency(C_EURO_SIGN);
aguiCurrency(C_BITCOIN_SIGN);

aguiMvText(11, 2, "Math: ");
aguiMath(M_INFINITY);
aguiMath(M_SQUARE_ROOT);
aguiMath(M_NOT_EQUAL);
```

---

## License

This project is distributed under the terms found in the [LICENSE](LICENSE) file.

---

*Created by Kelly Wiles. Library version 1.0.0.*
