#include "../include/runner.h"


#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

int my_strcmp(const char* str1, const char* str2) {
    // printf("%s\n" , str1 ) ;
    // printf("%s" , str2) ;
    if(!str1 || !str2) return 1 ;

    while(*str1 && (*str1 == *str2)) {
        str1++ , str2++ ;
    }
    return (*str1 == '\0' && *str2 == '\0') ? 0 : 1;
}
size_t my_strncmp(const char* str1, const char* str2, size_t n) {

    if(!str1 || !str2) return 0 ;

    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i]) {
            return 1;   
        }
    }
    return 0 ;
}


size_t my_strlen(const char* str) {
    size_t len = 0;

    while (*str != '\0') {
        len++, str++;
    }
    return len;
}

char* my_getenv(const char* name, char** env) {

    if(!name || !env) {
        return NULL ;
    }
    size_t name_len = my_strlen(name) ;
    
    for(size_t i = 0 ; env[i] ; i++) {
        if(!my_strncmp(env[i] , name, name_len) && env[i][name_len] == '=') {
            return &env[i][name_len + 1] ;
        }
    }
    return NULL ;
}

char* my_strcpy(char* dest, const char* src) {

    if(!src) return NULL ;

    char* res = dest ;

    while(*src) {
        *dest = *src ;
        dest++ , src++ ;
    }
    *dest = '\0' ;
    return res ;
}

char* my_strdup (const char* str) {

    if(!str) return NULL ;
        
    size_t len = my_strlen(str) ;
    char* duplicated = (char*) malloc((len + 1) * sizeof(char)) ;

    if(!duplicated) return NULL ;

    my_strcpy(duplicated, str) ;
    
    return duplicated ;
} 

char* my_strncpy(char* dst, char* src, size_t n) {
    size_t i ;

    for(i = 0 ; i < n && src[i] != '\0'; i++) {
        dst[i] = src[i] ;
    }
    while(i < n) dst[i++] = '\0' ;

    return dst ;  
} 

// char** my_strtok(const char* str, const char* delimeter ) {
    
   
// }
