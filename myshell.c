// Matthew Chiang and Ashley Yiu
// OS Assignment 1
// 2/28/2018

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/wait.h>

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
int runCommand(char*, int);

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
char* HOME_DIREC;
int fd[2];
int HISTORY_START = 0;
int HISTORY_INDEX = 0; // points to next avail space
char* HISTORY_CMDS[MAX_HISTORY_CMDS][MAX_HISTORY_CMDS];

int main( int argc, char *argv[] )  {
	char buffer[BUFF_SIZE];
	// const char homeDirec[BUFF_SIZE];
	// getcwd(homeDirec, BUFF_SIZE);
	// HOME_DIREC = &homeDirec;
	getcwd(HOME_DIREC, BUFF_SIZE);

	printf("$");
	while (fgets (buffer, BUFF_SIZE, stdin))
	{
		if (strcmp(buffer, "exit\n")==0) {
			exit(0);
		}
		if (!strcmp(buffer, "\n") || !strcmp(buffer, "\r\n")) { //empty line
			printf("$");
			continue;
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
	char* semicolon = NULL;
	char* ampersandLoc = NULL;
	int offsetInt;
	int rv;
	int i = 0;
	int retVal;
	char inputCopy[BUFF_SIZE];
	char input_to_tokens[BUFF_SIZE];

	strcpy(inputCopy, input); // dest, source
	strcpy(input_to_tokens, input); // dest, source
	char* command = strtok(input_to_tokens, " "); // first argument of input

	skipBlanks(inputCopy);

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

	// check for ampersand
	ampersandLoc = strchr(inputCopy, '&');
	if (ampersandLoc) {
		// printf("has ampersand: tokenize\n");
		memmove(ampersandLoc, ampersandLoc+1, strlen(inputCopy)); //remove & from input
	}

	// DEBUG: Print the actual command to run
	printf("Now tokenizing input: \"%s\"\n", inputCopy);

	// cd
	if (!strcmp(command, "cd")) {
		printf("is cd\n");
		char* destination = strtok(NULL, " ");

		//no destination: go home
		if (destination == NULL) {
			if (HOME_DIREC == NULL) {
				printf("no dest or home direc\n");
			}
			changeDirectory(HOME_DIREC);
		}
		// destination exists
		else { 
			changeDirectory(destination);
		}
		return 1; //write in history
	} //end cd
	// history
	else if (strcmp(inputCopy, "history") == 0) {
		store_in_history(inputCopy);
		showHistory();
		return 1;
	}
	else if (strcmp(inputCopy, "history -c") == 0) {
		printf("clear history\n");
		clearHistory();
		return 1;
	} 
	else if (strncmp(inputCopy, "history ", 8) == 0) {
		offset = (inputCopy+8);
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
	} //end history

	//not cd or history: run command
	else {
		retVal = runCommand(inputCopy, (ampersandLoc == NULL) ? 0 : 1);
		return retVal;
	}

	return -1;
}
	//you will have to handle (; & < >) characters.


void createPipeWrapper(char* input)
{	printf("Creating createPipeWrapper\n");
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
	createPipe(allCommands[0][0], allCommands[1][0]);

}

void createPipe(char* cmd1, char* cmd2)
{
	printf("in createPipe where cmd1 is %s and cmd2 is %s\n", cmd1, cmd2);

	char cmdPath1[256];
	char cmdPath2[256];
	strcpy(cmdPath1, "/bin/");
	strcpy(cmdPath2, "/bin/");

	char* tmpCmd1 = {strdup(cmd1)};
	char* tmpCmd2 = {strdup(cmd2)};
	char* tmpCmd;
	
	tmpCmd = strtok(tmpCmd1, " ");
	printf("tmpCmd1: %s\n", tmpCmd);
	strcat(cmdPath1, tmpCmd);

	tmpCmd = strtok(tmpCmd2, " ");
	printf("tmpCmd2: %s\n", tmpCmd);
	strcat(cmdPath2, tmpCmd);

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
		    printf("executing cmd1\n");
        	execv(cmdPath1, cmd1); // cmd1 writes
		}
	    wait(NULL);
	    printf("executing cmd2\n");
	    execv(cmdPath2, cmd2); // cmd2 reads
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
	if (getcwd(cwd, sizeof(cwd))) {
		printf("Current Directory is: %s\n", cwd);
	}
	if (!chdir(newDirectory)) {
		if (getcwd(cwd, sizeof(cwd))) {
			printf("New Directory is now: %s\n", cwd);
		} else {
			perror("getcwd() error");
		}
	} else {
		perror("Failed to change directory");
	}

}

int runCommand(char* input, int hasAmpersand) {
	printf("Here is input: %s\n", input);
	printf("Here is hasAmpersand: %i\n", hasAmpersand);
	// createPipeWrapper(input);
	int argNum=0;
	char* shellArgs[BUFF_SIZE];
	char command[BUFF_SIZE];
	char *inputCopy= {strdup(input)}; //strtok changes input, so copy

	shellArgs[argNum] = strtok(inputCopy," "); //first arg
	while(shellArgs[argNum] != NULL) //additional args
	{
		shellArgs[++argNum] = strtok(NULL," "); //parse any more args
	}
	printf("Num of args: %i\n", argNum);
	printf("Listing all arguments:\n");
	for (int index=0;index<argNum; index++)
	{
		printf("[%i] %s\n", index, shellArgs[index]);
	}

	strcpy(command, "/bin/");
	strcat(command, shellArgs[0]);
	int pid = fork();
	if (pid < 0) { // fork failed
		printf("Fork failed\n");
		return -1;
	}
	else if (pid == 0) { // child runs command
		printf("Fork success\n");

		execv(command, shellArgs);
		printf("Exec failed\n");
	}
	else { // parent: wait until child is done (unless &)
		if (!hasAmpersand) {
			waitpid(pid, NULL, 0);
			printf("after wait\n");
		}
		else {
			return 0;
			printf("has amp\n");
		}

	}
	return 0;
}

void skipBlanks(char* inputStr) {
	while (inputStr[0] == ' ') {
		memmove(inputStr, inputStr+1, strlen(inputStr));
	}
}
