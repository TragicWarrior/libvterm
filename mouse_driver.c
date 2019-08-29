#include <stdlib.h>
#include <string.h>

#include "vterm_private.h"
#include "mouse_driver.h"
#include "stringv.h"
#include "macros.h"

#define EVERY_MOUSE_EVENT   (ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION)

mouse_state_t   *mouse_state = NULL;

void
mouse_driver_init(vterm_t *vterm)
{
    extern mouse_state_t    *mouse_state;

    mmask_t                 mouse_mask = EVERY_MOUSE_EVENT;

    /*
        among all the instances, this is the first one and we need to
        claim it.  the has_mouse() function tells us whether the mouse
        driver was started externally or not.  based on this we set origin
        accordingly -- null means the driver instantiated externally and
        if it's not null it means that the driver was started by a libvterm
        instance.  when it's not null it almost certainly means we're
        in a shared environment (like the VWM window manager).
    */
    if(mouse_state == NULL)
    {
        mouse_state = calloc(1, sizeof(mouse_state_t));

        if(has_mouse() == TRUE)
        {
            mouse_state->origin = NULL;
        }
        else
        {
            mouse_state->origin = vterm;

            /*
                as the instantiator, we can set this up however we want
                without worrying about any consequence.
            */
            mousemask(mouse_mask, NULL);
            mouseinterval(0);
        }
    }

    mouse_state->ref_count++;

    if(vterm->mouse_config == NULL)
    {
        vterm->mouse_config = calloc(1, sizeof(mouse_config_t));
    }

    return;
}

void
mouse_driver_start(vterm_t *vterm)
{
    if(vterm->mouse_config == NULL) return;

    vterm->mouse_config->state |= MOUSE_STATE_RUNNING;

    return;
}

void
mouse_driver_stop(vterm_t *vterm)
{
    if(vterm->mouse_config == NULL) return;

    // reset the mouse driver setup
    vterm->mouse_mode = 0;
    vterm->mouse_driver = NULL;

    // indicate that we're stopped
    vterm->mouse_config->state = ~MOUSE_STATE_RUNNING;

    return;
}

ssize_t
mouse_driver_default(vterm_t *vterm, unsigned char *buf)
{
    MEVENT      discard_event;
    ssize_t     mbytes = 0;

    if((vterm->mouse_config->state & MOUSE_STATE_RUNNING) == 0)
    {
        getmouse(&discard_event);
        return 0;
    }

    // save the previous mouse state so we can restore it later
    mouse_driver_save_state(vterm);

    // SGR is a special variant of VT200
    if(vterm->mouse_mode & VTERM_MOUSE_SGR)
    {
        mbytes = mouse_driver_SGR(vterm, buf);
        mouse_driver_restore_state(vterm);

        return mbytes;
    }

    if(vterm->mouse_mode & VTERM_MOUSE_VT200)
    {
        mbytes = mouse_driver_vt200(vterm, buf);
        mouse_driver_restore_state(vterm);

        return mbytes;
    }


    return 0;
}

void
mouse_driver_save_state(vterm_t *vterm)
{
    extern mouse_state_t    *mouse_state;

    mmask_t     mouse_mask = EVERY_MOUSE_EVENT;

    /*
        if the mouse origin is anyone but this instance, then we need to
        save the previous mask and interval so we can restore it later.
    */
    if(mouse_state->origin != vterm)
    {
        mousemask(mouse_mask, &vterm->mouse_config->mouse_mask);
        vterm->mouse_config->mouse_interval = mouseinterval(0);
    }

    return;
}

void
mouse_driver_restore_state(vterm_t *vterm)
{
    extern mouse_state_t    *mouse_state;

    /*
        if the mouse origin is anyone but this instance, then we need to
        restore the saved mask and interval.
    */
    if(mouse_state->origin != vterm)
    {
        mousemask(vterm->mouse_config->mouse_mask, NULL);
        mouseinterval(vterm->mouse_config->mouse_interval);
    }

    return;
}

void
mouse_driver_free(vterm_t *vterm)
{
    extern mouse_state_t    *mouse_state;

    // release the mouse config
    if(vterm->mouse_config != NULL)
    {
        free(vterm->mouse_config);
        vterm->mouse_config = NULL;
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
