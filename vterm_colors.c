
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_colors.h"
#include "vterm_buffer.h"


struct _color_cache_s
{
    uint8_t     ref;
    short       pair;
    short       fg;
    short       bg;
};

struct _my_color_pair_s
{
    short fg;
    short bg;
};

typedef struct _my_color_pair_s my_color_pair_t;

typedef struct _color_cache_s   color_cache_t;

#define COLOR_CACHE_SLOTS   6
#define MAX_COLOR_PAIRS     512

my_color_pair_t *color_palette = NULL;
int palette_size = 0;

void
init_color_space()
{
    int i, j, n;

    if(color_palette == NULL)
    {
        color_palette = calloc(MAX_COLOR_PAIRS, sizeof(my_color_pair_t));

        if(color_palette == NULL)
        {
            fprintf(stderr, "ERROR: cannot allocate color palette!\n");
            exit(1);
        }
    }

    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            n = i * 8 + j;

            if(n >= MAX_COLOR_PAIRS)
            {
                fprintf(stderr, "ERROR: cannot set color pair %d!\n", n);
                exit(1);
            }

            color_palette[n].fg = 7 - i;
            color_palette[n].bg = j;

            if(n + 1 > palette_size )
            {
                palette_size = n + 1;
            }
        }
    }
}

void
free_color_space()
{
    if(color_palette == NULL) return;

    free(color_palette);
    color_palette = NULL;
    palette_size = 0;
}

short
find_color_pair_simple(vterm_t *vterm, short fg, short bg)
{
    my_color_pair_t *cp;
    int i = 0;

    (void)vterm;            // for later use.  suppress warnings for now

    if(color_palette == NULL)
    {
        init_color_space();
    }

    for(i = 0; i < palette_size; ++i)
    {
        cp = color_palette + i;

        if(cp->fg == fg && cp->bg == bg)
        {
            return i;
        }
    }

    return -1;
}


void
vterm_set_pair_selector(vterm_t *vterm, VtermPairSelect ps)
{
    int     default_color = 0;

    if(vterm == NULL) return;

    // todo:  in the future, make a NULL value revert to defaults
    if(ps == NULL) return;

    vterm->pair_select = ps;

    /*
        if the user has specified NOCURSES, we need to use the 
        new routine to locate the white on black color pair.
    */
    if(vterm->flags & VTERM_FLAG_NOCURSES)
    {
        default_color = vterm->pair_select(vterm, COLOR_WHITE, COLOR_BLACK);

        if(default_color < 0 || default_color > 255)
            default_color = 0;

        vterm->vterm_desc[0].curattr = (default_color & 0xff) << 8;
    }

    return;
}

VtermPairSelect
vterm_get_pair_selector(vterm_t *vterm)
{
    if(vterm == NULL) return NULL;

    return vterm->pair_select;
}

void
vterm_set_pair_splitter(vterm_t *vterm, VtermPairSplit ps)
{
    if(vterm == NULL) return;

    if(ps == NULL) return;

    vterm->pair_split = ps;

    return;
}

VtermPairSplit
vterm_get_pair_splitter(vterm_t *vterm)
{
    if(vterm == NULL) return NULL;

    return vterm->pair_split;
}

int
vterm_set_colors(vterm_t *vterm, short fg, short bg)
{
    vterm_desc_t    *v_desc = NULL;
    short           colors;
    int             idx;

    if(vterm == NULL) return -1;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

#ifdef NOCURSES
    if(has_colors() == FALSE) return -1;
#endif

    colors = vterm->pair_select(vterm, fg, bg);
    if(colors == -1) colors = 0;

    v_desc->colors = colors;

    return 0;
}

short
vterm_get_colors(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return -1;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

#ifndef NOCURSES
        if(has_colors() == FALSE) return -1;
#endif

    return v_desc->colors;
}

short
find_color_pair(vterm_t *vterm, short fg,short bg)
{
    int             i;

    if(vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
        return find_color_pair_simple(vterm, fg, bg);
    }
    else // ncurses
    {
#ifdef NOCURSES
        return -1;
#else
        short   fg_color, bg_color;
        if(has_colors() == FALSE)
            return -1;

        for(i = 1; i < COLOR_PAIRS; i++)
        {
            vterm->pair_split(vterm, i, &fg_color, &bg_color);
            if(fg_color == fg && bg_color == bg) break;
        }

        if(i == COLOR_PAIRS)
            return -1;
#endif
    }

    return i;
}

/*
    this is a wrapper for pair_content() which can be really
    inefficient.  applications using libvterm should supply a different
    function through vterm_set_pair_splitter() for better performance.
*/
int
_native_pair_splitter_1(vterm_t *vterm, short pair, short *fg, short *bg)
{
    static color_cache_t    *color_cache = NULL;
    static color_cache_t    *pos;
    static color_cache_t    *end;
    int                     i;

    (void)vterm;            // make compiler happy

    // one time init of cache
    if(color_cache == NULL)
    {
        color_cache = (color_cache_t*)calloc(COLOR_CACHE_SLOTS,
            sizeof(color_cache_t));

        // init pairs to -1 so we don't match on zero accidentally
        for(i = 0; i < COLOR_CACHE_SLOTS; i++)
        {
            color_cache[i].pair = -1;
        }

        pos = color_cache;
        end = pos + COLOR_CACHE_SLOTS;
    }

    // iterate through cache looking for a match or an open slot
    for(;;)
    {
        // found a match
        if(pos->pair == pair) break;

        // found an empty slot
        if(pos->ref == 0) break;

        pos->ref--;

        if(pos == end)
        {
            pos = color_cache;
            continue;
        }

        pos++;
    }

    // loop broke because we found an empty slot.   load it.
    if(pos->ref == 0)
    {
        pair_content(pair, &pos->fg, &pos->bg);
        pos->pair = pair;
        if(pos->ref < 0xFF) pos->ref++;
    }

    *fg = pos->fg;
    *bg = pos->bg;

    return 0;
}

int
_native_pair_splitter_2(vterm_t *vterm, short pair, short *fg, short *bg)
{
    (void)vterm;        // make compiler happy

    if(color_palette == NULL || pair >= palette_size || pair < 0)
    {
        *fg = 0;
        *bg = 0;
        return -1;
    }

    *fg = color_palette[pair].fg;
    *bg = color_palette[pair].bg;

    return 0;
}
