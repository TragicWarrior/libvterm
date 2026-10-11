#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtty_private.h"
#include "mouse_driver.h"
#include "stringv.h"
#include "macros.h"

#define EVERY_MOUSE_EVENT   (ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION)

mouse_state_t   *mouse_state = NULL;

void
mouse_driver_init(vtty_t *vtty)
{
    extern mouse_state_t    *mouse_state;

    mmask_t                 mouse_mask = EVERY_MOUSE_EVENT;

    /*
        among all the instances, this is the first one and we need to
        claim it.  the has_mouse() function tells us whether the mouse
        driver was started externally or not.  based on this we set origin
        accordingly -- null means the driver instantiated externally and
        if it's not null it means that the driver was started by a libvtty
        instance.  when it's not null it almost certainly means we're
        in a shared environment (like the VWM window manager).
    */
    if(mouse_state == NULL)
    {
        mouse_state = calloc(1, sizeof(mouse_state_t));

        if(has_mouse() == TRUE || (vtty->flags & VTTY_FLAG_EXTMOUSE))
        {
            /*
                the host owns the real terminal's mouse -- either ncurses
                reports it armed, or the host told us so explicitly via
                VTTY_FLAG_EXTMOUSE.  the explicit flag is required when the
                host enables mouse with raw escape sequences that bypass
                ncurses' mask (e.g. vwm), because has_mouse() then reports
                FALSE and we would otherwise wrongly seize the mask out from
                under it.
            */
            mouse_state->origin = NULL;
        }
        else
        {
            mouse_state->origin = vtty;

            /*
                as the instantiator, we can set this up however we want
                without worrying about any consequence.
            */
            mousemask(mouse_mask, NULL);
            mouseinterval(0);
        }
    }

    mouse_state->ref_count++;

    if(vtty->mouse_config == NULL)
    {
        vtty->mouse_config = calloc(1, sizeof(mouse_config_t));

        // no cell has been reported yet
        vtty->mouse_config->last_x = -1;
        vtty->mouse_config->last_y = -1;
    }

    return;
}

/*
    bring the real terminal's motion reporting in line with what the
    instances need.  ncurses only ever asks the terminal for button
    events (mode 1000), so when a child wants drags or hover the terminal
    has to be asked for them directly.

    nothing is written when the host owns the mouse (VTTY_FLAG_EXTMOUSE):
    the terminal is the host's to configure, and it hands us the motion
    events it wants the child to see.  nor when curses is not in use.
*/
static void
mouse_driver_sync_host(vtty_t *vtty)
{
    extern mouse_state_t    *mouse_state;
    int                     want = 0;

    if(vtty->flags & (VTTY_FLAG_EXTMOUSE | VTTY_FLAG_NOCURSES)) return;

    // any-motion is a superset of button-motion
    if(mouse_state->any_event_count > 0) want = 1003;
    else if(mouse_state->btn_event_count > 0) want = 1002;

    if(want == mouse_state->host_motion) return;

    if(want != 0)
    {
        // setting a tracking mode replaces whichever one was in effect
        printf("\033[?%dh", want);
    }
    else
    {
        /*
            the tracking modes are alternatives on the terminal, so
            resetting this one turns mouse reporting off altogether.  put
            back the button reporting ncurses believes is still enabled,
            or every other instance goes deaf.
        */
        printf("\033[?%dl", mouse_state->host_motion);
        if(has_mouse() == TRUE) printf("\033[?1000h");
    }

    fflush(stdout);

    mouse_state->host_motion = want;

    return;
}

/*
    select which motion the child is told about:  VTTY_MOUSE_BTN_EVENT
    (motion while a button is held, DECSET 1002), VTTY_MOUSE_ANY_EVENT
    (all motion, DECSET 1003) or 0 for none.  as in xterm the two are
    alternatives, so setting one replaces the other.
*/
void
mouse_driver_set_motion(vtty_t *vtty, uint16_t mode)
{
    extern mouse_state_t    *mouse_state;
    uint16_t                mask;
    uint16_t                old;

    if(mouse_state == NULL) return;

    mask = VTTY_MOUSE_BTN_EVENT | VTTY_MOUSE_ANY_EVENT;
    old = vtty->mouse_mode & mask;
    mode &= mask;

    if(old == mode) return;

    if(old & VTTY_MOUSE_BTN_EVENT) mouse_state->btn_event_count--;
    if(old & VTTY_MOUSE_ANY_EVENT) mouse_state->any_event_count--;
    if(mode & VTTY_MOUSE_BTN_EVENT) mouse_state->btn_event_count++;
    if(mode & VTTY_MOUSE_ANY_EVENT) mouse_state->any_event_count++;

    vtty->mouse_mode = (vtty->mouse_mode & ~mask) | mode;

    mouse_driver_sync_host(vtty);

    return;
}

