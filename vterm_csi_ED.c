
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"

#define SWITCH(jtable, x)    goto *jtable[x]


/* interprets an 'erase display' (ED) escape sequence */
void
interpret_csi_ED(vterm_t *vterm, int param[], int pcount)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             r, c;
    int             start_row, start_col, end_row, end_col;
    int             idx;

    unsigned int    p;
    static void     *table[] = {&&csi_ED_1, &&csi_ED_2, &&csi_ED_3};

    // set vterm desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    p = (pcount > 0 ? param[0] : 3);

    /* decide range */
    SWITCH(table, p - 1);

    csi_ED_1:
    start_row = 0;
    start_col = 0;
    end_row = v_desc->crow;
    end_col = v_desc->ccol;
    goto jmp_done;

    csi_ED_2:
    start_row = 0;
    start_col = 0;
    end_row = v_desc->rows - 1;
    end_col = v_desc->cols - 1;
    goto jmp_done;

    csi_ED_3:
    start_row = v_desc->crow;
    start_col = v_desc->ccol;
    end_row = v_desc->rows - 1;
    end_col = v_desc->cols-1;
    goto jmp_done;

    jmp_done:

    /* clean range */
    for(r = start_row;r <= end_row; r++)
    {
        vcell = &v_desc->cells[r][0];

        for(c = start_col; c <= end_col; c++)
        {
            VCELL_SET_CHAR((*vcell), ' ');
            VCELL_SET_ATTR((*vcell), v_desc->curattr);
            VCELL_SET_COLORS((*vcell), v_desc);

            vcell++;
        }
    }
}
