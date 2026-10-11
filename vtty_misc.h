
#ifndef _VTTY_MISC_H_
#define _VTTY_MISC_H_

#include "vtty_private.h"

static inline void
clamp_cursor_to_bounds(vtty_t *vtty)
{
    vtty_desc_t     *v_desc = vtty->v_desc_active;

    if(v_desc->crow < 0) v_desc->crow = 0;
    if(v_desc->ccol < 0) v_desc->ccol = 0;
    if(v_desc->crow >= v_desc->rows) v_desc->crow = v_desc->rows - 1;
    if(v_desc->ccol >= v_desc->cols) v_desc->ccol = v_desc->cols - 1;
}

#endif
