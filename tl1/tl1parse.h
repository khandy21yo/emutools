
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

enum {
	TL1_ACK_IP,
	TL1_ACK_PF,
	TL1_ACK_OK,
	TL1_ACK_NA,
	TL1_ACK_NG,
	TL1_ACK_RL,
	TL1_IICM,
	TL1_DENY,
	TL1_PLNA,
	TL1_MISP
};

/* Globals */

extern int auth;
extern char hostname[];

/* tl1command.c */

int tl1_command(char *buffer);
int tl1_execute(int rows, char *fields[]);
int tl1_execute_default(int cmd, int rows, char *fields[]);
int tl1_execute_actuser(int cmd, int rows, char *fields[]);
int tl1_execute_break(int cmd, int rows, char *fields[]);
int tl1_execute_dump(int rows, char *fields[]);

/* tl1error.c */

void tl1_ack(int code, FILE *os, int rows, char *fields[]);
void tl1_error(int errno, FILE *os, int rows, char *fields[]);

/* tl1parser.c */

int tl1_parse_line(char *buffer);

/* tl1readline.c */

int tl1_rl_readline(FILE *stream);

