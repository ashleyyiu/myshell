// Matthew Chiang and Ashley Yiu
// OS Assignment 1: myshell
// 2/28/2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 2048
#define MAX_HISTORY_CMDS 5
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

int tokenize(char *input);
int changeDirectory(char *newDirectory);
void remove_trailing_newline_char(char *input);
void skipBlanks(char*);
int convert_to_int(char*);
void store_in_history(char* buffer);
void showHistory();
void clearHistory();
int createPipeWrapper(char* input);
void createPipe(char* cmd1, char* cmd2);
int runCommand(char*, int);
char* getCommandPath(char* input);
char* getCommandArgs(char* input);

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
const char* PATHS[3] = {"/bin/","/usr/bin/","./"};
const int NUM_PATHS = 3;
char* HOME_DIREC = NULL;
int fd[2];
int HISTORY_START = 0;
int HISTORY_INDEX = 0; // points to next avail space
char* HISTORY_CMDS[MAX_HISTORY_CMDS][MAX_HISTORY_CMDS];

int main( int argc, char *argv[] )  {
	char input_buffer[MAX_INPUT_SIZE];
	// const char homeDirec[MAX_INPUT_SIZE];
	// getcwd(homeDirec, MAX_INPUT_SIZE);
	// HOME_DIREC = &homeDirec;
	getcwd(HOME_DIREC, MAX_INPUT_SIZE);

	printf("$");
	while (fgets (input_buffer, MAX_INPUT_SIZE, stdin))
	{
		//check + ignore empty lines
		if (!strcmp(input_buffer, "\n") || !strcmp(input_buffer, "\r\n")) {
			printf("$");
			continue;
		}
		remove_trailing_newline_char(input_buffer);
		printf("your input: %s\n", input_buffer);
		printf("HISTORY_INDEX %d\n", HISTORY_INDEX);
		store_in_history(input_buffer);
		int retVal = tokenize(input_buffer);
		if (retVal) {
			printf("Invalid command: nothing run\n");
		}
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
	char* semicolon = NULL;
	char* ampersandLoc = NULL;
	int offsetInt;
	int rv;
	int i = 0;
	int retVal;
	char inputCopy[MAX_INPUT_SIZE];
	char input_to_tokens[MAX_INPUT_SIZE];

	strcpy(inputCopy, input); // dest, source
	strcpy(input_to_tokens, input); // dest, source
	char* command = strtok(input_to_tokens, " "); // first argument of input

	skipBlanks(inputCopy);

	// check for empty input
	if (!strcmp(inputCopy, "")) {
		return 0;
	}

	// check for multiple commands
	semicolon = strchr(inputCopy, ';');
	if (semicolon) {
		char* charIter = inputCopy;
		while (semicolon != NULL) {
			char cur_command[256] = {0};
			for (; charIter < semicolon; charIter++) {
				// printf("letters before semicolon: %c\n", *charIter);
				int cur_len = strlen(cur_command);
				cur_command[cur_len] = *charIter;
				cur_command[cur_len+1] = '\0';
			}
			// printf("current command: %s\n", cur_command);

			// run command
			retVal = tokenize(cur_command);
			semicolon = strchr(semicolon+1, ';'); // if valid, continue w/ next command
			charIter++; //skip ; char
		}
		// printf("at end\n");
		char* endOfInput = strchr(inputCopy, '\0');
		char cur_command[256] = {0};
		for (; charIter < endOfInput; charIter++) {
			// printf("letters before end: %c\n", *charIter);
			int cur_len = strlen(cur_command);
			cur_command[cur_len] = *charIter;
			cur_command[cur_len+1] = '\0';
		}
		// printf("last command: %s\n", cur_command);
		retVal = tokenize(cur_command);
		return retVal;
	}

	// check for &
	ampersandLoc = strchr(inputCopy, '&');
	if (ampersandLoc) {
		// printf("has ampersand: tokenize\n");
		memmove(ampersandLoc, ampersandLoc+1, strlen(inputCopy)); //remove & from input
	}

	// DEBUG: Print the actual command to run
	printf("Now tokenizing input: \"%s\"\n", inputCopy);

	// exit
	if (!strcmp(command, "exit")) {
		exit(0);
	}

	// cd
	else if (!strcmp(command, "cd")) {
		printf("is cd\n");
		char* destination = strtok(NULL, " "); //parse destination

		//no destination: go home
		if (destination == NULL) {
			if (HOME_DIREC == NULL) {
				printf("no dest or home direc\n");
			}
			return changeDirectory(HOME_DIREC);
		}
		// destination exists
		else {
			return changeDirectory(destination);
		}
	} //end cd

	// history
	else if (!strcmp(command, "history")) {
		printf("is history\n");
		char* hist_args = strtok(NULL, " ");

		if (!hist_args) {
			showHistory();
			return 0;
		}
		if (!strcmp(hist_args, "-c")) {
			clearHistory();
			return 0;
		}
		int index = convert_to_int(hist_args);
		if (index != -1) {
			printf("returned val: %i\n", index);//*******************************************************************************
			//now run command in history[index] (+ offset of starting point)

			// else if (strncmp(inputCopy, "history ", 8) == 0) {
			// 	offset = (inputCopy+8);
			// 	printf("offset: '%s'\n", offset);
			// 	rv = regcomp(&regex, "[[:digit:]]{1,2}", REG_EXTENDED);
			// 	if (rv != 0) {
			// 		printf("regcomp failed with %d\n", rv);
			// 	} 
			// 	else {
			// 		rv = regexec(&regex, offset, 0, NULL, 0);
			// 		printf("rv: %d\n", rv);
			// 		if (!rv) {
			// 			offsetInt = atoi(offset);
			// 			printf("history offset match: %d\n", offsetInt);
			// 			if (HISTORY_START == HISTORY_INDEX+1) {// just reached max capacity, had to bump start over to make room
			// 				printf("about to run the cmd at index %d\n", (offsetInt+HISTORY_START-1)%MAX_HISTORY_CMDS);
			// 				tokenize(HISTORY_CMDS[((offsetInt+HISTORY_START-1)%MAX_HISTORY_CMDS)][0]);
			// 			} 
			// 			else {
			// 				printf("about to run the cmd at index %d\n", (offsetInt+HISTORY_START)%MAX_HISTORY_CMDS);
			// 				tokenize(HISTORY_CMDS[( offsetInt+HISTORY_START)%MAX_HISTORY_CMDS][0]);
			// 			}
			// 		} 
			// 		else {
			// 			printf("no offset match\n");
			// 		}
			// 	}
			// } //end history
			return 0;
		}
		printf("Error in history arguments\n");
	}

	//not cd or history: run command
	else {
		return runCommand(inputCopy, (ampersandLoc == NULL) ? 0 : 1);
	}

	return -1;
} //end tokenize


int createPipeWrapper(char* input)
{	
	printf("Creating createPipeWrapper\n");
	int pipes = 0;
    char* tmpInput = input;
    char* tmp;
    char* inputCopy = {strdup(input)};
    int i = 0;
    int positions[strlen(input)];
    const char *needle = "|";
	while (tmpInput = strstr(tmpInput, needle))
	{
		printf("Found | at position %d\n", (int) (tmpInput-input));
		positions[pipes] = (int) (tmpInput-input);
		++tmpInput;
		++pipes;
	}
	printf("We need %d pipes\n", pipes);
    
    char* allCommands[pipes][256];

    //split up the commands
	tmp = strtok (inputCopy, "|");

	while (tmp != NULL)
    {
        allCommands[i++][0] = tmp;
        tmp = strtok (NULL, "|");
    }

	//assuming we have 2 only
	for (int i=0; i<pipes+1; i++)
	{
		printf("allCommands at %d: %s\n", i, allCommands[i][0]);
	}
	if (pipes)
		createPipe(allCommands[0][0], allCommands[1][0]);
	return pipes;
}

char* getCommandPath(char* input)
{
	char command[MAX_INPUT_SIZE];
	char *shellArgs[MAX_INPUT_SIZE];
	char *inputCopy= {strdup(input)};
	char *cmdPath[MAX_INPUT_SIZE];
	char *firstCmd;
	int found = 0;

	firstCmd = strtok(inputCopy," "); //first arg
	for (int cur_path = 0; cur_path < NUM_PATHS; cur_path++) {
		strcpy(command, PATHS[cur_path]);
		strcat(command, firstCmd);
		if (!access(command, X_OK)) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		printf("Error: entered command not found\n");
		printf("Accepted paths are:\n");
		for (int cur_path = 0; cur_path < NUM_PATHS; cur_path++) {
			printf("\t%s\n", PATHS[cur_path]);
		}
	} else {
		strcpy(cmdPath, command);
	}
	printf("In getCmdPath, returning cmdpath: %s\n", cmdPath);
	return cmdPath;
}

void createPipe(char* cmd1, char* cmd2)
{
	int argNum = 0;
	int found = 0;
	char *execCmd1[MAX_INPUT_SIZE];
	char *execCmd2[MAX_INPUT_SIZE];
	char *command1Copy= {strdup(cmd1)};
	char *command2Copy= {strdup(cmd2)};
	char *command1CopyArgs= {strdup(cmd1)};
	char *command2CopyArgs= {strdup(cmd2)};
	char *cmdPath1;
	char *cmdPath2;
	printf("in createPipe where cmd1 is %s and cmd2 is %s\n", cmd1, cmd2);
	cmdPath1 = strdup(getCommandPath(command1Copy)); // duplicate so it doesn't get overwritten
	cmdPath2 = strdup(getCommandPath(command2Copy));

	execCmd1[argNum] = strtok(command1CopyArgs," "); //first arg
	while(execCmd1[argNum] != NULL) //additional args
	{
		execCmd1[++argNum] = strtok(NULL," "); //parse any more args
	}

	printf("In createPipe, cmdpath1: %s\n", cmdPath1);
	for (int index=0;index<argNum; index++)
	{
		printf("[%i] %s\n", index, execCmd1[index]);
	}

	argNum = 0;

	execCmd2[argNum] = strtok(command2CopyArgs," "); //first arg
	while(execCmd2[argNum] != NULL) //additional args
	{
		execCmd2[++argNum] = strtok(NULL," "); //parse any more args
	}

	printf("In createPipe, cmdpath2: %s\n", cmdPath2);
	for (int index=0;index<1; index++)
	{
		printf("[%i] %s\n", index, execCmd2[index]);
	}

	pipe(fd);

	int rc = fork();
	if (rc < 0){
		printf("Fork failed\n");
	} else if (rc == 0) {
		printf("Child proc 1\n");
	    dup2(fd[0], STDIN_FILENO); // fd[0] for reading
		if (fork() == 0) {
			printf("Child proc 2\n");
			dup2(fd[1], STDOUT_FILENO); // fd[1] for writing
		    // printf("executing cmd1\n");
        	execv(cmdPath1, execCmd1); // cmd1 writes
		}
	    wait(NULL);
	    printf("executing cmd2\n");
	    execv(cmdPath2, execCmd2); // cmd2 reads
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
	HISTORY_START = 0;
	HISTORY_INDEX = 0;
	for (int i=0; i<MAX_HISTORY_CMDS; i++) {
		HISTORY_CMDS[i][0] = NULL;
	}
}

int changeDirectory(char *newDirectory)
{
	printf("Changing Directory to: '%s'\n", newDirectory);
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd))) {
		printf("Current Directory is: %s\n", cwd);
	}
	if (!chdir(newDirectory)) {
		if (getcwd(cwd, sizeof(cwd))) {
			printf("New Directory is now: %s\n", cwd);
			return 0;
		}
		else {
			perror("getcwd() error");
			return -1;
		}
	} 
	else {
		perror("Failed to change directory");
		return -1;
	}
	return -1;
}

