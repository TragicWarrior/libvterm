
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

#include <wchar.h>

#include "macros.h"
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_csi.h"
#include "vterm_buffer.h"
#include "color_cache.h"

void
_vterm_set_color_pair_safe(vterm_desc_t *v_desc, short colors, int fg, int bg);

long
interpret_custom_color(vterm_t *vterm, int param[], int pcount);

/* interprets a 'set attribute' (SGR) CSI escape sequence */
void
interpret_csi_SGR(vterm_t *vterm, int param[], int pcount)
{
    extern short    rRGB[], gRGB[], bRGB[];
    vterm_desc_t    *v_desc = NULL;
    int             i;
    int             colors;
    short           mapped_color;
    int             idx;
    short           fg, bg;
    int             retval;
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
            v_desc->fg = param[i] - 30;

            // find the required pair in the cache
            colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

            if(colors == -1)
            {
                colors = color_cache_add_pair(vterm,
                    v_desc->fg, v_desc->bg);
            }

            _vterm_set_color_pair_safe(v_desc, colors,
                v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_XCOLOR_FG:
            // code 38
            // set custom foreground color
            fg = interpret_custom_color(vterm, param, pcount);
            mapped_color = vterm_get_mapped_color(vterm, fg, 0, 0, 0);

            if(mapped_color > 0) fg = mapped_color;

            if(fg != -1)
            {
                v_desc->fg = fg;

                colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

                if(colors == -1)
                {
                    colors = color_cache_add_pair(vterm, 
                        v_desc->fg, v_desc->bg);
                }

                _vterm_set_color_pair_safe(v_desc, colors,
                    v_desc->fg, v_desc->bg);
            }

            i += 2;
            continue;

        csi_sgr_RESET:
            // SGR 0
            // reset attributes
            v_desc->curattr = A_NORMAL;

            _vterm_set_color_pair_safe(v_desc, v_desc->default_colors,
                v_desc->fg, v_desc->bg);

            // attribute reset is an implicit color reset too so we'll
            // fall-through to reset_fg and reset_bg

        csi_sgr_RESET_FG:
            // code 39
            // reset fg color
            retval = color_cache_split_pair(v_desc->default_colors, &fg, &bg);

            if(retval != -1)
            {
                v_desc->fg = fg;

                colors = color_cache_find_pair(v_desc->fg, v_desc->bg);
            }
            else
            {
                colors = 0;
            }

            // one addtl safeguard
            if(colors == -1) colors = 0;

            _vterm_set_color_pair_safe(v_desc, colors, v_desc->fg, v_desc->bg);

            if(param[i] != 0) continue;

        csi_sgr_RESET_BG:
            // code 49
            // reset bg color
            retval = color_cache_split_pair(v_desc->default_colors, &fg, &bg);

            if(retval != -1)
            {
                v_desc->bg = bg;

                colors = color_cache_find_pair(v_desc->fg, v_desc->bg);
            }
            else
            {
                colors = 0;
            }

            // one addtl safeguard
            if(colors == -1) colors = 0;

            _vterm_set_color_pair_safe(v_desc, colors,
                v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_BG:
            // set bg color (case # - 40 = fg color)
            v_desc->bg = param[i] - 40;

            // find the required pair in the cache
            colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

            // no color pair found so we'll try and add it
            if(colors == -1)
            {
                colors = color_cache_add_pair(vterm, v_desc->fg, v_desc->bg);
            }

            _vterm_set_color_pair_safe(v_desc, colors, v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_XCOLOR_BG:
            // code 48
            // set custom background color
            bg = interpret_custom_color(vterm, param, pcount);
            mapped_color = vterm_get_mapped_color(vterm, bg, 0, 0, 0);

            if(mapped_color > 0) bg = mapped_color;

            if(bg != -1)
            {
                v_desc->bg = bg;

                colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

                if(colors == -1)
                {
                    colors = color_cache_add_pair(vterm,
                            v_desc->fg, v_desc->bg);
                }

                _vterm_set_color_pair_safe(v_desc, colors,
                    v_desc->fg, v_desc->bg);

            }

            i += 2;

            continue;

        csi_sgr_AIX_FG:
            // codes 90 - 97
            // set 16 color fg (aixterm) 
            if(vterm->flags & VTERM_FLAG_C16)
            {
                fg = param[i] - 90;
                v_desc->fg = vterm_add_mapped_color(vterm, fg + 90,
                    rRGB[fg], gRGB[fg], bRGB[fg]);
            }
            else
            {
                if(vterm->flags & VTERM_FLAG_C8) v_desc->fg = param[i] - 90;
                else
                {
                    v_desc->fg = (param[i] - 90) + 8;
                }
            }

            // find the required pair in the cache
            colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

            if(colors == -1)
            {
                colors = color_cache_add_pair(vterm, v_desc->fg, v_desc->bg);
            }

            _vterm_set_color_pair_safe(v_desc, colors, v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_AIX_BG:
            // codes 100 - 107
            // set 16 color bg (aixterm)
            if(vterm->flags & VTERM_FLAG_C16)
            {
                bg = param[i] - 100;
                v_desc->bg = vterm_add_mapped_color(vterm, bg + 100,
                    rRGB[bg], gRGB[bg], bRGB[bg]);
            }
            else
            {
                v_desc->bg = param[i] - 100;
                if((vterm->flags & VTERM_FLAG_C8) == 0) v_desc->bg += 8;
            }

            // find the required pair in the cache
            colors = color_cache_find_pair(v_desc->fg, v_desc->bg);

            // no color pair found so we'll try and add it
            if(colors == -1)
            {
                colors = color_cache_add_pair(vterm, v_desc->fg, v_desc->bg);
            }

            _vterm_set_color_pair_safe(v_desc, colors,
                 v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_DEFAULT:
            continue;
    }
}

inline void
_vterm_set_color_pair_safe(vterm_desc_t *v_desc, short colors, int fg, int bg)
{
    v_desc->colors = colors;

    color_content(fg, &v_desc->f_rgb[0], &v_desc->f_rgb[1], &v_desc->f_rgb[2]);
    color_content(bg, &v_desc->b_rgb[0], &v_desc->b_rgb[1], &v_desc->b_rgb[2]);

    return;
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
inline long
interpret_custom_color(vterm_t *vterm, int param[], int pcount)
{
    int     method = 0;
    // short   red;
    // short   green;
    // short   blue;

    if(vterm == NULL) return -1;
    if(pcount < 2) return -1;

    method = param[1];

    // set to color pair
    if(method == 5)
    {
        /*
            normally the color number would be irrelevant because it
            is internal to the guest application color table.  however,
            xterm OSC ^]4 transmits color number and RGB values which
            we interpret and set.
        */
        if(pcount < 3) return -1;

        return (unsigned short)param[2];
    }

    // set to nearest rgb value
    if(method == 2)
    {
        if(pcount < 6) return -1;

        // red = param[3];
        // green = param[4];
        // blue = param[5];

        // endwin();
        // printf("r: %d, g: %d, b: %d\n\r", red, green, blue);
        // exit(0);

        return -1;
    }

/*
SGR_DEBUG:

    endwin();

    printf("params %d\n\r", pcount);
    int i;
    for(i = 0; i < pcount; i++)
    {
        printf("%d ", param[i]);
    }
    printf("\n\r");

    exit(0);
*/

    return -1;
}
