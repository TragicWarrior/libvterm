
#ifndef _VTERM_COLORS_H_
#define _VTERM_COLORS_H_

#include "vterm.h"

struct _color_pair_s
{
    uint8_t                 ref;

    short                   num;
    short                   fg;
    short                   bg;

    short                   red[2];
    short                   green[2];
    short                   blue[2];

    struct _color_pair_s    *next;
    struct _color_pair_s    *prev;
};


typedef struct _color_pair_s    color_pair_t;

struct _color_cache_s
{
    short           pair_count;

    color_pair_t    *pair_head;
};

typedef struct _color_cache_s   color_cache_t;

color_cache_t*
color_cache_init(int pairs);

short
color_cache_find_pair(color_cache_t *color_cache, short fg, short bg);

short
color_cache_find_color(color_cache_t *color_cache, short color,
    short r, short g, short b);

short
color_cache_split_pair(color_cache_t *color_cache, short pair_num,
    short *fg, short *bg);

#endif

