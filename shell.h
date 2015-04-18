/* Preprocessor constants */
#define MAX_CMD_LEN 512
#define MAX_CMD_TOKENS 50
#define MAX_PATH_LEN 512
#define HISTORY_LEN 20
#define ALIAS_LIMIT 10
#define NUM_CMDS 6
#define TOKEN_DELIMITERS " |><&;\t\n"
#define HIST_FILE "/.hist_list"


/* Function prototypes */
void getpathFn(char* cmd[]);
void setpathFn(char* cmd[]);
void changedirFn(char* cmd[]);
void printHistoryFn(char* cmd[]);
void addAliasFn(char* cmd[]);
void removeAliasFn(char* cmd[]);

void runCommand(char* input);
void runHistory(char* cmd[]);
void runAlias(char* cmd[], int index);
void runExternalCommand(char *cmd[]);

void appendArray(char* str, char* array[], int index);
void printAliasList(void);
char* getString(void);
char** getTokens(char* str);

void addHistory(char* cmd);
void saveHistory(char* path);
void loadHistory(char* path);


/* Type mapping alias name to a command */
typedef struct {
	char name[MAX_CMD_LEN];
	char command[MAX_CMD_LEN];
} alias_map_t;

/* Type mapping internal function name to a pointer to that function */
typedef struct {
	char *name;
	void (*function)(char**);
} cmd_map_t;


/* Global variables for history, alias, and alias substitution lists */
static char historyMap[HISTORY_LEN][MAX_CMD_LEN];
static int historyCounter = 0;

static alias_map_t aliasMap[ALIAS_LIMIT];
static int numAliases = 0;

static char usedAliases[ALIAS_LIMIT][MAX_CMD_LEN];
static int numUsedAliases = 0;


/* Array mapping function names to a pointer to one of the built-in function */
cmd_map_t commandMap [] = {
	{"getpath", &getpathFn},
	{"setpath", &setpathFn},
	{"cd", &changedirFn},
	{"history", &printHistoryFn},
	{"alias", &addAliasFn},
	{"unalias", &removeAliasFn}
};