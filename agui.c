#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "agui.h"

void setColor(int fg, int bg) {
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
}

void setEffect(int effect) {
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

void homeCursor() {
    printf("\033[H");
}

void clearScreen() {
    printf("\033[H\033[0J");
}

void mvClearEol(int row, int col) {
    printf("\033[%d;%dH\033[K", row, col);
}

void clearEol() {
    printf("\033[K");
}

void hideCursor() {
    printf("\033[?25l");
}

void unhideCursor() {
    printf("\033[?25h");
}

void mvCursor(int row, int col) {
    char cmd[128];

    sprintf(cmd, "\033[%d;%dH", row, col);
    printf("%s", cmd);
}

void mvText(int row, int col, char *txt, ...) {
    va_list args;
    va_start(args, txt);

    mvCursor(row, col);
    vprintf(txt, args);

    va_end(args);
}

void setAll(int fg, int bg, int effect) {
    if (effect > 0)
        setEffect(effect);
    if (fg > 0 && bg > 0)
        setColor(fg, bg);
}
