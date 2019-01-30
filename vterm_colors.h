
#ifndef _VTERM_COLORS_H_
#define _VTERM_COLORS_H_

#include "vterm.h"

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


struct _color_pair_s
{
    uint8_t                 ref;

    int                     num;
    short                   fg;
    short                   bg;

    bool                    unbound;

    rgb_values_t            rgb_values[2];
    hsl_values_t            hsl_values[2];
    cie_values_t            cie_values[2];

    struct _color_pair_s    *next;
    struct _color_pair_s    *prev;
};


typedef struct _color_pair_s    color_pair_t;

struct _color_cache_s
{
    int             pair_count;

    int             reserved_pair;

    int             term_colors;
    int             term_pairs;

    color_pair_t    *head[PALETTE_MAX];
};

typedef struct _color_cache_s   color_cache_t;

void
vterm_color_cache_init(void);

int
vterm_color_cache_add_pair(short fg, short bg);

void
vterm_color_cache_destroy();

void
vterm_color_cache_save_palette(int cache_id);

void
vterm_color_cache_copy_palette(int source, int target);

void
vterm_color_cache_load_palette(int cache_id);

void
vterm_color_cache_free_palette(int cache_id);

int
vterm_color_cache_find_pair(short fg, short bg);

int
vterm_color_cache_find_exact_color(int color, short r, short g, short b);

int
vterm_color_cache_find_nearest_color(short r, short g, short b);

int
vterm_color_cache_split_pair(int pair_num, short *fg, short *bg);


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

