
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_cursor.h"
#include "vterm_buffer.h"

static vterm_cell_t** _vterm_buffer_alloc_raw(int rows, int cols);

void
vterm_buffer_alloc(vterm_t *vterm, int idx, int width, int height)
{
    vterm_desc_t    *v_desc;

    if(vterm == NULL) return;

    if(width < 0 || height < 0) return;

    v_desc = &vterm->vterm_desc[idx];

    v_desc->cells = _vterm_buffer_alloc_raw(height, width);

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
    int             delta_y = 0;
    int             start_x = 0;
    int             max_cols_old;
    int             new_width;
    uint16_t        r;
    uint16_t        c;

    if(vterm == NULL) return;

    if(width == 0 || height == 0) return;
    if(width == -1 && height == -1) return;

    // set the vterm description buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // a -1 means the caller wants to keep an existing dimension
    if(width == -1) width = v_desc->cols;
    if(height == -1) height = v_desc->rows;

    delta_y = height - v_desc->rows;

    // true up total colum count and never reduce buffer row width
    max_cols_old = v_desc->max_cols;
    new_width = width;

    if(new_width < v_desc->max_cols) new_width = v_desc->max_cols;
    if(new_width >= v_desc->max_cols) v_desc->max_cols = new_width;

    // realloc to accomodate the new matrix size
    v_desc->cells = (vterm_cell_t**)realloc(v_desc->cells,
        sizeof(vterm_cell_t*) * height);

    for(r = 0; r < height; r++)
    {
        // when adding new rows, we can just calloc() them.
        if((delta_y > 0) && (r > (v_desc->rows - 1)))
        {
            v_desc->cells[r] =
                (vterm_cell_t*)calloc(1, (sizeof(vterm_cell_t) * new_width));

            // fill new row with blanks
            for(c = 0; c < new_width; c++)
            {
                VCELL_SET_CHAR(v_desc->cells[r][c], ' ');
            }

            continue;
        }

        // this handles existing rows
        v_desc->cells[r] = (vterm_cell_t*)realloc(v_desc->cells[r],
            sizeof(vterm_cell_t) * new_width);

        // fill new cols with blanks
        start_x = max_cols_old - 1;
        for(c = start_x; c < new_width; c++)
        {
            VCELL_ZERO_ALL(v_desc->cells[r][c]);
            VCELL_SET_CHAR(v_desc->cells[r][c], ' ');
        }
    }

    v_desc->rows = height;
    v_desc->cols = width;

    v_desc->scroll_max = height - 1;

    return;
}


void
vterm_buffer_dealloc(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc;
    int             r;

    if(vterm == NULL) return;

    v_desc = &vterm->vterm_desc[idx];

    // prevent a double-free
    if(v_desc->cells == NULL) return;

    for(r = 0; r < v_desc->rows; r++)
    {
        free(v_desc->cells[r]);
        v_desc->cells[r] = NULL;
    }

    free(v_desc->cells);
    v_desc->cells = NULL;

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

    // update the vterm buffer desc index
    vterm->vterm_desc_idx = idx;
    v_desc = &vterm->vterm_desc[idx];
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
    vterm_cell_t    **vcell_src;
    vterm_cell_t    **vcell_dst;
    int             curr_row;
    int             region;
    int             col_mem;
    int             r;

    // set vterm desc buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // computer real values when -1 is supplied
    if(top_row == -1) top_row = 0;
    if(bottom_row == -1) bottom_row = v_desc->rows - 1;

    region = bottom_row - top_row;
    if(region < 1) return -1;

    curr_row = top_row + stride;

    vcell_src = &v_desc->cells[curr_row];
    vcell_dst = &v_desc->cells[top_row];

    col_mem = sizeof(vterm_cell_t) * v_desc->cols;

    for(r = 0; r < region; r++)
    {
        if(curr_row > bottom_row) break;

        memcpy(*vcell_dst, *vcell_src, col_mem);

        curr_row++;
        vcell_src++;
        vcell_dst++;
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
    vterm_cell_t    **vcell_src;
    vterm_cell_t    **vcell_dst;
    int             region;
    int             col_mem;
    int             r;

    // set vterm desc buffer selector
    v_desc = &vterm->vterm_desc[idx];

    // computer real values when -1 is supplied
    if(top_row == -1) top_row = 0;
    if(bottom_row == -1) bottom_row = v_desc->rows - 1;

    region = bottom_row - top_row;
    if(region < 1) return -1;

    vcell_src = &v_desc->cells[bottom_row - stride];
    vcell_dst = &v_desc->cells[bottom_row];

    col_mem = sizeof(vterm_cell_t) * v_desc->cols;

    for(r = 0; r < region; r++)
    {
        memcpy(*vcell_dst, *vcell_src, col_mem);

        vcell_src--;
        vcell_dst--;
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

        VCELL_ROW_SET_DIRTY(*vcell_dst, stride);

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
    int             idx;
    int             r;

    if(vterm == NULL) return NULL;
    if(rows == NULL || cols == NULL) return NULL;

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    *rows = v_desc->rows;
    *cols = v_desc->max_cols;

    buffer = _vterm_buffer_alloc_raw(*rows, *cols);

    for(r = 0; r < *rows; r++)
    {
        // mem copy one row at a time
        memcpy(&buffer[r][0], &v_desc->cells[r][0],
            v_desc->cols * sizeof(vterm_cell_t));

        VCELL_ROW_SET_DIRTY(buffer[r], v_desc->cols);
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

        VCELL_ROW_SET_DIRTY(buffer[r], cols);
    }

    return buffer;
}


