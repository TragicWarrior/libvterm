#ifndef _VTERM_CTRL_CHAR_H_
#define _VTERM_CTRL_CHAR_H_

#define IS_C0(x) \
            ((unsigned int)x >= 1 && (unsigned int)x <= 31)
#define IS_C1(x) \
            ((unsigned int)x >= 128 && (unsigned int)x <= 159)
#define IS_CTRL_CHAR(x) \
            (IS_C0(x) || IS_C1(x))

void    vterm_interpret_ctrl_char(vterm_t *vterm, char *data);

#endif
