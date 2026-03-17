/* Asciiescape code library for displaying information.
 * Create by Kelly Wiles
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>

#include "agui.h"

#include "chartable.c"
#include "linkedList.c"

char *prgVersion = "1.0.0";

int currRow = 0;
int currCol = 0;
int screenRows = 0;
int screenCols = 0;

WidthHeight _wh;
RowCol _savedCursor;
LinkedList list;
FILE *_debugfd = NULL;

extern char *trimWhiteSpace(char *str);
char *strqtok (char *s1, const char *s2);
int qparse(char *str, const char *chrs, char **argz, int max_argz);

int parse(char *str, const char *chrs, char **argz, int max_argz);

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
}

// Set text efects like boldness and italics.
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

// NOTE: You can not query the screen to get characters on screen. So you
// can not restore the screen when the program ending.

char *aguiVersion() {
    return prgVersion;
}

void aguiBegin() {
	aguiScreenSize(&_wh);
}

void aguiEnd() {
	if (_debugfd != NULL)
		fclose(_debugfd);
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

void aguiBox(int row, int col, int width, int height, int sord) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH) {
        u = single_line;
    } else {
        u = double_line;
	}

    aguiMvText(r, c++, "%s", u[TOP_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c++, "%s", u[HORIZ_LINE].symbol);
    }
    aguiMvText(r, c, "%s", u[TOP_RIGHT].symbol);

    r = row + 1;
    for (int i = 2; i < height; i++) {
        aguiMvText(r, col, "%s", u[VERT_LINE].symbol);
        aguiMvText(r++, col + width, "%s", u[VERT_LINE].symbol);
    }

    c = col;
    aguiMvText(r, c++, "%s", u[BOTTOM_LEFT].symbol);
    for (int i = 1; i < width; i++) {
        aguiMvText(r, c++, "%s", u[HORIZ_LINE].symbol);
    }
    aguiMvText(r, c, "%s", u[BOTTOM_RIGHT].symbol);
    currRow = row;
    currCol = col;
}

void aguiBoxTop(int row, int col, int width, int sord) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH)
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

void aguiBoxBottom(int row, int col, int width, int sord) {
    int r = row;
    int c = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH)
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

void aguiHorizLine(int row, int col, int width, int sord) {
    int r = row;
    int c = col;
    currRow = row;
    currCol = col;

	// aguiDebug("width %d", width);

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH)
        u = single_line;
    else
        u = double_line;

    int i = 0;

    for (; i <= width; i++) {
        aguiMvText(r, c, "%s", u[HORIZ_LINE].symbol);
        c++;
    }
}

void aguiVertLine(int row, int col, int height, int sord) {
    int r = row;
    int c = col;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH)
        u = single_line;
    else
        u = double_line;

    int i = 0;

    for (; i < height; i++) {
        aguiMvText(r, c, "%s", u[VERT_LINE].symbol);
        r++;
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
void aguiBoxHoriz(int row, int col, int sord) {
	int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[D_HORIZ_LINE].symbol);
}

// Draw a single vertical line at row/col
void aguiBoxVert(int row, int col, int sord) {
	int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH) {
        u = single_line;
    } else {
        u = double_line;
    }

    aguiMvText(r++, col, "%s", u[D_VERT_LINE].symbol);
}

void aguiBoxLeft(int row, int col, int height, int sord) {
    int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH) {
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

void aguiBoxRight(int row, int col, int height, int sord) {
    int r = row;
    currRow = row;
    currCol = col;

    UnicodeChar *u = NULL;

    if (sord == S_GRAPH) {
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

void aguiScreenSize(WidthHeight *wh) {
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	wh->height = w.ws_row;
	wh->width = w.ws_col;
}

// Used to help debug library.  Writes messages to a log file.
void aguiDebug(char *fmt, ...) {
	va_list valist;

    va_start(valist, fmt);

	if (_debugfd == NULL) {
		_debugfd = fopen("agui.debug", "a+");

		if (_debugfd == NULL)
			return;
	}

	vfprintf(_debugfd, fmt, valist);

	if (strrchr(fmt, '\n') == NULL)
		fprintf(_debugfd, "\n");
	fflush(_debugfd);

    va_end(valist);
}

// Returns zero on success.
int aguiLoadScreen(char *screenName) {
	if (access(screenName, F_OK) != 0) {
		return 1;
	}

	WidthHeight wh;

	aguiScreenSize(&wh);
	if (wh.height > 0)
		wh.height--;

	// aguiMvText(10, 10, "width %d, height %d\n", wh.width, wh.height);
	// getchar();

	FILE *f = fopen(screenName, "rb");
	if (f != NULL) {
		char line[256];

		listDestroy(&list);
		listInit(&list);

		int count = 0;
		while (fgets(line, sizeof(line), f) != NULL) {
			char *p = strchr(line, '\n');
			if (p != NULL)
				*p = '\0';
			if (strncmp(line, "#@", 2) == 0) {
				char *args[128];
				char *s = strdup(line);
				qparse(s, " ", args, 128);
				char type = args[0][2];
				// aguiDebug("line '%s'", line);
				if (type == 'B') {
					NodeData *nd = calloc(1, sizeof(NodeData));
					nd->type = args[0][2];
					nd->r = atoi(args[1]);
					nd->c = atoi(args[2]);
					nd->w = atoi(args[3]);
					nd->h = atoi(args[4]);
					nd->color = atoi(args[5]);
					nd->line_dir = args[6][0];		// Horizontal or vertical
					nd->style = args[7][0];			// single or double line graphics
					listPushBack(&list, *nd);
				} else if (type == 'T') {
					NodeData *nd = calloc(1, sizeof(NodeData));
					nd->type = args[0][2];
					nd->r = atoi(args[1]);
					nd->c = atoi(args[2]);
					nd->w = atoi(args[3]);
					nd->h = atoi(args[4]);
					nd->color = atoi(args[5]);
					nd->line_dir = args[6][0];
					nd->style = args[7][0];
					strcpy(nd->text, args[8]);
					// aguiDebug("nd->text: '%s'", nd->text);
					listPushBack(&list, *nd);
				} else if (type == 'L') {
					NodeData *nd = calloc(1, sizeof(NodeData));
					nd->type = args[0][2];
					nd->r = atoi(args[1]);
					nd->c = atoi(args[2]);
					nd->w = atoi(args[3]);
					nd->h = atoi(args[4]);
					nd->color = atoi(args[5]);
					nd->line_dir = args[6][0];
					nd->style = args[7][0];
					listPushBack(&list, *nd);
				} else if (type == 'G') {
					NodeData *nd = calloc(1, sizeof(NodeData));
					nd->type = args[0][2];
					nd->r = atoi(args[1]);
					nd->c = atoi(args[2]);
					nd->w = atoi(args[3]);
					nd->h = atoi(args[4]);
					nd->color = atoi(args[5]);
					nd->line_dir = args[6][0];
					nd->style = args[7][0];
					strcpy(nd->text, args[8]);
					listPushBack(&list, *nd);
				}
				free(s);
				continue;
			}
			// printf("%s", line);

			if (++count >= wh.height)
				break;
		}

		fclose(f);

		// apply color and style

		// NOTE: Ncurses is zero relative and ascci escape codes are 1 relative

		ListNode *cur = list.head;
		while(cur) {
			if (cur->data.type == 'B') {
				aguiSetColor(cur->data.color, COLOR_DEFAULT);
				aguiBox(cur->data.r+1, cur->data.c+1, cur->data.w, cur->data.h, cur->data.style);
				aguiSetColor(COLOR_DEFAULT, COLOR_DEFAULT);
			} else if (cur->data.type == 'T') {
				aguiSetColor(cur->data.color, COLOR_DEFAULT);
				aguiMvText(cur->data.r+1, cur->data.c+1, cur->data.text);
				aguiSetColor(COLOR_DEFAULT, COLOR_DEFAULT);
			} else if (cur->data.type == 'L') {
			// aguiDebug("type=%c, r=%d, c=%d, w=%d, h=%d, color=%d, line_dir=%c, style=%c, test=%s",
					// cur->data.type, cur->data.r+1, cur->data.c+1, cur->data.w, cur->data.h,
					// cur->data.color, cur->data.line_dir, cur->data.style, cur->data.text);
				aguiSetColor(COLOR_WHITE, COLOR_DEFAULT);
				if (cur->data.line_dir == H_DIR) {
					aguiHorizLine(cur->data.r+1, cur->data.c+1, cur->data.w, cur->data.style);
				} else {
					aguiVertLine(cur->data.r+1, cur->data.c+1, cur->data.h, cur->data.style);
				}
				aguiSetColor(COLOR_DEFAULT, COLOR_DEFAULT);
			} else if (cur->data.type == 'G') {
				aguiSetColor(cur->data.color, COLOR_DEFAULT);
				aguiMvText(cur->data.r+1, cur->data.c+1, cur->data.text);
				aguiSetColor(COLOR_DEFAULT, COLOR_DEFAULT);
			}
			cur = cur->next;
		}
	} else {
		return 2;
	}

	return 0;
}
