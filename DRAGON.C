/* ------------------------------------------------------------------------- */
/*  Code to implement a user-abuse feature :-)            DRAGON.C           */
/* ------------------------------------------------------------------------- */

#include "ctdl.h"
#include "proto.h"
#include "global.h"

#ifdef DRAGON
/* ------------------------------------------------------------------------- */
/*  Global stuff / defines                                                   */
/* ------------------------------------------------------------------------- */
static int     dAtt = 0;           /* Dragon's attitude                      */
void    dAttAdj(int);
void    dragonAct(void);

#define rnd(d)  (rand() % (d))

static char *c4[] = {
    "You have done well, strive to do better\"",
    "Hmph, your OK\"",
    "Bleeeeeehh!\"",
    "Did you bring lunch?\"",
    "Runtime error R6000: out of stack space!\" then dissapers in a"
        " puff of logic.",
    ""
};

/* ------------------------------------------------------------------------- */
/*  External stuff                                                           */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/*  Ajdust his attitude                                                      */
/* ------------------------------------------------------------------------- */
void    dAttAdj(int adj)
{
    if (adj > 0)
    {
        dAtt = dAtt > (dAtt + adj) ?  dAtt : (dAtt + adj);
    }else{
        dAtt = dAtt < (dAtt + adj) ?  dAtt : (dAtt + adj);
    }
}

/* ------------------------------------------------------------------------- */
/*  The Fun Starts Here                                                      */
/* ------------------------------------------------------------------------- */
void    dragonAct(void)
{
    int att = 0;
    char tmpOut;
    
    tmpOut = outFlag;
    outFlag = IMPERVIOUS;
    
    if (rnd(100) > 4 && !debug)
        return;

    if (debug) mPrintf("(%d:", dAtt);

    dAtt += (rnd(200) - 100);

    if (debug) mPrintf("%d:", dAtt);

    if (dAtt > 0)
        att = 4;
    if (dAtt > 100)
        att = 3;
    if (dAtt > 500)
        att = 2;
    if (dAtt > 1000)
        att = 1;

    if (dAtt == 0)
        att = 5;

    if (dAtt < 0)
        att = 6;
    if (dAtt < -100)
        att = 7;
    if (dAtt < -500)
        att = 8;
    if (dAtt < -1000)
        att = 9;

    if (debug) cPrintf("%d)", att);

    switch(att)
    {
        case 1:
        case 2:
        case 3:
        case 4:         /* > 0 */
            doCR();
            mPrintf(" A dragon flys up to you and says \"%s", c4[rnd(5)]);
            doCR();
            break;
        case 5:         /* 0 */
            doCR();
            mPrintf(" You see a dragon fly by.");
            doCR();
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        default:
            break;
    }
    outFlag = tmpOut;

}
    
#endif

