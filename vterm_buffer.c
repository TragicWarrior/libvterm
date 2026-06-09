
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_cursor.h"
#include "vterm_buffer.h"

static vterm_cell_t** _vterm_buffer_alloc_raw(int rows, int cols);

/*
    Contiguous matrix allocators.  The cell data for an entire buffer
    lives in ONE block; cells[r] points STRIDE cells apart into it.
    cells[0] is therefore the block base -- dealloc frees cells[0] (the
    whole block) plus the row-pointer array.  This holds because nothing
    ever swaps/rotates row pointers (scroll/clone memcpy contents, never
    pointers), so cells[0] stays the base for the buffer's lifetime.
    Wins: one malloc header instead of N, contiguous rows for cache
    locality during scroll/erase, less fragmentation.

    NOTE: vterm_copy_buffer keeps the per-row _vterm_buffer_alloc_raw
    below -- its returned buffer is freed per-row by the caller (vwm),
    so that allocation must stay per-row to preserve the public
    free contract.
*/
static vterm_cell_t**
_vterm_cells_alloc_contig(int rows, int stride)
{
    vterm_cell_t    **ptrs;
    vterm_cell_t    *block;
    int             r;

    if(rows < 1 || stride < 1) return NULL;

    ptrs  = (vterm_cell_t **)calloc((size_t)rows, sizeof(vterm_cell_t *));
    block = (vterm_cell_t *)calloc((size_t)rows * (size_t)stride,
                sizeof(vterm_cell_t));

    if(ptrs == NULL || block == NULL)
    {
        free(ptrs);
        free(block);
        return NULL;
    }

    for(r = 0; r < rows; r++)
        ptrs[r] = block + (size_t)r * (size_t)stride;

    return ptrs;
}

static uint8_t**
_vterm_dirty_alloc_contig(int rows, int row_bytes)
{
    uint8_t **ptrs;
    uint8_t  *block;
    int      r;

    if(rows < 1 || row_bytes < 1) return NULL;

    ptrs  = (uint8_t **)calloc((size_t)rows, sizeof(uint8_t *));
    block = (uint8_t *)calloc((size_t)rows * (size_t)row_bytes, 1);

    if(ptrs == NULL || block == NULL)
    {
        free(ptrs);
        free(block);
        return NULL;
    }

    for(r = 0; r < rows; r++)
        ptrs[r] = block + (size_t)r * (size_t)row_bytes;

    return ptrs;
}

void
vterm_buffer_alloc(vterm_t *vterm, int idx, int width, int height)
{
    vterm_desc_t    *v_desc;

    if(vterm == NULL) return;

    if(width < 0 || height < 0) return;

    v_desc = &vterm->vterm_desc[idx];

    v_desc->cells = _vterm_cells_alloc_contig(height, width);
    v_desc->dirty_bits = _vterm_dirty_alloc_contig(height,
        VCELL_DIRTY_ROW_BYTES(width));

    v_desc->rows = height;
    v_desc->cols = width;
    v_desc->max_cols = width;

    v_desc->scroll_min = 0;
    v_desc->scroll_max = height - 1;

    return;
}

