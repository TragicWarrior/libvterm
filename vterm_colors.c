
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
#include "color_math.h"

#include "utlist.h"

#define PAIR_FG_R(_pair_ptr)    (_pair_ptr->rgb_values[0].r)
#define PAIR_FG_G(_pair_ptr)    (_pair_ptr->rgb_values[0].g)
#define PAIR_FG_B(_pair_ptr)    (_pair_ptr->rgb_values[0].b)

#define PAIR_BG_R(_pair_ptr)    (_pair_ptr->rgb_values[1].r)
#define PAIR_BG_G(_pair_ptr)    (_pair_ptr->rgb_values[1].g)
#define PAIR_BG_B(_pair_ptr)    (_pair_ptr->rgb_values[1].b)

void
_color_cache_profile_pair(color_pair_t *pair);

color_cache_t*
color_cache_init(void)
{
    color_cache_t       *color_cache;
    color_pair_t        *pair;
    int                 i;

    color_cache = (color_cache_t *)calloc(1, sizeof(color_cache_t));

    color_cache->term_colors = tigetnum("colors");
    color_cache->term_pairs = tigetnum("pairs");

    // clamp max pairs at 0x7FFF
#ifdef NCURSES_EXT_COLORS
    if(color_cache->term_pairs > 0x7FFF) color_cache->term_pairs = 0x7FFF;
#else
    if(color_cache->term_pairs > 0xFF) color_cache->term_pairs = 0xFF;
#endif

    // profile all colors
    for(i = 0; i < color_cache->term_pairs; i++)
    {
        pair = (color_pair_t *)calloc(1, sizeof(color_pair_t));
        pair->num = i;

        _color_cache_profile_pair(pair);

        CDL_APPEND(color_cache->head[PALETTE_DEFAULT], pair);
        color_cache->pair_count++;
    }

    color_cache_save_palette(color_cache);

    return color_cache;
}

void
color_cache_save_palette(color_cache_t *color_cache)
{
    color_pair_t    *pair;
    color_pair_t    *tmp1;
    // color_pair_t    *tmp2;

    if(color_cache == NULL) return;

    // free the saved palette if it exists
    color_cache_free_palette(color_cache, PALETTE_SAVED);

    // copy the active palette into a saved palette
    CDL_FOREACH(color_cache->head[PALETTE_DEFAULT], pair)
    {
        tmp1 = (color_pair_t *)malloc(sizeof(color_pair_t));
        memcpy(tmp1, pair, sizeof(color_pair_t));

        CDL_PREPEND(color_cache->head[PALETTE_SAVED], tmp1);
    }

    return;
}

void
color_cache_free_palette(color_cache_t *color_cache, int cache_id)
{
    color_pair_t    *pair;
    color_pair_t    *tmp1;
    color_pair_t    *tmp2;

    if(color_cache == NULL) return;

    if(color_cache->head[cache_id] != NULL)
    {
        CDL_FOREACH_SAFE(color_cache->head[cache_id], pair, tmp1, tmp2)
        {
            CDL_DELETE(color_cache->head[cache_id], pair);

            free(pair);
        }

        color_cache->head[cache_id] = NULL;
    }

    return;
}

// use the pair at the end of the list as it's the lowest risk
int
color_cache_add_pair(color_cache_t *color_cache, short fg, short bg)
{
    color_pair_t            *pair;
    short                   i;
    int                     retval;

    if(color_cache == NULL) return -1;
    i = color_cache->term_pairs - 1;

    pair = (color_pair_t *)calloc(1, sizeof(color_pair_t));

    // start backward looking for an unenumerated pair.
    while(i > 0)
    {
        memset(pair, 0, sizeof(color_pair_t));

        pair->num = i;
        retval = pair_content(i, (short *)&pair->fg, (short *)&pair->bg);

        // if we can't explode the pair, keep searching
        if(retval == -1)
        {
            i--;
            continue;
        }

        /*
            FG and BG of zero would be a truly odd pair and almost certianly
            indicates an unused pair.
        */
        if(pair->fg == 0 && pair->bg == 0) break;

        i--;
    }

    init_pair(pair->num, fg, bg);
    _color_cache_profile_pair(pair);

    CDL_PREPEND(color_cache->head[PALETTE_DEFAULT], pair);

    return i;
}


void
color_cache_destroy(color_cache_t *color_cache)
{
    if(color_cache == NULL) return;

    color_cache_free_palette(color_cache, PALETTE_SAVED);

    color_cache_free_palette(color_cache, PALETTE_DEFAULT);

    free(color_cache);

    return;
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
    long            colors;
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

    v_desc->default_colors = colors;

    return 0;
}

