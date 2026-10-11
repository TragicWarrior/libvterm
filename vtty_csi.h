
#ifndef _VTTY_CSI_H_
#define _VTTY_CSI_H_

#include "vtty.h"

#define MAX_CSI_ES_PARAMS   32
#define MAX_XTERM_ES_PARAMS 32

/*
    this string is emitted by for resetting an RXVT terminal (RS1).
    xterm is far more sane using a single \Ec control code to perform
    the same termcap operation.  the sequence is codified in terminfo
    database.  this can be inspected with the infocmp tool.
*/
#define RXVT_RS1    "\033>\033[1;3;4;5;6l\033[?7h\033[m\033[r\033[2J\033[H"
#define XTERM_RS1   "\033c"

int     vtty_interpret_csi(vtty_t *vtty);

void    interpret_esc_IND(vtty_t *vtty);
void    interpret_esc_NEL(vtty_t *vtty);
void    interpret_esc_RI(vtty_t *vtty);
void    interpret_csi_IRM(vtty_t *vtty, bool replace_mode);

void    interpret_dec_DECALN(vtty_t *vtty);
void    interpret_dec_SM(vtty_t *vtty, int param[], int pcount);
void    interpret_dec_RM(vtty_t *vtty, int param[], int pcount);

void    interpret_csi_SGR(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_ED(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_CUP(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_ED(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_EL(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_ICH(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_DCH(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_IL(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_DL(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_ECH(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_REP(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_CUx(vtty_t *vtty, char verb, int param[], int pcount);
void    interpret_csi_DECSTBM(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_SAVECUR(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_RESTORECUR(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_CBT(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_SU(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_SD(vtty_t *vtty, int param[], int pcount);
void    interpret_csi_DSR(vtty_t *vtty, int param[], int pcount);

int     interpret_csi_RS1_rxvt(vtty_t *vtty, char *data);
int     interpret_csi_RS1_xterm(vtty_t *vtty, char *data);

// trap for unknown escape sequences
void    interpret_csi_UNKNOWN(vtty_t *vtty, int param[], int pcount);

#endif

