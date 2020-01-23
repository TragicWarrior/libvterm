#ifndef _MACROS_H_
#define _MACROS_H_

#define VAR_UNUSED(x)       ((void) x)

// speedy way to get the absolute value of on a 2s-complement system
#define ABSINT(x)	((x^(x>>((sizeof(x)*8)-1)))-(x>>((sizeof(x)*8)-1)))

#define ALLBITS(x)  (~(x & 0))

// calculate the number of elements in a static array (scalar)
#define ARRAY_SZ(x)    (sizeof(x) / sizeof(x[0]))

#define CALLOC_PTR(_ptr)                        \
        ({                                      \
            __typeof__ (*_ptr) _tmp;            \
            (_ptr) = calloc(1, sizeof(_tmp));   \
        })

#define SWITCH(jtable, idx, catchall)           \
            do                                  \
            {                                   \
                if(idx >= ARRAY_SZ(jtable))     \
                    goto *jtable[catchall];     \
                                                \
                if(jtable[idx] == 0)            \
                    goto *jtable[catchall];     \
                                                \
                goto *jtable[idx];              \
            }                                   \
            while(0)

#define USE_MIN(a, b)                           \
            ({                                  \
                __typeof__ (a) _a = (a);        \
                __typeof__ (b) _b = (b);        \
                _a < _b ? _a : _b;              \
            })

#endif
