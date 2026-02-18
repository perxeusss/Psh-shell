#include "../include/posix_lib.h"
#include "../include/signals.h"
#include "../include/shell.h"
#include "../include/jobs.h"

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>


static volatile sig_atomic_t fg_pgid = -1 ;
static char fg_cmd[256] ;
extern shell_state global_shell_state ;

static void on_sigint(int sig) {
    pid_t pg = fg_pgid ;

    if(pg > 0) {
        kill(-pg, SIGINT) ;
    }
}

static void on_sigtstp(int sig) {
    pid_t pg = fg_pgid ;

    if(pg > 0) {
        kill(-pg, SIGTSTP) ;
        int id = jobs_add(&global_shell_state, pg, fg_cmd) ;
        if(id > 0) {
           for(int i = 0 ; i < MAX_JOBS ; i++) {
               if(global_shell_state.jobs[i].active && global_shell_state.jobs[i].id == id) {
                    global_shell_state.jobs[i].state = JOB_STOPPED ;
                    global_shell_state.jobs[i].pid = pg ;
                    break ;
                }
            }
            printf("[%d] Stopped %s with pid %d\n", id, fg_cmd, (int) pg) ;
            fflush(stdout) ;
        }
    }
}

void signals_init() {
    struct sigaction sa_int, sa_ststp ;

    memset(&sa_int, 0, sizeof(sa_int)) ;
    memset(&sa_ststp, 0, sizeof(sa_ststp)) ;

    sa_int.sa_handler = on_sigint ;
    sa_ststp.sa_handler = on_sigtstp ;
    
    sigemptyset(&sa_int.sa_mask) ;
    sigemptyset(&sa_ststp.sa_mask) ;

    sa_int.sa_flags = SA_RESTART ;
    sa_ststp.sa_flags = SA_RESTART ;

    sigaction(SIGINT, &sa_int, NULL) ;
    sigaction(SIGTSTP, &sa_ststp, NULL) ;

    signal(SIGQUIT, SIG_IGN);
}

void signals_set_fg_pgid(pid_t pgid, const char *cmd) {
    fg_pgid = pgid ;
    strncpy(fg_cmd, cmd, sizeof(fg_cmd) - 1) ;
    fg_cmd[sizeof(fg_cmd) - 1] = '\0' ;
}

pid_t signals_get_fg_pgid(void) { return fg_pgid; }
const char *signals_get_fg_cmd(void) { return fg_cmd; }