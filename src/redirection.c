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

void check_redirection(char *input){

    for(int i=0; (input+i)!='\0'; i++){
        if(*(input+i)=='>')
            output_redirect()
    }
}