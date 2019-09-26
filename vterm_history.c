#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

void
vterm_set_history_size(vterm_t *vterm, int rows)
{
    vterm_desc_t    *v_desc;
    int             delta;

    if(vterm == NULL) return;

    v_desc = &vterm->vterm_desc[VTERM_BUF_HISTORY];

    // nothing to do
    delta = rows - v_desc->rows;
    if(delta == 0) return;

    // buffer is shrinking
    if(delta < 0)
    {
        vterm_buffer_shift_up(vterm, VTERM_BUF_HISTORY, -1, -1, ABSINT(delta));
        vterm_buffer_realloc(vterm, VTERM_BUF_HISTORY, -1, rows);

        return;
    }

    // buffer is growing
    vterm_buffer_realloc(vterm, VTERM_BUF_HISTORY, -1, rows);
    vterm_buffer_shift_down(vterm, VTERM_BUF_HISTORY, -1, -1, delta);

    return;
}

int
vterm_get_history_size(vterm_t *vterm)
{
    vterm_desc_t    *v_desc;

    if(vterm == NULL) return -1;

    v_desc = &vterm->vterm_desc[VTERM_BUF_HISTORY];

    return v_desc->rows;
}
