#include "vterm_private.h"
#include "vterm_cursor.h"
#include "vterm_buffer.h"

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
