/************************************************************************/
/*                              ctdl.c                                  */
/*              Command-interpreter code for Citadel                    */
/************************************************************************/

#define MAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include <stdarg.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* unsigned _stklen    = 1024*12; */         /* set up our stackspace */
/* unsigned _ovrbuffer = 0x2E00 / 0x0F; */   /* 0x2D7A last check, config.c */

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      doRegular()             fanout for above commands               */
/*      getCommand()            prints prompt and gets command char     */
/*      main()                  has the central menu code               */
/************************************************************************/

/************************************************************************/
/*              External function definitions for CTDL.C                */
/************************************************************************/
/* static  char doRegular(char x,char c); */
/* static  char getCommand(char *c); */
extern  void main(int argc,char **argv);
/* static  void parseArgs(int argc, char *argv[]); */
static  int  online(void);
static  int  waitEvent(void);
/* static  void listEvents(void); */

/* -------------------------------------------------------------------- */
/*  main()          Call init, general system loop, then shutdown       */
/* -------------------------------------------------------------------- */
void main(int argc, char *argv[])
{
    int e;
    
    tzset();
    
    parseArgs(argc, argv);

    initCitadel();

    do
    {
        dowhat = WAITCALL;
        e = waitEvent();
        dowhat = DUNO;
        
        cls();
        update25();
        
        switch(e)
        {
        case E_IDLE:    /* handled in wait event */
            break;
        
        case E_CARRIER:
        case E_LOCAL:
            while (online())
                ;
            if (slv_slave)
                e = E_SHUTDOWN;
            break;
            
        case E_SYSOP:
            setdefaultTerm(2); /* Ansi-CLR */
            doSysop();
            setdefaultTerm(0);  /* TTY */
            break;
            
        case E_SHELL:
            shellescape(0);
            break;
        
        case E_SWAPSHELL:
            shellescape(1);
            break;
        
        default:
            break;
        }
        
    }
    while (e != E_SHUTDOWN && !ExitToMsdos);
    
    exitcitadel();
}

/* -------------------------------------------------------------------- */
/*  doRegular()         High level command menu.                        */
/* -------------------------------------------------------------------- */
char doRegular(char x, char c)
{
    char toReturn;

    toReturn = FALSE;
    outFlag = IMPERVIOUS;
    
    switch (toupper(c))
    {

    case 'S':
        if (sysop && x)
        {
            mPrintf("\bSysop Menu");
            doCR();
            doSysop();
        } 
        else 
        {
            toReturn=TRUE;
        }
        break;

    case 'A':
        if (aide && x)
        {
            doAide(x, 'E');
        } else {
            doEnter(FALSE, 'a');
        }
        break;

/* This little gem lets me squeeze the new contents of a room into a file */
/* in Cit format, so I can analyze the contents with another file.        */

#ifdef SQUEEZE
    case 'V': NewRoom (thisRoom, "ROOM.ALL");     break;
#endif

    case 'B': doGoto(x, TRUE);                    break;
    case 'C': doChat(x, '\0');                    break;
    case 'D': doDownload(x);                      break;
    case 'E': doEnter(x, 'm');                    break;
    case 'F': doRead(x, 'f');                     break;
    case 'G': doGoto(x, FALSE);                   break;
    case 'H': doHelp(x);                          break;
    case 'I': doIntro();                          break;
    case 'J': unGotoRoom();                       break;
    case 'K': doKnown(x, 'r');                    break;
/*  case 'L': doList(x, 'i');                     break; */
    case 'M': doEnter(FALSE, 'e');                break;
    case 'N': doRead(x, 'n');                     break;
    case 'O': doRead(x, 'o');                     break;
    case 'Q': mPrintf ("Quack!") ; doCR() ;       break;
    case 'R': doRead(x, 'r');                     break;
    case 'T': doLogout(x, 'q');                   break;
    case 'U': doUpload(x);                        break;
    case 'X':
        if (!x)
        {
            doEnter( x, 'x' );
        }else{
            doXpert();
        }
        break;

    case '=':
    case '+': doNext()         ;                    break;
    case '-': doPrevious()     ;                    break;

    case ']':
    case '>': doNextHall()     ;                    break;
    case '[':
    case '<': doPreviousHall() ;                    break;

    case ';': doSmallChat()    ;                    break;
    
    case '?':
        oChar('?');
        if (!x)
        {
            tutorial("mainopt.mnu");
        }
        else
        {
            tutorial("maindot.mnu");
        }
        break;
    
    case 0:
        break;  /* irrelevant value */
        
    default:
        toReturn = (char)!execDoor(c);
        if (toReturn) oChar(c);
        break;
    }
    
    return toReturn;
}

