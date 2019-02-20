
#ifndef _VTERM_BUFFER_H_
#define _VTERM_BUFFER_H_

#include <string.h>

#include "vterm.h"

enum
{
    VTERM_BUFFER_STD    =   0x00,
    VTERM_BUFFER_ALT,
};

void    vterm_buffer_alloc(vterm_t *vterm, int idx, int width, int height);

void    vterm_buffer_realloc(vterm_t *vterm, int idx, int width, int height);

void    vterm_buffer_dealloc(vterm_t *vterm, int idx);

int     vterm_buffer_set_active(vterm_t *vterm, int idx);

int     vterm_buffer_get_active(vterm_t *vterm);

#define VCELL_ZERO_ALL(_cell) \
            { memset(&_cell, 0, sizeof(_cell)); }

#define VCELL_SET_CHAR(_cell, _ch) \
            { \
                swprintf(_cell.wch, 2, L"%c", _ch); \
                setcchar(&_cell.uch, _cell.wch, 0, 0, NULL); \
            }

#define VCELL_SET_ATTR(_cell, _attr) \
                { _cell.attr = _attr; }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
                { *_attr_ptr = _cell.attr; }

#define VCELL_SET_COLORS(_cell, _desc) \
                { \
                    _cell.colors = _desc->colors; \
                    _cell.f_rgb[0] = _desc->f_rgb[0]; \
                    _cell.f_rgb[1] = _desc->f_rgb[1]; \
                    _cell.f_rgb[2] = _desc->f_rgb[2]; \
                    _cell.b_rgb[0] = _desc->b_rgb[0]; \
                    _cell.b_rgb[1] = _desc->b_rgb[1]; \
                    _cell.b_rgb[2] = _desc->b_rgb[2]; \
                }

#define VCELL_SET_DEFAULT_COLORS(_cell, _desc) \
                { _cell.colors = _desc->default_colors; }

#define VCELL_GET_COLORS(_cell, _colors_ptr) \
                { *_colors_ptr = _cell.colors; }

#endif

