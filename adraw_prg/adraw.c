/*
 * draw.c - ncurses drawing program
 * Features: draw single/double-line boxes, colored text,
 *           select/move/recolor objects, save/load UTF-8 file,
 *           side panel showing commands and status
 *
 * Build: gcc -o adraw adraw.c -lncursesw
 * Usage: draw [file]   (file is optional; loads screen on startup)
 *
 * Controls:
 *   Arrow keys  - move cursor
 *   b           - start/finish single-line box
 *   B           - start/finish double-line box
 *   t           - enter text at cursor
 *   v           - Single vertical line
 *   V           - Double vertical line
 *   h           - Single horizontal line
 *   H           - Double horizontal line
 *   c           - color picker popup
 *   s           - select/move object  (s or Enter to place, ESC cancel)
 *   d           - delete object under cursor
 *   w           - save to UTF-8 file
 *   l           - load from UTF-8 file
 *   q           - quit
 *   ESC         - cancel current action
 */

#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

#define MAX_ENTRIES 1024

#define MAX_OBJECTS  256
#define MAX_TEXT     256
#define MAX_FILENAME 256
#define PANEL_W      32      /* side panel width including border */

#define H_DIR       'H'
#define V_DIR       'V'
#define U_DIR       'U'		// unused in the object type.

#define D_GRAPH     'D'		// double line graphics
#define S_GRAPH     'S'		// single line graphics
#define U_GRAPH     'U'		// unsed in the object type.

/* ── UTF-8 box-drawing sequences ───────────────────────────────── */
/* single line */
static const char U_UL[] = "\xe2\x94\x8c";  /* ┌ */
static const char U_UR[] = "\xe2\x94\x90";  /* ┐ */
static const char U_LL[] = "\xe2\x94\x94";  /* └ */
static const char U_LR[] = "\xe2\x94\x98";  /* ┘ */
static const char U_HZ[] = "\xe2\x94\x80";  /* ─ */
static const char U_VT[] = "\xe2\x94\x82";  /* │ */
/* double line */
static const char D_UL[] = "\xe2\x95\x94";  /* ╔ */
static const char D_UR[] = "\xe2\x95\x97";  /* ╗ */
static const char D_LL[] = "\xe2\x95\x9a";  /* ╚ */
static const char D_LR[] = "\xe2\x95\x9d";  /* ╝ */
static const char D_HZ[] = "\xe2\x95\x90";  /* ═ */
static const char D_VT[] = "\xe2\x95\x91";  /* ║ */

#define CSIZ 5   /* max UTF-8 bytes per cell + NUL */
#define GCELL(r,c)  (grid + ((size_t)((r)*cols+(c))*CSIZ))
#define GSET(r,c,s) do { if((r)>=0&&(r)<gr&&(c)>=0&&(c)<cols) \
                         strncpy(GCELL(r,c),(s),CSIZ-1); } while(0)

char *types[] = { "Box", "Text", "Glyph", "Line" };

typedef enum { OBJ_BOX, OBJ_TEXT, OBJ_GLYPH, OBJ_LINE } ObjType;

typedef struct {
    ObjType type;
    int x, y, w, h;
    char text[MAX_TEXT];
    int  color;
	int  line_dir;   /* 0=horizontal 1=vertical */
    int  box_style;
    int  active;
} Object;

static Object objects[MAX_OBJECTS];
static int    num_objects = 0;
static int    cur_x = 0, cur_y = 0;
static int    cur_color  = 7;
static int    cur_bstyle = S_GRAPH;

typedef enum {
    STATE_NORMAL,
    STATE_DRAWING_BOX,
	STATE_DRAWING_LINE,
    STATE_MOVING,
    STATE_COLOR_PICK,
    STATE_GLYPH_PICK
} AppState;

FILE *logfd = NULL;

static AppState state    = STATE_NORMAL;
static int box_sx, box_sy;
static int line_sx, line_sy;		/* line start anchor */
static int line_dir_cur = H_DIR;	/* 0=horiz 1=vert */
static int line_dbl_cur = S_GRAPH;  /* 0=single 1=double */
static int selected      = -1;
static int move_off_x, move_off_y;
static int cpick_idx     = 0;
static int gpick_row     = 0;  /* glyph picker cursor row */
static int gpick_col     = 0;  /* glyph picker cursor col */
static int gpick_page    = 0;  /* glyph picker page      */
static int dirty         = 0;  /* 1 = unsaved changes exist */

/* Short feedback message shown in the panel */
static char status_msg[128] = "Ready";

char *strqtok (char *s1, const char *s2);
int qparse(char *str, const char *chrs, char **argz, int max_argz);
char *trim_tail(char *str);
char *trim_head(char *str);
char *trim_chars(char *str, char *chrs);
char *trim_head_chars(char *str, char *chrs);
char *trim_tail_chars(char *str, char *chrs);
char *trim(char *str);
int parse(char *str, const char *chrs, char **argz, int max_argz);

/* ── color tables ──────────────────────────────────────────────── */
static const int         color_ids[7] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};
static const char *const color_names[7] = {
    "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"
};

int directory_exists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        // stat() returned an error
        if (errno == ENOENT || errno == ENOTDIR) {
            return 0; // The directory does not exist or a path component is not a directory
        }
        perror("stat error"); // Handle other potential errors
        return -1; // An error occurred
    } else if (S_ISDIR(info.st_mode)) {
        return 1; // It is a directory
    } else {
        return 0; // It exists, but it's not a directory (e.g., it's a file)
    }
}

void createLogFile() {
	char *home = getenv("HOME");

	char dir[256];
	sprintf(dir, "%s/.agui", home);

	if (directory_exists(dir) != 1) {
		mkdir(dir, 0755);
	}

	char logFile[256];
	sprintf(logFile, "%s/agui.log", dir);

	logfd = fopen(logFile, "a");
}

void logIt(char *msg, ...) {
	va_list valist;

    va_start(valist, msg);

    if (logfd != NULL) {
        vfprintf(logfd, msg, valist);

        if (strrchr(msg, '\n') == NULL)
            fprintf(logfd, "\n");
        fflush(logfd);
    }
    va_end(valist);
}

typedef struct {
    char name[NAME_MAX + 1];
    int is_dir;
} Entry;

static int entry_cmp(const void *a, const void *b) {
    const Entry *ea = a;
    const Entry *eb = b;

    /* Directories first */
    if (ea->is_dir != eb->is_dir)
        return eb->is_dir - ea->is_dir;

    /* Alphabetical order */
    return strcasecmp(ea->name, eb->name);
}

static int load_dir(const char *path, Entry *entries) {
    DIR *dir = opendir(path);
    struct dirent *de;
    struct stat st;
    char full[PATH_MAX];
    int count = 0;

    if (!dir) return -1;

    while ((de = readdir(dir)) && count < MAX_ENTRIES) {
        if (!strcmp(de->d_name, ".")) continue;

        snprintf(full, sizeof(full), "%s/%s", path, de->d_name);
        if (stat(full, &st) == 0) {
            strncpy(entries[count].name, de->d_name, NAME_MAX);
            entries[count].is_dir = S_ISDIR(st.st_mode);
            count++;
        }
    }

    closedir(dir);
    return count;
}

/*
 * Color pairs:
 *   1-7   : colored fg, default bg
 *   11-17 : default fg, colored bg  (highlight)
 *   20    : black on white  (panel header / divider)
 *   21    : white on black  (panel body / picker)
 *   22    : bold white on blue (panel title)
 */
