#ifndef _MOUSE_DRIVER_H_
#define _MOUSE_DRIVER_H_

#include <signal.h>

#ifdef __linux__
#include <ncursesw/curses.h>
#endif

#ifdef __FreeBSD__
#include <ncurses.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
// #include <ncurses.h>
#include <ncursesw/ncurses.h>
#endif

#include "vtty.h"
#include "vtty_private.h"

typedef struct _mouse_state_s   mouse_state_t;

struct _mouse_state_s
{
    vtty_t          *origin;
    sig_atomic_t    ref_count;
    int             btn_event_count;                //  instances in mode 1002
    int             any_event_count;                //  instances in mode 1003
    int             host_motion;                    /*
                                                        motion mode enabled
                                                        on the real terminal
                                                        (1002, 1003 or 0)
                                                    */
};

typedef struct _mouse_config_s  mouse_config_t;

struct _mouse_config_s
{
    mmask_t         mouse_mask;                     //  previous mouse mask
    int             mouse_interval;                 //  previous mouse interval
    unsigned int    state;
    int             held;                           /*
                                                        button being held
                                                        down (1 - 3) or 0
                                                    */
    int             last_x;                         //  cell of the last
    int             last_y;                         //  motion report
};

#define MOUSE_STATE_RUNNING     (1 << 0)            //  1

/*
    what a mouse event means to the child, before it is encoded for the
    protocol the child asked for.
*/
typedef struct _mouse_report_s  mouse_report_t;

struct _mouse_report_s
{
    int             kind;                           //  MOUSE_REPORT_*
    int             button;                         //  0 - 2, or 3 for none
    int             mods;                           //  MOUSE_MOD_* bits
    int             x;                              //  1-based column
    int             y;                              //  1-based row
};

enum
{
    MOUSE_REPORT_PRESS = 1,
    MOUSE_REPORT_RELEASE,
    MOUSE_REPORT_MOTION,
    MOUSE_REPORT_WHEEL_UP,
    MOUSE_REPORT_WHEEL_DOWN
};

// modifier and event bits of the xterm button code
#define MOUSE_MOD_SHIFT         (1 << 2)            //  4
#define MOUSE_MOD_ALT           (1 << 3)            //  8 (xterm: meta)
#define MOUSE_MOD_CTRL          (1 << 4)            //  16
#define MOUSE_CODE_MOTION       (1 << 5)            //  32
#define MOUSE_CODE_WHEEL        (1 << 6)            //  64


void    mouse_driver_init(vtty_t *vtty);

void    mouse_driver_start(vtty_t *vtty);

void    mouse_driver_stop(vtty_t *vtty);

void    mouse_driver_free(vtty_t *vtty);

ssize_t mouse_driver_default(vtty_t *vtty, unsigned char *buf);

void    mouse_driver_save_state(vtty_t *vtty);

void    mouse_driver_restore_state(vtty_t *vtty);

void    mouse_driver_set_motion(vtty_t *vtty, uint16_t mode);

int     mouse_driver_decode(vtty_t *vtty, mouse_report_t *report);

ssize_t mouse_driver_vt200(vtty_t *vtty, unsigned char *buf);

ssize_t mouse_driver_SGR(vtty_t *vtty, unsigned char *buf);


#endif
