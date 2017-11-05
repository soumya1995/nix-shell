#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <readline/readline.h>

#include "sfish.h"
#include "debug.h"
#include "csapp.h"

char* last_dir;


bool eval(char* input /*input has a \n at the end*/, char* envp[], char* process_name){

    char *arg[MAXARGS];
    char *prog[MAXARGS]; /*STORING THE EXECUTABLE*/
    pid_t pid; /*PROCESS ID*/

    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);
    int bg;
    int i=0, size;

    size = parse(input, arg);

    /*EXECUTABLE*/
    if((bg = (*arg[size-1] == '&')) != 0) /*IF THE JOB SHOULD RUN IN THE BACKGROUND*/
        arg[--size] = NULL;

    if(arg[0] == NULL) /*IGNORE EMPTY LINES*/
        return true;

    int j=0;
    while(arg[j] != NULL){

        if((strcmp(arg[j],">") == 0) || (strcmp(arg[j],"<") == 0))
            break;

        prog[j] = arg[j];
        j++;
    }
    prog[j] = NULL;

    if(!check_builtin(arg, size)){
    while(arg[i]!= NULL){

        if(strcmp(arg[i],"<") == 0){

            if(arg[i+1] == NULL){/*IF THERE IS NO ARGUMENT AFTER <*/
                printf(SYNTAX_ERROR,"unexpected token 'newline'");
                return false;

            }

            if(arg[i+2]==NULL){
               return file_redirect(prog, arg[i+1], NULL, 0);/*OPTION 0: input*/

            }

            if((strcmp(arg[i+2],">") == 0)){

                if(arg[i+3] ==NULL){/*IF THERE IS NO ARGUMENT AFTER >*/
                    printf(SYNTAX_ERROR,"unexpected token 'newline'");
                    return false;
                }
                else{

                    return file_redirect(prog, arg[i+1], arg[i+3], 2); /*OPTION 2: input and output*/

                }
            }
            else{
                printf(SYNTAX_ERROR,arg[i+2]);
                return false;
            }

        }

        else if(strcmp(arg[i],">") == 0){

            if(arg[i+1] == NULL){
                printf(SYNTAX_ERROR,"unexpected token 'newline'");
                return false;
            }

            if(arg[i+2] == NULL){
                return file_redirect(prog, NULL, arg[i+1], 1); /*OPTION 1: output*/

            }

            if((strcmp(arg[i+2],"<") == 0)){

                if(arg[i+3] ==NULL){/*IF THERE IS NO ARGUMENT AFTER >*/
                    printf(SYNTAX_ERROR,"unexpected token 'newline'");
                    return false;
                }
                else{
                    return file_redirect(prog, arg[i+3], arg[i+1], 2); /*OPTION 2: input and output*/

                }
            }
            else{
                printf(SYNTAX_ERROR,arg[i+2]);
                return false;
            }
        }
        else if(strcmp(arg[i],"|") == 0) /*HANDLING MULTIPLE PIPING*/{

            return piped(arg, size);
        }

        i++;
    }

        /*LAUNCH THE EXECUTABLE WHEN NO REDIRECTION OR PIPING IS NOT FOUND*/
       Sigprocmask(SIG_BLOCK, &mask, &prev); /*BLOCK SIGCHLD*/
        if((pid = Fork()) == 0){

            Setpgid(0,0);
            Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

            if(execvp(arg[0],arg)<0){
                return false;
            }
        }

        process_name[strlen(process_name)-1] = ' '; /*REPLACING TRAILING \n with space*/
        addjob(table, pid, process_name, (bg == 1? BG:FG));
        sigsuspend(&prev);

        Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

        if(!bg)
            waitfg(pid);
        else
            printf(JOBS_LIST_ITEM, pid_to_jid(table,pid), process_name);

    return true;
    }

    else /*THE COMMAND WAS A BUILT-IN AND WAS TRUE*/
        return true;

}