static void init_colors(void) {
    start_color();
    use_default_colors();
    for (int i = 0; i < 7; i++) {
        init_pair(i + 1,  color_ids[i], -1);
        init_pair(i + 11, -1, color_ids[i]);
    }
    init_pair(20, COLOR_BLACK, COLOR_WHITE);
    init_pair(21, COLOR_WHITE, COLOR_BLACK);
    init_pair(22, COLOR_WHITE, COLOR_BLUE);
}

/* canvas width = terminal width minus panel */
static int canvas_cols(void) {
    int rows, cols; (void)rows;
    getmaxyx(stdscr, rows, cols);
    return cols - PANEL_W;
}

/* ── draw a line object ────────────────────────────────────────── */
static void draw_line_obj(int x, int y, int len, int dir, int dbl, int color) {
    if (len < 1)
		return;
    // int pair = hi ? (color + 10) : color;
    // attron(COLOR_PAIR(pair));

    if (dir == H_DIR) {
        /* horizontal — build the whole line as one string, single mvaddstr */
        const char *ch = dbl==D_GRAPH ? D_HZ : U_HZ;
        char *buf = malloc((size_t)len * 3 + 1);
        if (!buf) {
			// attroff(COLOR_PAIR(pair));
			return;
		}
        for (int i = 0; i < len; i++) {
			memcpy(buf + i*3, ch, 3);
		}
        buf[len*3] = '\0';
        mvaddstr(y, x, buf);
        free(buf);
    } else {
        /* vertical — one mvaddstr per cell (each already at a fresh position) */
        const char *ch = dbl==D_GRAPH ? D_VT : U_VT;
        for (int i = 0; i < len; i++) {
            mvaddstr(y + i, x, ch);
		}
    }
    // attroff(COLOR_PAIR(pair));
}

/* ── ncurses box drawing ───────────────────────────────────────── */
static void draw_ncurses_box(int x, int y, int w, int h, int color, int hi, int dbl)
{
    int pair = hi ? (color + 10) : color;
    attron(COLOR_PAIR(pair));

    if (dbl) {
        int inner = w - 2;
        char *hrow = malloc(3 + inner * 3 + 3 + 1);
        /* top: ╔═...═╗ */
        memcpy(hrow, D_UL, 3);
        for (int i = 0; i < inner; i++) memcpy(hrow + 3 + i*3, D_HZ, 3);
        memcpy(hrow + 3 + inner*3, D_UR, 3);
        hrow[3 + inner*3 + 3] = '\0';
        mvaddstr(y, x, hrow);
        /* bottom: ╚═...═╝ */
        memcpy(hrow, D_LL, 3);
        for (int i = 0; i < inner; i++) memcpy(hrow + 3 + i*3, D_HZ, 3);
        memcpy(hrow + 3 + inner*3, D_LR, 3);
        hrow[3 + inner*3 + 3] = '\0';
        mvaddstr(y+h-1, x, hrow);
        free(hrow);
        for (int i = 1; i < h-1; i++) {
            mvaddstr(y+i, x,     D_VT);
            mvaddstr(y+i, x+w-1, D_VT);
        }
    } else {
        mvaddch(y,     x,     ACS_ULCORNER);
        mvaddch(y,     x+w-1, ACS_URCORNER);
        mvaddch(y+h-1, x,     ACS_LLCORNER);
        mvaddch(y+h-1, x+w-1, ACS_LRCORNER);
        for (int i = 1; i < w-1; i++) {
            mvaddch(y,     x+i, ACS_HLINE);
            mvaddch(y+h-1, x+i, ACS_HLINE);
        }
        for (int i = 1; i < h-1; i++) {
            mvaddch(y+i, x,     ACS_VLINE);
            mvaddch(y+i, x+w-1, ACS_VLINE);
        }
    }
    attroff(COLOR_PAIR(pair));
}

static void draw_object(int idx) {
    Object *o = &objects[idx];
    if (!o->active) return;
    int hi = (idx == selected);
    if (o->type == OBJ_BOX) {
        draw_ncurses_box(o->x, o->y, o->w, o->h, o->color, hi, o->box_style == D_GRAPH);
	} else if (o->type == OBJ_GLYPH) {
        int pair = hi ? (o->color + 10) : o->color;
        attron(COLOR_PAIR(pair));
        mvaddstr(o->y, o->x, o->text);
        attroff(COLOR_PAIR(pair));
	} else if (o->type == OBJ_LINE) {
        int pair = hi ? (o->color + 10) : o->color;
        attron(COLOR_PAIR(pair));
		if (o->line_dir == H_DIR)
			draw_line_obj(o->x, o->y, o->w, o->line_dir, o->box_style, o->color);
		else
			draw_line_obj(o->x, o->y, o->h, o->line_dir, o->box_style, o->color);
        attroff(COLOR_PAIR(pair));
    } else {
        int pair = hi ? (o->color + 10) : o->color;
        attron(COLOR_PAIR(pair));
        mvaddstr(o->y, o->x, o->text);
        attroff(COLOR_PAIR(pair));
    }
}

/* ── side panel ────────────────────────────────────────────────── */
static void draw_panel(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int px = cols - PANEL_W;   /* panel starts at this column */
    int pw = PANEL_W;          /* total panel width           */
    int iw = pw - 2;           /* inner width (inside border) */

    /* vertical divider line on the left edge of the panel */
    attron(COLOR_PAIR(21));
    for (int r = 0; r < rows; r++)
        mvaddch(r, px, ACS_VLINE);

    /* fill panel background */
    for (int r = 0; r < rows; r++)
        mvhline(r, px+1, ' ', pw-1);

    /* ── Title ── */
    attron(COLOR_PAIR(22) | A_BOLD);
    mvprintw(0, px+1, " %-*s", iw-1, "  DRAW");
    attroff(COLOR_PAIR(22) | A_BOLD);

    /* ── Commands section ── */
    int row = 2;
    attron(COLOR_PAIR(21) | A_BOLD | A_UNDERLINE);
    mvprintw(row++, px+2, "%-*s", iw-1, "Commands");
    attroff(A_UNDERLINE);

    /* command entries: key | description */
    struct { const char *key; const char *desc; } cmds[] = {
        { "b",       "Single box"    },
        { "B",       "Double box"    },
        { "t",       "Place text"    },
        { "s",       "Select/move"   },
        { "d",       "Delete"        },
        { "c",       "Color picker"  },
        { "w",       "Save file"     },
        { "v",       "Single vertical line"     },
        { "V",       "Double vertical line"     },
        { "h",       "Single horizontal line"     },
        { "H",       "Double horizontal line"     },
        { "g",       "Glyph picker"  },
        { "l",       "Load file"     },
        { "q",       "Quit"          },
        { "Arrows",  "Move cursor"   },
        { "Enter",   "Confirm"       },
        { "ESC",     "Cancel"        },
    };
    int ncmds = (int)(sizeof cmds / sizeof cmds[0]);
    attroff(A_BOLD);

    for (int i = 0; i < ncmds && row < rows - 6; i++, row++) {
        attron(COLOR_PAIR(22));
        mvprintw(row, px+2, "%-7s", cmds[i].key);
        attroff(COLOR_PAIR(22));
        attron(COLOR_PAIR(21));
        mvprintw(row, px+9, "%-*s", iw-8, cmds[i].desc);
        attroff(COLOR_PAIR(21));
    }

    /* ── Status section ── */
    row++;
    attron(COLOR_PAIR(21) | A_BOLD | A_UNDERLINE);
    mvprintw(row++, px+2, "%-*s", iw-1, "Status");
    attroff(A_UNDERLINE | A_BOLD);

    /* State */
    const char *state_str = "Normal";
    if      (state == STATE_DRAWING_BOX) state_str = "Drawing box";
    else if (state == STATE_MOVING)      state_str = "Moving";
    else if (state == STATE_COLOR_PICK)  state_str = "Color pick";
    else if (state == STATE_GLYPH_PICK)  state_str = "Glyph pick";

    attron(COLOR_PAIR(21));
    mvprintw(row++, px+2, "%-*s", iw-1, state_str);

    /* Color swatch + name */
    mvprintw(row, px+2, "Color: ");
    attroff(COLOR_PAIR(21));
    attron(COLOR_PAIR(cur_color + 10));
    mvprintw(row, px+9, "   ");
    attroff(COLOR_PAIR(cur_color + 10));
    attron(COLOR_PAIR(21));
    mvprintw(row, px+13, " %-*s", iw-12, color_names[cur_color-1]);
    attroff(COLOR_PAIR(21));
    row++;

    /* Style */
    attron(COLOR_PAIR(21));
    mvprintw(row++, px+2, "Style: %-*s", iw-7,
             cur_bstyle == D_GRAPH ? "Double" : "Single");

    /* Cursor position */
    mvprintw(row++, px+2, "Pos:   %d, %d", cur_x, cur_y);

    /* Objects count + unsaved indicator */
    if (dirty)
        mvprintw(row++, px+2, "Objects: %d *unsaved*", num_objects);
    else
        mvprintw(row++, px+2, "Objects: %d", num_objects);
    attroff(COLOR_PAIR(21));

    /* ── Message (feedback) at bottom of panel ── */
    attron(COLOR_PAIR(20));
    mvhline(rows-1, px+1, ' ', pw-1);
    /* truncate message to fit */
    char msg[64];
    snprintf(msg, sizeof msg, " %-*.*s", iw-1, iw-1, status_msg);
    mvaddstr(rows-1, px+1, msg);
    attroff(COLOR_PAIR(20));
}

