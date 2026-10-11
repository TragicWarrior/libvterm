/*
    Mouse report encoding tests (headless, NOPTY | NOCURSES | EXTMOUSE).

    Events are handed to the driver the way an embedding host does it
    (vtty->pending_mouse), so no terminal and no ncurses screen is
    needed.

    Pins:
      - SGR (1006): left / middle / right press and release
      - modifiers on clicks, with xterm's bit values
        (shift 4, alt 8, ctrl 16)
      - wheel codes 64 / 65 plus modifiers; alternate scroll (1007)
      - motion is not reported in mode 1000
      - mode 1002 reports motion only while a button is held, once per
        cell, whether or not the host names the held button
      - mode 1003 reports motion with no button held (button 3)
      - resetting a tracking mode stops the driver
      - X10 encoding (1000 without 1006): button, release as 3,
        modifiers, motion, and the 223-cell limit

    Build: cmake -DBUILD_TESTS=ON . && make test_mouse
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtty.h"
#include "vtty_private.h"

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

/* render a NUL-terminated escape string */
static void
R(vtty_t *vt, const char *s)
{
    vtty_render(vt, (char *)s, (int)strlen(s));
}

/*
    hand one event to the installed driver and return what it would
    write to the child, as a C string ("" when nothing is reported).
*/
static const char *
ev(vtty_t *vt, mmask_t bstate, int x, int y)
{
    static unsigned char    buf[64];
    MEVENT                  me;
    ssize_t                 n = 0;

    memset(&me, 0, sizeof(me));
    me.bstate = bstate;
    me.x = x;
    me.y = y;

    memset(buf, 0, sizeof(buf));

    if(vt->mouse_driver != NULL)
    {
        vt->pending_mouse = &me;
        n = vt->mouse_driver(vt, buf);
        vt->pending_mouse = NULL;
    }

    if(n < 0) n = 0;
    buf[n] = '\0';

    return (const char *)buf;
}

/* compare a report with what was expected, naming the case on failure */
static void
expect(const char *name, const char *got, const char *want)
{
    CHECK(strcmp(got, want) == 0, "%s: got [%s] want [%s]",
        name, got + (got[0] == '\033'), want + (want[0] == '\033'));
}