void
mouse_driver_start(vtty_t *vtty)
{
    if(vtty->mouse_config == NULL) return;

    vtty->mouse_config->state |= MOUSE_STATE_RUNNING;

    return;
}

void
mouse_driver_stop(vtty_t *vtty)
{
    if(vtty->mouse_config == NULL) return;

    // stop asking the real terminal for motion on this instance's behalf
    mouse_driver_set_motion(vtty, 0);

    // reset the mouse driver setup
    vtty->mouse_mode = 0;
    vtty->mouse_driver = NULL;
    vtty->mouse_config->held = 0;

    // indicate that we're stopped
    vtty->mouse_config->state &= ~MOUSE_STATE_RUNNING;

    return;
}

ssize_t
mouse_driver_default(vtty_t *vtty, unsigned char *buf)
{
    MEVENT      discard_event;
    ssize_t     mbytes = 0;

    if((vtty->mouse_config->state & MOUSE_STATE_RUNNING) == 0)
    {
        getmouse(&discard_event);
        return 0;
    }

    // save the previous mouse state so we can restore it later
    mouse_driver_save_state(vtty);

    // SGR is a special variant of VT200
    if(vtty->mouse_mode & VTTY_MOUSE_SGR)
    {
        mbytes = mouse_driver_SGR(vtty, buf);
        mouse_driver_restore_state(vtty);

        return mbytes;
    }

    if(vtty->mouse_mode & VTTY_MOUSE_VT200)
    {
        mbytes = mouse_driver_vt200(vtty, buf);
        mouse_driver_restore_state(vtty);

        return mbytes;
    }


    return 0;
}

