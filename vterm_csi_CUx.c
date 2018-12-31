
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_misc.h"
#include "vterm_buffer.h"

/* Interpret the 'relative mode' sequences: CUU, CUD, CUF, CUB, CNL,
 * CPL, CHA, HPR, VPA, VPR, HPA */
void
interpret_csi_CUx(vterm_t *vterm, char verb, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             idx;
    int             n = 1;

    if(pcount && param[0] > 0) n = param[0];

    // set active vterm description selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    switch (verb)
    {
        case 'A':   v_desc->crow -= n;             break;
        case 'B':
        case 'e':   v_desc->crow += n;             break;
        case 'C':
        case 'a':   v_desc->ccol += n;             break;
        case 'D':   v_desc->ccol -= n;             break;
        case 'E':
        {
            v_desc->crow += n;
            v_desc->ccol = 0;
            break;
        }
        case 'F':
        {
            v_desc->crow -= n;
            v_desc->ccol = 0;
            break;
        }
        case 'G':
        case '`':   v_desc->ccol = param[0] - 1;   break;
        case 'd':   v_desc->crow = param[0] - 1;   break;

        /*
            ESC [ r ; c H
            where 'r' is row number and 'c' is column number
            - or -
            ESC [ H
            which means 0, 0 implied
        */
        case 'H':
        {
            if(pcount == 0)
            {
                v_desc->crow = 0;
                v_desc->ccol = 0;
                break;
            }

            v_desc->crow = param[0] - 1;
            v_desc->ccol = param[1] - 1;
            // endwin();
            // printf("r:%d, c:%d\n\r", param[0] - 1, param[1] - 1);
            // exit(0);
            break;
        }
    }

    clamp_cursor_to_bounds(vterm);

    return;
}

