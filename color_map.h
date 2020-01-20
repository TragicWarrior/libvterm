#ifndef _COLOR_MAP_H_
#define _COLOR_MAP_H_

typedef struct _color_map_s   color_map_t;

struct _color_map_s
{
    short           global_color;               /*
                                                    the internal color number
                                                    we have mapped the
                                                    private_color to.
                                                */

    short           private_color;              /*
                                                    the color that the guest
                                                    application is expecting
                                                    to use.
                                                */

    float           red;                        //  RGB values of the color
    float           green;
    float           blue;

    color_map_t     *next;                      //  next entry in the map
    color_map_t     *prev;                      //  prev entry in the map
};

#endif
