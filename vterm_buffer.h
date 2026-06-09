
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

/*
    capture one row of src_idx into the scrollback ring: the row is
    copied over the OLDEST history row and the ring head advances.
*/
int     vterm_buffer_history_append(vterm_t *vterm, int src_idx,
            int src_row);


/*
    dirty-bit primitives.  dirty state lives in a side-table bitmap
    on vterm_desc_t (dirty_bits[row][col >> 3] bit (col & 7)) -- one
    bit per cell instead of a per-cell byte that wasted 4 bytes after
    struct padding.
*/

#define VCELL_DIRTY_ROW_BYTES(_cols)    (((_cols) + 7) >> 3)

#define VCELL_DIRTY_SET(_desc, _r, _c) \
            ((_desc)->dirty_bits[(_r)][(_c) >> 3] |= \
                (uint8_t)(1u << ((_c) & 7)))

#define VCELL_DIRTY_CLEAR(_desc, _r, _c) \
            ((_desc)->dirty_bits[(_r)][(_c) >> 3] &= \
                (uint8_t)~(1u << ((_c) & 7)))

#define VCELL_DIRTY_TEST(_desc, _r, _c) \
            (((_desc)->dirty_bits[(_r)][(_c) >> 3] >> ((_c) & 7)) & 1u)

#define VCELL_DIRTY_SET_ROW(_desc, _r) \
            memset((_desc)->dirty_bits[(_r)], 0xFF, \
                VCELL_DIRTY_ROW_BYTES((_desc)->cols))

#define VCELL_DIRTY_CLR_ROW(_desc, _r) \
            memset((_desc)->dirty_bits[(_r)], 0x00, \
                VCELL_DIRTY_ROW_BYTES((_desc)->cols))

#define VCELL_DIRTY_SET_RANGE(_desc, _r, _start, _count) \
            { \
                int _i; \
                for(_i = 0; _i < (_count); _i++) \
                    VCELL_DIRTY_SET((_desc), (_r), (_start) + _i); \
            }

/*
    the dirty bitmap is one contiguous block with rows
    VCELL_DIRTY_ROW_BYTES(max_cols) apart (alloc/realloc size it from
    the physical stride), so the whole-buffer set is a single memset.
    bits for offscreen tail columns (cols..max_cols-1) get set too --
    harmless: readers only test bits below cols, and a later re-widen
    wants the revealed columns repainted anyway.
*/
#define VCELL_ALL_SET_DIRTY(_vdesc) \
            { \
                __typeof__ (_vdesc) _vtmp = (_vdesc); \
                memset(_vtmp->dirty_bits[0], 0xFF, \
                    (size_t)_vtmp->rows * \
                    (size_t)VCELL_DIRTY_ROW_BYTES(_vtmp->max_cols)); \
            }


/*
    cell mutation macros.  every mutation also marks the cell dirty.
    they take (desc, row, col, ...) instead of a cell pointer so they
    can update the bitmap at the same site.
*/

#define VCELL_ZERO_ALL(_desc, _r, _c) \
            { \
                memset(&(_desc)->cells[(_r)][(_c)], 0, \
                    sizeof((_desc)->cells[(_r)][(_c)])); \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_CHAR(_desc, _r, _c, _ch) \
            { \
                (_desc)->cells[(_r)][(_c)].wch[0] = (_ch); \
                (_desc)->cells[(_r)][(_c)].wch[1] = L'\0'; \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_ATTR(_desc, _r, _c, _attr) \
            { \
                (_desc)->cells[(_r)][(_c)].attr = (_attr); \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_COLORS(_desc, _r, _c) \
            { \
                (_desc)->cells[(_r)][(_c)].colors = (_desc)->colors; \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_DEFAULT_COLORS(_desc, _r, _c) \
            { \
                (_desc)->cells[(_r)][(_c)].colors = (_desc)->default_colors; \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
                { *_attr_ptr = _cell.attr; }

#define VCELL_GET_COLORS(_cell, _colors_ptr) \
                { *_colors_ptr = _cell.colors; }

#endif

