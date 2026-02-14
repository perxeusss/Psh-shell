#include<string.h>
#include<stdio.h>


norm_and_and(const char *in, char *out, size_t sz){

    size_t j = 0;
    for(size_t i = 0; in[i] && j + 1 < sz; i++, j++) {
        if(in[i] == '&' && in[i + 1] == '&') {
            out[j] = ';', i++ ;
        } else {
            out[j] = in[i];
        }
    }
    out[j] = '\0';
}