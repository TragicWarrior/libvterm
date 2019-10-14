
#ifndef _VTERM_BUFFER_H_
#define _VTERM_BUFFER_H_

#include <string.h>
#include <stdio.h>
#include <wchar.h>

#include "vterm.h"

void    vterm_buffer_alloc(vterm_t *vterm, int idx, int width, int height);

void    vterm_buffer_realloc(vterm_t *vterm, int idx, int width, int height);

void    vterm_buffer_dealloc(vterm_t *vterm, int idx);

int     vterm_buffer_set_active(vterm_t *vterm, int idx);

int     vterm_buffer_get_active(vterm_t *vterm);

int     vterm_buffer_shift_up(vterm_t *vterm, int idx,
            int top_row, int bottom_row, int stride);

int     vterm_buffer_shift_down(vterm_t *vterm, int idx,
            int top_row, int bottom_row, int stride);

int     vterm_buffer_clone(vterm_t *vterm, int src_idx, int dst_idx,
            int src_offset, int dst_offset, int rows);


#define VCELL_ZERO_ALL(_cell) \
            { memset(&_cell, 0, sizeof(_cell)); }

#define VCELL_SET_CHAR(_cell, _ch) \
            { \
                _cell.wch[0] = _ch; \
                _cell.wch[1] = '\0';    \
            }

                // swprintf(_cell.wch, 2, L"%c", _ch);


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

