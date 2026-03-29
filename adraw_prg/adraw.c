/*
 * adraw.c - ncurses drawing program
 *
 * Features:
 *   Single, double, and ASCII-style boxes and lines
 *   Place text and Unicode glyphs from a paged picker
 *   Select, move, recolor, delete, and center objects
 *   Automatic junction characters where lines/boxes meet
 *   Side panel with live status, commands, and color display
 *   Configurable panel background color
 *   Save/load with full metadata, human-readable comments
 *
 * Build: gcc -o adraw adraw.c -lncurses
 * Usage: adraw [file]       file is optional; loads screen on startup
 *
 * Controls:
 *   Arrow keys   move cursor (also drags selected object)
 *
 *   b / B        single / double-line box        (press twice)
 *   a            ASCII box  + - |                (press twice)
 *   h / H        single / double horizontal line (press twice)
 *   \            ASCII horizontal line           (press twice)
 *   v / V        single / double vertical line   (press twice)
 *   |            ASCII vertical line             (press twice)
 *   t            place text at cursor
 *   g            glyph picker popup  (arrows, PgUp/Dn, Enter)
 *
 *   s            select object under cursor / confirm move
 *   d            delete object under cursor (or selected)
 *   c            color picker
 *   C            center object  (H=horiz  V=vert  B=both)
 *   p            cycle panel background color
 *
 *   w            save to file
 *   l            load from file
 *   q            quit  (warns if unsaved changes exist)
 *   ?            help screen
 *   ESC          cancel current action
 *
 * File format  (lines starting with ## are comments, ignored on load):
 *   #@B x y w h color style          box
 *   #@L x y len color dir style      line
 *   #@T x y 0 0 color 0 "text"       text or glyph
 *
 *   color   1-7  Red Green Yellow Blue Magenta Cyan White
 *   style   0=single  1=double  2=ascii
 *   dir     0=horizontal  1=vertical
 *   Objects saved in order: boxes, lines, glyphs, text.
 *   Inline ## comments are stripped on load.
 */

#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OBJECTS  256
#define MAX_TEXT     128
#define MAX_FILENAME 256
#define PANEL_W      22      /* side panel width including border */
#define ADRAW_VERSION "1.0.0"  /* major.minor.patch */

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
/* ASCII */
static const char A_UL[] = "+";  /* + */
static const char A_UR[] = "+";  /* + */
static const char A_LL[] = "+";  /* + */
static const char A_LR[] = "+";  /* + */
static const char A_HZ[] = "-";  /* - */
static const char A_VT[] = "|";  /* | */

#define BSTYLE_SINGLE 1
#define BSTYLE_DOUBLE 2
#define BSTYLE_ASCII  0

#define CSIZ 5   /* max UTF-8 bytes per cell + NUL */
#define GCELL(r,c)  (grid + ((size_t)((r)*cols+(c))*CSIZ))
#define GSET(r,c,s) do { if((r)>=0&&(r)<gr&&(c)>=0&&(c)<cols) \
                         strncpy(GCELL(r,c),(s),CSIZ-1); } while(0)

typedef enum { OBJ_BOX, OBJ_TEXT, OBJ_LINE } ObjType;

typedef struct {
    ObjType type;
    int x, y, w, h;
    char text[MAX_TEXT];
    int  color;
    int  box_style;
    int  line_dir;   /* 0=horizontal 1=vertical */
    int  active;
} Object;

static Object objects[MAX_OBJECTS];
static int    num_objects = 0;
static int    cur_x = 0, cur_y = 0;
static int    cur_color  = 7;
static int    cur_bstyle = BSTYLE_SINGLE;

typedef enum {
    STATE_NORMAL,
    STATE_DRAWING_BOX,
    STATE_DRAWING_LINE,
    STATE_MOVING,
    STATE_COLOR_PICK,
    STATE_GLYPH_PICK,
    STATE_HELP
} AppState;

static AppState state    = STATE_NORMAL;
static int box_sx, box_sy;
static int line_sx, line_sy;  /* line start anchor */
static int line_dir_cur = 0;   /* 0=horiz 1=vert */
static int line_dbl_cur = 0;   /* 0=single 1=double */
static int selected      = -1;
static int move_off_x, move_off_y;
static int cpick_idx     = 0;
static int gpick_row     = 0;  /* glyph picker cursor row */
static int gpick_col     = 0;  /* glyph picker cursor col */
static int gpick_page    = 0;  /* glyph picker page      */
static int dirty         = 0;  /* 1 = unsaved changes exist */
static int panel_bg      = 0;  /* index into panel_bg_ids */

/* Short feedback message shown in the panel */
static char status_msg[128] = "Ready";

/* ── color tables ──────────────────────────────────────────────── */
static const int         color_ids[7] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};
static const char *const color_names[7] = {
    "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"
};

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
    /* panel pairs are initialised via reinit_panel_colors() below */
}

/* ── panel background colour options ──────────────────────────── */
static const int panel_bg_ids[] = {
    COLOR_BLACK, COLOR_BLUE, COLOR_RED, COLOR_MAGENTA,
    COLOR_GREEN, COLOR_CYAN, COLOR_YELLOW, COLOR_WHITE
};
static const char *const panel_bg_names[] = {
    "Black", "Blue", "Red", "Magenta",
    "Green", "Cyan", "Yellow", "White"
};
#define NUM_PANEL_BGS 8

static void reinit_panel_colors(void) {
    int bg      = panel_bg_ids[panel_bg];
    int body_fg = (panel_bg >= 6) ? COLOR_BLACK : COLOR_WHITE;
    int key_fg  = (panel_bg >= 6) ? COLOR_BLUE  : COLOR_YELLOW;
    int msg_fg  = (panel_bg >= 6) ? COLOR_BLACK : COLOR_WHITE;
    init_pair(20, msg_fg,  bg);
    init_pair(21, body_fg, bg);
    init_pair(22, key_fg,  bg);
}

/* canvas width = terminal width minus panel */
static int canvas_cols(void) {
    int rows, cols; (void)rows;
    getmaxyx(stdscr, rows, cols);
    return cols - PANEL_W;
}