/* ── glyph picker popup ────────────────────────────────────────── */
/*
 * A paged grid of Unicode characters grouped into named categories.
 * Arrow keys navigate, PgUp/PgDn switch pages, Enter places, ESC cancels.
 */

typedef struct { const char *name; const char *chars; } GlyphCat;

static const GlyphCat glyph_cats[] = {
    { "Box Single",
      "\xe2\x94\x80\xe2\x94\x82\xe2\x94\x8c\xe2\x94\x90\xe2\x94\x94\xe2\x94\x98\xe2\x94\x9c\xe2\x94\xa4"
      "\xe2\x94\xac\xe2\x94\xb4\xe2\x94\xbc\xe2\x95\xa4\xe2\x95\xa7\xe2\x95\xaa\xe2\x95\x9e\xe2\x95\xa1"
	  "\xe2\x95\xab\xe2\x95\xa8\xe2\x95\x9e\xe2\x95\x95\xe2\x95\x92"
	  "\xe2\x95\xb4\xe2\x95\xb5\xe2\x95\xb6\xe2\x95\xb7"
    },
    { "Box Double",
      "\xe2\x95\x90\xe2\x95\x91\xe2\x95\x94\xe2\x95\x97\xe2\x95\x9a\xe2\x95\x9d\xe2\x95\xa0\xe2\x95\xa3"
      "\xe2\x95\xa6\xe2\x95\xa9\xe2\x95\xac\xe2\x95\x92\xe2\x95\x93\xe2\x95\x95\xe2\x95\x96\xe2\x95\x98"
      "\xe2\x95\x99\xe2\x95\x9b\xe2\x95\x9c"
    },
    { "Block",
      "\xe2\x96\x88\xe2\x96\x89\xe2\x96\x8a\xe2\x96\x8b\xe2\x96\x8c\xe2\x96\x8d\xe2\x96\x8e\xe2\x96\x8f"
      "\xe2\x96\x90\xe2\x96\x91\xe2\x96\x92\xe2\x96\x93\xe2\x96\x84\xe2\x96\x80"
    },
    { "Geometric",
      "\xe2\x96\xb2\xe2\x96\xb3\xe2\x96\xb4\xe2\x96\xb5\xe2\x96\xb6\xe2\x96\xb7\xe2\x96\xb8\xe2\x96\xb9"
      "\xe2\x96\xbc\xe2\x96\xbd\xe2\x96\xbe\xe2\x96\xbf\xe2\x97\x80\xe2\x97\x81\xe2\x97\x82\xe2\x97\x83"
      "\xe2\x97\x86\xe2\x97\x87\xe2\x97\x8b\xe2\x97\x8f\xe2\x97\x8e\xe2\x97\x89"
    },
    { "Arrows",
      "\xe2\x86\x90\xe2\x86\x91\xe2\x86\x92\xe2\x86\x93\xe2\x86\x94\xe2\x86\x95\xe2\x87\x90\xe2\x87\x92"
      "\xe2\x87\x91\xe2\x87\x93\xe2\x87\x94\xe2\x87\x95\xe2\x86\x96\xe2\x86\x97\xe2\x86\x98\xe2\x86\x99"
      "\xe2\x87\xa6\xe2\x87\xa7\xe2\x87\xa8\xe2\x87\xa9"
    },
    { "Math",
      "\xc2\xb1\xc3\xb7\xc3\x97\xe2\x89\xa0\xe2\x89\xa4\xe2\x89\xa5\xe2\x88\x9e\xe2\x88\x91"
      "\xe2\x88\x9a\xe2\x88\xab\xe2\x89\x88\xc2\xb0\xc2\xb5\xe2\x80\xb0\xc2\xb2\xc2\xb3"
      "\xc2\xbc\xc2\xbd\xc2\xbe"
    },
    { "Misc Symbols",
      "\xe2\x98\x80\xe2\x98\x81\xe2\x98\x82\xe2\x98\x83\xe2\x98\x85\xe2\x98\x86\xe2\x99\xa0\xe2\x99\xa1"
      "\xe2\x99\xa2\xe2\x99\xa3\xe2\x99\xa4\xe2\x99\xa5\xe2\x99\xa6\xe2\x99\xa7\xe2\x98\x8e\xe2\x98\x8f"
      "\xe2\x9c\x93\xe2\x9c\x97"
    },
    { "Punctuation",
      "\xe2\x80\xa2\xc2\xb7\xe2\x80\xa6\xe2\x80\x94\xe2\x80\x93\xc2\xab\xc2\xbb\xe2\x80\xb9"
      "\xe2\x80\xba\xe2\x80\xa0\xe2\x80\xa1\xc2\xa7\xc2\xb6\xc2\xa9\xc2\xae\xe2\x84\xa2"
    },
};
#define NUM_GLYPH_CATS ((int)(sizeof glyph_cats / sizeof glyph_cats[0]))

/* Count UTF-8 characters (not bytes) in a string */
static int utf8_count(const char *s) {
    int n = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if      (c < 0x80) s += 1;
        else if (c < 0xE0) s += 2;
        else if (c < 0xF0) s += 3;
        else               s += 4;
        n++;
    }
    return n;
}