void
vterm_buffer_realloc(vterm_t *vterm, int idx, int width, int height)
{
    vterm_desc_t    *v_desc;
    vterm_cell_t    **old_cells;
    uint8_t         **old_dirty;
    vterm_cell_t    **new_cells;
    uint8_t         **new_dirty;
    int             old_rows;
    int             max_cols_old;       // == old physical row stride
    int             old_dbytes;
    int             new_stride;         // ratcheted physical row width
    int             new_dbytes;
    int             copy_rows;
    int             copy_cell_bytes;
    int             r, c;

    if(vterm == NULL) return;

    if(width == 0 || height == 0) return;
    if(width == -1 && height == -1) return;

    // set the vterm description buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // a -1 means the caller wants to keep an existing dimension
    if(width == -1) width = v_desc->cols;
    if(height == -1) height = v_desc->rows;

    old_rows     = v_desc->rows;
    max_cols_old = v_desc->max_cols;
    old_dbytes   = VCELL_DIRTY_ROW_BYTES(max_cols_old);

    /*
        physical row width never shrinks (the historical max_cols ratchet).
        a request narrower than the historical max keeps the wider stride;
        only the logical width (cols) drops.
    */
    new_stride = (width > max_cols_old) ? width : max_cols_old;
    new_dbytes = VCELL_DIRTY_ROW_BYTES(new_stride);

    old_cells = v_desc->cells;
    old_dirty = v_desc->dirty_bits;

    /*
        allocate fresh contiguous blocks at the new geometry, copy the
        surviving content across, then free the old blocks.  resize is a
        rare, human-driven event -- not a hot path -- so the extra
        alloc+copy buys a far simpler, race-free path than reshaping a
        contiguous block in place (which has to reason about row-stride
        changes and overlapping moves).  the descriptor is only ever
        observed in a fully-consistent state.
    */
    new_cells = _vterm_cells_alloc_contig(height, new_stride);
    new_dirty = _vterm_dirty_alloc_contig(height, new_dbytes);

    if(new_cells == NULL || new_dirty == NULL)
    {
        // allocation failed -- leave the buffer untouched
        if(new_cells != NULL) { free(new_cells[0]); free(new_cells); }
        if(new_dirty != NULL) { free(new_dirty[0]); free(new_dirty); }
        return;
    }

    // copy surviving rows -- content and dirty bits -- from old to new.
    // stride only ever grows, so an old row always fits in a new row.
    copy_rows = (old_rows < height) ? old_rows : height;
    copy_cell_bytes = max_cols_old * (int)sizeof(vterm_cell_t);

    for(r = 0; r < copy_rows; r++)
    {
        memcpy(new_cells[r], old_cells[r], (size_t)copy_cell_bytes);
        memcpy(new_dirty[r], old_dirty[r], (size_t)old_dbytes);
    }

    // release the old blocks (cells[0]/dirty_bits[0] are the block bases)
    free(old_cells[0]);
    free(old_cells);
    free(old_dirty[0]);
    free(old_dirty);

    /*
        commit the new geometry BEFORE any VCELL_* fill so the dirty-bit
        macros (which index off v_desc->cells / v_desc->dirty_bits) see a
        consistent descriptor.
    */
    v_desc->cells      = new_cells;
    v_desc->dirty_bits = new_dirty;
    v_desc->rows       = height;
    v_desc->cols       = width;
    v_desc->max_cols   = new_stride;

    // blank brand-new rows entirely (space-fill marks each cell dirty)
    for(r = copy_rows; r < height; r++)
    {
        for(c = 0; c < new_stride; c++)
            VCELL_SET_CHAR(v_desc, r, c, ' ');
    }

    // blank newly-added columns on surviving rows when the stride grew
    if(new_stride > max_cols_old)
    {
        for(r = 0; r < copy_rows; r++)
        {
            for(c = max_cols_old; c < new_stride; c++)
            {
                VCELL_ZERO_ALL(v_desc, r, c);
                VCELL_SET_CHAR(v_desc, r, c, ' ');
            }
        }
    }

    v_desc->scroll_max = height - 1;

    /*
        scroll_max is reset above, but scroll_min must be brought back into
        range too.  shrinking the buffer can leave a previously-set DECSTBM
        top margin pointing at or past the new last row.  a stale scroll_min
        then drives out-of-bounds row writes -- interpret_csi_SD's clear
        loop, origin-mode cursor home, and vterm_scroll_down all index
        cells[] starting at scroll_min and would walk past the row pointer
        array, corrupting the heap.  when the old region no longer fits,
        drop it back to full height.
    */
    if(v_desc->scroll_min > v_desc->scroll_max)
    {
        v_desc->scroll_min = 0;
        v_desc->buffer_state &= ~STATE_SCROLL_SHORT;
    }

    return;
}


