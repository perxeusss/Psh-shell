#include "../include/zash.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>


void shell_loop() {

    char *line = NULL;
    int cap = 0 ;

    for(;;) {

        printf("Psh >") ;
        fflush(stdout) ;

        ssize_t n = getline(&line, &cap, stdin);

        if (n < 0) {
            printf("logout\n");
            break;
        }

        while(n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) {
            line[--n] = '\0';
        }

        if(line[0] == '\0') continue ;

        char norm[2048];
        norm_and_and(line, norm, sizeof(norm)) ;
       
    }
}

int main() {

    shell_loop();

    return 0;
}