/* Get pointer to the nth UTF-8 character in s */
static const char *utf8_nth(const char *s, int n) {
    while (n-- > 0 && *s) {
        unsigned char c = (unsigned char)*s;
        if      (c < 0x80) s += 1;
        else if (c < 0xE0) s += 2;
        else if (c < 0xF0) s += 3;
        else               s += 4;
    }
    return s;
}

/* Copy the nth UTF-8 character from s into dst (NUL-terminated) */
static void utf8_nth_copy(const char *s, int n, char *dst) {
    const char *p = utf8_nth(s, n);
    unsigned char c = (unsigned char)*p;
    int len = (c < 0x80)?1:(c < 0xE0)?2:(c < 0xF0)?3:4;
    memcpy(dst, p, len);
    dst[len] = '\0';
}

#define GPICK_COLS 25   /* characters per row in the picker grid */

static void draw_glyph_picker(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int cat = gpick_page;
    if (cat < 0) {
		cat = 0;
	}
    if (cat >= NUM_GLYPH_CATS) {
		cat = NUM_GLYPH_CATS - 1;
	}

    const char *chars = glyph_cats[cat].chars;
    int total  = utf8_count(chars);
    int gcols  = GPICK_COLS;
    int grows  = (total + gcols - 1) / gcols;

    /* popup dimensions: each glyph gets 2 screen columns (glyph + space) */
    int pw = gcols * 2 + 4;
    int ph = grows + 6;
    if (pw > cw - 2) pw = cw - 2;
    int px = (cw - pw) / 2;
    int py = (rows - ph) / 2;
    if (py < 0) py = 0;

    /* shadow */
    attron(A_DIM | COLOR_PAIR(21));
    for (int r = 1; r <= ph; r++) {
		mvhline(py+r, px+2, ' ', pw);
	}
    attroff(A_DIM | COLOR_PAIR(21));

    /* border + background */
    attron(COLOR_PAIR(21) | A_BOLD);
    for (int r = 0; r < ph; r++) {
		mvhline(py+r, px, ' ', pw);
	}
    mvaddch(py,      px,      ACS_ULCORNER);
    mvaddch(py,      px+pw-1, ACS_URCORNER);
    mvaddch(py+ph-1, px,      ACS_LLCORNER);
    mvaddch(py+ph-1, px+pw-1, ACS_LRCORNER);
    for (int i = 1; i < pw-1; i++){
		mvaddch(py,px+i,ACS_HLINE);
		mvaddch(py+ph-1,px+i,ACS_HLINE);
	}
    for (int i = 1; i < ph-1; i++){
		mvaddch(py+i,px,ACS_VLINE);
		mvaddch(py+i,px+pw-1,ACS_VLINE);
	}

    /* title */
    char title[48];
    snprintf(title, sizeof title, " %s ", glyph_cats[cat].name);
    mvprintw(py, px + (pw - (int)strlen(title)) / 2, "%s", title);
    attroff(COLOR_PAIR(21) | A_BOLD);

    /* page indicator */
    attron(COLOR_PAIR(21));
    mvprintw(py+1, px+2, "Page %d/%d  (PgUp/PgDn)", cat+1, NUM_GLYPH_CATS);
    attroff(COLOR_PAIR(21));

    /*
     * Glyph grid — draw one complete row at a time using a single mvaddstr
     * call per row.  This is the same technique used for double-line boxes:
     * a single mvaddstr passes the bytes straight through to the terminal
     * without ncurses doing per-character width analysis (which inserts
     * unwanted spaces after each multi-byte character).
     *
     * The selected cell is drawn separately with highlight colour after the
     * normal row so the highlight attr doesn't bleed into adjacent chars.
     */
    /* max row buf: gcols * (4 bytes glyph + 1 space) + NUL */
    char rowbuf[GPICK_COLS * 5 + 2];

    for (int gr = 0; gr < grows; gr++) {
        int ry   = py + 3 + gr;
        if (ry >= py + ph - 2) break;
        int rx   = px + 2;
        int col0 = gr * gcols;
        int col1 = col0 + gcols - 1;
        if (col1 >= total) col1 = total - 1;

        /* build the row string, leaving a blank at the selected column */
        int pos = 0;
        for (int gc = 0; gc <= col1 - col0; gc++) {
            int idx = col0 + gc;
            int sel = (gr == gpick_row && gc == gpick_col);
            if (sel) {
                /* placeholder: two spaces — we'll overdraw with highlight */
                rowbuf[pos++] = ' ';
                rowbuf[pos++] = ' ';
            } else {
                char glyph[5] = {0};
                utf8_nth_copy(chars, idx, glyph);
                int glen = (int)strlen(glyph);
                memcpy(rowbuf + pos, glyph, glen);
                pos += glen;
                rowbuf[pos++] = ' ';   /* separator space */
            }
        }
        rowbuf[pos] = '\0';

        attron(COLOR_PAIR(cur_color));
        mvaddstr(ry, rx, rowbuf);
        attroff(COLOR_PAIR(cur_color));

        /* overdraw selected cell with highlight using its own mvaddstr */
        if (gr == gpick_row && gpick_col <= col1 - col0) {
            char sel_glyph[6] = {0};
            utf8_nth_copy(chars, col0 + gpick_col, sel_glyph);
            int sel_rx = rx + gpick_col * 2;
            attron(COLOR_PAIR(cur_color + 10) | A_BOLD);
            mvaddstr(ry, sel_rx, sel_glyph);
            attroff(COLOR_PAIR(cur_color + 10) | A_BOLD);
        }
    }

    /* footer */
    attron(COLOR_PAIR(21));
    mvprintw(py+ph-2, px+2, "Arrows=nav  PgUp/Dn=page  Enter=place  Esc=cancel");
    attroff(COLOR_PAIR(21));
}

/* Handle a keypress while in glyph-picker mode.
   Returns 1 if a glyph was placed (caller should mark dirty). */
static int handle_glyph_pick(int ch, int place_x, int place_y) {
    int cat   = gpick_page;
    if (cat < 0) {
		cat = 0;
	}
    if (cat >= NUM_GLYPH_CATS) {
		cat = NUM_GLYPH_CATS - 1;
	}
    const char *chars = glyph_cats[cat].chars;
    int total  = utf8_count(chars);
    int gcols  = GPICK_COLS;
    int grows  = (total + gcols - 1) / gcols;

    /* clamp cursor */
    if (gpick_row >= grows) gpick_row = grows - 1;
    int max_col = (gpick_row == grows-1)
                  ? ((total-1) % gcols)
                  : gcols - 1;
    if (gpick_col > max_col) {
		gpick_col = max_col;
	}

    switch (ch) {
    case KEY_UP:
        if (gpick_row > 0)
			gpick_row--;
        break;
    case KEY_DOWN:
        if (gpick_row < grows-1)
			gpick_row++;
        break;
    case KEY_LEFT:
        if (gpick_col > 0)
			gpick_col--;
        else if (gpick_row > 0) {
			gpick_row--;
			gpick_col = gcols-1;
		}
        break;
    case KEY_RIGHT: {
        int mc = (gpick_row == grows-1) ? ((total-1)%gcols) : gcols-1;
        if (gpick_col < mc) {
			gpick_col++;
        } else if (gpick_row < grows-1) {
			gpick_row++;
			gpick_col = 0;
		}
        break;
    }
    case KEY_PPAGE:  /* PgUp */
        gpick_page = (gpick_page > 0) ? gpick_page-1 : NUM_GLYPH_CATS-1;
        gpick_row = 0; gpick_col = 0;
        break;
    case KEY_NPAGE:  /* PgDn */
        gpick_page = (gpick_page < NUM_GLYPH_CATS-1) ? gpick_page+1 : 0;
        gpick_row = 0; gpick_col = 0;
        break;
    case '\n': case KEY_ENTER: {
        int idx = gpick_row * gcols + gpick_col;
        if (idx < total && num_objects < MAX_OBJECTS) {
            char glyph[5] = {0};
            utf8_nth_copy(chars, idx, glyph);
            Object *o = &objects[num_objects++];
            memset(o, 0, sizeof *o);
            o->type   = OBJ_GLYPH;
            o->x      = place_x;
			o->y      = place_y;
            o->color  = cur_color;
            o->active = 1;
            strncpy(o->text, glyph, MAX_TEXT-1);
            state = STATE_NORMAL;
            snprintf(status_msg, sizeof status_msg, "Glyph placed");
            return 1;
        }
        break;
    }
    case 27:  /* ESC */
        state = STATE_NORMAL;
        snprintf(status_msg, sizeof status_msg, "Cancelled");
        break;
    }
    return 0;
}

