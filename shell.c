#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_LEN 512
#define MAX_CMD_SIZE 10

char** getCommand() {
	static char str[MAX_CMD_LEN];
	static char* output[MAX_CMD_SIZE];
	
	memset(output, 0, MAX_CMD_SIZE); // Reset array, since it's static it'd normally retain values between commands
	printf("> ");
	
	// Get input
	// fgets returns null if ctrl-D is pressed, so return exit if so
	if (fgets(str, sizeof(str), stdin) == NULL){
		strcpy(output[0], "exit");
	}

	// If last character in string is not \n, input was too long, so extra 
	// characters are flushed from stdin
	if (str[strlen(str) - 1] != '\n') {
		char c;
		while ((c = getchar()) != '\n' && c != EOF);
	}
	
	// Remove newline
	str[strlen(str) - 1] = '\0';
	
	// Iterate through the string using strtok to split into whitespace seperated tokens
	// If there's a better way to do this feel free to tear it up
	// Also malloc some stuff onto the first entry 
	
	int i = 0;
	char* input = strtok(str, " ");
	output[0] = malloc(MAX_CMD_LEN); // bit hacky, to stop the while loop not finding something at index 0 and crashing spectacularly
	strcpy(output[0], "");
	
	while(input){
		output[i] = malloc(MAX_CMD_LEN); // Allocating space to a new array entry
		strcpy(output[i], input); // Copying the next input into the array
		input = strtok(NULL, " "); // Getting next input
		i++;
	};
	
	return output;
}

int main(int argc, char *argv[]) {
	char** tokens; // Array of string tokens, command is first entry
	
	
	while(strcmp((tokens = getCommand())[0], "exit") != 0) {
		int n = 0;
		while(tokens[n] != NULL){
			printf("Command entered: %s\n", tokens[n]); // Just a test to see if it's working okay
			n++;
		}
	}
	
	printf("Quitting...\n");
	return 0;
}