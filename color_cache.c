
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>

#include "macros.h"
#include "nc_wrapper.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "color_math.h"
#include "color_cache.h"

#include "utlist.h"

#define PAIR_FG_R(_pair_ptr)    (_pair_ptr->rgb_values[0].r)
#define PAIR_FG_G(_pair_ptr)    (_pair_ptr->rgb_values[0].g)
#define PAIR_FG_B(_pair_ptr)    (_pair_ptr->rgb_values[0].b)

#define PAIR_BG_R(_pair_ptr)    (_pair_ptr->rgb_values[1].r)
#define PAIR_BG_G(_pair_ptr)    (_pair_ptr->rgb_values[1].g)
#define PAIR_BG_B(_pair_ptr)    (_pair_ptr->rgb_values[1].b)

#define C16(a, b, c, d) b,
short rRGB[] = {
#include "c16_color.def"
};
#undef C16

#define C16(a, b, c, d) c,
short gRGB[] = {
#include "c16_color.def"
};
#undef C16

#define C16(a, b, c, d) d,
short bRGB[] = {
#include "c16_color.def"
};
#undef C16

void
_color_cache_reset_pair(color_pair_t *pair);

void
_color_cache_profile_pair(color_pair_t *pair);

void
color_cache_init(void)
{
    extern color_cache_t    *color_cache;

    /*
        color cache has already be initialized by another instance
        so just increment the ref count and return.
    */
    if(color_cache != NULL)
    {
        color_cache->ref_count++;
        return;
    }

    color_cache = (color_cache_t *)calloc(1, sizeof(color_cache_t));
    color_cache->ref_count = 1;

    color_cache->term_colors = tigetnum("colors");
    color_cache->term_pairs = tigetnum("pairs");

    // clamp max pairs at 0x7FFF
#ifdef NCURSES_EXT_COLORS
    if(color_cache->term_pairs > 0x7FFF)
        color_cache->term_pairs = 0x7FFF;
#else
    if(color_cache->term_pairs > 0xFF)
        color_cache->term_pairs = 0xFF;
#endif

    // take a snapshot of the current palette
    color_cache_save_palette(PALETTE_HOST);

    color_cache_copy_palette(PALETTE_HOST, PALETTE_ACTIVE);

    return;
}

void
color_cache_copy_palette(int source, int target)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;

    if(color_cache->head[source] == NULL) return;

    // if the target cache is already used, free it
    if(color_cache->head[target] != NULL)
    {
        color_cache_free_palette(target);
    }

    CDL_FOREACH(color_cache->head[source], pair)
    {
        tmp1 = (color_pair_t *)malloc(sizeof(color_pair_t));

        memcpy(tmp1, pair, sizeof(color_pair_t));

        CDL_APPEND(color_cache->head[target], tmp1);
    }

    return;
}

void
color_cache_save_palette(int cache_id)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    int                     i;

    if(color_cache->head[cache_id] != NULL)
    {
        // free the saved palette if it exists
        color_cache_free_palette(cache_id);
    }

    color_cache->reserved_pair = -1;

    // profile all colors
    for(i = 0; i < color_cache->term_pairs; i++)
    {
        pair = (color_pair_t *)calloc(1, sizeof(color_pair_t));
        pair->num = i;

        _color_cache_profile_pair(pair);

        // reserve one pair where fg 0 and bg 0
        if(pair->fg == 0 && pair->bg == 0)
        {
            if(color_cache->reserved_pair == -1)
            {
                color_cache->reserved_pair = i;
            }
        }

        CDL_APPEND(color_cache->head[cache_id], pair);
    }

    return;
}

void
color_cache_load_palette(int cache_id)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;

    if(color_cache->head[cache_id] != NULL)
    {
        // free the default palette (the active palette)
        color_cache_free_palette(cache_id);

        // copy the specified palette into
        CDL_FOREACH(color_cache->head[cache_id], pair)
        {
            tmp1 = (color_pair_t *)malloc(sizeof(color_pair_t));
            memcpy(tmp1, pair, sizeof(color_pair_t));

            CDL_PREPEND(color_cache->head[cache_id], tmp1);

            ncw_init_pair(pair->num, pair->fg, pair->bg);
        }
    }

    return;
}


