#ifndef SFISH_H
#define SFISH_H
#define MAXARGS 128

/*FUNCTIONS FOR evaluate.c*/
bool eval(char* input, char* envp[]);

int parseline(char* buf, char **argv);

/*FUNCTIONS FOR builtin.c*/
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

#endif