void addjob(process_fields *table, pid_t pid, char *name, int status){


    for(int i=0; i<MAXJOBS; i++){

        if(table[i].pid == 0){

            table[i].pid = pid;
            table[i].jid = i;
            table[i].name = calloc(strlen(name), strlen(name));
            strcpy(table[i].name, name);
            table[i].status = status;
            return;
        }
    }
}

bool deletejob(process_fields *table, pid_t pid){

    for(int i=0; i<MAXJOBS; i++){

        if(table[i].pid == pid){
            free(table[i].name);
            resetjob(&table[i]);
            return true;
        }
    }
    return false;
}

void resetjob(process_fields *process){

    process -> pid = 0;
    process -> jid = 0;
    process -> status = UNDEFINED;
    process -> name = '\0';

}

void initializejobs(process_fields *table){

    for(int i=0;i<MAXJOBS; i++)
        resetjob(&table[i]);

}

pid_t fgpid(process_fields *table){

    for(int i=0; i<MAXJOBS; i++){

        if(table[i].status == FG)
            return table[i].pid;
    }

    return 0;
}

process_fields *getjob_byjid(process_fields *table, int jid){

    for(int i=0; i<MAXJOBS; i++){

        if(table[i].jid == jid)
            return &table[i];
    }

    return NULL;
}

process_fields *getjob_bypid(process_fields *table, pid_t pid){

    for(int i=0; i<MAXJOBS; i++){

        if(table[i].pid == pid)
            return &table[i];
    }


    return NULL;
}

int pid_to_jid(process_fields *table, pid_t pid){

    for (int i = 0; i < MAXJOBS; ++i){

        if(table[i].pid == pid)
            return table[i].jid;
    }

    return 0;

}

void print_jobs(process_fields *table){

    for(int i=0; i<MAXJOBS; i++){

        if(table[i].pid != 0 && table[i].status == STOP){

            printf(JOBS_LIST_ITEM, table[i].jid, table[i].name);
        }
    }
}

/*HANDLE MULTIPLE PIPES*/
bool piped(char *argv[], int argc){

    int pipefd[2];

    int process; /*NUMBER OF PROCESS*/
    int process_no = 0; /*THE ith PROCESS STARTING FROM 0*/
    int end =0; /*INDICATE END OF PROCESS*/
    int read = STDIN_FILENO; /*READ END*/
    int write = STDOUT_FILENO; /*WRITE END*/

    char *prog[MAXARGS];
    pid_t pid;

    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);

    /*FIND NUMBER OF PROCESS*/
    for(int i=0; argv[i]!=NULL; i++){

        if(strcmp(argv[i], "|") == 0)
            process++;
    }

    process++;

    int i=0;

    Sigprocmask(SIG_BLOCK, &mask, &prev); /*BLOCK SIGCHLD*/

    while((argv[i]!=NULL && end !=1)){

        int j = 0;
        while(strcmp(argv[i],"|") != 0){

            prog[j] = argv[i];
            i++;
            if(argv[i] == NULL){
                end = 1;
                j++;
                prog[j] = NULL;
                break;

            }
            j++;
        }

        prog[j] = NULL;
        i++;


           pipe(pipefd);
           write = pipefd[1];

           if((pid = Fork()) == 0){

                Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

                if(read != STDIN_FILENO){

                    Dup2(read, STDIN_FILENO);
                    Close(read);
                }

                if(write != STDOUT_FILENO){

                    Dup2(write, STDOUT_FILENO);
                    Close(write);
                }

                if(execvp(prog[0],prog)<0)
                    return false;

           }

        Close(write);
        read = pipefd[0];

        Sigsuspend(&prev);

        Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

        process_no++;
    }



    if((pid = Fork()) == 0){

        Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

        if(read != STDIN_FILENO){
            Dup2(read, STDIN_FILENO);
            Close(read);
        }

        if(execvp(prog[0],prog)<0)
            return false;
    }

    sigsuspend(&prev);

    Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/


    return true;
}

