
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_render.h"
#include "vterm_buffer.h"

void
vterm_erase(vterm_t *vterm, int idx, char fill_char)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;

    if(vterm == NULL) return;

    // use a space if no fill char is specified
    if(fill_char == 0) fill_char = ' ';

    // a value of -1 means current, active buffer
    if(idx == -1)
    {
        v_desc = vterm->v_desc_active;
    }
    else
    {
        v_desc = &vterm->vterm_desc[idx];
    }

    // post event that term is about to be erased
    if(vterm->event_mask & VTERM_MASK_TERM_PRECLEAR)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm,
                VTERM_EVENT_TERM_PRECLEAR, NULL);
        }
    }

    for(r = 0;r < v_desc->rows; r++)
    {
        vterm_fill_span(v_desc, r, 0, v_desc->cols - 1,
            (wchar_t)fill_char, A_NORMAL, v_desc->default_colors);
    }

    return;
}

void
vterm_erase_row(vterm_t *vterm, int row, bool reset_colors, char fill_char)
{
    vterm_desc_t    *v_desc = NULL;

    if(vterm == NULL) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

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
    vterm_fill_span(v_desc, row, 0, v_desc->cols - 1,
        (wchar_t)fill_char, A_NORMAL,
        reset_colors == TRUE ? v_desc->default_colors : v_desc->colors);

    return;
}

void
vterm_erase_rows(vterm_t *vterm, int start_row, char fill_char)
{
    vterm_desc_t    *v_desc = NULL;

    if(vterm == NULL) return;
    if(start_row < 0) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

    while(start_row < v_desc->rows)
    {
        vterm_erase_row(vterm, start_row, FALSE, fill_char);
        start_row++;
    }

    return;
}

void
vterm_erase_col(vterm_t *vterm, int col, char fill_char)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;

    if(vterm == NULL) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

    if(col == -1) col = v_desc->ccol;

    // a vertical 1-wide span: one whole-cell store per row
    for(r = 0; r < v_desc->rows; r++)
    {
        VCELL_SET_ALL(v_desc, r, col, (wchar_t)fill_char, A_NORMAL,
            v_desc->default_colors);
    }

    return;
}

void
vterm_erase_cols(vterm_t *vterm, int start_col, char fill_char)
{
    vterm_desc_t    *v_desc = NULL;
    int             r;

    if(vterm == NULL) return;
    if(start_col < 0) return;

    if(fill_char == 0) fill_char = ' ';

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

    // row-major: one span per row instead of one full-rows pass per column
    for(r = 0; r < v_desc->rows; r++)
    {
        vterm_fill_span(v_desc, r, start_col, v_desc->cols - 1,
            (wchar_t)fill_char, A_NORMAL, v_desc->default_colors);
    }

    return;
}

