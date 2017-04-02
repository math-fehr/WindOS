#include "string.h"

char *strcpy(char* restrict dest, const char* restrict src) {
    /*if we would return directly dest, the result would be incorrect
      (like the size when dest is a string with constant size) */
    char *temp = dest;
    while((*dest++ = *src++))
        ;
    return temp;
}


char *strncpy(char* restrict dest, const char* restrict src, const size_t count) {
    /*if we would return directly dest, the result would be incorrect
      (like the size when dest is a string with constant size) */
    char *temp = dest;
    if(!count) {
        return temp;
    }
    size_t i;
    while((*dest++ = *src++) && ++i != count)
        ;

    for(size_t j = i; j < count; j++) {
        *dest++ = '\0';
    }

    return temp;
}


char *strcat(char* restrict dest, const char* restrict src) {
    /*if we would return directly dest, the result would be incorrect
      (like the size when dest is a string with constant size) */
    char *temp = dest;
    if(*dest != '\0') {
        while((*(++dest) != '\0'))
            ;
    }
    strcpy(dest,src);
    return temp;
}


char *strncat(char* restrict dest, const char* restrict src, const size_t count) {
    /*if we would return directly dest, the result would be incorrect
      (like the size when dest is a string with constant size) */
    char *temp = dest;
    while((*(dest++) != '\0'))
        ;
    strncpy(--dest, src, count);
    return temp;
}


size_t strxfrm(char* restrict dest, const char* restrict src, const size_t count) {
    size_t size_src = strlen(src);
    if(count > size_src) {
        strcpy(dest,src);
    }
    else {
        strncpy(dest,src,count);
    }
    return size_src;
}


size_t strlen(const char* restrict str) {
    size_t i;
    for(i = 0; str[i] != '\0'; i++)
        ;
    return i;
}


int strcmp(const char* restrict str1, const char* restrict str2) {
    while(*str1 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (int)((unsigned char)(*str1))-(int)((unsigned char)(*str2));
}


int strncmp(const char* restrict str1, const char* restrict str2, const size_t count) {
    for(size_t i = 0; i<count; i++) {
        if(!*str1 || *str1 != *str2) {
            return (int)((unsigned char)(*str1))-(int)((unsigned char)(*str2));
        }
        str1++;
        str2++;
    }
    return 0;
}


int strcoll(const char* restrict str1, const char* restrict str2) {
    size_t s1 = strxfrm(NULL,str1,0)+1; //+1 is for the '\0' character
    size_t s2 = strxfrm(NULL,str2,0)+1;
    char str1_local[s1];
    char str2_local[s2];
    strxfrm(str1_local,str1,s1);
    strxfrm(str2_local,str2,s2);
    return strcmp(str1_local,str2_local);
}


char* strchr(const char* restrict str, int ch) {
    while(*str != (const char)ch){
        if(!*(str++)){
            return NULL;
        }
    }
    return (char*)str;
}


char* strrchr(const char* restrict str, int ch) {
    const char* ret = NULL;
    do {
        if(*str == (char)ch){
            ret = str;
        }
    } while(*str++);
    return (char*)ret;
}


size_t strspn(const char* restrict dest, const char* restrict characters) {
    size_t ret = 0;
    while(*dest && strchr(characters,*dest)){
        dest++;
        ret++;
    }
    return ret;
}

size_t strcspn(const char* restrict dest, const char* restrict characters) {
    size_t ret = 0;
    while(*dest && !strchr(characters,*dest)){
        dest++;
        ret++;
    }
    return ret;
}


char* strpbrk(const char* restrict dest, const char* restrict breakset) {
    while(*dest) {
        if(strchr(breakset,*(dest++))) {
            return (char*)--dest;
        }
    }
    return NULL;
}


char* strstr(const char* restrict str, const char* restrict substr) {
    size_t size_substr = strlen(substr);
    while(*str) {
        if(!memcmp(str++,substr,size_substr)) {
            return (char*)--str;
        }
    }
    return NULL;
}


char* strtok(char* str, const char* restrict delim) {
    static char* str_p = NULL;
    if(str) {
        str_p = str;
    }
    else if(!str_p) {
        return NULL;
    }
    str = str_p + strspn(str_p,delim);
    str_p = str + strcspn(str,delim);
    if(str_p == str) {
        return str_p = NULL;
    }
    if(*str_p) {
        *str_p = 0;
        str_p++;
    }
    else {
        str_p = NULL;
    }
    return str;
}


void* memchr(const void* ptr, int c, size_t count) {
    if(!ptr) {
        return NULL;
    }
    const char* restrict str = ptr;
    for(size_t i = 0; i<count; i++){
        if(*(str++) == c){
            str--;
            return (void*)str;
        }
    }
    return NULL;
}


int memcmp(const void* array1, const void* array2, size_t count) {
    const char* restrict str1 = array1;
    const char* restrict str2 = array2;
    for(size_t i = 0; i<count; i++) {
        if(*(str1++) != *(str2++)) {
            return (int)((unsigned char)(*(--str1))) - (int)((unsigned char)(*(--str2)));
        }
    }
    return 0;
}


void* memset(void* b, int c, size_t count) {
    char* restrict str = b;
    while(count--) {
        *(str++) = (char)(c);
    }
    return b;
}


void* memcpy(void* restrict dest, const void* restrict src, size_t count) {
    char* restrict dest_str = dest;
    const char* restrict src_str = src;
    while(count--) {
        *(dest_str++) = *(src_str++);
    }
    return dest;
}


void* memmove(void *dest, const void *src, size_t count)
{
    unsigned char *dest_str = dest;
    const unsigned char *src_str = src;

    if(src_str < dest_str) {
        for (dest_str += count, src_str += count; count--;) {
            *(--dest_str) = *(--src_str);
        }
    }
    else {
        while(count--) {
            *(dest_str++) = *(src_str++);
        }
    }
    return dest;
}


char* strerror(const int errnum) {
    if(!errnum) {
        return "There is no error";
    }
    //TODO implement more descriptive messages
    return "There is an error";
}