/* ── color picker popup ────────────────────────────────────────── */
static void draw_color_picker(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int pw = 36, ph = 11;
    int px = (cw - pw) / 2;
    int py = (rows - ph) / 2;

    attron(A_DIM | COLOR_PAIR(21));
    for (int r = 1; r <= ph; r++)
        mvhline(py+r, px+2, ' ', pw);
    attroff(A_DIM | COLOR_PAIR(21));

    attron(COLOR_PAIR(21) | A_BOLD);
    for (int r = 0; r < ph; r++)
        mvhline(py+r, px, ' ', pw);
    mvaddch(py,     px,      ACS_ULCORNER);
    mvaddch(py,     px+pw-1, ACS_URCORNER);
    mvaddch(py+ph-1,px,      ACS_LLCORNER);
    mvaddch(py+ph-1,px+pw-1, ACS_LRCORNER);
    for (int i = 1; i < pw-1; i++) {
        mvaddch(py,      px+i, ACS_HLINE);
        mvaddch(py+ph-1, px+i, ACS_HLINE);
    }
    for (int i = 1; i < ph-1; i++) {
        mvaddch(py+i, px,      ACS_VLINE);
        mvaddch(py+i, px+pw-1, ACS_VLINE);
    }
    mvprintw(py, px + (pw-14)/2, " Choose Color ");
    attroff(COLOR_PAIR(21) | A_BOLD);

    for (int i = 0; i < 7; i++) {
        int ry  = py + 1 + i;
        int sel = (i == cpick_idx);
        attron(COLOR_PAIR(i+11));
        mvaddstr(ry, px+2, "   ");
        attroff(COLOR_PAIR(i+11));
        if (sel) {
            attron(COLOR_PAIR(i+11) | A_BOLD);
            mvprintw(ry, px+6, " %-13s", color_names[i]);
            mvaddch(ry, px+20, ACS_LTEE);
            mvaddch(ry, px+21, ACS_HLINE);
            attroff(COLOR_PAIR(i+11) | A_BOLD);
        } else {
            attron(COLOR_PAIR(21));
            mvprintw(ry, px+6, " %-13s  ", color_names[i]);
            attroff(COLOR_PAIR(21));
        }
    }
    attron(COLOR_PAIR(21));
    mvprintw(py+ph-2, px+2, "arrows=move  Enter=ok  Esc=cancel");
    attroff(COLOR_PAIR(21));
}

/* ── main render ───────────────────────────────────────────────── */
static void render(void) {
    clear();

    /* draw objects (clipped to canvas automatically by terminal) */
    for (int i = 0; i < num_objects; i++) {
        draw_object(i);
	}

	/* overdraw junction characters where lines/boxes meet */
    // draw_junctions();

    /* live box preview */
    if (state == STATE_DRAWING_BOX) {
        int x1 = box_sx < cur_x ? box_sx : cur_x;
        int y1 = box_sy < cur_y ? box_sy : cur_y;
        int x2 = box_sx > cur_x ? box_sx : cur_x;
        int y2 = box_sy > cur_y ? box_sy : cur_y;
        int w = x2-x1+1, h = y2-y1+1;
        if (w >= 2 && h >= 2)
            draw_ncurses_box(x1, y1, w, h, cur_color, 0, line_dbl_cur);
    }

	/* live line preview */
    if (state == STATE_DRAWING_LINE) {
        if (line_dir_cur == H_DIR) {		// Horizontal
            int x1 = line_sx < cur_x ? line_sx : cur_x;
            int len = abs(cur_x - line_sx) + 1;
            draw_line_obj(x1, line_sy, len, 0, line_dbl_cur, cur_color);
        } else {						// vertical
            int y1 = line_sy < cur_y ? line_sy : cur_y;
            int len = abs(cur_y - line_sy) + 1;
            draw_line_obj(line_sx, y1, len, 1, line_dbl_cur, cur_color);
        }
    }

    /* side panel (drawn last so it always overlays canvas edge) */
    draw_panel();

    if (state == STATE_COLOR_PICK)
        draw_color_picker();

    if (state == STATE_GLYPH_PICK)
        draw_glyph_picker();

    move(cur_y, cur_x);
    refresh();
}

/* ── find topmost object at canvas position ────────────────────── */
static int find_object_at(int x, int y) {
	int i = num_objects-1;
    for (; i >= 0; i--) {
        Object *o = &objects[i];
        if (o->active != 1)
			continue;
        if (o->type == OBJ_BOX) {
            if (x >= o->x && x < o->x+o->w && y >= o->y && y < o->y+o->h) {
                return i;
			}
		} else if (o->type == OBJ_GLYPH) {
            if (y == o->y && x == o->x) {
                return i;
			}
		} else if (o->type == OBJ_LINE) {
            if (o->line_dir == H_DIR) {  /* horizontal */
                if (y == o->y && x >= o->x && x < o->x + o->w) {
                    return i;
				}
            } else {                  /* vertical */
                if (x == o->x && y >= o->y && y < o->y + o->h) {
                    return i;
				}
            }
        } else if(o->type == OBJ_TEXT) {
            int len = (int)strlen(o->text);
            if (y == o->y && x >= o->x && x < o->x+len) {
                return i;
			}
        } else {
			logIt("Unknown object type %d", o->type);
		}
    }
    return -1;
}

/* ── confirm discard popup ─────────────────────────────────────── */
/*
 * Show a centred Y/N dialog when there are unsaved changes.
 * Returns 1 if the user confirms (discard), 0 if they cancel.
 */
