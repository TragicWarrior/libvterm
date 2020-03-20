#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "vterm.h"
#include "vterm_private.h"
#include "nc_wrapper.h"
#include "color_map.h"
#include "utlist.h"
#include "macros.h"
#include "color_math.h"
#include "color_list.h"

#define C16_COLORS(name, rgbval)    rgbval,
static unsigned long c16_colors_rgbval[] = {
#include "c16_color.def"
};
#undef C16_COLORS

color_map_t *
_vterm_create_mapped_color(vterm_t *vterm, color_map_t *mapped_color,
    int red, int green, int blue);

color_map_t *
_vterm_query_ncurses_color(vterm_t *vterm,
    int red, int green, int blue, float proximity);

void
_vterm_prepend_mapped_color(vterm_t *vterm, color_map_t *mapped_color);

void
vterm_color_map_init(vterm_t *vterm)
{
    color_map_t     *color_map;
    int             array_sz;
    int             r, g, b;
    int             i;

    /*
        init the first 16 colors of the color map (8 CGA + 8 AIX).
        see c16_colors.def for specifics.
    */
    array_sz = ARRAY_SZ(c16_colors_rgbval);

    for(i = 0; i < array_sz; i++)
    {
        r = (c16_colors_rgbval[i] & 0xFF0000) >> 24;
        g = (c16_colors_rgbval[i] & 0x00FF00) >> 16;
        b = (c16_colors_rgbval[i] & 0x0000FF);

        color_map = _vterm_create_mapped_color(vterm, NULL, r, g, b);
        color_map->global_color = i;
        color_map->private_color = i;

        CDL_PREPEND(vterm->color_map_head, color_map);
    }

    return;
}

short
vterm_add_mapped_color(vterm_t *vterm, short color,
    int red, int green, int blue, float proximity)
{
    color_map_t     *mapped_color;
    int             global_color;
    // int             r, g, b;

    /*
        is color one of the standard colors?
        the init_color() does not permit the basic colors to be redefined
        and SGR codes are wooden in this regard.
    */
    if(color >= COLOR_BLACK && color <= COLOR_WHITE) return color;

    red = ABSINT(red);
    green = ABSINT(green);
    blue = ABSINT(blue);
    red = red & 0x00FF;
    green = green & 0x00FF;
    blue = blue & 0x00FF;

    if((red + green + blue) == 0) return COLOR_BLACK;

    // excluding anonymous colors, check to see if the color is already mapped
    if(color != -1)
    {
        CDL_SEARCH_SCALAR(vterm->color_map_head, mapped_color,
            private_color, color);

        if(mapped_color != NULL)
        {
            _vterm_create_mapped_color(vterm, mapped_color, red, green, blue);

            // FILE *log = fopen("colors.log", "a+");
            // fprintf(log, "%d:%d - %d %d %d\n", global_color, color, red, green, blue);
            // fclose(log);

            red = RGB_TO_NCURSES(red);
            green = RGB_TO_NCURSES(green);
            blue = RGB_TO_NCURSES(blue);
            ncw_init_color(mapped_color->global_color, red, green, blue);

            return mapped_color->global_color;
        }
    }

    // find a close match and the alias it
    global_color = vterm_get_mapped_rgb(vterm, red, green, blue, proximity);
    if(global_color != -1)
    {
        mapped_color = _vterm_create_mapped_color(vterm, NULL, red, green, blue);
        mapped_color->global_color = global_color;
        mapped_color->private_color = color;
        _vterm_prepend_mapped_color(vterm, mapped_color);

        // FILE *log = fopen("colors.log", "a+");
        // fprintf(log, "%d:%d - %d %d %d\n", global_color, color, red, green, blue);
        // fclose(log);

        return global_color;
    }

    /*
        no matches found so we need to add a new color to the
        mapped color list.  if there are no free colors then
        we will purge the LRU color and add a new one.
    */
    global_color = color_list_get_free(TRUE);
    color_list_promote_color(global_color);

    mapped_color = _vterm_create_mapped_color(vterm, NULL, red, green, blue);
    mapped_color->global_color = global_color;
    mapped_color->private_color = color;
    _vterm_prepend_mapped_color(vterm, mapped_color);

    FILE *log = fopen("colors.log", "a+");
    fprintf(log, "%d:%d - %d %d %d\n", global_color, color, red, green, blue);
    fclose(log);

    // move this elsewhere
    red = RGB_TO_NCURSES(red);
    green = RGB_TO_NCURSES(green);
    blue = RGB_TO_NCURSES(blue);
    ncw_init_color(global_color, red, green, blue);

    return global_color;
}

