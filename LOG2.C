/* -------------------------------------------------------------------- */
/*  LOG2.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                     Overlayed newuser log code                       */
/*                  and configuration / userlog edit                    */
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
/*  initroomgen()   initializes room gen# with log gen                  */
/*  newlog()        sets up a new log entry for new users returns ERROR */
/*                  if cannot find a usable slot                        */
/*  newslot()       attempts to find a slot for a new user to reside in */
/*                  puts slot in global var  thisSlot                   */
/*  newUser()       prompts for name and password                       */
/*  newUserFile()   Writes new user info out to a file                  */
/*  configure()     sets user configuration via menu                    */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
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
/*  forwardaddr()   sets up forwarding address for private mail         */
/* -------------------------------------------------------------------- */
void forwardaddr(void)
{
    label name;
    int logno;

    doCR();
    
    getNormStr("forwarding name", name, NAMESIZE, ECHO);
    doCR();
    
    if( !strlen(name) )
    {
        mPrintf("Exclusive messages now routed to you"); doCR();
        logBuf.forward[0] = '\0';
    }
    else
    {
        logno = findPerson(name, lBuf2);

        if (logno == ERROR)
        {
            mPrintf("No '%s' known.", name); doCR();
            return;
        }

        mPrintf("Exclusive messages now forwarded to %s", lBuf2->lbname); doCR();
        strcpy(logBuf.forward, lBuf2->lbname);
    }
}

/* -------------------------------------------------------------------- */
/*  killuser()      sysop special to kill a log entry                   */
/* -------------------------------------------------------------------- */
void killuser(void)
{
    label who;
    int logno;

    getNormStr("who", who, NAMESIZE, ECHO);

    logno   = findPerson(who, &lBuf);

    if (logno == ERROR || !strlen(who))  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    if (strcmpi(logBuf.lbname, who) == SAMESTRING)
    {
        mPrintf("Cannot kill your own account, log out first.\n");
        return;
    }

    if (!getYesNo(confirm, 0))  return;

    mPrintf( "\'%s\' terminated.\n ", who);

    /* trap it */
    sprintf(msgBuf->mbtext, "User %s terminated", who);
    trap(msgBuf->mbtext, T_SYSOP);

    lBuf.lbname[0] = '\0';
    lBuf.lbin[  0] = '\0';
    lBuf.lbpw[  0] = '\0';
    lBuf.lbflags.L_INUSE   = FALSE;
    lBuf.lbflags.PERMANENT = FALSE;

    putLog(&lBuf, logno);
}

/* -------------------------------------------------------------------- */
/*  newPW()         is menu-level routine to change password & initials */
/* -------------------------------------------------------------------- */
void newPW(void)
{
    char InitPw[42];
    char passWord[42];
    char Initials[42];
    char oldPw[42];
    char *semicolon;

    int  goodpw;

    /* display old pw & initials */
    displaypw(logBuf.lbname, logBuf.lbin, logBuf.lbpw);

    if (!getYesNo(confirm, 0))  return;

    strcpy(oldPw, logBuf.lbpw);

    getNormStr("your new initials", InitPw, 40, NO_ECHO);
    dospCR();

    semicolon = strchr(InitPw, ';');

    if(semicolon)
    {
        normalizepw(InitPw, Initials, passWord);
    }
    else  strcpy(Initials, InitPw);

    /* dont allow anything over 19 characters */
    Initials[19] = '\0';

    do                           
    {
        if (!semicolon) 
        {
            getNormStr("new password", passWord, NAMESIZE, NO_ECHO);
            dospCR();
        }
        goodpw = ( ((pwexists(passWord) == ERROR) && strlen(passWord) >= 2)
            || (strcmpi(passWord, oldPw) == SAMESTRING));

        if ( !goodpw) mPrintf("\n Poor password\n ");
        semicolon = FALSE;
    } 
    while ( !goodpw && CARRIER);

    strcpy(logBuf.lbin, Initials);
    strcpy(logBuf.lbpw, passWord);

    /* insure against loss of carrier */
    if (CARRIER)
    {
        logTab[0].ltinhash      = hash(Initials);
        logTab[0].ltpwhash      = hash(passWord);

        storeLog();
    }

    /* display new pw & initials */
    displaypw(logBuf.lbname, logBuf.lbin, logBuf.lbpw);

    /* trap it */
    trap("Password changed", T_PASSWORD);
}

