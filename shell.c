#include <stdio.h>
#include <string.h>

#define MAX_CMD_LEN 512

char* getCommand() {
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

int main(int argc, char *argv[]) {
	char* command;
	
	while(strcmp((command = getCommand()), "exit") != 0) {
		printf("Command entered: %s\n", command);
	}
	
	printf("Quitting...\n");
	return 0;
}