int
main(void)
{
    vtty_t      *vt;
    char        x10[8];

    vt = vtty_create(80, 24,
        VTTY_FLAG_NOPTY | VTTY_FLAG_NOCURSES | VTTY_FLAG_EXTMOUSE);
    if(vt == NULL)
    {
        fprintf(stderr, "vtty_create failed\n");
        return 1;
    }

    CHECK(vt->mouse_driver == NULL, "no driver before the child asks");

    /* ---- SGR: buttons ---- */
    R(vt, "\033[?1000h\033[?1006h");
    CHECK(vt->mouse_driver != NULL, "driver installed by 1000 / 1006");

    expect("left press", ev(vt, BUTTON1_PRESSED, 5, 3), "\033[<0;5;3M");
    expect("left release", ev(vt, BUTTON1_RELEASED, 5, 3), "\033[<0;5;3m");
    expect("middle press", ev(vt, BUTTON2_PRESSED, 5, 3), "\033[<1;5;3M");
    expect("middle release", ev(vt, BUTTON2_RELEASED, 5, 3), "\033[<1;5;3m");
    expect("right press", ev(vt, BUTTON3_PRESSED, 5, 3), "\033[<2;5;3M");
    expect("right release", ev(vt, BUTTON3_RELEASED, 5, 3), "\033[<2;5;3m");

    /* ---- SGR: modifiers on clicks ---- */
    expect("shift+left", ev(vt, BUTTON1_PRESSED | BUTTON_SHIFT, 5, 3),
        "\033[<4;5;3M");
    expect("alt+left", ev(vt, BUTTON1_PRESSED | BUTTON_ALT, 5, 3),
        "\033[<8;5;3M");
    expect("ctrl+left", ev(vt, BUTTON1_PRESSED | BUTTON_CTRL, 5, 3),
        "\033[<16;5;3M");
    expect("ctrl+left release", ev(vt, BUTTON1_RELEASED | BUTTON_CTRL, 5, 3),
        "\033[<16;5;3m");
    expect("shift+right", ev(vt, BUTTON3_PRESSED | BUTTON_SHIFT, 5, 3),
        "\033[<6;5;3M");
    ev(vt, BUTTON3_RELEASED, 5, 3);

    /* ---- SGR: wheel ---- */
    expect("wheel up", ev(vt, BUTTON4_PRESSED, 9, 9), "\033[<64;9;9M");
    expect("wheel down", ev(vt, BUTTON5_PRESSED, 9, 9), "\033[<65;9;9M");
    expect("shift+wheel up", ev(vt, BUTTON4_PRESSED | BUTTON_SHIFT, 9, 9),
        "\033[<68;9;9M");
    expect("alt+wheel up", ev(vt, BUTTON4_PRESSED | BUTTON_ALT, 9, 9),
        "\033[<72;9;9M");
    expect("ctrl+wheel down", ev(vt, BUTTON5_PRESSED | BUTTON_CTRL, 9, 9),
        "\033[<81;9;9M");

    /* ---- mode 1000 does not report motion ---- */
    ev(vt, BUTTON1_PRESSED, 5, 3);
    expect("1000: drag not reported",
        ev(vt, REPORT_MOUSE_POSITION, 6, 3), "");
    expect("1000: ncurses-style drag is not a press",
        ev(vt, BUTTON1_PRESSED | REPORT_MOUSE_POSITION, 7, 3), "");
    ev(vt, BUTTON1_RELEASED, 7, 3);

    /* ---- mode 1002: motion while a button is held ---- */
    R(vt, "\033[?1002h");

    expect("1002: hover not reported",
        ev(vt, REPORT_MOUSE_POSITION, 20, 20), "");

    expect("1002: press", ev(vt, BUTTON1_PRESSED, 5, 3), "\033[<0;5;3M");
    expect("1002: drag", ev(vt, REPORT_MOUSE_POSITION, 6, 3),
        "\033[<32;6;3M");
    expect("1002: same cell not repeated",
        ev(vt, REPORT_MOUSE_POSITION, 6, 3), "");
    expect("1002: ctrl+drag",
        ev(vt, REPORT_MOUSE_POSITION | BUTTON_CTRL, 7, 4), "\033[<48;7;4M");
    expect("1002: release", ev(vt, BUTTON1_RELEASED, 7, 4), "\033[<0;7;4m");
    expect("1002: no motion after release",
        ev(vt, REPORT_MOUSE_POSITION, 8, 4), "");

    /* a host using ncurses names the held button in the motion event */
    expect("1002: ncurses-style right drag",
        ev(vt, BUTTON3_PRESSED | REPORT_MOUSE_POSITION, 9, 5),
        "\033[<34;9;5M");
    ev(vt, BUTTON3_RELEASED, 9, 5);

    /* ---- mode 1003: all motion ---- */
    R(vt, "\033[?1003h");

    expect("1003: hover", ev(vt, REPORT_MOUSE_POSITION, 30, 10),
        "\033[<35;30;10M");
    ev(vt, BUTTON2_PRESSED, 30, 10);
    expect("1003: middle drag", ev(vt, REPORT_MOUSE_POSITION, 31, 10),
        "\033[<33;31;10M");
    ev(vt, BUTTON2_RELEASED, 31, 10);

    /* ---- alternate scroll ---- */
    R(vt, "\033[?1007h");
    expect("altscroll up", ev(vt, BUTTON4_PRESSED, 9, 9), "\033OA");
    expect("altscroll down", ev(vt, BUTTON5_PRESSED, 9, 9), "\033OB");

    /* ---- resetting a tracking mode stops the driver ---- */
    R(vt, "\033[?1003l");
    CHECK(vt->mouse_driver == NULL, "1003l stops the driver");
    expect("stopped: nothing reported", ev(vt, BUTTON1_PRESSED, 5, 3), "");

    R(vt, "\033[?1002h\033[?1006h");
    CHECK(vt->mouse_driver != NULL, "1002 alone installs the driver");
    R(vt, "\033[?1002l");
    CHECK(vt->mouse_driver == NULL, "1002l stops the driver");

    /* ---- X10 encoding: 1000 without 1006 ---- */
    R(vt, "\033[?1000h");

    snprintf(x10, sizeof(x10), "\033[M%c%c%c", 32 + 0, 32 + 5, 32 + 3);
    expect("x10 left press", ev(vt, BUTTON1_PRESSED, 5, 3), x10);

    snprintf(x10, sizeof(x10), "\033[M%c%c%c", 32 + 3, 32 + 5, 32 + 3);
    expect("x10 release", ev(vt, BUTTON1_RELEASED, 5, 3), x10);

    snprintf(x10, sizeof(x10), "\033[M%c%c%c", 32 + 2, 32 + 5, 32 + 3);
    expect("x10 right press", ev(vt, BUTTON3_PRESSED, 5, 3), x10);
    ev(vt, BUTTON3_RELEASED, 5, 3);

    snprintf(x10, sizeof(x10), "\033[M%c%c%c", 32 + 1 + 16, 32 + 5, 32 + 3);
    expect("x10 ctrl+middle", ev(vt, BUTTON2_PRESSED | BUTTON_CTRL, 5, 3),
        x10);
    ev(vt, BUTTON2_RELEASED, 5, 3);

    expect("x10 beyond 223 dropped", ev(vt, BUTTON1_PRESSED, 300, 3), "");
    ev(vt, BUTTON1_RELEASED, 5, 3);

    expect("x10 wheel up", ev(vt, BUTTON4_PRESSED, 9, 9), "\033OA");

    R(vt, "\033[?1002h");
    ev(vt, BUTTON1_PRESSED, 5, 3);
    snprintf(x10, sizeof(x10), "\033[M%c%c%c", 32 + 0 + 32, 32 + 6, 32 + 3);
    expect("x10 drag", ev(vt, REPORT_MOUSE_POSITION, 6, 3), x10);
    ev(vt, BUTTON1_RELEASED, 6, 3);

    vtty_destroy(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: mouse test completed\n");
    return 0;
}
