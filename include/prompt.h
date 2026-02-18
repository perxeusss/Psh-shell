#ifndef PROMPT_H
#define PROMPT_H

#include "./shell.h"

#define PATH_MAX 4096

void init_prompt(shell_state *st) ;
void show_prompt(const shell_state *st) ;

#endif 