void
vterm_buffer_dealloc(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc;

    if(vterm == NULL) return;

    v_desc = &vterm->vterm_desc[idx];

    /*
        cells/dirty_bits are each one contiguous block reached through a
        row-pointer array.  [0] is the block base (rows are never
        swapped), so freeing [0] releases the whole block.  guard each
        independently so a partial alloc failure can't deref a NULL base.
    */
    if(v_desc->cells != NULL)
    {
        free(v_desc->cells[0]);
        free(v_desc->cells);
        v_desc->cells = NULL;
    }

    if(v_desc->dirty_bits != NULL)
    {
        free(v_desc->dirty_bits[0]);
        free(v_desc->dirty_bits);
        v_desc->dirty_bits = NULL;
    }

    return;
}

int
vterm_buffer_set_active(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc = NULL;
    int             curr_idx;
    struct winsize  ws = {.ws_xpixel = 0, .ws_ypixel = 0};
    int             width;
    int             height;
    int             std_width, std_height;
    int             curr_width, curr_height;

    if(vterm == NULL) return -1;
    if(idx != VTERM_BUF_STANDARD && idx != VTERM_BUF_ALTERNATE) return -1;

    curr_idx = vterm_buffer_get_active(vterm);

    /*
        check to see if current buffer index is the same as the
        requested one.  if so, this is a no-op.
    */
    if(idx == curr_idx) return 0;

    // run the event hook if installed
    if(vterm->event_mask & VTERM_MASK_BUFFER_PREFLIP)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm, VTERM_EVENT_BUFFER_PREFLIP,
                (void *)&idx);
        }
    }

    /*
        get current terminal size using the best methods available.  typically,
        that means using TIOCGWINSZ ioctl which is pretty portable.  an
        alternative method could be getmaxyx() which is found in most curses
        implementations.  however, that causes problems for uses of libvterm
        where the rendering apparatus is not a curses WINDOW.
    */
    ioctl(vterm->pty_fd, TIOCGWINSZ, &ws);
    height = ws.ws_row;
    width = ws.ws_col;

    // treat the standard buffer special -- it never goes away
    if(idx == VTERM_BUF_STANDARD)
    {
        /*
            if the current buffer not the STD buffer, we need to handle
            a few housekeeping items and then tear down the other
            buffer before switching.
        */
        if(curr_idx == VTERM_BUF_ALTERNATE)
        {
            // check to see if a resize happened
            std_width = vterm->vterm_desc[VTERM_BUF_STANDARD].cols;
            std_height = vterm->vterm_desc[VTERM_BUF_STANDARD].rows;
            curr_width = vterm->vterm_desc[curr_idx].cols;
            curr_height = vterm->vterm_desc[curr_idx].rows;

            if(std_height != curr_height || std_width != curr_width)
            {
                vterm_buffer_realloc(vterm, VTERM_BUF_STANDARD,
                    curr_width, curr_height);
            }

            // run the event hook if installed
            if(vterm->event_mask & VTERM_MASK_BUFFER_DEACTIVATED)
            {
                if(vterm->event_hook != NULL)
                {
                    vterm->event_hook(vterm, VTERM_EVENT_BUFFER_DEACTIVATED,
                        (void *)&curr_idx);
                }
            }

            vterm_buffer_shift_up(vterm, VTERM_BUF_HISTORY,
                -1, -1, curr_height);
            vterm_buffer_clone(vterm,
                VTERM_BUF_ALTERNATE, VTERM_BUF_HISTORY, 0,
                vterm->vterm_desc[VTERM_BUF_HISTORY].rows - curr_height,
                curr_height);

            // destroy the old buffer
            vterm_buffer_dealloc(vterm, VTERM_BUF_ALTERNATE);

            vterm_cursor_show(vterm, VTERM_BUF_STANDARD);
        }
    }

    /*
        Switch to the ALT buffer.

        given the above conditional, this could have been an if-else.
        however, this is more readable and it should optimize
        about the same.
    */
    if(idx != VTERM_BUF_STANDARD)
    {
        vterm_buffer_alloc(vterm, idx, width, height);
        v_desc = &vterm->vterm_desc[idx];

        // copy some defaults from standard buffer
        v_desc->default_colors =
            vterm->vterm_desc[VTERM_BUF_STANDARD].default_colors;

        // erase the newly created buffer
        vterm_erase(vterm, idx, 0);
    }

    // update the vterm buffer desc index and the cached active pointer.
    // these two MUST stay in sync -- v_desc_active is read on hot paths.
    vterm->vterm_desc_idx = idx;
    vterm->v_desc_active = &vterm->vterm_desc[idx];
    v_desc = vterm->v_desc_active;
    VCELL_ALL_SET_DIRTY(v_desc);

    // run the event hook if installed
    if(vterm->event_mask & VTERM_MASK_BUFFER_ACTIVATED)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm, VTERM_EVENT_BUFFER_ACTIVATED,
                (void *)&idx);
        }
    }

    return 0;
}

