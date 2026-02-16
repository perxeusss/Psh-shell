#ifndef HELPERS_H
#define HELPERS_H

#include<unistd.h> 


char* my_getenv(const char* name, char** env) ;
size_t my_strlen(const char* str) ;
size_t my_strncmp(const char* str1, const char* str2, size_t n) ;
char* my_strcpy(char* dest, const char* src) ;  
int my_strcmp(const char* str1, const char* str2) ;
char* my_strncpy(char* dest, const char* src, size_t n) ;

#endif