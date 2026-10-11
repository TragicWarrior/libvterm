
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vtty_private.h"
#include "vtty_cursor.h"
#include "vtty_buffer.h"

void
vtty_cursor_move_home(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

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
vtty_cursor_move_backward(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;
    int             min_row;

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

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
vtty_cursor_move_tab(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;
    int             tab_sz = 8;

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

    // a tab on the last column push to the beginning of the next row
    if(v_desc->ccol == v_desc->cols - 1)
    {
        if(v_desc->crow < v_desc->scroll_max)
        {
            v_desc->crow++;
        }
        else
        {
            vtty_scroll_up(vtty, FALSE);
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
vtty_cursor_show(vtty_t *vtty, int idx)
{
    vtty_desc_t     *v_desc = NULL;

    if(vtty == NULL) return;
    if(idx != VTTY_BUF_STANDARD && idx != VTTY_BUF_ALTERNATE) return;

    v_desc = &vtty->vtty_desc[idx];

    v_desc->buffer_state &= ~STATE_CURSOR_INVIS;

    return;
}

void
vtty_cursor_hide(vtty_t *vtty, int idx)
{
    vtty_desc_t     *v_desc = NULL;

    if(vtty == NULL) return;
    if(idx != VTTY_BUF_STANDARD && idx != VTTY_BUF_ALTERNATE) return;

    v_desc = &vtty->vtty_desc[idx];

    v_desc->buffer_state |= STATE_CURSOR_INVIS;

    return;
}

void
vtty_cursor_save(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;

    // set vtty description buffer selector
    v_desc = vtty->v_desc_active;

    v_desc->saved_x = v_desc->ccol;
    v_desc->saved_y = v_desc->crow;

    return;
}

void
vtty_cursor_restore(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = NULL;

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    v_desc->ccol = v_desc->saved_x;
    v_desc->crow = v_desc->saved_y;

    return;
}

void
vtty_get_cursor_position(vtty_t *vtty, int *column, int *row)
{
    vtty_desc_t     *v_desc = NULL;

    // set the vtty description buffer selector
    v_desc = vtty->v_desc_active;

    *column = v_desc->ccol;
    *row = v_desc->crow;

    return;
}
