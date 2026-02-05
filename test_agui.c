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

    aguiMvText(10, 20, "Hello World !!!");

    getchar();

    aguiClearScreen();
    aguiiUnhideCursor();

    return 0;
}
