#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMD_LEN 512
#define MAX_CMD_TOKENS 50
#define MAX_PATH_LEN 512
#define NUM_CMDS 2


// Built-in command functions
void getpathFn(char *cmd[]) {
	if (cmd[1] != NULL) {
		fprintf(stderr, "Error: Unexpected parameter - ""%s""\n", cmd[1]);
		
	} else {
		printf("PATH: %s\n", getenv("PATH"));
	}
}

void setpathFn(char *cmd[]) {
	if (cmd[2] != NULL){
		fprintf(stderr, "Error: Unexpected parameter - ""%s""\n", cmd[2]);
		
	} else if (cmd[1] != NULL) {
		printf("Setting path to %s\n", cmd[1]);
		setenv("PATH", cmd[1], 1);
		
	} else {
		fprintf(stderr, "Error: Parameter expected\n");
	}
}


// Array mapping names to function pointers for above functions
typedef struct {
	char *name;
	void (*function)(char**);
} cmd_map_t;

cmd_map_t commandMap [] = {
	{"getpath", &getpathFn},
	{"setpath", &setpathFn}
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

void printError(char* err, char* src) {
	fprintf(stderr, "%s - \"%s\": %s\n", err, src, strerror(errno));
}

void runExternalCommand(char *cmd[]){
	// Create child process
	pid_t pid = fork();
	
	// pid < 0 means error occured creating child process
	if (pid < 0) {
		printError("Error creating child process", cmd[0]);
		exit(-1);
		
	// pid == 0 means process is child, so run command
	} else if (pid == 0) {
		if ((execvp(cmd[0], cmd)) < 0) {
			printError("Error executing program", cmd[0]);
		}
		exit(0);
	
	// pid > 0 means process is parent, so wait for child to finish
	} else if (pid > 0) {
		wait(NULL);
	}
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
	
	
	// Loop until getString() returns "exit"
	while(strcmp((input = getString()), "exit") != 0) {
		cmd = getTokens(input);
		
		// Check if command is blank
		if (cmd[0] != NULL){
			
			// Print list of tokens, to ensure everything works
			int n = 0;
			while(cmd[n] != NULL){
				printf("Command token %d: '%s'\n", n, cmd[n]);
				n++;
			}
			
			// Search the command map for the index of the entered command
			int i = 0;
			while (strcmp(commandMap[i].name, cmd[0]) != 0 && i < NUM_CMDS){
				i++;
			}
			
			// If i < NUM_CMDS, internal cmd found, otherwise, run external cmd
			if (i < NUM_CMDS){
				(*commandMap[i].function)(cmd);
			} else {
				runExternalCommand(cmd);
			}
		}
	}
	
	// Restore the PATH variable
	setenv("PATH", path, 1);
	printf("Restoring PATH: %s\n", getenv("PATH"));
	
	printf("Quitting...\n");
	return 0;
}
