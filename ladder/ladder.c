#include "ladder.h"
#include <unistd.h>
#include <sys/time.h>



int lads, score, level, scrno;
int speed = 0;
int boni[DIMSCRN];

char *unmerge(char *s)
{
    static char t[128];
    char *d = t;
    int i;
    
    for( ; *s; *s++ ) 
        if( *s & 0200 )
            for( i = 210 - (unsigned char)*s; i; i-- )
                *d++ = ' ';
        else
            *d++ = *s;
    *d = '\0';
    return t;
}

static int getcmd(int row, int col)
{
    static char *funny[2] =
    {
        "You eat quiche!",
        "Come on, we don't have all day!"
    };
    struct timeval tv;
    fd_set fdset;
    
    for( ;; )
    {
        mvaddstr(row,col,"Enter one of the above: ");
        refresh();
        FD_ZERO(&fdset);
        FD_SET(0,&fdset);
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        if( select(1,&fdset,(fd_set *)0,(fd_set *)0,&tv) )
            break;
        mvaddstr(row + 2,col,funny[rand() % DIM(funny)]);
        refresh();
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        select(0,(fd_set *)0,(fd_set *)0,(fd_set *)0,&tv);
        move(row + 2,col);
        clrtoeol();
    }
    return getch();
}

static char menu(void)
{
#define LM  2
#define RM0 33
#define RM1 40

    static char *text[] =
    {
        "LL\275dd\313dd",
        "LL\275dd\313dd\274tm",
        "LL\311aaaa\315ddddd\316ddddd\316eeee\317rrrrrrr",
        "LL\312aa\320aa\317dd\320dd\317dd\320dd\317ee\320ee\320rr\316rr",
        "LL\312aa\320aa\317dd\320dd\317dd\320dd\317eeeeee\320rr",
        "LL\312aa\320aa\317dd\320dd\317dd\320dd\317ee\314rr",
        "LLLLLLLL\317aaa\321aa\317ddd\321dd\317ddd\321dd\317eeee\317rr"
    };
    int r;

    for( r = 0; r < DIM(text); r++ )
        mvaddstr(r + 1,11,unmerge(text[r]));
    r += 3;
    mvaddstr(r,LM,
        "(c) in 1982, 1983: Yahoo Software, cloned by Andreas Burmester.");
    r += 2;
    mvaddstr(r,LM,"Version:    n/a");
    mvaddstr(r,RM0,"Up = k|8  Down = j|2  Left = h|4  Right = l|6");
    r++;
    mvprintw(r,LM,"Terminal:   %s",termname());
    mvaddstr(r,RM0,"Jump = Space   Stop = Other");
    r++;
    mvprintw(r,LM,"Play Speed: %d",speed + 1);
    r++;
    prt_score(r,RM1);
    r++;
    mvaddstr(r++,LM,"P = Play game");
    mvaddstr(r++,LM,"L = Change level of difficulty");
    mvaddstr(r++,LM,"I = Instructions");
    mvaddstr(r++,LM,"E = Exit Ladder");
    r++;
    refresh();
    return getcmd(r,LM);

#undef LM
#undef RM0
#undef RM1
}


static void instructions(void)
{
    static char *text[] =
    {
        "You are a Lad trapped in a maze.  Your mission is to explore the",
        "dark corridors never before seen by human eyes and find hidden",
        "treasures and riches.","",
        "You control Lad by typing the direction buttons and jumping by",
        "typing SPACE.  But beware of the falling rocks called Der rocks.",
        "You must find and grasp the treasure (shown as $) BEFORE the",
        "bonus time runs out.","",
        "A new Lad will be awarded for every 10,000 points.",
        "Extra points are awarded for touching the gold",
        "statues (shown as &).  You will receive the bonus time points",
        "that are left when you have finished the level.",
        "Remember, there is more than one way to skin a cat. (Chum)",
        "Type an ESCape to pause the game.","",
        "Good luck Lad.","","",
        "Type RETURN to return to main menu: "
    };
    int r;

    clear();
    for( r = 0; r < DIM(text); r++ )
        mvaddstr(r + 2,4,text[r]);
    refresh();
    nodelay(stdscr,FALSE);
    getch();
    nodelay(stdscr,TRUE);
}

static void play(void)
{
    int hi_scrno;

    memcpy(boni,st_boni,sizeof(st_boni));
    lads = 5;
    score = 0;
    scrno = 0;
    hi_scrno = 1;
    clear();
    for( level = 0; ; level++ )
    {
        if( lplay() == DEAD )
            break;
        boni[scrno] -= 2;
        if( ++scrno > hi_scrno )
        {
            if( hi_scrno != DIMSCRN - 1)
                hi_scrno++;
            scrno = 0;
        }
    }
    upd_score();
}

int main(void)
{
    if( (int)initscr() == ERR )
    {
        fputs("Curses initialization failed.\n",stderr);
        return EXIT_FAILURE;
    }
    if( LINES < 24 || COLS < 80 )
    {
        addstr("Unsufficient Screen Dimensions.");
        mexit0();
    }
    cbreak();
    noecho();
    nodelay(stdscr,TRUE);
    leaveok(stdscr,TRUE);
    /* keypad(stdscr,TRUE); */
    typeahead(-1);
    signal(SIGINT,mexit1);
    signal(SIGQUIT,mexit1);
    srand(getpid());

    clear();
    for( ;; )
        switch( toupper(menu()) )
        {
            case 'P':
                play();
                clear();
                break;
            case 'I':
                instructions();
                clear();
                break;
            case 'L':
                if( ++speed == HISPEED )
                    speed = 0;
                break;
            case 'R'-'@':
            case 'L'-'@':
            case KEY_CLEAR:
                wrefresh(curscr);
                break;
            case 'E':
                mexit1();
            default:
                flash();
        }
    mexit1();
}

void mexit0()
{
    refresh();
    noraw();
    echo();
    endwin();
    exit(EXIT_SUCCESS);
}

void mexit1()
{
    move(23,0);
    mexit0();
}
