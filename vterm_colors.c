
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_colors.h"
#include "vterm_buffer.h"


struct _my_color_pair_s
{
    short fg;
    short bg;
};

typedef struct _my_color_pair_s my_color_pair_t;

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
find_color_pair(vterm_t *vterm, short fg, short bg)
{
    color_cache_t   *last;
    color_cache_t   *item;
    short           pair = -1;
    int             i;

    if(vterm == NULL) return -1;

    item = &vterm->color_cache[0];

    // check our cache
    for(i = 0; i < COLOR_BUF_SZ; i++)
    {
        if(item->fg == fg && item->bg == bg)
        {
            if(item->ref < 0xFF) item->ref++;
            return item->pair;
        }

        item++;
    }

    /*
        a "page fault" has occurred.  item wasn't found in cache so we have
        to look it up via the "long path".
    */
    if(vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
        pair = find_color_pair_simple(vterm, fg, bg);
    }
    else // ncurses
    {
#ifdef NOCURSES
        return -1;
#else
        short   fg_color, bg_color;
        if(has_colors() == FALSE)
            return -1;

        // this is expensive
        for(i = 1; i < COLOR_PAIRS; i++)
        {
            vterm->pair_split(vterm, i, &fg_color, &bg_color);
            if(fg_color == fg && bg_color == bg)
            {
                pair = i;
                break;
            }
        }

        if(i == COLOR_PAIRS) pair = -1;
#endif
    }

    // something went wrong... return error
    if(pair == -1) return -1;

    last = &vterm->color_cache[COLOR_BUF_SZ - 1];
    for(;;)
    {
        // found an empty slot
        if(vterm->cc_pos->ref == 0) break;

        vterm->cc_pos->ref--;

        if(vterm->cc_pos == last)
        {
            // use long notation for readability
            vterm->cc_pos = &vterm->color_cache[0];
            continue;
        }

        vterm->cc_pos++;
    }

    vterm->cc_pos->pair = pair;
    vterm->cc_pos->fg = fg;
    vterm->cc_pos->bg = bg;
    vterm->cc_pos->ref = 1;

    return pair;
}

/*
    this is a wrapper for pair_content() which can be really
    inefficient.  applications using libvterm should supply a different
    function through vterm_set_pair_splitter() for better performance.
*/
int
_native_pair_splitter_1(vterm_t *vterm, short pair, short *fg, short *bg)
{
    (void)vterm;    //make compiler happy

    pair_content(pair, fg, bg);

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
