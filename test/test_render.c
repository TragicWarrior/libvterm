/*
    Content-verifying harness for the vterm_render -> vterm_put_char
    glyph path.

    Pins the cell-write contract before/after put_char refactors:
      - plain ASCII lands with the current attr + colors and advances
        the cursor
      - attribute SGRs (bold / reverse / underline, on and off) are
        carried by subsequent glyphs
      - SO / SI (0x0E / 0x0F) set and clear A_ALTCHARSET
      - UTF-8 multibyte glyphs decode to wch[0], terminate at wch[1],
        and advance one column
      - wrap at the right margin moves to column 0 of the next row
      - written cells are marked dirty
    Color-changing SGRs are deliberately NOT exercised: they route
    through the color cache, which needs a live curses terminal.

    "bench" argv streams ~16MB of plain text lines through an 80x24
    instance (put_char fast path + scroll + erase + history capture)
    and prints ms.

    Build: cmake -DBUILD_TESTS=ON . && make test_render
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
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

static vterm_t *
make_vterm(int width, int height)
{
    vterm_t *vt;

    vt = vterm_alloc();
    vt->v_desc_active = &vt->vterm_desc[VTERM_BUF_STANDARD];

    vterm_buffer_alloc(vt, VTERM_BUF_STANDARD, width, height);
    vterm_buffer_alloc(vt, VTERM_BUF_HISTORY, width, height * 4);

    vterm_erase(vt, VTERM_BUF_STANDARD, 0);
    vterm_erase(vt, VTERM_BUF_HISTORY, '-');

    /*
        headless, tigetnum() fails so the cache initializes EMPTY --
        split/find walk nothing and report misses.  that makes the
        SGR 0 failure branch reachable without curses.
    */
    color_cache_init();

    return vt;
}

static void
destroy_vterm(vterm_t *vt)
{
    color_cache_release();
    vterm_buffer_dealloc(vt, VTERM_BUF_STANDARD);
    vterm_buffer_dealloc(vt, VTERM_BUF_HISTORY);
    free(vt);
}


/* render a NUL-terminated escape string -- no hand-counted lengths */
static void
R(vterm_t *vt, const char *s)
{
    vterm_render(vt, (char *)s, (int)strlen(s));
}

#define CELL(_vt, _r, _c) \
            ((_vt)->vterm_desc[VTERM_BUF_STANDARD].cells[(_r)][(_c)])

static void
clear_dirty(vterm_desc_t *d)
{
    int r;

    for(r = 0; r < d->rows; r++) VCELL_DIRTY_CLR_ROW(d, r);
}

static void
run_bench(void)
{
    vterm_t         *vt;
    struct timespec t0, t1;
    char            line[81];
    long            ms;
    int             i;

    vt = make_vterm(80, 24);

    memset(line, 'x', 79);
    line[79] = '\n';
    line[80] = '\0';

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for(i = 0; i < 200000; i++)
        vterm_render(vt, line, 80);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    ms = (t1.tv_sec - t0.tv_sec) * 1000L
        + (t1.tv_nsec - t0.tv_nsec) / 1000000L;

    printf("bench: 200000 lines (16MB) through 80x24: %ld ms\n", ms);

    destroy_vterm(vt);
}

