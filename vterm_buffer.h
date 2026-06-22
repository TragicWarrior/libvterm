
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

struct _vterm_desc_s;

/*
    fill a horizontal span (col_start..col_end inclusive) of one row
    with a template cell and mark the span dirty in one pass.  the
    erase-family handlers all funnel here; flavor (which attr, which
    colors) is the caller's explicit choice.  colors == -1 preserves
    each cell's existing colors while wch and attr are still
    overwritten (no caller currently uses this flavor: erase_row now
    passes a real pair for both reset_colors flavors so a recycled
    row never inherits a prior occupant's background).
*/
void    vterm_fill_span(struct _vterm_desc_s *v_desc, int row,
            int col_start, int col_end, wchar_t ch, attr_t attr,
            int colors);

// set dirty bits col_start..col_end (inclusive) of one row
void    vterm_dirty_set_span(struct _vterm_desc_s *v_desc, int row,
            int col_start, int col_end);


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

/*
    whole-cell store: one address computation and ONE dirty-bit RMW.
    the single-field macros below each redo the cells[r][c] walk and
    the dirty update -- and because the dirty store goes through a
    uint8_t (character type), it may legally alias the row-pointer
    arrays, forcing the compiler to reload them between consecutive
    macros.  use this on per-glyph paths.
*/
#define VCELL_SET_ALL(_desc, _r, _c, _ch, _attr, _colors) \
            { \
                vterm_cell_t *_vcell = &(_desc)->cells[(_r)][(_c)]; \
                _vcell->wch[0] = (_ch); \
                _vcell->wch[1] = L'\0'; \
                _vcell->attr = (_attr); \
                _vcell->colors = (_colors); \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_CHAR(_desc, _r, _c, _ch) \
            { \
                (_desc)->cells[(_r)][(_c)].wch[0] = (_ch); \
                (_desc)->cells[(_r)][(_c)].wch[1] = L'\0'; \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_SET_COLORS(_desc, _r, _c) \
            { \
                (_desc)->cells[(_r)][(_c)].colors = (_desc)->colors; \
                VCELL_DIRTY_SET((_desc), (_r), (_c)); \
            }

#define VCELL_GET_ATTR(_cell, _attr_ptr) \
                { *_attr_ptr = _cell.attr; }

#define VCELL_GET_COLORS(_cell, _colors_ptr) \
                { *_colors_ptr = _cell.colors; }

#endif

