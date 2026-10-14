
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
#include "vtty.h"
#include "color_map.h"
#include "vtty_private.h"
#include "vtty_csi.h"
#include "vtty_buffer.h"
#include "color_cache.h"

void
_vtty_set_color_pair_safe(vtty_desc_t *v_desc, vtty_t *vtty, short colors,
    int fg, int bg);

static int
_rgb_to_xterm256(int r, int g, int b)
{
    static const int cube_values[] = { 0, 0x5f, 0x87, 0xaf, 0xd7, 0xff };
    int     ci, gi;
    int     cr, cg, cb;
    int     gv, cube_dist, gray_dist;

    cr = (r < 48) ? 0 : (r < 115) ? 1 : (r - 35) / 40;
    cg = (g < 48) ? 0 : (g < 115) ? 1 : (g - 35) / 40;
    cb = (b < 48) ? 0 : (b < 115) ? 1 : (b - 35) / 40;
    if(cr > 5) cr = 5;
    if(cg > 5) cg = 5;
    if(cb > 5) cb = 5;

    ci = 16 + (36 * cr) + (6 * cg) + cb;
    cube_dist = (r - cube_values[cr]) * (r - cube_values[cr])
              + (g - cube_values[cg]) * (g - cube_values[cg])
              + (b - cube_values[cb]) * (b - cube_values[cb]);

    gi = (r + g + b - 24) / 30;
    if(gi < 0) gi = 0;
    if(gi > 23) gi = 23;
    gv = 8 + (10 * gi);
    gray_dist = (r - gv) * (r - gv) + (g - gv) * (g - gv) + (b - gv) * (b - gv);

    return (gray_dist < cube_dist) ? 232 + gi : ci;
}

long
interpret_custom_color(vtty_t *vtty, int param[], int pcount, int *processed);

// the 16 ANSI colors as the linux console and VGA show them
static const unsigned char  ansi[16][3] =
                                {
                                    {   0,   0,   0 },
                                    { 170,   0,   0 },
                                    {   0, 170,   0 },
                                    { 170,  85,   0 },
                                    {   0,   0, 170 },
                                    { 170,   0, 170 },
                                    {   0, 170, 170 },
                                    { 170, 170, 170 },
                                    {  85,  85,  85 },
                                    { 255,  85,  85 },
                                    {  85, 255,  85 },
                                    { 255, 255,  85 },
                                    {  85,  85, 255 },
                                    { 255,  85, 255 },
                                    {  85, 255, 255 },
                                    { 255, 255, 255 }
                                };

/*
    how many colors the real terminal has, as terminfo reports it.
    0 means it is not known (no color cache yet, or no terminal).
*/
static int
_vtty_host_colors(void)
{
    extern color_cache_t    *color_cache;

    if(color_cache == NULL) return 0;
    if(color_cache->term_colors < 0) return 0;

    return color_cache->term_colors;
}

/*
    fit an xterm-256 color number to a terminal that has fewer colors.

    a child may ask for any of 256 colors, or for a 24-bit color that is
    first matched to one of the 256, whatever TERM says.  a terminal
    with 8 or 16 colors has no such color number, and a pair built from
    one does not exist, so what reaches the screen is arbitrary: some
    text shows and some is drawn black on black.

    the color is replaced by the nearest of the colors the terminal does
    have:  the 8 ANSI colors, or all 16 when it has at least that many.
    a color the terminal already has is returned as it is, and so is
    every color when the terminal has 256 or its count is not known.
*/
int
vtty_fit_color(int color, int host_colors)
{
    static const int            cube[6] = { 0, 95, 135, 175, 215, 255 };
    int                         limit;
    int                         r, g, b;
    int                         lo, hi;
    int                         gray;
    int                         dist;
    int                         best = 0;
    int                         best_dist = -1;
    int                         n;

    if(host_colors <= 0 || host_colors >= 256) return color;
    if(color < 0 || color > 255) return color;

    limit = (host_colors >= 16) ? 16 : 8;

    if(color < limit) return color;

    // with 8 colors a bright color is its plain counterpart
    if(color < 16) return color - 8;

    // what the color number looks like
    if(color < 232)
    {
        // the 6 x 6 x 6 color cube
        n = color - 16;
        r = cube[n / 36];
        g = cube[(n / 6) % 6];
        b = cube[n % 6];
    }
    else
    {
        // the 24 step gray ramp
        r = g = b = 8 + (10 * (color - 232));
    }

    /*
        a gray is matched only against the grays (black, white and their
        bright forms).  by plain distance a dark gray is nearer to the
        console's brown than to black or white, and gray text must not
        come out colored.
    */
    lo = (r < g) ? r : g;
    if(b < lo) lo = b;
    hi = (r > g) ? r : g;
    if(b > hi) hi = b;
    gray = (hi - lo <= 24);

    /*
        the nearest color the terminal has.  on a tie the later one
        wins, which is the lighter of the two:  bright black is as far
        from black as from white, and it is better seen than not.
    */
    for(n = 0; n < limit; n++)
    {
        // colors 0, 7, 8 and 15 are the grays
        if(gray && (n % 8) != 0 && (n % 8) != 7) continue;

        dist = (r - ansi[n][0]) * (r - ansi[n][0])
             + (g - ansi[n][1]) * (g - ansi[n][1])
             + (b - ansi[n][2]) * (b - ansi[n][2]);

        if(best_dist < 0 || dist <= best_dist)
        {
            best = n;
            best_dist = dist;
        }
    }

    return best;
}

