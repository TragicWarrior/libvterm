
#ifndef _COLOR_CACHE_H_
#define _COLOR_CACHE_H_

#include <signal.h>

#include "vterm.h"

#define C16(a, b, c, d) a,
enum {
#include "c16_color.def"
};
#undef C16

/*
    ncurses RGB values range from 0 to 1000 while the rest of the world
    pretty much uses 0 - 255 so we need to scale.
*/
#define NCURSES_RGB(x)              (((float)x / 1000.0) * 255.0)


enum
{
    PALETTE_HOST    =   0x00,
    PALETTE_ACTIVE,
    PALETTE_MAX                     // not a real palette.  just a counter.
};


struct _rgb_values_s
{
    short   r;
    short   g;
    short   b;
};

struct _hsl_values_s
{
    float   h;
    float   s;
    float   l;
};

struct _cie_values_s
{
    float   l;
    float   a;
    float   b;
};

typedef struct _rgb_values_s    rgb_values_t;
typedef struct _hsl_values_s    hsl_values_t;
typedef struct _cie_values_s    cie_values_t;

typedef struct _color_pair_s    color_pair_t;

struct _color_pair_s
{
    int                     num;
    int                     fg;
    int                     bg;

    rgb_values_t            rgb_values[2];
    hsl_values_t            hsl_values[2];
    cie_values_t            cie_values[2];

    vterm_t                 *origin;        /*
                                                indicates which instance
                                                added the pair to the
                                                global table.
                                            */

    bool                    custom;         /*
                                                indicates if the pair was
                                                loaded from the host
                                                palette.
                                            */
    color_pair_t            *next;
    color_pair_t            *prev;
};

struct _color_cache_s
{
    sig_atomic_t    ref_count;              /*
                                                incremented when an new
                                                vterm instance is allocated.
                                                it's decremented when a
                                                vterm instance is destroyed.
                                                when it reaches zero, all
                                                resources are freed.

                                                use atomic type to prevent
                                                race condition.
                                            */
    int             pair_count;

    int             reserved_pair;

    int             term_colors;
    int             term_pairs;

    color_pair_t    *head[PALETTE_MAX];
};

typedef struct _color_cache_s   color_cache_t;

void
color_cache_init(void);

void
color_cache_release(void);

void
color_cache_save_palette(int cache_id);

void
color_cache_copy_palette(int source, int target);

void
color_cache_load_palette(int cache_id);

void
color_cache_free_palette(int cache_id);

int
color_cache_add_pair(vterm_t *origin, short fg, short bg);

void
color_cache_free_pairs(vterm_t *origin);

short
color_cache_find_lru_pair(void);

short
color_cache_find_unused_pair(void);

int
color_cache_find_pair(short fg, short bg);

int
color_cache_find_exact_color(short r, short g, short b);

int
color_cache_find_nearest_color(short r, short g, short b);

int
color_cache_split_pair(int pair_num, short *fg, short *bg);


#define DEBUG_COLOR_PAIRS(cache, max)                                       \
            {                                                               \
                color_pair_t    *_pair;                                     \
                int             _limit = max;                               \
                                                                            \
                CDL_FOREACH((cache)->head, _pair)                           \
                {                                                           \
                    if(_limit == 0) break;                                  \
                    printf("Pair Num:   %d\n\r", _pair->num);               \
                    printf("  Fg:         %d\n\r", _pair->fg);              \
                    printf("  F RGB:      r: %d, g: %d, b: %d\n\r",         \
                        _pair->rgb_values[0].r,                             \
                        _pair->rgb_values[0].g,                             \
                        _pair->rgb_values[0].b);                            \
                    printf("  Bg:         %d\n\r", _pair->bg);              \
                    printf("  B RGB:      r: %d, g: %d, b: %d\n\r",         \
                        _pair->rgb_values[1].r,                             \
                        _pair->rgb_values[1].g,                             \
                        _pair->rgb_values[1].b);                            \
                    _limit--;                                               \
                }                                                           \
                printf("cached pairs: %d\n\r", (cache)->pair_count);        \
            }

#endif

