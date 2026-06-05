
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

/*
    interprets an 'erase display' (ED) escape sequence

    vt100 and vt220 have different sequences but effectively
    parse the same with 'J' being the terminating character.
    for practical reasons, the caller of this function doesn't
    distinguish between the two.

    vt100 - ESC [ x J

    vt220 - ESC [ ? x J

    where 'x' dictates the type of operation.

    0 = erase below (default)
    1 = erase above
    2 = erase all
*/
void
interpret_csi_ED(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             r, c;
    int             start_row, start_col, end_row, end_col;

    unsigned int    p;

    // set vterm desc buffer selector
    v_desc = vterm->v_desc_active;

    p = (pcount > 0 ? param[0] : 3);

    /* decide range */
    switch(p)
    {
        // case 2 is more prevalent and small switches don't convert to
        // jump tables but rather conditional branching.
        case 2:
            // is this valid?
            if(v_desc->ccol < v_desc->cols)
            {
                interpret_csi_EL(vterm, (int[1]) { 0 }, 1);
            }
            start_row = 0;
            start_col = 0;
            end_row = v_desc->rows - 1;
            end_col = v_desc->cols - 1;

            v_desc->default_colors = v_desc->colors;

            break;

        case 1:
            if(v_desc->ccol > 0)
            {
                interpret_csi_EL(vterm, (int[1]) { 1 }, 1);
            }
            start_row = 0;
            start_col = 0;
            end_row = v_desc->crow - 1;
            end_col = v_desc->cols - 1;
            break;

        default:
            start_row = v_desc->crow;
            // start_col = v_desc->ccol;
            start_col = 0;
            end_row = v_desc->rows - 1;
            end_col = v_desc->cols - 1;
            break;
    }

    /* clean range */
    for(r = start_row; r <= end_row; r++)
    {
        for(c = start_col; c <= end_col; c++)
        {
            VCELL_SET_CHAR(v_desc, r, c, ' ');
            VCELL_SET_ATTR(v_desc, r, c, v_desc->curattr);
            VCELL_SET_COLORS(v_desc, r, c);
        }
    }
}