void
color_cache_free_palette(int cache_id)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;
    color_pair_t            *tmp2;

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

short
color_cache_find_unused_pair()
{
    extern color_cache_t    *color_cache;
    color_pair_t            pair;
    short                   i;
    int                     retval;

    i = color_cache->term_pairs - 1;

    // start backward looking for an unenumerated pair.
    while(i > 0)
    {
        memset(&pair, 0, sizeof(color_pair_t));

        retval = ncw_pair_content(i, &pair.fg, &pair.bg);

        // if we can't explode the pair, keep searching
        if(retval == -1)
        {
            i--;
            continue;
        }

        // if this is the reserved pair, skip
        if(i == color_cache->reserved_pair)
        {
            i--;
            continue;
        }

        /*
            FG and BG of zero would be a truly odd pair and almost certianly
            indicates an unused pair.
        */
        if(pair.fg == 0 && pair.bg == 0) break;

        i--;
    }

    return i;
}

short
color_cache_find_lru_pair()
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;

    pair = color_cache->head[PALETTE_ACTIVE];

    for(;;)
    {
        // break out of an infitite loop scenario
        if(pair->prev == color_cache->head[PALETTE_ACTIVE])
        {
             // no free pairs
             return -1;
        }

        pair = pair->prev;

        // if this is the reserve pair, keep going
        if(pair->num == color_cache->reserved_pair) continue;

        /*
            From the ncurses man pages:

            color pair 0 is special; it denotes “no color”.

            Color pair 0 is assumed to be white on black, but is
            actually whatever the terminal implements before color is
            initialized.  It  cannot be modified by the application.
        */
        if(pair->num == 0) continue;

        // make sure the pair is not part of the system palette
        if(pair->custom == TRUE) break;
    }

    return pair->num;
}

// use the pair at the end of the list as it's the lowest risk
int
color_cache_add_pair(vterm_t *origin, short fg, short bg)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    short                   i;

    i = color_cache_find_unused_pair();

    if(i <= 0)
    {
        i = color_cache_find_lru_pair();
    }

    // we've exhausted all options.  return pair 0.
    if(i <= 0) return 0;

    CDL_SEARCH_SCALAR(color_cache->head[PALETTE_ACTIVE], pair, num, i);

    if(pair != NULL)
    {
        CDL_DELETE(color_cache->head[PALETTE_ACTIVE], pair);

        pair->origin = origin;
        pair->custom = TRUE;
        pair->num = i;

        ncw_init_pair(pair->num, fg, bg);
        _color_cache_profile_pair(pair);

        CDL_PREPEND(color_cache->head[PALETTE_ACTIVE], pair);

        return i;
    }

    return 0;
}

void
color_cache_free_pairs(vterm_t *origin)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;

    if(color_cache == NULL) return;
    if(origin == NULL) return;

    CDL_FOREACH(color_cache->head[PALETTE_ACTIVE], pair)
    {
        // safety check.  pair should never be a system pair
        if(pair->custom == FALSE) continue;

        // only origin matters
        if(pair->origin == origin)
        {
            _color_cache_reset_pair(pair);
        }
    }

    return;
}


void
color_cache_release(void)
{
    extern color_cache_t    *color_cache;
    int                     i;

    color_cache->ref_count--;

    // if ref count is not zero return
    if(color_cache->ref_count > 0) return;

    for(i = 0; i < PALETTE_MAX; i++)
    {
        color_cache_free_palette(i);
    }

    free(color_cache);
    color_cache = NULL;

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

    colors = color_cache_find_pair(fg, bg);
    if(colors == -1) colors = 0;

    v_desc->default_colors = (short)colors;

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


int
color_cache_find_exact_color(short r, short g, short b)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;
    color_pair_t            *tmp2;
    bool                    fg_found = FALSE;
    bool                    bg_found = FALSE;

    CDL_FOREACH_SAFE(color_cache->head[PALETTE_ACTIVE], pair, tmp1, tmp2)
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
            fg_found = TRUE;
        }
        else
        {
            fg_found = FALSE;
            continue;
        }

        // check the background color for a match
        if(pair->rgb_values[1].r == r &&
            pair->rgb_values[1].g == g &&
            pair->rgb_values[1].b == b)
        {
            bg_found = TRUE;
        }
        else
        {
            bg_found = FALSE;
            continue;
        }

        if(fg_found == TRUE && bg_found == TRUE)
        {
            CDL_DELETE(color_cache->head[PALETTE_ACTIVE], pair);
            break;
        }
    }

    if(fg_found == FALSE || bg_found == FALSE) return -1;

    /*
        push the pair to the front of the list to make subseqent look-ups
        faster.
    */
    CDL_PREPEND(color_cache->head[PALETTE_ACTIVE], pair);

    return pair->num;
}

