
#include "vterm.h"
#include "vterm_misc.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

inline void
clamp_cursor_to_bounds(vterm_t *vterm)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    // set vterm description buffer seletor
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(v_desc->crow < 0) v_desc->crow = 0;

    if(v_desc->ccol < 0) v_desc->ccol = 0;

    if(v_desc->crow >= v_desc->rows) v_desc->crow = v_desc->rows - 1;

    if(v_desc->ccol >= v_desc->cols) v_desc->ccol = v_desc->cols - 1;

    return;
}

