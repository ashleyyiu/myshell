# myshell
COSC 255 Assignment 1 - creating our own shell

To compile:
	>make
If that doesn't work:
	>gcc myshell.c -o myshell

To run:
	>./myshell


Built-in commands:
	cd [direc]
	history [-c] [offset]
	exit

Runnable external commands:
	Currently, you can run any command in /bin/, /usr/bin/, or ./
	The paths in which you can run commands are in the PATHS and NUM_PATHS global variables, which can be adjusted before compiling.

