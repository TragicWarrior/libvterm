#ifndef _VTERM_CTRL_CHAR_H_
#define _VTERM_CTRL_CHAR_H_

#define IS_CTRL_CHAR(x) \
            ((unsigned int)x >= 1 && (unsigned int)x <= 31)

void    vterm_interpret_ctrl_char(vterm_t *vterm, char c);

#endif