static int confirm_discard(const char *action) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int pw = 38, ph = 5;
    int px = (cw - pw) / 2;
    int py = (rows - ph) / 2;

    /* shadow */
    attron(A_DIM | COLOR_PAIR(21));
    for (int r = 1; r <= ph; r++)
        mvhline(py+r, px+2, ' ', pw);
    attroff(A_DIM | COLOR_PAIR(21));

    /* box */
    attron(COLOR_PAIR(21) | A_BOLD);
    for (int r = 0; r < ph; r++) mvhline(py+r, px, ' ', pw);
    mvaddch(py,     px,      ACS_ULCORNER);
    mvaddch(py,     px+pw-1, ACS_URCORNER);
    mvaddch(py+ph-1,px,      ACS_LLCORNER);
    mvaddch(py+ph-1,px+pw-1, ACS_LRCORNER);
    for (int i=1;i<pw-1;i++){mvaddch(py,px+i,ACS_HLINE);mvaddch(py+ph-1,px+i,ACS_HLINE);}
    for (int i=1;i<ph-1;i++){mvaddch(py+i,px,ACS_VLINE);mvaddch(py+i,px+pw-1,ACS_VLINE);}
    mvprintw(py, px+(pw-18)/2, " Unsaved Changes ");
    attroff(COLOR_PAIR(21) | A_BOLD);

    attron(COLOR_PAIR(21));
    mvprintw(py+1, px+2, "%-*s", pw-3, action);
    mvprintw(py+2, px+2, "%-*s", pw-3, "Unsaved changes will be lost.");
    mvprintw(py+3, px+2, "%-*s", pw-3, "  Y = discard    N = cancel");
    attroff(COLOR_PAIR(21));
    refresh();

    while (1) {
        int ch = getch();
        if (ch == 'y' || ch == 'Y') return 1;
        if (ch == 'n' || ch == 'N' || ch == 27) return 0;
    }
}

/* ── save screen as UTF-8 text file ────────────────────────────── */
/*
 * File format:
 *   Lines starting with "#@" are metadata (object definitions).
 *   Format: #@<type> <x> <y> <w> <h> <color> <style> <text...>
 *     type : B=box  T=text  G=Glyph
 *     w,h  : used for boxes (0 for text)
 *     style: 0=single 1=double
 *     text : rest of line (text objects only)
 */
static void save_to_file(const char *filename) {
    int rows, cols_total;
    getmaxyx(stdscr, rows, cols_total);
    int cols = cols_total - PANEL_W;
    int gr   = rows;

    char *grid = calloc((size_t)(gr * cols * CSIZ), 1);
    if (!grid) {
		snprintf(status_msg,sizeof(status_msg),"Out of memory!");
		return;
	}

    for (int r = 0; r < gr; r++) {
        for (int c = 0; c < cols; c++) {
            GCELL(r,c)[0] = ' ', GCELL(r,c)[1] = '\0';
		}
	}

    for (int i = 0; i < num_objects; i++) {
        Object *o = &objects[i];
        if (!o->active) {
			continue;
		}
        if (o->type == OBJ_BOX) {
            int x1=o->x, y1=o->y, w=o->w, h=o->h;
            int dbl = (o->box_style == D_GRAPH);
            GSET(y1,    x1,     dbl ? D_UL : U_UL);
            GSET(y1,    x1+w-1, dbl ? D_UR : U_UR);
            GSET(y1+h-1,x1,     dbl ? D_LL : U_LL);
            GSET(y1+h-1,x1+w-1, dbl ? D_LR : U_LR);
            for (int c = 1; c < w-1; c++) {
                GSET(y1,    x1+c, dbl ? D_HZ : U_HZ);
                GSET(y1+h-1,x1+c, dbl ? D_HZ : U_HZ);
            }
            for (int r = 1; r < h-1; r++) {
                GSET(y1+r, x1,     dbl ? D_VT : U_VT);
                GSET(y1+r, x1+w-1, dbl ? D_VT : U_VT);
            }
		} else if (o->type == OBJ_GLYPH) {
            for (int c = 0; o->text[c] && o->x+c < cols; c++) {
                char tmp[2] = { o->text[c], '\0' };
                GSET(o->y, o->x+c, tmp);
            }
		} else if (o->type == OBJ_LINE) {
            int dbl = (o->box_style == D_GRAPH);
            if (o->line_dir == H_DIR) {  /* horizontal */
                for (int c = 0; c < o->w; c++)
                    GSET(o->y, o->x+c, dbl ? D_HZ : U_HZ);
            } else {                  /* vertical */
                for (int r = 0; r < o->w; r++)
                    GSET(o->y+r, o->x, dbl ? D_VT : U_VT);
            }
        } else {
            for (int c = 0; o->text[c] && o->x+c < cols; c++) {
                char tmp[2] = { o->text[c], '\0' };
                GSET(o->y, o->x+c, tmp);
            }
        }
    }

    FILE *f = fopen(filename, "w");
    if (!f) {
        snprintf(status_msg,sizeof(status_msg),"Can't open '%s'",filename);
        free(grid); return;
    }
	
	fprintf(f, "## ncurses is zero relative but the agui library is one relative.\n");
	fprintf(f, "## The agui library will adjust the row and col\n");
	fprintf(f, "## row col width height color dir style text\n");
	fprintf(f, "## color       - (0-6) RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE\n");
	fprintf(f, "## dir         - Direction Vertical=V, Horizontal=H or Unused=U\n");
	fprintf(f, "## style       - Double=D or single=S line graphics\n");
	fprintf(f, "## All blank lines need to start with ##\n");
	fprintf(f, "## The objects are listed in order of Boxes, Lines, Glyphs, Text.\n");
	fprintf(f, "## When list is searched, it is done in reverse order or bottom to top.\n");
	fprintf(f, "##\n");

	// Save all boxes first
    for (int i = 0; i < num_objects; i++) {
        Object *o = &objects[i];
        if (!o->active) {
			continue;
		}
        if (o->type == OBJ_BOX) {
            fprintf(f, "#@B %3d %3d %3d %3d %3d %c %c 'null'\n",
                    o->y, o->x, o->w, o->h, o->color, o->line_dir, o->box_style);
        }
    }
	// Save all lines next
    for (int i = 0; i < num_objects; i++) {
        Object *o = &objects[i];
        if (!o->active) {
			continue;
		}
		if (o->type == OBJ_LINE) {
			fprintf(f, "#@L %3d %3d %3d %3d %3d %c %c 'null'\n",
					o->y, o->x, o->w, o->h, o->color, o->line_dir, o->box_style);
        }
    }
	// Save all Text next
    for (int i = 0; i < num_objects; i++) {
        Object *o = &objects[i];
        if (!o->active) {
			continue;
		}
        if (o->type == OBJ_TEXT) {
            fprintf(f, "#@T %3d %3d   0   0 %3d U U '%s'\n",
                    o->y, o->x, o->color, o->text);
        }
    }
	// Save all glyph next
    for (int i = 0; i < num_objects; i++) {
        Object *o = &objects[i];
        if (!o->active) {
			continue;
		}
		if (o->type == OBJ_GLYPH) {
            fprintf(f, "#@G %3d %3d   0   0 %3d U U '%s'\n",
                    o->y, o->x, o->color, o->text);
        }
    }

    fclose(f);
    free(grid);
    dirty = 0;
    snprintf(status_msg, sizeof(status_msg), "Saved: %s", filename);
}

/* ── load screen from UTF-8 text file ──────────────────────────── */
static int cell_is(const char *cell, const char *seq) {
    return (unsigned char)cell[0] == (unsigned char)seq[0] &&
           (unsigned char)cell[1] == (unsigned char)seq[1] &&
           (unsigned char)cell[2] == (unsigned char)seq[2];
}

static int read_utf8(const char *src, char *dst) {
    unsigned char c = (unsigned char)src[0];
    if (c == 0) return 0;
    int len = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
    for (int i = 0; i < len && src[i]; i++) dst[i] = src[i];
    dst[len] = '\0';
    return len;
}

