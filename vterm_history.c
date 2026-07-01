#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

void
vterm_set_history_size(vterm_t *vterm, int rows)
{
    vterm_desc_t    *v_desc;
    int             old_rows;
    int             delta;

    if(vterm == NULL) return;
    if(rows < 1) return;

    v_desc = &vterm->vterm_desc[VTERM_BUF_HISTORY];

    old_rows = v_desc->rows;

    // nothing to do
    delta = rows - old_rows;
    if(delta == 0) return;

    /*
        shrinking: advance the ring head past the rows being dropped so
        the NEWEST rows survive -- the linearizing realloc below copies
        logical rows 0..rows-1, and after the bump logical 0 is the
        oldest survivor.
    */
    if(delta < 0)
    {
        v_desc->head += -delta;
        while(v_desc->head >= old_rows) v_desc->head -= old_rows;

        vterm_buffer_realloc(vterm, VTERM_BUF_HISTORY, -1, rows);

        // dropped the oldest rows; never report more filled than we can hold
        if(v_desc->fill > rows) v_desc->fill = rows;

        return;
    }

    /*
        growing: the realloc linearizes content to the physical top and
        blank-fills the new rows below it.  rotate the head so the
        content sits at the logical TAIL instead -- newest stays
        adjacent to the live screen.
    */
    vterm_buffer_realloc(vterm, VTERM_BUF_HISTORY, -1, rows);
    v_desc->head = old_rows;

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

int
vterm_get_history_used(vterm_t *vterm)
{
    vterm_desc_t    *v_desc;

    if(vterm == NULL) return -1;

    v_desc = &vterm->vterm_desc[VTERM_BUF_HISTORY];

    return v_desc->fill;
}
