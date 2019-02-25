
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_render.h"
#include "vterm_buffer.h"

void
vterm_erase(vterm_t *vterm, int idx)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             r, c;

    if(vterm == NULL) return;

    // a value of -1 means current, active buffer
    if(idx == -1)
    {
        // set the vterm description buffer selector
        idx = vterm_buffer_get_active(vterm);
    }

    v_desc = &vterm->vterm_desc[idx];

    for(r = 0;r < v_desc->rows; r++)
    {
        vcell = &v_desc->cells[r][0];

        for(c = 0; c < v_desc->cols; c++)
        {
            VCELL_SET_CHAR((*vcell), ' ');
            VCELL_SET_DEFAULT_COLORS((*vcell), v_desc);

            vcell++;
        }
    }

    return;
}

void
vterm_erase_row(vterm_t *vterm, int row)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             c;
    int             idx;

    if(vterm == NULL) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(row == -1) row = v_desc->crow;
    vcell = &v_desc->cells[row][0];

    for(c = 0;c < v_desc->cols; c++)
    {
        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_DEFAULT_COLORS((*vcell), v_desc);

        vcell++;
    }

    return;
}

void
vterm_erase_rows(vterm_t *vterm, int start_row)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return;
    if(start_row < 0) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    while(start_row < v_desc->rows)
    {
        vterm_erase_row(vterm, start_row);
        start_row++;
    }

    return;
}

void
vterm_erase_col(vterm_t *vterm, int col)
{
    vterm_cell_t    *vcell;
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             r;

    if(vterm == NULL) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(col == -1) col = v_desc->ccol;

    for(r = 0; r < v_desc->rows; r++)
    {
        // store the cell address to reduce scalar look-ups
        vcell = &v_desc->cells[r][col];

        VCELL_SET_CHAR((*vcell), ' ');
        VCELL_SET_DEFAULT_COLORS((*vcell), v_desc);
    }

    return;
}

void
vterm_erase_cols(vterm_t *vterm,int start_col)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;

    if(vterm == NULL) return;
    if(start_col < 0) return;

    // set the vterm description buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    while(start_col < v_desc->cols)
    {
        vterm_erase_col(vterm, start_col);
        start_col++;
    }

    return;
}