static void load_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        snprintf(status_msg, sizeof(status_msg), "Can't open '%s'", filename);
        return;
    }

    num_objects = 0;
    selected    = -1;
    state       = STATE_NORMAL;

    /* ── Pass 1: read metadata header lines (#@ prefix) ── */
    int has_meta = 0;
    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), f)) {

        if (linebuf[0] == '#' && linebuf[1] == '#') {
			continue;		// skip commets.
		}

        has_meta = 1;
        if (num_objects >= MAX_OBJECTS) {
			continue;
		}

		char *p = strchr(linebuf, '\n');
		if (p != NULL)
			*p = '\0';

		if (linebuf[0] == '\0')
			continue;

		char *args[32];
        char type = linebuf[2];
        int x=0,y=0,w=0,h=0,color=7,style=0,line_dir;
        char text[MAX_TEXT] = "";

        if (type == 'B') {
			char *s = strdup(linebuf);
			int nn = qparse(s, " ", args, 32);

			y = atoi(args[1]);
			x = atoi(args[2]);
			w = atoi(args[3]);
			h = atoi(args[4]);
			color = atoi(args[5]);
			line_dir = args[6][0];
			style = args[7][0];

			free(s);
		} else if (type == 'L') {
			char *s = strdup(linebuf);
			int n = qparse(s, " ", args, 32);

			y = atoi(args[1]);
			x = atoi(args[2]);
			w = atoi(args[3]);
			h = atoi(args[4]);
			color = atoi(args[5]);
			line_dir = args[6][0];
			style = args[7][0];

			free(s);
        } else if (type == 'G') {
			char *s = strdup(linebuf);
			int nn = qparse(s, " ", args, 32);

			y = atoi(args[1]);
			x = atoi(args[2]);
			w = atoi(args[3]);
			h = atoi(args[4]);
			color = atoi(args[5]);
			line_dir = args[6][0];
			style = args[7][0];
			strcpy(text, args[8]);

			free(s);
        } else if (type == 'T') {
			char *s = strdup(linebuf);
			int n = qparse(s, " ", args, 32);

			y = atoi(args[1]);
			x = atoi(args[2]);
			w = atoi(args[3]);
			h = atoi(args[4]);
			color = atoi(args[5]);
			line_dir = args[6][0];
			style = args[7][0];
			strcpy(text, args[8]);

			free(s);
		} else {
			continue;
		}

		/* clamp color to valid range */
        if (color < 1 || color > 7)
			color = 7;

        Object *o = &objects[num_objects++];
        memset(o, 0, sizeof *o);		// clear structure
        o->active    = 1;
        o->x         = x;
		o->y		 = y;
		o->w         = w;
		o->h         = h;
        o->color     = color;

		if (type == 'L') {
            o->type      = OBJ_LINE;
            o->line_dir  = line_dir;
            o->box_style = style;
		} else if (type == 'B') {
			o->type      = OBJ_BOX;
            o->line_dir  = U_DIR;
            o->box_style = style;
		} else if (type == 'G') {
            o->type = OBJ_GLYPH;
            o->line_dir  = U_DIR;
            strncpy(o->text, text, MAX_TEXT-1);
		} else if (type == 'T') {
            o->type = OBJ_TEXT;
            o->line_dir  = U_DIR;
            strncpy(o->text, text, MAX_TEXT-1);
        } else {
            o->type = OBJ_TEXT;
            o->line_dir  = U_DIR;
            strncpy(o->text, text, MAX_TEXT-1);
        }
    }

    fclose(f);

    dirty = 0;
    snprintf(status_msg, sizeof(status_msg), "Loaded: %s (%d obj)", filename, num_objects);
}

/* ── inline text prompt overlaid on the panel message row ──────── */
static void prompt_string(const char *prompt, char *out, int maxlen) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = cols - PANEL_W;   /* full canvas width available for input */

    /* Draw prompt bar across the entire bottom of the canvas */
    attron(COLOR_PAIR(20));
    mvhline(rows-1, 0, ' ', cw);
    int label_len = (int)strlen(prompt) + 2;  /* "Prompt: " */
    mvprintw(rows-1, 0, " %s: ", prompt);
    attroff(COLOR_PAIR(20));

    /* Position cursor right after the label; input can fill the rest */
    int input_col = 1 + label_len;
    int input_w   = cw - input_col - 1;
    if (input_w < 1) input_w = 1;

    move(rows-1, input_col);
    echo();
    curs_set(1);
    /* Limit what getnstr accepts to the smaller of maxlen and input_w,
       but since the terminal scrolls the field we just cap at maxlen. */
    getnstr(out, maxlen-1);
    noecho();
    curs_set(1);

    /* Restore the bottom canvas row by clearing it */
    mvhline(rows-1, 0, ' ', cw);
}


