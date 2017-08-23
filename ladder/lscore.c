#include "ladder.h"
#include <unistd.h>
#include <pwd.h>

#define MAXSCORE    5
#define XMASK       12345

static char sf[] = SCOREFILE;
static char lf[] = SCOREFILE ".LCK";
typedef struct
{
    int score, level;
    uid_t uid;      /* to avoid user names */
    int xor;
} SCORE;
SCORE scores[MAXSCORE + 1];

static void lock_score(void)
{
    int i;
    FILE *lfp;

    for( i = 3; i; i-- )
        if( lfp = fopen(lf,"r") )
        {
            fclose(lfp);
            sleep(2);
        }
        else
            break;
    if( !i || !(lfp = fopen(lf,"w")) )
    {
        perror(lf);
        exit(EXIT_FAILURE);     
    }
    fclose(lfp);
}

/* Read the score file. Note, that if it can be read, it is sorted. If it
 * can't be read, is malformed or if it's too short, the scores themselves
 * become 0. Any possible error doesn't matter at all.
 */
static void read_scores(void)
{
    FILE *sfp;
    SCORE *scp;

    lock_score();
    memset(scores,0,sizeof(scores));
    if( sfp = fopen(sf,"r") )
    {
        for( scp = scores; scp < &scores[MAXSCORE]; scp++ )
            if( fscanf(sfp,"%d%d%d%d",
                &scp->score,&scp->level,&scp->uid,&scp->xor) != 4
                || scp->score ^ scp->level ^ scp->uid ^ scp->xor ^ XMASK )
            {
                scp->score = 0;
                break;
            }
        fclose(sfp);
    }
    remove(lf);
}

static void percolate(void)
{
    SCORE *scp;
    for( scp = &scores[MAXSCORE]; scp > scores; scp-- )
        if( scp[0].score >  scp[-1].score ||
            scp[0].score == scp[-1].score &&
            scp[0].level >= scp[-1].level )
        {
            SCORE tmp = scp[0];
            scp[0] = scp[-1];
            scp[-1] = tmp;
        }
}

void upd_score()
{
    FILE *sfp;
    SCORE *scp;

    read_scores();
    lock_score();
    if( !(sfp = fopen(sf,"w")) )
    {
        perror(sf);
        mexit1();
    }
    scores[MAXSCORE].score = score;
    scores[MAXSCORE].level = level;
    scores[MAXSCORE].xor = XMASK ^ score ^ level ^
        (scores[MAXSCORE].uid = getuid());
    percolate();
    for( scp = scores; scp < &scores[MAXSCORE]; scp++ )
        fprintf(sfp,"%d %d %d %d\n",
            scp->score,scp->level,scp->uid,scp->xor);
    fclose(sfp);
    remove(lf);
}

void prt_score(int r, int c)
{
    int i;
    struct passwd *pw;

    read_scores();
    mvaddstr(r,c,"High Scores");
    for( i = 0; i < MAXSCORE; i++ )
    {
        mvprintw(r + i + 2,c,"%d)  ",i + 1);
        if( scores[i].score )
        {
            printw("%5d00  %2d  ",scores[i].score,scores[i].level + 1);
            if( pw = getpwuid(scores[i].uid) )
                addstr(pw->pw_name);
            else
                printw("UID %d",scores[i].uid);
        }
        else
            clrtoeol();
    }
}
