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
        aguiMvCurrency(2, 36, C_POUND_SIGN);
        aguiMvMath(2, 37, M_SQUARE_ROOT);
        aguiMvMusic(2, 38, BEAMED_EIGHT);
        aguiMvArrow(2, 39, UP_ARROW);
        aguiMvShape(2, 40, B_UP_TRIANGLE);
        aguiMvCard(2, 41, CLUB);

        aguiBox(3, 20, 20, 10, true);
        aguiBox(3, 50, 20, 10, false);

        aguiBoxTop(14, 20, 20, true);
        aguiBoxBottom(16, 20, 20, true);

        aguiBoxTop(14, 50, 20, false);
        aguiBoxBottom(16, 50, 20, false);

        aguiHorizLine(23, 50, 20, true);
        aguiHorizLine(24, 50, 20, false);

        aguiVertLine(4, 5, 10, true);
        aguiVertLine(4, 8, 10, false);

        aguiBlockBox(25, 20, 10, 8, true);
        aguiBlockBox(25, 5, 10, 8, false);

        aguiBoxLeft(25, 35, 10, true);
        aguiBoxRight(25, 37, 10, true);

        aguiBoxLeft(25, 40, 10, false);
        aguiBoxRight(25, 42, 10, false);


    getchar();

    aguiClearScreen();
    aguiUnhideCursor();

    return 0;
}