/* ── entry point ───────────────────────────────────────────────── */
int main(int argc, char **argv) {
    setlocale(LC_ALL, "");

	createLogFile();

	logIt("Starting AGUI Draw");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Terminal does not support colors.\n");
        return 1;
    }
    init_colors();

    /* load file passed on the command line, if any */
    if (argc > 1)
        load_from_file(argv[1]);

    int rows, cols;

    while (1) {
        getmaxyx(stdscr, rows, cols);
        int cw = cols - PANEL_W;   /* usable canvas width */

        render();
        int ch = getch();

        /* ══ glyph picker mode ══════════════════════════════════ */
        if (state == STATE_GLYPH_PICK) {
            if (handle_glyph_pick(ch, cur_x, cur_y))
                dirty = 1;
            continue;
        }

        /* ══ color picker mode ═══════════════════════════════════ */
        if (state == STATE_COLOR_PICK) {
            switch (ch) {
            case KEY_UP:   cpick_idx = (cpick_idx+6)%7; break;
            case KEY_DOWN: cpick_idx = (cpick_idx+1)%7; break;
            case '\n': case KEY_ENTER:
                cur_color = cpick_idx + 1;
                if (selected >= 0) {
					objects[selected].color = cur_color;
					dirty = 1;
				}
                state = STATE_NORMAL;
                snprintf(status_msg, sizeof(status_msg),
                         "Color: %s", color_names[cpick_idx]);
                break;
            case 27:
                state = STATE_NORMAL;
                snprintf(status_msg, sizeof(status_msg), "Ready");
                break;
            }
            continue;
        }

        /* ══ normal / drawing / moving modes ════════════════════ */
        switch (ch) {

        case 'q':
            if (dirty) {
                render();
                if (!confirm_discard("Quit without saving?")) break;
            }
            endwin();
            return 0;

        /* cursor movement — clamp to canvas, also drags selected object */
        case KEY_UP:
            if (cur_y > 0)
				cur_y--;
            if (state == STATE_MOVING && selected >= 0) {
                objects[selected].y = cur_y - move_off_y;
			}
            break;
        case KEY_DOWN:
            if (cur_y < rows-1)
				cur_y++;
            if (state == STATE_MOVING && selected >= 0) {
                objects[selected].y = cur_y - move_off_y;
			}
            break;
        case KEY_LEFT:
            if (cur_x > 0)
				cur_x--;
            if (state == STATE_MOVING && selected >= 0) {
                objects[selected].x = cur_x - move_off_x;
			}
            break;
        case KEY_RIGHT:
            if (cur_x < cw-1)
				cur_x++;
            if (state == STATE_MOVING && selected >= 0) {
                objects[selected].x = cur_x - move_off_x;
			}
            break;

        /* color picker */
        case 'c':
            cpick_idx = cur_color - 1;
            state = STATE_COLOR_PICK;
            snprintf(status_msg, sizeof(status_msg), "Pick a color");
            break;

        /* glyph picker */
        case 'g':
            gpick_row = 0; gpick_col = 0; gpick_page = 0;
            state = STATE_GLYPH_PICK;
            snprintf(status_msg, sizeof(status_msg), "Pick a glyph");
            break;

        /* draw box */
        case 'b':
        case 'B': {
            int dbl = S_GRAPH;
			if (ch == 'B')
				dbl = D_GRAPH;
            if (state == STATE_NORMAL) {
                box_sx = cur_x;
				box_sy = cur_y;
				line_dbl_cur = dbl;
                state  = STATE_DRAWING_BOX;
                snprintf(status_msg, sizeof(status_msg), "Corner set, press %c", (char)ch);
            } else if (state == STATE_DRAWING_BOX) {
                int x1 = box_sx < cur_x ? box_sx : cur_x;
                int y1 = box_sy < cur_y ? box_sy : cur_y;
                int w  = (box_sx > cur_x ? box_sx : cur_x) - x1 + 1;
                int h  = (box_sy > cur_y ? box_sy : cur_y) - y1 + 1;
                if (w >= 2 && h >= 2 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type = OBJ_BOX;
					o->x=x1;
					o->y=y1;
                    o->w=w;
					o->h=h;
					o->color=cur_color;
                    o->box_style=dbl;
					o->active=1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg), "%s box created",
							cur_bstyle==D_GRAPH ? "Double" : "Single");
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Box too small");
                }
                state = STATE_NORMAL;
            }
            break;
        }

		/* draw horizontal line (h=single, H=double) */
        case 'h':
        case 'H': {
            int dbl = S_GRAPH;
			if (ch == 'H')
				dbl = D_GRAPH;

            if (state == STATE_NORMAL) {
                line_sx = cur_x;
				line_sy = cur_y;
                line_dir_cur = H_DIR;
				line_dbl_cur = dbl;
                state = STATE_DRAWING_LINE;
                snprintf(status_msg, sizeof(status_msg),
                         "%s horiz line: move then press %c",
                         dbl ? "Double" : "Single", (char)ch);
            } else if (state == STATE_DRAWING_LINE && line_dir_cur == H_DIR) {
                int x1  = (line_sx < cur_x)? line_sx : cur_x;
                int len = abs(cur_x - line_sx) + 1;
                if (len >= 1 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type      = OBJ_LINE;
                    o->x         = x1;
					o->y         = line_sy;
                    o->w         = len;
                    o->h         = 0;
                    o->line_dir  = H_DIR;
                    o->box_style = dbl;
                    o->color     = cur_color; o->active = 1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg),
                             "%s horiz line (%d chars)",
                             dbl ? "Double" : "Single", len);
                }
                state = STATE_NORMAL;
            }
            break;
        }

        /* draw vertical line (v=single, V=double) */
        case 'v':
        case 'V': {
            int dbl = S_GRAPH;
			if(ch == 'V')
				dbl = D_GRAPH;
            if (state == STATE_NORMAL) {
                line_sx = cur_x;
				line_sy = cur_y;
                line_dir_cur = V_DIR;
				line_dbl_cur = dbl;
                state = STATE_DRAWING_LINE;
                snprintf(status_msg, sizeof(status_msg),
                         "%s vert line: move then press %c",
                         dbl ? "Double" : "Single", (char)ch);
            } else if (state == STATE_DRAWING_LINE && line_dir_cur == V_DIR) {
                int y1  = line_sy < cur_y ? line_sy : cur_y;
                int len = abs(cur_y - line_sy) + 1;
                if (len >= 1 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type      = OBJ_LINE;
                    o->x         = line_sx;
					o->y         = y1;
                    o->w         = 0;
                    o->h         = len;
                    o->line_dir  = V_DIR;
                    o->box_style = dbl;
					logIt("style %c", o->box_style);
                    o->color     = cur_color; o->active = 1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg),
                             "%s vert line (%d chars)",
                             dbl ? "Double" : "Single", len);
					logIt("Here 2, x=%d, y=%d, w=%d, h=%d", o->x, o->y, o->w, o->h);
                }
                state = STATE_NORMAL;
            }
            break;
        }

        /* place text */
        case 't':
            if (state == STATE_NORMAL && num_objects < MAX_OBJECTS) {
                char text[MAX_TEXT] = "";
                prompt_string("Text", text, MAX_TEXT);
                if (text[0]) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type=OBJ_TEXT;
					o->x=cur_x;
					o->y=cur_y;
                    o->color=cur_color;
					o->active=1;
                    strncpy(o->text, text, MAX_TEXT-1);
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg), "Text added");
                }
            }
            break;

        /* select / move */
        case 's':
            if (state == STATE_MOVING) {
                state = STATE_NORMAL;
				selected = -1;
                dirty = 1;
                snprintf(status_msg, sizeof(status_msg), "Placed");
            } else if (state == STATE_NORMAL) {
                int idx = find_object_at(cur_x, cur_y);
                if (idx >= 0) {
                    selected   = idx;
                    move_off_x = cur_x - objects[idx].x;
                    move_off_y = cur_y - objects[idx].y;
                    cur_color  = objects[idx].color;
                    // if (objects[idx].type == OBJ_BOX)
                    cur_bstyle = objects[idx].box_style;
                    state = STATE_MOVING;
                    snprintf(status_msg, sizeof(status_msg), "Moving object");
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Nothing here");
                }
            }
            break;

        /* confirm */
        case '\n': case KEY_ENTER:
            if (state == STATE_MOVING) {
                state = STATE_NORMAL; selected = -1;
                dirty = 1;
                snprintf(status_msg, sizeof(status_msg), "Placed");
            }
            break;

        /* cancel */
        case 27:
            if (state == STATE_MOVING) {
                state = STATE_NORMAL; selected = -1;
                snprintf(status_msg, sizeof(status_msg), "Cancelled");
            } else if (state == STATE_DRAWING_BOX) {
                state = STATE_NORMAL;
                snprintf(status_msg, sizeof(status_msg), "Cancelled");
            } else {
                selected = -1;
                snprintf(status_msg, sizeof(status_msg), "Ready");
            }
            break;

        /* delete */
        case 'd': {
            int idx = (selected >= 0) ? selected : find_object_at(cur_x, cur_y);
			logIt("idx %d", idx);
            if (idx >= 0) {
                objects[idx].active = 0;
                for (int i = idx; i < num_objects-1; i++) {
                    objects[i] = objects[i+1];
				}
                num_objects--;
                if (selected == idx)
					selected = -1;
                state = STATE_NORMAL;
                dirty = 1;
                snprintf(status_msg, sizeof(status_msg), "Deleted");
            }
            break;
        }

        /* save */
        case 'w': {
            char filename[MAX_FILENAME] = "";
            prompt_string("Save", filename, MAX_FILENAME);
            if (filename[0]) {
				save_to_file(filename);
			}
            break;
        }

        /* load */
        case 'l': {
            if (dirty) {
                render();
                if (!confirm_discard("Load file (discard current)?")) break;
            }
            char filename[MAX_FILENAME] = "";
            prompt_string("Load", filename, MAX_FILENAME);
            if (filename[0])
				load_from_file(filename);
            break;
        }

        default: break;
        }
    }

    endwin();
    return 0;
}
