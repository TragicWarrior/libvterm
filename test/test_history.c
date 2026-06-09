/*
    Contract harness for the scrollback history buffer.

    Drives the PRODUCTION capture path (vterm_scroll_up with the cursor
    pinned to the bottom margin) plus vterm_set_history_size and the
    width-realloc path, and verifies history content through the logical
    row accessor (vterm_desc_row_phys) -- so the same assertions hold for
    the legacy shift-the-whole-buffer implementation (head always 0) and
    the ring implementation (head rotates).

    Contract checked:
      - history holds the last hist_rows captured rows, oldest ->
        newest in logical order, '-' fill before first wrap
      - capture count > hist_rows wraps correctly
      - set_history_size shrink keeps the NEWEST rows
      - set_history_size grow keeps all content at the logical tail
        (rows above it are unspecified: legacy leaves stale copies,
        ring leaves blanks)
      - width realloc preserves logical content
      - more captures after each resize land correctly

    "bench" argv runs 200k captures at 80x24/hist 96 and prints ms.

    Build: cmake -DBUILD_TESTS=ON . && make test_history
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

// unique, never-colliding row marker for capture number k
#define MARK(k)     ((wchar_t)(0x2000 + (k)))

static vterm_t *
make_vterm(int width, int height, int hist_rows)
{
    vterm_t *vt;

    vt = vterm_alloc();

    // mirror the vterm_init() bits the buffer paths rely on
    vt->v_desc_active = &vt->vterm_desc[VTERM_BUF_STANDARD];

    vterm_buffer_alloc(vt, VTERM_BUF_STANDARD, width, height);
    vterm_buffer_alloc(vt, VTERM_BUF_HISTORY, width, hist_rows);
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

/* run one capture: stamp STANDARD row 0 with MARK(k), pin the cursor to
   the bottom margin, scroll -- row 0 is what scroll_up sends to history */
static void
capture(vterm_t *vt, int k)
{
    vterm_desc_t    *std = &vt->vterm_desc[VTERM_BUF_STANDARD];
    int             c;

    for(c = 0; c < std->cols; c++)
    {
        std->cells[0][c].wch[0] = MARK(k);
        std->cells[0][c].wch[1] = L'\0';
    }

    std->crow = std->scroll_max;
    std->ccol = 0;

    vterm_scroll_up(vt, FALSE);
}

/* TRUE when every logical cell of LOGICAL history row r holds ch */
static int
hist_row_is(vterm_t *vt, int r, wchar_t ch)
{
    vterm_desc_t    *hist = &vt->vterm_desc[VTERM_BUF_HISTORY];
    int             phys = vterm_desc_row_phys(hist, r);
    int             c;

    for(c = 0; c < hist->cols; c++)
    {
        if(hist->cells[phys][c].wch[0] != ch) return 0;
    }

    return 1;
}

/*
    after captures 1..k into a hist_rows history, logical row j must hold:
    capture (k - (hist_rows - 1 - j)) when that's >= first_k, else fill.
*/
static void
check_tail(vterm_t *vt, int k, int first_k, wchar_t fill, const char *tag)
{
    vterm_desc_t    *hist = &vt->vterm_desc[VTERM_BUF_HISTORY];
    int             j;

    for(j = 0; j < hist->rows; j++)
    {
        int cap = k - (hist->rows - 1 - j);

        if(cap >= first_k)
        {
            CHECK(hist_row_is(vt, j, MARK(cap)),
                "%s: logical row %d != capture %d", tag, j, cap);
        }
        else if(fill != L'\0')
        {
            CHECK(hist_row_is(vt, j, fill),
                "%s: logical row %d != fill", tag, j);
        }
    }
}

static void
run_bench(void)
{
    vterm_t         *vt;
    struct timespec t0, t1;
    long            ms;
    int             k;

    vt = make_vterm(80, 24, 96);

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for(k = 1; k <= 200000; k++) capture(vt, k);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    ms = (t1.tv_sec - t0.tv_sec) * 1000L
        + (t1.tv_nsec - t0.tv_nsec) / 1000000L;

    printf("bench: 200000 captures, 80x24 / hist 96: %ld ms\n", ms);

    destroy_vterm(vt);
}