/* -------------------------------------------------------------------- */
/*  Readlog()       handles read userlog                                */
/* -------------------------------------------------------------------- */
void Readlog(BOOL verbose, BOOL revOrder)
{
    int i, grpslot;
    char dtstr[80];
    char flags[11];
    char wild=FALSE;
    char buser=FALSE;
    char step;

    grpslot = ERROR;

    if (mf.mfUser[0])
    {
        getNormStr("user", mf.mfUser, NAMESIZE, ECHO);                     

        stripansi(mf.mfUser);
                                                                           
        if (personexists(mf.mfUser) == ERROR)                              
        {                                                                  
            if(   strpos('?',mf.mfUser)                                       
               || strpos('*',mf.mfUser)                                       
               || strpos('[',mf.mfUser))                                      
            {                                                                 
                wild = TRUE;                                                  
            }                                                                 
            else                                                              
            {                                                                 
                mPrintf(" \nNo such user!\n ");                               
                return;                                                       
            }                                                                 
        }                                                                  
        else                                                               
        {                                                                  
            buser = TRUE;                                                    
        }                                                                  
    }

    outFlag = OUTOK;

    if (mf.mfLim && (cfg.readluser || sysop || aide))
    {
        doCR();
        getgroup();
        if (!mf.mfLim)
            return;
        grpslot = groupexists(mf.mfGroup);
    }
    else
    {
        mf.mfLim = FALSE;
    }

    outFlag = OUTOK;

    if (!expert) mPrintf(" \n \n <J>ump <N>ext <P>ause <S>top");

    if (!revOrder)
    {
        step = 1;
        i = 0;
    }
    else
    {
        step = -1;
        i = cfg.MAXLOGTAB - 1;
    }
    
    for ( ; 
          ( (i < cfg.MAXLOGTAB) && (outFlag != OUTSKIP) && (i >= 0) ); 
          i += step)
    {
        if(mAbort(FALSE))
            return;

        if (logTab[i].ltpwhash != 0 &&
            logTab[i].ltnmhash != 0)
        {
            if (buser && (int)hash(mf.mfUser) != logTab[i].ltnmhash)
                continue;

            getLog(&lBuf,logTab[i].ltlogSlot);

            if (buser && strcmpi(mf.mfUser, lBuf.lbname) != SAMESTRING)
                continue;

            if(wild && !u_match(deansi(lBuf.lbname), mf.mfUser))
                continue;

            if (mf.mfLim
              && lBuf.groups[grpslot] != grpBuf.group[grpslot].groupgen)
              continue;
       
            /* Show yourself even if unlisted */
            if ( (!i && loggedIn) || 
                 (lBuf.lbflags.L_INUSE
                  && (aide || !lBuf.lbflags.UNLISTED) )  )
            {


                if (verbose)
                {
                    strftime(dtstr, 79, cfg.vdatestamp, lBuf.calltime);

                    if ((cfg.surnames || cfg.titles) && /* verbose >= 2 */
                         logBuf.DISPLAYTS)
                    {
                        doCR();
                        doCR();
                        if (*lBuf.title)   mPrintf(" [%s]", lBuf.title);
                                           mPrintf(" 3%s0",   lBuf.lbname);
                        if (*lBuf.surname) mPrintf(" [%s]", lBuf.surname);
                        doCR();
                        mPrintf(" #%lu %s",  lBuf.callno, dtstr);
                    }
                    else
                    {
                        doCR();
                        mPrintf(" 3%-20s0 #%lu %s", lBuf.lbname, lBuf.callno, dtstr);
                    }
                }
                else
                {
                    doCR();
#if 1
                    if (aide || lBuf.lbflags.NODE)
#endif
                    mPrintf(" %-20s",lBuf.lbname);
#if 1
                    else
                        mPrintf(" %s", lBuf.lbname);
#endif
                }

                if (aide )    /*   A>ide T>wit P>erm U>nlist N>etuser S>ysop */
                {
                    if (cfg.accounting && verbose)
                    {
                        if (lBuf.lbflags.NOACCOUNT)
                             mPrintf( " %10s", "N/A");
                        else mPrintf( " %10.2f", lBuf.credits);
                    }
    
                    strcpy(flags, "         ");

                    if ( lBuf.lbflags.AIDE)      flags[0] = 'A';
                    if ( lBuf.lbflags.PROBLEM)   flags[1] = 'T';
                    if ( lBuf.lbflags.PERMANENT) flags[2] = 'P';
                    if ( lBuf.lbflags.NETUSER)   flags[3] = 'N';
                    if ( lBuf.lbflags.UNLISTED)  flags[4] = 'U';
                    if ( lBuf.lbflags.SYSOP)     flags[5] = 'S';
                    if ( lBuf.lbflags.NOMAIL)    flags[6] = 'M';
                    if ( lBuf.VERIFIED)          flags[7] = 'V';
                    if ( lBuf.DUNGEONED)         flags[8] = 'D';
                    if ( lBuf.MSGAIDE)           flags[9] = 'm';
    
                    mPrintf(" %s",flags);
                }

                if (lBuf.lbflags.NODE)
                {
                    mPrintf(" (Node) ");
                }
            }

            


#ifdef GOODBYE
                if (verbose)
                {
                    strftime(dtstr, 79, cfg.vdatestamp, lBuf.calltime);

                    if ((cfg.surnames || cfg.titles) && logBuf.DISPLAYTS)
                    {
                        doCR();
                        mPrintf(" [%s] 3%s0 [%s]", 
                                  lBuf.title, lBuf.lbname, lBuf.surname);
                        doCR();
                        mPrintf(" #%lu %s",  lBuf.callno, dtstr);
                    }
                    else
                    {
                        doCR();
                        mPrintf(" 3%-20s0 #%lu %s", lBuf.lbname, lBuf.callno, dtstr);
                    }
                }
                else
                {
                    doCR();
                    mPrintf(" %-20s",lBuf.lbname);
                }

                if (aide )    /*   A>ide T>wit P>erm U>nlist N>etuser S>ysop */
                {
                    if (cfg.accounting && verbose)
                    {
                        if (lBuf.lbflags.NOACCOUNT)
                             mPrintf( " %10s", "N/A");
                        else mPrintf( " %10.2f", lBuf.credits);
                    }
    
                    strcpy(flags, "         ");

                    if ( lBuf.lbflags.AIDE)      flags[0] = 'A';
                    if ( lBuf.lbflags.PROBLEM)   flags[1] = 'T';
                    if ( lBuf.lbflags.PERMANENT) flags[2] = 'P';
                    if ( lBuf.lbflags.NETUSER)   flags[3] = 'N';
                    if ( lBuf.lbflags.UNLISTED)  flags[4] = 'U';
                    if ( lBuf.lbflags.SYSOP)     flags[5] = 'S';
                    if ( lBuf.lbflags.NOMAIL)    flags[6] = 'M';
                    if ( lBuf.VERIFIED)          flags[7] = 'V';
                    if ( lBuf.DUNGEONED)         flags[8] = 'D';
                    if ( lBuf.MSGAIDE)           flags[9] = 'm';
    
                    mPrintf(" %s",flags);
                }

                if (lBuf.lbflags.NODE)
                {
                    mPrintf(" (Node) ");
                }

                if (verbose) doCR();
            }

#endif

        }
    }
    doCR();
}