/*
    read the next mouse event and work out what it means to the child:
    which button, which modifiers, press / release / motion / wheel, and
    where.  returns 1 with *report filled in, or 0 when the event is not
    one the child should hear about.

    the event comes from the host when it injected one
    (vtty_write_mouse_event) and from ncurses otherwise.
*/
int
mouse_driver_decode(vtty_t *vtty, mouse_report_t *report)
{
    static const mmask_t    pressed[3] =
                                {
                                    BUTTON1_PRESSED,
                                    BUTTON2_PRESSED,
                                    BUTTON3_PRESSED
                                };
    static const mmask_t    released[3] =
                                {
                                    BUTTON1_RELEASED,
                                    BUTTON2_RELEASED,
                                    BUTTON3_RELEASED
                                };
    mouse_config_t          *config = vtty->mouse_config;
    MEVENT                  mouse_event;
    mmask_t                 bstate;
    int                     x, y;
    int                     n;

    if(vtty->pending_mouse != NULL)
        memcpy(&mouse_event, vtty->pending_mouse, sizeof(MEVENT));
    else
        getmouse(&mouse_event);

    bstate = mouse_event.bstate;
    x = mouse_event.x;
    y = mouse_event.y;

    if(vtty->window != NULL)
    {
        /*
            get mouse returns screen relative coords and we need to transform
            those into window relative coords.  wmouse_trafo() does exactly
            that and returns false if the conversion is not possible--for
            example if the coord are out of bounds.
        */
        if(wmouse_trafo(vtty->window, &y, &x, FALSE) == FALSE)
        {
            return 0;
        }

        // ncurses top left is is 0,0 whereas for the protocols it is 1,1
        x++;
        y++;
    }

    memset(report, 0, sizeof(mouse_report_t));
    report->x = x;
    report->y = y;

    if(bstate & BUTTON_SHIFT) report->mods |= MOUSE_MOD_SHIFT;
    if(bstate & BUTTON_ALT) report->mods |= MOUSE_MOD_ALT;
    if(bstate & BUTTON_CTRL) report->mods |= MOUSE_MOD_CTRL;

    /*
        motion.  this is tested first because ncurses reports a drag as
        "button pressed" plus the position flag, and that must not be
        mistaken for a new press.
    */
    if(bstate & REPORT_MOUSE_POSITION)
    {
        /*
            ncurses names the held button in the event.  a host that
            builds its own events (vwm) sends the position flag alone,
            so otherwise go by the button last seen pressed.
        */
        for(n = 0; n < 3; n++)
        {
            if(bstate & pressed[n]) config->held = n + 1;
        }

        // the child did not ask for motion
        if(!(vtty->mouse_mode
            & (VTTY_MOUSE_BTN_EVENT | VTTY_MOUSE_ANY_EVENT))) return 0;

        // mode 1002 reports motion only while a button is down
        if(config->held == 0 && !(vtty->mouse_mode & VTTY_MOUSE_ANY_EVENT))
            return 0;

        // like xterm, report only when the pointer changes cells
        if(x == config->last_x && y == config->last_y) return 0;

        config->last_x = x;
        config->last_y = y;

        report->kind = MOUSE_REPORT_MOTION;
        report->button = (config->held > 0) ? config->held - 1 : 3;

        return 1;
    }

    for(n = 0; n < 3; n++)
    {
        if(bstate & pressed[n])
        {
            config->held = n + 1;
            config->last_x = x;
            config->last_y = y;

            report->kind = MOUSE_REPORT_PRESS;
            report->button = n;

            return 1;
        }

        if(bstate & released[n])
        {
            if(config->held == n + 1) config->held = 0;

            report->kind = MOUSE_REPORT_RELEASE;
            report->button = n;

            return 1;
        }
    }

// only the newer ABI supports the wheel mouse properly
#if NCURSES_MOUSE_VERSION > 1

    if(bstate & BUTTON4_PRESSED)
    {
        report->kind = MOUSE_REPORT_WHEEL_UP;
        return 1;
    }

    if(bstate & BUTTON5_PRESSED)
    {
        report->kind = MOUSE_REPORT_WHEEL_DOWN;
        return 1;
    }

#endif

    return 0;
}

void
mouse_driver_save_state(vtty_t *vtty)
{
    extern mouse_state_t    *mouse_state;

    mmask_t     mouse_mask = EVERY_MOUSE_EVENT;

    /*
        when the host owns the mouse (VTTY_FLAG_EXTMOUSE) we must never
        touch ncurses' mousemask -- not even the transient save/restore
        below, which would momentarily re-arm it and (on each event) emit
        ncurses' own mouse enable/disable escapes, fighting the host's setup.
    */
    if(vtty->flags & VTTY_FLAG_EXTMOUSE) return;

    /*
        if the mouse origin is anyone but this instance, then we need to
        save the previous mask and interval so we can restore it later.
    */
    if(mouse_state->origin != vtty)
    {
        mousemask(mouse_mask, &vtty->mouse_config->mouse_mask);
        vtty->mouse_config->mouse_interval = mouseinterval(0);
    }

    return;
}

void
mouse_driver_restore_state(vtty_t *vtty)
{
    extern mouse_state_t    *mouse_state;

    /* host owns the mouse -- save_state was a no-op, so is this */
    if(vtty->flags & VTTY_FLAG_EXTMOUSE) return;

    /*
        if the mouse origin is anyone but this instance, then we need to
        restore the saved mask and interval.
    */
    if(mouse_state->origin != vtty)
    {
        mousemask(vtty->mouse_config->mouse_mask, NULL);
        mouseinterval(vtty->mouse_config->mouse_interval);
    }

    return;
}

void
mouse_driver_free(vtty_t *vtty)
{
    extern mouse_state_t    *mouse_state;

    // release the mouse config
    if(vtty->mouse_config != NULL)
    {
        // a child that died with motion tracking on never turned it off
        mouse_driver_set_motion(vtty, 0);

        free(vtty->mouse_config);
        vtty->mouse_config = NULL;
    }

    if(mouse_state != NULL)
    {
        // tear down the reference count
        mouse_state->ref_count--;

        if(mouse_state->ref_count == 0)
        {
            if(mouse_state->origin != NULL)
            {
                mousemask(0, NULL);
            }
        }
    }

    return;
}
