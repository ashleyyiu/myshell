myshell
Matthew Chiang and Ashley Yiu
COSC 255 Assignment 1 - creating our own shell


To compile:
	>make

To run:
	>./myshell


Built-in commands:
	cd [direc]
	history [-c] [offset]
	exit


Runnable external commands:
	Currently, you can run any command in /bin/, /usr/bin/, or ./ without supplying the path. For example, you may simply run
		>ls
	Alternatively, you can run any command by entering the full path. For example, you can run:
		>/usr/bin/ls
	The paths in which you can run commands without the full path are stored in the PATHS and NUM_PATHS global variables, which can be adjusted before compiling.


Notes on commands:
	--All commands, including broken commands, are written in history. The exception is for "history offset" commands to prevent infinite recursion. This is modeled after history in Bash. Ill-formed "history offset" commands aren't stored in history. Correctly-formed "history offset" commands store the command ran instead of "history offset". For example, running:
	>ls
	>history 0
	>history

	Will print:
	0 ls
	1 ls		# NOT history 0
	2 history


	--Modeled after the cd in Bash, running "cd" without arguments will take you to the home directory.

