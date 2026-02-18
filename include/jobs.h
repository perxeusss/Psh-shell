#ifndef JOBS_H
#define JOBS_H 

int jobs_add(shell_state *st, pid_t pid, char *cmd) ;
void jobs_check(shell_state *st) ;
void jobs_init(shell_state *st) ;

#endif 