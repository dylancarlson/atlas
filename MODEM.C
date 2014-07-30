/* -------------------------------------------------------------------- */
/*  MODEM.C                  Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  High level modem code, should not need to be changed for porting(?) */
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
/*  domcr()         print cr on modem, nulls and lf's if needed         */
/*  offhook()       sysop fn: to take modem off hook                    */
/*  outstring()     push a string directly to the modem                 */
/*  Mflush()        Flush garbage characters from the modem.            */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Code moved to Input.c, output.c, and timedate.c */
/*  02/07/89    (PAT)   Hardeware dependant code moved to port.c,       */
/*                      History recreated. PLEASE KEEP UP-TO-DATE       */
/*  05/11/82    (CrT)   Created                                         */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  domcr()         print cr on modem, nulls and lf's if needed         */
/* -------------------------------------------------------------------- */
void domcr(void)
{
    int i;

    outMod('\r');
    for (i = termNulls;  i;  i--) outMod(0);
    if (termLF) outMod('\n');
}

/* -------------------------------------------------------------------- */
/*  offhook()       sysop fn: to take modem off hook                    */
/* -------------------------------------------------------------------- */
void offhook(void)
{
    Initport();
    outstring("ATM0H1\r");
    disabled = TRUE;
}

/* -------------------------------------------------------------------- */
/*  outstring()     push a string directly to the modem                 */
/* -------------------------------------------------------------------- */
void outstring(char *string)
{
    int mtmp;

    mtmp = modem;
    modem = TRUE;

    while(*string)
    {
        outMod(*string++);  /* output string */
    }

    modem = (uchar)mtmp;
}

/* -------------------------------------------------------------------- */
/*  Mflush()        Flush garbage characters from the modem.            */
/* -------------------------------------------------------------------- */
void Mflush(void)
{
    while (MIReady())
        getMod();
}

/* -------------------------------------------------------------------- */
/*      Hangup() breaks the modem connection                            */
/* -------------------------------------------------------------------- */
#ifndef TERM
void Hangup(void)
{
    if (!slv_door)
        pHangup();
    else
        modStat = FALSE;    /* hangup simulation ... */
}
#endif

