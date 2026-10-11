
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_render.h"
#include "vtty_buffer.h"

void
vtty_erase(vtty_t *vtty, int idx, char fill_char)
{
    vtty_desc_t     *v_desc = NULL;
    int             r;

    if(vtty == NULL) return;

    // use a space if no fill char is specified
    if(fill_char == 0) fill_char = ' ';

    // a value of -1 means current, active buffer
    if(idx == -1)
    {
        v_desc = vtty->v_desc_active;
    }
    else
    {
        v_desc = &vtty->vtty_desc[idx];
    }

    // post event that term is about to be erased
    if(vtty->event_mask & VTTY_MASK_TERM_PRECLEAR)
    {
        if(vtty->event_hook != NULL)
        {
            vtty->event_hook(vtty,
                VTTY_EVENT_TERM_PRECLEAR, NULL);
        }
    }

    for(r = 0;r < v_desc->rows; r++)
    {
        vtty_fill_span(v_desc, r, 0, v_desc->cols - 1,
            (wchar_t)fill_char, A_NORMAL, v_desc->default_colors);
    }

    return;
}

void
vtty_erase_row(vtty_t *vtty, int row, bool reset_colors, char fill_char)
{
    vtty_desc_t     *v_desc = NULL;

    if(vtty == NULL) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    if(row == -1) row = v_desc->crow;

    /*
        reset_colors TRUE  -> blank with the default pair (NEL / RI).
        reset_colors FALSE -> blank with the CURRENT pair (BCE), matching
        EL/ED/ECH/SU/SD/IL/DL and xterm.  This path clears the row a
        newline-scroll just recycled; using -1 (preserve each cell's old
        colors) left the stale background of whatever previously occupied
        that row -- e.g. an inline-code span's gray bg -- and apps that
        rely on the scrolled-in line being clean (glow, which emits no
        \e[K after redrawing) showed it bleeding down the screen.
    */
    vtty_fill_span(v_desc, row, 0, v_desc->cols - 1,
        (wchar_t)fill_char, A_NORMAL,
        reset_colors == TRUE ? v_desc->default_colors : v_desc->colors);

    return;
}

void
vtty_erase_rows(vtty_t *vtty, int start_row, char fill_char)
{
    vtty_desc_t     *v_desc = NULL;

    if(vtty == NULL) return;
    if(start_row < 0) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    while(start_row < v_desc->rows)
    {
        vtty_erase_row(vtty, start_row, FALSE, fill_char);
        start_row++;
    }

    return;
}

void
vtty_erase_col(vtty_t *vtty, int col, char fill_char)
{
    vtty_desc_t     *v_desc = NULL;
    int             r;

    if(vtty == NULL) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    if(col == -1) col = v_desc->ccol;

    /* same pending-wrap guard as the cursor draw in vtty_wnd.c: ccol can
       rest at cols, and an out-of-range col would store past each row */
    if(col >= v_desc->cols) col = v_desc->cols - 1;
    if(col < 0) col = 0;

    // a vertical 1-wide span: one whole-cell store per row
    for(r = 0; r < v_desc->rows; r++)
    {
        VCELL_SET_ALL(v_desc, r, col, (wchar_t)fill_char, A_NORMAL,
            v_desc->default_colors);
    }

    return;
}

void
vtty_erase_cols(vtty_t *vtty, int start_col, char fill_char)
{
    vtty_desc_t     *v_desc = NULL;
    int             r;

    if(vtty == NULL) return;
    if(start_col < 0) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    // row-major: one span per row instead of one full-rows pass per column
    for(r = 0; r < v_desc->rows; r++)
    {
        vtty_fill_span(v_desc, r, start_col, v_desc->cols - 1,
            (wchar_t)fill_char, A_NORMAL, v_desc->default_colors);
    }

    return;
}

