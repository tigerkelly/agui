#ifndef AGUI_H
#define AGUI_H

#define SHM_NAME "/nms_shm"
#define MAX_PROTOCOLS 256

enum {
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BRIGHT_BLACK,
    COLOR_BRIGHT_RED,
    COLOR_BRIGHT_GREEN,
    COLOR_BRIGHT_YELLOW,
    COLOR_BRIGHT_BLUE,
    COLOR_BRIGHT_MAGENTA,
    COLOR_BRIGHT_CYAN,
    COLOR_BRIGHT_WHITE,
    COLOR_DEFAULT
};

enum {
    TEXT_BOLD,
    TEXT_DIM,
    TEXT_ITALIC,
    TEXT_UNDERLINE,
    TEXT_BLINKING,
    TEXT_REVERSE,
    TEXT_HIDDEN,
    TEXT_STRIKE,
    TEXT_DOUBLE,        // Underline
    TEXT_NORMAL,
    TEXT_NOTITALIC,
    TEXT_NOTUNDERLINED,
    TEXT_NOTBLINKING,
    TEXT_NOTREVERSE,
    TEXT_NOTHIDDEN,
    TEXT_NOTSTRIKE
};

enum {
    DRAW_VERT_LINE,
    DRAW_HORIZ_LINE,
    DRAW_TOP_LEFT,          // top left corner
    DRAW_TOP_RIGHT,         // top right corner
    DRAW_BOTTOM_LEFT,       // bottom left corner
    DRAW_BOTTOM_RIGHT,      // bottom right corner
    DRAW_LEFT_T,
    DRAW_RIGHT_T,
    DRAW_TOP_T,
    DRAW_BOTTOM_T,
    DRAW_CROSS
};

void aguiSetColor(int fg, int bg);
void aguiSetEffect(int effect);
void aguiHomeCursor();
void aguiClearScreen();
void aguiMvClearEol(int row, int col);
void aguiClearEol();
void aguiHideCursor();
void aguiUnhideCursor();
void aguiMvCursor(int row, int col);
void aguiMvText(int row, int col, char *txt, ...);
void aguiSetAll(int fg, int bg, int effect);


#endif