/* interprets a 'set attribute' (SGR) CSI escape sequence */
/*
    fit the foreground and background the child asked for to the colors
    the real terminal has, and keep the text readable while doing it.

    fitting each color on its own is not enough.  with 8 colors bright
    black has nowhere to go but black, so the dim text a program draws
    in bright black on a black background would vanish.  when two colors
    that the child asked for as different come out the same, the
    foreground is changed to black or white, whichever shows against the
    background.  colors the child asked for as the same are left the
    same:  that is how a program hides text on purpose.

    the caller has stored the newly asked-for color in fg / fg_asked /
    fg_given (or the bg equivalents).  the other color of the pair may
    have been set by a plain SGR 30 - 37 since it was last fitted, in
    which case it no longer equals what was given and is taken as it is.
*/
static void
_vtty_fit_colors(vtty_desc_t *v_desc, int host_colors)
{
    int     fg_asked, bg_asked;
    int     fg, bg;
    int     lum;

    // nothing to fit, and no reason to disturb what the child set
    if(host_colors <= 0 || host_colors >= 256) return;

    fg_asked = (v_desc->fg == v_desc->fg_given) ? v_desc->fg_asked : v_desc->fg;
    bg_asked = (v_desc->bg == v_desc->bg_given) ? v_desc->bg_asked : v_desc->bg;

    fg = vtty_fit_color(fg_asked, host_colors);
    bg = vtty_fit_color(bg_asked, host_colors);

    // two different colors became one:  make the text show
    if(fg == bg && fg_asked != bg_asked && bg >= 0 && bg < 16)
    {
        lum = ((ansi[bg][0] * 299) + (ansi[bg][1] * 587)
            + (ansi[bg][2] * 114)) / 1000;

        fg = (lum > 127) ? COLOR_BLACK : COLOR_WHITE;
    }

    v_desc->fg = fg;
    v_desc->fg_asked = fg_asked;
    v_desc->fg_given = fg;

    v_desc->bg = bg;
    v_desc->bg_asked = bg_asked;
    v_desc->bg_given = bg;

    return;
}