/* -------------------------------------------------------------------- */
/*  getCommand()    prints menu prompt and gets command char            */
/*      Returns: char via parameter and expand flag as value  --        */
/*               i.e., TRUE if parameters follow else FALSE.            */
/* -------------------------------------------------------------------- */
char getCommand(char *c)
{
    char expand;

    /*
     * Carrier loss!
     */
    if (!CARRIER)
    {
        *c = 0;
        return 0;
    }
    
    outFlag = IMPERVIOUS;

    /* update user's balance */
    if( cfg.accounting && !logBuf.lbflags.NOACCOUNT )
    {
        updatebalance();
    }

#ifdef DRAGON
    dragonAct();    /* user abuse rutine :-) */
#endif

    if (cfg.borders)
    {
        doBorder();
    }

    givePrompt();

    dowhat = MAINMENU;
    do
    {
        *c = (char)iCharNE();
    }
    while( ((char)toupper(*c)) == 'P'); 

    expand  = (char)
              ( (*c == ' ') || (*c == '.') || (*c == ',') || (*c == '/') );

    if (expand)
    {
        mPrintf("%c", *c);
        *c = (char)iCharNE();
    }
    dowhat = DUNO;

    return expand;
}



/* -------------------------------------------------------------------- */
/*  parseArgs()     sets global flags baised on command line            */
/* -------------------------------------------------------------------- */
void parseArgs(int argc, char *argv[])
{
    int i, i2;
    long l;
    
    cfg.bios = 1;
    cfg.attr = 7;   /* logo gets white letters */
    
    for(i = 1; i < argc; i++)
    {
        if (   argv[i][0] == '/'
            || argv[i][0] == '-')
        {
            switch(toupper((int)argv[i][1]))
            {
            case 'D':
                cfg.bios = 0;
                break;
                
            case 'B':
                l = atol(argv[i]+2);
                for (i2 = 0; i2<7; i2++)
                {
                    if (l == bauds[i2])
                        slv_baud = i2;
                }
                /* fall into */
                
            case 'S':
                slv_slave = TRUE;
                start_p = FALSE;
                break;
            
            case 'L':
                slv_local = TRUE;
                slv_slave = TRUE;
                start_p = FALSE;
                break;
                
            case 'A':
                slv_door = TRUE;
                break;
                
            case 'C':
                unlink("etc.dat");
                break;

            case 'E':
                readconfigcit = TRUE;
                break;

            case 'N':
                if (toupper(argv[i][2]) == 'B')
                {
                    cfg.noBells = TRUE;
                }
                if (toupper(argv[i][2]) == 'C')
                {
                    cfg.noChat = TRUE;
                }
                break;

            case 'M':
                conMode = atoi(argv[i]+2);
                break;
                
            case '*':
                start_p = FALSE;
                break;
                
            case '!':
                slv_net = TRUE;
                start_p = FALSE;
                if (argv[i][2])
                    strcpy(slv_node, argv[i]+2);
                else
                {
                    i++;
                    strcpy(slv_node, argv[i]);
                }
                break;

            case 'P':               /* log in user */
                login_pw = TRUE;
                if (argv[i][2])
                    strcpy(cmd_login, argv[i]+2);
                else
                {
                    i++;
                    strcpy(cmd_login, argv[i]);
                }
                break;

            case 'U':               /* log in user with password */
                login_user = TRUE;
                if (argv[i][2])
                    strcpy(cmd_login, argv[i]+2);
                else
                {
                    i++;
                    strcpy(cmd_login, argv[i]);
                }
                break;
               
            default:
                cPrintf("\nUnknown command line switch '%s'.\n", argv[i]);
            case  '?':    
            case  'H':    
                usage();
                exit(200);
            }
        }
    }
}

