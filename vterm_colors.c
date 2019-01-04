
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

#include "utlist.h"

color_cache_t*
color_cache_init(int pairs)
{
    color_cache_t   *color_cache;
    color_pair_t    *pair;

    int     i;

    color_cache = (color_cache_t *)calloc(1, sizeof(color_cache_t));

    for(i = 0; i < pairs; i++)
    {
        pair = (color_pair_t *)calloc(1, sizeof(color_pair_t));
        pair->num = i;

        // explode pair
        pair_content(i, &pair->fg, &pair->bg);

        // extract foreground RGB
        color_content(pair->fg, 
            &pair->red[0], &pair->green[0], &pair->blue[0]);

        // extract backgoroud RGB
        color_content(pair->bg,
            &pair->red[1], &pair->green[1], &pair->blue[1]);

        DL_PREPEND(color_cache->pair_head, pair);
    }

    return color_cache;
}

/*
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
*/

/*
void
free_color_space()
{
    if(color_palette == NULL) return;

    free(color_palette);
    color_palette = NULL;
    palette_size = 0;
}
*/

/*
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
*/


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

    // colors = vterm->pair_select(vterm, fg, bg);
    colors = color_cache_find_pair(vterm->color_cache, fg, bg);
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
color_cache_find_color(color_cache_t *color_cache, short color,
        short r, short g, short b)
{
    color_pair_t    *pair;
    color_pair_t    *pair_tmp;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    // if a color value was supplied, explode it and use those values
    if(color != -1)
    {
        color_content(color, &r, &g, &b);
    }

    DL_FOREACH_SAFE(color_cache->pair_head, pair, pair_tmp)
    {
        /*
            we're searching for a color that contains a specific RGB
            composition.  it doesn't matter whether that color is part
            of the foreground or background color of the pair.  the
            color number is the same no matter what.
        */

        // check the foreground color for a match
        if(pair->red[0] == r && pair->green[0] == g && pair->blue[0] == b)
        {
            DL_DELETE(color_cache->pair_head, pair);
            found = TRUE;
            break;
        }

        // check the background color for a match
        if(pair->red[1] == r && pair->green[1] == g && pair->blue[1] == b)
        {
            DL_DELETE(color_cache->pair_head, pair);
            found = TRUE;
            break;
        }
    }

    if(found == FALSE) return -1;

    /*
        push the pair to the front of the list to make subseqent look-ups
        faster.
    */
    DL_PREPEND(color_cache->pair_head, pair);

    return pair->num;
}

short
color_cache_find_pair(color_cache_t *color_cache, short fg, short bg)
{
    color_pair_t    *pair;
    color_pair_t    *pair_tmp;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    // iterate through the cache looking for a match
    DL_FOREACH_SAFE(color_cache->pair_head, pair, pair_tmp)
    {
        if(pair->fg == fg && pair->bg == bg)
        {
            // unlink the node so we can prepend it
            DL_DELETE(color_cache->pair_head, pair);
            found = TRUE;
            break;
        }
    }

    // match wasn't found
    if(found == FALSE) return -1;

    /*
        push the pair to the front of the list to make subseqent look-ups
        faster.
    */
    DL_PREPEND(color_cache->pair_head, pair);

    return pair->num;
}

short
color_cache_split_pair(color_cache_t *color_cache, short pair_num,
    short *fg, short *bg)
{
    color_pair_t    *pair;
    color_pair_t    *pair_tmp;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    DL_FOREACH_SAFE(color_cache->pair_head, pair, pair_tmp)
    {
        if(pair->num == pair_num)
        {
            DL_DELETE(color_cache->pair_head, pair);
            found = TRUE;
            break;
        }
    }

    if(found == FALSE)
    {
        *fg = -1;
        *bg = -1;

        return -1;
    }

    DL_PREPEND(color_cache->pair_head, pair);

    *fg = pair->fg;
    *bg = pair->bg;

    return 0;
}
