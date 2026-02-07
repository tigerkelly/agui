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

// Home cursor
void aguiHomeCursor() {
    printf("\033[H");
}

// Clear screen
void aguiClearScreen() {
    printf("\033[H\033[0J");
}

// Move to position and clear to end of line
void aguiMvClearEol(int row, int col) {
    printf("\033[%d;%dH\033[K", row, col);
}

// Enable blinking cursor
void aguiBlinkCursor() {
        printf("\033[?12h");
}

// Disable blinking cursor
void aguiUnblinkCursor() {
        printf("\033[?12l");
}

// Move forward N columns
void aguiForward(int numCols) {
        printf("\033[%dC", numCols);
}

// Move backward N columns
void aguiBackward(int numCols) {
        printf("\033[%dC", numCols);
}

// Move to column N
void aguiMvColumn(int colNum) {
        printf("\033[%dG", colNum);
}

// Move curosr up N lines
void aguiMvUp(int numLines) {
    if (numLines > 0)
        printf("\033[%dA", numLines);
}

// Move cursor down N lines
void aguiMvDown(int numLines) {
    if (numLines > 0)
        printf("\033[%dB", numLines);
}

// Clear to EOL from current position
void aguiClearEol() {
    printf("\033[K");
}

// Hide cursor
void aguiHideCursor() {
    printf("\033[?25l");
}

// unhide cursor
void aguiUnhideCursor() {
    printf("\033[?25h");
}

// Move cursor
void aguiMvCursor(int row, int col) {
    char cmd[128];

    sprintf(cmd, "\033[%d;%dH", row, col);
    printf("%s", cmd);
}

// Move cursor and print text
void aguiMvText(int row, int col, char *txt, ...) {
    va_list args;
    va_start(args, txt);

    aguiMvCursor(row, col);
    vprintf(txt, args);

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
}

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
}

void aguiHorizLine(int row, int col, int width, bool useSingleLine) {
        int r = row;
        int c = col;

        UnicodeChar *u = NULL;

        if (useSingleLine == true)
                u = single_line;
        else
                u = double_line;

        for (int i = 0; i < width; i++) {
                aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
                c++;
        }
}

void aguiVertLine(int row, int col, int height, bool useSingleLine) {
        int r = row;
        int c = col;

        UnicodeChar *u = NULL;

        if (useSingleLine == true)
                u = single_line;
        else
                u = double_line;

        for (int i = 0; i < height; i++) {
                aguiMvText(r, c, "%s", u[VERT_LINE].symbol);
                r++;
        }
}

void aguiBlockBox(int row, int col, int width, int height, bool useHalfBlock) {
        int r = row;
        int c = col;

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

void aguiBoxLeft(int row, int col, int height, bool useSingleLine) {
        int r = row;

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

void aguiMvShape(int row, int col, int shape) {
        aguiMvText(row, col, "%s", geometric_shapes[shape].symbol);
}

void aguiShape(int shape) {
        printf("%s", geometric_shapes[shape].symbol);
}

void aguiMvArrow(int row, int col, int arrow) {
        aguiMvText(row, col, "%s", arrows[arrow].symbol);
}

void aguiArrow(int arrow) {
        printf("%s", arrows[arrow].symbol);
}

// Card suits
void aguiMvCard(int row, int col, int card) {
        aguiMvText(row, col, "%s", card_suits[card].symbol);
}

// Card suits
void aguiCard(int card) {
        printf("%s", card_suits[card].symbol);
}

void aguiMvMusic(int row, int col, int note) {
        aguiMvText(row, col, "%s", music_symbols[note].symbol);
}

void aguiMusic(int note) {
        printf("%s", music_symbols[note].symbol);
}

void aguiMvMath(int row, int col, int symbol) {
        aguiMvText(row, col, "%s", math_symbols[symbol].symbol);
}

void aguiMath(int symbol) {
        printf("%s", math_symbols[symbol].symbol);
}

void aguiMvCurrency(int row, int col, int sign) {
        aguiMvText(row, col, "%s", currency_symbols[sign].symbol);
}

void aguiCurrency(int sign) {
        printf("%s", currency_symbols[sign].symbol);
}