long
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

    return v_desc->default_colors;
}


long
color_cache_find_exact_color(color_cache_t *color_cache,
    unsigned short color, short r, short g, short b)
{
    color_pair_t    *pair;
    color_pair_t    *tmp1;
    color_pair_t    *tmp2;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    color_content(color, &r, &g, &b);

    CDL_FOREACH_SAFE(color_cache->head[PALETTE_DEFAULT], pair, tmp1, tmp2)
    {
        /*
            we're searching for a color that contains a specific RGB
            composition.  it doesn't matter whether that color is part
            of the foreground or background color of the pair.  the
            color number is the same no matter what.
        */

        // check the foreground color for a match
        if(pair->rgb_values[0].r == r &&
            pair->rgb_values[0].g == g &&
            pair->rgb_values[0].b == b)
        {
            CDL_DELETE(color_cache->head[PALETTE_DEFAULT], pair);
            found = TRUE;
            break;
        }

        // check the background color for a match
        if(pair->rgb_values[1].r == r &&
            pair->rgb_values[1].g == g &&
            pair->rgb_values[1].b == b)
        {
            CDL_DELETE(color_cache->head[PALETTE_DEFAULT], pair);
            found = TRUE;
            break;
        }
    }

    if(found == FALSE) return -1;

    /*
        push the pair to the front of the list to make subseqent look-ups
        faster.
    */
    CDL_PREPEND(color_cache->head[PALETTE_DEFAULT], pair);

    return pair->num;
}

long
color_cache_find_pair(color_cache_t *color_cache, short fg, short bg)
{
    color_pair_t    *pair;
    color_pair_t    *tmp1;
    color_pair_t    *tmp2;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    // iterate through the cache looking for a match
    CDL_FOREACH_SAFE(color_cache->head[PALETTE_DEFAULT], pair, tmp1, tmp2)
    {
        if(pair->fg == fg && pair->bg == bg)
        {
            // unlink the node so we can prepend it
            CDL_DELETE(color_cache->head[PALETTE_DEFAULT], pair);
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
    CDL_PREPEND(color_cache->head[PALETTE_DEFAULT], pair);

    return pair->num;
}

short
color_cache_split_pair(color_cache_t *color_cache,
    unsigned short pair_num, short *fg, short *bg)
{
    color_pair_t    *pair;
    color_pair_t    *tmp1;
    color_pair_t    *tmp2;
    bool            found = FALSE;

    if(color_cache == NULL) return -1;

    CDL_FOREACH_SAFE(color_cache->head[PALETTE_DEFAULT], pair, tmp1, tmp2)
    {
        if(pair->num == pair_num)
        {
            CDL_DELETE(color_cache->head[PALETTE_DEFAULT], pair);
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

    CDL_PREPEND(color_cache->head[PALETTE_DEFAULT], pair);

    *fg = pair->fg;
    *bg = pair->bg;

    return 0;
}

void
_color_cache_profile_pair(color_pair_t *pair)
{
    // int             retval;
    short           r, g, b;

    if(pair == NULL) return;

    // explode pair
    pair_content(pair->num, &pair->fg, &pair->bg);

    // extract foreground RGB
    color_content(pair->fg, &r, &g, &b);

    pair->rgb_values[0].r = r;
    pair->rgb_values[0].g = g;
    pair->rgb_values[0].b = b;

    // extract background RGB
    color_content(pair->bg, &r, &g, &b);

    pair->rgb_values[1].r = r;
    pair->rgb_values[1].g = g;
    pair->rgb_values[1].b = b;

    return;

    /*
        store the HSL foreground values

        ncurses uses a RGB range from 0 to 1000 but most "community" code
        use 0 - 256 so we'll normalize that by 0.25.
    */
    rgb2hsl((float)r * 0.25, (float)g * 0.25, (float)b *0.25,
        &pair->hsl_values[0].h,
        &pair->hsl_values[0].s,
        &pair->hsl_values[0].l);

    // store the CIE2000 lab foreground values
    rgb2lab(r / 4, g / 4, b / 4,
        &pair->cie_values[0].l,
        &pair->cie_values[0].a,
        &pair->cie_values[0].b);


    // store the HLS background values
    rgb2hsl((float)r * 0.25, (float)g * 0.25, (float)b * 0.25,
        &pair->hsl_values[1].h,
        &pair->hsl_values[1].s,
        &pair->hsl_values[1].l);

    // store the CIE2000 lab background values
    rgb2lab(r / 4, g / 4, b / 4,
        &pair->cie_values[1].l,
        &pair->cie_values[1].a,
        &pair->cie_values[1].b);

    return;
}
