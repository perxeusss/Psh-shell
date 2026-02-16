#ifndef BUILTINS_H
#define BUILTINS_H

int command_cd(char **args);
int command_pwd(void);
int command_echo(char **args, char **env);
int command_env(char **env);
int command_which(char **args, char **env);
int command_setenv(char **args);
int command_unsetenv(char **args);

#endif
