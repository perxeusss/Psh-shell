#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "../include/helpers.h"
#include "../include/runner.h"
#include "../include/builtins.h"    

extern char **environ;
static char *prev_dir = NULL;


int command_cd(char **args) {

    char *oldcwd = getcwd(NULL, 0);
    if (!oldcwd) {
        perror("getcwd");
        return 1;
    }

    const char *target = NULL;

    if (!args[1] || strcmp(args[1], "~") == 0) {

        target = getenv("HOME");

        if (!target) {
            fprintf(stderr, "cd: HOME not set\n");
            free(oldcwd);
            return 1;
        }
    }
    else if (strcmp(args[1], "-") == 0) {

        if (!prev_dir) {
            fprintf(stderr, "cd: no previous directory\n");
            free(oldcwd);
            return 1;
        }

        target = prev_dir;
    }
    else {
        target = args[1];
    }
    if (chdir(target) != 0) {
        perror("cd");
        free(oldcwd);
        return 1;
    }

    free(prev_dir);
    prev_dir = oldcwd;

    if (args[1] && strcmp(args[1], "-") == 0) {

        char *now = getcwd(NULL, 0);
        if (now) {
            printf("%s\n", now);
            free(now);
        }
    }

    return 0;
}

int command_pwd(void) {

    char *cwd = getcwd(NULL, 0);

    if (!cwd) {
        perror("getcwd");
        return 1;
    }
    puts(cwd);
    free(cwd);

    return 0;
}

int command_echo(char **args, char **env) {

    int newline = 1;
    size_t i = 1;

    while (args[i] && strcmp(args[i], "-n") == 0) {
        newline = 0;
        i++;
    }

    for (; args[i]; i++) {
        fputs(args[i], stdout);
        if (args[i + 1]) putchar(' ');
    }
    if (newline)
        putchar('\n');

    return 0;
}

int command_env(char **env) {

    char **walker = env ? env : environ;

    for (size_t i = 0; walker[i]; i++) {
        puts(walker[i]);
    }
    return 0;
}

static char *find_in_path(const char *cmd, char **env) {

    char *path = env ?
        my_getenv("PATH", env) :
        getenv("PATH");

    if (!path) return NULL;

    char *dup = strdup(path);
    if (!dup) {
        perror("strdup");
        return NULL;
    }

    char buf[1024];
    char *tok = strtok(dup, ":");

    while (tok) {
        snprintf(buf, sizeof(buf), "%s/%s", tok, cmd);

        if (access(buf, X_OK) == 0) {
            char *res = strdup(buf);
            free(dup);
            return res;
        }

        tok = strtok(NULL, ":");
    }
    free(dup);
    return NULL;
}

int command_which(char **args, char **env) {

    if (!args[1]) {
        fprintf(stderr, "which: expected argument\n");
        return 1;
    }

    const char *builtins[] = {
        "cd","pwd","echo","env",
        "setenv","unsetenv","which","exit"
    };

    size_t n = sizeof(builtins)/sizeof(builtins[0]);

    for (size_t i = 0; i < n; i++) {
        if (strcmp(args[1], builtins[i]) == 0) {
            printf("%s: shell built-in command\n", args[1]);
            return 0;
        }
    }

    char *res = find_in_path(args[1], env);

    if (!res) {
        printf("%s not found\n", args[1]);
        return 1;
    }

    printf("%s\n", res);
    free(res);

    return 0;
}


int command_setenv(char **args) {

    if (!args[1]) {
        fprintf(stderr,
            "Usage: setenv VAR=value | setenv VAR value\n");
        return 1;
    }
    char *eq = strchr(args[1], '=');

    if (eq) {
        char name[256];
        size_t n = eq - args[1];

        if (n == 0 || n >= sizeof(name)) {
            fprintf(stderr, "setenv: invalid name\n");
            return 1;
        }
        memcpy(name, args[1], n);
        name[n] = '\0';

        if (setenv(name, eq + 1, 1) != 0) {
            perror("setenv");
            return 1;
        }
    }
    else if (args[2]) {

        if (setenv(args[1], args[2], 1) != 0) {
            perror("setenv");
            return 1;
        }
    }
    else {
        fprintf(stderr,
            "Usage: setenv VAR=value | setenv VAR value\n");
        return 1;
    }
    return 0;
}

int command_unsetenv(char **args) {

    if (!args[1]) {
        fprintf(stderr, "Usage: unsetenv VAR\n");
        return 1;
    }
    if (unsetenv(args[1]) != 0) {
        perror("unsetenv");
        return 1;
    }
    return 0;
}
