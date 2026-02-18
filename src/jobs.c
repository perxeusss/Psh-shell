
#include "../include/posix_lib.h"
#include "../include/shell.h" 
#include "../include/jobs.h"

#include<stdio.h>
#include <sys/wait.h>



void jobs_init(shell_state *st) {
    st -> next_job_id = 1 ;
    for(int i = 0 ; i < MAX_JOBS ; i++) {
        st -> jobs[i].active = 0 ;
    }
}

int jobs_add(shell_state *st, pid_t pid, char *cmd) {
    if(st -> next_job_id >= MAX_JOBS) {
        fprintf(stderr, "Maximum number of background jobs reached.\n") ;
        return -1 ;
    }
    for(int i = 0 ; i < MAX_JOBS ; i++) {
        if(st -> jobs[i].active) continue ;

        st -> jobs[i].active = 1 ;
        st -> jobs[i].pid = pid ;
        st -> jobs[i].state = JOB_RUNNING ;
        strncpy(st -> jobs[i].cmd, cmd, sizeof(st->jobs[i].cmd) -1);
        st -> jobs[i].cmd[sizeof(st->jobs[i].cmd) - 1] = '\0' ;
        st -> jobs[i].id = st -> next_job_id++ ;        
        return st -> jobs[i].id ;
    }
    return -1 ;
}

void jobs_check(shell_state *st) {
    int status ;
    pid_t rpid ;

    while((rpid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        int idx = -1 ;
        for(int i = 0 ; i < MAX_JOBS ; i++) {
            if(st -> jobs[i].active && st -> jobs[i].pid == rpid) {
                idx = i ; break ;
            }
        }
        if(idx == -1) continue ;
        if(WIFSTOPPED(status)) {
            st -> jobs[idx].state = JOB_STOPPED ; continue ;
        }
        if(WIFCONTINUED(status)) {
            st -> jobs[idx].state = JOB_RUNNING ; continue ;
        }
        if(WIFEXITED(status)) {
            int code = WEXITSTATUS(status) ;

            if(code == 0) {
                printf("%s with pid %d exited normally\n",
                       name_of(st -> jobs[idx].cmd), (int) st -> jobs[idx].pid);
            }
            else {
                printf("%s with pid %d exited abnormally\n",
                       name_of(st -> jobs[idx].cmd), (int)st -> jobs[idx].pid);
            }
            fflush(stdout) ;
            st -> jobs[idx].active = 0 ; continue ;
        }
        // true means the child process was terminated abnormally due to another signal (e.g., SIGKILL, SIGSEGV, etc.)
        if(WIFSIGNALED(status)) {
            printf("%s with pid %d was terminated by a signal\n",
                   name_of(st -> jobs[idx].cmd), (int) st -> jobs[idx].pid);
            fflush(stdout) ;
            st -> jobs[idx].active = 0 ;
        }
    }
}