int runCommand(char* input, int hasAmpersand) {
	int pipesNeeded = createPipeWrapper(input);
	if (pipesNeeded == 0) // execute here if no pipe needed
	{
		int argNum = 0;
		int found = 0;
		char* shellArgs[MAX_INPUT_SIZE];
		char command[MAX_INPUT_SIZE];
		char *inputCopy= {strdup(input)}; //strtok changes input, so copy

		shellArgs[argNum] = strtok(inputCopy," "); //first arg
		while(shellArgs[argNum] != NULL) //additional args
		{
			shellArgs[++argNum] = strtok(NULL," "); //parse any more args
		}
		// printf("Num of args: %i\n", argNum);
		// printf("Listing all arguments:\n");
		// for (int index=0;index<argNum; index++)
		// {
		// 	printf("[%i] %s\n", index, shellArgs[index]);
		// }
		for (int cur_path = 0; cur_path < NUM_PATHS; cur_path++) {
			strcpy(command, PATHS[cur_path]);
			strcat(command, shellArgs[0]);
			if (!access(command, X_OK)) {
				found = 1;
				break;
			}
		}
		if (found == 0) {
			printf("Error: entered command not found\n");
			printf("Accepted paths are:\n");
			for (int cur_path = 0; cur_path < NUM_PATHS; cur_path++) {
				printf("\t%s\n", PATHS[cur_path]);
			}
			return -1;
		}
		int pid = fork();
		if (pid < 0) { // fork failed
			printf("Fork failed\n");
			return -1;
		}
		else if (pid == 0) { // child runs command
			// printf("Fork success\n");
			execv(command, shellArgs);
			printf("Exec failed\n");
			return -1;
		}
		else { // parent: wait until child is done (unless &)
			if (!hasAmpersand)
				waitpid(pid, NULL, 0);
			return 0;
		}
	}
	return -1;
}

int convert_to_int(char* inputStr) {
	int retVal = 0;
	int inputStr_len = strlen(inputStr);
	for (int i = 0; i < inputStr_len; i++) {
		if (inputStr[i] < '0' || inputStr[i] > '9')
			return -1;
		else
			retVal = (inputStr[i] - '0') + 10 * retVal;
	}
	return retVal;
}

void skipBlanks(char* inputStr) {
	while (inputStr[0] == ' ') {
		memmove(inputStr, inputStr+1, strlen(inputStr));
	}
}
