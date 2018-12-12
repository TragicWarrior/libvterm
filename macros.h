#ifndef _MACROS_H_
#define _MACROS_H_

// speedy way to get the absolute value of on a 2s-complement system
#define  ABSINT(x)	((x^(x>>((sizeof(x)*8)-1)))-(x>>((sizeof(x)*8)-1)))

// calculate the number of elements in a static array (scalar)
#define  ARRAY_SZ(x)    (sizeof(x) / sizeof(x[0]))

#endif
