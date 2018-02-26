// Matthew Chiang and Ashley Yiu
// OS Assignment 1
// 2/26/2018

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
 
#define MAX_INPUT_SIZE 2048
#define MAX_HISTORY_CMDS 5
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

int tokenize(char *input);
int changeDirectory(char *newDirectory);
void remove_trailing_newline_char(char *input);
int convert_to_int(char*);
void store_in_history(char* buffer);
void showHistory();
void clearHistory();
int createPipeWrapper(char* input);
void createPipe(char* cmd1, char* cmd2);
int runCommand(char*, int);
char* getCommandPath(char* input);
void trimLeadingWhitespace(char*);
char* trimTrailingWhitespace(char*);

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;
const char* PATHS[] = {"/bin/","/usr/bin/","./"};
const int NUM_PATHS = 3;
char* HOME_DIREC = NULL;
int fd[2];
int HISTORY_INDEX = 0; // points to next avail space
char* HISTORY_CMDS[MAX_HISTORY_CMDS];


int main( int argc, char *argv[] )  {
	char input_buffer[MAX_INPUT_SIZE];
	// const char homeDirec[MAX_INPUT_SIZE];
	// getcwd(homeDirec, MAX_INPUT_SIZE);
	// HOME_DIREC = &homeDirec;
	getcwd(HOME_DIREC, MAX_INPUT_SIZE);
	clearHistory();

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
		int toStore = tokenize(input_buffer);
		if (toStore) {
			printf("$");
			continue;
		}
		store_in_history(input_buffer);
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

int tokenize(char *input) {
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

	trimLeadingWhitespace(inputCopy);

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
		tokenize(cur_command);
		return 0;
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
			changeDirectory(HOME_DIREC);
			return 0;
		}
		// destination exists
		else {
			changeDirectory(destination);
			return 0;
		}
	} //end cd

	// history
	else if (!strcmp(command, "history")) {
		char* hist_args = strtok(NULL, " ");

		if (!hist_args) {
			store_in_history(input);
			showHistory();
			return 1; //already stored before show, so don't store again
		}
		if (!strcmp(hist_args, "-c")) {
			clearHistory();
			return 1; //don't store clear
		}
		int index = convert_to_int(hist_args);
		if (index != -1) {
			//test if index is valid
			int histFull = (HISTORY_CMDS[HISTORY_INDEX] != NULL);
			if ((histFull && index >= MAX_HISTORY_CMDS) || (!histFull && index >= HISTORY_INDEX)) {
				printf("Error: invalid history index\n");
				return 1;
			}
			if (histFull) {
				tokenize(HISTORY_CMDS[(index + HISTORY_INDEX)%MAX_HISTORY_CMDS]);
				store_in_history(HISTORY_CMDS[(HISTORY_INDEX + index)%MAX_HISTORY_CMDS]); // store the cmd you ran to avoid infinite recursion
			}
			else {
				tokenize(HISTORY_CMDS[(index)%MAX_HISTORY_CMDS]);
				store_in_history(HISTORY_CMDS[(index)%MAX_HISTORY_CMDS]); // store the cmd you ran to avoid infinite recursion
			}
			return 1; //just stored cmd ran, so don't store again
		}
		printf("Error in history arguments\n");
	}

	//not cd or history: run command
	else {
		runCommand(inputCopy, (ampersandLoc == NULL) ? 0 : 1);
		return 0;
	}

	return 0;
} //end tokenize()


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

