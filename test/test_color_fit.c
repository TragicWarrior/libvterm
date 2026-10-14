/*
    Color fitting tests: an xterm-256 color number on a terminal with 8
    or 16 colors (headless).

    Pins vtty_fit_color():
      - a color the terminal has is returned unchanged
      - nothing changes on 256 colors, or when the count is not known
      - with 8 colors a bright color (8 - 15) is its plain counterpart
      - cube colors go to the nearest ANSI color of the right hue
      - grays go only to grays, never to a color
      - with 16 colors the bright forms are used
      - out-of-range numbers are returned unchanged

    and that SGR 38 / 48 (256-color and 24-bit forms) and the aixterm
    bright colors apply it, using the color count the color cache holds
    for the real terminal.

    and the pair rule:  two colors the child asked for as different do
    not become one (bright black on black stays readable), while colors
    asked for as the same stay the same.

    Build: cmake -DBUILD_TESTS=ON . && make test_color_fit
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_buffer.h"
#include "vtty_csi.h"
#include "color_cache.h"

static int failures = 0;

#define CHECK(_cond, ...)                                   \
            do                                              \
            {                                               \
                if(!(_cond))                                \
                {                                           \
                    fprintf(stderr, "FAIL: " __VA_ARGS__);  \
                    fprintf(stderr, "\n");                  \
                    failures++;                             \
                }                                           \
            }                                               \
            while(0)

/* check one color against what it should become on 8 and on 16 */
static void
fit(const char *name, int color, int want8, int want16)
{
    CHECK(vtty_fit_color(color, 8) == want8, "%s (%d) on 8: got %d want %d",
        name, color, vtty_fit_color(color, 8), want8);
    CHECK(vtty_fit_color(color, 16) == want16,
        "%s (%d) on 16: got %d want %d",
        name, color, vtty_fit_color(color, 16), want16);
}

/* render a NUL-terminated escape string */
static void
R(vtty_t *vt, const char *s)
{
    vtty_render(vt, (char *)s, (int)strlen(s));
}

