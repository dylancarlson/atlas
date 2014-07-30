/* -------------------------------------------------------------------- */
/*  LOGOUT.C                 Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                      Overlayed logout log code                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  terminate()     is menu-level routine to exit system                */
/*  setalloldrooms()    set all rooms to be old.                        */
/*  initroomgen()   initializes room gen# with log gen                  */
/*  setdefaultconfig()  this sets the global configuration variables    */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  08/10/89    (PAT)   Modified into LOGOUT.C, moved functions around  */
/*                                                                      */
/*  06/14/89    (PAT)   Created from LOG.C to move some of the system   */
/*                      out of memory. Also cleaned up moved code to    */
/*                      -W3, ext.                                       */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  terminate()     is menu-level routine to exit system                */
/* -------------------------------------------------------------------- */
void terminate(char discon, char verbose)
{
    float balance;
    char  doStore;
    int   traptype;

    backout = FALSE;
    
    chatReq = FALSE;
    
    doStore = (BOOL)(CARRIER);

    if (discon || !doStore)
    {
        sysopNew = FALSE;
    }
      
    balance = logBuf.credits;

    outFlag = OUTOK;

    if (doStore && verbose == 2)
    {
        doCR();
        mPrintf(" You were caller %s", ltoac(cfg.callno));
        doCR();
        mPrintf(" You were logged in for: "); diffstamp(logtimestamp);
        doCR();
        mPrintf(" You entered %d %s", entered, cfg.msgs_nym);
        doCR();
        mPrintf(" and read %d.", mread);
        doCR();
        if (cfg.accounting && !logBuf.lbflags.NOACCOUNT)
        {
            mPrintf(" %.0f %s used this is call",startbalance - logBuf.credits,
              ( (int)(startbalance - logBuf.credits) == 1)?"minute":"minutes" );
            doCR();
            mPrintf(" Your balance is %.0f %s", logBuf.credits,
                 ( (int)logBuf.credits == 1 ) ? "minute" : "minutes" );
            doCR();
        }
    }

    if (doStore && verbose) goodbye();

    outFlag = IMPERVIOUS;

    if (loggedIn) mPrintf(" %s logged out\n ", logBuf.lbname);

    thisHall = 0;    /* go to ROOT hallway */

    if (discon) 
    {
        if (gotCarrier())
        {
            Hangup();
        }
        whichIO = MODEM;
        onConsole = FALSE;
    }

    if  (!doStore)  /* if carrier dropped */
    {
        /* trap it */
        sprintf(msgBuf->mbtext, "Carrier dropped");
        trap(msgBuf->mbtext, T_CARRIER);
    }   

    /* update new pointer only if carrier not dropped */
    if (loggedIn && doStore)
    {
        logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen;
        logBuf.lbroom[thisRoom].lvisit   = 0;

        /* logBuf.lbroom[thisRoom].mail  = 0; */
        talleyBuf->room[thisRoom].hasmail = 0;
    }

    if (loggedIn)
    {
        logBuf.callno      = cfg.callno;
        logBuf.calltime    = logtimestamp;
        logBuf.lbvisit[0]  = cfg.newest;
        logTab[0].ltcallno = cfg.callno;

        storeLog();
        loggedIn = FALSE;

        /* trap it */
        if (!logBuf.lbflags.NODE) 
        {
            sprintf(msgBuf->mbtext, "Logout %s", logBuf.lbname);
            trap(msgBuf->mbtext, T_LOGIN);
        }
        else
        {
            sprintf(msgBuf->mbtext, "NetLogout %s", logBuf.lbname);
            trap(msgBuf->mbtext, T_NETWORK);
        }

        if (cfg.accounting)  unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        logBuf.lbname[0] = 0;

        setalloldrooms();
    }

    /* setTerm(""); */
    setdefaultTerm(0); /* TTY */

    
    update25();

    setdefaultconfig();
    roomtalley();
    getRoom(LOBBY);

    if (!logBuf.lbflags.NODE)
        traptype = T_ACCOUNT;
    else
        traptype = T_NETWORK;


    sprintf(msgBuf->mbtext, "  ----- %4d messages entered", entered);
    trap(msgBuf->mbtext, traptype);

    sprintf(msgBuf->mbtext, "  ----- %4d messages read",  mread);
    trap(msgBuf->mbtext, traptype);

    if (logBuf.lbflags.NODE)
    {
       sprintf(msgBuf->mbtext, "  ----- %4d messages expired",  expired);
       trap(msgBuf->mbtext, T_NETWORK);

       sprintf(msgBuf->mbtext, "  ----- %4d messages duplicate",  duplicate);
       trap(msgBuf->mbtext, T_NETWORK);
    }    

    sprintf(msgBuf->mbtext, "Cost was %ld", (long)startbalance - (long)balance);
    trap(msgBuf->mbtext, T_ACCOUNT);
}

/* -------------------------------------------------------------------- */
/*  setalloldrooms()    set all rooms to be old.                        */
/* -------------------------------------------------------------------- */
void setalloldrooms(void)
{
    int i;

    for (i = 1; i < MAXVISIT; i++)
        logBuf.lbvisit[i] = cfg.newest;

    logBuf.lbvisit[0] = cfg.newest;
}

/* -------------------------------------------------------------------- */
/*  initroomgen()   initializes room gen# with log gen                  */
/* -------------------------------------------------------------------- */
void initroomgen(void)
{
    int i;

    for (i = 0; i < MAXROOMS;  i++)
    {
        /* Clear mail and xclude flags in logbuff for every room */

        /* logBuf.lbroom[i].mail    = FALSE;  */
        talleyBuf->room[i].hasmail = 0;

        logBuf.lbroom[i].xclude  = FALSE;

        if (roomTab[i].rtflags.PUBLIC == 1)
        {
            /* make public rooms known: */
            logBuf.lbroom[i].lbgen  = roomTab[i].rtgen;
            logBuf.lbroom[i].lvisit = MAXVISIT - 1;

        } else
        {
            /* make private rooms unknown: */
            logBuf.lbroom[i].lbgen =
                (uchar)((roomTab[i].rtgen + (MAXGEN-1)) % MAXGEN);

            logBuf.lbroom[i].lvisit = MAXVISIT - 1;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  setdefaultconfig()  this sets the global configuration variables    */
/* -------------------------------------------------------------------- */
void setdefaultconfig(void)
{
    prevChar    = ' ';
    termWidth   = cfg.width;
    termLF      = (BOOL)cfg.linefeeds;
    termUpper   = (BOOL)cfg.uppercase;
    termNulls   = cfg.nulls;
    expert      = FALSE;
    aide        = FALSE;
    sysop       = FALSE;
    twit        = cfg.user[D_PROBLEM];
    unlisted    = FALSE;
    termTab     = (BOOL)cfg.tabs;
    oldToo      = cfg.readOld;   /* later a cfg.lastold */
    roomtell    = FALSE;
    
    memset(&logBuf, 0, sizeof(logBuf));
    
    /* logBuf.linesScreen = cfg.linesScreen; */
    
    /* strcpy(logBuf.tty, "TTY"); */
    setdefaultTerm(0); /* TTY */
}