inline int
vterm_buffer_get_active(vterm_t *vterm)
{
    if(vterm == NULL) return -1;

    return vterm->vterm_desc_idx;
}

int
vterm_buffer_shift_up(vterm_t *vterm, int idx,
    int top_row, int bottom_row, int stride)
{
    vterm_desc_t    *v_desc = NULL;
    int             rows_moved;

    // set vterm desc buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // compute real values when -1 is supplied
    if(top_row == -1) top_row = 0;
    if(bottom_row == -1) bottom_row = v_desc->rows - 1;

    if(bottom_row - top_row < 1) return -1;

    /*
        rows that survive the shift.  a stride >= the region height
        shifts everything out -- nothing to copy (callers blank the
        vacated rows).
    */
    rows_moved = bottom_row - top_row - stride + 1;

    if(rows_moved > 0)
    {
        if(v_desc->cols == v_desc->max_cols)
        {
            /*
                un-ratcheted (the common case): rows are contiguous in
                one block (_vterm_cells_alloc_contig), so the whole
                shift is a single overlapping move.
            */
            memmove(v_desc->cells[top_row],
                v_desc->cells[top_row + stride],
                (size_t)rows_moved * (size_t)v_desc->max_cols
                    * sizeof(vterm_cell_t));
        }
        else
        {
            /*
                ratcheted stride: physical rows are wider than the
                visible width.  copy only the visible span -- a full-
                stride block move multiplies the bytes copied by
                max_cols/cols and blows test_resize's ASan watchdog in
                its ratchet storm.  ascending order: dst row is always
                below src row.
            */
            size_t  col_mem = (size_t)v_desc->cols * sizeof(vterm_cell_t);
            int     r;

            for(r = 0; r < rows_moved; r++)
            {
                memcpy(v_desc->cells[top_row + r],
                    v_desc->cells[top_row + stride + r], col_mem);
            }
        }
    }

    /*
        scrolling the buffer always invalidates all rows.  skip the
        operation for the history buffer because we don't maintain the
        dirty flags on that one.
    */
    if(idx != VTERM_BUF_HISTORY)
    {
        VCELL_ALL_SET_DIRTY(v_desc);
    }

    return 0;
}

