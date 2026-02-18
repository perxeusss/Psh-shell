#ifndef HISTORY_H 
#define HISTORY_H 

#include "./shell.h" 

void history_add_if_needed(shell_state *st, const char *cmd) ;
void history_save(shell_state *st) ;
void history_load(shell_state *st) ;


#endif 