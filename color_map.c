#include <stdlib.h>
#include <math.h>

#include "vterm.h"
#include "vterm_private.h"
#include "nc_wrapper.h"
#include "color_map.h"
#include "utlist.h"

void
_vterm_align_color_band(vterm_t *vterm, int *r, int *g, int *b);

short
vterm_add_mapped_color(vterm_t *vterm, short color,
    int red, int green, int blue)
{
    color_map_t     *mapped_color;
    short           global_color;
    int             r, g, b;
    int             retval;

    /*
        is color one of the standard colors?
        the init_color() does not permit the basic colors to be redefined
        and SGR codes are wooden in this regard.
    */
    if(color >= COLOR_BLACK && color <= COLOR_WHITE) return color;

    if(red < 0) red = 0;
    if(green < 0) green = 0;
    if(blue < 0) blue = 0;

    if((red + green + blue) == 0) return COLOR_BLACK;

    if(red > 255) red = 255;
    if(green > 255) green = 255;
    if(blue > 255) blue = 255;

    // excluding anonymous colors, check to see if the color is already mapped
    if(color != -1)
    {
        global_color = vterm_get_mapped_color(vterm, color);
        if(global_color != -1)
        {
            return global_color;
        }
    }

    global_color = vterm_get_mapped_rgb(vterm, red, green, blue);
    if(global_color != -1) return global_color;

    // start looking past the first 8 basic colors
    global_color = COLOR_WHITE + 1;

    // look for a free color number in the global color table
    for(;;)
    {
        // no free color numbers in the global color table
        // if(global_color == 0x7FFF) return -1;
        if(global_color == 0x3FFF) return -1;

        // explode color
        retval = ncw_color_content(global_color, &r, &g, &b);

        if(retval == ERR)
        {
            // TODO:  handle color exhausion
            return -1;
        }

        // a black value indicates an unused color
        if((r + g + b) == 0) break;

        global_color++;
    }

    _vterm_align_color_band(vterm, &red, &green, &blue);

    if((red + green + blue) == COLOR_BLACK) return COLOR_BLACK;

    mapped_color = (color_map_t *)calloc(1, sizeof(color_map_t));

    mapped_color->private_color = color;
    mapped_color->global_color = global_color;
    mapped_color->red = red;
    mapped_color->green = green;
    mapped_color->blue = blue;

    // convert traditional RGB values (0 - 255) to ncurses range of 0 - 1000
    red = RGB_TO_NCURSES(red);
    green = RGB_TO_NCURSES(green);
    blue = RGB_TO_NCURSES(blue);

    retval = ncw_init_color(global_color, red, green, blue);

    CDL_APPEND(vterm->color_map_head, mapped_color);

    return global_color;
}

short
vterm_get_mapped_rgb(vterm_t *vterm, int red, int green, int blue)
{
    color_map_t     *mapped_color;

    _vterm_align_color_band(vterm, &red, &green, &blue);

    CDL_FOREACH(vterm->color_map_head, mapped_color)
    {
        if( mapped_color->red == red &&
            mapped_color->green == green &&
            mapped_color->blue == blue)
        {
            return mapped_color->global_color;
        }
    }

    return -1;
}

short
vterm_get_mapped_color(vterm_t *vterm, short color)
{
    color_map_t     *mapped_color;

    // don't search for anonymous colors
    if(color == -1) return -1;

    CDL_SEARCH_SCALAR(vterm->color_map_head, mapped_color,
        private_color, color);

    if(mapped_color == NULL)
    {
        return -1;
    }

    if(mapped_color->private_color == color)
    {
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
        init_color(mapped_color->global_color, 0, 0, 0);

        free(mapped_color);
    }

    vterm->color_map_head = NULL;

    return;
}

void
_vterm_align_color_band(vterm_t *vterm, int *r, int *g, int *b)
{
    int     remainder;

    if(vterm == NULL) return;

    if(*r < 0) *r = 0;
    if(*g < 0) *g = 0;
    if(*b < 0) *b = 0;

    if(*r > 255) *r = 255;
    if(*g > 255) *g = 255;
    if(*b > 255) *b = 255;

    remainder = *r % vterm->rgb_step;
    if(remainder < vterm->rgb_half_step)
        *r -= remainder;
    else
        *r += (vterm->rgb_step - remainder);

    remainder = *g % vterm->rgb_step;
    if(remainder < vterm->rgb_half_step)
        *g -= remainder;
    else
        *g += (vterm->rgb_step - remainder);

    remainder = *b % vterm->rgb_step;
    if(remainder < vterm->rgb_half_step)
        *b -= remainder;
    else
        *b += vterm->rgb_step - remainder;

    return;
}