/* -------------------------------------------------------------------- */
/*  showuser()      aide fn: to display any user's config.              */
/* -------------------------------------------------------------------- */
void showuser(void)
{  
    label who;
    int logno, oldloggedIn, oldthisLog;

    oldloggedIn = loggedIn;
    oldthisLog  = thisLog;

    loggedIn = TRUE;

    getNormStr("who", who, NAMESIZE, ECHO);

    if( strcmpi(who, logBuf.lbname) == SAMESTRING)
    {
        showconfig(&logBuf);
    }
    else
    {
        logno   = findPerson(who, &lBuf);

        if ( !strlen(who) || logno == ERROR)
        {
            mPrintf("No \'%s\' known. \n ", who);
        }
        else
        {
            showconfig(&lBuf);
        }
    }
   
    loggedIn = (BOOL)oldloggedIn;
    thisLog  = oldthisLog;
}



/* -------------------------------------------------------------------- */
/*  enterName()     alows a users to enter a name w/o certain characters*/
/* -------------------------------------------------------------------- */
void enterName(char *prompt, char *name, char *oldname)
{
    unsigned char c;
    
    for (c=0; c<128; c++)
    {
        if (isalpha(c) || isdigit(c) || c == '.' || c == '-' || c == '\'') 
        {
            tfilter[c] = c;
        }
        else
        {
            tfilter[c] = '\0';
        }
    }

    tfilter[1   ]  = 1   ;  /* Ctrl-A    = Ctrl-A    */    
    tfilter[0x7f]  = 8   ;  /* del       = backspace */
    tfilter[8   ]  = 8   ;  /* backspace = backspace */
    tfilter['\r']  = 10  ;  /* '\r'      = NEWLINE   */
    tfilter[' ' ]  = ' ' ;
    
    if (oldname == NULL)
    {
        getNormStr(prompt, name, NAMESIZE, ECHO); 
    }
    else
    {
        getString(prompt, name, NAMESIZE, FALSE, ECHO, oldname);
    }
    
    asciitable();
}



