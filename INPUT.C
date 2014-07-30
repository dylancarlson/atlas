/* -------------------------------------------------------------------- */
/*  INPUT.C                  Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  This file contains the input functions                              */
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
/*  getNormStr()    gets a string and normalizes it. No default.        */
/*  getNumber()     Get a number in range (top, bottom)                 */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/*  setio()         set io flags according to whicio, echo and outflag  */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/26/89    (PAT)   Created from MISC.C to break that moduel into   */
/*                      more managable and logical peices. Also draws   */
/*                      off MODEM.C                                     */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  getNormStr()    gets a string and normalizes it. No default.        */
/* -------------------------------------------------------------------- */
void getNormStr(char *prompt, char *s, int size, char doEcho)
{
    getString(prompt, s, size, FALSE, doEcho, "");
    normalizeString(s);
}

/* -------------------------------------------------------------------- */
/*  getNumber()     Get a number in range (top, bottom)                 */
/* -------------------------------------------------------------------- */
long getNumber(char *prompt, long bottom, long top, long dfaultnum)
{
    long try;
    label numstring;
    label buffer;
    char *dfault;

    dfault = ltoa(dfaultnum, buffer, 10);

    if (dfaultnum == -1l) dfault[0] = '\0';

    do
    {
        getString(prompt, numstring, NAMESIZE, FALSE, ECHO, dfault);
        try     = atol(numstring);
        if (try < bottom)  mPrintf("Sorry, must be at least %ld\n", bottom);
        if (try > top   )  mPrintf("Sorry, must be no more than %ld\n", top);
    }
    while ((try < bottom ||  try > top) && CARRIER);
    return  (long) try;
}

/* -------------------------------------------------------------------- */
/*  getString()     gets a string from user w/ prompt & default, ext.   */
/* -------------------------------------------------------------------- */
void getString(char *prompt, char *buf, int lim, char QuestIsSpecial, 
               char doEcho, char *dfault)
/* char *prompt; */          /* Enter PROMPT */
/* char *buf; */             /* Where to put it */
/* char doEcho; */           /* To echo, or not to echo, that is the question */
/* int  lim; */              /* max # chars to read */
/* char QuestIsSpecial; */   /* Return immediately on '?' input? */
/* char *dfault;*/           /* Default for the lazy. */
{
    char c, d, oldEcho, errors = 0;
    int  i;

    if (!CARRIER)
    {
        buf[0] = '\0';
        return;
    }
    
    outFlag = IMPERVIOUS;

    if ( strlen(prompt) )
    {
        doCR();

        if (strlen(dfault))
        {
            sprintf(gprompt, "2Enter %s [%s]:0 ", prompt, dfault);
        }
        else
        {
            sprintf(gprompt, "2Enter %s:0 ", prompt);
        }

        mPrintf(gprompt);

        dowhat = PROMPT;    
    }
    
    setio(whichIO, echo, outFlag);

    oldEcho = echo;
    echo     = NEITHER;
    
    if (!doEcho)
    {
        if (!cfg.nopwecho)
        {
            echoChar = 1;
        }
        else if (cfg.nopwecho == 1)
        {
            echoChar = '\0';
        }
        else 
        {
            echoChar = cfg.nopwecho;
        }
    }

    i   = 0;

    for (i =  0, c = (char)iChar(); 
         c != 10 /* NEWLINE */ && CARRIER;
         c = (char)iChar()
        )
    {
        outFlag = OUTOK;

        /*
         * handle delete chars: 
         */
        if (c == '\b')
        {
            if (i != 0)
            {
                doBS();
                i--;

                if ( (echoChar >= '0') && (echoChar <= '9'))
                {
                    echoChar--;
                    if (echoChar < '0') echoChar = '9';
                }
            }
            else 
            {
                oChar(7 /* bell */);
            }
        }
        else
        if (c == 0)
        {
            if (CARRIER)
            {
                continue;
            }
            else    
            {
                i = 0;
                break;
            }
        }
        else
        {
            if (c == CTRL_A && (i < lim-1) && cfg.colors)
            {
                /* CTRL-A>nsi   */
                d = (char)iChar();

                if (
                      (d >= '0' && d <= '4') 
                   || (d >= 'a' && d <= 'h')
                   || (d >= 'A' && d <= 'H')
                   )
                {
                    echo = oldEcho;
                    termCap(d);
                    echo = NEITHER;

                    buf[i++]   = 0x01;
                    buf[i++]   = d;
                }
                else 
                {
                    oChar(7);
                }
            }
            else

            if (i < lim && c != '\t')
            {
                if ( (echoChar >= '0') && (echoChar <= '9'))
                {
                    echoChar++;
                    if (echoChar > '9') echoChar = '0';
                }

                buf[i] = c;

                if (doEcho || cfg.nopwecho == 0)
                {
                    oChar(c);
                }
                else
                {
                    oChar(echoChar);   
                }

                i++;
            }
            else
            {
                oChar(7 /* bell */);

                errors++;
                if (errors > 15 && !onConsole)
                {
                    drop_dtr();
                }
            }
        }

        /* kludge to return immediately on single '?': */
        if (QuestIsSpecial && *buf == '?')  
        {
            doCR();
            break;
        }
    }

    echo     = oldEcho;
    buf[i]   = '\0';
    echoChar = '\0';

    if ( strlen(dfault) && !strlen(buf) ) strcpy(buf,dfault);

    dowhat = DUNO;

    doCR();
}


