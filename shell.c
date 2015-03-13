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

char* historyMap[HISTORY_LENGTH][MAX_CMD_TOKENS];




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
		fprintf(stderr, "Error: Unexpected parameter - ""%s""\n", cmd[1]);

	} else {
		int i;
		int p;
		
		for(i = 0; i < (HISTORY_LENGTH); i++){
			
			if(historyMap[i][0] != NULL){
				printf("%d: ", (i+1));
				for(p = 0; (historyMap[i][p] != NULL && p < MAX_CMD_TOKENS); p++){
					printf("%s ", historyMap[i][p]);
				}
				printf("\n");
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

int curHistV; // monitor to keep track of the last inserted history element

void insertHistory(char *cmd[]){

	memmove(historyMap[curHistV], cmd, sizeof(cmd));
	
	curHistV++;
}

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

void pickCommand(char *cmd[]){
	int i;  // Counter for the command map searcher
    int n; // COunter for the token printer

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


void processHistory(char *cmd[]){
	int chosenH; // Variable to hold the chosen history val
	int i; // Validation iterator
	
	// Strip the !- from the history command if exists

	if(cmd[0][1] == '-'){
		memmove(cmd[0], cmd[0]+2, strlen(cmd[0]));
	} else { // Otherwise just strip the !
		memmove(cmd[0], cmd[0]+1, strlen(cmd[0]));
	}

	// Validate if numerical

	for(i = 0; i < strlen(cmd[0]); i++){
		if(!isdigit(cmd[0][i])){
			fprintf(stderr, "Error: Not a number - ""%s""\n", cmd[0]);
			return;
		}
	}
	
	chosenH = atoi(cmd[0]);
	
	printf("Chosen history value: %d", chosenH);

}

int main(int argc, char *argv[]) {
	char* input; // String before parsing into tokens
	char** cmd;  // Array of tokens, cmd[0] is command, cmd[1...n] are args

	// Backup PATH
	char* path = getenv("PATH");

	// Change CWD to home
	chdir(getenv("HOME"));

	char cwd[MAX_PATH_LEN];
	getcwd(cwd, MAX_PATH_LEN);

	printf("Stored PATH: %s\n\n", path);
	printf("Current Working Directory: %s\n\n", cwd);
	curHistV = 0; // Initialise the history counter

	// Loop until getString() returns "exit"
	while(strcmp((input = getString()), "exit") != 0) {
		cmd = getTokens(input);

		// Check if command is blank
		if (cmd[0] != NULL){
			
			// Check if it's a history command
			if(strcspn(cmd[0], "!") < strlen(cmd[0])){
				processHistory(cmd);
			} else {
				insertHistory(cmd);
				pickCommand(cmd);
			}
		}
	}

	// Restore the PATH variable
	setenv("PATH", path, 1);
	printf("Restoring PATH: %s\n", getenv("PATH"));

	printf("Quitting...\n");
	return 0;
}