int is_even(int n){ /*RETURN VALUE 0 IS EVEN AND 1 IS ODD*/

    if(n%2 == 0)
        return 0;
    else
        return 1;
}

/*EXECUTING PROCESSES WITH REDIRECTS*/
bool file_redirect(char *argv[], char *inputfile, char *outputfile, int option){

    sigset_t mask, prev;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGCHLD);


    pid_t pid; /*PROCESS ID*/
    int in_flags, out_flags, fd;

    in_flags = O_RDONLY;
    out_flags = O_WRONLY|O_CREAT|O_TRUNC;

    Sigprocmask(SIG_BLOCK, &mask, &prev); /*BLOCK SIGCHLD*/

    if((pid = Fork()) == 0){

        Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/
        if(option == 0){ /*input redirection*/

            fd = open(inputfile, in_flags, S_IRWXU);
            Dup2(fd, STDIN_FILENO); /*REPLACE STDIN WITH THE INPUT FILE*/
            close(fd);
        }

        if(option == 1){ /*output redirection*/

            fd = open(outputfile, out_flags, S_IRWXU);
            Dup2(fd, STDOUT_FILENO); /*REPLACE STDOUT WITH OUTPUT FILE*/
            close(fd);
        }

        if(option == 2){ /*input and output redirection*/

            /*INPUT REDIRECTION*/
            fd = open(inputfile, in_flags, S_IRWXU);
            Dup2(fd, STDIN_FILENO); /*REPLACE STDIN WITH THE INPUT FILE*/
            close(fd);

            /*OUTPUT REDIRECTION*/
            fd = open(outputfile, out_flags, S_IRWXU);
            Dup2(fd, STDOUT_FILENO); /*REPLACE STDOUT WITH OUTPUT FILE*/
            close(fd);
        }

        if(execvp(argv[0],argv)<0){
            return false;
        }


    }


    Sigsuspend(&prev);

    Sigprocmask(SIG_SETMASK, &prev, NULL); /*UNBLOCK SIGHLD*/

        return true;

}

void waitfg(pid_t pid){

    process_fields *process = getjob_bypid(table, pid);

    if(!process)
        return;

    while(process -> pid == pid && process -> status == FG)
        Sleep(1);

    return;
}


void sigchld_handler(int sig){


    int status;
    pid_t pid;


   while((pid = waitpid(-1, &status, 0|WNOHANG|WUNTRACED)) > 0){


        if(WIFSTOPPED(status)){

            process_fields *process = getjob_bypid(table, pid);
            process -> status = STOP;

            //fprintf(stdout, "[%d] %s stopped by signal %d \n",pid_to_jid(table, pid), process -> name, WSTOPSIG(status));
        }

        else if(WIFSIGNALED(status)){
            deletejob(table, pid);

            //fprintf(stdout, "[%d] %s terminated by signal %d \n",pid_to_jid(table, pid), process -> name, WTERMSIG(status));
        }

        else if(WIFEXITED(status)){

            deletejob(table, pid);
        }

        else
            unix_error("waitpid() error");
   }


   return;

}

void sigint_handler(int sig){

    pid_t pid;


    if((pid = fgpid(table)) > 0){

       // process_fields *process = getjob_bypid(table, pid);

        Kill(-pid, SIGINT);

        //fprintf(stdout, "\n [%d] %s terminated by CTRL-C signal\n",pid_to_jid(table, pid), process -> name);
    }


    return;

}

void sigtstp_handler(int sig){

    pid_t pid;

    if((pid = fgpid(table)) > 0){

        process_fields *process = getjob_bypid(table, pid);

        Kill(-pid, SIGTSTP);

        fprintf(stdout, "\n [%d] %s stopped\n",pid_to_jid(table, pid), process -> name);
    }
}

