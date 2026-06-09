#ifndef _COLOR_MATH_H_
#define _COLOR_MATH_H_

#include <math.h>

// should be defined math.h but sometimes isn't
#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define DEG2RAD(angle)  ((angle) * (M_PI / 180.0))
#define RAD2DEG(angle)  ((angle) * (180.0 / M_PI))

#define RGB_R(_rgb)             (_rgb[0])
#define RGB_G(_rgb)             (_rgb[1])
#define RGB_B(_rgb)             (_rgb[2])


typedef float                   rgb_t[3];

// converts RGB values to CIELAB2000 values
void    rgb2lab(rgb_t rgb, float *l, float *a, float *b);

// compute the distance between to CIELAB colors using 1976 method
float   cie76_delta(float l1, float a1, float b1, float l2, float a2, float b2);



#endif