void
interpret_csi_SGR(vtty_t *vtty, int param[], int pcount)
{
    extern short    rRGB[], gRGB[], bRGB[];
    vtty_desc_t     *v_desc = NULL;
    int             i;
    short           mapped_color;
    int             processed = 0;
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

    // set vtty_desc buffer selector
    v_desc = vtty->v_desc_active;

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
            vtty->internal_state &= ~(STATE_ALT_CHARSET);
            continue;

        csi_sgr_SMACS:
            // code 11
            vtty->internal_state |= STATE_ALT_CHARSET;
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

            _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                v_desc->bg);

            continue;

        csi_sgr_XCOLOR_FG:
            // code 38
            // set custom foreground color
            fg = interpret_custom_color(vtty, &param[i], pcount - i, &processed);
            if(fg == -1)
            {
                i += (pcount - 1);
                continue;
            }

            mapped_color = vtty_get_mapped_color(vtty, fg);

            if(mapped_color != -1) fg = mapped_color;

            if(fg != -1)
            {
                v_desc->fg = fg;
                v_desc->fg_asked = fg;
                v_desc->fg_given = fg;

                // a terminal with 8 or 16 colors has no such color number
                _vtty_fit_colors(v_desc, _vtty_host_colors());

                _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                    v_desc->bg);
            }

            i += processed;
            continue;

        csi_sgr_RESET:
            // SGR 0
            // reset attributes
            v_desc->curattr = A_NORMAL;

            /*
                SGR 0 is an implicit color reset too.  the default
                pair's components are re-read from the cache on EVERY
                reset on purpose: default_colors is a pair NUMBER into
                the shared table, and sibling instances can recycle
                and rebind its contents -- snapshotting fg/bg at
                set-time is the f0b705c failure family.  but the
                resulting pair IS default_colors by construction, so
                the old fall-through into RESET_FG + RESET_BG (a
                second split, two find_pair walks, and a possible
                phantom add_pair for the intermediate fg/old-bg
                combination) collapses to this single split.
            */
            retval = color_cache_split_pair(v_desc->default_colors,
                &fg, &bg);

            if(retval != -1)
            {
                v_desc->fg = fg;
                v_desc->bg = bg;
                v_desc->colors = v_desc->default_colors;
            }
            else
            {
                v_desc->colors = 0;
            }

            continue;

        csi_sgr_RESET_FG:
            // code 39
            // reset fg color
            retval = color_cache_split_pair(v_desc->default_colors, &fg, &bg);

            if(retval != -1) v_desc->fg = fg;

            _vtty_set_color_pair_safe(v_desc, vtty, retval != -1 ? -1 : 0,
                v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_RESET_BG:
            // code 49
            // reset bg color
            retval = color_cache_split_pair(v_desc->default_colors, &fg, &bg);

            if(retval != -1) v_desc->bg = bg;

            _vtty_set_color_pair_safe(v_desc, vtty, retval != -1 ? -1 : 0,
                v_desc->fg, v_desc->bg);

            continue;

        csi_sgr_BG:
            // set bg color (case # - 40 = fg color)
            v_desc->bg = param[i] - 40;

            _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                v_desc->bg);

            continue;

        csi_sgr_XCOLOR_BG:
            // code 48
            // set custom background color
            bg = interpret_custom_color(vtty, &param[i], pcount - i, &processed);
            if(bg == -1)
            {
                i += (pcount - 1);
                continue;
            }

            mapped_color = vtty_get_mapped_color(vtty, bg);

            if(mapped_color != -1) bg = mapped_color;

            if(bg != -1)
            {
                v_desc->bg = bg;
                v_desc->bg_asked = bg;
                v_desc->bg_given = bg;

                // a terminal with 8 or 16 colors has no such color number
                _vtty_fit_colors(v_desc, _vtty_host_colors());

                _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                    v_desc->bg);
            }

            i += processed;
            continue;

        csi_sgr_AIX_FG:
            // codes 90 - 97
            // set 16 color fg (aixterm)
            if(vtty->flags & VTTY_FLAG_C16)
            {
                fg = param[i] - 90;
                v_desc->fg = vtty_add_mapped_color(vtty, fg + 90,
                    rRGB[fg], gRGB[fg], bRGB[fg]);
            }
            else
            {
                if(vtty->flags & VTTY_FLAG_C8) v_desc->fg = param[i] - 90;
                else
                {
                    v_desc->fg = (param[i] - 90) + 8;
                }

                /*
                    the child asked for a bright color whether or not
                    the terminal has one.  bright black on black is the
                    usual way to draw dim text, and it must not vanish.
                */
                v_desc->fg_asked = (param[i] - 90) + 8;
                v_desc->fg_given = v_desc->fg;

                _vtty_fit_colors(v_desc, _vtty_host_colors());
            }

            _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                v_desc->bg);

            continue;

        csi_sgr_AIX_BG:
            // codes 100 - 107
            // set 16 color bg (aixterm)
            if(vtty->flags & VTTY_FLAG_C16)
            {
                bg = param[i] - 100;
                v_desc->bg = vtty_add_mapped_color(vtty, bg + 100,
                    rRGB[bg], gRGB[bg], bRGB[bg]);
            }
            else
            {
                v_desc->bg = param[i] - 100;
                if((vtty->flags & VTTY_FLAG_C8) == 0) v_desc->bg += 8;

                // as for the foreground:  a bright color was asked for
                v_desc->bg_asked = (param[i] - 100) + 8;
                v_desc->bg_given = v_desc->bg;

                _vtty_fit_colors(v_desc, _vtty_host_colors());
            }

            _vtty_set_color_pair_safe(v_desc, vtty, -1, v_desc->fg,
                v_desc->bg);

            continue;

        csi_sgr_DEFAULT:
            continue;
    }
}

// inline void
inline void
_vtty_set_color_pair_safe(vtty_desc_t *v_desc, vtty_t *vtty, short colors,
    int fg, int bg)
{
    if(colors == -1 && vtty->pair_select != NULL)
        colors = vtty->pair_select(vtty, fg, bg);

    // find the required pair in the cache
    if(colors == -1) colors = color_cache_find_pair(fg, bg);

    // no color pair found so we'll try and add it (if requested)
    if(colors == -1) colors = color_cache_add_pair(vtty, fg, bg);

    // one addtl safeguard
    if(colors == -1) colors = 0;
    v_desc->colors = colors;

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
long
interpret_custom_color(vtty_t *vtty, int param[], int pcount, int *processed)
{
    int         method = 0;
    short       new_color = -1;

    *processed = 0;

    if(vtty == NULL) return -1;
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
        if(!(vtty->flags & VTTY_FLAG_TRUECOLOR))
        {
            if((pcount % 5) == 0)
            {
                *processed = 4;
                return _rgb_to_xterm256(param[2], param[3], param[4]);
            }

            if((pcount % 6) == 0)
            {
                *processed = 5;
                return _rgb_to_xterm256(param[3], param[4], param[5]);
            }

            *processed = pcount;
            return -1;
        }
        if((pcount % 5) == 0)
        {
            new_color = vtty_add_mapped_color(vtty, -1,
                (float)param[2], (float)param[3], (float)param[4]);

            *processed = 4;

            if(new_color >= 0) return new_color;
        }

        if((pcount % 6) == 0)
        {
            new_color = vtty_add_mapped_color(vtty, -1,
                (float)param[3], (float)param[4], (float)param[5]);

            *processed = 5;

            if(new_color >= 0) return new_color;
        }
    }

    *processed = pcount;
    return -1;
}
