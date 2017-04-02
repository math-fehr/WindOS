#include "ctype.h"


int isalnum(int const c) {
    return ((c >= (int)('a') && c <= (int)('z')) ||
            (c >= (int)('A') && c <= (int)('Z')) ||
            (c >= (int)('0') && c <= (int)('9')));
}


int isalpha(int const c) {
    return ((c >= (int)('a') && c <= (int)('z')) ||
            (c >= (int)('A') && c <= (int)('Z')));
}


int isdigit(int const c) {
    return (c >= (int)('0') && c <= (int)('9'));
}


int isxdigit(int const c) {
    return ((c >= (int)('a') && c <= (int)('f')) ||
            (c >= (int)('A') && c <= (int)('F')) ||
            (c >= (int)('0') && c <= (int)('9')));
}


int islower(int const c) {
    return (c >= (int)('a') && c <= (int)('z'));
}


int isupper(int const c) {
    return (c >= (int)('A') && c <= (int)('Z'));
}


int isprint(int const c) {
    return (c >= 0x20 && c <= 0x7E);
}


int isgraph(int const c) {
    return (c >= 0x21 && c <= 0x7E);
}


int ispunct(int const c) {
    return (isprint(c) && !isspace(c) && !isalnum(c));
}


int isspace(int const c) {
    return ((c == ' ') ||
            (c == '\t') ||
            (c == '\r') ||
            (c == '\n') ||
            (c == '\v') ||
            (c == '\f'));
}


int iscntrl(int const c) {
    return (((c <= 0x1F) && (c >= 0x00)) ||
            (c == 0x7F));
}


int tolower(int const c) {
    if((c >= 'A') && (c <= 'Z')) {
        return c + 32;
    }
    return c;
}


int toupper(int const c) {
    if((c >= 'a') && (c <= 'z')) {
        return c - 32;
    }
    return c;
}
