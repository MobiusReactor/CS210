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
#define HISTORY_LEN 20
#define ALIAS_LEN 10
#define NUM_CMDS 6
#define HIST_FILE "/.hist_list"


typedef struct {
	char name[MAX_CMD_LEN];
	char cmd[MAX_CMD_LEN];
} alias_map_t;

// Global arrays for history and alias maps
static char historyMap[HISTORY_LEN][MAX_CMD_LEN];
static alias_map_t aliasMap[ALIAS_LEN];
static char usedAliases[ALIAS_LEN][MAX_CMD_LEN];
static int numSubstitutions = 0;
static int numAliases = 0;
static int historyCounter = 0;


// Helper functions
void printError(char* err, char* src, bool sysErr) {
	if (sysErr) {
		fprintf(stderr, "%s - \"%s\": %s\n", err, src, strerror(errno));
		
	} else {
		fprintf(stderr, "%s - \"%s\"\n", err, src);
	}
}

void printAliasList() {
	int i = 0;
	for(i = 0; i < (numAliases); i++){
		// if(strlen(aliasMap[i].name) != 0){
		if(aliasMap[i].name[0] != 0){
			printf("%s: %s\n", aliasMap[i].name, aliasMap[i].cmd);
		}
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

void printHistoryFn(char *cmd[]) {
	if (cmd[1] != NULL) {
		printError("Error: Unexpected parameter", cmd[1], false);

	} else {
		int i;
		for(i = 0; i < (HISTORY_LEN); i++){
			if(historyMap[i][0] != 0){
				printf("%d: %s\n", (i + 1), historyMap[i]);
			}
		}
	}
}

void addAliasFn(char *cmd[]) {
	if (cmd[2] != NULL) {
		// Convert command array back into a string, removing "alias <name>"
		char str[MAX_CMD_LEN];
		strcpy(str, cmd[2]);

		int j = 3;
		while (cmd[j] != NULL){
			strcat(str, " ");
			strcat(str, cmd[j]);
			j++;
		}

		// Search for alias
		int i = 0;
		while (i < numAliases && strcmp(aliasMap[i].name, cmd[1]) != 0){
			i++;
		}

		// If alias exists already, replace it
		if (i < numAliases) {
			strcpy(aliasMap[i].cmd, str);
			printf("Alias replaced\n"); 

		// If another alias can be added, add it
		} else if (numAliases < ALIAS_LEN) {
			strcpy(aliasMap[numAliases].name, cmd[1]);			
			strcpy(aliasMap[numAliases].cmd, str);
			printf("Alias created\n"); 

			numAliases++;

		// Print error message
		} else {
			fprintf(stderr, "Error: alias limit reached\n");
		}

	} else if (cmd[1] != NULL) {
		fprintf(stderr, "Error: parameter expected\n");

	} else {
		printAliasList();
	}
}

void removeAliasFn(char *cmd[]) {
	if (cmd[2] != NULL) {
		fprintf(stderr, "Error: unexpected parameter\n");

	} else if (cmd[1] == NULL) {
		fprintf(stderr, "Error: parameter expected\n");

	} else {
		// Search for alias
		int i = 0;
		while (i < numAliases && strcmp(aliasMap[i].name, cmd[1]) != 0){
			i++;
		}
		
		// If alias exists
		if (i < numAliases) {
			
			// Move all aliases after the target to the previous index
			for ( ; i < ALIAS_LEN; i++){
				strcpy(aliasMap[i].name, aliasMap[i + 1].name);
				strcpy(aliasMap[i].cmd, aliasMap[i + 1].cmd);
			}
			
			numAliases--;
		} else {
			fprintf(stderr, "Error: alias not found\n");
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
	{"history", &printHistoryFn},
	{"alias", &addAliasFn},
	{"unalias", &removeAliasFn}
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

void addHistory(char *cmd){
	// If there's still room to add more commands to history
	if (historyCounter < HISTORY_LEN){
		strcpy(historyMap[historyCounter], cmd);
		historyCounter++;

	// Otherwise, history is already full
	} else {
		// Move all commands to the previous index
		for (int i = 0; i < HISTORY_LEN; i++){
			strcpy(historyMap[i], historyMap[i + 1]);	
		}
		
		// Add new command to end of the array
		strcpy(historyMap[19], cmd);
	}
}

char* getHistory(char *cmd){
	// Strip "!" from command
	cmd++;
	
	// Convert to int
	int index = atoi(cmd);
	
	// Handle any errors
	if (index == 0 && (strcmp(cmd, "0") != 0) && (strcmp(cmd, "-0") != 0)) {
		fprintf(stderr, "Error: invalid input after '!'\n");
	} else if (index > historyCounter || index < (1 - historyCounter)) {
		fprintf(stderr, "Error: number out of range\n");

	// Otherwise, return appropriate value
	} else if (index > 0 && index <= historyCounter) {
		return historyMap[index - 1];

	} else if (index <= 0 && index > (0 - historyCounter)) {
		return historyMap[historyCounter + index - 1];
	}
	
	// Return null if error occurred
	return NULL;
}

void storeHistory(char* path) {
	// Original home directory is stored in homeDir
	FILE *storeHist = fopen(path, "w");

	if(storeHist == NULL) {
		printError("Error saving history", path, true);
		return;
	}

	for (int i = 0; i < HISTORY_LEN; i++) {
		fprintf(storeHist, "%s\n", historyMap[i]);
	}

	fclose(storeHist);
}

void loadHistory(char* path) {
	char lineBuffer[MAX_CMD_LEN];

	FILE *loadHist = fopen(path, "r");

	printf("Loading history from %s\n", path);
	if(loadHist == NULL){
		fprintf(stderr, "%s\n", strerror(errno));
		return;
	}

	for (int i = 0; i < HISTORY_LEN; i++) {
		// Load current line into temporary buffer
		fgets(lineBuffer, MAX_CMD_LEN, (FILE*) loadHist);

		// If not newline, there's a command there
		if(strcmp(lineBuffer, "\n") != 0){
			// Strip the newline for insertion into array
			lineBuffer[strlen(lineBuffer)-1] = '\0';
			addHistory(lineBuffer);
		}
	}

	fclose(loadHist);
}

void runCommand(char *input){
	char** cmd = getTokens(input);
	
	// If no command was entered, abort
	if (cmd[0] == NULL) {
		return;
	}
	
	// Print list of tokens, to ensure everything works
	int n = 0;
	while(cmd[n] != NULL){
		printf("Command token %d: '%s'\n", n, cmd[n]);
		n++;
	}
	
	// Check if command is history invocation
	if(input[0] == '!'){
		input = getHistory(input);
		
		if (input != NULL) {
			// Add any additional arguments
			char str[MAX_CMD_LEN];
			strcpy(str, input);

			int arg = 1;
			while (cmd[arg] != NULL){
				strcat(str, " ");
				strcat(str, cmd[arg]);
				arg++;
			}
			
			printf("<Running command from history - \"%s\">\n", input);
			runCommand(str);
		}
		return;
	}
	
	//Search the alias map for entered command
	for (int i = 0; i < numAliases; i++) {
		if (strcmp(aliasMap[i].name, cmd[0]) == 0) {
			for (int j = 0; j < numSubstitutions; j++) {
				if (strcmp(usedAliases[j], cmd[0]) == 0) {
					fprintf(stderr, "Alias loop detected - abort\n");
					return;
				}
			}
			
			printf("\nAlias found: %s - \"%s\"\n\n", cmd[0], aliasMap[i].cmd);
			
			strcpy(usedAliases[numSubstitutions], cmd[0]);
			numSubstitutions++;
			
			printf("Used aliases:\n");
			for (int j = 0; j < numSubstitutions; j++) {
				printf("<%s>  \n", usedAliases[j]);
			}

			// Add any additional arguments
			char str[MAX_CMD_LEN];
			strcpy(str, aliasMap[i].cmd);

			int arg = 1;
			while (cmd[arg] != NULL){
				strcat(str, " ");
				strcat(str, cmd[arg]);
				arg++;
			}
			
			runCommand(str);
			return;
		}
	}
	
	// Search the command map for the index of the entered command
	for (int i = 0; i < NUM_CMDS; i++) {
		if (strcmp(commandMap[i].name, cmd[0]) == 0) {
			(*commandMap[i].function)(cmd);
			return;
		}
	}

	// If command entered is not internal command or alias
	runExternalCommand(cmd);
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
	
	// Load history from file
	char str[MAX_PATH_LEN] = "";
	strcpy(str, getenv("HOME"));
	
	char* historyPath = strcat(str, HIST_FILE);
	loadHistory(historyPath);

	// Loop until getString() returns "exit"
	while(strcmp((input = getString()), "exit") != 0) {
		
		// Check if command is blank
		if (input[0] != 0){
			
			// Check if it's a history command
			if(input[0] != '!'){
				addHistory(input);
			}
			
			runCommand(input);
		}
		
		memset(usedAliases, 0, sizeof(usedAliases));
		numSubstitutions = 0;
	}

	// Save history to file
	storeHistory(historyPath);

	// Restore the PATH variable
	setenv("PATH", path, 1);
	printf("Restoring PATH: %s\n", getenv("PATH"));

	printf("Quitting...\n");
	return 0;
}