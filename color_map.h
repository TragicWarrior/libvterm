#ifndef _COLOR_MAP_H_
#define _COLOR_MAP_H_

typedef struct _color_map_s   color_map_t;

struct _color_map_s
{
    int             global_color;               /*
                                                    the internal color number
                                                    we have mapped the
                                                    private_color to.
                                                */

    int             private_color;              /*
                                                    the color that the guest
                                                    application is expecting
                                                    to use.
                                                */

    int             r, g, b;                    //  RGB values of the color
    float           h, s, l;                    //  HSL values of the color
    float           L, A, B;                    //  CIELAB values of the color

    color_map_t     *next;                      //  next entry in the map
    color_map_t     *prev;                      //  prev entry in the map
};

// init an seed the mapped color system
void vterm_color_map_init(vterm_t *vterm);

#endif