/* -------------------------------------------------------------------- */
/*  getYesNo()      Gets a yes/no/abort or the default                  */
/* -------------------------------------------------------------------- */
int getYesNo(char *prompt, char dfault)
{
    int  toReturn;
    char  c;
    char oldEcho;
    
    if (!CARRIER)
    {
        switch(dfault)
        {
        case 0:
        case 3:
            return FALSE;
            
        case 1:
        case 4:
            return TRUE;
            
        default:
            return 2;
        }
    }

    doCR();
    toReturn = ERROR;

    outFlag = IMPERVIOUS;
    sprintf(gprompt, "2%s? ", prompt);

    switch(dfault)
    {
    case 0: strcat(gprompt, "(Y/N)[N]");      break;
    case 1: strcat(gprompt, "(Y/N)[Y]");      break;
    case 2: strcat(gprompt, "(Y/N/A)[A]");    break;
    case 3: strcat(gprompt, "(Y/N/A)[N]");    break;
    case 4: strcat(gprompt, "(Y/N/A)[Y]");    break;
    default:                   
            strcat(gprompt, "(Y/N)[N]");
            dfault = 0;
            break;
    }
    
    strcat(gprompt, ":0 ");

    mPrintf(gprompt);

    dowhat = PROMPT;    
    
    do {
        oldEcho = echo;
        echo    = NEITHER;
        c       = (char)iChar();
        echo    = oldEcho;

        if ( (c == '\n') || (c == '\r') )
        {
            if (dfault == 1 || dfault == 4)  c = 'Y';
            if (dfault == 0 || dfault == 3)  c = 'N';
            if (dfault == 2)                 c = 'A';
        }

        switch (toupper(c))
        {
            case 'Y': mPrintf("Yes"  ); doCR(); toReturn   = 1;  break;
            case 'N': mPrintf("No"   ); doCR(); toReturn   = 0;  break;
            case 'A': 
                if (dfault > 1) 
                {
                    mPrintf("Abort");  doCR();

                    toReturn   = 2; 
                }
                break;
        }
    } while( toReturn == ERROR && CARRIER );

    outFlag = OUTOK;
    dowhat = DUNO;
    return   toReturn;
}

/* -------------------------------------------------------------------- */
/*  BBSCharReady()  Returns if char is avalible from modem or con       */
/* -------------------------------------------------------------------- */
int BBSCharReady(void)
{
    return (   (gotCarrier() && (whichIO == MODEM) && MIReady()) 
            || KBReady()  
           );
}

/* -------------------------------------------------------------------- */
/*  setio()         set io flags according to whicio, echo and outflag  */
/* -------------------------------------------------------------------- */
void setio(char whichio, char echo, char outflag)
{
    if ( !(outflag == OUTOK || outflag == IMPERVIOUS || outflag == NOSTOP))
    {
        modem   = FALSE;
        console = FALSE;
    }
    else if (echo == BOTH)
    {
        modem   = TRUE;
        console = TRUE;
    }  
    else if (echo == CALLER)
    {
        if (whichio == MODEM)
        {
           modem   = TRUE;
           console = FALSE;
        } 
        else if (whichio == CONSOLE)
        {
           modem   = FALSE;
           console = TRUE;
        }
    }
    else if (echo == NEITHER)
    {
        modem   = FALSE; 
        console = FALSE; 
    }

    if (!gotCarrier() || !modStat)  modem = FALSE;
}

/* -------------------------------------------------------------------- */
/*  iCharNE()   don't echo it..                                         */
/* -------------------------------------------------------------------- */
int iCharNE(void)
{
    char  c;
    char oldEcho;
    
    oldEcho = echo;
    echo    = NEITHER;
    c       = (char)iChar();
    echo    = oldEcho;

    return c;
}


/* -------------------------------------------------------------------- */
/*  iChar()         Get a character from user. This also indicated      */
/*                  timeout, carrierdetect, and a host of other things  */
/* -------------------------------------------------------------------- */
int iChar(void)
{
    char c = 0;
    long timer;

    sysopkey = FALSE; /* go into sysop mode from main()? */
    eventkey = FALSE; /* fo an event? */

    time(&timer);

    for (;;) /* while(TRUE) */
    {
        /*
         * Exit to MS-DOS key hit
         */
        if (ExitToMsdos) return 0;

        /*
         * Carrier lost state
         */
        if (!CARRIER)
        {
            cPrintf("Carrier lost!\n");
            return 0;
        }
        
        /*
         * Keyboard press
         */
        if (KBReady())
        {
            c = (char)ciChar();
            break;
        }

        /*
         *  Get character from modem..
         */
        if (MIReady() && gotCarrier() && modStat)
        {
            c = (char)getMod();

            if (whichIO == MODEM)
                break;
        }

        /*
         *  Request for sysop menu at main..
         */
        if ((sysopkey || chatkey) && dowhat == MAINMENU)  
            return(0);

        /*
         *  Sysop initiated chat..
         */
        if (chatkey && dowhat == PROMPT)
        {
            char oldEcho;

            oldEcho = echo;
            echo    = BOTH;

            doCR();
            chat();
            doCR();
            mPrintf(gprompt);

            echo   = oldEcho;

            time(&timer);

            chatkey = FALSE;
        }

        /*
         *  Keypress timeout
         */
        if (systimeout(timer)) 
        { 
            mPrintf("Sleeping? Call again! :-) \n "); 
            if (onConsole)
                onConsole = FALSE;
            Hangup(); 
        } 
    }

    c = tfilter[(uchar)c];

    if (c != 1    /* don't print ^A's          */
        && ((c != 'p' && c != 'P') || dowhat != MAINMENU)
        /* dont print out the P at the main menu... */
       ) echocharacter(c);  

    return(c);
}

