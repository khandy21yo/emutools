#include "ladder.h"

static struct
{
    int row, col;           /* current coordinates */
    int st_row, st_col;     /* start coordinates */
    DIR dir;
    int jst;
} lad;

static char laddirs[] = "gbdqp";

typedef struct
{
    int row, col;
    DIR dir;
    int launch;
} DER;
static DER *ders = 0;

typedef struct { int row, col; } RELEASE;
static RELEASE releases[3];
static int bonus;
static char bg[DIMROW][DIMCOL];

/* these extra "00" in score & bonus are extremly silly - 
   but that is how the original did it */
static void stat_lads()     { mvprintw(DIMROW,0,"Lads  %3d",lads); }
static void stat_level()    { mvprintw(DIMROW,14,"Level  %3d",level + 1); }
static void stat_score()    { mvprintw(DIMROW,29,"Score  %4d00",score); }
static void stat_bonus()    { mvprintw(DIMROW,59,"Bonus time  %4d00",bonus); }

static void add_score(int add)
{
    if( score / 100 < (score + add) / 100 )
        lads++, stat_lads();
    score += add;
    stat_score();
}

static void ldscreen(void)
{
    int row,i;
    RELEASE *rel = releases;
    char *s,*t;

    memset(bg,' ',sizeof(bg));
    for( row = 0; row < DIMROW; row++ ) 
    {
        s = screens[scrno][row];
        t = bg[row];
        memcpy(t,unmerge(s),strlen(unmerge(s)));
        t[DIMCOL-1] = '\0';
        mvaddstr(row,0,t);

        /* find points of release */
        for( s = t; s = strchr(s,CRELEAS); s++ )
        {
            rel->row = row;
            rel->col = s - t;
            rel++;
        }
    }
    /* mark the rest of releases */
    for( ; rel < &releases[DIM(releases)]; rel++ )
        rel->row = rel->col = EOF;

    /* find lad */
    for( row = 0; row < DIMROW; row++ ) 
        for( s = t = bg[row]; s = strchr(s,CLAD); s++ )
        {
            /* nasty, check for CLAD's surrounded by CFREEs */
            if( s[-1] != CFREE || s[1] != CFREE )
                continue;
            lad.row = lad.st_row = row;
            lad.col = lad.st_col = s - t;
            lad.dir = NONE;
            lad.jst = 0;
            bg[lad.row][lad.col] = CFREE;
            break;
        }

    /* init ders */
    if( !ders )
    {
        int hi = -1;
        for( i = 0; i < DIMSCRN; i++ )
            if( hi < hiders[i] )
                hi = hiders[i];
        ders = malloc(sizeof(DER) * (hi + 1));
        ders[hi].row = EOF;
    }
    for( i = 0; i < hiders[scrno]; i++ )
    {
        ders[i].launch = i + 1;
        ders[i].dir = XDOWN;
    }
    for( ; ders[i].row != EOF; i++ )
        ders[i].launch = -1;

    move(LINES - 1, 0);
    refresh();
}

static void reldscreen(void)
{
    int row,i;

    for( row = 0; row < DIMROW; row++ ) 
        mvaddstr(row,0,bg[row]);

    /* deal with lad */
    lad.row = lad.st_row;
    lad.col = lad.st_col;
    lad.dir = NONE;
    lad.jst = 0;
    mvaddch(lad.row,lad.col,CLAD);

    /* deal with ders */
    for( i = 0; i < hiders[scrno]; i++ )
    {
        ders[i].launch = i + 1;
        ders[i].dir = XDOWN;
    }

    move(LINES - 1, 0);
    refresh();
}

#define SOLID(C)    ((C) == CBAR || (C) == CGROUND || (C) == CTRAP1)

/* drive a single der, tell whether it left the board or hit lad */
static RESULT drv_der(DER *dp)
{
#define LorR    dchoice[rand() % 2]
#define LorRorD dchoice[rand() % 3] 

    static DIR dchoice[] = {LEFT,RIGHT,XDOWN};
    int row = dp->row,
        col = dp->col;
    DIR dir = dp->dir;
    char c;

    c = bg[row][col];       /* restore prev content */
    mvaddch(row,col,c);
    if( c == CEXIT )
        return EXIT;
    for( ;; )
    {
        if( dir == XDOWN )
        {
            c = bg[row + 1][col];
            if( SOLID(c) )
            {
                dir = LorR;
                continue;
            }
            row++;
            break;
        }
        if( dir == LEFT )
        {
            if( col == 0 || bg[row][col - 1] == CBAR )
            {
                dir = RIGHT;
                continue;
            }
            col--;
        }
        if( dir == RIGHT )
        {
            if( col == DIMCOL - 2 || bg[row][col + 1] == CBAR )
            {
                dir = LEFT;
                continue;
            }
            col++;
        }
        if( bg[row][col] == CLADDER )
            dir = LorRorD;
        else
        {
            c = bg[row + 1][col];
            if( !SOLID(c) )
                dir = XDOWN;
        }
        break;
    }
    c = mvinch(row,col);
    addch(CDER);
    dp->row = row;
    dp->col = col;
    dp->dir = dir;
    return strchr(laddirs,c) ? DEAD : NORMAL;

#undef  LorR
#undef  LorRorD
}

