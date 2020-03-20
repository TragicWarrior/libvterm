
/*
    VT100 SGR documentation
    From http://vt100.net/docs/vt510-rm/SGR table 5-16
    0   All attributes off
    1   Bold
    4   Underline
    5   Blinking
    7   Negative image
    8   Invisible image
    10  The ASCII character set is the current 7-bit
        display character set (default) - SCO Console only.
    11  Map Hex 00-7F of the PC character set codes
        to the current 7-bit display character set
        - SCO Console only.
    12  Map Hex 80-FF of the current character set to
        the current 7-bit display character set - SCO
        Console only.
    22  Bold off
    24  Underline off
    25  Blinking off
    27  Negative image off
    28  Invisible image off
    38  Custom foreground color
    48  Custom background color
*/

#include <stdlib.h>
#include <wchar.h>

#include "macros.h"
#include "vterm.h"
#include "color_map.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"
#include "color_cache.h"
#include "nc_wrapper.h"

void
_vterm_set_color_pair_safe(vterm_desc_t *v_desc, vterm_t *vterm, short colors,
    int fg, int bg);

long
interpret_custom_color(vterm_t *vterm, int param[], int pcount, int *processed);

/* interprets a 'set attribute' (SGR) CSI escape sequence */
void
interpret_csi_SGR(vterm_t *vterm, int param[], int pcount)
{
    vterm_desc_t    *v_desc = NULL;
    int             i;
    short           mapped_color;
    int             processed = 0;
    int             idx;
    short           fg, bg;
    int             r, g, b;
    static void     *sgr_table[] =
                        {
                            [0]             = &&csi_sgr_RESET,
                            [1]             = &&csi_sgr_BOLD_ON,
                            [2]             = &&csi_sgr_DIM_ON,
                            [4]             = &&csi_sgr_UNDERLINE_ON,
                            [5]             = &&csi_sgr_BLINK_ON,
                            [7]             = &&csi_sgr_REVERSE_ON,
                            [8]             = &&csi_sgr_INVISIBLE_ON,
                            [10]            = &&csi_sgr_RMACS,
                            [11]            = &&csi_sgr_SMACS,
                            [22]            = &&csi_sgr_NORMAL,
                            [24]            = &&csi_sgr_UNDERLINE_OFF,
                            [25]            = &&csi_sgr_BLINK_OFF,
                            [27]            = &&csi_sgr_REVERSE_OFF,
                            [28]            = &&csi_sgr_INVISIBLE_OFF,
                            [30 ... 37]     = &&csi_sgr_FG,
                            [38]            = &&csi_sgr_XCOLOR_FG,
                            [39]            = &&csi_sgr_RESET_FG,
                            [40 ... 47]     = &&csi_sgr_BG,
                            [48]            = &&csi_sgr_XCOLOR_BG,
                            [49]            = &&csi_sgr_RESET_BG,
                            [90 ... 97]     = &&csi_sgr_AIX_FG,
                            [100 ... 107]   = &&csi_sgr_AIX_BG,
                            [127]           = &&csi_sgr_DEFAULT,
                        };

    // set vterm_desc buffer selector
    idx = vterm_buffer_get_active(vterm);
    v_desc = &vterm->vterm_desc[idx];

    if(pcount == 0)
    {
        pcount = 1;
        param[0] = 0;
    }

    for(i = 0; i < pcount; i++)
    {
        // jump table
        SWITCH(sgr_table, (unsigned int)param[i], 127);

        csi_sgr_BOLD_ON:
            // code 1
            v_desc->curattr |= A_BOLD;
            continue;

        csi_sgr_DIM_ON:
            // code 2
            v_desc->curattr |= A_DIM;
            continue;

        csi_sgr_UNDERLINE_ON:
            // code 4
            v_desc->curattr |= A_UNDERLINE;
            continue;

        csi_sgr_BLINK_ON:
            // code 5
            v_desc->curattr |= A_BLINK;
            continue;

        csi_sgr_REVERSE_ON:
            // code 7
            v_desc->curattr |= A_REVERSE;
            continue;

        csi_sgr_INVISIBLE_ON:
            // code 8
            v_desc->curattr |= A_INVIS;
            continue;

        csi_sgr_RMACS:
            // code 10
            vterm->internal_state &= ~(STATE_ALT_CHARSET);
            continue;

        csi_sgr_SMACS:
            // code 11
            vterm->internal_state |= STATE_ALT_CHARSET;
            continue;

        csi_sgr_NORMAL:
            // code 22
            // bold and dim off
            v_desc->curattr &= ~(A_BOLD);
            v_desc->curattr &= ~(A_DIM);
            continue;

        csi_sgr_UNDERLINE_OFF:
            // code 24
            v_desc->curattr &= ~(A_UNDERLINE);
            continue;

        csi_sgr_BLINK_OFF:
            // code 25
            v_desc->curattr &= ~(A_BLINK);
            continue;

        csi_sgr_REVERSE_OFF:
            // code 27
            v_desc->curattr &= ~(A_REVERSE);
            continue;

        csi_sgr_INVISIBLE_OFF:
            // code 28
            v_desc->curattr &= ~A_INVIS;
            continue;

        csi_sgr_FG:
            // set fg color (case # - 30 = fg color)
            v_desc->fg = vterm_get_mapped_color(vterm, param[i] - 30,
                &v_desc->f_rgb[0], &v_desc->f_rgb[1], &v_desc->f_rgb[2]);

            continue;

        csi_sgr_XCOLOR_FG:
            // code 38
            // set custom foreground color
            fg = interpret_custom_color(vterm, &param[i], pcount, &processed);

            if(fg == -1)
            {
                i += (pcount - 1);
                continue;
            }

            mapped_color = vterm_get_mapped_color(vterm, fg, &r, &g, &b);

            if(mapped_color != -1) fg = mapped_color;

            if(fg != -1)
            {
                v_desc->fg = fg;
            }

            i += processed;
            continue;

        csi_sgr_RESET:
            // SGR 0
            // reset attributes
            v_desc->curattr = A_NORMAL;

            // attribute reset is an implicit color reset too so we'll
            // fall-through to reset_fg and reset_bg

        csi_sgr_RESET_FG:
            // code 39
            // reset fg color
            v_desc->fg = vterm_get_mapped_color(vterm, v_desc->default_fg,
                &v_desc->f_rgb[0], &v_desc->f_rgb[1], &v_desc->f_rgb[2]);

            // if param[i] == 0 then we fall through to resetting BG as well
            if(param[i] != 0) continue;

        csi_sgr_RESET_BG:
            // code 49
            // reset bg color
            v_desc->bg = vterm_get_mapped_color(vterm, v_desc->default_bg,
                &v_desc->b_rgb[0], &v_desc->b_rgb[1], &v_desc->b_rgb[2]);

            continue;

        csi_sgr_BG:
            // set bg color (case # - 40 = fg color)
            v_desc->bg = vterm_get_mapped_color(vterm, param[i] - 40,
                &v_desc->b_rgb[0], &v_desc->b_rgb[1], &v_desc->b_rgb[2]);

            continue;

        csi_sgr_XCOLOR_BG:
            // code 48
            // set custom background color
            bg = interpret_custom_color(vterm, &param[i], pcount, &processed);
            if(bg == -1)
            {
                i += (pcount - 1);
                continue;
            }

            mapped_color = vterm_get_mapped_color(vterm, bg, &r, &g, &b);

            if(mapped_color != -1) bg = mapped_color;

            if(bg != -1)
            {
                v_desc->bg = bg;
            }

            i += processed;
            continue;

        csi_sgr_AIX_FG:
            // codes 90 - 97
            // set 16 color fg (aixterm)
            fg = param[i] - 90;
            if(!(vterm->flags & VTERM_FLAG_C8)) fg += 8;
            v_desc->fg = vterm_get_mapped_color(vterm, fg, &r, &g, &b);

            continue;

        csi_sgr_AIX_BG:
            // codes 100 - 107
            // set 16 color bg (aixterm)
            bg = param[i] - 100;
            if(!(vterm->flags & VTERM_FLAG_C8)) bg += 8;
            v_desc->bg = vterm_get_mapped_color(vterm, bg, &r, &g, &b);

            continue;

        csi_sgr_DEFAULT:
            continue;
    }
}


