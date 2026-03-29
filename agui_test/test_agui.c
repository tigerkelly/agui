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
    RowCol rc;

    printf("AGUI Version: %s\n", aguiVersion());

    sleep(1);
    
	aguiBegin();
    aguiClearScreen();
    aguiHideCursor(true);

    aguiWindowTitle("AGUI SAMPLE");

	aguiSetAll(COLOR_BLUE, COLOR_DEFAULT, TEXT_BOLD);
    aguiMvText(2, 20, "Hello World !!!");
	aguiSetAll(COLOR_DEFAULT, COLOR_DEFAULT, TEXT_BOLD);
    aguiBox(3, 20, 20, 10, SINGLE_BORDER);
    aguiHorizLine(5, 20, 21, true);
    aguiVertLine(5, 25, 8, true);
    aguiVertLine(5, 32, 8, true);
    aguiBox(3, 50, 20, 10, DOUBLE_BORDER);
    aguiHorizLine(5, 50, 21, DOUBLE_BORDER);
    aguiVertLine(5, 55, 8, SINGLE_BORDER);
    aguiVertLine(5, 62, 8, SINGLE_BORDER);

        aguiBoxTop(14, 20, 20, SINGLE_BORDER);
        aguiBoxBottom(16, 20, 20, SINGLE_BORDER);

        aguiBoxTop(14, 50, 20, DOUBLE_BORDER);
        aguiBoxBottom(16, 50, 20, DOUBLE_BORDER);

		aguiHorizLine(20, 50, 20, SINGLE_BORDER);
		aguiHorizLine(22, 50, 20, SINGLE_BORDER);

		aguiHorizLine(24, 50, 20, SINGLE_BORDER);
		aguiHorizLine(25, 50, 20, SINGLE_BORDER);

		aguiVertLine(4, 5, 10, SINGLE_BORDER);
		aguiVertLine(4, 8, 10, SINGLE_BORDER);

		aguiVertLine(4, 10, 10, SINGLE_BORDER);
		aguiVertLine(4, 12, 10, SINGLE_BORDER);

        aguiBlockBox(25, 20, 10, 8, BLOCK_HALF);
        aguiBlockBox(25, 5, 10, 8, BLOCK_FULL);

        aguiBoxLeft(25, 35, 10, SINGLE_BORDER);
        aguiBoxRight(25, 37, 10, SINGLE_BORDER);

        aguiBoxLeft(25, 40, 10, SINGLE_BORDER);
        aguiBoxRight(25, 42, 10, SINGLE_BORDER);

    aguiBox(25, 50, 20, 10, ASCII_BORDER);

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

    aguiCurrentPosition(&rc);

    getchar();

    aguiClearScreen();
    aguiHideCursor(false);
	aguiEnd();

    printf("Row %d, Col %d\n", rc.row, rc.col);
    
    return 0;
}