static RESULT drv_ders(void)
{
    DER *derp;
    for( derp = ders; derp->row != EOF; derp++ )
    {
        if( derp->launch == -1 )
            continue;
        if( derp->launch == 0 )
        {
            RESULT result = drv_der(derp);
            if( result == DEAD )
                return DEAD;
            if( result == EXIT )
                derp->launch = 5;       /* set new start time */
            continue;
        }
        if( --derp->launch == 0 )
            /* select a point of release */
            for( ;; )
            {
                int n = rand() % DIM(releases);
                if( releases[n].row != EOF )
                {
                    derp->row = releases[n].row;
                    derp->col = releases[n].col;
                    derp->dir = XDOWN;
                    break;
                }
            }
    }
    return NOTHING_HAPPENED;
}

static void lad_died(void)
{
    int i,j;
    static char rot[] = "b+d+q+p+";
    ctnplay();
    for( i = 0; i < 5; i++ )
        for( j = 0; j < DIM(rot) - 1; j++ )
        {
            mvaddch(lad.row,lad.col,rot[j]);
            move(LINES - 1,0);
            refresh();
            waitct();
        }
}

static void do_the_hooka(void)
{
    for( bonus-- ; bonus >= 0; bonus-- )
    {
        add_score(1);
        stat_bonus();
        move(DIMROW + 2,0);
        if( bonus & 1 )
            addstr("Hooka!");
        else
            clrtoeol();
        move(LINES - 1,0);
        refresh();
        waitct();
    }
}

static void pause(void)
{
    mvaddstr(DIMROW + 2,0,"Type RETURN to continue: ");
    refresh();
    nodelay(stdscr,FALSE);
    while( getch() != '\n' )
        ;
    nodelay(stdscr,TRUE);
    move(DIMROW + 2,0);
    clrtoeol();
    
}

static void over_der(int row,int col)
{
    /* Funny how lad jumps over "Sc`o're" - avoid it? Na. */
    if( mvinch(row + 1,col) == CDER || mvinch(row + 2,col) == CDER )
        add_score(2);
}

