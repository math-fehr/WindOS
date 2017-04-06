#ifndef __LIBC_STDLIB_H
#define __LIBC_STDLIB_H

#include <stddef.h>
#include <errno.h>

#ifndef NULL
#define NULL 0   //The value of a null pointer
#endif  //NULL


#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1   //The value to return when the program fail
#endif  //EXIT_FAILURE


#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 1   //The value to return when
                         //the program terminate normally
#endif  //EXIT_SUCCESS


//TODO Change that value when rand will be defined
#ifndef RAND_MAX
#define RAND_MAX 42  //The maximum value that can be returned from rand
#endif  //RAND_MAX


//TODO Change that when we will have defined the extended characters
#ifndef MB_CUR_MAX
#define MB_CUR_MAX 42
#endif  //MB_CUR_MAX


/**
 * Represent a fraction
 */
typedef struct {
    int quot;
    int rem;
} div_t;


/**
 * Represent a fraction with more precision
 */
typedef struct {
   long int quot;
   long int rem;
} ldiv_t;


/**
 * Represent a fraction with even more precision
 */

typedef struct {
    long long quot;
    long long rem;
} lldiv_t;



/**
 * The function convert the initial portion of the string to a double
 * whitespaces before the first portion are skipped
 * The conversion stop to the first unrecognized character
 * If the value is out of range of the type double, HUGE_VAL is returned and
 * ERANGE is stored in errno
 * IF the value is too small, 0 is returned and ERANGE is stored in errno
 */
double atof(const char* nptr);


/**
 * The function convert the initial portion of the string to an int
 * whitespaces before the first portion are skipped
 * The conversion stop to the first unrecognized character
 * On error, 0 is returned
 */
int atoi(const char* str);


/**
 * The function convert the initial portion of the string to an long int
 * whitespaces before the first portion are skipped
 * The conversion stop to the first unrecognized character
 * On error, 0 is returned
 */
long int atol(const char* nptr);


/**
 * The function convert the initial portion of the string to a long int
 * whitespaces before the first portion are skipped
 * The conversion stop to the first unrecognized character
 * On error, 0 is returned
 */
long long int atoll(const char* nptr);



/**
 * The function convert the initial portion of the string to a double
 * whitespaces before the first portion are skipped
 * The conversion stop to the first unrecognized character
 * The adress of the character that stopped the scan is stored in the pointer
 * that endptr points to
 * If endptr is the null pointer, the function will still work
 * If the value is out of range of the type double, HUGE_VAL is returned and
 * ERANGE is stored in errno
 * IF the value is too small, 0 is returned and ERANGE is stored in errno
 */
double strtod(const char *str, char **endptr);


/**
 * The function convert the initial portion of the string to an int
 * whitespaces before the first portion are skipped
 * The string must consist of an optional sign and a string of digits
 * The conversion stop to the first unrecognized character
 * On error, 0 is returned
 * If endptr is the null pointer, the function will not set endptr
 */
long strtol(const char *str, char **endptr, int base);

#endif  //__LIBC_STDLIB_H
