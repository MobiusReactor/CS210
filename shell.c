#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdbool.h>

#include "shell.h"

/*
 * Main program loop
 */
int main(void) {
	char* path = getenv("PATH");
	char* home = getenv("HOME");
	char historyPath[MAX_PATH_LEN];
	char* input;
	
	strcpy(historyPath, home);
	strcat(historyPath, HIST_FILE);
	
	/* Load history and change working directory to HOME */
	loadHistory(historyPath);
	chdir(home);
	
	while (strcmp((input = getString()), "exit") != 0) {
		/* Check to ensure command does not start with delimiter */
		if (strchr(TOKEN_DELIMITERS, input[0]) == NULL) {
			/* Add command to history if it isn't a history invocation */
			if (input[0] != '!'){
				addHistory(input);
			}
			runCommand(input);
		}
		
		/* Reset used aliases list */
		memset(usedAliases, 0, sizeof(usedAliases));
		numUsedAliases = 0;
	}
	
	/* Restore the PATH variable and save history to file */
	setenv("PATH", path, 1);
	saveHistory(historyPath);
	
	printf("Quitting...\n");
	return 0;
}

/*
 * Gets a line of input from the user
 */
char* getString(void) {
	static char str[MAX_CMD_LEN];
	int c;
	
	printf("> ");

	/* fgets returns null if ctrl-D is pressed, so return "exit" if so */
	if (fgets(str, MAX_CMD_LEN, stdin) == NULL){
		return "exit";
	}

	/* If last character in string is not \n, input was too long, so extra
	 * characters are flushed from stdin, truncating input to MAX_CMD_LEN */
	if (str[strlen(str) - 1] != '\n') {
		while ((c = getchar()) != '\n' && c != EOF);
	}

	/* Remove newline */
	str[strlen(str) - 1] = '\0';

	return str;
}

/*
 * Converts the string to an array of tokens
 */
char** getTokens(char* str) {
	static char* tokens[MAX_CMD_TOKENS];
	int i = 0;

	/* Get first token */
	tokens[i] = strtok(str, TOKEN_DELIMITERS);

	/* Loop until token is null or max no. of tokens is reached */
	while (tokens[i] != NULL && i < (MAX_CMD_TOKENS - 2)) {
		tokens[++i] = strtok(NULL, TOKEN_DELIMITERS);
	};

	/* Add null pointer to end of array */
	tokens[++i] = NULL;
	
	return tokens;
}

/*
 * Concatenates the elements of the provided array to string str, starting at 
 * array[index]
 */
void appendArray(char* str, char* array[], int index) {
	while (array[index] != NULL){
		strcat(str, " ");
		strcat(str, array[index]);
		index++;
	}
}

/* 
 * Prints a list of all the aliases that have been defined
 */
void printAliasList(void) {
	int i;
	
	if (numAliases == 0) {
		printf("No aliases found\n");

	} else {
		for (i = 0; i < (numAliases); i++){
			printf("%s: %s\n", aliasMap[i].name, aliasMap[i].command);
		}
	}
}

/*
 * Adds the command to the history array
 */
void addHistory(char *cmd) {
	int i;
	
	/* Add command to history if history isn't already full */
	if (historyCounter < HISTORY_LEN){
		strcpy(historyMap[historyCounter], cmd);
		historyCounter++;

	} else {
		/* Move all commands in history to the previous index */
		for (i = 0; i < HISTORY_LEN; i++){
			strcpy(historyMap[i], historyMap[i + 1]);	
		}
		
		/* Add new command to end of the array */
		strcpy(historyMap[19], cmd);
	}
}

/* 
 * Saves the history to HIST_FILE in the HOME directory
 */
void saveHistory(char* path) {
	FILE *storeHist = fopen(path, "w");
	int i;

	if (storeHist == NULL) {
		perror("Error saving history");
		return;
	}

	for (i = 0; i < HISTORY_LEN; i++) {
		fprintf(storeHist, "%s\n", historyMap[i]);
	}

	fclose(storeHist);
}

/* 
 * Loads the history from HIST_FILE in the HOME directory
 */
void loadHistory(char* path) {
	char lineBuffer[MAX_CMD_LEN];
	FILE *loadHist = fopen(path, "r");
	int i;
	
	if (loadHist == NULL){
		/* If file is not found, errno is set to ENOENT */
		if (errno == ENOENT) {
			printf("History file not found\n");
			
		/* Otherwise, print error message */
		} else {
			perror("Error loading history file");
		}
		return;
	}

	for (i = 0; i < HISTORY_LEN; i++) {
		/* Load current line into temporary buffer */
		fgets(lineBuffer, MAX_CMD_LEN, (FILE*) loadHist);

		/* If not newline, there's a command there */
		if (strcmp(lineBuffer, "\n") != 0){
			/* Strip the newline and add command to history */
			lineBuffer[strlen(lineBuffer) - 1] = '\0';
			addHistory(lineBuffer);
		}
	}

	fclose(loadHist);
}

