#include <stdio.h>
#include <wchar.h>
#include <locale.h>

// Single Line Box Drawing
UnicodeChar single_line[] = {
    {0x2500, "─", "BOX DRAWINGS LIGHT HORIZONTAL"},
    {0x2502, "│", "BOX DRAWINGS LIGHT VERTICAL"},
    {0x250C, "┌", "BOX DRAWINGS LIGHT DOWN AND RIGHT"},
    {0x2510, "┐", "BOX DRAWINGS LIGHT DOWN AND LEFT"},
    {0x2514, "└", "BOX DRAWINGS LIGHT UP AND RIGHT"},
    {0x2518, "┘", "BOX DRAWINGS LIGHT UP AND LEFT"},
    {0x251C, "├", "BOX DRAWINGS LIGHT VERTICAL AND RIGHT"},
    {0x2524, "┤", "BOX DRAWINGS LIGHT VERTICAL AND LEFT"},
    {0x252C, "┬", "BOX DRAWINGS LIGHT DOWN AND HORIZONTAL"},
    {0x2534, "┴", "BOX DRAWINGS LIGHT UP AND HORIZONTAL"},
    {0x253C, "┼", "BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL"},
};

// Double Line Box Drawing
UnicodeChar double_line[] = {
    {0x2550, "═", "BOX DRAWINGS DOUBLE HORIZONTAL"},
    {0x2551, "║", "BOX DRAWINGS DOUBLE VERTICAL"},
    {0x2554, "╔", "BOX DRAWINGS DOUBLE DOWN AND RIGHT"},
    {0x2557, "╗", "BOX DRAWINGS DOUBLE DOWN AND LEFT"},
    {0x255A, "╚", "BOX DRAWINGS DOUBLE UP AND RIGHT"},
    {0x255D, "╝", "BOX DRAWINGS DOUBLE UP AND LEFT"},
    {0x2560, "╠", "BOX DRAWINGS DOUBLE VERTICAL AND RIGHT"},
    {0x2563, "╣", "BOX DRAWINGS DOUBLE VERTICAL AND LEFT"},
    {0x2566, "╦", "BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL"},
    {0x2569, "╩", "BOX DRAWINGS DOUBLE UP AND HORIZONTAL"},
    {0x256C, "╬", "BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL"},
};

// Block Elements
const UnicodeChar block_elements[] = {
    {0x2580, "▀", "UPPER HALF BLOCK"},
    {0x2584, "▄", "LOWER HALF BLOCK"},
    {0x2588, "█", "FULL BLOCK"},
    {0x258C, "▌", "LEFT HALF BLOCK"},
    {0x2590, "▐", "RIGHT HALF BLOCK"},
    {0x2591, "░", "LIGHT SHADE"},
    {0x2592, "▒", "MEDIUM SHADE"},
    {0x2593, "▓", "DARK SHADE"},
};

// Geometric Shapes
const UnicodeChar geometric_shapes[] = {
    {0x25A0, "■", "BLACK SQUARE"},
    {0x25A1, "□", "WHITE SQUARE"},
    {0x25AA, "▪", "BLACK SMALL SQUARE"},
    {0x25AB, "▫", "WHITE SMALL SQUARE"},
    {0x25AC, "▬", "BLACK RECTANGLE"},
    {0x25B2, "▲", "BLACK UP-POINTING TRIANGLE"},
    {0x25B3, "△", "WHITE UP-POINTING TRIANGLE"},
    {0x25BA, "►", "BLACK RIGHT-POINTING POINTER"},
    {0x25BC, "▼", "BLACK DOWN-POINTING TRIANGLE"},
    {0x25BD, "▽", "WHITE DOWN-POINTING TRIANGLE"},
    {0x25C4, "◄", "BLACK LEFT-POINTING POINTER"},
    {0x25C6, "◆", "BLACK DIAMOND"},
    {0x25C7, "◇", "WHITE DIAMOND"},
    {0x25CA, "◊", "LOZENGE"},
    {0x25CB, "○", "WHITE CIRCLE"},
    {0x25CC, "◌", "DOTTED CIRCLE"},
    {0x25CE, "◎", "BULLSEYE"},
    {0x25CF, "●", "BLACK CIRCLE"},
    {0x25D8, "◘", "INVERSE BULLET"},
    {0x25D9, "◙", "INVERSE WHITE CIRCLE"},
    {0x25E6, "◦", "WHITE BULLET"},
};

