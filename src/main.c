#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <readline/readline.h>

#include "sfish.h"
#include "debug.h"
#include "csapp.h"

int main(int argc, char *argv[], char* envp[]) {

    extern char* last_dir;

    char* input;
    char *process_name;
    char* prompt;
    char* mod_prompt;
    bool exited = false;
    char* netid = " :: sodas >> ";

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }


    /*INITIALIZE JOB TABLE*/

    initializejobs(table);

    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT, sigint_handler); /*CTRL-C*/
    Signal(SIGTSTP, sigtstp_handler); /*CTRL-Z*/

    do {

        /*SET THE PROMPT*/
        char* path = pwd();

        if(strcmp(path,getenv("HOME")) <= 0)
            prompt = path;

        else{
        int home_len = strlen(getenv("HOME"));
        *(path+home_len-1) = '~';
        prompt = path+home_len-1;
        }

        mod_prompt = calloc(strlen(prompt)+strlen(netid), strlen(prompt)+strlen(netid));
        strcpy(mod_prompt, prompt);
        strcat(mod_prompt, netid);

        input = readline(mod_prompt);

        free(path); /*free after calling pwd()*/
        free(mod_prompt); /*free after calloc*/


        // If EOF is read (aka ^D) readline returns NULL
        if(input == NULL) {
            printf("\n");
            continue;
        }

        strcat(input,"\n"); /*ADD NEWLINE TO END OF input*/

        /*CALL check_builtin() to check if user have entered a vaid built-in and execute it*/
        process_name = calloc(strlen(input), strlen(input));
        strcpy(process_name, input);

        if(eval(input, envp, process_name) == false){
            printf(EXEC_NOT_FOUND, input);
            exit(0);
        }

        // Readline mallocs the space for input. You must free it.
        rl_free(input);
        rl_free(process_name);

    } while(!exited);

    free(last_dir);/*free the last_dir variable since it was malloc()*/

    exit(0);
    debug("%s", "user entered 'exit'");

    return EXIT_SUCCESS;
}