/* ── ncurses box drawing ───────────────────────────────────────── */
static void draw_ncurses_box(int x, int y, int w, int h,
                              int color, int hi, int style)
{
    int pair = hi ? (color + 10) : color;
    attron(COLOR_PAIR(pair));

    if (style == BSTYLE_DOUBLE || style == BSTYLE_ASCII) {
        /* build top/bottom rows as a single string for correct UTF-8 rendering */
        const char *ul = (style==BSTYLE_DOUBLE) ? D_UL : A_UL;
        const char *ur = (style==BSTYLE_DOUBLE) ? D_UR : A_UR;
        const char *ll = (style==BSTYLE_DOUBLE) ? D_LL : A_LL;
        const char *lr = (style==BSTYLE_DOUBLE) ? D_LR : A_LR;
        const char *hz = (style==BSTYLE_DOUBLE) ? D_HZ : A_HZ;
        const char *vt = (style==BSTYLE_DOUBLE) ? D_VT : A_VT;
        int cw   = (style==BSTYLE_DOUBLE) ? 3 : 1;  /* bytes per char */
        int inner = w - 2;
        char *hrow = malloc((size_t)(cw + inner * cw + cw + 1));
        memcpy(hrow,                  ul, cw);
        for (int i = 0; i < inner; i++) memcpy(hrow + cw + i*cw, hz, cw);
        memcpy(hrow + cw + inner*cw,  ur, cw);
        hrow[cw + inner*cw + cw] = '\0';
        mvaddstr(y, x, hrow);
        memcpy(hrow,                  ll, cw);
        for (int i = 0; i < inner; i++) memcpy(hrow + cw + i*cw, hz, cw);
        memcpy(hrow + cw + inner*cw,  lr, cw);
        hrow[cw + inner*cw + cw] = '\0';
        mvaddstr(y+h-1, x, hrow);
        free(hrow);
        for (int i = 1; i < h-1; i++) {
            mvaddstr(y+i, x,     vt);
            mvaddstr(y+i, x+w-1, vt);
        }
    } else {
        /* BSTYLE_SINGLE: use ACS constants */
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

/* ── draw a line object ────────────────────────────────────────── */
static void draw_line_obj(int x, int y, int len, int dir, int style, int color, int hi) {
    if (len < 1) return;
    int pair = hi ? (color + 10) : color;
    attron(COLOR_PAIR(pair));

    const char *hch = (style==BSTYLE_DOUBLE) ? D_HZ : (style==BSTYLE_ASCII) ? A_HZ : U_HZ;
    const char *vch = (style==BSTYLE_DOUBLE) ? D_VT : (style==BSTYLE_ASCII) ? A_VT : U_VT;
    int cw = (style==BSTYLE_DOUBLE || style==BSTYLE_SINGLE) ? 3 : 1;

    if (dir == 0) {
        /* horizontal — one mvaddstr of the full row */
        char *buf = malloc((size_t)len * cw + 1);
        if (!buf) { attroff(COLOR_PAIR(pair)); return; }
        for (int i = 0; i < len; i++) memcpy(buf + i*cw, hch, cw);
        buf[len*cw] = '\0';
        mvaddstr(y, x, buf);
        free(buf);
    } else {
        /* vertical — one mvaddstr per cell */
        for (int i = 0; i < len; i++)
            mvaddstr(y + i, x, vch);
    }
    attroff(COLOR_PAIR(pair));
}

/* ── junction rendering ────────────────────────────────────────── */
/*
 * At any cell (x,y) multiple line/box edges may meet. We compute which
 * of the four directions (L R U D) carry a stroke and whether each is
 * single (0) or double (1), then look up the Unicode box-drawing char.
 *
 * stroke_info: per-direction: bit0 = present, bit1 = double
 *   index: 0=L 1=R 2=U 3=D
 */

/* Return stroke info (0=none,1=single,3=double) for one direction at (x,y)
   contributed by object idx (pass -1 to check all objects). */
static int cell_stroke(int x, int y, int dir, int only_idx) {
    /* dir: 0=L 1=R 2=U 3=D  — does this cell have a stroke going that way? */
    int result = 0;
    int start = (only_idx >= 0) ? only_idx : 0;
    int end   = (only_idx >= 0) ? only_idx+1 : num_objects;

    for (int i = start; i < end; i++) {
        Object *o = &objects[i];
        if (!o->active) continue;
        if (o->box_style == BSTYLE_ASCII) continue; /* ASCII skips junction logic */
        int dbl = (o->box_style == BSTYLE_DOUBLE) ? 2 : 1;

        if (o->type == OBJ_LINE) {
            if (o->line_dir == 0) {  /* horizontal line */
                if (y != o->y) continue;
                if (x < o->x || x >= o->x + o->w) continue;
                /* this cell is ON a horizontal line */
                if (dir == 0 && x > o->x)            result |= dbl; /* L */
                if (dir == 1 && x < o->x + o->w - 1) result |= dbl; /* R */
            } else {                  /* vertical line */
                if (x != o->x) continue;
                if (y < o->y || y >= o->y + o->w) continue;
                /* this cell is ON a vertical line */
                if (dir == 2 && y > o->y)            result |= dbl; /* U */
                if (dir == 3 && y < o->y + o->w - 1) result |= dbl; /* D */
            }
        } else if (o->type == OBJ_BOX) {
            int x1=o->x, y1=o->y, x2=o->x+o->w-1, y2=o->y+o->h-1;
            /* top edge */
            if (y == y1 && x >= x1 && x <= x2) {
                if (dir == 0 && x > x1)  result |= dbl;
                if (dir == 1 && x < x2)  result |= dbl;
            }
            /* bottom edge */
            if (y == y2 && x >= x1 && x <= x2) {
                if (dir == 0 && x > x1)  result |= dbl;
                if (dir == 1 && x < x2)  result |= dbl;
            }
            /* left edge */
            if (x == x1 && y >= y1 && y <= y2) {
                if (dir == 2 && y > y1)  result |= dbl;
                if (dir == 3 && y < y2)  result |= dbl;
            }
            /* right edge */
            if (x == x2 && y >= y1 && y <= y2) {
                if (dir == 2 && y > y1)  result |= dbl;
                if (dir == 3 && y < y2)  result |= dbl;
            }
        }
    }
    return result;
}

/*
 * junction_utf8: given stroke presence/weight for L R U D
 *   (0=none, 1=single, 2=double), return the UTF-8 box-drawing char.
 * We encode the four directions as a 4-digit base-3 number: L*27+R*9+U*3+D
 */
static const char *junction_utf8(int L, int R, int U, int D) {
    /* clamp to 0/1/2 */
    if (L>2)L=2; if (R>2)R=2; if (U>2)U=2; if (D>2)D=2;

    /* Only render a junction char when at least two directions are active */
    int count = (L>0)+(R>0)+(U>0)+(D>0);
    if (count < 2) return NULL;

    /* --- All-single junctions --- */
    if (L==1&&R==1&&U==0&&D==0) return U_HZ;   /* ─ */
    if (L==0&&R==0&&U==1&&D==1) return U_VT;   /* │ */
    if (L==0&&R==1&&U==0&&D==1) return U_UL;   /* ┌ */
    if (L==1&&R==0&&U==0&&D==1) return "\xe2\x94\x90"; /* ┐ */
    if (L==0&&R==1&&U==1&&D==0) return "\xe2\x94\x94"; /* └ */
    if (L==1&&R==0&&U==1&&D==0) return "\xe2\x94\x98"; /* ┘ */
    if (L==1&&R==1&&U==0&&D==1) return "\xe2\x94\xac"; /* ┬ */
    if (L==1&&R==1&&U==1&&D==0) return "\xe2\x94\xb4"; /* ┴ */
    if (L==0&&R==1&&U==1&&D==1) return "\xe2\x94\x9c"; /* ├ */
    if (L==1&&R==0&&U==1&&D==1) return "\xe2\x94\xa4"; /* ┤ */
    if (L==1&&R==1&&U==1&&D==1) return "\xe2\x94\xbc"; /* ┼ */

    /* --- All-double junctions --- */
    if (L==2&&R==2&&U==0&&D==0) return D_HZ;
    if (L==0&&R==0&&U==2&&D==2) return D_VT;
    if (L==0&&R==2&&U==0&&D==2) return D_UL;   /* ╔ */
    if (L==2&&R==0&&U==0&&D==2) return D_UR;   /* ╗ */
    if (L==0&&R==2&&U==2&&D==0) return D_LL;   /* ╚ */
    if (L==2&&R==0&&U==2&&D==0) return D_LR;   /* ╝ */
    if (L==2&&R==2&&U==0&&D==2) return "\xe2\x95\xa6"; /* ╦ */
    if (L==2&&R==2&&U==2&&D==0) return "\xe2\x95\xa9"; /* ╩ */
    if (L==0&&R==2&&U==2&&D==2) return "\xe2\x95\xa0"; /* ╠ */
    if (L==2&&R==0&&U==2&&D==2) return "\xe2\x95\xa3"; /* ╣ */
    if (L==2&&R==2&&U==2&&D==2) return "\xe2\x95\xac"; /* ╬ */

    /* --- Mixed single/double junctions --- */
    /* single-H crosses double-V */
    if (L==1&&R==1&&U==2&&D==2) return "\xe2\x95\xab"; /* ╫ */
    if (L==1&&R==1&&U==2&&D==0) return "\xe2\x95\xa8"; /* ╨ */
    if (L==1&&R==1&&U==0&&D==2) return "\xe2\x95\xa5"; /* ╥ */
    /* double-H crosses single-V */
    if (L==2&&R==2&&U==1&&D==1) return "\xe2\x95\xaa"; /* ╪ */
    if (L==2&&R==2&&U==1&&D==0) return "\xe2\x95\xa7"; /* ╧ */
    if (L==2&&R==2&&U==0&&D==1) return "\xe2\x95\xa4"; /* ╤ */
    /* single-H meets double-V (T junctions) */
    if (L==0&&R==1&&U==2&&D==2) return "\xe2\x95\x9e"; /* ╞ */
    if (L==1&&R==0&&U==2&&D==2) return "\xe2\x95\xa1"; /* ╡ */
    /* double-H meets single-V */
    if (L==2&&R==0&&U==1&&D==1) return "\xe2\x95\x95"; /* ╕ */
    if (L==0&&R==2&&U==1&&D==1) return "\xe2\x95\x92"; /* ╒ */
    /* corners: single meets double */
    if (L==0&&R==1&&U==0&&D==2) return "\xe2\x95\x93"; /* ╓ */
    if (L==1&&R==0&&U==0&&D==2) return "\xe2\x95\x96"; /* ╖ */
    if (L==0&&R==1&&U==2&&D==0) return "\xe2\x95\x99"; /* ╙ */
    if (L==1&&R==0&&U==2&&D==0) return "\xe2\x95\x9c"; /* ╜ */
    if (L==0&&R==2&&U==0&&D==1) return "\xe2\x95\x92"; /* ╒ */
    if (L==2&&R==0&&U==0&&D==1) return "\xe2\x95\x95"; /* ╕ */
    if (L==0&&R==2&&U==1&&D==0) return "\xe2\x95\x98"; /* ╘ */
    if (L==2&&R==0&&U==1&&D==0) return "\xe2\x95\x9b"; /* ╛ */

    /* fallback: just use single or double based on majority */
    int sdbl = (L==2||R==2||U==2||D==2);
    if (L&&R&&!U&&!D) return sdbl ? D_HZ : U_HZ;
    if (!L&&!R&&U&&D) return sdbl ? D_VT : U_VT;
    return sdbl ? "\xe2\x95\xac" : "\xe2\x94\xbc";
}

/*
 * After all objects are drawn, scan every cell that is a potential
 * junction (where ≥2 objects contribute strokes) and overdraw with the
 * correct combined character.
 */
static void draw_junctions(void) {
    int rows, cols_total;
    getmaxyx(stdscr, rows, cols_total);
    int cw = cols_total - PANEL_W;

    /* Collect all cells that belong to any line or box border */
    /* We iterate over all pairs of objects and check their overlap cells */
    for (int i = 0; i < num_objects; i++) {
        Object *oi = &objects[i];
        if (!oi->active) continue;
        if (oi->type != OBJ_LINE && oi->type != OBJ_BOX) continue;

        /* Determine the set of cells this object contributes */
        int x0,y0,x1,y1;
        if (oi->type == OBJ_LINE) {
            x0 = oi->x; y0 = oi->y;
            if (oi->line_dir == 0) { x1 = oi->x+oi->w-1; y1 = oi->y; }
            else                   { x1 = oi->x;          y1 = oi->y+oi->w-1; }
        } else { /* BOX: only border cells */
            x0 = oi->x; y0 = oi->y;
            x1 = oi->x+oi->w-1; y1 = oi->y+oi->h-1;
        }

        /* Walk border cells of this object */
        int cx, cy;
        for (int pass = 0; pass < (oi->type==OBJ_BOX ? 4 : 1); pass++) {
            /* pass 0=horiz scan, pass 1=vert scan for lines;
               pass 0=top,1=bottom,2=left,3=right for boxes */
            int lx0,lx1,ly0,ly1;
            if (oi->type == OBJ_LINE) {
                lx0=x0; lx1=x1; ly0=y0; ly1=y1;
            } else {
                if      (pass==0){lx0=x0;lx1=x1;ly0=y0;ly1=y0;}
                else if (pass==1){lx0=x0;lx1=x1;ly0=y1;ly1=y1;}
                else if (pass==2){lx0=x0;lx1=x0;ly0=y0;ly1=y1;}
                else             {lx0=x1;lx1=x1;ly0=y0;ly1=y1;}
            }
            for (cy=ly0; cy<=ly1; cy++) {
                for (cx=lx0; cx<=lx1; cx++) {
                    if (cx < 0 || cx >= cw || cy < 0 || cy >= rows) continue;

                    int L = cell_stroke(cx, cy, 0, -1);
                    int R = cell_stroke(cx, cy, 1, -1);
                    int U = cell_stroke(cx, cy, 2, -1);
                    int D = cell_stroke(cx, cy, 3, -1);

                    int count = (L>0)+(R>0)+(U>0)+(D>0);
                    if (count < 2) continue;  /* not a junction */

                    /* find the color and style of the most-recently-placed
                       object contributing to this cell */
                    int use_color = 7;
                    int use_ascii = 0;  /* 1 if all contributors are ASCII */
                    int ascii_contributors = 0, total_contributors = 0;
                    for (int k = num_objects-1; k >= 0; k--) {
                        Object *ok = &objects[k];
                        if (!ok->active) continue;
                        int contributes = 0;
                        if (ok->type == OBJ_LINE) {
                            if (ok->line_dir==0 && cy==ok->y && cx>=ok->x && cx<ok->x+ok->w)
                                contributes = 1;
                            if (ok->line_dir==1 && cx==ok->x && cy>=ok->y && cy<ok->y+ok->w)
                                contributes = 1;
                        } else if (ok->type == OBJ_BOX) {
                            int bx1=ok->x,by1=ok->y,bx2=ok->x+ok->w-1,by2=ok->y+ok->h-1;
                            if ((cy==by1||cy==by2) && cx>=bx1 && cx<=bx2)
                                contributes = 1;
                            if ((cx==bx1||cx==bx2) && cy>=by1 && cy<=by2)
                                contributes = 1;
                        }
                        if (contributes) {
                            total_contributors++;
                            if (ok->box_style == BSTYLE_ASCII) ascii_contributors++;
                            if (use_color == 7) use_color = ok->color;
                        }
                    }
                    /* If any contributor is ASCII style, use + for the junction */
                    if (ascii_contributors > 0) {
                        attron(COLOR_PAIR(use_color));
                        mvaddstr(cy, cx, "+");
                        attroff(COLOR_PAIR(use_color));
                        continue;
                    }

                    const char *jch = junction_utf8(L, R, U, D);
                    if (!jch) continue;
                    attron(COLOR_PAIR(use_color));
                    mvaddstr(cy, cx, jch);
                    attroff(COLOR_PAIR(use_color));
                }
            }
        }
    }
}

static void draw_object(int idx) {
    Object *o = &objects[idx];
    if (!o->active) return;
    int hi = (idx == selected);
    if (o->type == OBJ_BOX) {
        draw_ncurses_box(o->x, o->y, o->w, o->h,
                         o->color, hi, o->box_style);
    } else if (o->type == OBJ_LINE) {
        draw_line_obj(o->x, o->y, o->w, o->line_dir,
                      o->box_style, o->color, hi);
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
    mvprintw(0, px+1, " ADRAW %-*s", iw-7, ADRAW_VERSION);
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
        { "a",       "ASCII box"      },
        { "t",       "Place text"    },
        { "s",       "Select/move"   },
        { "C",       "Center H/V/B"  },
        { "d",       "Delete"        },
        { "c",       "Color picker"  },
        { "w",       "Save file"     },
        { "h/H/\\",  "Horiz line"    },
        { "v/V/|",   "Vert line"     },
        { "p",       "Panel color"   },
        { "g",       "Glyph picker"  },
        { "l",       "Load file"     },
        { "q",       "Quit"          },
        { "Arrows",  "Move cursor"   },
        { "Enter",   "Confirm"       },
        { "ESC",     "Cancel"        },
        { "?",       "Help"           },
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
    else if (state == STATE_HELP)         state_str = "Help";
    else if (state == STATE_DRAWING_LINE) state_str = "Drawing line";

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
             cur_bstyle == BSTYLE_DOUBLE ? "Double" :
             cur_bstyle == BSTYLE_ASCII  ? "ASCII"  : "Single");

    /* Cursor position */
    mvprintw(row++, px+2, "Pos:   %d, %d", cur_x, cur_y);

    /* Panel background colour */
    mvprintw(row, px+2, "Panel: ");
    attroff(COLOR_PAIR(21));
    attron(COLOR_PAIR(21) | A_REVERSE);
    mvprintw(row, px+9, " %-8s", panel_bg_names[panel_bg]);
    attroff(COLOR_PAIR(21) | A_REVERSE);
    attron(COLOR_PAIR(21));
    row++;

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
      "\xe2\x94\xac\xe2\x94\xb4\xe2\x94\xbc\xe2\x95\xb4\xe2\x95\xb5\xe2\x95\xb6\xe2\x95\xb7"
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

#define GPICK_COLS 16   /* characters per row in the picker grid */

static void draw_glyph_picker(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int cat = gpick_page;
    if (cat < 0) cat = 0;
    if (cat >= NUM_GLYPH_CATS) cat = NUM_GLYPH_CATS - 1;

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
    for (int r = 1; r <= ph; r++) mvhline(py+r, px+2, ' ', pw);
    attroff(A_DIM | COLOR_PAIR(21));

    /* border + background */
    attron(COLOR_PAIR(21) | A_BOLD);
    for (int r = 0; r < ph; r++) mvhline(py+r, px, ' ', pw);
    mvaddch(py,      px,      ACS_ULCORNER);
    mvaddch(py,      px+pw-1, ACS_URCORNER);
    mvaddch(py+ph-1, px,      ACS_LLCORNER);
    mvaddch(py+ph-1, px+pw-1, ACS_LRCORNER);
    for (int i=1;i<pw-1;i++){mvaddch(py,px+i,ACS_HLINE);mvaddch(py+ph-1,px+i,ACS_HLINE);}
    for (int i=1;i<ph-1;i++){mvaddch(py+i,px,ACS_VLINE);mvaddch(py+i,px+pw-1,ACS_VLINE);}

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
    if (cat < 0) cat = 0;
    if (cat >= NUM_GLYPH_CATS) cat = NUM_GLYPH_CATS - 1;
    const char *chars = glyph_cats[cat].chars;
    int total  = utf8_count(chars);
    int gcols  = GPICK_COLS;
    int grows  = (total + gcols - 1) / gcols;

    /* clamp cursor */
    if (gpick_row >= grows) gpick_row = grows - 1;
    int max_col = (gpick_row == grows-1)
                  ? ((total-1) % gcols)
                  : gcols - 1;
    if (gpick_col > max_col) gpick_col = max_col;

    switch (ch) {
    case KEY_UP:
        if (gpick_row > 0) gpick_row--;
        break;
    case KEY_DOWN:
        if (gpick_row < grows-1) gpick_row++;
        break;
    case KEY_LEFT:
        if (gpick_col > 0) gpick_col--;
        else if (gpick_row > 0) { gpick_row--; gpick_col = gcols-1; }
        break;
    case KEY_RIGHT: {
        int mc = (gpick_row == grows-1) ? ((total-1)%gcols) : gcols-1;
        if (gpick_col < mc) gpick_col++;
        else if (gpick_row < grows-1) { gpick_row++; gpick_col = 0; }
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
            o->type   = OBJ_TEXT;
            o->x      = place_x; o->y = place_y;
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

/* ── help screen ───────────────────────────────────────────────── */
static void draw_help_screen(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int pw = cw - 4;
    if (pw > 74) pw = 74;
    int ph = rows - 4;
    if (ph > 36) ph = 36;
    int px = (cw - pw) / 2;
    int py = (rows - ph) / 2;

    /* shadow */
    attron(A_DIM | COLOR_PAIR(21));
    for (int r = 1; r <= ph; r++) mvhline(py+r, px+2, ' ', pw);
    attroff(A_DIM | COLOR_PAIR(21));

    /* box + background */
    attron(COLOR_PAIR(21) | A_BOLD);
    for (int r = 0; r < ph; r++) mvhline(py+r, px, ' ', pw);
    mvaddch(py,      px,      ACS_ULCORNER);
    mvaddch(py,      px+pw-1, ACS_URCORNER);
    mvaddch(py+ph-1, px,      ACS_LLCORNER);
    mvaddch(py+ph-1, px+pw-1, ACS_LRCORNER);
    for (int i=1;i<pw-1;i++){mvaddch(py,px+i,ACS_HLINE);mvaddch(py+ph-1,px+i,ACS_HLINE);}
    for (int i=1;i<ph-1;i++){mvaddch(py+i,px,ACS_VLINE);mvaddch(py+i,px+pw-1,ACS_VLINE);}
    mvprintw(py, px+(pw-8)/2, "  Help  ");
    attroff(COLOR_PAIR(21) | A_BOLD);

    int iw = pw - 4;   /* inner width */
    int row = py + 1;

    /* helper macros rendered inline */
    #define HL_HEAD(title) \
        attron(COLOR_PAIR(22) | A_BOLD | A_UNDERLINE); \
        mvprintw(row++, px+2, "%-*s", iw, (title)); \
        attroff(COLOR_PAIR(22) | A_BOLD | A_UNDERLINE);

    #define HL_ROW(key, desc) \
        attron(COLOR_PAIR(22) | A_BOLD); \
        mvprintw(row, px+2,   "%-10s", (key)); \
        attroff(COLOR_PAIR(22) | A_BOLD); \
        attron(COLOR_PAIR(21)); \
        mvprintw(row, px+12,  "%-*s", iw-10, (desc)); \
        attroff(COLOR_PAIR(21)); \
        row++;

    #define HL_BLANK() \
        attron(COLOR_PAIR(21)); \
        mvhline(row++, px+2, ' ', iw); \
        attroff(COLOR_PAIR(21));

    HL_HEAD("Drawing");
    HL_ROW("b / B",       "Single / double-line box  (press twice)");
    HL_ROW("a",           "ASCII box using + - |  (press twice)");
    HL_ROW("h / H",       "Single / double horizontal line  (press twice)");
    HL_ROW("\\ ",          "ASCII horizontal line  (press twice)");
    HL_ROW("v / V",       "Single / double vertical line  (press twice)");
    HL_ROW("|",           "ASCII vertical line  (press twice)");
    HL_ROW("t",           "Place text at cursor");
    HL_ROW("g",           "Glyph picker  (arrows, PgUp/Dn, Enter)");
    HL_BLANK();
    HL_HEAD("Editing");
    HL_ROW("s",           "Select object under cursor / place when moving");
    HL_ROW("d",           "Delete object under cursor (or selected)");
    HL_ROW("c",           "Color picker popup");
    HL_ROW("C",           "Center object  (H=horiz  V=vert  B=both)");
    HL_ROW("p",           "Cycle panel background color");
    HL_BLANK();
    HL_HEAD("Navigation");
    HL_ROW("Arrow keys",  "Move cursor  (also drags selected object)");
    HL_ROW("Enter",       "Confirm / place selected object");
    HL_ROW("ESC",         "Cancel current action");
    HL_BLANK();
    HL_HEAD("File");
    HL_ROW("w",           "Save to file");
    HL_ROW("l",           "Load from file");
    HL_ROW("q",           "Quit  (warns if unsaved changes)");
    HL_BLANK();
    HL_HEAD("File format");
    HL_ROW("#@B",         "Box:  x y w h color style");
    HL_ROW("#@L",         "Line: x y len color dir style");
    HL_ROW("#@T",         "Text/glyph: x y 0 0 color 0 \"text\"");
    HL_ROW("##",          "Comment line (ignored on load)");
    HL_BLANK();

    #undef HL_HEAD
    #undef HL_ROW
    #undef HL_BLANK

    attron(COLOR_PAIR(20) | A_BOLD);
    mvprintw(py+ph-2, px+2, "%-*s", iw, "Press any key to close");
    attroff(COLOR_PAIR(20) | A_BOLD);
}

/* ── color picker popup ────────────────────────────────────────── */
static void draw_color_picker(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int cw = canvas_cols();

    int pw = 26, ph = 11;
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
    for (int i = 0; i < num_objects; i++)
        draw_object(i);

    /* overdraw junction characters where lines/boxes meet */
    draw_junctions();

    /* live box preview */
    if (state == STATE_DRAWING_BOX) {
        int x1 = box_sx < cur_x ? box_sx : cur_x;
        int y1 = box_sy < cur_y ? box_sy : cur_y;
        int x2 = box_sx > cur_x ? box_sx : cur_x;
        int y2 = box_sy > cur_y ? box_sy : cur_y;
        int w = x2-x1+1, h = y2-y1+1;
        if (w >= 2 && h >= 2)
            draw_ncurses_box(x1, y1, w, h, cur_color, 0, cur_bstyle);
    }

    /* live line preview */
    if (state == STATE_DRAWING_LINE) {
        if (line_dir_cur == 0) {
            int x1 = line_sx < cur_x ? line_sx : cur_x;
            int len = abs(cur_x - line_sx) + 1;
            draw_line_obj(x1, line_sy, len, 0, cur_bstyle, cur_color, 0);
        } else {
            int y1 = line_sy < cur_y ? line_sy : cur_y;
            int len = abs(cur_y - line_sy) + 1;
            draw_line_obj(line_sx, y1, len, 1, cur_bstyle, cur_color, 0);
        }
    }

    /* side panel (drawn last so it always overlays canvas edge) */
    draw_panel();

    if (state == STATE_COLOR_PICK)
        draw_color_picker();

    if (state == STATE_GLYPH_PICK)
        draw_glyph_picker();

    if (state == STATE_HELP)
        draw_help_screen();

    move(cur_y, cur_x);
    refresh();
}

/* ── find topmost object at canvas position ────────────────────── */
static int find_object_at(int x, int y) {
    for (int i = num_objects-1; i >= 0; i--) {
        Object *o = &objects[i];
        if (!o->active) continue;
        if (o->type == OBJ_BOX) {
            if (x >= o->x && x < o->x+o->w && y >= o->y && y < o->y+o->h)
                return i;
        } else if (o->type == OBJ_LINE) {
            if (o->line_dir == 0) {  /* horizontal */
                if (y == o->y && x >= o->x && x < o->x + o->w)
                    return i;
            } else {                  /* vertical */
                if (x == o->x && y >= o->y && y < o->y + o->w)
                    return i;
            }
        } else {
            int len = (int)strlen(o->text);
            if (y == o->y && x >= o->x && x < o->x+len)
                return i;
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
 * File format: one "#@" metadata line per object, nothing else.
 *   #@B x y w h color style        — box
 *   #@L x y len color dir style    — line
 *   #@T x y 0 0 color 0 text...   — text
 * Lines beginning with "##" are comments and are ignored on load.
 */
/* Return 1 if a text object's content is a single non-ASCII (glyph) character */
static int is_glyph_obj(const Object *o) {
    if (o->type != OBJ_TEXT) return 0;
    const unsigned char *p = (const unsigned char *)o->text;
    if (*p == 0 || *p < 0x80) return 0;   /* empty or plain ASCII */
    /* decode one UTF-8 codepoint and check nothing follows */
    int len = (*p < 0xE0) ? 2 : (*p < 0xF0) ? 3 : 4;
    for (int i = 1; i < len; i++)
        if ((p[i] & 0xC0) != 0x80) return 0;  /* bad continuation */
    return p[len] == 0;  /* true only if exactly one multibyte char */
}

static void save_to_file(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        snprintf(status_msg, sizeof(status_msg), "Can't open '%s'", filename);
        return;
    }

    fprintf(f, "## ncurses is zero relative but the agui library is one relative.\n");
    fprintf(f, "## The agui library will adjust the row and col\n");
    fprintf(f, "## row col width height color dir style \"text\"\n");
    fprintf(f, "## color       - (0-6) RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE\n");
    fprintf(f, "## dir         - Direction Vertical=0, Horizontal=1\n");
    fprintf(f, "## style       - Double=2, single=1 or ascii=0 line graphics\n");
    fprintf(f, "## All blank lines need to start with ##\n");
    fprintf(f, "## The objects are listed in order of Boxes, Lines, Glyphs, Text.\n");
    fprintf(f, "## When list is searched, it is done in reverse order or bottom to top.\n");
    fprintf(f, "##\n");

    /* Write in order: BOX, LINE, TEXT, GLYPH */
    static const int pass_types[4] = { 0, 0, 0, 0 }; /* unused — use pass logic */
    for (int pass = 0; pass < 4; pass++) {
        for (int i = 0; i < num_objects; i++) {
            Object *o = &objects[i];
            if (!o->active) continue;
            int glyph = is_glyph_obj(o);
            /* pass 0=BOX, 1=LINE, 2=TEXT(non-glyph), 3=GLYPH */
            if (pass == 0 && o->type != OBJ_BOX)              continue;
            if (pass == 1 && o->type != OBJ_LINE)             continue;
            if (pass == 2 && !(o->type == OBJ_TEXT && !glyph)) continue;
            if (pass == 3 && !(o->type == OBJ_TEXT &&  glyph)) continue;

            /* human-readable style/color/dir labels for inline comment */
            static const char *snames[] = { "single", "double", "ascii" };
            static const char *dnames[] = { "horizontal", "vertical" };
            const char *cname = (o->color >= 1 && o->color <= 7)
                                ? color_names[o->color - 1] : "?";

            if (o->type == OBJ_BOX) {
                const char *sname = (o->box_style >= 0 && o->box_style <= 2)
                                    ? snames[o->box_style] : "?";
                fprintf(f, "#@B %3d %3d %3d %3d %3d %3d"
                        " ## Box col=%d row=%d w=%d h=%d color=%s style=%s\n",
                        o->x, o->y, o->w, o->h, o->color, o->box_style,
                        o->x, o->y, o->w, o->h, cname, sname);
            } else if (o->type == OBJ_LINE) {
                const char *sname = (o->box_style >= 0 && o->box_style <= 2)
                                    ? snames[o->box_style] : "?";
                const char *dname = (o->line_dir == 0 || o->line_dir == 1)
                                    ? dnames[o->line_dir] : "?";
                fprintf(f, "#@L %3d %3d %3d %3d %3d %3d"
                        " ## Line col=%d row=%d len=%d color=%s dir=%s style=%s\n",
                        o->x, o->y, o->w, o->color, o->line_dir, o->box_style,
                        o->x, o->y, o->w, cname, dname, sname);
            } else {
                /* TEXT and GLYPH: quote the text so spaces/special chars are preserved */
                const char *kind = is_glyph_obj(o) ? "Glyph" : "Text";
                fprintf(f, "#@T %3d %3d   0   0 %3d   0 \"%s\""
                        " ## %s col=%d row=%d color=%s text=%s\n",
                        o->x, o->y, o->color, o->text,
                        kind, o->x, o->y, cname, o->text);
            }
        }
    }
    fclose(f);
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

    /* ── Pass 1: read metadata lines ── */
    /* Lines starting with ## are comments (ignored).
       Inline ## comments are stripped from #@ lines.
       Any non-# line ends the metadata section. */
    int has_meta = 0;
    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), f)) {
        /* strip trailing newline */
        int ll = (int)strlen(linebuf);
        while (ll > 0 && (linebuf[ll-1]=='\n'||linebuf[ll-1]=='\r'))
            linebuf[--ll] = '\0';

        /* blank lines: skip */
        if (linebuf[0] == '\0') continue;

        /* ## comment lines: skip entirely */
        if (linebuf[0] == '#' && linebuf[1] == '#') continue;

        /* non-# lines end the metadata section (legacy canvas or EOF) */
        if (linebuf[0] != '#' || linebuf[1] != '@') break;

        /* strip inline ## comment: find first " ##" or "##" after the data */
        char *comment = strstr(linebuf + 2, "##");
        if (comment) *comment = '\0';

        has_meta = 1;
        if (num_objects >= MAX_OBJECTS) continue;

        char type = linebuf[2];
        int x=0,y=0,w=0,h=0,color=7,style=0;
        char text[MAX_TEXT] = "";

        if (type == 'B') {
            sscanf(linebuf+3, "%d %d %d %d %d %d", &x,&y,&w,&h,&color,&style);
        } else if (type == 'L') {
            /* fields: x y len color dir style */
            int ldir = 0;
            sscanf(linebuf+3, "%d %d %d %d %d %d", &x,&y,&w,&color,&ldir,&style);
            h = ldir;  /* borrow h to carry dir through to reconstruction */
        } else if (type == 'T') {
            int n = sscanf(linebuf+3, "%d %d %*d %*d %d %*d ", &x,&y,&color);
            if (n == 3) {
                /* text starts after the 6 integers */
                const char *p = linebuf+3;
                int fields = 0;
                while (*p && fields < 6) {
                    while (*p == ' ') p++;
                    while (*p && *p != ' ') p++;
                    fields++;
                }
                while (*p == ' ') p++;
                int tlen = (int)strlen(p);
                /* strip trailing newline, carriage-return, AND spaces
                   (spaces appear when an inline ## comment was removed) */
                while (tlen > 0 && (p[tlen-1]=='\n'||p[tlen-1]=='\r'||p[tlen-1]==' ')) tlen--;
                /* strip surrounding double-quotes if present */
                if (tlen >= 2 && p[0] == '"' && p[tlen-1] == '"') {
                    p++;
                    tlen -= 2;
                }
                if (tlen > 0) {
                    strncpy(text, p, tlen < MAX_TEXT-1 ? tlen : MAX_TEXT-2);
                    text[tlen < MAX_TEXT-1 ? tlen : MAX_TEXT-2] = '\0';
                }
            }
        } else {
            continue;
        }

        /* clamp color to valid range */
        if (color < 1 || color > 7) color = 7;

        Object *o = &objects[num_objects++];
        memset(o, 0, sizeof *o);
        o->active    = 1;
        o->x         = x; o->y = y;
        o->color     = color;
        if (type == 'B') {
            o->type      = OBJ_BOX;
            o->w         = w; o->h = h;
            o->box_style = style;
        } else if (type == 'L') {
            o->type      = OBJ_LINE;
            o->w         = w;
            o->line_dir  = h;      /* h was used to carry dir */
            o->box_style = style;
        } else {
            o->type = OBJ_TEXT;
            strncpy(o->text, text, MAX_TEXT-1);
        }
    }

    /* If we had a metadata header, we're done — objects fully restored */
    fclose(f);

    if (!has_meta) {
        /* ── Legacy / external file: fall back to shape detection ── */
        FILE *f2 = fopen(filename, "r");
        if (!f2) { snprintf(status_msg,sizeof(status_msg),"Can't reopen"); return; }

        int rows, cols_total;
        getmaxyx(stdscr, rows, cols_total);
        int cols = cols_total - PANEL_W;
        int gr   = rows;

        char *grid = calloc((size_t)(gr * cols * CSIZ), 1);
        if (!grid) { fclose(f2); snprintf(status_msg,sizeof(status_msg),"Out of memory!"); return; }
        for (int r = 0; r < gr; r++)
            for (int c = 0; c < cols; c++)
                GCELL(r,c)[0] = ' ', GCELL(r,c)[1] = '\0';

        int row = 0;
        while (row < gr && fgets(linebuf, sizeof(linebuf), f2)) {
            int llen = (int)strlen(linebuf);
            while (llen > 0 && (linebuf[llen-1]=='\n'||linebuf[llen-1]=='\r'))
                linebuf[--llen] = '\0';
            int col = 0;
            const char *p = linebuf;
            while (*p && col < cols) {
                char cell[CSIZ] = {0};
                int consumed = read_utf8(p, cell);
                if (!consumed) break;
                strncpy(GCELL(row, col), cell, CSIZ-1);
                p += consumed; col++;
            }
            row++;
        }
        fclose(f2);

        char *used = calloc((size_t)(gr * cols), 1);
        if (!used) { free(grid); snprintf(status_msg,sizeof(status_msg),"Out of memory!"); return; }

        num_objects = 0;

        for (int r = 0; r < gr && num_objects < MAX_OBJECTS; r++) {
            for (int c = 0; c < cols && num_objects < MAX_OBJECTS; c++) {
                char *cell = GCELL(r, c);
                int is_single = cell_is(cell, U_UL);
                int is_double = cell_is(cell, D_UL);
                int is_ascii  = (cell[0] == '+' && cell[1] == '\0');
                if (!is_single && !is_double && !is_ascii) continue;
                int bstyle = is_double ? BSTYLE_DOUBLE : is_ascii ? BSTYLE_ASCII : BSTYLE_SINGLE;
                const char *hz = (bstyle==BSTYLE_DOUBLE)?D_HZ:(bstyle==BSTYLE_ASCII)?A_HZ:U_HZ;
                const char *vt = (bstyle==BSTYLE_DOUBLE)?D_VT:(bstyle==BSTYLE_ASCII)?A_VT:U_VT;
                const char *ur = (bstyle==BSTYLE_DOUBLE)?D_UR:(bstyle==BSTYLE_ASCII)?A_UR:U_UR;
                const char *ll = (bstyle==BSTYLE_DOUBLE)?D_LL:(bstyle==BSTYLE_ASCII)?A_LL:U_LL;
                const char *lr = (bstyle==BSTYLE_DOUBLE)?D_LR:(bstyle==BSTYLE_ASCII)?A_LR:U_LR;
                int w=0;
                for (int cc=c+1;cc<cols;cc++) {
                    char *tc=GCELL(r,cc);
                    if (cell_is(tc,hz)){w=cc-c+1;continue;}
                    if (cell_is(tc,ur)){w=cc-c+1;break;}
                    w=0;break;
                }
                if (w<2) continue;
                int h=0;
                for (int rr=r+1;rr<gr;rr++) {
                    char *lc=GCELL(rr,c);
                    if (cell_is(lc,vt)){h=rr-r+1;continue;}
                    if (cell_is(lc,ll)){h=rr-r+1;break;}
                    h=0;break;
                }
                if (h<2) continue;
                if (!cell_is(GCELL(r+h-1,c+w-1),lr)) continue;
                Object *o=&objects[num_objects++];
                memset(o,0,sizeof *o);
                o->type=OBJ_BOX;o->x=c;o->y=r;o->w=w;o->h=h;
                o->color=cur_color;o->box_style=bstyle;o->active=1;
                for (int cc=c;cc<c+w;cc++){used[r*cols+cc]=1;used[(r+h-1)*cols+cc]=1;}
                for (int rr=r;rr<r+h;rr++){used[rr*cols+c]=1;used[rr*cols+c+w-1]=1;}
            }
        }
        for (int r=0;r<gr&&num_objects<MAX_OBJECTS;r++) {
            int c=0;
            while (c<cols) {
                if (used[r*cols+c]||(GCELL(r,c)[0]==' '&&GCELL(r,c)[1]=='\0')){c++;continue;}
                int start=c;
                char text[MAX_TEXT]="";
                int tlen=0;
                while (c<cols&&tlen<MAX_TEXT-2&&!used[r*cols+c]&&
                       !(GCELL(r,c)[0]==' '&&GCELL(r,c)[1]=='\0')) {
                    int clen=(int)strlen(GCELL(r,c));
                    if (tlen+clen>=MAX_TEXT-1) break;
                    memcpy(text+tlen,GCELL(r,c),clen);
                    tlen+=clen;c++;
                }
                text[tlen]='\0';
                if (tlen>0&&num_objects<MAX_OBJECTS) {
                    Object *o=&objects[num_objects++];
                    memset(o,0,sizeof *o);
                    o->type=OBJ_TEXT;o->x=start;o->y=r;
                    o->color=cur_color;o->active=1;
                    strncpy(o->text,text,MAX_TEXT-1);
                }
            }
        }
        free(used); free(grid);
    }

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
    reinit_panel_colors();

    /* load file passed on the command line, if any */
    if (argc > 1)
        load_from_file(argv[1]);

    int rows, cols;

    while (1) {
        getmaxyx(stdscr, rows, cols);
        int cw = cols - PANEL_W;   /* usable canvas width */

        render();
        int ch = getch();

        /* ══ help screen mode ═══════════════════════════════════ */
        if (state == STATE_HELP) {
            state = STATE_NORMAL;
            snprintf(status_msg, sizeof(status_msg), "Ready");
            continue;
        }

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
                if (selected >= 0) { objects[selected].color = cur_color; dirty = 1; }
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
            if (cur_y > 0) cur_y--;
            if (state == STATE_MOVING && selected >= 0)
                objects[selected].y = cur_y - move_off_y;
            break;
        case KEY_DOWN:
            if (cur_y < rows-1) cur_y++;
            if (state == STATE_MOVING && selected >= 0)
                objects[selected].y = cur_y - move_off_y;
            break;
        case KEY_LEFT:
            if (cur_x > 0) cur_x--;
            if (state == STATE_MOVING && selected >= 0)
                objects[selected].x = cur_x - move_off_x;
            break;
        case KEY_RIGHT:
            if (cur_x < cw-1) cur_x++;
            if (state == STATE_MOVING && selected >= 0)
                objects[selected].x = cur_x - move_off_x;
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

        /* cycle panel background colour */
        case 'p':
            panel_bg = (panel_bg + 1) % NUM_PANEL_BGS;
            reinit_panel_colors();
            snprintf(status_msg, sizeof(status_msg),
                     "Panel: %s", panel_bg_names[panel_bg]);
            break;

        /* help screen */
        case '?':
            state = STATE_HELP;
            break;

        /* draw box */
        case 'a':
        case 'b':
        case 'B': {
            int want_style = (ch == 'B') ? BSTYLE_DOUBLE : (ch == 'a') ? BSTYLE_ASCII : BSTYLE_SINGLE;
            if (state == STATE_NORMAL) {
                cur_bstyle = want_style;
                box_sx = cur_x; box_sy = cur_y;
                state  = STATE_DRAWING_BOX;
                snprintf(status_msg, sizeof(status_msg),
                         "Corner set, press %c", (char)ch);
            } else if (state == STATE_DRAWING_BOX) {
                int x1 = box_sx < cur_x ? box_sx : cur_x;
                int y1 = box_sy < cur_y ? box_sy : cur_y;
                int w  = (box_sx > cur_x ? box_sx : cur_x) - x1 + 1;
                int h  = (box_sy > cur_y ? box_sy : cur_y) - y1 + 1;
                if (w >= 2 && h >= 2 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type = OBJ_BOX; o->x=x1; o->y=y1;
                    o->w=w; o->h=h; o->color=cur_color;
                    o->box_style=cur_bstyle; o->active=1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg),
                             "%s box created",
                             cur_bstyle==BSTYLE_DOUBLE ? "Double" : cur_bstyle==BSTYLE_ASCII ? "ASCII" : "Single");
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Box too small");
                }
                state = STATE_NORMAL;
            }
            break;
        }

        /* draw horizontal line (h=single, H=double) */
        case 'h':
        case 'H':
        case '\\': {  /* backslash = ASCII horizontal line */
            int lstyle = (ch == 'H') ? BSTYLE_DOUBLE
                       : (ch == '\\') ? BSTYLE_ASCII
                       : BSTYLE_SINGLE;
            if (state == STATE_NORMAL) {
                line_sx = cur_x; line_sy = cur_y;
                line_dir_cur = 0; line_dbl_cur = lstyle;
                cur_bstyle = lstyle;
                state = STATE_DRAWING_LINE;
                snprintf(status_msg, sizeof(status_msg),
                         "%s horiz line: move then press %c",
                         lstyle==BSTYLE_DOUBLE?"Double":lstyle==BSTYLE_ASCII?"ASCII":"Single", (char)ch);
            } else if (state == STATE_DRAWING_LINE && line_dir_cur == 0) {
                int x1  = line_sx < cur_x ? line_sx : cur_x;
                int len = abs(cur_x - line_sx) + 1;
                if (len >= 1 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type      = OBJ_LINE;
                    o->x         = x1; o->y = line_sy;
                    o->w         = len;
                    o->line_dir  = 0;
                    o->box_style = lstyle;
                    o->color     = cur_color; o->active = 1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg),
                             "%s horiz line (%d chars)",
                             lstyle==BSTYLE_DOUBLE?"Double":lstyle==BSTYLE_ASCII?"ASCII":"Single", len);
                }
                state = STATE_NORMAL;
            }
            break;
        }

        /* draw vertical line (v=single, V=double) */
        case 'v':
        case 'V':
        case '|': {  /* pipe = ASCII vertical line */
            int lstyle = (ch == 'V') ? BSTYLE_DOUBLE
                       : (ch == '|') ? BSTYLE_ASCII
                       : BSTYLE_SINGLE;
            if (state == STATE_NORMAL) {
                line_sx = cur_x; line_sy = cur_y;
                line_dir_cur = 1; line_dbl_cur = lstyle;
                cur_bstyle = lstyle;
                state = STATE_DRAWING_LINE;
                snprintf(status_msg, sizeof(status_msg),
                         "%s vert line: move then press %c",
                         lstyle==BSTYLE_DOUBLE?"Double":lstyle==BSTYLE_ASCII?"ASCII":"Single", (char)ch);
            } else if (state == STATE_DRAWING_LINE && line_dir_cur == 1) {
                int y1  = line_sy < cur_y ? line_sy : cur_y;
                int len = abs(cur_y - line_sy) + 1;
                if (len >= 1 && num_objects < MAX_OBJECTS) {
                    Object *o = &objects[num_objects++];
                    memset(o, 0, sizeof *o);
                    o->type      = OBJ_LINE;
                    o->x         = line_sx; o->y = y1;
                    o->w         = len;
                    o->line_dir  = 1;
                    o->box_style = lstyle;
                    o->color     = cur_color; o->active = 1;
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg),
                             "%s vert line (%d chars)",
                             lstyle==BSTYLE_DOUBLE?"Double":lstyle==BSTYLE_ASCII?"ASCII":"Single", len);
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
                    o->type=OBJ_TEXT; o->x=cur_x; o->y=cur_y;
                    o->color=cur_color; o->active=1;
                    strncpy(o->text, text, MAX_TEXT-1);
                    dirty = 1;
                    snprintf(status_msg, sizeof(status_msg), "Text added");
                }
            }
            break;

        /* select / move */
        case 's':
            if (state == STATE_MOVING) {
                state = STATE_NORMAL; selected = -1;
                dirty = 1;
                snprintf(status_msg, sizeof(status_msg), "Placed");
            } else if (state == STATE_NORMAL) {
                int idx = find_object_at(cur_x, cur_y);
                if (idx >= 0) {
                    selected   = idx;
                    move_off_x = cur_x - objects[idx].x;
                    move_off_y = cur_y - objects[idx].y;
                    cur_color  = objects[idx].color;
                    if (objects[idx].type == OBJ_BOX)
                        cur_bstyle = objects[idx].box_style;
                    state = STATE_MOVING;
                    snprintf(status_msg, sizeof(status_msg), "Moving object");
                } else {
                    snprintf(status_msg, sizeof(status_msg), "Nothing here");
                }
            }
            break;

        /* center selected (or under-cursor) object on the canvas */
        case 'C': {
            int idx = (selected >= 0) ? selected : find_object_at(cur_x, cur_y);
            if (idx < 0) {
                snprintf(status_msg, sizeof(status_msg), "Nothing to center");
                break;
            }

            /* ask H / V / B */
            render();
            {
                int rows, cols_total;
                getmaxyx(stdscr, rows, cols_total);
                int cw = cols_total - PANEL_W;
                int pw = 34, ph = 5;
                int px = (cw - pw) / 2;
                int py = (rows - ph) / 2;

                attron(A_DIM | COLOR_PAIR(21));
                for (int r = 1; r <= ph; r++) mvhline(py+r, px+2, ' ', pw);
                attroff(A_DIM | COLOR_PAIR(21));
                attron(COLOR_PAIR(21) | A_BOLD);
                for (int r = 0; r < ph; r++) mvhline(py+r, px, ' ', pw);
                mvaddch(py,      px,      ACS_ULCORNER);
                mvaddch(py,      px+pw-1, ACS_URCORNER);
                mvaddch(py+ph-1, px,      ACS_LLCORNER);
                mvaddch(py+ph-1, px+pw-1, ACS_LRCORNER);
                for (int i=1;i<pw-1;i++){mvaddch(py,px+i,ACS_HLINE);mvaddch(py+ph-1,px+i,ACS_HLINE);}
                for (int i=1;i<ph-1;i++){mvaddch(py+i,px,ACS_VLINE);mvaddch(py+i,px+pw-1,ACS_VLINE);}
                mvprintw(py, px+(pw-8)/2, " Center ");
                attroff(COLOR_PAIR(21) | A_BOLD);
                attron(COLOR_PAIR(21));
                mvprintw(py+1, px+2, "%-*s", pw-3, "Center on which axis?");
                mvprintw(py+2, px+2, "%-*s", pw-3, "H=Horizontal  V=Vertical  B=Both");
                mvprintw(py+3, px+2, "%-*s", pw-3, "Any other key cancels");
                attroff(COLOR_PAIR(21));
                refresh();
            }

            int axis = getch();
            if (axis != 'h' && axis != 'H' &&
                axis != 'v' && axis != 'V' &&
                axis != 'b' && axis != 'B') {
                snprintf(status_msg, sizeof(status_msg), "Cancelled");
                break;
            }

            {
                Object *o = &objects[idx];
                int cw = canvas_cols();
                int rows, cols_total;
                getmaxyx(stdscr, rows, cols_total);
                (void)cols_total;

                /* compute object's visual width and height */
                int ow, oh;
                if (o->type == OBJ_BOX) {
                    ow = o->w; oh = o->h;
                } else if (o->type == OBJ_LINE) {
                    ow = (o->line_dir == 0) ? o->w : 1;
                    oh = (o->line_dir == 1) ? o->w : 1;
                } else {
                    ow = 0;
                    const char *p = o->text;
                    while (*p) {
                        unsigned char c = (unsigned char)*p;
                        p += (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
                        ow++;
                    }
                    oh = 1;
                }

                int do_h = (axis=='h'||axis=='H'||axis=='b'||axis=='B');
                int do_v = (axis=='v'||axis=='V'||axis=='b'||axis=='B');

                if (do_h) { o->x = (cw - ow) / 2; if (o->x < 0) o->x = 0; }
                if (do_v) { o->y = (rows - oh) / 2; if (o->y < 0) o->y = 0; }

                dirty = 1;
                const char *label = do_h && do_v ? "Both"       :
                                    do_h         ? "Horizontal" : "Vertical";
                snprintf(status_msg, sizeof(status_msg), "Centered: %s", label);
            }
            break;
        }

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
            } else if (state == STATE_DRAWING_BOX || state == STATE_DRAWING_LINE) {
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
            if (idx >= 0) {
                objects[idx].active = 0;
                for (int i = idx; i < num_objects-1; i++)
                    objects[i] = objects[i+1];
                num_objects--;
                if (selected == idx) selected = -1;
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
            if (filename[0]) save_to_file(filename);
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
            if (filename[0]) load_from_file(filename);
            break;
        }

        default: break;
        }
    }

    endwin();
    return 0;
}
