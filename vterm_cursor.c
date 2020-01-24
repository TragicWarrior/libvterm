
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vterm_private.h"
#include "vterm_cursor.h"
#include "vterm_buffer.h"

void
vterm_cursor_move_home(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->ccol = 0;

    if(v_desc->buffer_state & STATE_ORIGIN_MODE)
    {
        v_desc->crow = v_desc->scroll_min;
    }
    else
    {
        v_desc->crow = 0;
    }

    return;
}

void
vterm_cursor_move_backward(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             min_row;
    int             idx;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->buffer_state & STATE_ORIGIN_MODE)
        min_row = v_desc->scroll_min;
    else
        min_row = 0;

    // cursor is at the left margin
    if(v_desc->ccol > 0)
    {
        v_desc->ccol--;
        return;
    }

    if(v_desc->crow > min_row)
    {
        v_desc->ccol = v_desc->cols - 1;
        v_desc->crow--;
    }

    return;
}

void
vterm_cursor_move_tab(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             tab_sz = 8;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    // a tab on the last column push to the beginning of the next row
    if(v_desc->ccol == v_desc->cols - 1)
    {
        if(v_desc->crow < v_desc->scroll_max)
        {
            v_desc->crow++;
        }
        else
        {
            vterm_scroll_up(vterm, FALSE);
        }

        v_desc->ccol = 0;
        return;
    }

    /*
        if the cursor is within a tab size of the right-hand margin,
        jump to the edge.
    */
    if((v_desc->cols - v_desc->ccol) < tab_sz)
    {
        v_desc->ccol = v_desc->cols - 1;

        return;
    }

    // in all other case, just advance to next tab stop
    tab_sz = 8 - (v_desc->ccol % 8);
    v_desc->ccol += tab_sz;

    return;
}

void
vterm_cursor_show(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc = NULL;

    if(vterm == NULL) return;
    if(idx != VTERM_BUF_STANDARD && idx != VTERM_BUF_ALTERNATE) return;

    v_desc = &vterm->vterm_desc[idx];

    v_desc->buffer_state &= ~STATE_CURSOR_INVIS;

    return;
}

void
vterm_cursor_hide(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc = NULL;

    if(vterm == NULL) return;
    if(idx != VTERM_BUF_STANDARD && idx != VTERM_BUF_ALTERNATE) return;

    v_desc = &vterm->vterm_desc[idx];

    v_desc->buffer_state |= STATE_CURSOR_INVIS;

    return;
}

void
vterm_cursor_save(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->saved_x = v_desc->ccol;
    v_desc->saved_y = v_desc->crow;

    return;
}

void
vterm_cursor_restore(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    v_desc->ccol = v_desc->saved_x;
    v_desc->crow = v_desc->saved_y;

    return;
}

void
vterm_get_cursor_position(vterm_t *vterm, int *column, int *row)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    *column = v_desc->ccol;
    *row = v_desc->crow;

    return;
}