int parse(char *buff, char **argv){

    buff[strlen(buff)-1] = ' '; /*REPLACING TRAILING \n with space*/
    int argc =0;
    char delim;

    char *copy = strdup(buff); /*MUST FREE IT*/

    char *seg = strtok(buff, "><| ");


    while(seg != NULL){

        argv[argc++] = seg;

        int j =0;
        delim = copy[seg-buff+strlen(seg)];
        while(delim == '>' || delim == '<' || delim == '|' || delim == ' '){

            if(delim == '>')
                argv[argc++] = out;
            if(delim == '<')
                argv[argc++] = in;
            if(delim == '|')
                argv[argc++] = pipe_char;

            j++;
            delim = copy[seg-buff+strlen(seg)+j];

        }

        seg = strtok(NULL, "><| ");
    }

    argv[argc] = NULL;

    free(copy);

    return argc;

}


int parseline(char *buff, char **argv){


    buff[strlen(buff)-1] = ' '; /*REPLACING TRAILING \n with space*/

    while(*buff && (*buff == ' ')) /*IGNORE LEADING SPACE*/
        buff++;

    /*BUILD argv parameter list*/
    int argc =0;
    char *delim; /*POINTS TO FIRST SPACE DELIMETER*/
    while((delim = strchr(buff, ' '))){

        argv[argc++] = buff;
        *delim ='\0';
        buff = delim+1;
        while(*buff && (*buff == ' ')) /*IGNORE SPACES*/
            buff++;
    }

    argv[argc] = NULL;

    return argc;


}



bool check_builtin(char **argv, int argc){ /*STILL NEED TO IMPLEMENT JUNK ARGUMENTS AFTER ONE ARGUMENT*/


    if(strcmp(argv[0], "jobs") == 0){ /*LIST JOBS STOPPED BY CTRL-Z*/

        print_jobs(table);
        return true;
    }

    if(strcmp(argv[0], "kill") == 0){

        return kill_process(argv);
    }

    if(strcmp(argv[0], "fg") == 0){

        return fg(argv);
    }

    if(strcmp(argv[0], "&") == 0)
        return true;

    if(strcmp(argv[0], "help") == 0){  /*PRINTS THE HELP MENU FOR BUILT-INS*/
        help(argv, argc);
        return true;
    }
    if(strcmp(argv[0], "exit") == 0){  /*EXITS THE SHELL USING exit(3)*/
        exit(0);
        return true;
    }
    if(strcmp(argv[0], "cd") == 0){  /*CHANGES DIRECTORY*/
        cd(argv, argc);
        return true;
    }
    if(strcmp(argv[0], "pwd") == 0){  /*PRINTS THE CURRENT DIRECTORY*/
        char* path = pwd();


        int i =0, opt = 0, fd;
        int out_flags = O_WRONLY|O_CREAT|O_TRUNC;

        for(i= 0;i<argc;i++){
            if(strcmp(argv[i],">") == 0){
                opt = 1;
                break;
            }
        }
        if(opt ==1 && argc -1 == i){
            printf(SYNTAX_ERROR,  " unexpected token 'newline'");
            return true;
        }

        if(opt ==1){
            fd = open(argv[i+1], out_flags, S_IRWXU);
            write(fd, path, strlen(path));
            Close(fd);
        }

        else{
        write(1,path,strlen(path));
        printf("\n");
    }
        free(path);
        return true;
    }

    return false;
}

bool fg(char **argv){

    process_fields *process = NULL;

    if(argv[1] == NULL)
        return false;

    else if(argv[1][0] != '%')
        return false;

    else{

        int jid = atoi(&argv[1][1]);

        if(!(process = getjob_byjid(table, jid))){
            printf("[%c]: No such job\n",argv[1][1]);
            return true;
        }

        Kill((process -> pid), SIGCONT);
        process -> status = FG;
        waitfg(process -> pid);
        //printf(JOBS_LIST_ITEM, process -> jid, process -> name );

        return true;

    }

    printf("fg: internal error\n");
    return false;

}

