
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"


/*
    vt100 - ESC [ x K

    vt200 - ESC [ ? x K

    where 'x' dictates the behavior

    default value: 0
    Erases some or all characters in the active line
    according to the parameter. Editor Function
    Parameter Parameter Meaning

    0   Erase from the active position to the end of the
        line, inclusive (default)

    1   Erase from the start of the screen to the active
        position, inclusive

    2   Erase all of the line, inclusive
*/

/* Interpret the 'erase line' escape sequence */
void
interpret_csi_EL(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             erase_start, erase_end, c;
    int             cmd = 0;
    int             row;

    // set the vterm description buffer selector
    v_desc = vterm->v_desc_active;

    cmd =  (pcount > 0 ? param[0] : 0);

    switch(cmd)
    {
        case 1:
        {
            erase_start = 0;
            erase_end = v_desc->ccol;
            break;
        }

        case 2:
        {
            erase_start = 0;
            erase_end = v_desc->cols - 1;
            break;
        }

        case 0:
        default:
        {
            erase_start = v_desc->ccol;
            erase_end = v_desc->cols - 1;
            break;
        }
    }

    row = v_desc->crow;

    for(c = erase_start; c <= erase_end; c++)
    {
        VCELL_SET_CHAR(v_desc, row, c, ' ');
        VCELL_SET_ATTR(v_desc, row, c, v_desc->curattr);
        VCELL_SET_COLORS(v_desc, row, c);
    }

    return;
}

