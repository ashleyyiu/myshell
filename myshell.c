// Ashley Yiu
// OS Assignment 1
// 2/28/2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define BUFF_SIZE 2048
#define MAX_HISTORY_CMDS 5
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

int tokenize(char *input);
void changeDirectory(char *newDirectory);
void remove_trailing_newline_char(char *input);
void store_in_history(char* buffer);
void showHistory();
void clearHistory();
void createPipeWrapper(char* input);
void createPipe(char* cmd1, char* cmd2);
void skipBlanks(char* inputStr);

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
int fd[2];
int HISTORY_START = 0;
int HISTORY_INDEX = 0; // points to next avail space
char* HISTORY_CMDS[MAX_HISTORY_CMDS][MAX_HISTORY_CMDS];

int main( int argc, char *argv[] )  {
	char buffer[BUFF_SIZE];
	printf("$");
    while (fgets (buffer, BUFF_SIZE, stdin))
    {
    	if (strcmp(buffer, "exit")==0) {
    		exit(0);
    	}
		remove_trailing_newline_char(buffer);
		skipBlanks(buffer);
		printf("your input: %s\n", buffer);
		printf("HISTORY_INDEX %d\n", HISTORY_INDEX);
		int ret = tokenize(buffer);
		if (ret) {
			printf("$");
			continue; //bad input: don't write to history
		}
		store_in_history(buffer);
		printf("Now we are about to process line #%d\n", HISTORY_INDEX);
		printf("$");
    }

    return 0;
}

// removes trailing \n inline
void remove_trailing_newline_char(char* input)
{
	size_t length = strlen(input);
	if(input[length-1] == '\n') {
        input[length-1] ='\0';
	}
}

void store_in_history(char* buffer) 
{
	char *bufferCopy= {strdup(buffer)};
	// printf("This cmd has length %lu\n", strlen(buffer));
	// printf("HISTORY_INDEX: %d\n", HISTORY_INDEX);
	// for (int i=0; i<MAX_HISTORY_CMDS; i++) {
	// 	printf("%d: %s\n", i, HISTORY_CMDS[i][0]);
	// }
	
	HISTORY_CMDS[HISTORY_INDEX][0] = bufferCopy;
	HISTORY_CMDS[HISTORY_INDEX][strlen(buffer)] = '\0';

	// printf("In row %d, value is: %s\n", HISTORY_INDEX, HISTORY_CMDS[HISTORY_INDEX][0]);

	HISTORY_INDEX = (HISTORY_INDEX+1)%MAX_HISTORY_CMDS;

	if (HISTORY_INDEX == HISTORY_START) {
		HISTORY_START++;
	}
	// printf ("HISTORY_START: %d, HISTORY_INDEX: %d\n", HISTORY_START, HISTORY_INDEX);

	// for (int i=0; i<MAX_HISTORY_CMDS; i++) {
	// 	printf("%d: %s\n", i, HISTORY_CMDS[i][0]);
	// }

}