int
color_cache_find_pair(short fg, short bg)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;
    color_pair_t            *tmp2;

    // iterate through the cache looking for a match
    CDL_FOREACH_SAFE(color_cache->head[PALETTE_ACTIVE], pair, tmp1, tmp2)
    {
        if(pair->fg == fg && pair->bg == bg)
        {
            // unlink the node so we can prepend it
            CDL_DELETE(color_cache->head[PALETTE_ACTIVE], pair);

            /*
                push the pair to the front of the list to make subseqent
                look-ups faster.
            */
            CDL_PREPEND(color_cache->head[PALETTE_ACTIVE], pair);

            return pair->num;
        }
    }

    return -1;
}

int
color_cache_split_pair(int pair_num, short *fg, short *bg)
{
    extern color_cache_t    *color_cache;
    color_pair_t            *pair;
    color_pair_t            *tmp1;
    color_pair_t            *tmp2;
    bool                    found = FALSE;

    CDL_FOREACH_SAFE(color_cache->head[PALETTE_ACTIVE], pair, tmp1, tmp2)
    {
        if(pair->num == pair_num)
        {
            CDL_DELETE(color_cache->head[PALETTE_ACTIVE], pair);
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

    CDL_PREPEND(color_cache->head[PALETTE_ACTIVE], pair);

    *fg = pair->fg;
    *bg = pair->bg;

    return 0;
}

void
_color_cache_profile_pair(color_pair_t *pair)
{
    int     r, g, b;

    if(pair == NULL) return;

    // explode pair
    ncw_pair_content(pair->num, &pair->fg, &pair->bg);

    // extract foreground RGB
    ncw_color_content(pair->fg, &r, &g, &b);

    pair->rgb_values[0].r = r;
    pair->rgb_values[0].g = g;
    pair->rgb_values[0].b = b;

    rgb2hsl(RGB_FLOAT(NCURSES_RGB(r), NCURSES_RGB(g), NCURSES_RGB(b)),
        &pair->hsl_values[0].h,
        &pair->hsl_values[0].s,
        &pair->hsl_values[0].l);

    // store the CIE2000 lab foreground values
    // rgb2lab(RGB_FLOAT(NCURSES_RGB(r), NCURSES_RGB(g), NCURSES_RGB(b)),
    //    &pair->cie_values[0].l,
    //    &pair->cie_values[0].a,
    //    &pair->cie_values[0].b);

    // extract background RGB
    ncw_color_content(pair->bg, &r, &g, &b);

    pair->rgb_values[1].r = r;
    pair->rgb_values[1].g = g;
    pair->rgb_values[1].b = b;

    // store the HLS background values
    rgb2hsl(RGB_FLOAT(NCURSES_RGB(r), NCURSES_RGB(g), NCURSES_RGB(b)),
        &pair->hsl_values[1].h,
        &pair->hsl_values[1].s,
        &pair->hsl_values[1].l);

    // store the CIE2000 lab background values
    // rgb2lab(RGB_FLOAT(NCURSES_RGB(r), NCURSES_RGB(g), NCURSES_RGB(b)),
    //    &pair->cie_values[1].l,
    //    &pair->cie_values[1].a,
    //    &pair->cie_values[1].b);

    return;
}

void
_color_cache_reset_pair(color_pair_t *pair)
{
    if(pair == NULL) return;

    pair->origin = NULL;
    pair->fg = 0;
    pair->bg = 0;

    memset(pair->rgb_values, 0, sizeof(pair->rgb_values));
    memset(pair->hsl_values, 0, sizeof(pair->hsl_values));

    ncw_init_pair(pair->num, 0, 0);

    return;
}