/*
    this function uses several approaches to finding a color match
    for the RGB value specified.

    pass 1 - query the mapped color list

    1.  check for an exact RGB match
    2.  calculated Delta E using CIELAB76 and compare with proximty value

    pass 2 - query the ncurses color list

    (same as above)
*/

short
vterm_get_mapped_rgb(vterm_t *vterm, int r, int g, int b, float proximity)
{
    color_map_t     *mapped_color;
    color_map_t     *best_match = NULL;
    float           delta_new;
    float           delta_old;
    float           L, A, B;

    rgb2lab(r, g, b, &L, &A, &B);

    // force value to be out-of-range
    delta_old = proximity + 1.0;

    CDL_FOREACH(vterm->color_map_head, mapped_color)
    {
        if( mapped_color->r == r &&
            mapped_color->g == g &&
            mapped_color->b == b)
        {
            // exact match. pop to the top and return
            return mapped_color->global_color;
        }

        delta_new = cie76_delta(L, A, B,
            mapped_color->L, mapped_color->A, mapped_color->B);

        // not even close... keep looking
        if(delta_new > proximity) continue;

        if(delta_new < delta_old)
        {
            delta_old = delta_new;
            best_match = mapped_color;
        }
    }

    // we found a match in the mapped color list so return
    if(best_match != NULL)
    {
        return best_match->global_color;
    }

    return -1;
}

short
vterm_get_mapped_color(vterm_t *vterm, short color,
    int *red, int *green, int *blue)
{
    color_map_t     *mapped_color;

    // don't search for anonymous colors
    if(color == -1) return -1;

    CDL_SEARCH_SCALAR(vterm->color_map_head, mapped_color,
        private_color, color);

    if(mapped_color == NULL)
    {
        *red = -1;
        *green = -1;
        *blue = -1;

        return -1;
    }

    if(mapped_color->private_color == color)
    {
        *red = mapped_color->r;
        *green = mapped_color->g;
        *blue = mapped_color->b;

        return mapped_color->global_color;
    }

    return -1;
}

void
vterm_free_mapped_colors(vterm_t *vterm)
{
    color_map_t     *mapped_color;
    color_map_t     *tmp1;
    color_map_t     *tmp2;

    if(vterm == NULL) return;

    if(vterm->color_map_head == NULL) return;

    CDL_FOREACH_SAFE(vterm->color_map_head, mapped_color, tmp1, tmp2)
    {
        // set the color back to 0-0-0 to indicate that it's free
        ncw_init_color(mapped_color->global_color, 0, 0, 0);

        free(mapped_color);
    }

    vterm->color_map_head = NULL;

    return;
}

color_map_t *
_vterm_create_mapped_color(vterm_t *vterm, color_map_t *mapped_color,
    int red, int green, int blue)
{
    float   h, s, l;
    float   L, A, B;

    VAR_UNUSED(vterm);

    if(mapped_color == NULL)
    {
        mapped_color = CALLOC_PTR(mapped_color);
    }

    rgb2hsl(red, green, blue, &h, &s, &l);
    rgb2lab(red, green, blue, &L, &A, &B);

    mapped_color->r = red;
    mapped_color->g = green;
    mapped_color->b = blue;
    mapped_color->h = h;
    mapped_color->s = s;
    mapped_color->l = l;
    mapped_color->L = L;
    mapped_color->A = A;
    mapped_color->B = B;

    return mapped_color;
}

