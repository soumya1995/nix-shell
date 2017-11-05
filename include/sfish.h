#ifndef SFISH_H
#define SFISH_H
#define MAXARGS 128
#define MAXJOBS 30

#define UNDEFINED 0
#define FG 1
#define BG 2
#define STOP 3

/*FUNCTIONS FOR eval*/
bool file_redirect(char *argv[], char *inputfile, char *outputfile, int option);

bool piped(char *argv[], int argc);

int is_even(int n);

bool eval(char* input, char* envp[], char* process_name);

int parse_redirect(char *argv[], char *argv_mod[]);

int parseline(char* buf, char **argv);

int parse(char *buff, char **argv);

int parse_string(char *buff, char **argv, int pointer);

/*SIGNAL HANDLERS*/
void sigchld_handler(int sig);

void sigint_handler(int sig);

void sigtstp_handler(int sig);

/*FUNCTIONS FOR builtin*/
bool check_builtin(char **argv, int argc);

void help(char **argv, int argc);

char* substring(char* input, int start);

char* trim(char* input);

void cd(char **argv, int argc);

char* pwd();


/*Variable declarations*/

    char *in;
    char *out;
    char *pipe_char;

struct process_fields{
    pid_t pid;
    int jid;
    char* name;
    int status; /*3 means STOP*/
};

typedef struct process_fields process_fields;

process_fields table[MAXJOBS];

//int jid_count = 1; /*number of jobs*/

/*FUNCTIONS FOR JOBS*/

void waitfg(pid_t pid);

void initializejobs(process_fields *table);

void addjob(process_fields *table, pid_t pid, char *name, int status);

bool deletejob(process_fields *table, pid_t pid);

void resetjob(process_fields *process);

pid_t fgpid(process_fields *table);

process_fields *getjob_byjid(process_fields *table, int jid);

process_fields *getjob_bypid(process_fields *table, pid_t pid);

int pid_to_jid(process_fields *table, pid_t pid);

void print_jobs(process_fields *table);

bool fg(char **argv);

bool kill_process(char **argv);



/* Format Strings */
#define EXEC_NOT_FOUND "sfish: %s: command not found\n"
#define JOBS_LIST_ITEM "[%d] %s\n"
#define STRFTIME_RPRMT "%a %b %e, %I:%M%p"
#define BUILTIN_ERROR  "sfish builtin error: %s\n"
#define SYNTAX_ERROR   "sfish syntax error: %s\n"
#define EXEC_ERROR     "sfish exec error: %s\n"


#endif
