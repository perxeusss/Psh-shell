#ifndef EXECUTE_H 
#define EXECUTE_H

#include <unistd.h>
#include <sys/types.h>

int execute_command(char *line, int wait_fg, pid_t *first_pid) ;

#endif  