/* -------------------------------------------------------------------- */
/*      defaulthall() handles enter default hallway   .ed               */
/* -------------------------------------------------------------------- */
void defaulthall(char *def)
{
    label hallname;
    int slot, accessible;

    doCR();
    
    if (logBuf.LOCKHALL)
    {
        doCR();
        mPrintf("Your default hallway is locked.");
        doCR();
        return;
    }
    
    getString("hallway", hallname, NAMESIZE, FALSE, ECHO, def);
    normalizeString(hallname);
    if (!strlen(hallname))
    {
        doCR();
        mPrintf("No default hallway.");
        doCR();
        logBuf.hallhash = 0;
        return;
    }
    
    slot = partialhall(hallname);
    if (slot != ERROR) accessible = accesshall(slot);

    if ( (slot == ERROR) || !strlen(hallname) || !accessible )
    {
        doCR();
        mPrintf("No such hall, or not accessable from here.");
        return;
    }

    strcpy(hallname, hallBuf->hall[slot].hallname);

    doCR();
    mPrintf("Default hallway now: %s", hallname);
    doCR();
    
    logBuf.hallhash = hash(hallname);

    /* 0 for root hallway */
    if (slot == 0) logBuf.hallhash = 0;
}

/* -------------------------------------------------------------------- */
/*  configure()     sets user configuration via menu                    */
/* -------------------------------------------------------------------- */
void configure(BOOL new)
{
    BOOL    prtMess ;
    BOOL    quit    = FALSE;
    int     c;
    int     i;
    label   temp;
    label   dHall;
    label   dProtocol;
    char    ich;
    
    doCR();

    setlogconfig();
    memcpy(&lBuf, &logBuf, sizeof(struct logBuffer));

    *dHall = '\0';

    /* Only display that cruddy, long menu if the user isn't an expert.  */

    prtMess = (BOOL) !expert ;
    
    do 
    {
        if (prtMess)
        {
            if (logBuf.hallhash)
            {
                for (i = 1; i < MAXHALLS; ++i)
                {
                    if ( (int)hash( hallBuf->hall[i].hallname ) ==
                            logBuf.hallhash && hallBuf->hall[i].h_inuse)
                    {
                        if (groupseeshall(i))
                            strcpy(dHall, hallBuf->hall[i].hallname);
                    }
                }
            }
            else
            {
                strcpy(dHall, hallBuf->hall[0].hallname);
            }

            if (logBuf.protocol)
            {
                i = strpos(logBuf.protocol, extrncmd);
                if (!i)
                {
                    dProtocol[0] = 0;
                }
                else
                {
                    strcpy(dProtocol, extrn[i-1].ex_name);
                }
            }
            else
            {
                dProtocol[0] = 0;
            }
            
            doCR();
            outFlag = OUTOK;


            mPrintf("<310> Ansi Mode......... %s",
                        logBuf.IBMANSI ? "On" : "Off") ;
            doCR() ;

            mPrintf("<320> Ansi Color........ %s",
                        logBuf.IBMCOLOR ? "On" : "Off") ;
            doCR() ;

            mPrintf("<330> IBM graphics chrs. %s",
                        logBuf.IBMGRAPH ? "On" : "Off") ;
            doCR() ;

            if (logBuf.IBMANSI)
                mPrintf(
                "<340> Attributes........ 0Normal0, 1Blinking0, 2Inverted0, 3Boldface0, 4Underline0"
                ) ; doCR() ;

            mPrintf("<3W0> Screen 3W0idth...... %d", termWidth) ; doCR() ;
            mPrintf("<3L0> 3L0ines per Screen.. %s", logBuf.linesScreen 
                                        ? itoa(logBuf.linesScreen, temp, 10) : 
                                        "Screen Pause Off"); doCR() ;
        /* mPrintf("<3T0> 3T0erminal Type..... %s", term.name); doCR(); */


            mPrintf("<3H0> 3H0elpful Hints..... %s", 
                                                !expert ? "On" : "Off"); doCR() ;
            mPrintf("<3U0> List in 3U0serlog... %s", 
                                                !unlisted ? "Yes" : "No"); doCR() ;
            mPrintf("<3O0> Last 3O0ld on New... %s",
                                                oldToo ? "On" : "Off");  doCR() ;
            mPrintf("<3R0> 3R0oom descriptions. %s",
                                                roomtell ? "On" : "Off"); doCR() ;

            if (cfg.surnames || cfg.netsurname || cfg.titles || cfg.nettitles)
                {
                mPrintf("<3I0> T3i0tles/surnames... %s",
                                           logBuf.DISPLAYTS ? "On" : "Off");
                doCR() ;
                }
            mPrintf("<3J0> Sub3j0ects.......... %s",
                                           logBuf.SUBJECTS ? "On" : "Off");doCR() ;
            mPrintf("<3G0> Si3g0natures........ %s",
                                           logBuf.SIGNATURES ? "On" : "Off");doCR() ;

            mPrintf("<3X0> Auto-ne3X0t hall.... %s", 
                                                logBuf.NEXTHALL ? "On" : "Off");    doCR() ;
            mPrintf("<3C0> Upper3C0ase only.... %s",
                                                termUpper ? "On" : "Off");doCR() ;
            mPrintf("<3F0> Line3F0eeds......... %s", 
                                                termLF ? "On" : "Off");  doCR() ;
            mPrintf("<3B0> Ta3B0s.............. %s", 
                                                termTab ? "On" : "Off"); doCR() ;
            mPrintf("<3N0> 3N0ulls............. %s", 
                                                termNulls ?
                                                itoa(termNulls, temp, 10) : 
                                                "Off");doCR() ;
            mPrintf("<3D0> 3D0efault Hall...... %s", dHall);doCR() ;
            mPrintf("<3P0> Default 3P0rotocol.. %s", dProtocol);doCR() ;
            mPrintf("<350> Forwarding To..... %s", logBuf.forward);doCR() ;
            
            if (!new)
            {
                doCR();
                mPrintf("<3S0> to save, <3A0> to abort.") ;doCR() ;
            }
            prtMess = (BOOL)(!expert);
        }

        if (new)
        {
            if (getYesNo("Is this OK", 1))
            {
                quit = TRUE;
                continue;
            }
            new = FALSE;
        }

        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("2Change:0 ");
        
        c       = iCharNE();

        if (!(CARRIER))
            return;

        switch(toupper(c))
        {
        case '1':
            logBuf.IBMANSI = (BOOL)(!logBuf.IBMANSI);
            mPrintf("Ansi Mode %s", logBuf.IBMANSI ? "On" : "Off"); 
            doCR();
            setdefaultcolors();
            setlogTerm();
            break;
        case '2':
            logBuf.IBMCOLOR = (BOOL)(!logBuf.IBMCOLOR);
            mPrintf("Ansi Color %s", logBuf.IBMCOLOR ? "On" : "Off"); 
            doCR();
            setdefaultcolors();
            setlogTerm();
            break;
        case '3':
            logBuf.IBMGRAPH = (BOOL)(!logBuf.IBMGRAPH);
            mPrintf("IBM graphics chars %s", logBuf.IBMGRAPH ? "On" : "Off"); 
            doCR();
            setlogTerm();
            break;
        case 'I':
            if (!(cfg.surnames || cfg.netsurname
                  || cfg.titles || cfg.nettitles))
                break;

            logBuf.DISPLAYTS = (BOOL)(!logBuf.DISPLAYTS);
            mPrintf("D%sisplay titles/surnames", logBuf.DISPLAYTS
                     ? "" : "o not d");
            doCR();
            break;

#ifdef GOODBYE
            if (!(cfg.surnames || cfg.netsurname
                  || cfg.titles || cfg.nettitles))
                break;
            if (logBuf.DISPLAYTS && (logBuf.title[0] || logBuf.surname[0]))
            {
                if (!getYesNo("Nullify title and surname", 1))
                    break;
                logBuf.title[0] = '\0';
                logBuf.surname[0] = '\0';
                doCR();
            }
            logBuf.DISPLAYTS = (BOOL)(!logBuf.DISPLAYTS);
            mPrintf("D%sisplay titles/surnames", logBuf.DISPLAYTS
                     ? "" : "o not d");
            doCR();
            break;

#endif
        case 'J':
            logBuf.SUBJECTS = (BOOL)(!logBuf.SUBJECTS);
            mPrintf("D%sisplay subjects", logBuf.SUBJECTS ? "" : "o not d");
            doCR();
            break;
        case 'G':
            logBuf.SIGNATURES = (BOOL)(!logBuf.SIGNATURES);
            mPrintf("D%sisplay signatures", logBuf.SIGNATURES
                                            ? "" : "o not d");
            doCR();
            break;
        case 'W':
            mPrintf("Screen Width"); doCR();
            termWidth = 
                (uchar)getNumber("Screen width", 10l, 255l,(long)termWidth);
            /* kludge for carr-loss */
            if (termWidth < 10) termWidth = cfg.width;
            break;

        case 'L':
            if (!logBuf.linesScreen)
            {
                mPrintf("Pause on full screen"); doCR();
                logBuf.linesScreen =
                    (uchar) getNumber("Lines per screen", 10L, 80L, 21L);
            }
            else
            {
                mPrintf("Pause on full screen off"); doCR();
                logBuf.linesScreen = 0;
            }
            break;
              
        case 'C':
            termUpper = (BOOL)(!termUpper);
            mPrintf("Uppercase only %s", termUpper ? "On" : "Off"); doCR();
            break;

        case 'F':
            termLF = (BOOL)(!termLF);
            mPrintf("Linefeeds %s", termLF ? "On" : "Off");  doCR();
            break;

        case 'B':
            termTab = (BOOL)(!termTab);
            mPrintf("Tabs %s", termTab ? "On" : "Off"); doCR();
            break;

        case 'N':
            if (!termNulls)
            {
                mPrintf("Nulls"); doCR();
                termNulls = (uchar) getNumber("number of Nulls", 0L, 255L, 5L);
            }
            else
            {
                mPrintf("Nulls off"); doCR();
                termNulls = 0;
            }
            break;

#ifdef GOODBYE
        case 'T':
            mPrintf("Terminal Emulation"); doCR();
            doCR();
            askTerm();
            break;
#endif

        case 'H':
            expert = (BOOL)(!expert);
            mPrintf("Helpful Hints %s", !expert ? "On" : "Off"); doCR();
            prtMess = (BOOL)(!expert);
            break;

        case 'U':
            unlisted = (BOOL)(!unlisted);
            mPrintf("List in userlog %s", !unlisted ? "Yes" : "No"); doCR();
            break;

        case 'O':
            oldToo = (BOOL)(!oldToo);
            mPrintf("Last Old on New %s", oldToo ? "On" : "Off");  doCR();
            break;

        case 'R':
            roomtell = (BOOL)(!roomtell);
            mPrintf("Room descriptions %s", roomtell ? "On" : "Off"); doCR();
            break;

        case 'X':
            logBuf.NEXTHALL = (BOOL)(!logBuf.NEXTHALL);
            mPrintf("Auto-next hall %s", logBuf.NEXTHALL ? "On" : "Off"); 
                doCR();
            break;

        case 'S':
            mPrintf("Save changes"); doCR();
            if (getYesNo("Save changes", 1))
            {
                quit = TRUE;
                storeLog();

                /* setlogconfig(); */
                /* putLog(&lBuf, thisLog); */
            }
            break;

        case 'A':
            mPrintf("Abort"); doCR();
            if (getYesNo("Abort changes", 1))
            {
                memcpy(&logBuf, &lBuf, sizeof(struct logBuffer));
                setsysconfig();
                /* setlogTerm(); */
                return;
            }
            break;

        case 'D':
            mPrintf("Default Hallway");
            defaulthall(dHall);
            break;
            
        case '5':
            mPrintf("Forwarding Address");
            forwardaddr();
            break;
            
        case 'P':
            c = 0;           
            mPrintf("Default Protocol");
            doCR();
            doCR();

            do
            {
                if (!expert || (c == '?'))
                {
                    if (c == '?')
                        oChar((char)c);
                    doCR();
                    for (i=0; i<(int)strlen(extrncmd); i++)
                        mPrintf(" 3%c0>%s\n", *(extrn[i].ex_name),
                            (extrn[i].ex_name + 1));
                    doCR();
                    doCR();
                }          
                                       
                mPrintf("2Protocol:0 ");
                c       = tolower(ich=(char)iCharNE());
            } while (c == '?');
            
            i = strpos((char)c, extrncmd);
            if (!i)
            {
                oChar(ich);
                doCR();
                mPrintf(" '?' for menu.");
                doCR();
                doCR();
                break;
            }   
            else
            {
                mPrintf("%s", extrn[i-1].ex_name);
                doCR();
                doCR();
                logBuf.protocol = (char)c;
                break;
            }
            break;
            
        case '\r':
        case '\n':
        case '?':
            mPrintf("Menu"); doCR();
            prtMess = TRUE;
            break;


        case '4':
            if (logBuf.IBMANSI)
            {
                mPrintf("Attributes");
                askAttributes();
                break;
            }   /* Fall through */
        default:
            mPrintf("%c ? for help", c); doCR();
            break;
        }
    
    } while (!quit);
}

