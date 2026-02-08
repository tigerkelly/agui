/* To compile:
 * run make and then run the following.
 * gcc test_agui.c -o test_agui -L./ -lagui
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "agui.h"

int main(int argc, char *argv[]) {

    aguiClearScreen();
    aguiHideCursor();

    aguiMvText(2, 20, "Hello World !!!");
        aguiBox(3, 20, 20, 10, true);
        aguiBox(3, 50, 20, 10, false);

        aguiBoxTop(14, 20, 20, true);
        aguiBoxBottom(16, 20, 20, true);

        aguiBoxTop(14, 50, 20, false);
        aguiBoxBottom(16, 50, 20, false);

        aguiHorizLine(24, 50, 20, true);
        aguiHorizLine(25, 50, 20, false);

        aguiVertLine(4, 5, 10, true);
        aguiVertLine(4, 8, 10, false);

        aguiBlockBox(25, 20, 10, 8, true);
        aguiBlockBox(25, 5, 10, 8, false);

        aguiBoxLeft(25, 35, 10, true);
        aguiBoxRight(25, 37, 10, true);

        aguiBoxLeft(25, 40, 10, false);
        aguiBoxRight(25, 42, 10, false);

        aguiMvText(17, 2, "Currency:");
        aguiMvCurrency(17, 12, C_CENT_SIGN);
        aguiMvCurrency(17, 13, C_POUND_SIGN);
        aguiMvCurrency(17, 14, C_CURRENCY_SIGN);
        aguiMvCurrency(17, 15, C_YEN_SIGN);
        aguiMvCurrency(17, 16, C_EURO_SIGN);
        aguiMvCurrency(17, 17, C_RUPEE_SIGN);
        aguiMvCurrency(17, 18, C_LIRA_SIGN);
        aguiMvCurrency(17, 19, C_RUBLE_SIGN);
        aguiMvCurrency(17, 20, C_BITCOIN_SIGN);

        aguiMvText(18, 2, "Math:");
        aguiMvMath(18, 12, M_MINUS_PLUS);
        aguiMvMath(18, 13, M_MULTIPLICATION);
        aguiMvMath(18, 14, M_DIVISION);
        aguiMvMath(18, 15, M_FOR_ALL);
        aguiMvMath(18, 16, M_DIFFERENTIAL);
        aguiMvMath(18, 17, M_NABLA);
        aguiMvMath(18, 18, M_ARY_PRODUCT);
        aguiMvMath(18, 19, M_ARY_SUMMATION);
        aguiMvMath(18, 20, M_MINUS_SIGN);
        aguiMvMath(18, 21, M_MINUS_PLUS);
        aguiMvMath(18, 22, M_SQUARE_ROOT);
        aguiMvMath(18, 23, M_INFINITY);
        aguiMvMath(18, 24, M_RIGHT_ANGLE);
        aguiMvMath(18, 25, M_INTERSECTION);
        aguiMvMath(18, 26, M_UNION);
        aguiMvMath(18, 17, M_INTERGAL);
        aguiMvMath(18, 24, M_ALMOST_EQUAL);
        aguiMvMath(18, 25, M_NOT_EQUAL);
        aguiMvMath(18, 26, M_INDENTICAL);
        aguiMvMath(18, 27, M_LESS_EQUAL);
        aguiMvMath(18, 28, M_GREATER_EQUAL);
        aguiMvMath(18, 29, M_UP_TACK);


        aguiMvText(19, 2, "Shapes:");
        aguiMvShape(19, 12, B_SQUARE);
        aguiMvShape(19, 13, W_SQUARE);
        aguiMvShape(19, 14, S_B_SQUARE);
        aguiMvShape(19, 15, S_W_SQUARE);
        aguiMvShape(19, 16, B_RECTANGLE);
        aguiMvShape(19, 17, B_UP_TRIANGLE);
        aguiMvShape(19, 18, W_UP_TRIANGLE);
        aguiMvShape(19, 19, B_RIGHT_TRIANGLE);
        aguiMvShape(19, 20, W_DOWN_TRIANGLE);
        aguiMvShape(19, 21, B_LEFT_TRIANGLE);
        aguiMvShape(19, 22, W_LEFT_TRIANGLE);
        aguiMvShape(19, 23, B_DIAMOND);
        aguiMvShape(19, 24, W_DIAMOND);
        aguiMvShape(19, 25, LOZENGE);
        aguiMvShape(19, 26, W_CIRCLE);
        aguiMvShape(19, 27, D_CIRCLE);
        aguiMvShape(19, 28, BULLSEYE);
        aguiMvShape(19, 29, B_CIRCLE);
        aguiMvShape(19, 30, INVERSE_BULLET);
        aguiMvShape(19, 31, INVERSE_W_CIRCLE);
        aguiMvShape(19, 32, W_BULLET);

        aguiMvText(20, 2, "Music:");
        aguiMvMusic(20, 12, EIGHT_NOTE);
        aguiMvMusic(20, 13, BEAMED_EIGHT);
        aguiMvMusic(20, 14, FLAT_SIGN);
        aguiMvMusic(20, 15, SHARP_SIGN);

        aguiMvText(21, 2, "Arrows:");
        aguiMvArrow(21, 12, LEFT_ARROW);
        aguiMvArrow(21, 13, UP_ARROW);
        aguiMvArrow(21, 14, RIGHT_ARROW);
        aguiMvArrow(21, 15, LEFT_RIGHT_ARROW);
        aguiMvArrow(21, 16, UP_DOWN_ARROW);

        aguiMvText(22, 2, "Cards:");
        aguiMvCard(22, 12, SPADE);
        aguiMvCard(22, 13, CLUB);
        aguiMvCard(22, 14, HEART);
        aguiMvCard(22, 15, DIAMOND);


    getchar();

    aguiClearScreen();
    aguiUnhideCursor();

    return 0;
}
