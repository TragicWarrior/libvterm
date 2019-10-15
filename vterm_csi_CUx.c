
#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_misc.h"
#include "vterm_buffer.h"

/*
    Interpret the 'dynamic movement' sequences: CUU, CUD, CUF, CUB, CNL,
    CPL, CHA, HPR, VPA, VPR, HPA, HPV

    CUF - ESC [ n C

    The CUF sequence moves the active position to the right.  The distance
    moved is determined by the parameter. A parameter value of zero or
    one moves the active position one position to the right. A parameter
    value of n moves the active position n positions to the right. If an
    attempt is made to move the cursor to the right of the right margin,
    the cursor stops at the right margin.

    CUB - ESC [ n D

    The CUB sequence moves the active position to the left.  The distance
    moved is determined by the parameter. If the parameter value is zero
    or one, the active position is moved one position to the left. If the
    parameter value is n, the active position is moved n positions to
    the left. If an attempt is made to move the cursor to the left of
    the left margin, the cursor stops at the left margin.

    HVP - ESC [ r ; c f

    Where r = row and c = column.
*/
void
interpret_csi_CUx(vterm_t *vterm, char verb, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             n = 1;
    static void     *curmove_table[] =
                        {
                            [0]     = &&csi_DEFAULT,
                            ['A']   = &&csi_CUU,
                            ['B']   = &&csi_CUD,
                            ['e']   = &&csi_CUD,
                            ['C']   = &&csi_CUF,
                            ['a']   = &&csi_CUF,
                            ['D']   = &&csi_CUB,
                            ['E']   = &&csi_CNL,
                            ['F']   = &&csi_CPL,
                            ['G']   = &&csi_CHA,
                            ['`']   = &&csi_CHA,
                            ['d']   = &&csi_VPA,
                            ['f']   = &&csi_HVP,
                            ['H']   = &&csi_HVP,
                        };

    if(pcount > 0)
    {
        if(param[0] > 0) n = param[0];
    }

    // set active vterm description selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    SWITCH(curmove_table, (unsigned int)verb, 0);

    csi_CUU:
        // CSI 'CUU' (cursor up)
        v_desc->crow -= n;
        goto csi_DEFAULT;

    csi_CUD:
        // CSI 'CUD' (cursor down)
        v_desc->crow += n;
        goto csi_DEFAULT;

    csi_CUF:
        // CSI 'CUF' (cursor forward)
        v_desc->ccol += n;
        goto csi_DEFAULT;

    csi_CUB:
        // CSI 'CUB' (cursor backward)
        v_desc->ccol -= n;
        goto csi_DEFAULT;

    csi_CNL:
        // CSI 'CNL' (next line x times)
        v_desc->crow += n;
        v_desc->ccol = 0;
        goto csi_DEFAULT;

    csi_CPL:
        // CSI 'CPL' (prev line x times)
        v_desc->crow -= n;
        v_desc->ccol = 0;
        goto csi_DEFAULT;

    csi_CHA:
        // CSI 'CHA' / 'HPA' (move to absolute column)
        v_desc->ccol = param[0] - 1;
        goto csi_DEFAULT;

    csi_VPA:
        // CSI 'VPA' (move to absolute row)
        v_desc->crow = param[0] - 1;
        goto csi_DEFAULT;

    csi_HVP:
        /*
            CSI 'HVP' / 'CUP'
            ESC [ r ; c H
            where 'r' is row number and 'c' is column number

            - or -

            ESC [ H
            which means 0, 0 implied

            - or -

            ESC [ r ; c f
        */
        if(pcount == 0)
        {
            v_desc->crow = 0;
            v_desc->ccol = 0;
            goto csi_DEFAULT;
        }
        v_desc->crow = param[0] - 1;
        v_desc->ccol = param[1] - 1;


// all calls above jump here

    csi_DEFAULT:

    clamp_cursor_to_bounds(vterm);

    return;
}

