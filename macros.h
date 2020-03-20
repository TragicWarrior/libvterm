#ifndef _MACROS_H_
#define _MACROS_H_

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define VAR_UNUSED(x)       ((void) x)

#define ALLBITS(x)          (~(x & 0))

#define POW2(x)             ((x) * (x))

// speedy way to get the absolute value of on a 2s-complement system
#define ABSINT(x) \
    ((x ^ ( x>> ((sizeof(x) * 8) - 1))) - (x >> ((sizeof(x) * 8) - 1)))


// calculate the number of elements in a static array (scalar)
#define ARRAY_SZ(x)    (sizeof(x) / sizeof(x[0]))

#define CALLOC_PTR(_ptr) \
        ({ \
            __typeof__ (*_ptr) _tmp; \
            (_ptr) = calloc(1, sizeof(_tmp)); \
        })

#define SWITCH(jtable, idx, catchall) \
            do \
            { \
                if(idx >= ARRAY_SZ(jtable)) \
                    goto *jtable[catchall]; \
            \
                if(jtable[idx] == 0) \
                    goto *jtable[catchall]; \
            \
                goto *jtable[idx]; \
            } \
            while(0)

#define DEG2RAD(angle)  ((angle) * (M_PI / 180.0))
#define RAD2DEG(angle)  ((angle) * (180.0 / M_PI))

#ifndef MIN_VAL
#define MIN_VAL(a, b) \
            ({ \
                __typeof__ (a) _a = (a); \
                __typeof__ (b) _b = (b); \
                _a < _b ? _a : _b; \
            })
#endif

#ifndef MAX_VAL
#define MAX_VAL(a, b) \
            ({ \
                __typeof__ (a) _a = (a); \
                __typeof__ (b) _b = (b); \
                _a > _b ? _a : _b; \
            })
#endif

#endif
