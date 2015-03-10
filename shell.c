#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_CMD_LEN 512
#define MAX_CMD_TOKENS 50

char** getTokens(char* str) {
	static char* tokens[MAX_CMD_TOKENS];
	int i = 0;
	
	// Get first token
	tokens[i] = strtok(str, " |><&;");
	
	// Loop until token is null or max no. of tokens is reached
	while(tokens[i] != NULL && i < (MAX_CMD_TOKENS - 1)) {
		tokens[++i] = strtok(NULL, " |><&;");
	};
	
	return tokens;
}

char* getString() {
	static char str[MAX_CMD_LEN];
	printf("> ");
	
	// Get input
	// fgets returns null if ctrl-D is pressed, so return exit if so
	if (fgets(str, sizeof(str), stdin) == NULL){
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

void runExternalCommand(char *tokens[]){
	// Create child process
	pid_t pid = fork();
	
	// pid < 0 means error occured creating child process
	if (pid < 0) {
		printf("Error creating child process\n");
		exit(-1);
		
	// pid == 0 means process is child, so run command
	} else if (pid == 0) {
		if ((execvp(tokens[0], tokens)) < 0) {
			printf("Error executing program\n");
		}
		exit(0);
	
	// pid > 0 means process is parent, so wait for child to finish
	} else if (pid > 0) {
		wait(NULL);
	}
}

int main(int argc, char *argv[]) {
	char* input; // String before parsing into tokens
	char** cmd;  // Array of string tokens, cmd[0] is the command
	
	while(strcmp((input = getString()), "exit") != 0) {
		cmd = getTokens(input);
		
		// Print list of tokens, to ensure everything works
		int n = 0;
		while(cmd[n] != NULL){
			printf("Command token %d: '%s'\n", n, cmd[n]);
			n++;
		}
		
		// Calls a helper function to run an external command
		runExternalCommand(cmd);
	}
	
	printf("Quitting...\n");
	return 0;
}
