#ifndef LIBC_CTYPE_H
#define LIBC_CTYPE_H

/**
 * This is the modules that contains the is... and to... functions,
 * which deals with characters
 */


/**
 * Return a non-null value if the given character is an alphanumeric value
 * (a letter (A to Z or a to Z) or a digit (0 to 9))
 */
int isalnum(int const c);


/**
 * Return a non-null value if the given character is a letter
 * (A to Z or a to Z)
 */
int isalpha(int const c);


/**
 * Return a non-null value if the given character is a decimal digit
 */
int isdigit(int const c);


/**
 * Return a non-null value if the given character is a hexadecimal digit
 * (0 to 9, A to F or a to f)
 */
int isxdigit(int const c);


/**
 * Return a non-null value if the given character is a lowercase letter
 */
int islower(int const c);


/**
 * Return a non-null value if the given character is an uppercase letter
 */
int isupper(int const c);


/**
 * Return a non-null value if the given character is any printing character
 */
int isprint(int const c);


/**
 * Return a non-null value if the given character is any printing character
 * except for space character
 */
int isgraph(int const c);


/**
 * Return a non-null value if the given character is any printing character
 * except for space character or ialnum
 */
int ispunct(int const c);


/**
 * Return a non-null value if the given character is a whitespace character
 */
int isspace(int const c);


/**
 * Return a non-null value if the given character is a control character
 * (0x00 to 0x1F or 0x7F)
 */
int iscntrl(int const c);


/**
 * Return the lowercase letter, if it is an uppercase letter
 */
int tolower(int const c);


/**
 * Return the uppercase letter, if it is a lowercase letter
 */
int toupper(int const c);


#endif