int
vterm_buffer_shift_down(vterm_t *vterm, int idx,
    int top_row, int bottom_row, int stride)
{
    vterm_desc_t    *v_desc = NULL;
    int             rows_moved;

    // set vterm desc buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // compute real values when -1 is supplied
    if(top_row == -1) top_row = 0;
    if(bottom_row == -1) bottom_row = v_desc->rows - 1;

    if(bottom_row - top_row < 1) return -1;

    /*
        rows that survive the shift; rows [top_row .. bottom_row - stride]
        land on [top_row + stride .. bottom_row].  a stride >= the region
        height shifts everything out -- nothing to copy.
    */
    rows_moved = bottom_row - top_row - stride + 1;

    // see shift_up for the fast-path / ratchet rationale
    if(rows_moved > 0)
    {
        if(v_desc->cols == v_desc->max_cols)
        {
            memmove(v_desc->cells[top_row + stride],
                v_desc->cells[top_row],
                (size_t)rows_moved * (size_t)v_desc->max_cols
                    * sizeof(vterm_cell_t));
        }
        else
        {
            /*
                descending order: dst row is always above src row, so
                the highest destination must be written first.
            */
            size_t  col_mem = (size_t)v_desc->cols * sizeof(vterm_cell_t);
            int     r;

            for(r = rows_moved - 1; r >= 0; r--)
            {
                memcpy(v_desc->cells[top_row + stride + r],
                    v_desc->cells[top_row + r], col_mem);
            }
        }
    }

    /*
        scrolling the buffer always invalidates all rows.  skip the
        operation for the history buffer because we don't maintain the
        dirty flags on that one.
    */
    if(idx != VTERM_BUF_HISTORY)
    {
        VCELL_ALL_SET_DIRTY(v_desc);
    }

    return 0;
}

int
vterm_buffer_clone(vterm_t *vterm, int src_idx, int dst_idx,
     int src_offset, int dst_offset, int rows)
{
    vterm_desc_t    *v_desc_src = NULL;
    vterm_desc_t    *v_desc_dst = NULL;
    vterm_cell_t    **vcell_src;
    vterm_cell_t    **vcell_dst;
    int             col_mem;
    int             stride;
    int             r;

    if(vterm == NULL) return -1;

    v_desc_src = &vterm->vterm_desc[src_idx];
    v_desc_dst = &vterm->vterm_desc[dst_idx];

    vcell_src = &v_desc_src->cells[src_offset];
    vcell_dst = &v_desc_dst->cells[dst_offset];

    stride = USE_MIN(v_desc_src->max_cols, v_desc_dst->max_cols);

    col_mem = sizeof(vterm_cell_t) * stride;

    for(r = 0; r < rows; r++)
    {
        memcpy(*vcell_dst, *vcell_src, col_mem);

        // mark the cloned cells dirty in the destination's bitmap
        VCELL_DIRTY_SET_RANGE(v_desc_dst, dst_offset + r, 0, stride);

        vcell_src++;
        vcell_dst++;
    }

    return 0;
}

vterm_cell_t**
vterm_copy_buffer(vterm_t *vterm, int *rows, int *cols)
{
    vterm_desc_t    *v_desc = NULL;
    vterm_cell_t    **buffer;
    int             r;

    if(vterm == NULL) return NULL;
    if(rows == NULL || cols == NULL) return NULL;

    // set vterm desc buffer selector
    v_desc = vterm->v_desc_active;

    *rows = v_desc->rows;
    *cols = v_desc->max_cols;

    buffer = _vterm_buffer_alloc_raw(*rows, *cols);

    for(r = 0; r < *rows; r++)
    {
        // mem copy one row at a time
        memcpy(&buffer[r][0], &v_desc->cells[r][0],
            v_desc->cols * sizeof(vterm_cell_t));
    }

    return buffer;
}

vterm_cell_t **
_vterm_buffer_alloc_raw(int rows, int cols)
{
    vterm_cell_t    **buffer;
    int             r;

    buffer = (vterm_cell_t **)calloc(rows, sizeof(vterm_cell_t*));

    for(r = 0; r < rows; r++)
    {
        buffer[r] = (vterm_cell_t *)calloc(cols, sizeof(vterm_cell_t));
    }

    return buffer;
}


