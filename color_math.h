#ifndef _COLOR_MATH_H_
#define _COLOR_MATH_H_

#include <math.h>


#define RGB_FLOAT(_r, _g, _b)   ((rgb_t) { _r, _g, _b })
#define RGB_R(_rgb)             (_rgb[0])
#define RGB_G(_rgb)             (_rgb[1])
#define RGB_B(_rgb)             (_rgb[2])


typedef float                   rgb_t[3];

// converts RGB values to CIELAB2000 values
void    rgb2lab(int r, int g, int b, float *L, float *A, float *B);

// converts RGB values to Hue, Luminosity, Saturation values
void    rgb2hsl(int r, int g, int b, float *h, float *l, float *s);

// converts HSL values to Red, Green, Blue values
void    hsl2rgb(float h, float s, float l, int *r, int *g, int *b);

// compute the distance between to CIELAB colors using 1976 method
float   cie76_delta(float l1, float a1, float b1, float l2, float a2, float b2);



#endif
