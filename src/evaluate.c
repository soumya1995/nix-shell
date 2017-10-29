#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <readline/readline.h>

#include "sfish.h"
#include "debug.h"
#include "csapp.h"

char* last_dir;

bool eval(char* input /*input has a \n at the end*/, char* envp[]){

    char *arg[MAXARGS];
    char *prog[MAXARGS]; /*STORING THE EXECUTABLE*/
    pid_t pid; /*PROCESS ID*/
   // int bg;
    int i=0;

    int size = parseline(input, arg);

    /*EXECUTABLE*/
    //if((bg = (*arg[size-1] == '&')) != 0) /*IF THE JOB SHOULD RUN IN THE BACKGROUND*/
       // arg[--size] = NULL;

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

        i++;
    }

        if((pid = Fork()) == 0){ /*LAUNCH THE EXECUTABLE WHEN NO REDIRECTION OR PIPING IS NOT FOUND*/

            if(execvp(arg[0],arg)<0)
                return false;
        }

        int status;

        if(waitpid(pid, &status, 0) < 0)
            unix_error("fg: waitpid error");

        return true;
    }

    else /*THE COMMAND WAS A BUILT-IN AND WAS TRUE*/
        return true;

}

/*EXECUTING PROCESSES WITH REDIRECTS*/
bool file_redirect(char *argv[], char *inputfile, char *outputfile, int option){


    pid_t pid; /*PROCESS ID*/
    int in_flags, out_flags, fd;

    in_flags = O_RDONLY;
    out_flags = O_WRONLY|O_CREAT|O_TRUNC;

    if((pid = Fork()) == 0){
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
    /*else{
        printf("Cannot create child\n");
        return false;
    }*/

    int status;

        if(waitpid(pid, &status, 0) < 0)
            unix_error("fg: waitpid error");


        return true;

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
        write(1,path,strlen(path));
        printf("\n");
        free(path);
        return true;
    }

    return false;
}


void help(char **argv, int argc){


    if(argc == 1){
        printf("These shell commands are defined iternally. Type 'help' to see this list.\nType 'help name' to find out more about function 'name'.\n cd [dir] \n exit [n] \n pwd[-LP] \n help [-d]\n");
    }

    else{

        if(strcmp(argv[1],"cd") == 0)
            printf("cd: cd [dir]\n    Change the shellworking directory\n");

        else if(strcmp(argv[1],"exit") == 0)
            printf("exit: exit [n]\n      Exit the shell\n");

        else if(strcmp(argv[1],"pwd") == 0)
            printf("pwd: pwd [-LP]\n     Print the name of the current working directory\n");

        else if(strcmp(argv[1],"help")==0)
            printf("help: help [-d]\n      Display information about built-in commands\n");
        else
            printf("sfish: help: no help topics match '%s' . Try 'help help'\n", argv[1]);



    }


}

void cd(char **argv, int argc){


    //char* dir = trim(substring(input,3));

    if(argc == 1){

        last_dir = pwd();
        chdir(getenv("HOME")); /*SET TO USER'S HOME DIRECTORY*/
    }

    else if(strcmp(argv[1],"-") == 0){ /*SET TO THE LAST USER DIRECTORY*/
        char* curr_dir = pwd();
        chdir(last_dir);

        //free(curr_dir); /*free the call to pwd for current directory*/
        last_dir = curr_dir;
    }

    else {
        //free(last_dir); /*free the last call to pwd*/

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

    /*pwd() allocates mem and needs to be freed*/
    //free(buff);

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

