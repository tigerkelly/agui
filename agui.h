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
    HORIZ_LINE,
    VERT_LINE,
    TOP_LEFT,                           // top left corner
    TOP_RIGHT,                          // top right corner
    BOTTOM_LEFT,                        // bottom left corner
    BOTTOM_RIGHT,                       // bottom right corner
    LEFT_T,
    RIGHT_T,
    TOP_T,
    BOTTOM_T,
    CROSS
};

enum {
        D_HORIZ_LINE,
        D_VERT_LINE,
        D_TOP_LEFT,                     // BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
        D_TOP_LEFT2,            // BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
        D_TOP_LEFT3,            // BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
        D_TOP_RIGHT,            // BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
        D_TOP_RIGHT2,           // BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
        D_TOP_RIGHT3,           // BOX DRAWINGS DOUBLE DOWN AND LEFT
        D_BOTTOM_LEFT,          // BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
        D_BOTTOM_LEFT2,         // BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
        D_BOTTOM_LEFT3,         // BOX DRAWINGS DOUBLE UP AND RIGHT
        D_BOTTOM_RIGHT,         // BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
        D_BOTTOM_RIGHT2,        // BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
        D_BOTTOM_RIGHT3,        // BOX DRAWINGS DOUBLE UP AND LEFT
        D_TEE_LEFT,                     // BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
        D_TEE_LEFT2,            // BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
        D_TEE_LEFT3,            // BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
        D_TEE_RIGHT,            // BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
        D_TEE_RIGHT2,           // BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
        D_TEE_RIGHT3,           // BOX DRAWINGS DOUBLE VERTICAL AND LEFT
        D_T_TOP,                        // BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
        D_T_TOP2,                       // BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
        D_T_TOP3,                       // BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
        D_T_BOTTOM,                     // BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
        D_T_BOTTOM2,            // BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
        D_T_BOTTOM3,            // BOX DRAWINGS DOUBLE UP AND HORIZONTAL
        D_CROSS,                        // BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
        D_CROSS2,                       // BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
        D_CROSS3                        // BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
};

enum {
        BLK_UPPER_HALF,         // UPPER HALF BLOCK
        BLK_LOWER_HALF,         // LOWER HALF BLOCK
        BLK_FULL,                       // FULL BLOCK
        BLK_LEFT_HALF,          // LEFT HALF BLOCK
        BLK_RIGHT_HALF,         // RIGHT HALF BLOCK
        BLK_LIGHT_SHADE,        // LIGHT SHADE
        BLK_MEDIUM_SHADE,       // MEDIUM SHADE
        BLK_DARK_SHADE          // DARK SHADE
};

enum {
        BELL_CHAR,
        BS_CHAR,
        TAB_CHAR,
        LN_CHAR,
        VT_CHAR,
        FF_CHAR,
        CR_CHAR,
        ESC_CHAR
};

// geometric_shapes
enum {
        B_SQUARE,                       // BLACK SQUARE
        W_SQUARE,                       // WHITE SQUARE
        S_B_SQUARE,                     // BLACK SMALL SQUARE
        S_W_SQUARE,                     // WHITE SMALL SQUARE
        B_RECTANGLE,            // BLACK RECTANGLE
        B_UP_TRIANGLE,          // BLACK UP-POINTING TRIANGLE
        W_UP_TRIANGLE,          // WHITE UP-POINTING TRIANGLE
        B_RIGHT_TRIANGLE,       // BLACK RIGHT-POINTING POINTER
        W_DOWN_TRIANGLE,        // BLACK DOWN-POINTING TRIANGLE
        B_LEFT_TRIANGLE,        // WHITE DOWN-POINTING TRIANGLE
        W_LEFT_TRIANGLE,        // BLACK LEFT-POINTING POINTER
        B_DIAMOND,                      // BLACK DIAMOND
        W_DIAMOND,                      // WHITE DIAMOND
        LOZENGE,                        // LOZENGE
        W_CIRCLE,                       // WHITE CIRCLE
        D_CIRCLE,                       // DOTTED CIRCLE
        BULLSEYE,                       // BULLSEYE
        B_CIRCLE,                       // BLACK CIRCLE
        INVERSE_BULLET,         // INVERSE BULLET
        INVERSE_W_CIRCLE,       // INVERSE WHITE CIRCLE
        W_BULLET                        // WHITE BULLET
};

