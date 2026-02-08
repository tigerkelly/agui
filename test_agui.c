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

        aguiMvText(17, 2, "Currency: ");
        aguiCurrency(C_CENT_SIGN);
        aguiCurrency(C_POUND_SIGN);
        aguiCurrency(C_CURRENCY_SIGN);
        aguiCurrency(C_YEN_SIGN);
        aguiCurrency(C_EURO_SIGN);
        aguiCurrency(C_RUPEE_SIGN);
        aguiCurrency(C_LIRA_SIGN);
        aguiCurrency(C_RUBLE_SIGN);
        aguiCurrency(C_BITCOIN_SIGN);

        aguiMvText(18, 2, "Math:     ");
        aguiMath(M_MINUS_PLUS);
        aguiMath(M_MULTIPLICATION);
        aguiMath(M_DIVISION);
        aguiMath(M_FOR_ALL);
        aguiMath(M_DIFFERENTIAL);
        aguiMath(M_NABLA);
        aguiMath(M_ARY_PRODUCT);
        aguiMath(M_ARY_SUMMATION);
        aguiMath(M_MINUS_SIGN);
        aguiMath(M_MINUS_PLUS);
        aguiMath(M_SQUARE_ROOT);
        aguiMath(M_INFINITY);
        aguiMath(M_RIGHT_ANGLE);
        aguiMath(M_INTERSECTION);
        aguiMath(M_UNION);
        aguiMath(M_INTERGAL);
        aguiMath(M_ALMOST_EQUAL);
        aguiMath(M_NOT_EQUAL);
        aguiMath(M_INDENTICAL);
        aguiMath(M_LESS_EQUAL);
        aguiMath(M_GREATER_EQUAL);
        aguiMath(M_UP_TACK);


        aguiMvText(19, 2, "Shapes:   ");
        aguiShape(B_SQUARE);
        aguiShape(W_SQUARE);
        aguiShape(S_B_SQUARE);
        aguiShape(S_W_SQUARE);
        aguiShape(B_RECTANGLE);
        aguiShape(B_UP_TRIANGLE);
        aguiShape(W_UP_TRIANGLE);
        aguiShape(B_RIGHT_TRIANGLE);
        aguiShape(W_DOWN_TRIANGLE);
        aguiShape(B_LEFT_TRIANGLE);
        aguiShape(W_LEFT_TRIANGLE);
        aguiShape(B_DIAMOND);
        aguiShape(W_DIAMOND);
        aguiShape(LOZENGE);
        aguiShape(W_CIRCLE);
        aguiShape(D_CIRCLE);
        aguiShape(BULLSEYE);
        aguiShape(B_CIRCLE);
        aguiShape(INVERSE_BULLET);
        aguiShape(INVERSE_W_CIRCLE);
        aguiShape(W_BULLET);

        aguiMvText(20, 2, "Music:    ");
        aguiMusic(EIGHT_NOTE);
        aguiMusic(BEAMED_EIGHT);
        aguiMusic(FLAT_SIGN);
        aguiMusic(SHARP_SIGN);

        aguiMvText(21, 2, "Arrows:   ");
        aguiArrow(LEFT_ARROW);
        aguiArrow(UP_ARROW);
        aguiArrow(RIGHT_ARROW);
        aguiArrow(LEFT_RIGHT_ARROW);
        aguiArrow(UP_DOWN_ARROW);

        aguiMvText(22, 2, "Cards:    ");
        aguiCard(SPADE);
        aguiCard(CLUB);
        aguiCard(HEART);
        aguiCard(DIAMOND);


    getchar();

    aguiClearScreen();
    aguiUnhideCursor();

    return 0;
}
