#ifndef SIGNALS_H 
#define SIGNALS_H

#include <signal.h>

void signals_init() ;
void signals_set_fg_pgid(pid_t pgid, const char *cmd) ;
pid_t signals_get_fg_pgid() ;   

#endif 