int tokenize(char *input) {
	regex_t regex;
	char* offset;
	int offsetInt;
	int rv;
	int i = 0;

	printf("Now tokenizing input: %s...\n", input);
	if (strncmp(input, "cd ", 3) == 0) {
		printf("is cd\n");
		changeDirectory(input+3);
	} else if (strcmp(input, "history") == 0) {
		store_in_history(input);
		showHistory();
		return 1;
	} else if (strcmp(input, "history -c") == 0) {
		printf("clear history\n");
		clearHistory();
		return 1;
	} else if (strncmp(input, "history ", 8) == 0) {
		offset = (input+8);
		printf("offset: '%s'\n", offset);
		rv = regcomp(&regex, "[[:digit:]]{1,2}", REG_EXTENDED);
		if (rv != 0) {
			printf("regcomp failed with %d\n", rv);
		} 
		else {
			rv = regexec(&regex, offset, 0, NULL, 0);
			printf("rv: %d\n", rv);
			if (!rv) {
				offsetInt = atoi(offset);
				printf("history offset match: %d\n", offsetInt);
				if (HISTORY_START == HISTORY_INDEX+1) {// just reached max capacity, had to bump start over to make room
					printf("about to run the cmd at index %d\n", (offsetInt+HISTORY_START-1)%MAX_HISTORY_CMDS);
					tokenize(HISTORY_CMDS[((offsetInt+HISTORY_START-1)%MAX_HISTORY_CMDS)][0]);
				} 
				else {
					printf("about to run the cmd at index %d\n", (offsetInt+HISTORY_START)%MAX_HISTORY_CMDS);
					tokenize(HISTORY_CMDS[( offsetInt+HISTORY_START)%MAX_HISTORY_CMDS][0]);
				}
			} 
			else {
				printf("no offset match\n");
			}
		}
	}
	//not cd or history: run command
	else { // run built-in executables
		// createPipeWrapper(input);
		int argNum=0;
		char* shellArgs[BUFF_SIZE];
		char command[256];

		shellArgs[argNum] = strtok(input," "); //first arg
		while(shellArgs[argNum] != NULL) //additional args
		{
			shellArgs[++argNum] = strtok(NULL," ");
		}
		shellArgs[argNum] = NULL;
		printf("Num of args: %i\n", argNum);
		printf("parsed arg:\n");
		for (int index=0;index<argNum; index++)
		{
			printf("%s\n", shellArgs[index]);
		}

		strcpy(command, "/bin/");
		strcat(command, shellArgs[0]);
		int rc = fork();
		if (rc < 0) { // fork failed
			printf("Fork failed\n");
			return -1;
		}
		else if (rc == 0) { // child runs command
			printf("Fork success\n");

			execv(command, shellArgs);
			printf("After fork\n");
		}
		else { // parent: wait until child is done
			wait();
		}
	}
	return 0; //success
}
	//you will have to handle (; & < >) characters.


void createPipeWrapper(char* input)
{	printf("Creating createPipeWrapper\n");
	int pipes = 0;
    char *tmp = input;
	char* cmd1;
	char* cmd2;
    int positions[strlen(input)];
    const char *needle = "|";
	while (tmp = strstr(tmp, needle))
	{
		printf("Found | at position %d\n", (int) (tmp-input));
		positions[pipes] = (int) (tmp-input);
		++tmp;
		++pipes;
	}
	printf("We need %d pipes\n", pipes);
	for (int i=0; i<pipes; i++)
	{
		printf("%d \n", positions[i]);
		//send to createpipe

	}

}

void createPipe(char* cmd1, char* cmd2)
{
	printf("in createPipe where cmd1 is %s and cmd2 is %s\n", cmd1, cmd2);

	pipe(fd);
	pid_t pid = fork();
	
	if (fork() == 0) {
		printf("Child proc 1\n");
	    dup2(fd[0], STDIN_FILENO); // fd[0] for reading
	    close(fd[1]);
    	close(fd[0]);
		if (fork() == 0) {
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
    		close(fd[0]);
        	execvp(cmd1[0], cmd1);
		}
	    wait(NULL);
	    execvp(cmd2[0], cmd2);
	}
	close(fd[1]);
    close(fd[0]);
    wait(NULL);
}

void showHistory()
{
	printf("In showHistory function\n");
	for (int i=HISTORY_START; i<=HISTORY_START+MAX_HISTORY_CMDS; i++) {
		if (HISTORY_CMDS[i][0] == NULL)
			break;
		printf("%d: %s\n", i-HISTORY_START, HISTORY_CMDS[i][0]);
	}
}

void clearHistory()
{
	printf("In clearHistory function\n");
	char* NEW_HISTORY_CMDS[MAX_HISTORY_CMDS][MAX_HISTORY_CMDS];
	HISTORY_START = 0;
	HISTORY_INDEX = 0;
	for (int i=0; i<MAX_HISTORY_CMDS; i++) {
		HISTORY_CMDS[i][0] = NULL;
	}
}

void changeDirectory(char *newDirectory)
{
	printf("Changing Directory to: '%s'\n", newDirectory);
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
	   printf("Current Directory is: %s\n", cwd);
	}
	if (chdir(newDirectory) == 0) {
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
		   printf("New Directory is now: %s\n", cwd);
		} else {
		   perror("getcwd() error");
		}
	} else {
		perror("Failed to change directory");
	}

}

void skipBlanks(char* inputStr) {
	while (inputStr[0] == ' ') {
		memmove(inputStr, inputStr+1, strlen(inputStr));
	}
}