#ifndef _VTTY_CTRL_CHAR_H_
#define _VTTY_CTRL_CHAR_H_

#define IS_C0(x) \
            ((unsigned int)x >= 1 && (unsigned int)x <= 31)
#define IS_C1(x) \
            ((unsigned int)x >= 128 && (unsigned int)x <= 159)
#define IS_CTRL_CHAR(x) \
            (IS_C0(x) || IS_C1(x))

void    vtty_interpret_ctrl_char(vtty_t *vtty, char *data);

#endif
