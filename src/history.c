#include "../include/builtins.h"
#include "../include/history.h"
#include "../include/shell.h"
#include "../include/helpers.h"

#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *history_file_path(void) {
    static char path[1024] ;
    static int inited = 0 ;

    if(!inited) {
        const char *home_env = getenv("HOME");

        if(!home_env) {
            struct passwd *pw = getpwuid(getuid());
            home_env = (pw && pw -> pw_dir) ? pw -> pw_dir : "." ;
        }
        snprintf(path, sizeof(path), "%s/.Psh_history", home_env);
        inited = 1 ;
    } 
    return path ;
}

static int is_blank (const char *s) {
    for( ; *s ; s++) {
        if(*s == '\n' || *s == '\t' || *s == ' ' || *s == '\r') {
            continue ;
        }
        return 0 ;
    }
    return 1 ;
}

static int contains_atomic_log(const char *cmd) {
    char buf[1024] ;
    strncpy(buf, cmd, sizeof(buf) - 1) ;
    buf[sizeof(buf) - 1] = '\0' ;

    char *tok = strtok(buf, " \t|><;&") ;
    for( ; tok ; tok = strtok(NULL, " \t|><;&") ) {
        if(!strcmp(tok, "log")) return 1 ;
    }
    return 0 ;
}

void history_load(shell_state *st) {
    FILE *f = fopen(history_file_path(), "r") ;
    if(!f) return ;

    char line[4096] ;
    while(fgets(line, sizeof(line), f)) {
        size_t n = strlen(line) ;

        if(n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0' ;
        }
        if(st -> log_count == LOG_SIZE) {
            for(int i = 0 ; i + 1 < LOG_SIZE ; i++) {
                strcpy(st -> log[i], st -> log[i + 1]) ;
            }
            st -> log_count-- ;
        }
        strcpy(st -> log[st -> log_count++], line) ;
    }
    fclose(f) ;
}

void history_save(shell_state *st) {
    FILE *f = fopen(history_file_path(), "w") ;
    if(!f) return ;

    for(int i = 0 ; i < st -> log_count ; i++) {
        fputs(st -> log[i], f) ;
        fputc('\n', f) ;
    }
    fclose(f) ;
}

void history_add_if_needed(shell_state *st, const char *cmd) {
    if(is_blank(cmd)) return  ;
    if(contains_atomic_log(cmd)) return ;
    if(st -> log_count > 0 && !strcmp(st -> log[st -> log_count - 1], cmd)) return ;
    if(st -> log_count == LOG_SIZE) {
        for(int i = 0 ; i + 1 < LOG_SIZE ; i++) {
            strcpy(st -> log[i], st -> log[i + 1]) ;
        }
        st -> log_count-- ;
    }
    strcpy(st -> log[st -> log_count++], cmd) ;
    history_save(st) ;
}