#ifndef LIBC_STRING_H
#define LIBC_STRING_H

/**
 * This is the string module which contains the functions that deals with srings (char*)
 * There is also functions that deals with raw data (functions starting with mem)
 */

//type returned by the sizeof operator
typedef unsigned int size_t;


/**
 * Copies the content of string src in string dest, and return the copied string
 * The resulting string is null-terminated
 * If the strings overlap, the behavior is undefined
 * If the destination string is not large enough, the behavior is undefined
 */
char *strcpy(char* restrict dest, const char* restrict src);


/**
 * Copies at most count characters from string count to string src, and return the new string
 * If count is reached before the entire string was copied,
 * the resulting character is not null terminated
 * If src's size is smaller than count, then additionnal null characters are written to dest
 * If the string overlap, the behavior is undefined
 */
char *strncpy(char* restrict dest, const char* restrict src, const size_t count);


/**
 * Append a copy of the string src to the end of the string dest, and return the new string
 * The returned string is null terminated
 * If the strings overlap, the behavior is undefined
 * If the destination string is not large enough, the behavior is undefined
 */
char *strcat(char* restrict dest, const char* restrict src);


/**
 * Append a copy of the string src to the end of the string dest, and return the new string
 * If count is reached before the entire string was copied,
 * the resulting character is not null terminated
 * If the strings overlap, the behavior is undefined
 * If the destination string is not large enough, the behavior is undefined
 */
char *strncat(char* restrict dest, const char* restrict src, const size_t count);


/**
 * The function copy and transform the string pointed by src to the destination dest,
 * such that the resulting string will react the same way to strcoll() and strcmp().
 * No more than count characters are copied
 * If count is equal to 0, dest can be the null pointer
 * The function return the length of the transformed string (not including the null character)
 * If the return value is bigger than count, the contents pointed by dest are undetermined
 * If the strings overlap, the behavior is undefined
 * If the destination string is not large enough, the behavior is undefined
 */
size_t strxfrm(char* restrict dest, const char* restrict src, const size_t count);


/**
 * Returns the length of the given string (not including the null character)
 * The behavior is undefined if there is no null character
 */
size_t strlen(const char* restrict str);


/**
 * Compare two null-terminated strings lexicographically
 * The sign of the result is the sign of the difference between
 * the values of the first pair of characters that differ in the strings
 * The behavior is undefined if the string are not null-terminated
 */
int strcmp(const char* restrict str1, const char* restrict str2);


/**
 * Compare at most count characters of two null-terminated strings lexicographically
 * Characters following the null character are not compared
 * The sign of the result is the sign of the difference between
 * the values of the first pair of characters that differ in the strings
 * The behavior is undefined if the string are not null-terminated
 */
int strncmp(const char* restrict str1, const char* restrict str2, const size_t count);


/**
 * Compare two null-terminated strings according to the current locale
 * The sign of the result is the sign of the difference between
 * the values of the first pair of characters that differ in the strings
 * The behavior is undefined if the string are not null-terminated
 */
int strcoll(const char* restrict str1, const char* restrict str2);


/**
 * Return a pointer to the first occurence of (char)(c) in the string
 * Return null if the character is not part of the string
 * The null character is considered to be a part of the string
 * The behavior is undefined if the string are not null-terminated
 */
char* strchr(const char* restrict str, int ch);


/**
 * Return a pointer to the last occurence of (char)(c) in the string
 * Return null if the character is not part of the string
 * The null character is considered to be a part of the string
 * The behavior is undefined if the string are not null-terminated
 */
char* strrchr(const char* restrict str, int ch);


/**
 * Return the length of the maximum prefix of dest that contains only
 * characters from the string characters
 * The behavior is undefined if the string are not null-terminated
 */
size_t strspn(const char* restrict dest, const char* restrict characters);


/**
 * Return the length of the maximum prefix of dest that contains only
 * characters that are not from the string characters
 * The behavior is undefined if the string are not null-terminated
 */
size_t strcspn(const char* restrict dest, const char* restrict characters);


/**
 * Return a pointer to the first character contained in the string breakset
 * Return null if there is no such character
 * The behavior is undefined if the string are not null-terminated
 */
char* strpbrk(const char* restrict dest, const char* restrict breakset);


/**
 * Return the first occurence of a substring of characters
 * The null characters are not compared
 * Return null pointer if no such substring exists
 * The behavior is undefined if the string are not null-terminated
 */
char* strstr(const char* restrict str, const char* restrict substr);


/**
 * Finds the next token in a string str.
 * The separator characters are the characters of the string delim
 * This function change when called multiple times
 * If str is the null pointer, then the function will use the new str from the last call
 * If no token is found, then the function return a null pointer
 * The behavior is undefined if the string are not null-terminated
 */
char* strtok(char* str, const char* restrict delim);


/**
 * The function locates the first occurence of c (converted to an unsigned char)
 * in the initial count characters of the object pointed by ptr
 * Return a pointer to the character, or the null pointer if the character was not found
 * Return the null pointer if ptr is the null pointer
 * The behavior is undefined if count is bigger than the size of the ptr array
 */
void* memchr(const void* ptr, int c, const size_t count);


/**
 * Reinterprets two objects in unsigned char array, and copare the first count characters
 * The comparison is done lexicographically
 * The return has the same sign as the difference of the two first different characters
 * The behavior is undefined if count is bigger than the size of the arrays
 */
int memcmp(const void* array1, const void* array2, const size_t count);


/**
 * Fills a buffer b with count characters (unsigned char)c
 * Return a pointer to the filled buffer
 * the behavior is undefined if count is bigger than the size of the buffer
 */
void* memset(void* b, int c, size_t count);


/**
 * Copy the first count bytes from src to dest
 * Return a pointer to the copied buffer
 * If dest or src are smaller than n bytes, the behavior is undefined
 * Source and destination may not overlap, otherwise behavior is undefined
 */
void* memcpy(void* restrict dest, const void* restrict src, size_t count);


/**
 * Copy the first count bytes from src to dest
 * Return a pointer to the copied buffer
 * If dest or src are smaller than n bytes, the behavior is undefined
 * Source and destination may overlap
 */
void* memmove(void* dest, const void* src, size_t count);


/**
 * Maps the number in errnum to a message string
 * errnum comes usually from errno
 * The return value shall not be modified
 */
char* strerror(const int errnum);

#endif
