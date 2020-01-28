#ifndef _NC_WRAPPER_H_
#define _NC_WRAPPER_H_

int     ncw_init_pair(int pair, int f, int b);

int     ncw_init_color(int color, int r, int g, int b);

int     ncw_pair_content(int pair, int *f, int *b);

int     ncw_color_content(int color, int *r, int *g, int *b);

#endif


