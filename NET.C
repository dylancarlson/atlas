/* -------------------------------------------------------------------- */
/*  NET.C                    Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */
#ifdef NETWORK

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  net_slave()     network entry point from LOGIN                      */
/*  net_master()    entry point to call a node                          */
/*  slave()         Actual networking slave                             */
/*  master()        During network master code                          */
/*  n_dial()        call the bbs in the node buffer                     */
/*  n_login()       Login to the bbs with the macro in the node file    */
/*  wait_for()      wait for a string to come from the modem            */
/*  net_callout()   Entry point from CRON.C				*/
/*  cleanup()       Done with other system, save mail and messages      */
/*  get_first_room()    get the first room in the room list             */
/*  get_next_room() gets the next room in the list                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  09/17/90    (PAT)   Split NET.C into smaller peices for overlays    */
/*  06/05/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code.                                      */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  net_slave()     network entry point from LOGIN                      */
/* -------------------------------------------------------------------- */
BOOL net_slave(void)
{
    if (readnode() == FALSE)
    {
        cPrintf("\n No node.cit entry!\n ");
        return FALSE;
    }

    if (!onConsole) 
    {
        netError = FALSE;
        
        /* cleanup */

	changedir(cfg.temppath);
	ambigUnlink("room.*",	FALSE);
        ambigUnlink("roomin.*", FALSE);
        unlink(roomreqin);
        unlink(roomreqout);
        unlink(roomdatain);
        unlink(roomdataout);
        unlink("mailin.tmp");
	unlink("mailout.tmp");
        
        switch (node.network)
        {
        default:
        case NET_DCIT10:
            slave();
            if (master())
            {
                cleanup();
                did_net(node.ndname);
            }
            else
            {
                return FALSE;            
            }
            break;
        
        case NET_DCIT15:
        case NET_DCIT16:
            if (dc15network(FALSE))
            {
                cleanup();
                did_net(node.ndname);
            }
            else
            {
                return FALSE;            
            }
            break;
        
#ifdef HENGE        
        case NET_HENGE:
            if (!hengeNet(FALSE)) return FALSE;
            break;
#endif        
        }
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  net_master()    entry point to call a node                          */
/* -------------------------------------------------------------------- */
BOOL net_master(void)
{
    if (readnode() == FALSE)
    {
        cPrintf("\n No node.cit entry!\n ");
        return FALSE;
    }
  
    if (n_dial()  == FALSE) return FALSE;
    if (n_login() == FALSE) return FALSE;
    netError = FALSE;

    /* cleanup */
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);
    unlink(roomreqin);
    unlink(roomreqout);
    unlink(roomdatain);
    unlink(roomdataout);
    unlink("mailin.tmp");
    unlink("mailout.tmp");

    switch (node.network)
    {
    default:
    case NET_DCIT10:
        if (master()  == FALSE) return FALSE;
        if (slave()   == FALSE) return FALSE;
        break;
    
    case NET_DCIT15:
    case NET_DCIT16:
        if (dc15network(TRUE) == FALSE) 
            return FALSE;
        break;
    
#ifdef HENGE        
    case NET_HENGE:
        return hengeNet(TRUE);
#endif    
    }
    
    cleanup();

    Initport();
    
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  slave()         Actual networking slave                             */
/* -------------------------------------------------------------------- */
BOOL slave(void)
{
    char line[100];
    label troo;
    label fn;
    FILE *file;
    int i = 0, rm;
    
    cPrintf(" Sending mail file.");
    doccr();

    sprintf(line, "%s\\%s", cfg.transpath, node.ndmailtmp);
    wxsnd(cfg.temppath, line, 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
  
    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving room request file.");
    doccr();
    
    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    unlink(line);
    
    wxrcv(cfg.temppath, "roomreq.tmp", 
            (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    if (!gotCarrier()) return FALSE;
    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    if ((file = fopen(line, "rb")) == NULL)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    doccr();
    cPrintf(" Fetching:");
    doccr();

    GetStr(file, troo, LABELSIZE);
    
    while(strlen(troo) && !feof(file))
    {
        if ((rm = roomExists(troo)) != ERROR)
        {
	    if (nodecanseeroom(node.ndname, rm))
            {
                sprintf(fn, "room.%d", i);
                cPrintf(" %-20.20s  ", troo);
                if( !((i+1) % 3) ) doccr();
                NewRoom(rm, fn);
            }
            else
            {
                doccr();
                cPrintf(" No access to %s room.", troo);
                doccr();
                amPrintf(" '%s' room not available to remote.\n", troo);
                netError = TRUE;
            }
        }
        else
        {
            doccr();
            cPrintf(" No %s room on system.", troo);
            doccr();
            amPrintf(" '%s' room not found for remote.\n", troo);
            netError = TRUE;
        }

        i++;
        GetStr(file, troo, LABELSIZE);
    }
    doccr();
    fclose(file);
    unlink(line);

    cPrintf(" Sending message files.");
    doccr();
  
    if (!gotCarrier()) return FALSE;
    wxsnd(cfg.temppath, "room.*",
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    ambigUnlink("room.*",   FALSE);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  master()        During network master code                          */
/* -------------------------------------------------------------------- */
BOOL master(void)
{
    char line[100], line2[100];
    label here, there;
    FILE *file, *fopen();
    int i, rms;
    time_t t;

    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving mail file.");
    doccr();

    wxrcv(cfg.temppath, "mailin.tmp", 
          (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\roomreq.tmp", cfg.temppath);
    if((file = fopen(line, "wb")) == NULL)
    {
        perror("Error opening roomreq.tmp");
        return FALSE;
    }

    for (i=get_first_room(here, there), rms=0;
         i;
         i=get_next_room(here, there), rms++)
    {
        PutStr(file, there);
    }

    PutStr(file, "");
    fclose(file);

    cPrintf(" Sending room request file.");
    doccr();

    wxsnd(cfg.temppath, "roomreq.tmp", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    unlink(line);

    if (!gotCarrier()) return FALSE;

    /* clear the buffer */
    while (gotCarrier() && MIReady())
    {
        getMod();
    }
    
    cPrintf(" Waiting for transfer.");
    /* wait for them to get thier shit together */
    t = time(NULL);
    while (gotCarrier() && !MIReady())
    {
        if (time(NULL) > (t + (15 * 60))) /* only wait 15 minutes */
        {
            drop_dtr();
        }
    }
    doccr();

    if (!gotCarrier()) return FALSE;

    cPrintf(" Receiving message files.");
    doccr();

    wxrcv(cfg.temppath, "", 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    for (i=0; i<rms; i++)
    {
        sprintf(line,  "room.%d",   i);
        sprintf(line2, "roomin.%d", i);
        rename(line, line2);
    }
    
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  n_login()       Login to the bbs with the macro in the node file    */
/* -------------------------------------------------------------------- */
BOOL n_login(void)
{
#ifdef GOODBYE
    union REGS in, out;
    register int ptime, j=0, k;
#endif
    char ch;
    int i, count;
    char *words[256];
    long j, maxtime;

    cPrintf("\n Logging in...");

    count = parse_it( words, node.ndlogin);

    i = 1;

    while(i <= count)
    {
        switch(tolower(*words[i++]))
        {     
            case 'p':
		if (debug) cPrintf("Pause For (%s)", words[i]);
#ifdef GOODBYE
		ptime=atoi(words[i++]);
                in.h.ah=0x2C;
                intdos(&in, &out);
                k = out.h.dl/10;
                while(j < ptime)
                {
                    in.h.ah=0x2C;
                    intdos(&in, &out);
                    if(out.h.dl/10 < k)
                        j += (10+(out.h.dl/10))-k;
                    else
                        j += (out.h.dl/10)-k;
                    k = out.h.dl/10;
                    if (MIReady())
                    {
                        ch = (char)getMod();
                        if (debug) outCon(ch);
                    }
		}
#endif
		maxtime = atol(words[i++]);
		maxtime += (j = time(&j));
		while (j < maxtime)
		    j = time(&j);
		break;
	    case 's':
		if (debug) cPrintf("Sending (%s)", words[i]);
                outstring(words[i++]);
                break;
            case 'w':
                if (debug) cPrintf("Wait For (%s)", words[i]);
                if (!wait_for(words[i++]))
                {
                    cPrintf("failed");
                    return FALSE;
                }
                break;
            case '!':
                apsystem(words[i++]);
                break;
            default:
                break;
        }
    }
    cPrintf("success");
    doccr();
    doccr();
    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  wait_for()      wait for a string to come from the modem            */
/* -------------------------------------------------------------------- */
BOOL wait_for(char *str)
{
    char line[80];
    long st;
    int i, stl;
   
    stl = strlen(str);

    for (i=0; i<stl; i++) 
        line[i] = '\0';

    time(&st);
   
    for(;;) /*while(TRUE)*/
    {
        if (MIReady())
        {
            memcpy(line, line+1, stl);
            line[stl-1]  = (char) getMod();
            line[stl] = '\0';
            if (debug) outCon(line[stl-1]);
            if (strcmpi(line, str) == SAMESTRING) 
                return TRUE;
        }
        else
        {
            if ((time(NULL) - st) > (long)node.ndwaitto || !gotCarrier())
                return FALSE;
            if (KBReady())                             /* Aborted by user */
            {
                ciChar();
                return FALSE;
            }
        }
    }   
}   

/* -------------------------------------------------------------------- */
/*  net_callout()   Entry point from Cron.C                             */
/* -------------------------------------------------------------------- */
BOOL net_callout(char *node)
{
    int slot;
    int tmp;

    /* login user */

    mread = 0; entered = 0;

    slot = personexists(node);

    if (slot == ERROR)
    {
        cPrintf("\n No such node in userlog!");
        return FALSE;
    }

    getLog(&logBuf, logTab[slot].ltlogSlot);

    thisSlot = slot;
    thisLog = logTab[slot].ltlogSlot;
 
    callout     = TRUE;
    loggedIn    = TRUE;
    setsysconfig();
    setgroupgen();
    setroomgen();
    setlbvisit();

    update25();

    sprintf( msgBuf->mbtext, "NetCallout %s", logBuf.lbname);
    trap(msgBuf->mbtext, T_NETWORK);

    /* node logged in */
     
    tmp = net_master();

    /* terminate user */

    if (tmp == TRUE)
    { 
        logBuf.callno      = cfg.callno;
        time(&logtimestamp);
        logBuf.calltime    = logtimestamp;
        logBuf.lbvisit[0]  = cfg.newest;
        logTab[0].ltcallno = cfg.callno;

        slideLTab(thisSlot);
        cfg.callno++;

        storeLog();
        loggedIn = FALSE;

        /* trap it */
        sprintf(msgBuf->mbtext, "NetLogout %s (succeded)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);

        outFlag = IMPERVIOUS;
        cPrintf("Networked with \"%s\"\n ", logBuf.lbname);

        if (cfg.accounting)  unlogthisAccount();
        heldMessage = FALSE;
        cleargroupgen();
        initroomgen();

        logBuf.lbname[0] = 0;

        setalloldrooms();
    }
    else
    {
        loggedIn = FALSE;

        sprintf(msgBuf->mbtext, "NetLogout %s (failed)", logBuf.lbname);
        trap(msgBuf->mbtext, T_NETWORK);
    }

    setdefaultconfig();

    /* user terminated */
    onConsole       = FALSE;
    callout         = FALSE;

    pause(100);
   
    Initport();

    return (BOOL)(tmp);
}

/* -------------------------------------------------------------------- */
/*  cleanup()       Done with other system, save mail and messages      */
/* -------------------------------------------------------------------- */
void cleanup(void)
{
    int t, i, rm, err;
    uint new=0, exp=0, dup=0, rms=0;
    label fn, here, there;
    char line[100];
  
    drop_dtr();
    
    sprintf(line, "%s\\%s", cfg.transpath, node.ndmailtmp);
    unlink(line);

    doccr();
    cPrintf(" Incorporating:");                                         doccr();
    cPrintf("                 Room:  New: Expired: Duplicate:");        doccr();
           /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");       doccr();
    for (t=get_first_room(here, there), i=0;
         t;
         t=get_next_room(here, there), i++)
    {
        sprintf(fn, "roomin.%d", i);

        rm = roomExists(here);
        if (rm != ERROR)
        {
            cPrintf(" %20.20s  ", here);
            err = ReadMsgFl(rm, fn, here, there);
            if (err != ERROR)
            {
                cPrintf("  %2u     %2u     %2u", err, expired, duplicate);
                new += err;
                exp += expired;
                dup += duplicate;
                rms ++;
            }
            else
            {
                amPrintf(" No '%s' room on remote\n", there);
                netError = TRUE;
                cPrintf(" Room not found on other system.");
            }
            doccr();
        }
        else
        {
            cPrintf(" %20.20s   Room not found on local system.", here);
            amPrintf(" No '%s' room local.\n", here);
            netError = TRUE;
            doccr();
        }

        unlink(fn);
    }
    cPrintf("อออออออออออออออออออออออออออออออออออออออออออออออออ");       doccr();
    cPrintf("Totals:            %2u    %2u     %2u     %2u", rms, new, exp, dup);
           /* XXXXXXXXXXXXXXXXXXXX    XX     XX     XX*/                doccr();
    doccr();
    cPrintf("Incorporating MAIL");
    i =  ReadMsgFl(MAILROOM, "mailin.tmp", "", "");
    cPrintf(
        " %d new message(s), %d routed, %d rejected, and %d total.", 
        i == ERROR ? 0 : i, 
        expired,
        duplicate,
        (i == ERROR ? 0 : i) + expired + duplicate
        );
    doccr();

    cPrintf ("Updating mail routes.") ;   doccr() ;
    updateRoutes (logBuf.lbname) ;
  
    changedir(cfg.temppath);
    ambigUnlink("room.*",   FALSE);
    ambigUnlink("roomin.*", FALSE);
    unlink("mailin.tmp");
    changedir(cfg.homepath);
  
    expired = exp;
    duplicate = dup;
    entered = new;
  
    if (netError)
    {
        amPrintf(" \n Netting with '%s'\n", logBuf.lbname );
        SaveAideMess();
    }
    
}

/* ------------------------------------------------------------------------ */
/*  the folowing two rutines are used for scaning through the rooms         */
/*  requested from a node                                                   */
/* ------------------------------------------------------------------------ */
FILE *nodefile;

/* -------------------------------------------------------------------- */
/*  get_first_room()    get the first room in the room list             */
/* -------------------------------------------------------------------- */
BOOL get_first_room(char *here, char *there)
{
    if (!node.roomoff) return FALSE;

    /* move to home-path */
    changedir(cfg.homepath);

    if ((nodefile = fopen("nodes.cit", "r")) == NULL)  /* ASCII mode */
    {  
        crashout("Can't find nodes.cit!");
    }

    changedir(cfg.temppath);

    fseek(nodefile, node.roomoff, SEEK_SET);

    return get_next_room(here, there);
}

/* -------------------------------------------------------------------- */
/*  get_next_room() gets the next room in the list                      */
/* -------------------------------------------------------------------- */
BOOL get_next_room(char *here, char *there)
{
    char  line[95];
    char *words[256];
    int  count;
   
    while (fgets(line, 90, nodefile) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        if (strcmpi(words[0], "#NODE") == SAMESTRING)
        {
            fclose(nodefile);
            return FALSE;
        }

        if (strcmpi(words[0], "#ROOM") == SAMESTRING)
        {
            strncpy(here,  words[1], LABELSIZE);

            if (count > 2) 
                strncpy(there, words[2], LABELSIZE);
            else
                strcpy(there, here);

            here[LABELSIZE]  = 0;
            there[LABELSIZE] = 0;

            return TRUE;
        }
    }
    fclose(nodefile);
    return FALSE;
}  


/* -------------------------------------------------------------------- */
/*  n_dial()        call the bbs in the node buffer                     */
/* -------------------------------------------------------------------- */
BOOL n_dial(void)
{
    time_t ts, tx, ty;
    char str[50];
    int checkbaud;

    cPrintf("\n \n Dialing");

    if (debug) cPrintf("(%s%s)", cfg.dialpref, node.ndphone);
    
    cPrintf("... 1");
    
    /* baud(node.ndbaud); */
    baud(cfg.initbaud); 
    update25();

    outstring(cfg.dialsetup);
    outstring("\r");

    pause(100);
    Mflush();   /* Clear the buffer */
    
  
    strcpy(str, cfg.dialpref);
    strcat(str, node.ndphone);
    outstring(str);
    outstring("\r");

    time(&ts);
    tx = ts;
    
    for(;;) /* while(TRUE) */
    {
        if ((int)(time(&ty) - ts) > node.nddialto)  /* Timeout */
        {
            cPrintf(" timeout ");
            return FALSE;
        }
        
        if (tx != ty)
        {
            cPrintf("\r Dialing");
            if (debug) cPrintf("(%s%s)", cfg.dialpref, node.ndphone);
            cPrintf("... %ld", ty - ts);
            tx = ty;
        }
        
        if (KBReady())                             /* Aborted by user */
        {
            ciChar();
            cPrintf(" aborted ");
            return FALSE;
        }

        if (MIReady())
        {
            checkbaud = smartbaud();

            if (checkbaud == TRUE)                 /* got carrier!  */ 
            {
                cPrintf(" success");
                return TRUE;
            }
            if (checkbaud == ERROR)
            {
                cPrintf(" failure");
                return FALSE;
            }
        }
    }
}

#endif /* NETWORK */