/*
 * Takes the raw input string and runs the command it represents
 *
 * If command is a history invocation or an alias, calls the appropriate helper
 * function, which in turn calls this function with the appropriate command from 
 * the history or the alias map
 */
void runCommand(char *input){
	char** cmd = getTokens(input);
	int i;
	int n = 0;
	
	/* If no command was entered, abort */
	if (cmd[0] == NULL) {
		return;
	}
	
	while(cmd[n] != NULL){
		printf("Command token %d: '%s'\n", n, cmd[n]);
		n++;
	}
	
	/* If command is history invocation */
	if (cmd[0][0] == '!'){
		runHistory(cmd);
		return;
	}
	
	/* If command is alias */
	for (i = 0; i < numAliases; i++) {
		if (strcmp(aliasMap[i].name, cmd[0]) == 0) {
			runAlias(cmd, i);
			return;
		}
	}
	
	/* If command is internal */
	for (i = 0; i < NUM_CMDS; i++) {
		if (strcmp(commandMap[i].name, cmd[0]) == 0) {
			(*commandMap[i].function)(cmd);
			return;
		}
	}

	/* If command is not history invocation, alias or internal command */
	runExternalCommand(cmd);
}

/*
 * Gets and runs the appropriate command when a history invocation occurs
 * (eg. when a command starts with "!") 
 */
void runHistory(char* cmd[]) {
	char newCmd[MAX_CMD_LEN];
	int index;
	
	/* Strip "!" from command */
	cmd[0]++;
	
	/* Convert to int */
	index = atoi(cmd[0]);
	
	/* Handle any errors */
	if (index == 0 && (strcmp(cmd[0], "-0") != 0)) {
		fprintf(stderr, "Error: invalid input after '!'\n");
		return;
	} else if (index > historyCounter || index < (1 - historyCounter)) {
		fprintf(stderr, "Error: number out of range\n");
		return;

	/* If no errors occur, return appropriate value */
	} else if (index > 0) {
		strcpy(newCmd, historyMap[index - 1]);
		
	} else {
		strcpy(newCmd, historyMap[historyCounter + index - 1]);
	}
	
	appendArray(newCmd, cmd, 1);
	
	/* Add any additional arguments and run command */
	printf("Running command from history - \"%s\"\n", newCmd);
	runCommand(newCmd);
}

/*
 * Runs the alias pointed to by index, adding any additional parameters from cmd
 */
void runAlias(char* cmd[], int index) {
	char newCmd[MAX_CMD_LEN];
	int i;
	
	/* Check to see if alias has already been used */
	for (i = 0; i < numUsedAliases; i++) {
		if (strcmp(usedAliases[i], cmd[0]) == 0) {
			fprintf(stderr, "Error: Alias loop detected\n");
			return;
		}
	}
	
	printf("Alias found: %s - \"%s\"\n", cmd[0], aliasMap[index].command);
	
	/* Add alias to array of used aliases */
	strcpy(usedAliases[numUsedAliases], cmd[0]);
	numUsedAliases++;
	
	/* Add any additional arguments, then run command */
	strcpy(newCmd, aliasMap[index].command);
	appendArray(newCmd, cmd, 1);
	runCommand(newCmd);
}

/*
 * Creates a child process and runs an external command
 */
void runExternalCommand(char *cmd[]) {
	/* Create child process */
	pid_t pid = fork();

	/* pid < 0 means error occured creating child process */
	if (pid < 0) {
		perror("Error creating child process");
		exit(-1);

	/* pid == 0 means process is child, so run command */
	} else if (pid == 0) {
		if ((execvp(cmd[0], cmd)) < 0) {
			perror("Error executing program");
		}
		exit(0);

	/* pid > 0 means process is parent, so wait for child to finish */
	} else if (pid > 0) {
		wait(NULL);
	}
}

/*
 * Returns the value of the PATH environment variable
 */
void getpathFn(char *cmd[]) {
	if (cmd[1] != NULL) {
		fprintf(stderr, "Error: Unexpected parameter - \"%s\"\n", cmd[1]);
	} else {
		printf("PATH: %s\n", getenv("PATH"));
	}
}

