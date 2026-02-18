#include "../include/main.h"
#include "../include/runner.h"
#include "../include/parser.h"
#include "../include/signals.h"
#include "../include/jobs.h"
#include "../include/prompt.h"  
#include "../include/shell.h"
#include "../include/history.h"


#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

shell_state global_shell_state ;

static void kill_all(shell_state *st) {
    pid_t g = signals_get_fg_pgid();
    if (g > 0) kill(-g, SIGKILL);
    for (int i = 0; i < MAX_JOBS; i++) {
        if (st->jobs[i].active) {
            kill(-st->jobs[i].pgid, SIGKILL);
        }
    }
}

void shell_loop() {

    char *line = NULL;
    size_t cap = 0 ;

    for(;;) {
        jobs_check(&global_shell_state) ;
        show_prompt(&global_shell_state);
        fflush(stdout) ;

        ssize_t n = getline(&line, &cap, stdin);

        if (n < 0) {
            printf("logout\n");
            kill_all(&global_shell_state);
            break;
        }

        while(n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }

        jobs_check(&global_shell_state) ;
        if(line[0] == '\0') continue ;

        history_add_if_needed(&global_shell_state, line) ;

        char norm[2048];
        norm_and_and(line, norm, sizeof(norm)) ;    
        
        if(!parse_shell_cmd(norm)) {
            fprintf(stderr, "Syntax error\n");
            continue ;
        } 
        run_sequence(norm) ;
        jobs_check(&global_shell_state) ;
    }
    free(line) ;
}

int main() {
    init_prompt(&global_shell_state);

    global_shell_state.prev[0] = '\0';
    global_shell_state.log_count = 0;

    jobs_init(&global_shell_state);
    signals_init() ;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);

    shell_loop();

    return 0;
}
