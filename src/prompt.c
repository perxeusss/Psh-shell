#include "../include/shell.h" 
#include "../include/prompt.h"
#include "../include/builtins.h"

#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<stdio.h>


static void path_for_prompt(const shell_state *st, char buf[PATH_MAX]) {
    char cwd[PATH_MAX] ;
    if(getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(buf, "?") ;
        return ;
    }   
    size_t len = strlen(cwd) ;
    if(len && !strncmp(cwd, st -> home, len) && (
        st -> home[len] == '\0' || st -> home[len] == '/')) {
        strcpy(buf, "~") ;
        if(st -> home[len] == '/') {
            strcat(buf, cwd + len) ;
        }
    } else {
        strcpy(buf, cwd) ;
    }
}
void init_prompt(shell_state *st) {
    char cwd[PATH_MAX] ;
    if(getcwd(cwd, sizeof(cwd))) {
        strcpy(st -> home, cwd) ;
    }
    else st -> home[0] = '\0' ;
}

void show_prompt(const shell_state *st) {
    char host[256] ;

    if(gethostname(host, sizeof(host))) {
        // fprintf(stderr, "Error getting hostname\n") ;
        // return ;
        strcpy(host, "?") ;
    }
    char *user = getenv("USER") ;
    if(!user) {
        user = "user" ;
    }
    char path[PATH_MAX] ;
    path_for_prompt(st, path) ;
    printf("%s@%s:%s$ ", user, host, path) ;
    fflush(stdout) ;
}