# agui
A library that uses the ascii escape codes to display information.

This is only for displaying information and it does not allow for user input other than the use of CTL-C to stop the program.
This does not mean you can not add input support yourself.

NOTE: The capabilities of a terminal program range greatly.  This works fine using the putty program version 0.83

To make the agui library just type make.
To compile the test_agui program see the comment at the top of the test_agui.c file.

I wrote this library to help display network statics in a putty terminal.  At some point I may make the Netowrk Monitor System avaiiable.
I like terminals to have white backgrounds and black forground colors, so you may have to play around with the aguiSetColor function if you use different colors.

Contact me if you have questions or issues.
Enjoy
