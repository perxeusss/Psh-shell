#include "../include/main.h"
#include "../include/runner.h"
#include "../include/parser.h"
#include "../include/signals.h"
#include "../include/jobs.h"
#include "../include/prompt.h"  
#include "../include/shell.h"


#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

extern shell_state global_shell_state ;


void shell_loop() {

    char *line = NULL;
    size_t cap = 0 ;

    for(;;) {

        printf("Psh >") ;
        fflush(stdout) ;

        ssize_t n = getline(&line, &cap, stdin);

        if (n < 0) {
            printf("logout\n");
            break;
        }

        while(n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }

        if(line[0] == '\0') continue ;

        char norm[2048];
        norm_and_and(line, norm, sizeof(norm)) ;    
        
        if(!parse_shell_cmd(norm)) {
            fprintf(stderr, "Syntax error\n");
            continue ;
        } 
        run_sequence(norm) ;
    }
    free(line) ;
}

int main() {
    init_prompt(&global_shell_state);

    global_shell_state.prev[0] = '\0';
    global_shell_state.log_count = 0;

    jobs_intit(&global_shell_state);
    signals_init() ;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    shell_loop();

    return 0;
}