static RESULT drv_lad(void)
{
    int row = lad.row;
    int col = lad.col;
    DIR dir = lad.dir;
    int jst = lad.jst;
    char c0,c1;
    int ch;

    while((ch = getch()) != ERR )
    {
     switch(ch)
     {
        case ERR:   /* no key */
            break;

        case 'h':
        case '4':
        case KEY_LEFT:
            dir = LEFT;
            break;

        case 'l':
        case '6':
        case KEY_RIGHT:
            dir = RIGHT;
            break;

        case 'k':
        case '8':
        case KEY_UP:
            if( !jst )
                dir = XUP;
            break;

        case 'j':
        case '2':
        case KEY_DOWN:
            if( !jst )
                dir = XDOWN;
            break;

        case ' ':
            if( !jst )      /* not while we're jumping */
                jst = 1;
            break;

        case 'R'-'@':
        case 'L'-'@':
        case KEY_CLEAR:
            wrefresh(curscr);
            break;

        case '['-'@':
            return PAUSE;

        case 'C'-'@':       /* who does set INTR to ^C, anyway? */
            for( ; lads >= 1; lads-- )
            {
                stat_lads();
                move(LINES - 1, 0);
                refresh();
                waitct();
            }
            lads = 1;
            return DEAD;

        default:
            dir = STOP;
     }
    }

    c0 = bg[row][col];
    c1 = bg[row + 1][col];
    if( jst < 2 && !SOLID(c1) && c0 != CLADDER && !(jst == 1 && c0 == CHAZARD) )
    {
        /* then fall */
        jst = 0;        /* no request for jumping */
        row++;
    }
    else
        if( jst >= 1 )   /* request for or within a jump */
        {
            if( jst == 1 && c1 == CFREE && c0 != CHAZARD )
                jst = 0;
            else
            {
                static int jra[7] = { 0, -1, -1, 0, 0, 1, 1 };
                int jc,jr;

                over_der(row,col);
                if( dir == XUP || dir == XDOWN )
                    dir = STOP;
                for( ; jst != 7; jst++ )
                {
                    jr = jra[jst];
                    jc = dir == STOP ? 0 : (dir == LEFT ? -1 : 1);
                    c0 = bg[row + jr][col + jc];
                    if( c0 != CBAR && c0 != CGROUND && !(jr == 1 && c0 == CTRAP1) )
                    {
                        if( (row += jr) < 0 || row > DIMROW - 2 )
                            row -= jr;
                        if( (col += jc) < 0 || col > DIMCOL - 2 )
                            col -= jc;
                        break;
                    }
                }
                if( ++jst >= 7 )
                    jst = 0;
                if( bg[row][col] == CLADDER )
                {
                    jst = 0;
                    dir = STOP;
                }
                if( dir != STOP )
                    over_der(row,col);
            }
        }
        else
        {
            if( c1  == CTRAP1 )
                mvaddch(row + 1,col,bg[row + 1][col] = CFREE);
            switch( dir )
            {
                case LEFT:
                    c1 = bg[row][col - 1];
                    if( col != 0 && c1 != CBAR && c1 != CGROUND )
                        col--;
                    else
                        dir = STOP;
                    break;
                case RIGHT:
                    c1 = bg[row][col + 1];
                    if( col != DIMCOL - 2 && c1 != CBAR && c1 != CGROUND )
                        col++;
                    else
                        dir = STOP;
                    break;
                case XUP:
                    if( c0 == CLADDER &&
                        ((c0 = bg[row - 1][col]) == CLADDER || c0 == CTARGET) )
                        row--;
                    else
                        dir = STOP;
                    break;
                case XDOWN:
                    if( c0 == CLADDER && c1 != CGROUND )
                        row++;
                    else
                        dir = STOP;
                    break;
            }
        }

    if( lad.row != row || lad.col != col || lad.dir != dir || lad.jst != jst )
    {
        mvaddch(lad.row,lad.col,bg[lad.row][lad.col]);
        {
            /* remove rubbish */
            static char s[] = {CGOLD,CRELEAS,CLADDER,CTARGET,
                CEXIT,CBAR,CGROUND,CHAZARD,CTRAP0,CTRAP1,CFREE};
            if( !strchr(s,bg[row][col]) )
                mvaddch(row,col,bg[row][col] = CFREE);
        }
        /* check for anything that matters */
        if( bg[row][col] == CGOLD )
        {
            mvaddch(row,col,bg[row][col] = CFREE);
            add_score(bonus);
        }
        if( bg[row][col] == CHAZARD )
        {
            dir = rand() & 1 ? LEFT : RIGHT;
            jst = rand() & 1;
        }
        lad.row = row;
        lad.col = col;
        lad.dir = dir;
        lad.jst = jst;
        if( mvinch(row,col) == CDER )
            return DEAD;
        addch(laddirs[dir]);
    }
    switch( bg[row][col] )
    {
        case CTARGET:
            return FINISH;
        case CTRAP0:
            return DEAD;
    }
    return NORMAL;
}

RESULT lplay(void)
{
    int tick;
    RESULT result;

    ldscreen();

    while( lads > 0 )
    {
        bonus = boni[scrno];
        ctplay();
        stat_lads();
        stat_level();
        stat_score();
        stat_bonus();
        mvaddstr(DIMROW + 2,0,"Get ready! ");
        refresh();
        for( tick = 7; tick; tick-- )
            waitct();
        move(DIMROW + 2,0);
        clrtoeol();
    
        for( tick = 20 * bonus; tick; tick-- )
        {
            if( !((tick - 1) % 20) )
            {
                bonus--;
                stat_bonus();
            }
            if( (result = drv_ders()) != DEAD )
                result = drv_lad();
            move(LINES - 1,0);
            refresh();
            waitct();
            if( result == PAUSE )
                pause(),
                result = NORMAL;
            if( result != NORMAL )
                break;
        }
        if( !tick )
            result = DEAD;
        if( result == DEAD )
        {
            lads--;
            stat_lads();
            lad_died();
        }
        if( result == FINISH )
        {
            do_the_hooka();
            return NORMAL;
        }
        reldscreen();
    }
    return DEAD;
}
