/*
    Console glyph tests (headless).

    Pins vtty_fit_glyph() and vtty_glyph_fit_wanted():
      - ASCII, Latin and box drawing (rounded corners too) are untouched
      - table entries take their first stand-in, skipping stand-ins that
        some stock console fonts lack
      - braille keeps its shape: a mark at the height of a sparse cell's
        dots, a shade as dense as a full one
      - every stand-in is single width and is left alone in turn
      - double-width characters are never touched
      - with the host's own list, what the font has is left alone and
        only stand-ins the font has are used; NULL goes back
      - only a host named linux* wants any of it

    Build: cmake -DBUILD_TESTS=ON . && make test_glyph_fit
*/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <term.h>

#include "vtty.h"
#include "glyph_fit.h"

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

/* check what one character becomes */
static void
fit(wchar_t from, wchar_t want)
{
    wchar_t got = vtty_fit_glyph(from);

    CHECK(got == want, "U+%04X: got U+%04X want U+%04X",
        (unsigned)from, (unsigned)got, (unsigned)want);
}

int
main(void)
{
    /* the console font of the machine this was tuned on, in part */
    static const wchar_t terminus[] =
    {
        0x2026, 0x2191, 0x21B5, 0x2219, 0x221A, 0x2500, 0x2502, 0x2514,
        0x256D, 0x2588, 0x2591, 0x2592, 0x25CF, 0x203A, 0x2016,
    };
    wchar_t     wch;
    int         err;

    if(setlocale(LC_ALL, "C.UTF-8") == NULL)
        setlocale(LC_ALL, "en_US.UTF-8");

    // left alone: ASCII, Latin, box drawing the console has
    fit(L'A', L'A');
    fit(0x00B7, 0x00B7);                // ·
    fit(0x2500, 0x2500);                // ─
    fit(0x256D, 0x256D);                // ╭
    fit(0x2588, 0x2588);                // █

    // the table, without the host's list
    fit(0x276F, 0x203A);                // ❯  ›
    fit(0x23FA, 0x25CF);                // ⏺  ●
    fit(0x23BF, 0x2514);                // ⎿  └
    fit(0x2714, L'v');                  // ✔  √ is not in every font
    fit(0x23F8, L'|');                  // ⏸  nor is ‖
    fit(0x21E7, 0x2191);                // ⇧  ⇑ is not, ↑ is
    fit(0x2581, L'_');                  // ▁
    fit(0x258C, 0x2592);                // ▌  ▒
    fit(0x2463, L'4');                  // ④
    fit(0x2777, L'2');                  // ❷
    fit(0x273B, L'*');                  // ✻
    fit(0x2603, 0x2603);                // ☃ not in the table

    // braille
    fit(0x2800, L' ');                  // blank
    fit(0x2801, L'\'');                 // dot 1: top
    fit(0x28C0, L'.');                  // dots 7, 8: bottom
    fit(0x2807, L':');                  // dots 1, 2, 3: spread
    fit(0x2839, 0x2591);                // dots 1, 4, 5, 6: ░
    fit(0x287E, 0x2592);                // six dots: ▒
    fit(0x28FF, 0x2588);                // all eight: █

    // every stand-in is single width and stays as it is
    for(wch = 0x2000; wch < 0x2C00; wch++)
    {
        wchar_t got = vtty_fit_glyph(wch);

        if(got == wch) continue;

        CHECK(wcwidth(wch) == 1, "U+%04X is replaced but is %d wide",
            (unsigned)wch, wcwidth(wch));
        CHECK(wcwidth(got) == 1, "U+%04X becomes U+%04X, %d wide",
            (unsigned)wch, (unsigned)got, wcwidth(got));
        CHECK(vtty_fit_glyph(got) == got, "stand-in U+%04X is replaced",
            (unsigned)got);
    }

    // the host's list
    CHECK(vtty_set_host_glyphs(terminus,
        (int)(sizeof(terminus) / sizeof(terminus[0]))) == 0,
        "the host list is stored");
    fit(0x203A, 0x203A);                // › the font has
    fit(0x2714, 0x221A);                // ✔  √ now that the font has it
    fit(0x23F8, 0x2016);                // ⏸  ‖
    fit(0x21E7, 0x2191);                // ⇧  ⇑ missing, ↑ present
    fit(0x23FA, 0x25CF);                // ⏺  ●
    fit(0x2699, 0x2699);                // ⚙ not in the table: left alone

    // a font with braille keeps it
    {
        wchar_t braille[] = { 0x2801, 0x28FF };

        vtty_set_host_glyphs(braille, 2);
        fit(0x2801, 0x2801);
        fit(0x28FF, 0x28FF);
        fit(0x2802, L'\'');             // dot 2: not in that font
    }

    // and NULL goes back to the built-in choice
    vtty_set_host_glyphs(NULL, 0);
    fit(0x2714, L'v');
    fit(0x2801, L'\'');

    // only the console asks for any of it
    if(setupterm("linux", 1, &err) == OK)
    {
        CHECK(vtty_glyph_fit_wanted(), "linux wants stand-ins");
        del_curterm(cur_term);
    }
    if(setupterm("xterm-256color", 1, &err) == OK)
    {
        CHECK(!vtty_glyph_fit_wanted(), "xterm does not");
        del_curterm(cur_term);
    }

    if(failures == 0) printf("test_glyph_fit: all passed\n");

    return failures == 0 ? 0 : 1;
}
