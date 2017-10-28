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

bool eval(char* input, char* envp[]){

    char *arg[MAXARGS];
    pid_t pid; /*PROCESS ID*/

    int size = parseline(input, arg);

    if(arg[0] == NULL) /*IGNORE EMPTY LINES*/
        return true;

    if(!check_builtin(arg, size)){

        if((pid = Fork()) == 0){

            if(execvp(arg[0],arg)<0)
                return false;
        }
    }
        printf("hi\n");
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

