
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
                wchar_t             _wch[CCHARW_MAX]; \
                swprintf(_wch, CCHARW_MAX, L"%c", _ch); \
                memcpy(&_cell.wch, _wch, sizeof(_cell.wch)); \
                setcchar(&_cell.uch, _wch, 0, 0, NULL); \
            } \

#define VCELL_SET_ATTR(_cell, _attr) \
                { _cell.attr = _attr; }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
                { *_attr_ptr = _cell.attr; }

#define VCELL_SET_COLOR(_cell, _colors) \
                { _cell.colors = colors; }

#endif