/* -------------------------------------------------------------------- */
/*  showconfig()    displays user configuration                         */
/* -------------------------------------------------------------------- */
void showconfig(struct logBuffer *lbuf)
{
    int i;
    char *dodisplay = "";
    char *dont = "o not d";
    label   dProtocol;


    outFlag = OUTOK;
    
    doCR();
    
    doCR();
    termCap(TERM_BOLD);
    mPrintf("User: ");
    termCap(TERM_NORMAL);
    doCR();
    
    if (cfg.titles && *lbuf->title && lbuf->DISPLAYTS)
    {
        mPrintf("[%s] ", lbuf->title);
    }
    
    mPrintf("%s", lbuf->lbname);
    
    if (cfg.surnames && *lbuf->surname && lbuf->DISPLAYTS)
    {
        mPrintf(" [%s]", lbuf->surname);
    }
    mPrintf("0"); doCR();
    
    doCR();
    termCap(TERM_BOLD);
    mPrintf("Access:");
    termCap(TERM_NORMAL);
    doCR();

    if (lbuf->lbflags.UNLISTED ||
        lbuf->lbflags.SYSOP    ||
        lbuf->lbflags.AIDE     ||
        lbuf->lbflags.NETUSER  ||
        lbuf->lbflags.NODE     ||
        lbuf->DUNGEONED        ||
        lbuf->MSGAIDE)
    {
        if (lbuf->lbflags.NODE)         mPrintf("(Node) ");
        if (lbuf->lbflags.AIDE)         mPrintf("Aide ");
        if (lbuf->lbflags.SYSOP)        mPrintf("Sysop ");
        if (lbuf->MSGAIDE)              mPrintf("Moderator ");
        if (lbuf->DUNGEONED)            mPrintf("Dungeoned ");
        if (lbuf->lbflags.NETUSER)      mPrintf("Netuser ");
        if (lbuf->lbflags.UNLISTED)     mPrintf("Unlisted ");
        if (lbuf->lbflags.PERMANENT)    mPrintf("Permanent");
        doCR();
    }
    
    mPrintf("Groups: ");

    prtList(LIST_START);
    for (i = 0; i < MAXGROUPS; ++i)
    {
        if (   grpBuf.group[i].g_inuse
            && (lbuf->groups[i] == grpBuf.group[i].groupgen)
           )
        {
            prtList(grpBuf.group[i].groupname);
        }
    }
    prtList(LIST_END);
    
#ifdef GOODBYE
    if (onConsole || lbuf == &logBuf)
    {
        mPrintf("Password: %s;%s", lbuf->lbin, lbuf->lbpw); doCR();
    }
#endif
    
    doCR();
    termCap(TERM_BOLD);
    mPrintf("Options:");
    termCap(TERM_NORMAL);
    doCR();


    mPrintf("Helpful Hints %s.", !expert ? "On" : "Off"); 
    doCR();

    if (lbuf->forward[0])
    {
        mPrintf("Exclusive messages forwarded to ");

        if ( personexists(lbuf->forward) != ERROR )
            mPrintf("%s", lbuf->forward);
        doCR();
    }

    if (lbuf->hallhash)
    {
        mPrintf("Default Hallway: ");

        for (i = 1; i < MAXHALLS; ++i)
        {
            if ( (int)hash( hallBuf->hall[i].hallname )  == lbuf->hallhash )
            {
                if (groupseeshall(i))
                    mPrintf("%s", hallBuf->hall[i].hallname);
            }
        }
        doCR();
    }




   /* beginning of default protocol stuff */

    if (lbuf->protocol)
    {
        i = strpos(lbuf->protocol, extrncmd);
        if (!i)
        {
            dProtocol[0] = 0;
        }
        else
        {
            strcpy(dProtocol, extrn[i-1].ex_name);
        }
    }
    else
    {
        dProtocol[0] = 0;
    }
            
    if (dProtocol[0])
    {
        mPrintf("Default Protocol: %s", dProtocol); 
        doCR();
    }

/***** end of default protocol stuff ****/


    
/* stuff gotten from ACIT */

    mPrintf("D%sisplay last Old %s on N>ew %s request.",
            lbuf->lbflags.OLDTOO ? dodisplay : dont,
            cfg.msg_nym, cfg.msg_nym);
    doCR();

    mPrintf("Auto-next hall");

    if (lbuf->NEXTHALL)
        mPrintf(" on.");
    else
        mPrintf(" off.");

        doCR();

    
    if (cfg.roomtell && loggedIn)
    {
        mPrintf("D%sisplay room descriptions.",
            lbuf->lbflags.ROOMTELL ? dodisplay : dont);
        doCR();
    }
    
    if (cfg.surnames || cfg.netsurname || cfg.titles || cfg.nettitles)
    {
        mPrintf("D%sisplay surnames and titles.",
                lbuf->DISPLAYTS ? dodisplay : dont);
        doCR();
    }

    mPrintf("D%sisplay subjects.",
            lbuf->SUBJECTS ? dodisplay : dont);
    doCR();

    mPrintf("D%sisplay signatures.",
            lbuf->SIGNATURES ? dodisplay : dont);
    doCR();

/* end of stuff gotten from ACIT */

   
    doCR();
    termCap(TERM_BOLD);
    mPrintf("Terminal:");
    termCap(TERM_NORMAL);
    doCR();
    /* mPrintf("Type: %s", lbuf->tty);    doCR(); */


    mPrintf("Ansi Mode %s.",  logBuf.IBMANSI ? "On" : "Off");
    doCR();
    mPrintf("Ansi Color %s.", logBuf.IBMCOLOR ? "On" : "Off");
    doCR();

    mPrintf("IBM Graphics characters %s" "abled.",
            lbuf->IBMGRAPH ? "en" : "dis");
    doCR();

    mPrintf("Attributes: 0Normal0, 1Blinking0, 2Inverted0, 3Boldface0, 4Underline0");
    doCR();

 
    mPrintf("Width %d, ", lbuf->lbwidth);
 
    if (lbuf->lbflags.UCMASK ) mPrintf("UPPERCASE ONLY, ");
 
    if (!lbuf->lbflags.LFMASK) mPrintf("No ");

    mPrintf("Linefeeds, ");
 

    if (lbuf->linesScreen)
    {
        mPrintf("%d Lines, ", lbuf->linesScreen);
    }


    mPrintf("%d Nulls, ", lbuf->lbnulls);

    if (!lbuf->lbflags.TABS) mPrintf("No ");

    mPrintf("Tabs");
    doCR();

    
    if (cfg.accounting && !lbuf->lbflags.NOACCOUNT)
    {
        doCR();
        mPrintf("Time in account 3%.0f0 minute(s)", lbuf->credits);
        doCR();
    }
}

