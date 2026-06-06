#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "nc_wrapper.h"
#include "color_map.h"
#include "utlist.h"

short
vterm_add_mapped_color(vterm_t *vterm, short color,
    float red, float green, float blue)
{
    color_map_t     *mapped_color;
    short           global_color;

    /*
        is color one of the standard colors?
        the init_color() does not permit the basic colors to be redefined
        and SGR codes are wooden in this regard.
    */
    if(color >= COLOR_BLACK && color <= COLOR_WHITE) return color;

    if(red < 0.0) red = 0.0;
    if(green < 0.0) green = 0.0;
    if(blue < 0.0) blue = 0.0;

    if((red + green + blue) == 0.0)
    {
        return COLOR_BLACK;
    }

    if(red > 255.0) red = 255.0;
    if(green > 255.0) green = 255.0;
    if(blue > 255.0) blue = 255.0;

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

    if(vterm->flags & VTERM_FLAG_TRUECOLOR)
    {
        static short next_color = 16;

        global_color = next_color;

        if(global_color >= COLORS) return -1;

        next_color++;

        mapped_color = (color_map_t *)calloc(1, sizeof(color_map_t));

        mapped_color->private_color = color;
        mapped_color->global_color = global_color;
        mapped_color->red = red;
        mapped_color->green = green;
        mapped_color->blue = blue;

        red = (red / 255.0) * 1000.0;
        green = (green / 255.0) * 1000.0;
        blue = (blue / 255.0) * 1000.0;

        ncw_init_color(global_color, (int)red, (int)green, (int)blue);

        CDL_APPEND(vterm->color_map_head, mapped_color);

        return global_color;
    }

    // non-truecolor: find nearest existing palette color
    {
        short nr, ng, nb;

        nr = (short)((red / 255.0) * 1000.0);
        ng = (short)((green / 255.0) * 1000.0);
        nb = (short)((blue / 255.0) * 1000.0);

        global_color = color_cache_find_nearest_color(nr, ng, nb);

        if(global_color < 0) return -1;

        mapped_color = (color_map_t *)calloc(1, sizeof(color_map_t));

        mapped_color->private_color = color;
        mapped_color->global_color = global_color;
        mapped_color->red = red;
        mapped_color->green = green;
        mapped_color->blue = blue;

        CDL_APPEND(vterm->color_map_head, mapped_color);

        return global_color;
    }
}

short
vterm_get_mapped_rgb(vterm_t *vterm, float red, float green, float blue)
{
    color_map_t     *mapped_color;


    CDL_FOREACH(vterm->color_map_head, mapped_color)
    {
        if( (int)mapped_color->red == (int)red &&
            (int)mapped_color->green == (int)green &&
            (int)mapped_color->blue == (int)blue)
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
        free(mapped_color);
    }

    vterm->color_map_head = NULL;

    return;
}