bool kill_process(char **argv){

    process_fields *process = NULL;

    if(argv[1] == NULL)
        return false;

    else{

        if(argv[1][0] == '%'){ /*FOR JID*/
            int jid = atoi(&argv[1][1]);

            if(!(process = getjob_byjid(table, jid))){
                printf("%s: No such job\n",argv[1] );
                return true;
            }
        }

        else if(isdigit(argv[1][0])){

            pid_t pid = atoi(argv[1]);

            if(!(process = getjob_bypid(table, pid))){
                printf("%d: No such process\n",pid );
                return true;
            }
        }

        else{

            printf(SYNTAX_ERROR,"argument must be PID or JID" );
            return true;
        }

        Kill((process -> pid), SIGKILL);
        printf(JOBS_LIST_ITEM, process -> jid, process -> name );
        deletejob(table, process -> pid);
        waitfg(process -> pid);

        return true;
    }

    printf("kill: internal error\n");
    return false;

}


void help(char **argv, int argc){

    int j =0, opt = 0, fd;
    int stdout;
        int out_flags = O_WRONLY|O_CREAT|O_TRUNC;

        for(j= 0;j<argc;j++){
            if(strcmp(argv[j],">") == 0){
                opt = 1;
                break;
            }
        }

        if(opt ==1 && argc -1 == j){
            printf(SYNTAX_ERROR,  " unexpected token 'newline'");
            return;
        }

        if(opt ==1){
            fd = open(argv[j+1], out_flags, S_IRWXU);
            stdout = dup(STDOUT_FILENO);
            Dup2(fd, STDOUT_FILENO);
        }

    if(argc == 1 || (opt ==1 && argc == 3)){
        printf("These shell commands are defined iternally. Type 'help' to see this list.\nType 'help name' to find out more about function 'name'.\n cd [dir] \n exit [n] \n pwd[-LP] \n help [-d]\n");
    }

    else{

        for(int i=1; i<j; i++){
            if(strcmp(argv[i],"cd") == 0)
                printf("cd: cd [dir]\n    Change the shellworking directory\n");

            else if(strcmp(argv[i],"exit") == 0)
                printf("exit: exit [n]\n      Exit the shell\n");

            else if(strcmp(argv[i],"pwd") == 0)
                printf("pwd: pwd [-LP]\n     Print the name of the current working directory\n");

            else if(strcmp(argv[i],"help")==0)
                printf("help: help [-d]\n      Display information about built-in commands\n");
            else
                printf("sfish: help: no help topics match '%s' . Try 'help help'\n", argv[i]);
         }

    }

    if(opt == 1){
        Dup2(stdout, STDOUT_FILENO);
        Close(fd);
    }


}

void cd(char **argv, int argc){



    if(argc == 1){

        last_dir = pwd();
        chdir(getenv("HOME")); /*SET TO USER'S HOME DIRECTORY*/
    }

    else if(strcmp(argv[1],"-") == 0){ /*SET TO THE LAST USER DIRECTORY*/
        char* curr_dir = pwd();
        chdir(last_dir);

        last_dir = curr_dir;
    }

    else {

        last_dir = pwd();
        if(chdir(argv[1]) == -1){
            write(1, "sfish: cd: ", strlen("sfish: cd: "));
            perror(argv[1]);
        }
    }



}

char* pwd(){


    size_t size = 40;

    char* buff = (char*)calloc(size, size);
    char* path = getcwd(buff, size);

    while(path == NULL && errno == ERANGE){

        size = size*2;
        buff = (char*)realloc(buff,size);
        path = getcwd(buff, size);
    }

    return path;

}

char* substring(char* input, int start){

    char* sub = (char*)calloc(strlen(input)-start, strlen(input)-start);
    memcpy(sub, input+start, strlen(input)-start);

    return sub;
}

char* trim(char* input){

    char* trim_input = input;
    while(isspace((unsigned char)*trim_input))
        trim_input++;

    if(*trim_input== 0)
        return trim_input;

    char* terminate = trim_input+strlen(trim_input)-1;
    while(terminate > trim_input && isspace((unsigned char)*terminate))
        terminate--;

    *(terminate+1) = 0;
    return trim_input;
}