int
main(int argc, char **argv)
{
    vterm_t         *vt;
    vterm_desc_t    *hist;
    int             k = 0;
    int             i;

    if(argc > 1 && strcmp(argv[1], "bench") == 0)
    {
        run_bench();
        return 0;
    }

    /* odd history size to make modulo mistakes show up */
    vt = make_vterm(8, 6, 11);
    hist = &vt->vterm_desc[VTERM_BUF_HISTORY];

    /* ---- partial fill: 4 captures into 11 rows ---- */
    while(k < 4) capture(vt, ++k);
    check_tail(vt, k, 1, L'-', "partial");

    /* ---- wrap: push well past capacity ---- */
    while(k < 30) capture(vt, ++k);
    check_tail(vt, k, k - hist->rows + 1, L'\0', "wrapped");

    /* ---- width realloc (the vterm_resize_full path) while populated.
       resize_full keeps STANDARD and HISTORY at the SAME width; the
       harness must mirror that or captures only carry min() columns ---- */
    vterm_buffer_realloc(vt, VTERM_BUF_STANDARD, 6, -1);
    vterm_buffer_realloc(vt, VTERM_BUF_HISTORY, 6, -1);
    CHECK(hist->cols == 6, "realloc: cols %d", hist->cols);
    check_tail(vt, k, k - hist->rows + 1, L'\0', "post-narrow");

    vterm_buffer_realloc(vt, VTERM_BUF_STANDARD, 10, -1);
    vterm_buffer_realloc(vt, VTERM_BUF_HISTORY, 10, -1);
    CHECK(hist->cols == 10 && hist->max_cols >= 10,
        "realloc: cols %d max_cols %d", hist->cols, hist->max_cols);
    /* widened rows gained blank tail cells; only column 0 still
       carries the marker on pre-widen rows */
    for(i = 0; i < hist->rows; i++)
    {
        int     cap = k - (hist->rows - 1 - i);
        int     phys = vterm_desc_row_phys(hist, i);

        CHECK(hist->cells[phys][0].wch[0] == MARK(cap),
            "post-widen: logical row %d col 0 != capture %d", i, cap);
    }

    /* ---- captures keep landing correctly after realloc.  push past
       capacity so every pre-widen (partial-width) row is flushed and
       full-width checks are strict again ---- */
    while(k < 42) capture(vt, ++k);
    check_tail(vt, k, k - hist->rows + 1, L'\0', "post-realloc-captures");

    /* ---- grow: content must end up at the logical tail ---- */
    vterm_set_history_size(vt, 17);
    CHECK(hist->rows == 17, "grow: rows %d", hist->rows);
    /* logical rows 6..16 = the 11 surviving captures; 0..5 unspecified */
    for(i = 6; i < 17; i++)
    {
        int cap = k - (16 - i);

        CHECK(hist_row_is(vt, i, MARK(cap)),
            "grow: logical row %d != capture %d", i, cap);
    }

    /* ---- captures into the grown buffer: after 17 more, every
       unspecified row has been pushed out and all 17 are markers ---- */
    while(k < 60) capture(vt, ++k);
    check_tail(vt, k, k - hist->rows + 1, L'\0', "post-grow-captures");

    /* ---- shrink: NEWEST rows must survive ---- */
    vterm_set_history_size(vt, 5);
    CHECK(hist->rows == 5, "shrink: rows %d", hist->rows);
    check_tail(vt, k, k - 5 + 1, L'\0', "shrunk");

    /* ---- captures into the shrunken buffer ---- */
    while(k < 70) capture(vt, ++k);
    check_tail(vt, k, k - 5 + 1, L'\0', "post-shrink-captures");

    destroy_vterm(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: history test completed\n");
    return 0;
}
