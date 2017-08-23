TARGET	= ladder
SRCS	= ladder.c lplay.c ltime.c lscore.c lscreens.c
OBJS	= ladder.o lplay.o ltime.o lscore.o lscreens.o

CC		= gcc
CFLAGS	= -O -pedantic -I/usr/include/ncurses
LDFLAGS	= -s
M4		= m4
LIBS	= -lncurses
SCOREFILE	= ./ladder.scores

.SUFFIXES:	.o .c .m4

.c.o:
	$(CC) -DSCOREFILE=\"$(SCOREFILE)\" -c $(CFLAGS) $<

.m4.c:
	$(M4) $< > $@

$(TARGET):	$(OBJS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS):	ladder.h

# ladder.c:	ladder.m4

# lscreens.c:	lscreens.m4

# ladder.m4 lscreens.m4:	merge.m4