/*
    ISO-8613-6 sequenences are as follows:

    Set foreground by RGB:
    ESC [ 38 ; 2 ; i ; r ; g ; b m

    Set foreground by color
    ESC [ 38 ; 5 ; n m

    Set bagkground by RGB:
    ESC [ 48 ; 2 ; i ; r ; g ; b m

    Set background by color
    ESC [ 48 ; 5 ; n m

    i = color space (always ignored)
    r = red value
    g = green value
    b = blue value
    n = a defined color
*/
long
interpret_custom_color(vterm_t *vterm, int param[], int pcount, int *processed)
{
    int         method = 0;
    short       new_color = -1;

    *processed = 0;

    if(vterm == NULL) return -1;
    if(pcount < 2)
    {
        *processed = pcount;
        return -1;
    }

    // a valid ISO-8613-6 color control must begin with 38 or 48
    if(param[0] != 38 && param[0] != 48)
    {
        *processed = pcount;
        return -1;
    }

    // paramter two must be either a 2 or 5 to indicate operation method
    method = param[1];

    // set to color pair (which should have been programmed by OSC 4)
    if(method == 5)
    {
        /*
            normally the color number would be irrelevant because it
            is internal to the guest application color table.  however,
            xterm OSC ^]4 transmits color number and RGB values which
            we interpret and set.
        */
        if(pcount < 3)
        {
            *processed = pcount;
            return -1;
        }

        *processed = 2;
        return (unsigned short)param[2];
    }

    // set to nearest rgb value
    if(method == 2)
    {
        if((pcount % 5) == 0)
        {
            new_color = vterm_add_mapped_color(vterm, -1,
                param[2], param[3], param[4], COLOR_PROXIMITY);

            *processed = 4;

            if(new_color >= 0) return new_color;
        }

        if((pcount % 6) == 0)
        {
            new_color = vterm_add_mapped_color(vterm, -1,
                param[3], param[4], param[5], COLOR_PROXIMITY);

            *processed = 5;

            if(new_color >= 0) return new_color;
        }
    }

    *processed = pcount;
    return -1;
}