enum {
        LEFT_ARROW,                     // LEFTWARDS ARROW
        UP_ARROW,                       // UPWARDS ARROW
        RIGHT_ARROW,            // RIGHTWARDS ARROW
        DOWN_ARROW,                     // DOWNWARDS ARROW
        LEFT_RIGHT_ARROW,       // LEFT RIGHT ARROW
        UP_DOWN_ARROW           // UP DOWN ARROW
};

enum {
        SPADE,          // BLACK SPADE SUIT
        CLUB,           // BLACK CLUB SUIT
        HEART,          // BLACK HEART SUIT
        DIAMOND         // BLACK DIAMOND SUIT
};

enum {
        EIGHT_NOTE,             // EIGHT NOTE
        BEAMED_EIGHT,   // BEAMED EIGHTH NOTES
        FLAT_SIGN,              // MUSIC FLAT SIGN
        SHARP_SIGN              // MUSIC SHARP SIGN
};

enum {
        M_PLUS_MINUS,           // PLUS-MINUS SIGN
        M_MULTIPLICATION,       // MULTIPLICATION SIGN
        M_DIVISION,                     // DIVISION SIGN
        M_FOR_ALL,                      // FOR ALL
        M_DIFFERENTIAL,         // PARTIAL DIFFERENTIAL
        M_NABLA,                        // NABLA
        M_ARY_PRODUCT,          // N-ARY PRODUCT
        M_ARY_SUMMATION,        // N-ARY SUMMATION
        M_MINUS_SIGN,           // MINUS SIGN
        M_MINUS_PLUS,           // MINUS-OR-PLUS SIGN
        M_SQUARE_ROOT,          // SQUARE ROOT
        M_INFINITY,                     // INFINITY
        M_RIGHT_ANGLE,          // RIGHT ANGLE
        M_INTERSECTION,         // INTERSECTION
        M_UNION,                        // UNION
        M_INTERGAL,                     // INTERGAL
        M_ALMOST_EQUAL,         // ALMOST EQUAL TO
        M_NOT_EQUAL,            // NOT EQUAL TO
        M_INDENTICAL,           // IDENTICAL TO
        M_LESS_EQUAL,           // LESS-THAN OR EQUAL TO
        M_GREATER_EQUAL,        // GREATER-THAN OR EQUAL TO
        M_UP_TACK                       // UP TACK
};

enum {
        C_CENT_SIGN,            // CENT SIGN
        C_POUND_SIGN,           // POUND SIGN
        C_CURRENCY_SIGN,        // CURRENTCY SIGN
        C_YEN_SIGN,                     // YEN SIOGN
        C_EURO_SIGN,            // EURO SIGN
        C_RUPEE_SIGN,           // INDIAN RUPEE SIGN
        C_LIRA_SIGN,            // TURKISH LIRA SIGN
        C_RUBLE_SIGN,           // RUBLE SIGN
        C_BITCOIN_SIGN          // BITCOIN SIGN
};

// Box Drawing Characters - Single Line
typedef struct {
    wchar_t code;
    const char *symbol;
    const char *description;
} UnicodeChar;


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
void aguiBlinkCursor();
void aguiUnblinkCursor();
void aguiForward(int numCols);
void aguiBackward(int numCols);
void aguiMvColumn(int colNum);
void aguiMvUp(int numLines);
void aguiMvDown(int numLines);
void aguiBox(int row, int col, int width, int height, bool useSingleLine);
void aguiBoxTop(int row, int col, int width, bool useSingleLine);
void aguiBoxBottom(int row, int col, int width, bool useSingleLine);
void aguiBoxLeft(int row, int col, int height, bool useSingleLine);
void aguiBoxRight(int row, int col, int height, bool useSingleLine);
void aguiHorizLine(int row, int col, int width, bool useSingleLine);
void aguiVertLine(int row, int col, int height, bool useSingleLine);
void aguiBlockBox(int row, int col, int width, int height, bool useHalfBlock);
void aguiShape(int shape);
void aguiMvShape(int row, int col, int shape);
void aguiArrow(int shape);
void aguiMvArrow(int row, int col, int shape);
void aguiCard(int card);
void aguiMvCard(int row, int col, int card);
void aguiMusic(int note);
void aguiMvMusic(int row, int col, int note);
void aguiMath(int symbol);
void aguiMvMath(int row, int col, int symbol);
void aguiCurrency(int sign);
void aguiMvCurrency(int row, int col, int sign);

#endif