/*
 * Changes the value of the PATH environment variable to the path specified
 * by cmd[1]
 */
void setpathFn(char *cmd[]) {
	if (cmd[2] != NULL){
		fprintf(stderr, "Error: Unexpected parameter - \"%s\"\n", cmd[2]);
		
	} else if (cmd[1] == NULL) {
		fprintf(stderr, "Error: Parameter expected - <path>\n");
		
	} else {
		/* If setenv returns 0, path has been set successfully */
		if (setenv("PATH", cmd[1], 1) == 0) {
			printf("Path set to %s\n", cmd[1]);
			
		} else {
			fprintf(stderr, "Error setting path to \"%s\"\n", cmd[1]);
			perror(NULL);
		}
	}
}

/*
 * Changes the current working directory to that specified by cmd[1]
 *
 * If cmd[1] is not present, uses HOME as the default
 */
void changedirFn(char *cmd[]) {
    if (cmd[2] != NULL) {
		fprintf(stderr, "Error: Unexpected parameter - \"%s\"\n", cmd[2]);
		
	} else {
		char* dir = getenv("HOME");
		
		/* Use cmd[1] if available, otherwise leave as the default */
		if (cmd[1] != NULL) {
			dir = cmd[1];
		}
		
		/* If chdir returns 0, working directory has been changed */
		if (chdir(dir) == 0){
			char cwd[MAX_PATH_LEN];
			getcwd(cwd, MAX_PATH_LEN);
			
		    printf("Working directory: %s\n", cwd);
		
		} else {
			fprintf(stderr, "Error changing to dir \"%s\"\n", dir);
			perror(NULL);
		}
	}
}

/*
 * Prints the history of commands entered
 */
void printHistoryFn(char *cmd[]) {
	int i;
	
	if (cmd[1] != NULL) {
		fprintf(stderr, "Error: Unexpected parameter - \"%s\"\n", cmd[1]);

	} else {
		for (i = 0; i < (historyCounter); i++){
			printf("%d: %s\n", (i + 1), historyMap[i]);
		}
	}
}

/* 
 * Adds an alias with the name specified by cmd[1] and command by cmd[2...n]
 *
 * If cmd[1] is empty, prints a list of all previously defined aliases instead
 */
void addAliasFn(char *cmd[]) {
	char aliasCmd[MAX_CMD_LEN];
	int i = 0;
	
	if (cmd[2] != NULL) {
		/* Convert command array back into a string, removing "alias <name>" */
		strcpy(aliasCmd, cmd[2]);
		appendArray(aliasCmd, cmd, 3);

		/* Search for previously defined alias with the same name */
		while (i < numAliases && strcmp(aliasMap[i].name, cmd[1]) != 0){
			i++;
		}

		/* If alias exists already, replace it */
		if (i < numAliases) {
			strcpy(aliasMap[i].command, aliasCmd);
			printf("Alias '%s' replaced\n", cmd[1]);

		/* If another alias can be added, add it */
		} else if (numAliases < ALIAS_LIMIT) {
			strcpy(aliasMap[numAliases].name, cmd[1]);
			strcpy(aliasMap[numAliases].command, aliasCmd);
			printf("Alias '%s' created\n", cmd[1]);

			numAliases++;

		} else {
			fprintf(stderr, "Error: alias limit reached\n");
		}

	} else if (cmd[1] != NULL) {
		fprintf(stderr, "Error: Two parameters expected - <name> <command>\n");
		fprintf(stderr, "Alternatively, provide no parameters to list aliases\n");

	} else {
		printAliasList();
	}
}

/*
 * Removes the alias with the name specified by cmd[1]
 */
void removeAliasFn(char *cmd[]) {
	if (cmd[2] != NULL) {
		fprintf(stderr, "Error: Unexpected parameter - \"%s\"\n", cmd[2]);

	} else if (cmd[1] == NULL) {
		fprintf(stderr, "Error: parameter expected - <name>\n");

	} else {
		/* Search for alias */
		int i = 0;
		while (i < numAliases && strcmp(aliasMap[i].name, cmd[1]) != 0){
			i++;
		}
		
		/* If alias exists */
		if (i < numAliases) {
			/* Move all aliases after the target to the previous index */
			for ( ; i < ALIAS_LIMIT; i++){
				strcpy(aliasMap[i].name, aliasMap[i + 1].name);
				strcpy(aliasMap[i].command, aliasMap[i + 1].command);
			}
			numAliases--;

			printf("Alias '%s' removed\n", cmd[1]);
			
		} else {
			fprintf(stderr, "Error: alias not found\n");
		}
	}
}