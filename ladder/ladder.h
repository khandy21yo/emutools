#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#define DIM(A)  (sizeof(A) / sizeof(*(A)))

#ifndef FALSE
#define FALSE   0
#define TRUE    1
#endif

#define DIMROW  20
#define DIMCOL  80
#define DIMSCRN 7
#define HISPEED 5

#define CLAD    'g'
#define CDER    'o'
#define CGOLD   '&'
#define CRELEAS 'V'
#define CLADDER 'H'
#define CTARGET '$'
#define CEXIT   '*'
#define CBAR    '|'
#define CGROUND '='
#define CHAZARD '.'
#define CTRAP0  '^'
#define CTRAP1  '-'
#define CFREE   ' '

typedef enum { NONE, STOP = 0, XUP = 1, XDOWN = 2, LEFT = 3, RIGHT = 4 } DIR;
typedef enum { NORMAL, NOTHING_HAPPENED = 0, EXIT, PAUSE, DEAD, FINISH } RESULT;

#define AT  printf("%s: at %d\r\n",__FILE__,__LINE__); sleep(2)

/* ladder.c */
void mexit0(void);
void mexit1();
char *unmerge(char *);
extern int lads, score, boni[DIMSCRN], level, scrno, speed;

/* lscreens.c */
extern char *screens[DIMSCRN][DIMROW];
extern int hiders[DIMSCRN];
extern int st_boni[DIMSCRN];

/* lplay.c */
RESULT lplay(void);

/* ltime.c */
void ctplay(void);
void ctnplay(void);
void waitct(void);

/* lscore.c */
void upd_score(void);
void prt_score(int,int);