int
main(int argc, char **argv)
{
    vterm_t         *vt;
    vterm_desc_t    *std;
    int             c;

    if(argc > 1 && strcmp(argv[1], "bench") == 0)
    {
        run_bench();
        return 0;
    }

    /* full-screen clear storm: ED 2 is the largest span workload */
    if(argc > 1 && strcmp(argv[1], "bench-clear") == 0)
    {
        struct timespec t0, t1;
        long            ms;
        int             i;

        vt = make_vterm(80, 24);

        clock_gettime(CLOCK_MONOTONIC, &t0);

        for(i = 0; i < 200000; i++)
            R(vt, "\033[2Jxx");

        clock_gettime(CLOCK_MONOTONIC, &t1);

        ms = (t1.tv_sec - t0.tv_sec) * 1000L
            + (t1.tv_nsec - t0.tv_nsec) / 1000000L;

        printf("bench-clear: 200000 full-screen ED2: %ld ms\n", ms);

        destroy_vterm(vt);
        return 0;
    }

    vt = make_vterm(20, 6);
    std = &vt->vterm_desc[VTERM_BUF_STANDARD];

    /* ---- plain ASCII ---- */
    vterm_render(vt, "Hello", 5);

    CHECK(CELL(vt, 0, 0).wch[0] == L'H', "plain: cell 0");
    CHECK(CELL(vt, 0, 4).wch[0] == L'o', "plain: cell 4");
    CHECK(CELL(vt, 0, 4).wch[1] == L'\0', "plain: wch[1] terminator");
    CHECK(CELL(vt, 0, 0).attr == A_NORMAL, "plain: attr normal");
    CHECK(CELL(vt, 0, 0).colors == std->colors, "plain: colors");
    CHECK(std->ccol == 5 && std->crow == 0, "plain: cursor %d,%d",
        std->ccol, std->crow);

    /* ---- attribute SGRs carried by glyphs ---- */
    vterm_render(vt, "\033[7mR\033[27m\033[1mB\033[22mN", 22);

    CHECK(CELL(vt, 0, 5).wch[0] == L'R', "sgr: reverse glyph");
    CHECK(CELL(vt, 0, 5).attr & A_REVERSE, "sgr: reverse attr");
    CHECK(CELL(vt, 0, 6).wch[0] == L'B', "sgr: bold glyph");
    CHECK((CELL(vt, 0, 6).attr & A_BOLD)
        && !(CELL(vt, 0, 6).attr & A_REVERSE), "sgr: bold only");
    CHECK(CELL(vt, 0, 7).wch[0] == L'N', "sgr: normal glyph");
    CHECK(CELL(vt, 0, 7).attr == A_NORMAL, "sgr: attrs dropped");

    /* ---- SO / SI alternate charset ---- */
    vterm_render(vt, "\x0Eq\x0Fq", 4);

    CHECK(CELL(vt, 0, 8).attr & A_ALTCHARSET, "acs: SO sets A_ALTCHARSET");
    CHECK(!(CELL(vt, 0, 9).attr & A_ALTCHARSET), "acs: SI clears it");

    /* ---- UTF-8 multibyte ---- */
    vterm_render(vt, "\xC3\xA9", 2);            /* e-acute, U+00E9 */

    CHECK(CELL(vt, 0, 10).wch[0] == 0xE9, "utf8: codepoint 0x%X",
        (unsigned int)CELL(vt, 0, 10).wch[0]);
    CHECK(CELL(vt, 0, 10).wch[1] == L'\0', "utf8: wch[1] terminator");
    CHECK(CELL(vt, 0, 10).attr == A_NORMAL, "utf8: attr");
    CHECK(std->ccol == 11, "utf8: single advance (ccol %d)", std->ccol);

    /* ---- written cells are dirty ---- */
    for(c = 0; c <= 10; c++)
        CHECK(VCELL_DIRTY_TEST(std, 0, c), "dirty: col %d", c);

    /* ---- CR / LF ---- */
    vterm_render(vt, "\r\nx", 3);

    CHECK(std->crow == 1, "crlf: row %d", std->crow);
    CHECK(CELL(vt, 1, 0).wch[0] == L'x', "crlf: glyph at row 1 col 0");

    /* ---- wrap at the right margin ---- */
    vterm_render(vt, "\r\n", 2);                /* row 2, col 0 */
    vterm_render(vt, "abcdefghijklmnopqrstUV", 22);

    CHECK(CELL(vt, 2, 19).wch[0] == L't', "wrap: last col of row 2");
    CHECK(CELL(vt, 3, 0).wch[0] == L'U', "wrap: first col of row 3");
    CHECK(CELL(vt, 3, 1).wch[0] == L'V', "wrap: second col of row 3");
    CHECK(std->crow == 3 && std->ccol == 2, "wrap: cursor %d,%d",
        std->ccol, std->crow);

    /* ---- SGR 0: attr reset + implicit color reset ----
       with the headless-empty cache the default-pair split fails, so
       the contract here is the failure branch: attrs drop to
       A_NORMAL, the pair falls back to 0, fg/bg untouched.  (the
       populated-cache branch -- colors = default_colors -- needs live
       curses; covered interactively via ls --color in vshell.) */
    std->colors = 5;
    R(vt, "\033[7m\033[0mZ");

    CHECK(CELL(vt, 3, 2).wch[0] == L'Z', "sgr0: glyph");
    CHECK(CELL(vt, 3, 2).attr == A_NORMAL, "sgr0: attrs dropped");
    CHECK(CELL(vt, 3, 2).colors == 0, "sgr0: pair fell back to 0 (%d)",
        CELL(vt, 3, 2).colors);
    CHECK(std->colors == 0, "sgr0: desc colors (%d)", std->colors);

    /* bare \e[m is SGR 0 too */
    std->colors = 5;
    R(vt, "\033[7m\033[mZ");
    CHECK(std->colors == 0 && CELL(vt, 3, 3).attr == A_NORMAL,
        "sgr0: bare \\e[m form");

    /*
        ==== erase / fill flavor contract ====

        the erase-family loops come in three flavors.  poke colors and
        default_colors to DIFFERENT values so the assertions can tell
        them apart (color SGRs would route through the color cache,
        which needs live curses -- direct field pokes don't).
    */
    std->colors = 7;            /* "current" colors  */
    std->default_colors = 3;    /* "default" colors  */

    /* ---- ED 2 (\e[2J): bce flavor -- ' ' + curattr + current colors.
       set reverse first so attr carriage is visible ---- */
    R(vt, "\033[7m\033[2J");

    CHECK(CELL(vt, 1, 5).wch[0] == L' ', "ed2: blank");
    CHECK(CELL(vt, 1, 5).attr & A_REVERSE, "ed2: carries curattr");
    CHECK(CELL(vt, 1, 5).colors == 7, "ed2: current colors (%d)",
        CELL(vt, 1, 5).colors);
    CHECK(CELL(vt, 5, 19).wch[0] == L' ', "ed2: last cell");
    R(vt, "\033[27m");

    /* ED 2 quirk: default_colors adopted current colors */
    CHECK(std->default_colors == 7, "ed2: default_colors quirk (%d)",
        std->default_colors);
    std->default_colors = 3;    /* restore for later flavor checks */

    /* ---- EL variants: span + dirty-bit boundaries ---- */
    R(vt, "\033[2;1H");           /* row 1 (0-based), col 0 */
    R(vt, "QQQQQQQQQQQQQQQQQQQQ");
    clear_dirty(std);

    R(vt, "\033[2;4H\033[K");     /* EL 0 from col 3 */

    CHECK(CELL(vt, 1, 2).wch[0] == L'Q', "el0: col 2 untouched");
    for(c = 3; c < 20; c++)
        CHECK(CELL(vt, 1, c).wch[0] == L' ', "el0: col %d blank", c);
    for(c = 0; c < 3; c++)
        CHECK(!VCELL_DIRTY_TEST(std, 1, c), "el0: col %d stays clean", c);
    for(c = 3; c < 20; c++)
        CHECK(VCELL_DIRTY_TEST(std, 1, c), "el0: col %d dirty", c);

    R(vt, "\033[3;1H");           /* row 2, col 0 */
    R(vt, "RRRRRRRRRRRRRRRRRRRR");
    clear_dirty(std);

    R(vt, "\033[3;18H\033[1K");  /* EL 1 to col 17 */

    for(c = 0; c <= 17; c++)
        CHECK(CELL(vt, 2, c).wch[0] == L' ', "el1: col %d blank", c);
    CHECK(CELL(vt, 2, 18).wch[0] == L'R', "el1: col 18 untouched");
    CHECK(VCELL_DIRTY_TEST(std, 2, 17), "el1: col 17 dirty");
    CHECK(!VCELL_DIRTY_TEST(std, 2, 18), "el1: col 18 stays clean");

    /* ---- ECH: n cells from the cursor, bce flavor ---- */
    R(vt, "\033[4;1HSSSSSSSSSS");
    R(vt, "\033[4;3H\033[4X");   /* 4 cells from col 2 */

    CHECK(CELL(vt, 3, 1).wch[0] == L'S', "ech: col 1 untouched");
    for(c = 2; c <= 5; c++)
    {
        CHECK(CELL(vt, 3, c).wch[0] == L' ', "ech: col %d blank", c);
        CHECK(CELL(vt, 3, c).colors == 7, "ech: col %d colors", c);
    }
    CHECK(CELL(vt, 3, 6).wch[0] == L'S', "ech: col 6 untouched");

    /* ---- ICH: shift right, inserted cells blank with bce flavor ---- */
    R(vt, "\033[5;1HTTTTT");
    R(vt, "\033[5;1H\033[2@");   /* insert 2 at col 0 */

    CHECK(CELL(vt, 4, 0).wch[0] == L' ', "ich: inserted blank");
    CHECK(CELL(vt, 4, 1).wch[0] == L' ', "ich: inserted blank 2");
    CHECK(CELL(vt, 4, 2).wch[0] == L'T', "ich: shifted content");

    /* ---- IL / DL: vacated rows blank with bce flavor ---- */
    R(vt, "\033[1;1H\033[2J");
    std->default_colors = 3;    /* the ED 2 quirk re-adopted current */
    R(vt, "\033[1;1HAAAA\r\nBBBB\r\nCCCC");
    R(vt, "\033[1;1H\033[1L");   /* insert line at row 0 */

    CHECK(CELL(vt, 0, 0).wch[0] == L' ', "il: new row blank");
    CHECK(CELL(vt, 1, 0).wch[0] == L'A', "il: pushed row");
    CHECK(CELL(vt, 2, 0).wch[0] == L'B', "il: pushed row 2");

    R(vt, "\033[1;1H\033[1M");   /* delete line at row 0 */

    CHECK(CELL(vt, 0, 0).wch[0] == L'A', "dl: rows pulled up");
    CHECK(CELL(vt, 1, 0).wch[0] == L'B', "dl: rows pulled up 2");
    CHECK(CELL(vt, 5, 0).wch[0] == L' ', "dl: vacated bottom blank");
    CHECK(CELL(vt, 5, 0).colors == 7, "dl: vacated bce colors");

    /* ---- DECALN: reset flavor -- 'E' + A_NORMAL + default colors ---- */
    R(vt, "\033#8");

    CHECK(CELL(vt, 2, 10).wch[0] == L'E', "decaln: E fill");
    CHECK(CELL(vt, 2, 10).attr == A_NORMAL, "decaln: A_NORMAL");
    CHECK(CELL(vt, 2, 10).colors == 3, "decaln: default colors (%d)",
        CELL(vt, 2, 10).colors);

    /* ---- erase_row(FALSE): preserve flavor -- per-cell colors
       must SURVIVE (the ALT-scroll / esc-IND path) ---- */
    for(c = 0; c < std->cols; c++)
    {
        CELL(vt, 4, c).wch[0] = L'Z';
        CELL(vt, 4, c).colors = 40 + c;         /* distinct per cell */
        CELL(vt, 4, c).attr = A_REVERSE;
    }

    vterm_erase_row(vt, 4, FALSE, 0);

    for(c = 0; c < std->cols; c++)
    {
        CHECK(CELL(vt, 4, c).wch[0] == L' ', "rowF: col %d blank", c);
        CHECK(CELL(vt, 4, c).attr == A_NORMAL, "rowF: col %d attr", c);
        CHECK(CELL(vt, 4, c).colors == 40 + c,
            "rowF: col %d colors PRESERVED (%d)", c, CELL(vt, 4, c).colors);
    }

    /* ---- erase_row(TRUE): reset flavor ---- */
    vterm_erase_row(vt, 4, TRUE, 0);

    for(c = 0; c < std->cols; c++)
        CHECK(CELL(vt, 4, c).colors == 3, "rowT: col %d default colors", c);

    /* ---- vterm_erase_cols: reset flavor from a start column ---- */
    R(vt, "\033[1;1H\033[2J");
    std->default_colors = 9;    /* distinct from current (7) */
    R(vt, "\033[1;1HXXXXXXXXXX");
    vterm_erase_cols(vt, 5, 0);

    CHECK(CELL(vt, 0, 4).wch[0] == L'X', "cols: col 4 untouched");
    CHECK(CELL(vt, 0, 5).wch[0] == L' ', "cols: col 5 blank");
    CHECK(CELL(vt, 5, 19).wch[0] == L' ', "cols: last row blank");
    CHECK(CELL(vt, 0, 5).colors == 9, "cols: default colors (%d)",
        CELL(vt, 0, 5).colors);

    destroy_vterm(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: render test completed\n");
    return 0;
}
