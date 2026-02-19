#ifndef INPUT_H
#define INPUT_H

#include "shell.h"

void input_enable_raw(void);
void input_disable_raw(void);
char *input_read_line(shell_state *st);

#endif