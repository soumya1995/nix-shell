#ifndef SFISH_H
#define SFISH_H
#define MAXARGS 128

/*FUNCTIONS FOR eval*/
bool file_redirect(char *argv[], char *inputfile, char *outputfile, int option);

bool piped(char *argv[], int argc);

int is_even(int n);

bool eval(char* input, char* envp[]);

int parse_redirect(char *argv[], char *argv_mod[]);

int parseline(char* buf, char **argv);

/*SIGNAL HANDLERS*/
void sigchld_handler(int sig);

void sigint_handler(int sig);

/*FUNCTIONS FOR builtin*/
bool check_builtin(char **argv, int argc);

void help(char **argv, int argc);

char* substring(char* input, int start);

char* trim(char* input);

void cd(char **argv, int argc);

char* pwd();

/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"


#endif