/* -------------------------------------------------------------------- */
/*  postEvent()     Add an even to the list of the last x events..      */
/* -------------------------------------------------------------------- */
void postEvent(char *fmt, ... )
{
    char buff[100];
    int  i;
    
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);
    
    for (i=elistlen-1; i>0; i--)
    {
        eventlist[i] = eventlist[i-1];
    }

    strcpy(eventlist[0].text, buff);
    eventlist[0].time = time(NULL);
}


/* -------------------------------------------------------------------- */
/*  listEvents()    List all the events to the screen.......            */
/* -------------------------------------------------------------------- */
void listEvents(void)
{
    int i;
    char buff[20];
    
    cPrintf("%8s%-50s Time", "", "Event");    doccr();
    
    for (i=0; i< /* elistlen */ scrollpos-14; i++)
    {
        if (eventlist[i].time)
        {
            strftime(buff, 19, "%I:%M%p, %b %d", eventlist[i].time);
            cPrintf("%8s%-50s %s", "",eventlist[i].text, buff);
        }
        doccr();
    }
}

/* -------------------------------------------------------------------- */
/*  waitEvent()     wait for some sort of event to happen.              */
/* -------------------------------------------------------------------- */
static  int  waitEvent(void)
{
    time_t  started;
    char    c;
    
    /*
     * Set system to a known state..
     */
    echo      = BOTH;
    onConsole = TRUE;
    whichIO   = CONSOLE;
    outFlag   = IMPERVIOUS;
    modStat   = FALSE;
    
    setio(whichIO, echo, outFlag);

#ifdef NETWORK
    if (slv_net)
    {
        cls();
        
        doccr();
        ansiattr = cfg.cattr;
        cCPrintf("Network with: %s", slv_node);
        ansiattr = cfg.attr;
        
        if (net_callout(slv_node))
        {
            did_net(slv_node);
        }
        else
        {
            return_code = 1;
        }
        
        return E_SHUTDOWN;
    }
#endif
    
    /* Handle command line log-in */
    if ((login_user || login_pw) && !slv_slave)
    {
        if (!gotCarrier())
        {

            onConsole = TRUE;
            whichIO   = CONSOLE;
            return E_LOCAL;
        }
        else
        {
            if (slv_baud != -1)
                baud(slv_baud);
            else
                baud(cfg.initbaud);
            
            carrdetect();
                
            onConsole   = FALSE;
            whichIO     = MODEM;
            return E_CARRIER;
        }
    }


    if (slv_slave) 
    {
        if (slv_local)
        {
            onConsole = TRUE;
            whichIO   = CONSOLE;
            return E_LOCAL;
        }
        else
        {
            if (slv_baud != -1)
                baud(slv_baud);
            else
                baud(cfg.initbaud);
            
            carrdetect();
                
            onConsole   = FALSE;
            whichIO     = MODEM;
            return E_CARRIER;
        }
    }
    
    if (sysReq == TRUE)
    {
        sysReq=FALSE;
        if (cfg.offhook)
        {
            offhook();
        }
        else
        {
            drop_dtr();
        }
        ringSystemREQ();
    }
    
    /*
     * Waiting for call screen.. 
     */
    logo();
    update25();
    listEvents();
    ansiattr = cfg.wattr;
    doccr();
    if (disabled)
    {
        cPrintf("Enabling modem...");
        Initport();
    }
    cPrintf
    (
        "\r== Waiting for Call or L)ogin, E)vent,"
        " eX)it, S)ysop, !) Shell, @) Swap shell ==\r"
    );
    update25();
    ansiattr = cfg.attr;
    
    started = time(NULL);
    
    for(;;)   /* while (TRUE) */
    {
        if (ExitToMsdos)
            return E_SHUTDOWN;
        
        if (sysopkey)    
            return E_SYSOP;
        
        if (eventkey)    
        {
            eventkey = FALSE;
            
            if (do_cron(CRON_TIMEOUT))
            {
                return E_IDLE;
            }
            else
            {
                started = time(NULL);
            }
        }
            
        if (KBReady())
        {
            c = (char)ciChar();
            switch(toupper(c))
            {
            case 'L':
                if (cfg.offhook)
                {
                    offhook();
                }
                return E_LOCAL;
            
            case 'E':
                if (do_cron(CRON_TIMEOUT))
                {
                    return E_IDLE;
                }
                else
                {
                    started = time(NULL);
                }
                break;
                
            case 'X':
                return E_SHUTDOWN;
            
            case 'S':
                return E_SYSOP;
            
            case '!':
                return E_SHELL;
            
            case '@':
                return E_SWAPSHELL;
                
            default:
                break;
            }
        }
        
        if ( (int)((time(NULL) - started)/(time_t)60) >= cfg.idle )
            if (do_cron(CRON_TIMEOUT))
            {
                return E_IDLE;
            }
            else
            {
                started = time(NULL);
            }
            
        if (carrier())
        {
            onConsole   = FALSE;
            whichIO     = MODEM;
            return E_CARRIER;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  online()        deal with an entire online session                  */
/* -------------------------------------------------------------------- */
static  int  online(void)
{
    int  trys = 0;
    char c,x;
    
    Mflush();
    greeting();

/*
 *  doCR();
 */

    doCR();

    do
    {
        login(); /* login modified to handle command line login */
    }
    while (!loggedIn && CARRIER && trys++<3);
    
    if (!CARRIER)
    {
        carrloss();
        return FALSE;
    }
    
    if (!loggedIn)
    {
        if (!onConsole)
            Hangup();
        return FALSE;
    }
    
#ifdef NETWORK
    if (logBuf.lbflags.NODE)
    {
        dowhat = NETWORKING;
        callout = TRUE;
        if (net_slave())
            postEvent("Net-Callin by %s, %d new", deansi(logBuf.lbname), entered);
        else
            postEvent("Net-Callin by %s, failed", deansi(logBuf.lbname), entered);
        callout = FALSE;
            
        dowhat = DUNO;

        pause(200);
        
        carrloss();
      
        cfg.callno++;
        terminate(FALSE, FALSE);
        
        return FALSE;
    }
#endif

    if (loggedIn)
    {
        if (onConsole)
            postEvent("Console login by %s", deansi(logBuf.lbname));
        else    
           postEvent("%5dbd login by %s", bauds[speed], deansi(logBuf.lbname));
    }

    while (CARRIER && loggedIn && !ExitToMsdos)
    {
        outFlag = IMPERVIOUS;
        update25();
        
        if (logBuf.VERIFIED)
        {
            terminate(TRUE, FALSE);
        }
        
        x = getCommand(&c);
        
        if (!CARRIER)
            break;
        
        if (chatkey)  chat();
        if (sysopkey) doSysop();
        
        if (c)
        {
            if (doRegular(x, c)) 
            {
                if (!expert)  mPrintf("\n '?' for menu, 'H' for help.\n \n" );
                else          mPrintf(" ?\n \n" );
            }
        }
    }
    
    if (loggedIn)
    {
        carrloss();
        terminate(FALSE, FALSE);
    }
    
    if (CARRIER && !ExitToMsdos)
        return TRUE;
    else
        return FALSE;
}
