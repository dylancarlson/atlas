/* -------------------------------------------------------------------- */
/*  MISC.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  Citadel garbage dump, if it aint elsewhere, its here.               */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#define MISC

/* MSC */
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* DragCit */
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/*  exitcitadel()   Done with cit, time to leave                        */
/*  filexists()     Does the file exist?                                */
/*  hash()          Make an int out of their name                       */
/*  initCitadel()   Load up data files and open everything.             */
/*  openFile()      Special to open a .cit file                         */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/*  h2memcpy()      Terible hack to copy over 64K, beacuse MSC cant. PT2*/
/*  changedir()     changes curent drive and directory                  */
/*  changedisk()    change to another drive                             */
/*  ltoac()         change a long into a number with ','s in it         */
/*  doBorder()      print a boarder line.                               */
/*  editBorder()    edit a boarder line.                                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Many of the functions move to other modules     */
/*  02/08/89    (PAT)   History Re-Started                              */
/*                      InitAideMess and SaveAideMess added             */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data definitions                                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  filexists()     Does the file exist?                                */
/* -------------------------------------------------------------------- */
BOOL filexists(char *filename)
{
    return (BOOL)((access(filename, 4) == 0) ? TRUE : FALSE);
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  hash()          Make an int out of their name                       */
/* -------------------------------------------------------------------- */
uint hash(char *str)
{
    int  h, shift;

    for (h=shift=0;  *str;  shift=(shift+1)&7, str++)
    {
        h ^= (toupper(*str)) << shift;
    }
    return h;
}
#endif

/* -------------------------------------------------------------------- */
/*  hash()          Make an int out of their name                       */
/* -------------------------------------------------------------------- */
uint hash(char *string)
{
    char *str;
    int  h, shift;

    str = deansi(string);

    for (h=shift=0;  *str;  shift=(shift+1)&7, str++)
    {
        h ^= (toupper(*str)) << shift;
    }
    return h;
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  hmemcpy()       Terible hack to copy over 64K, beacuse MSC cant.    */
/* -------------------------------------------------------------------- */
#define K32  (32840L)
void hmemcpy(void huge *xto, void huge *xfrom, long size)
{
    char huge *from;
    char huge *to;

    to = xto; from = xfrom;

    if (to > from)
    {
        h2memcpy(to, from, size);
        return;
    }

    while (size > K32)
    {
        memcpy((char far *)to, (char far *)from, (unsigned int)K32);
        size -= K32;
        to   += K32;
        from += K32;
    }

    if (size)
        memcpy((char far *)to, (char far *)from, (uint)size);
}

/* -------------------------------------------------------------------- */
/*  h2memcpy()      Terible hack to copy over 64K, beacuse MSC cant. PT2*/
/* -------------------------------------------------------------------- */
void h2memcpy(char huge *to, char huge *from, long size)
{
    to += size;
    from += size;

    size++;

    while(size--)
        *to-- = *from--;
}
#endif

/* -------------------------------------------------------------------- */
/*  changedir()     changes curent drive and directory                  */
/* -------------------------------------------------------------------- */
int changedir(char *path)
{
    /* uppercase   */ 
    path[0] = (char)toupper(path[0]);

    /* change disk */
    changedisk(path[0]);

    /* change path */
    if (chdir(path)  == -1) return -1;

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  changedisk()    change to another drive                             */
/* -------------------------------------------------------------------- */
void changedisk(char disk)
{
    union REGS REG;

    REG.h.ah = 0x0E;      /* select drive */

    REG.h.dl = (uchar)(disk - 'A');

    intdos(&REG, &REG);
}

/* -------------------------------------------------------------------- */
/*  ltoac()         change a long into a number with ','s in it         */
/* -------------------------------------------------------------------- */
char *ltoac(long num)
{
    char s1[30];
    static char s2[40];
    int i, i2, i3, l;

    sprintf(s1, "%lu", num);

    l = strlen(s1);

    for (i = l, i2 = 0, i3 = 0; s1[i2]; i2++, i--)
    {
        if (!(i % 3) && i != l)
        {
            s2[i3++] = ',';
        }
        s2[i3++] = s1[i2];
    }

    s2[i3] = '\0' /*NULL*/;

    return s2;
}

/* -------------------------------------------------------------------- */
/*  editBorder()    edit a boarder line.                                */
/* -------------------------------------------------------------------- */
void editBorder(void)
{
    int i;

    doCR();
    doCR();
        
    if (!cfg.borders)
    {
        mPrintf(" Border lines not enabled!");
        doCR();
        return;
    }

    outFlag = OUTOK;
    
    for (i = 0; i < MAXBORDERS; i++)
    {
        mPrintf("Border %d:", i+1);
        if (*cfg.border[i])
        {
            doCR();
            mPrintf("%s", cfg.border[i]);
        }
        else
        {
            mPrintf(" Empty!"); 
        }
        doCR();
        doCR();
    }

    i = (int)getNumber("border line to change", 0L, (long)MAXBORDERS, 0L);

    if (i)
    {
        doCR();
        getString("border line", cfg.border[i-1], 80, FALSE, ECHO, "");
    }
}

/* -------------------------------------------------------------------- */
/*  doBorder()      print a boarder line.                               */
/* -------------------------------------------------------------------- */
void doBorder(void)
{
    static count = 0;
    static line  = 0;
    int    tries;

    if (count++ == 20)
    {
        count = 0;

        for (line == MAXBORDERS-1 ? line = 0 : line++, tries = 0; 
             tries < MAXBORDERS + 1;
             line == MAXBORDERS-1 ? line = 0 : line++, tries++)
        {
            if (*cfg.border[line])
            {
                doCR();
                mPrintf("%s", cfg.border[line]);
                break;
            }
        }
    }
}

#ifdef GOODBYE

#define ADDITION            (1)
#define CHANGE              (3)
#define DELETION            (5)
#define COMP_LEN            (30)

#define ARR_SIZE            (COMP_LEN+1)
#define SMALLEST_OF(x,y,z)  ( (x<y) ? min(x,z) : min(y,z) )
#define ZERO_IF_EQUAL(x,y)  (tolower(requested[x-1]) == tolower(found[y-1]) ? 0 : CHANGE)

int     l_distance(char *requested, char *found)
{
    register int i,j;
    int r_len, f_len;
    int distance[ARR_SIZE][ARR_SIZE];
    
    /*
     * First character does not match 
     */
    if (tolower(requested[0]) != tolower(found[0]))
        return 1000;
    
    r_len = min(strlen(requested),COMP_LEN);
    f_len = min(strlen(found),    COMP_LEN);

    /*
     * The lengths are too diffrent
     */
    if ( abs(r_len - f_len) > (r_len/3))
        return 1001;
    
    distance[0][0] = 0;
    
    for (j = 1; j <= ARR_SIZE; j++)
        distance[0][j] = distance[0][j-1] + ADDITION;
    for (j = 1; j <= ARR_SIZE; j++)
        distance[j][0] = distance[j-1][0] + DELETION;
        
    for (i=1; i<=r_len; i++)    
        for (j=1; j<=f_len; j++)    
            distance[i][j] = SMALLEST_OF
                             (
                                (distance[i-1][j-1] + ZERO_IF_EQUAL(i,j)),
                                (distance[i  ][j-1] + ADDITION),
                                (distance[i-1][j  ] + DELETION)
                             );
    
    return (distance[r_len][f_len]);
}

#endif
