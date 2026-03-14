/* To compile:
 * run make and then run the following.
 * gcc test_adraw.c -o test_adraw -L./ -lagui
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "agui.h"

int main(int argc, char *argv[]) {

    printf("AGUI Version: %s\n", aguiVersion());

    sleep(1);
    
    aguiClearScreen();
    aguiHideCursor(true);

	aguiLoadScreen("nmsstat.agui");

    getchar();

    aguiClearScreen();
    aguiHideCursor(false);

    
    return 0;
}
