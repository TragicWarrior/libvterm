
#ifndef _VTERM_COLORS_H_
#define _VTERM_COLORS_H_

#include "vterm.h"

struct _color_cache_s
{
    uint8_t     ref;
    short       pair;
    short       fg;
    short       bg;
};

typedef struct _color_cache_s   color_cache_t;

#define CC_SIZE 6

short
find_color_pair(vterm_t *vterm, short fg, short bg);

short
find_color_pair_simple(vterm_t *vterm, short fg, short bg);

int
_native_pair_splitter_1(vterm_t *vterm, short pair, short *fg, short *bg);

int
_native_pair_splitter_2(vterm_t *vterm, short pair, short *fg, short *bg);

// short local hacks equivalent to ncurses, in case we don't link to it.
void
init_color_space();

void
free_color_cpace();

#endif

