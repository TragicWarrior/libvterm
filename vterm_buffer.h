
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
            { \
                memset(&_cell, 0, sizeof(_cell)); \
                _cell.dirty = 1; \
            }

#define VCELL_ROW_SET_DIRTY(_cell, _stride) \
            { \
                __typeof__ (_cell) _ctmp = (_cell); \
                __typeof__ (_stride) _stmp= (_stride); \
                do \
                { \
                    _ctmp->dirty = 1; \
                    _ctmp++; \
                    _stmp--; \
                } \
                while(_stmp > 0); \
            }


#define VCELL_ROW_SET_CLEAN(_cell, _stride) \
            { \
                __typeof__ (_cell) _ctmp = (_cell); \
                __typeof__ (_stride) _stmp = (_stride); \
                do \
                { \
                    _ctmp->dirty = 0; \
                    _ctmp++; \
                    _stmp--; \
                } \
                while(_stmp > 0); \
            }

#define VCELL_ALL_SET_DIRTY(_vdesc) \
            { \
                __typeof__ (_vdesc) _vtmp = (_vdesc); \
                for(int _r = 0; _r < _vtmp->rows; _r++) \
                { \
                    VCELL_ROW_SET_DIRTY(_vtmp->cells[_r], _vtmp->cols); \
                } \
            }

#define VCELL_SET_CHAR(_cell, _ch) \
            { \
                _cell.wch[0] = _ch; \
                _cell.wch[1] = '\0'; \
                _cell.dirty = 1; \
            }

#define VCELL_SET_ATTR(_cell, _attr) \
                { \
                    _cell.attr = _attr; \
                    _cell.dirty = 1 ; \
                }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
                { *_attr_ptr = _cell.attr; }

#define VCELL_SET_COLORS(_cell, _desc) \
                { \
                    _cell.colors = _desc->colors; \
                    _cell.fg = _desc->fg; \
                    _cell.bg = _desc->bg; \
                    _cell.f_rgb[0] = _desc->f_rgb[0]; \
                    _cell.f_rgb[1] = _desc->f_rgb[1]; \
                    _cell.f_rgb[2] = _desc->f_rgb[2]; \
                    _cell.b_rgb[0] = _desc->b_rgb[0]; \
                    _cell.b_rgb[1] = _desc->b_rgb[1]; \
                    _cell.b_rgb[2] = _desc->b_rgb[2]; \
                    _cell.dirty = 1; \
                }

#define VCELL_SET_DEFAULT_COLORS(_cell, _desc) \
                { \
                    _cell.colors = _desc->default_colors; \
                    _cell.dirty = 1; \
                }

#define VCELL_GET_COLORS(_cell, _colors_ptr) \
                { *_colors_ptr = _cell.colors; }

#endif

