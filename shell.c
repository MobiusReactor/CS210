#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_CMD_LEN 512
#define MAX_CMD_TOKENS 50
#define MAX_PATH_LEN 512
#define NUM_CMDS 4
#define HISTORY_LENGTH 20


// Global history array

static char historyMap[HISTORY_LENGTH][MAX_CMD_LEN];
static int historyCounter = 0;




// Helper function for consistent error reporting
void printError(char* err, char* src, bool sysErr) {
	if (sysErr) {
		fprintf(stderr, "%s - \"%s\": %s\n", err, src, strerror(errno));
		
	} else {
		fprintf(stderr, "%s - \"%s\"\n", err, src);
	}
}

// Built-in command functions
void getpathFn(char *cmd[]) {
	if (cmd[1] != NULL) {
		printError("Error: Unexpected parameter", cmd[1], false);

	} else {
		printf("PATH: %s\n", getenv("PATH"));
	}
}

void setpathFn(char *cmd[]) {
	if (cmd[2] != NULL){
		printError("Error: Unexpected parameter", cmd[2], false);
		
	} else if (cmd[1] != NULL) {
		// If setenv returns 0, success
		if (setenv("PATH", cmd[1], 1) == 0) {
			printf("Path set to %s\n", cmd[1]);
			
		// Otherwise, print error
		} else {
			printError("Error setting path", cmd[1], true);
		}

	} else {
		printError("Error: Parameter expected", "<path>", false);
	}
}

void changedirFn(char *cmd[]) {
    if (cmd[2] != NULL) {
		printError("Error: Unexpected parameter", cmd[2], false);
		
	} else {
		char* dir = getenv("HOME");
		
		// if cmd[1] is present, use as new dir, else, use HOME as default
		if(cmd[1] != NULL) {
			dir = cmd[1];
		}
		
		// If chdir returns 0, success
		if(chdir(dir) == 0){
			char cwd[MAX_PATH_LEN];
			getcwd(cwd, MAX_PATH_LEN);
			
		    printf("Working directory: %s\n", cwd);
		
		// Otherwise, print error
		} else {
			printError("Error changing to dir", dir, true);
		}
	}
}

void printHistory(char *cmd[]) {
	if (cmd[1] != NULL) {
		printError("Error: Unexpected parameter", cmd[1], false);

	} else {
		int i;
		for(i = 0; i < (HISTORY_LENGTH); i++){
			if(historyMap[i][0] != 0){
				printf("%d: %s\n", (i + 1), historyMap[i]);
			}
		}
	}
}


// Array mapping names to function pointers for above functions
typedef struct {
	char *name;
	void (*function)(char**);
} cmd_map_t;

cmd_map_t commandMap [] = {
	{"getpath", &getpathFn},
	{"setpath", &setpathFn},
	{"cd", &changedirFn},
	{"history", &printHistory}
};

char* getString() {
	static char str[MAX_CMD_LEN];
	printf("> ");

	// Get input
	// fgets returns null if ctrl-D is pressed, so return exit if so
	if (fgets(str, MAX_CMD_LEN, stdin) == NULL){
		return "exit";
	}

	// If last character in string is not \n, input was too long, so extra
	// characters are flushed from stdin
	if (str[strlen(str) - 1] != '\n') {
		char c;
		while ((c = getchar()) != '\n' && c != EOF);
	}

	// Remove newline
	str[strlen(str) - 1] = '\0';

	return str;
}

char** getTokens(char* str) {
	static char* tokens[MAX_CMD_TOKENS];
	int i = 0;

	// Get first token
	tokens[i] = strtok(str, " |><&;\t\n");

	// Loop until token is null or max no. of tokens is reached
	while(tokens[i] != NULL && i < (MAX_CMD_TOKENS - 2)) {
		tokens[++i] = strtok(NULL, " |><&;\t\n");
	};

	// Add null pointer to end of array
	tokens[++i] = NULL;

	return tokens;
}

void runExternalCommand(char *cmd[]){
	// Create child process
	pid_t pid = fork();

	// pid < 0 means error occured creating child process
	if (pid < 0) {
		printError("Error creating child process", cmd[0], true);
		exit(-1);

	// pid == 0 means process is child, so run command
	} else if (pid == 0) {
		if ((execvp(cmd[0], cmd)) < 0) {
			printError("Error executing program", cmd[0], true);
		}
		exit(0);

	// pid > 0 means process is parent, so wait for child to finish
	} else if (pid > 0) {
		wait(NULL);
	}
}

void runCommand(char *input){
	char** cmd;
	
	int i; // Counter for the command map searcher
    int n; // C0unter for the token printer
	
	cmd = getTokens(input);

	// Print list of tokens, to ensure everything works
	n = 0;
	while(cmd[n] != NULL){
		printf("Command token %d: '%s'\n", n, cmd[n]);
		n++;
	}

	// Search the command map for the index of the entered command
	i = 0;
	while (i < NUM_CMDS && strcmp(commandMap[i].name, cmd[0]) != 0){
		i++;
	}
	
	// If i < NUM_CMDS, internal cmd found, otherwise, run external cmd
	if (i < NUM_CMDS){
		(*commandMap[i].function)(cmd);
	} else {
		runExternalCommand(cmd);
	}
}

int curHistV; // monitor to keep track of the last inserted history element

void addHistory(char *cmd){
	if (historyCounter < HISTORY_LENGTH){
		strcpy(historyMap[historyCounter], cmd);
		historyCounter++;

	} else {
		int i;
		for (i = 0; i < HISTORY_LENGTH; i++){
			strcpy(historyMap[i], historyMap[i + 1]);	
		}
		
		strcpy(historyMap[19], cmd);
	}
}

char* getHistory(char *cmd){
	// Strip "!" from command
	cmd++;
	
	// Convert to int
	int index = atoi(cmd);
	
	// Handle any errors
	if (index == 0) {
		fprintf(stderr, "Error: invalid input after '!'\n");
	} else if (index > historyCounter || index < (0 - historyCounter)) {
		fprintf(stderr, "Error: number out of range\n");

	// Otherwise, return appropriate value
	} else if (index > 0 && index <= historyCounter) {
		return historyMap[index - 1];
	} else if (index < 0 && index > (0 - historyCounter - 1)) {
		return historyMap[historyCounter + index];
	}
	
	// Return null if error occurred
	return NULL;
}

int main(int argc, char *argv[]) {
	char* input; // String before parsing into tokens

	// Backup PATH
	char* path = getenv("PATH");

	// Change CWD to home
	chdir(getenv("HOME"));

	char cwd[MAX_PATH_LEN];
	getcwd(cwd, MAX_PATH_LEN);

	printf("Stored PATH: %s\n\n", path);
	printf("Current Working Directory: %s\n\n", cwd);

	// Loop until getString() returns "exit"
	while(strcmp((input = getString()), "exit") != 0) {
		
		// Check if command is blank
		if (input[0] != 0){
			
			// Check if it's a history command
			if(input[0] == '!'){
				input = getHistory(input);
				
				if (input != NULL) {
					printf("<Running command from history - \"%s\">\n", input);
					runCommand(input);
				}
			} else {
				addHistory(input);
				runCommand(input);
			}
		}
	}

	// Restore the PATH variable
	setenv("PATH", path, 1);
	printf("Restoring PATH: %s\n", getenv("PATH"));

	printf("Quitting...\n");
	return 0;
}