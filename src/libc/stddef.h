#ifndef __LIBC_STDDEF_H
#define __LIBC_STDDEF_H


#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef unsigned long int size_t; //type returned by the sizeof operator
#endif  //__SIZE_T_DEFINED


#ifndef __PTRDIFF_T_DEFINED
#define __PTRDIFF_T_DEFINED
typedef long int ptrdiff_t; //the type of the difference of two pointers
#endif  //__PTRDIFF_T_DEFINED


#ifndef __WCHAR_T_DEFINED
#define __WCHAR_T_DEFINED
typedef unsigned int wchar_t; //the type of the extended characters
#endif  //__WCHAR_T_DEFINED


#ifndef NULL
#define NULL 0  //The null pointer
#endif  //NULL


//Give the offset of a member in a struct or a union
#define offsetof(st, m) __builtin_offsetof(st, m)


#endif  //__LIBC_STDDEF_H

