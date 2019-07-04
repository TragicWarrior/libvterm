#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "utlist.h"

short
vterm_add_mapped_color(vterm_t *vterm, short color,
    float red, float green, float blue)
{
    vterm_cmap_t    *mapped_color;
    short           global_color;
    short           r, g, b;

    // color must not be one of the pre-defined colors
    if(color < 8) return -1;

    // check to see if color is already mapped
    global_color = vterm_get_mapped_color(vterm, color, red, green, blue);

    if(global_color > 0) return global_color;

    global_color = 8;

    // look for a free color number in the global color table
    for(;;)
    {
        // explode color
        color_content(global_color, &r, &g, &b);

        if(r == 0 && g == 0 && b == 0) break;

        // no free color numbers in the global color table
        if(global_color == 0x7FFF) return -1;

        global_color++;
    }

    mapped_color = (vterm_cmap_t *)calloc(1, sizeof(vterm_cmap_t));

    mapped_color->private_color = color;
    mapped_color->global_color = global_color;
    mapped_color->red = red;
    mapped_color->green = green;
    mapped_color->blue = blue;

    // convert traditional RGB values (0 - 255) to ncurses range of 0 - 1000
    red = (red / 255.0) * 1000.0;
    green = (green / 255.0) * 1000.0;
    blue = (blue / 255.0) * 1000.0;

    init_color(global_color, (short)red, (short)green, (short)blue);

    // endwin();
    // printf("%d %d %d \n\r", (short)red, (short)green, (short)blue);
    // fflush(stdout);
    // exit(0);

    CDL_APPEND(vterm->cmap_head, mapped_color);

    return global_color;
}

short
vterm_get_mapped_color(vterm_t *vterm, short color,
    float red, float green, float blue)
{
    vterm_cmap_t    *mapped_color;

    if((red + green + blue) > 0)
    {
        CDL_FOREACH(vterm->cmap_head, mapped_color)
        {
            if(mapped_color->red == red &&
                mapped_color->green == green &&
                mapped_color->blue == blue)
            {
                return mapped_color->global_color;
            }
        } 

        return -1;
    }

    CDL_SEARCH_SCALAR(vterm->cmap_head, mapped_color, private_color, color);

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
    vterm_cmap_t    *mapped_color;
    vterm_cmap_t    *tmp1;
    vterm_cmap_t    *tmp2;

    if(vterm == NULL) return;

    if(vterm->cmap_head == NULL) return;

    CDL_FOREACH_SAFE(vterm->cmap_head, mapped_color, tmp1, tmp2)
    {
        // set the color back to 0-0-0 to indicate that it's free
        init_color(mapped_color->global_color, 0, 0, 0);

        free(mapped_color);
    }

    vterm->cmap_head = NULL;

    return;
}
