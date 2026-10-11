#include "macros.h"
#include "vtty.h"
#include "vtty_private.h"
#include "vtty_buffer.h"

void
vtty_set_history_size(vtty_t *vtty, int rows)
{
    vtty_desc_t     *v_desc;
    int             old_rows;
    int             delta;

    if(vtty == NULL) return;
    if(rows < 1) return;

    v_desc = &vtty->vtty_desc[VTTY_BUF_HISTORY];

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

        vtty_buffer_realloc(vtty, VTTY_BUF_HISTORY, -1, rows);

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
    vtty_buffer_realloc(vtty, VTTY_BUF_HISTORY, -1, rows);
    v_desc->head = old_rows;

    return;
}

int
vtty_get_history_size(vtty_t *vtty)
{
    vtty_desc_t     *v_desc;

    if(vtty == NULL) return -1;

    v_desc = &vtty->vtty_desc[VTTY_BUF_HISTORY];

    return v_desc->rows;
}

int
vtty_get_history_used(vtty_t *vtty)
{
    vtty_desc_t     *v_desc;

    if(vtty == NULL) return -1;

    v_desc = &vtty->vtty_desc[VTTY_BUF_HISTORY];

    return v_desc->fill;
}
