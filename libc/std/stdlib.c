#include "stdlib.h"
#include "ctype.h"
#include <stdbool.h>


double atof(const char* nptr) {
    return 0;
}


int atoi(const char* str) {
    return strtol(str, NULL, 10);
}


long int atol(const char* str) {
    return strtol(str, NULL, 10);
}


long long int atoll(const char* str) {
    return 0;
}


double strtod(const char* str, char** endptr) {
    return 0;
}


long strtol(const char* str, char** endptr, int base) {
    bool isNegative = false;
    if(*str == '-') {
        str++;
        isNegative = true;
    }
    else if(*str == '+') {
        str++;
    }

    if(base == 0){
        if(*str == '0' && (*(str+1) == 'x' || (*(str+1)) == 'X')) {
            base = 16;
            str += 2;
        }
        else if(*str == '0' && *(str+1) <= '7' && *(str+1) >= '0'){
            base = 8;
        }
        else {
            base = 10;
        }
    }
    long value = 0;
    long digit;
    while(true) {
        if(*str <= '9' && *str >= '0') {
            digit = *str - '0';
        }
        else if(*str <= 'z' && *str >= 'a') {
            digit = (long)(*str - 'a') + (long)(10);
        }
        else {
            digit = (long)(*str - 'A') + (long)(10);
        }
        if(digit < base) {
            value *= base;
            if(isNegative) {
                value -= digit;
            }
            else {
                value += digit;
            }
            str++;
        }
        else {
            *endptr = (char*)str;
            return value;
        }
    }
}
