/* -------------------------------------------------------------------- */
/*  LOGEDIT.C                Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                          Userlog edit code                           */
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
/*  userEdit()      Edit a user via menu                                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  10/04/90    (PAT)   Created from LOG.C to move some of the system   */
/*                      out of memory.                                  */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  userEdit()      Edit a user via menu                                */
/* -------------------------------------------------------------------- */
void userEdit(void)
{
    BOOL    prtMess = TRUE;
    BOOL    quit    = FALSE;
    int     c;
    char    string[200];
    char    oldEcho;
    label   who, temp;
    int     logNo, tsys;
    BOOL    editSelf = FALSE;
    label   dHall;
    int     i;
    
    *dHall = '\0';
    
    getNormStr("who", who, NAMESIZE, ECHO);
    logNo    = findPerson(who, &lBuf);
    personexists(who);

    if ( !strlen(who) || logNo == ERROR)  
    {
        mPrintf("No \'%s\' known. \n ", who);
        return;
    }

    /* make sure we use curent info */
    if (strcmpi(who, logBuf.lbname) == SAMESTRING)
    {
        tsys = logBuf.lbflags.SYSOP;
        setlogconfig(); /* update curent user */
        logBuf.lbflags.SYSOP = tsys;
        lBuf = logBuf;  /* use their online logbuffer */
        editSelf = TRUE;
    }

    doCR();

    do 
    {
        if (prtMess)
        {
            if (lBuf.hallhash)
            {
                for (i = 1; i < MAXHALLS; ++i)
                {
                  if ( (int)hash( hallBuf->hall[i].hallname )  == lBuf.hallhash 
                       && hallBuf->hall[i].h_inuse)
                    {
                        strcpy(dHall, hallBuf->hall[i].hallname);
                    }
                }
            }
            else
            {
                strcpy(dHall, hallBuf->hall[0].hallname);
            }
            
            doCR();
            outFlag = OUTOK;
            mPrintf("<3N0> 3N0ame............. %s", lBuf.lbname);  doCR();
            mPrintf("<310> Title............ %s", lBuf.title);   doCR();
            mPrintf("<320> Surname.......... %s", lBuf.surname); doCR();
            mPrintf("<3L0> 3L0ock T & Surname. %s", 
                                            lBuf.SURNAMLOK ? "Yes" : "No");            
                                            doCR();
            mPrintf("<3Y0> S3y0sop............ %s", 
                                            lBuf.lbflags.SYSOP ? "Yes" : "No");
                                            doCR();
            mPrintf("<3D0> Ai3d0e............. %s", 
                                            lBuf.lbflags.AIDE ? "Yes" : "No");
                                            doCR();
            mPrintf("<3O0> N3o0de............. %s", 
                                            lBuf.lbflags.NODE ? "Yes" : "No");
                                            doCR();
            mPrintf("<3P0> 3P0ermanent........ %s", 
                                            lBuf.lbflags.PERMANENT ?"Yes":"No");
                                            doCR();
            mPrintf("<3E0> N3e0tuser.......... %s", 
                                            lBuf.lbflags.NETUSER ? "Yes" :"No");
                                            doCR();
            mPrintf("<3T0> 3T0wited........... %s", 
                                            lBuf.lbflags.PROBLEM ? "Yes" :"No");  
                                            doCR();
            mPrintf("<3M0> 3M0ail............. %s", 
                                            lBuf.lbflags.NOMAIL ? "Off" : "On");
                                            doCR();
            mPrintf("<3V0> 3V0erified......... %s",
                                            !lBuf.VERIFIED ? "Yes" : "No");
                                            doCR();
            mPrintf("<3B0> 3B0orders.......... %s", 
                                            lBuf.BOARDERS ? "Yes" :"No");  
                                            doCR();
            mPrintf("<3H0> Default 3H0all..... %s %s",
                                            dHall,
                                            lBuf.LOCKHALL ? "3<Locked>0" : ""
                                            );
                                            doCR();
            mPrintf("<3U0> 3U0nlisted......... %s",
                                            lBuf.lbflags.UNLISTED ? "Yes" : "No");
                                            doCR();
            mPrintf("<330> Auto Next Hall... %s",
                                            lBuf.NEXTHALL ? "On" : "Off");
                                            doCR();
            mPrintf("<340> Psyco user....... %s",         
                                            lBuf.PSYCHO ? "Yes" : "No");
                                            doCR();
                                            
            if (cfg.accounting)
            {
                mPrintf("<3I0> T3i0me (minutes)... ");
                
                if (lBuf.lbflags.NOACCOUNT)
                    mPrintf("N/A");
                else
                    mPrintf("%.0f", lBuf.credits);
    
                doCR();
            }
            
            mPrintf("<3R0> 3R0eset all new message pointers");  doCR();
            
            if (onConsole)
            {
                mPrintf("    Password.......... %s;%s", lBuf.lbin, lBuf.lbpw); 
                                                            doCR();
            }
            
            doCR();
            mPrintf("<3S0> to save, <3A0> to abort."); doCR();
            prtMess = (BOOL)(!expert);
        }

        outFlag = IMPERVIOUS;

        doCR();
        mPrintf("2Change:0 ");
        
        oldEcho = echo;
        echo    = NEITHER;
        c       = iChar();
        echo    = oldEcho;

        if (!CARRIER)
            return;

        switch(toupper(c))
        {
        case 'N':
            mPrintf("Name"); doCR();
            strcpy(temp, lBuf.lbname);
            getString("New name", lBuf.lbname, NAMESIZE, FALSE, ECHO, temp);
            normalizeString(lBuf.lbname);
            
            if ( 
                   (
                      personexists(lBuf.lbname) != ERROR 
                   && strcmpi (lBuf.lbname, temp) != SAMESTRING
                   )
                   || (strcmpi(lBuf.lbname, "Sysop") == SAMESTRING)
                   || (strcmpi(lBuf.lbname, "Aide") == SAMESTRING)
                   || !strlen (lBuf.lbname) 
               )
            {
                strcpy(lBuf.lbname, temp);
            }
            break;

        case '1':
            mPrintf("Title"); doCR();
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                doCR();
                mPrintf("User has locked thier title and surname!"); doCR();
            }
            else 
            {
                strcpy(temp, lBuf.title);
                getString("new title", lBuf.title, NAMESIZE, FALSE, ECHO, temp);
                if (!strlen(lBuf.title))
                {
                    strcpy(lBuf.title, temp);
                }
                normalizeString(lBuf.title);
            }
            break;
        
        case '2':
            mPrintf("Surname"); doCR();
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                doCR();
                mPrintf("User has locked thier title and surname!"); doCR();
            }
            else 
            {
                strcpy(temp, lBuf.surname);
                getString("New surname", lBuf.surname, NAMESIZE, FALSE, ECHO, temp);
                if (!strlen(lBuf.surname))
                {
                    strcpy(lBuf.surname, temp);
                }
                normalizeString(lBuf.surname);
            }
            break;

        case 'L':
            if (lBuf.lbflags.SYSOP && lBuf.SURNAMLOK && !editSelf)
            {
                mPrintf("Lock Title and Surname.");  doCR();
                doCR();
                mPrintf("You can not change that!"); doCR();
            }
            else
            {
                lBuf.SURNAMLOK = (BOOL)(!lBuf.SURNAMLOK);
                mPrintf("Lock Title and Surname: %s", 
                                                lBuf.SURNAMLOK ? "On" : "Off");
                                                doCR();
            }
            break;

        case 'Y':
            lBuf.lbflags.SYSOP = (BOOL)(!lBuf.lbflags.SYSOP);
            mPrintf("Sysop %s", lBuf.lbflags.SYSOP ? "Yes" : "No");  doCR();
            break;
        
        case 'B':
            lBuf.BOARDERS = (BOOL)(!lBuf.BOARDERS);
            mPrintf("Borders %s", lBuf.BOARDERS ? "Yes" : "No");  doCR();
            break;
        
        case 'U':
            lBuf.lbflags.UNLISTED = (BOOL)(!lBuf.lbflags.UNLISTED);
            mPrintf("Unlisted %s", lBuf.lbflags.UNLISTED ? "Yes" : "No");  doCR();
            break;
        
        case '3':
            lBuf.NEXTHALL = (BOOL)(!lBuf.NEXTHALL);
            mPrintf("Auto Next Hall %s", lBuf.NEXTHALL ? "On" : "Off");  
            doCR();
            break;
        
        case '4':
            lBuf.PSYCHO = (BOOL)(!lBuf.PSYCHO);
            mPrintf("Psycho %s", lBuf.PSYCHO ? "On" : "Off");  
            doCR();
            break;

        case 'D':
            lBuf.lbflags.AIDE = (BOOL)(!lBuf.lbflags.AIDE);
            mPrintf("Aide %s", lBuf.lbflags.AIDE ? "Yes" : "No");  doCR();
            break;

        case 'O':
            lBuf.lbflags.NODE = (BOOL)(!lBuf.lbflags.NODE);
            mPrintf("Node %s", lBuf.lbflags.NODE ? "Yes" : "No");  doCR();
            break;

        case 'P':
            lBuf.lbflags.PERMANENT = (BOOL)(!lBuf.lbflags.PERMANENT);
            mPrintf("Permanent %s", lBuf.lbflags.PERMANENT ? "Yes" : "No");  
                doCR();
            break;
 
        case 'E':
            lBuf.lbflags.NETUSER = (BOOL)(!lBuf.lbflags.NETUSER);
            mPrintf("Netuser %s", lBuf.lbflags.NETUSER ? "Yes" : "No");  
                doCR();
            break;

        case 'T':
            lBuf.lbflags.PROBLEM = (BOOL)(!lBuf.lbflags.PROBLEM);
            mPrintf("Twit/Problem user %s", lBuf.lbflags.PROBLEM ? "Yes" : "No");  
                doCR();
            break;

        case 'M':
            lBuf.lbflags.NOMAIL = (BOOL)(!lBuf.lbflags.NOMAIL);
            mPrintf("Mail %s", lBuf.lbflags.NOMAIL ? "Off" : "On");  
                doCR();
            break;

        case 'V':
            lBuf.VERIFIED = (BOOL)(!lBuf.VERIFIED);
            mPrintf("Verified %s", !lBuf.VERIFIED ? "Yes" : "No");  
                doCR();
            break;
        
        case 'I':
            mPrintf("Minutes"); doCR();
            if (cfg.accounting)
            {
                lBuf.lbflags.NOACCOUNT = 
                    getYesNo("Disable user's accounting", 
                        (BOOL)lBuf.lbflags.NOACCOUNT);
    
                if (!lBuf.lbflags.NOACCOUNT)
                {
                    lBuf.credits = (float)
                        getNumber("minutes in account", (long)0,
                        (long)cfg.maxbalance, (long)lBuf.credits);
                }
            }
            else 
            {
                doCR();
                mPrintf("Accounting turned off for system.");
            }
            break;

        case 'R':
            mPrintf("Reset all new message pointers"); doCR();
            for (c=0; c<MAXVISIT; c++)
            {
                lBuf.lbvisit[c] = cfg.oldest;
            }
            break;
            
        case 'S':
            mPrintf("Save"); doCR();
            if (getYesNo("Save changes", 0))
            {
                quit = TRUE;
            }
            break;

        case 'A':
            mPrintf("Abort"); doCR();
            if (getYesNo("Abort changes", 1))
            {
                return;
            }
            break;

        case '\r':
        case '\n':
        case '?':
            mPrintf("Menu"); doCR();
            prtMess = TRUE;
            break;

        case 'H':
            mPrintf("Default Hallway"); doCR();
        lBuf.LOCKHALL = getYesNo("Lock default hallway", (uchar)lBuf.LOCKHALL);
            break;
            
        default:
            mPrintf("%c ? for help", c); doCR();
            break;
        }

    } while (!quit);

    /* trap it */
    sprintf(string, "%s has:", who);
    if (lBuf.lbflags.SYSOP)     strcat(string, " Sysop:");
    if (lBuf.lbflags.AIDE)      strcat(string, " Aide:");
    if (lBuf.lbflags.NODE)      strcat(string, " Node:");
    if (cfg.accounting)
    {
        if (lBuf.lbflags.NOACCOUNT)
        {
            strcat(string, " No Accounting:");
        }
        else
        {
            sprintf(temp, " %.0f minutes:", lBuf.credits);
            strcat(string, temp);
        }
    }

    if (lBuf.lbflags.PERMANENT) strcat(string, " Permanent:");
    if (lBuf.lbflags.NETUSER)   strcat(string, " Net User:");
    if (lBuf.lbflags.PROBLEM)   strcat(string, " Problem User:");
    if (lBuf.lbflags.NOMAIL)    strcat(string, " No Mail:");
    if (lBuf.VERIFIED)          strcat(string, " Un-Verified:");
    
    trap(string, T_SYSOP);

    /* see if it is us: */
    if (loggedIn  &&  editSelf)
    {
        /* move it back */
        logBuf = lBuf;

        /* make our environment match */
        setsysconfig();
    }
            
    putLog(&lBuf, logNo);
}


