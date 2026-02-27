/* Ascii escape code library for displaying information.
 * Create by Kelly Wiles
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "agui.h"

#include "chartable.c"

char *prgVersion = "1.0.0";

int currRow = 0;
int currCol = 0;

RowCol _savedCursor;

extern char *trimWhiteSpace(char *str);

// Set fg and bg color.
void aguiSetColor(int fg, int bg) {
    switch (fg) {
    case COLOR_BLACK:           printf("\033[30m"); break;
    case COLOR_RED:             printf("\033[31m"); break;
    case COLOR_GREEN:           printf("\033[32m"); break;
    case COLOR_YELLOW:          printf("\033[33m"); break;
    case COLOR_BLUE:            printf("\033[34m"); break;
    case COLOR_MAGENTA:         printf("\033[35m"); break;
    case COLOR_CYAN:            printf("\033[36m"); break;
    case COLOR_WHITE:           printf("\033[37m"); break;
    case COLOR_BRIGHT_BLACK:    printf("\033[90m"); break;
    case COLOR_BRIGHT_RED:      printf("\033[91m"); break;
    case COLOR_BRIGHT_GREEN:    printf("\033[92m"); break;
    case COLOR_BRIGHT_YELLOW:   printf("\033[93m"); break;
    case COLOR_BRIGHT_BLUE:     printf("\033[94m"); break;
    case COLOR_BRIGHT_MAGENTA:  printf("\033[95m"); break;
    case COLOR_BRIGHT_CYAN:     printf("\033[96m"); break;
    case COLOR_BRIGHT_WHITE:    printf("\033[97m"); break;
    case COLOR_DEFAULT:         printf("\033[39m"); break;
    }

    switch (bg) {
    case COLOR_BLACK:           printf("\033[40m"); break;
    case COLOR_RED:             printf("\033[41m"); break;
    case COLOR_GREEN:           printf("\033[42m"); break;
    case COLOR_YELLOW:          printf("\033[43m"); break;
    case COLOR_BLUE:            printf("\033[44m"); break;
    case COLOR_MAGENTA:         printf("\033[45m"); break;
    case COLOR_CYAN:            printf("\033[46m"); break;
    case COLOR_WHITE:           printf("\033[47m"); break;
    case COLOR_BRIGHT_BLACK:    printf("\033[100m"); break;
    case COLOR_BRIGHT_RED:      printf("\033[101m"); break;
    case COLOR_BRIGHT_GREEN:    printf("\033[102m"); break;
    case COLOR_BRIGHT_YELLOW:   printf("\033[103m"); break;
    case COLOR_BRIGHT_BLUE:     printf("\033[104m"); break;
    case COLOR_BRIGHT_MAGENTA:  printf("\033[105m"); break;
    case COLOR_BRIGHT_CYAN:     printf("\033[106m"); break;
    case COLOR_BRIGHT_WHITE:    printf("\033[107m"); break;
    case COLOR_DEFAULT:         printf("\033[49m"); break;
    }
}// Set text efects like boldness and italics.
void aguiSetEffect(int effect) {
    switch (effect) {
    case TEXT_BOLD:             printf("\033[1m"); break;
    case TEXT_DIM:              printf("\033[2m"); break;
    case TEXT_ITALIC:           printf("\033[3m"); break;
    case TEXT_UNDERLINE:        printf("\033[4m"); break;
    case TEXT_BLINKING:         printf("\033[5m"); break;
    case TEXT_REVERSE:          printf("\033[7m"); break;
    case TEXT_HIDDEN:           printf("\033[8m"); break;
    case TEXT_STRIKE:           printf("\033[9m"); break;
    case TEXT_DOUBLE:           printf("\033[21m"); break;
    case TEXT_NORMAL:           printf("\033[22m"); break;
    case TEXT_NOTITALIC:        printf("\033[23m"); break;
    case TEXT_NOTUNDERLINED:    printf("\033[23m"); break;
    case TEXT_NOTBLINKING:      printf("\033[25m"); break;
    case TEXT_NOTREVERSE:       printf("\033[27m"); break;
    case TEXT_NOTHIDDEN:        printf("\033[28m"); break;
    case TEXT_NOTSTRIKE:        printf("\033[29m"); break;
    }
}

char *aguiVersion() {
    return prgVersion;
}

// Home cursor
void aguiHomeCursor() {
    printf("\033[H");
    currRow = 0;
    currCol = 0;
}

// Clear screen
void aguiClearScreen() {
    printf("\033[H\033[0J");
    currRow = 0;
    currCol = 0;
}

// Clear to EOL from current position
void aguiClearEol() {
    printf("\033[K");
}

// Move to position and clear to end of line
void aguiMvClearEol(int row, int col) {
    printf("\033[%d;%dH\033[K", row, col);
    currRow = row;
    currCol = col;
}

// Enable blinking cursor
void aguiBlinkCursor(bool onOff) {
    if (onOff == true)
        printf("\033[?12h");
    else
        printf("\033[?12l");
}

// Move forward N columns
void aguiForward(int numCols) {
    printf("\033[%dC", numCols);
    currCol += numCols;
}

// Move backward N columns
void aguiBackward(int numCols) {
    printf("\033[%dD", numCols);
    currCol -= numCols;
}

// Move to column N
void aguiMvColumn(int colNum) {
    printf("\033[%dG", colNum);
    currCol = colNum;
}

// Move curosr up N lines
void aguiMvUp(int numLines) {
    if (numLines > 0) {
        printf("\033[%dA", numLines);
        currRow += numLines;
    }
}

// Move cursor down N lines
void aguiMvDown(int numLines) {
    if (numLines > 0) {
        printf("\033[%dB", numLines);
        currRow -= numLines;
    }
}

// Hide cursor
void aguiHideCursor(bool cursorOff) {
    if (cursorOff == true)
        printf("\033[?25l");
    else
        printf("\033[?25h");
}

// Clear from cursor to end of screen.
void aguiClearEoS() {
    printf("\033[0J");
}

void aguiMvClearEoS(int row, int col) {
    aguiMvCursor(row, col);
    printf("\033[0J");
    currRow = row;
    currCol = col;
}

// Clear from cursor to beinning of screen.
void aguiClearBoS() {
    printf("\033[1J");
}

void aguiMvClearBoS(int row, int col) {
    aguiMvCursor(row, col);
    printf("\033[1J");
    currRow = row;
    currCol = col;
}

// Move cursor
void aguiMvCursor(int row, int col) {
    char cmd[128];

    sprintf(cmd, "\033[%d;%dH", row, col);
    printf("%s", cmd);
    currRow = row;
    currCol = col;
}

void aguiSaveCursor() {
    _savedCursor.row = currRow;
    _savedCursor.col = currCol;
}

void aguiRestoreCursor() {
    aguiMvCursor(_savedCursor.row, _savedCursor.col);
    currRow = _savedCursor.row;
    currCol = _savedCursor.col;
}

void aguiResetAllAttrib() {
    printf("\033[0m");
}

void aguiScrollUp(int lines) {
    printf("\033[%dS", lines);
    currRow -= lines;
}

void aguiScrollDown(int lines) {
    printf("\033[%dT", lines);
    currRow += lines;
}

void aguiWindowTitle(char *title) {
    printf("\033]0;%s\077", title);
}

void aguiCurrentPosition(RowCol *rc) {
    rc->row = currRow;
    rc->col = currCol;
}

// Move cursor and print text
void aguiMvText(int row, int col, char *txt, ...) {
    va_list args;
    va_start(args, txt);

    aguiMvCursor(row, col);
    vprintf(txt, args);
    currRow = row;
    currCol = col;

    va_end(args);
}

// Set fg, bg amd effect of future text.
void aguiSetAll(int fg, int bg, int effect) {
    if (effect > 0)
        aguiSetEffect(effect);
    if (fg > 0 && bg > 0)
        aguiSetColor(fg, bg);
}

void aguiBox(int row, int col, int width, int height, bool useSingleLine) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;

    aguiMvText(r, c++, "%s", u[TOP_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
    aguiMvText(r, c, "%s", u[TOP_RIGHT].symbol);

    r = row + 1;
    for (int i = 1; i < height; i++) {
        aguiMvText(r, col, "%s", u[VERT_LINE].symbol);
        aguiMvText(r, col + width, "%s", u[VERT_LINE].symbol);
        r++;
    }

    c = col;
    aguiMvText(r, c++, "%s", u[BOTTOM_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
    aguiMvText(r, c, "%s", u[BOTTOM_RIGHT].symbol);
    currRow = row;
    currCol = col;
}

#if(0)
void aguiBoxCharLeft(int row, int col, bool useSingleLine) {
	UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;	
	
	aguiMvText(row, col, "%s", u[HORIZ_LINE].symbol);
    currRow = row;
    currCol = col;
}

void aguiBoxRight(int row, int col, bool useSingleLine) {
	UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;	
	
	aguiMvText(row, col, "%s", u[HORIZ_LINE].symbol);
    currRow = row;
    currCol = col;
}
#endif

void aguiBoxTop(int row, int col, int width, bool useSingleLine) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;

    aguiMvText(r, c++, "%s", u[TOP_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
    aguiMvText(r, c, "%s", u[TOP_RIGHT].symbol);
    currRow = row;
    currCol = col;
}

void aguiBoxBottom(int row, int col, int width, bool useSingleLine) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;

    aguiMvText(r, c++, "%s", u[BOTTOM_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
    aguiMvText(r, c, "%s", u[BOTTOM_RIGHT].symbol);
    currRow = row;
    currCol = col;
}

void aguiHorizLine(int row, int col, int width, bool useSingleLine, bool addEnds) {
    int r = row;
    int c = col;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;

    int i = 0;
    if (addEnds == true) {
        aguiMvText(r, c, "%s", u[LEFT_T].symbol);
        i++;
        width--;
        c++;
    }

    for (; i < width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
    if (addEnds == true) {
        aguiMvText(r, c, "%s", u[RIGHT_T].symbol);
        i++;
        width--;
    }
}

void aguiVertLine(int row, int col, int height, bool useSingleLine, bool addEnds) {
    int r = row;
    int c = col;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true)
        u = single_line;
    else
        u = double_line;

    int i = 0;
    if (addEnds == true) {
        aguiMvText(r, c, "%s", u[TOP_T].symbol);
        i++;
        height--;
        r++;
    }

    for (; i < height; i++) {
        aguiMvText(r, c, "%s", u[VERT_LINE].symbol);
        r++;
    }
    if (addEnds == true) {
        aguiMvText(r, c, "%s", u[BOTTOM_T].symbol);
        i++;
        height--;
    }
}

void aguiBlockBox(int row, int col, int width, int height, bool useHalfBlock) {
    int r = row;
    int c = col;
    currRow = row;
    currCol = col;

    if (useHalfBlock == true) {
        aguiMvText(r, c++, "%s", block_elements[BLK_LOWER_HALF].symbol);
        for (int i = 1; i < width; i++) {
            aguiMvText(r, c, "%s", block_elements[BLK_LOWER_HALF].symbol);
            c++;
        }
        aguiMvText(r, c, "%s", block_elements[BLK_LOWER_HALF].symbol);

        r = row + 1;
        for (int i = 1; i < height; i++) {
            aguiMvText(r, col, "%s", block_elements[BLK_LEFT_HALF].symbol);
            aguiMvText(r, col + width, "%s", block_elements[BLK_RIGHT_HALF].symbol);
            r++;
        }

        c = col;
        aguiMvText(r, c++, "%s", block_elements[BLK_UPPER_HALF].symbol);
        for (int i = 1; i < width; i++) {
            aguiMvText(r, c, "%s", block_elements[BLK_UPPER_HALF].symbol);
            c++;
        }
        aguiMvText(r, c, "%s", block_elements[BLK_UPPER_HALF].symbol);
    } else {
        aguiMvText(r, c++, "%s", block_elements[BLK_FULL].symbol);
        for (int i = 1; i < width; i++) {
            aguiMvText(r, c, "%s", block_elements[BLK_FULL].symbol);
            c++;
        }
        aguiMvText(r, c, "%s", block_elements[BLK_FULL].symbol);

        r = row + 1;
        for (int i = 1; i < height; i++) {
            aguiMvText(r, col, "%s", block_elements[BLK_FULL].symbol);
            aguiMvText(r, col + width, "%s", block_elements[BLK_FULL].symbol);
            r++;
        }

        c = col;
        aguiMvText(r, c++, "%s", block_elements[BLK_FULL].symbol);
        for (int i = 1; i < width; i++) {
            aguiMvText(r, c, "%s", block_elements[BLK_FULL].symbol);
            c++;
        }
        aguiMvText(r, c, "%s", block_elements[BLK_FULL].symbol);
    }

}

// Draw a single Horiz line at row/col.
void aguiBoxHoriz(int row, int col, bool useSingleLine) {
	int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[D_HORIZ_LINE].symbol);
}

// Draw a single vertical line at row/col
void aguiBoxVert(int row, int col, bool useSingleLine) {
	int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[D_VERT_LINE].symbol);
}

void aguiBoxLeft(int row, int col, int height, bool useSingleLine) {
    int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[TOP_LEFT].symbol);
    for (int i = 0; i < height; i++) {
        aguiMvText(r, col, "%s", u[VERT_LINE].symbol);
        r++;
    }
    aguiMvText(row + height, col, "%s", u[BOTTOM_LEFT].symbol);
}

void aguiBoxRight(int row, int col, int height, bool useSingleLine) {
    int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (useSingleLine == true) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[TOP_RIGHT].symbol);
    for (int i = 0; i < height; i++) {
        aguiMvText(r, col, "%s", u[VERT_LINE].symbol);
        r++;
    }
    aguiMvText(row + height, col, "%s", u[BOTTOM_RIGHT].symbol);
}
// Geometric shapes
void aguiMvShape(int row, int col, int shape) {
    aguiMvText(row, col, "%s", geometric_shapes[shape].symbol);
    currRow = row;
    currCol = col;
}

void aguiShape(int shape) {
    printf("%s", geometric_shapes[shape].symbol);
}

// Arrows
void aguiMvArrow(int row, int col, int arrow) {
    aguiMvText(row, col, "%s", arrows[arrow].symbol);
    currRow = row;
    currCol = col;
}

void aguiArrow(int arrow) {
    printf("%s", arrows[arrow].symbol);
}

// Card suits
void aguiMvCard(int row, int col, int card) {
    aguiMvText(row, col, "%s", card_suits[card].symbol);
    currRow = row;
    currCol = col;
}

// Card suits
void aguiCard(int card) {
    printf("%s", card_suits[card].symbol);
}
// Music
void aguiMvMusic(int row, int col, int note) {
    aguiMvText(row, col, "%s", music_symbols[note].symbol);
    currRow = row;
    currCol = col;
}

void aguiMusic(int note) {
    printf("%s", music_symbols[note].symbol);
}

// Math
void aguiMvMath(int row, int col, int symbol) {
    aguiMvText(row, col, "%s", math_symbols[symbol].symbol);
    currRow = row;
    currCol = col;
}

void aguiMath(int symbol) {
    printf("%s", math_symbols[symbol].symbol);
}

// Currency
void aguiMvCurrency(int row, int col, int sign) {
    aguiMvText(row, col, "%s", currency_symbols[sign].symbol);
    currRow = row;
    currCol = col;
}

void aguiCurrency(int sign) {
    printf("%s", currency_symbols[sign].symbol);
}

void aguiMvInsertChar(int row, int col, int numChars) {
    aguiMvCursor(row, col);
    printf("\033[{%d}I", numChars);
    currRow = row;
    currCol = col;
}

void aguiMvDeleteChar(int row, int col, int numChars) {
    aguiMvCursor(row, col);
    printf("\033[{%d}P", numChars);
    currRow = row;
    currCol = col;
}

void aguiMvInsertLine(int row, int col, int numLines) {
    aguiMvCursor(row, col);
    printf("\033[{%d}L", numLines);
    currRow = row;
    currCol = col;
}

void aguiMvDeleteLine(int row, int col, int numLines) {
    aguiMvCursor(row, col);
    printf("\033[{%d}M", numLines);
    currRow = row;
    currCol = col;
}