int
main(void)
{
    extern color_cache_t    *color_cache;
    vtty_t                  *vt;
    vtty_desc_t             *std;
    int                     c;

    /* ---- colors the terminal has are left alone ---- */
    for(c = 0; c < 8; c++)
        CHECK(vtty_fit_color(c, 8) == c, "8: color %d kept", c);

    for(c = 0; c < 16; c++)
        CHECK(vtty_fit_color(c, 16) == c, "16: color %d kept", c);

    /* ---- nothing to do on 256 colors or an unknown terminal ---- */
    for(c = 0; c < 256; c++)
    {
        CHECK(vtty_fit_color(c, 256) == c, "256: color %d kept", c);
        CHECK(vtty_fit_color(c, 0) == c, "unknown: color %d kept", c);
        CHECK(vtty_fit_color(c, -1) == c, "unknown (-1): color %d kept", c);
    }

    /* ---- every result is a color the terminal has ---- */
    for(c = 0; c < 256; c++)
    {
        CHECK(vtty_fit_color(c, 8) >= 0 && vtty_fit_color(c, 8) < 8,
            "8: color %d fits (%d)", c, vtty_fit_color(c, 8));
        CHECK(vtty_fit_color(c, 16) >= 0 && vtty_fit_color(c, 16) < 16,
            "16: color %d fits (%d)", c, vtty_fit_color(c, 16));
    }

    /* an 88-color terminal is treated as a 16-color one */
    CHECK(vtty_fit_color(196, 88) == vtty_fit_color(196, 16), "88 colors");

    /* ---- out of range ---- */
    CHECK(vtty_fit_color(-1, 8) == -1, "-1 kept");
    CHECK(vtty_fit_color(256, 8) == 256, "256 kept");

    /* ---- with 8 colors a bright color is its plain counterpart ---- */
    for(c = 8; c < 16; c++)
        CHECK(vtty_fit_color(c, 8) == c - 8, "8: bright %d -> %d (%d)",
            c, c - 8, vtty_fit_color(c, 8));

    /* ---- the cube: pure colors keep their hue ---- */
    fit("red", 196, COLOR_RED, COLOR_RED);
    fit("green", 46, COLOR_GREEN, COLOR_GREEN);
    fit("blue", 21, COLOR_BLUE, COLOR_BLUE);
    fit("yellow", 226, COLOR_YELLOW, COLOR_YELLOW + 8);
    fit("magenta", 201, COLOR_MAGENTA, COLOR_MAGENTA + 8);
    fit("cyan", 51, COLOR_CYAN, COLOR_CYAN + 8);
    fit("dark red", 52, COLOR_RED, COLOR_RED);
    fit("dark blue", 17, COLOR_BLUE, COLOR_BLUE);
    fit("cube black", 16, COLOR_BLACK, COLOR_BLACK);
    fit("cube white", 231, COLOR_WHITE, COLOR_WHITE + 8);

    /* ---- grays go to grays (0, 7, 8, 15), never to a color ---- */
    for(c = 232; c < 256; c++)
    {
        int on8 = vtty_fit_color(c, 8);
        int on16 = vtty_fit_color(c, 16);

        CHECK(on8 == COLOR_BLACK || on8 == COLOR_WHITE,
            "gray %d on 8 is %d", c, on8);
        CHECK((on16 % 8) == COLOR_BLACK || (on16 % 8) == COLOR_WHITE,
            "gray %d on 16 is %d", c, on16);
    }

    /* the grays a dark-themed program draws with */
    fit("near black (20)", 233, COLOR_BLACK, COLOR_BLACK);
    fit("dim (88)", 240, COLOR_WHITE, COLOR_BLACK + 8);
    fit("mid (128)", 244, COLOR_WHITE, COLOR_WHITE);
    fit("light (225)", 253, COLOR_WHITE, COLOR_WHITE + 8);

    /* ---- SGR 38 / 48 apply it for the real terminal's color count ---- */
    vt = vtty_create(80, 24, VTTY_FLAG_NOPTY | VTTY_FLAG_NOCURSES);
    if(vt == NULL)
    {
        fprintf(stderr, "vtty_create failed\n");
        return 1;
    }

    std = &vt->vtty_desc[VTTY_BUF_STANDARD];

    if(color_cache == NULL)
    {
        fprintf(stderr, "no color cache\n");
        return 1;
    }

    /* an 8-color terminal */
    color_cache->term_colors = 8;

    R(vt, "\033[38;5;196m");
    CHECK(std->fg == COLOR_RED, "8: 38;5;196 fg %d", std->fg);
    R(vt, "\033[48;5;21m");
    CHECK(std->bg == COLOR_BLUE, "8: 48;5;21 bg %d", std->bg);

    /* 24-bit:  light gray text on a near-black background */
    R(vt, "\033[38;2;225;225;225;48;2;20;20;20m");
    CHECK(std->fg == COLOR_WHITE, "8: 24-bit fg %d", std->fg);
    CHECK(std->bg == COLOR_BLACK, "8: 24-bit bg %d", std->bg);

    R(vt, "\033[38;2;255;0;0m");
    CHECK(std->fg == COLOR_RED, "8: 24-bit red fg %d", std->fg);

    /* a bright color asked for by number */
    R(vt, "\033[38;5;12m");
    CHECK(std->fg == COLOR_BLUE, "8: 38;5;12 fg %d", std->fg);

    /*
        ---- two different colors must not become one ----
        with 8 colors bright black has nowhere to go but black.  dim
        text drawn in it on a black background has to stay readable.
    */
    R(vt, "\033[38;5;8;48;5;0m");
    CHECK(std->fg == COLOR_WHITE && std->bg == COLOR_BLACK,
        "8: bright black on black -> fg %d bg %d", std->fg, std->bg);

    /* the background given first */
    R(vt, "\033[48;5;0m\033[38;5;8m");
    CHECK(std->fg == COLOR_WHITE && std->bg == COLOR_BLACK,
        "8: bg then fg -> fg %d bg %d", std->fg, std->bg);

    /* the aixterm form of bright black */
    R(vt, "\033[48;5;0m\033[90m");
    CHECK(std->fg == COLOR_WHITE && std->bg == COLOR_BLACK,
        "8: SGR 90 on black -> fg %d bg %d", std->fg, std->bg);

    /* on a light background the same color fits without help */
    R(vt, "\033[38;5;8;48;5;7m");
    CHECK(std->fg == COLOR_BLACK && std->bg == COLOR_WHITE,
        "8: bright black on white -> fg %d bg %d", std->fg, std->bg);

    /* the background changes under text that was already adjusted */
    R(vt, "\033[38;5;8;48;5;0m");
    R(vt, "\033[48;5;7m");
    CHECK(std->fg == COLOR_BLACK && std->bg == COLOR_WHITE,
        "8: bg changed under dim text -> fg %d bg %d", std->fg, std->bg);

    /* bright white on a bright black background */
    R(vt, "\033[38;5;15;48;5;8m");
    CHECK(std->fg == COLOR_WHITE && std->bg == COLOR_BLACK,
        "8: bright white on bright black -> fg %d bg %d", std->fg, std->bg);

    /* bright white on white:  black text */
    R(vt, "\033[38;5;15;48;5;7m");
    CHECK(std->fg == COLOR_BLACK && std->bg == COLOR_WHITE,
        "8: bright white on white -> fg %d bg %d", std->fg, std->bg);

    /* 24-bit dim gray on near black */
    R(vt, "\033[38;2;80;80;88;48;2;20;20;20m");
    CHECK(std->fg == COLOR_WHITE && std->bg == COLOR_BLACK,
        "8: 24-bit dim on dark -> fg %d bg %d", std->fg, std->bg);

    /* colors asked for as the same stay the same:  hidden on purpose */
    R(vt, "\033[38;5;0;48;5;0m");
    CHECK(std->fg == COLOR_BLACK && std->bg == COLOR_BLACK,
        "8: black on black kept -> fg %d bg %d", std->fg, std->bg);

    R(vt, "\033[38;2;20;20;20;48;2;20;20;20m");
    CHECK(std->fg == COLOR_BLACK && std->bg == COLOR_BLACK,
        "8: 24-bit same on same kept -> fg %d bg %d", std->fg, std->bg);

    R(vt, "\033[38;5;8;48;5;8m");
    CHECK(std->fg == std->bg, "8: bright black on itself kept -> fg %d bg %d",
        std->fg, std->bg);

    /* a 16-color terminal */
    color_cache->term_colors = 16;

    /* it has bright black, so nothing needs adjusting */
    R(vt, "\033[38;5;8;48;5;0m");
    CHECK(std->fg == 8 && std->bg == COLOR_BLACK,
        "16: bright black on black -> fg %d bg %d", std->fg, std->bg);

    R(vt, "\033[38;2;225;225;225;48;2;20;20;20m");
    CHECK(std->fg == COLOR_WHITE + 8, "16: 24-bit fg %d", std->fg);
    CHECK(std->bg == COLOR_BLACK, "16: 24-bit bg %d", std->bg);

    R(vt, "\033[38;5;12m");
    CHECK(std->fg == 12, "16: 38;5;12 fg %d", std->fg);

    /* a 256-color terminal:  the color number is used as it is */
    color_cache->term_colors = 256;

    R(vt, "\033[38;5;196m\033[48;5;21m");
    CHECK(std->fg == 196 && std->bg == 21, "256: fg %d bg %d",
        std->fg, std->bg);

    R(vt, "\033[38;2;225;225;225m");
    CHECK(std->fg == 253, "256: 24-bit fg %d", std->fg);

    vtty_destroy(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: color fit test completed\n");
    return 0;
}
