#include "vterm.h"
#include "vterm_private.h"

void
vterm_set_userptr(vterm_t *vterm, void *anything)
{
    if(vterm == NULL) return;

    vterm->userptr = anything;

    return;
}

void*
vterm_get_userptr(vterm_t *vterm)
{
    if(vterm == NULL) return NULL;

    return vterm->userptr;
}
