#ifndef _COLOR_MATH_H_
#define _COLOR_MATH_H_

#include <math.h>

// should be defined math.h but sometimes isn't
#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define DEG2RAD(angle)  ((angle) * (M_PI / 180.0))
#define RAD2DEG(angle)  ((angle) * (180.0 / M_PI))

#ifndef MIN_VAL
#define MIN_VAL(a, b)           ({ \
                                    __typeof__ (a) _a = (a); \
                                    __typeof__ (b) _b = (b); \
                                    _a < _b ? _a : _b; \
                                })
#endif

#ifndef MAX_VAL
#define MAX_VAL(a, b)           ({ \
                                    __typeof__ (a) _a = (a); \
                                    __typeof__ (b) _b = (b); \
                                    _a > _b ? _a : _b; \
                                })
#endif

#define RGB_FLOAT(_r, _g, _b)   ((rgb_t) { _r, _g, _b })
#define RGB_R(_rgb)             (_rgb[0])
#define RGB_G(_rgb)             (_rgb[1])
#define RGB_B(_rgb)             (_rgb[2])


typedef float                   rgb_t[3];

// converts RGB values to CIELAB2000 values
void    rgb2lab(rgb_t rgb, float *l, float *a, float *b);

// converts RGB values to Hue, Luminosity, Saturation values
void    rgb2hsl(rgb_t rgb, float *h, float *l, float *s);

// converts HSL values to Red, Green, Blue values
void    hsl2rgb(float h, float s, float l, float *r, float *g, float *b);

// compute the distance between to CIELAB colors using 1976 method
float   cie76_delta(float l1, float a1, float b1, float l2, float a2, float b2);



#endif
