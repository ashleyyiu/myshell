// Matthew Chiang
// Assignment 1: Pseudo-shell

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

const int MAX_INPUT_SIZE = 2048;

char* skipBlanks(char* inputStr) {
	while (inputStr[0] == ' ') {
		memmove(inputStr, inputStr+1, strlen(inputStr));
	}
}

uint lineCount(FILE *fp) {
	long int fpPosition = ftell(fp);
	rewind(fp);
	uint count = 0;
	char ch = 0;
	while(!feof(fp)) {
		ch = fgetc(fp);
		if (ch == '\n')
			count++;
	}
	return count;
	fseek(fp, fpPosition, SEEK_SET);
}


int main(int argc, char* argv[]) {

	// set up variables
	char inputStr[MAX_INPUT_SIZE];
	char currentDirec[MAX_INPUT_SIZE];
	char homeDirec[MAX_INPUT_SIZE];

	getcwd(currentDirec, MAX_INPUT_SIZE); // set currentDirec to current directory
	getcwd(homeDirec, MAX_INPUT_SIZE);
	printf("%s$ ", currentDirec);

	FILE *history_fp;
	history_fp = fopen(".myshell_history", "a+");
	if (!history_fp) {
		printf("Couldn't open file\n");
		exit(2);
	}

	while (fgets(inputStr, MAX_INPUT_SIZE, stdin)) {
		// if (inputStr[0] == '\033') {
		// 	printf("arrow key\n");
		// 	continue;
		// }

		skipBlanks(inputStr); // remove extra spaces

		// input is empty line: go to next line
		if (!strcmp(inputStr, "\n") || !strcmp(inputStr, "\r\n")) {
			printf("%s$ ", currentDirec);
			continue;
		}

		//record in history
		fwrite(inputStr, 1, strlen(inputStr), history_fp);

		inputStr[strlen(inputStr)-1] = 0; //remove \n

		
		//printf("%i\n", sizeof(inputStr));


		if (!strcmp(inputStr, "exit")) {
			break;
		}

		// split up commands by space to get arguments
		char origInput[MAX_INPUT_SIZE];
		strcpy(origInput, inputStr);
		char* command = strtok(inputStr, " ");
		printf("First argument: %s\n", command);
		printf("Entire input: %s\n", origInput);


		if (!strcmp(command, "cd")) {
			char* destination = strtok(NULL, " ");
			if (destination == NULL) { // no arg: reset to home
				if (!chdir(homeDirec)) {
					getcwd(currentDirec, MAX_INPUT_SIZE);
				}
				else {
					printf("Error in cd without args\n");
				}
			}
			else { // arg exists: arg = destination
				if (!chdir(destination)) { 
					getcwd(currentDirec, MAX_INPUT_SIZE);
				}
				else {
					printf("Invalid directory\n");
				}
			}
		}

		else if (!strcmp(command, "history")) {
			char* args = strtok(NULL, " ");
			if (args == NULL) {
				uint numLines = lineCount(history_fp);
				numLines = ((numLines > 100) ? (numLines - 100) : 0);
				int i;

				rewind(history_fp);
				//skip lines
				for (i = 0; i < numLines; i++ ) {
					fgets(inputStr, MAX_INPUT_SIZE, history_fp);	
				}
				//read lines
				while (fgets(inputStr, MAX_INPUT_SIZE, history_fp)) { //print line numbers*********************************************************************
					printf("%s", inputStr);
				}
			}
		}

		printf("%s$ ", currentDirec);

	} //end while


	// // edit command line args
	// int timesToRun = atoi(argv[1]); //change #processes to int
	// if (timesToRun < 1) {
	// 	printf("error: invalid number of runs\n");
	// 	exit(2);
	// }
	// char strToRun[256]; //change program to run to command format
	// strcpy(strToRun, "./");
	// strcat(strToRun, argv[2]); //strToRun now = "./argv[2]"

	// // run command
	// unsigned int i;
	// for (i = 0; i < timesToRun; i++) {
	// 	int pid = fork();
	// 	if (pid < 0) {
	// 		printf("error creating fork\n");
	// 		exit(3);
	// 	}
	// 	if (pid == 0) {
	// 		printf("running iteration: %u with PID %u\n", i, getpid());
	// 		int ret;
	// 		char *cmd[] = {strToRun, argv[3], (char*)0};
	// 		ret = execvp(cmd[0],cmd);

	// 		//if we're here, exec didn't work correctly
	// 		printf("error with exec");
	// 		exit(4);
	// 	}
	// }

	fclose(history_fp);
	return 0;
}
