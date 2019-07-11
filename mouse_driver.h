#ifndef _MOUSE_DRIVER_H_
#define _MOUSE_DRIVER_H_

void    mouse_driver_init(void);

void    mouse_driver_unset(vterm_t *vterm);

ssize_t mouse_driver_default(vterm_t *vterm, unsigned char *buf);

ssize_t mouse_driver_vt200(vterm_t *vterm, unsigned char *buf);

ssize_t mouse_driver_SGR(vterm_t *vterm, unsigned char *buf);


#endif
