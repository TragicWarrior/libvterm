#ifndef _COLOR_MATH_H_
#define _COLOR_MATH_H_

// converts RGB values to CIELAB2000 values
void    rgb2lab(int red, int green, int blue, float *l, float *a, float *b);

// converts RGB values to Hue, Luminosity, Saturation values
void    rgb2hsl(float r, float g, float b, float *h, float *l, float *s);

// compute the distance between to CIELAB colors using 1976 method
float   cie76_delta(float l1, float a1, float b1, float l2, float a2, float b2);

#endif
