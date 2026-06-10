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

    return vt;
}

static void
destroy_vterm(vterm_t *vt)
{
    vterm_buffer_dealloc(vt, VTERM_BUF_STANDARD);
    vterm_buffer_dealloc(vt, VTERM_BUF_HISTORY);
    free(vt);
}

#define CELL(_vt, _r, _c) \
            ((_vt)->vterm_desc[VTERM_BUF_STANDARD].cells[(_r)][(_c)])

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

    destroy_vterm(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: render test completed\n");
    return 0;
}