// Arrows
const UnicodeChar arrows[] = {
    {0x2190, "←", "LEFTWARDS ARROW"},
    {0x2191, "↑", "UPWARDS ARROW"},
    {0x2192, "→", "RIGHTWARDS ARROW"},
    {0x2193, "↓", "DOWNWARDS ARROW"},
    {0x2194, "↔", "LEFT RIGHT ARROW"},
    {0x2195, "↕", "UP DOWN ARROW"},
};

// Card Suits
const UnicodeChar card_suits[] = {
    {0x2660, "♠", "BLACK SPADE SUIT"},
    {0x2663, "♣", "BLACK CLUB SUIT"},
    {0x2665, "♥", "BLACK HEART SUIT"},
    {0x2666, "♦", "BLACK DIAMOND SUIT"},
};

// Music Symbols
const UnicodeChar music_symbols[] = {
    {0x266A, "♪", "EIGHTH NOTE"},
    {0x266B, "♫", "BEAMED EIGHTH NOTES"},
    {0x266D, "♭", "MUSIC FLAT SIGN"},
    {0x266F, "♯", "MUSIC SHARP SIGN"},
};

// Mathematical Symbols
const UnicodeChar math_symbols[] = {
    {0x00B1, "±", "PLUS-MINUS SIGN"},
    {0x00D7, "×", "MULTIPLICATION SIGN"},
    {0x00F7, "÷", "DIVISION SIGN"},
    {0x2200, "∀", "FOR ALL"},
    {0x2202, "∂", "PARTIAL DIFFERENTIAL"},
    {0x2207, "∇", "NABLA"},
    {0x220F, "∏", "N-ARY PRODUCT"},
    {0x2211, "∑", "N-ARY SUMMATION"},
    {0x2212, "−", "MINUS SIGN"},
    {0x2213, "∓", "MINUS-OR-PLUS SIGN"},
    {0x221A, "√", "SQUARE ROOT"},
    {0x221E, "∞", "INFINITY"},
    {0x221F, "∟", "RIGHT ANGLE"},
    {0x2229, "∩", "INTERSECTION"},
    {0x222A, "∪", "UNION"},
    {0x222B, "∫", "INTEGRAL"},
    {0x2248, "≈", "ALMOST EQUAL TO"},
    {0x2260, "≠", "NOT EQUAL TO"},
    {0x2261, "≡", "IDENTICAL TO"},
    {0x2264, "≤", "LESS-THAN OR EQUAL TO"},
    {0x2265, "≥", "GREATER-THAN OR EQUAL TO"},
    {0x22A5, "⊥", "UP TACK"},
};

// Currency Symbols
const UnicodeChar currency_symbols[] = {
    {0x00A2, "¢", "CENT SIGN"},
    {0x00A3, "£", "POUND SIGN"},
    {0x00A4, "¤", "CURRENCY SIGN"},
    {0x00A5, "¥", "YEN SIGN"},
    {0x20AC, "€", "EURO SIGN"},
    {0x20B9, "₹", "INDIAN RUPEE SIGN"},
    {0x20BA, "₺", "TURKISH LIRA SIGN"},
    {0x20BD, "₽", "RUBLE SIGN"},
    {0x20BF, "₿", "BITCOIN SIGN"},
};

// Array sizes
const int SINGLE_LINE_SIZE = sizeof(single_line) / sizeof(UnicodeChar);
const int DOUBLE_LINE_SIZE = sizeof(double_line) / sizeof(UnicodeChar);
const int BLOCK_ELEMENTS_SIZE = sizeof(block_elements) / sizeof(UnicodeChar);
const int GEOMETRIC_SHAPES_SIZE = sizeof(geometric_shapes) / sizeof(UnicodeChar);
const int ARROWS_SIZE = sizeof(arrows) / sizeof(UnicodeChar);
const int CARD_SUITS_SIZE = sizeof(card_suits) / sizeof(UnicodeChar);
const int MUSIC_SYMBOLS_SIZE = sizeof(music_symbols) / sizeof(UnicodeChar);
const int MATH_SYMBOLS_SIZE = sizeof(math_symbols) / sizeof(UnicodeChar);
const int CURRENCY_SYMBOLS_SIZE = sizeof(currency_symbols) / sizeof(UnicodeChar);