color_map_t *
_vterm_query_ncurses_color(vterm_t *vterm,
    int red, int green, int blue, float proximity)
{
    color_map_t     mapped_color;
    color_map_t     *best_match;
    float           L, A, B;
    int             r, g, b;
    float           delta_old;
    float           delta_new;
    int             retval;
    int             i;

    best_match = CALLOC_PTR(best_match);
    memset(&mapped_color, 0, sizeof(mapped_color));

    // force value to be out-of-range for match
    delta_old = proximity + 1.0;

    rgb2lab(red, green, blue, &L, &A, &B);

    /*
        query the ncuress color table.  there might already
        be a good match and we just need to pick it up.
    */
    i = COLOR_WHITE;
    for(;;)
    {
        i++;

        // limit hit.  no match found
        if(i > 0x3FFF) break;

        retval = ncw_color_content(i, &r, &g, &b);

        // skip if black
        if((r + g + b) == 0) continue;

        // limit hit.  no match found
        if(retval == -1) break;

        if(red == r && green == g && blue == b)
        {
            best_match = _vterm_create_mapped_color(vterm, NULL, r, g, b);
            best_match->global_color = i;
            return best_match;
        }

        mapped_color.r = r;
        mapped_color.g = g;
        mapped_color.b = b;
        rgb2lab(r, g, b, &mapped_color.L, &mapped_color.A, &mapped_color.B);

        delta_new = cie76_delta(L, A, B,
            mapped_color.r, mapped_color.g, mapped_color.b);

        if(delta_new > proximity) continue;

        if(delta_new < delta_old)
        {
            delta_old = delta_new;
            memcpy(best_match, &mapped_color, sizeof(mapped_color));
        }
    }

    // we found a match
    if(delta_old < proximity)
    {
        return best_match;
    }

    free(best_match);

    return NULL;
}

void
_vterm_prepend_mapped_color(vterm_t *vterm, color_map_t *mapped_color)
{
    color_map_t     *tmp1;
    color_map_t     *tmp2;
    int             count = 0;

    CDL_PREPEND(vterm->color_map_head, mapped_color);

    // keep the color list within limits
    CDL_FOREACH_SAFE(vterm->color_map_head, mapped_color, tmp1, tmp2)
    {
        if(count > vterm->term_colors)
        {
            CDL_DELETE(vterm->color_map_head, mapped_color);
            free(mapped_color);
        }

        count++;
    }

    return;
}



/*
void
_vterm_align_color_band(vterm_t *vterm, int *r, int *g, int *b)
{
    float   remainder;

    if(vterm == NULL) return;

    if(*r < 0) *r = 0;
    if(*g < 0) *g = 0;
    if(*b < 0) *b = 0;

    if(*r > 255) *r = 255;
    if(*g > 255) *g = 255;
    if(*b > 255) *b = 255;

    remainder = fmodf((float)*r, vterm->rgb_step);
    if(remainder <= vterm->rgb_half_step)
        *r -= (int)remainder;
    else
        *r += (int)(vterm->rgb_step - remainder);

    remainder = fmodf((float)*g, vterm->rgb_step);
    if(remainder <= vterm->rgb_half_step)
        *g -= (int)remainder;
    else
        *g += (int)(vterm->rgb_step - remainder);

    remainder = fmodf((float)*b, vterm->rgb_step);
    if(remainder <= vterm->rgb_half_step)
        *b -= (int)remainder;
    else
        *b += (int)(vterm->rgb_step - remainder);

    if(*r > 255) *r = 255;
    if(*g > 255) *g = 255;
    if(*b > 255) *b = 255;

    return;
}
*/