// input: "ls -a"
// output: "/bin/ls"
// make sure you free return value after using
char* getCommandPath(char* input)
{
	char* cmd_and_path = malloc(MAX_INPUT_SIZE);
	char inputCopy[MAX_INPUT_SIZE];
	char* cmd;
	int found = 0;

	strcpy(inputCopy, input); // so you don't change parameter
	cmd = strtok(inputCopy, " "); // cmd = "ls" from "ls -a"
	for (int cur_path = 0; cur_path < NUM_PATHS; cur_path++) {
		strcpy(cmd_and_path, PATHS[cur_path]); //c_a_p = "/bin/"
		strcat(cmd_and_path, cmd); //c_a_p = "/bin/ls"
		if (!access(cmd_and_path, X_OK)) { //test if valid
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
		return 0;
	} 
	// printf("In getCmdPath, returning cmdpath: %s\n", cmd_and_path);
	return cmd_and_path;
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
	// printf("in createPipe where cmd1 is %s and cmd2 is %s\n", cmd1, cmd2);
	cmdPath1 = strdup(getCommandPath(command1Copy)); // duplicate so it doesn't get overwritten
	cmdPath2 = strdup(getCommandPath(command2Copy));

	execCmd1[argNum] = strtok(command1CopyArgs," "); //first arg
	while(execCmd1[argNum] != NULL) //additional args
	{
		execCmd1[++argNum] = strtok(NULL," "); //parse any more args
	}

	// printf("In createPipe, cmdpath1: %s\n", cmdPath1);
	// for (int index=0;index<argNum; index++)
	// {
	// 	printf("[%i] %s\n", index, execCmd1[index]);
	// }

	argNum = 0;

	execCmd2[argNum] = strtok(command2CopyArgs," "); //first arg
	while(execCmd2[argNum] != NULL) //additional args
	{
		execCmd2[++argNum] = strtok(NULL," "); //parse any more args
	}

	// printf("In createPipe, cmdpath2: %s\n", cmdPath2);
	// for (int index=0;index<1; index++)
	// {
	// 	printf("[%i] %s\n", index, execCmd2[index]);
	// }

	int PIPE_READ;
	int PIPE_WRITE;

	int fd[2];
	pipe(fd);

	pid_t rc = fork();
	pid_t rc2 = 0;

  	PIPE_WRITE = fd[1];
  	PIPE_READ = fd[0];

	if (rc < 0){
		printf("Fork failed\n");

	} else if (rc == 0) { // 1st child process will write to pipe
		printf("Child proc, PID:=%d\n", getpid());
		dup2(PIPE_WRITE, STDOUT_FILENO); // replace stdout with pipe input
		close(PIPE_READ);
		execv(cmdPath1, execCmd1);
		perror("exec");
	} else { // parent process will read from pipe
		wait(NULL);
		printf("Parent proc, PID:=%d\n", getpid());

		// 2nd child process will read from pipe
		rc2 = fork();
		if (rc2 < 0) {
			printf("Fork failed\n");
		} else if (rc2 == 0) {
			printf("Parent's second child proc, PID:=%d\n", getpid());
			dup2(PIPE_READ, STDIN_FILENO); // replace stdin with pipe output
			close(PIPE_WRITE);
			execv(cmdPath2, execCmd2);
			perror("exec");
		}
		close(PIPE_READ);
		close(PIPE_WRITE);
		waitpid(rc, NULL, 0);
		waitpid(rc2, NULL, 0);
	}
}

void store_in_history(char* cmd)
{
	if (HISTORY_CMDS[HISTORY_INDEX] != NULL && strcmp(cmd, HISTORY_CMDS[HISTORY_INDEX])) {
		free(HISTORY_CMDS[HISTORY_INDEX]);
	}
	char* toStore = strdup(cmd);
	HISTORY_CMDS[HISTORY_INDEX] = toStore;
	HISTORY_INDEX = (HISTORY_INDEX+1)%MAX_HISTORY_CMDS;
} // end store_in_history()

void showHistory()
{
	// hist not full yet
	if (HISTORY_CMDS[HISTORY_INDEX] == NULL) {
		for (int i=0; i < HISTORY_INDEX; i++) {
			printf("%d: %s\n", i, HISTORY_CMDS[i]);
		}
		return;
	}
	// hist is full
	for (int i=HISTORY_INDEX; i < HISTORY_INDEX+MAX_HISTORY_CMDS; i++) {
		printf("%d: %s\n", i-HISTORY_INDEX, HISTORY_CMDS[i % MAX_HISTORY_CMDS]);
	}
} // end showHistory()

void clearHistory()
{
	HISTORY_INDEX = 0;
	for (int i=0; i < MAX_HISTORY_CMDS; i++) {
		free(HISTORY_CMDS[i]);
		HISTORY_CMDS[i] = NULL;
	}
} // end clearHistory()

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

int redirect(char* input) // return 1 if need to redirect, else 0
{
	printf("In redirect\n");
	// find > or <
	char* inputCopy = strdup(input);
	char* fileName;
	char* cmd;
	char* cmdPath;
	char* shellArgs[MAX_INPUT_SIZE];
	int argNum = 0;
	int redirectInput = -1; //0 for redirecting output, 1 for redirecting input
	int fd;
	pid_t rc;

	if (strstr(inputCopy, ">")) {// ls > tmp.txt
		cmd = strtok(inputCopy, ">");
		fileName = strtok(NULL, " ");
		redirectInput = 0;
	} else if (strstr(inputCopy, "<")) {
		cmd = strtok(inputCopy, "<"); // cat < tmp.txt
		fileName = strtok(NULL, " ");
		redirectInput = 1;
	}

	if (redirectInput != -1) {
		fileName = trimTrailingWhitespace(fileName);
		cmd = trimTrailingWhitespace(cmd);
		shellArgs[argNum] = strtok(cmd," "); //first arg
		while(shellArgs[argNum] != NULL) //additional args
		{
			shellArgs[++argNum] = strtok(NULL," "); //parse any more args
		}
		printf("Listing all arguments:\n");
		for (int index=0;index<argNum; index++)
		{
			printf("[%i] %s\n", index, shellArgs[index]);
		}
	}

	cmdPath = getCommandPath(input);

	if (cmd && fileName) {
		printf("Redirecting where cmd pathcmd  is '%s' and filename is '%s'\n", cmdPath, fileName);
		rc = fork();
		if (rc < 0)
			printf ("Fork failed\n");
		else if (rc == 0) // child proc
		{
			if (redirectInput) { // open in "r" mode
				close(STDIN_FILENO); // close stdin fd
				fd = open(fileName, O_RDONLY); // repla it with file
			} else { // open in "w" mode
				close(STDOUT_FILENO);
				fd = open(fileName, O_WRONLY);
			}
			execv(cmdPath, shellArgs);
			perror("exec");
		} else { // parent
			waitpid(rc, NULL, 0);
		}
		return 1;
	}
	return 0;
}

char *trimTrailingWhitespace(char *input)
{
  char *end;

  // Trim trailing space
  end = input + strlen(input) - 1;
  while(end > input && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return input;
}


int runCommand(char* input, int hasAmpersand) {
	// int pipesNeeded = createPipeWrapper(input);
	// int redirectionNeeded = redirect(input);
	// if (redirectionNeeded != 0 || pipesNeeded != 0) {
	// 	return 0;
	// }

	
	// execute here if no pipe needed
	int argNum = 0;
	int found = 0;
	char* shellArgs[MAX_INPUT_SIZE];
	char* command;
	char* inputCopy= {strdup(input)}; //strtok changes input, so copy

	shellArgs[argNum] = strtok(inputCopy," "); //first arg
	while(shellArgs[argNum] != NULL) //additional args
	{
		shellArgs[++argNum] = strtok(NULL," "); //parse any more args
	}
	// printf("Num of args: %i\n", argNum);
	// printf("Listing all arguments:\n");
	// for (int index=0;index<argNum; index++)
	// printf("[%i] %s\n", index, shellArgs[index]);

	command = getCommandPath(input);
	if (!command) {
		free(inputCopy);
		free(command);
		return -1;
	}

	int pid = fork();
	if (pid < 0) { // fork failed
		printf("Fork failed\n");
		free(inputCopy);
		free(command);
		return -1;
	}
	else if (pid == 0) { // child runs command
		// printf("Fork success\n");
		execv(command, shellArgs);
		printf("Exec failed\n");
		free(inputCopy);
		free(command);
		return -1;
	}
	else { // parent: wait until child is done (unless &)
		if (!hasAmpersand)
			waitpid(pid, NULL, 0);
		free(inputCopy);
		free(command);
		return 0;
	}
}

// input: str("34")
// output: int(34)
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

// modifies string (memory) pointed to
void trimLeadingWhitespace(char* inputStr) {
	while (isspace((unsigned char)*inputStr)) {
		memmove(inputStr, inputStr+1, strlen(inputStr));
	}
}