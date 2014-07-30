/* -------------------------------------------------------------------- */
/*  MISC.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                        Overlayed misc stuff                          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#define MISC2
#include <malloc.h>
#include <bios.h>
#include <string.h>
#include <time.h>
#include <dos.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  systat()        System status                                       */
/*  ringSystemREQ() signals a system request for 2 minutes.             */
/*  dial_out()      dial out to other boards                            */
/*  logo()          prints out logo screen and quote at start-up        */
/*  greeting()      System-entry blurb etc                              */
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
/*  ringSystemREQ() signals a system request for 2 minutes.             */
/* -------------------------------------------------------------------- */
void ringSystemREQ(void)
{
    unsigned char row, col;
    char i;
    char answered = FALSE;
    char ringlimit = 120;

    doccr();
    doccr();
    readpos( &row, &col);
    (*stringattr)(row," * System Available * ",(uchar)(cfg.wattr | 128));
    update25();
    doccr();

    Hangup();
    
    answered = FALSE;
    for (i = 0; (i < ringlimit) && !answered; i++)
    {
        outCon(7 /* BELL */);
        pause(80);
        if (KBReady()) answered = TRUE;
    }

    if (!KBReady() && i >= ringlimit)  Initport();

    update25();
}

/* -------------------------------------------------------------------- */
/*  dial_out()      dial out to other boards                            */
/* -------------------------------------------------------------------- */
void dial_out(void)
{
    char con, mod;
    char fname[61];

    fname[0] = 60;

#ifdef GOODBYE
    outFlag = IMPERVIOUS;
    mPrintf("  Now in dial out mode, Control-Q to exit.\n\n ");
    outFlag = OUTOK;
#endif


    outFlag = IMPERVIOUS;
    cPrintf("  Now in dial out mode, Ctrl-Q to exit, Ctrl-N to net,\n"
            " Ctrl-B for baud rate change, Ctrl-E for shell escape,\n"
            " Ctrl-U for Xmodem upload, Ctrl-D for Xmodem download.\n \n ");
    outFlag = OUTOK;

    Hangup(); 

    disabled = FALSE;
 
    baud(cfg.initbaud);

    update25();

    outstring(cfg.dialsetup); 
    outstring("\r");

    pause(100);

    callout = TRUE;

    do
    {
        con = 0;  mod = 0;

        if (KBReady())
        {
            con = (char)ciChar();
            if (debug) oChar((char)con);
            if (con != 17 && con != 2 && con != 5
                          && con != 14 && con != 4 && con != 21)
            {              
                outMod(con);
            }
            else
            {
                switch(con)
                {
                case  2:
                        mPrintf("New baud rate [0-4]: ");
                        con = (char)ciChar();
                        doccr();
                        if (con > ('0' - 1) && con < '5')
                            baud(con-48);
                        update25();
                        break;
    
                case  5:
                        if (sysop || !ConLock)
                            shellescape(FALSE);
                        break;
    
#ifdef NETWORK
                case 14:
                        readnode();
                        master();
                        slave();
                        cleanup();
                        break;
#endif
    
                case  4:
                        doccr();
                        cPrintf("Filename: ");
                        cGets(fname);
                        if(!fname[2])
                        {
                            break;
                        }
                        xreceive(fname+2,0);
                        fname[2] = '\0';
                        break;
    
                case 21:
                        doccr();
                        cPrintf("Filename: ");
                        cGets(fname);
                        if(!fname[2])
                            break;
                        xsend(fname+2,0);
                        fname[2] = '\0';
                        break;
    
                default:
                        break;
                }
            }
        }

        if (MIReady())
        {
            mod = (char)getMod();

            if (debug)
            {
              mod = (char)( mod & 0x7F );
              mod = tfilter[mod];
            }

            if (mod == '\n')
              doccr();
            else
              if (mod != '\r')
                oChar(mod);
        }
    } while (con != 17 /* ctrl-q */);
    
    callout = FALSE;

    Initport();

    if (cfg.offhook)
        offhook();

    doCR();
}

/*----------------------------------------------------------------------*/
/*      logo()  prints out system logo at startup                       */
/*----------------------------------------------------------------------*/
void logo()
{
    int i;

#ifdef GOODBYE
    for (i=0; welcome[i]; i++)
        if(welcomecrcs[i] != stringcrc(welcome[i]))
            crashout("Some ASSHOLE tampered with the welcome message!");

    for (i=0; copyright[i]; i++)
        if(copycrcs[i] != stringcrc(copyright[i]))
            crashout("Some ASSHOLE tampered with the Copyright message!");
#endif

    cls();

    cCPrintf("%s v%s", programName, version);  doccr();
    
    if (strlen(testsite))
    {
        cCPrintf(testsite);
    }
    doccr();
    
    cCPrintf("Host is IBM, (%s)", compilerName); doccr();
    doccr();

    for (i=0; welcome[i] && i<3; i++)
    {
        cCPrintf(welcome[i]);
        doccr();
    }
    doccr();

    for (i=0; copyright[i]; i++)
    {
        cCPrintf(copyright[i]);
        doccr();
    }
    doccr();
}

/************************************************************************/
/*      greeting() gives system-entry blurb etc                         */
/************************************************************************/
void greeting()
{
    char dtstr[80];

    echo  =  BOTH;

    setdefaultconfig();
    initroomgen();
    cleargroupgen();
    
    if (cfg.accounting) unlogthisAccount();
    
    pause(10);
    
    cls();
    
    doccr();
    
    update25();

    if (CARRIER)  hello();

    outFlag = OUTOK;
    
    doCR();
    mPrintf("Welcome to %s, %s, %s", 
            cfg.nodeTitle, cfg.nodeRegion, cfg.nodeContry);
    doCR();
    mPrintf("Running %s Version %s (%s)", programName, version, compilerName);
    if (strlen(testsite))
    {
        doCR();
        mPrintf("%s", testsite);
    }
    doCR();
    doCR();

    strftime(dtstr, 79, cfg.vdatestamp, 0l);
    mPrintf(" %s", dtstr);

    getRoom(LOBBY);

    doCR();
}

/* -------------------------------------------------------------------- */
/*  systat()        System status                                       */
/* -------------------------------------------------------------------- */
void systat(BOOL verbose)
{
    union REGS r;
    int i;
    long average, work;
    char dtstr[80];
    int  public    = 0,     /* talleys.. */
         active    = 0,
         directory = 0,
         shared    = 0,
         private   = 0,
         anon      = 0,
         group     = 0,
         problem   = 0,
         perm      = 0,
         aides     = 0,
         sysops    = 0,
         nodes     = 0,
         moderated = 0,
         networked = 0;

    outFlag = OUTOK;

    
    /*
     * On...
     */
    doCR();
    mPrintf(" 3You are on:0\n %s, %s, %s", 
            cfg.nodeTitle, cfg.nodeRegion, cfg.nodeContry); doCR();
    
    doCR();
    mPrintf(" 3Running:0\n %s V%s", programName, version); doCR();
    if (verbose)
    {
        mPrintf(" Compiled on %s at %s", cmpDate, cmpTime); doCR();
        mPrintf(" %s", copyright[0]);                       doCR();
        mPrintf(" %s", copyright[1]);                       doCR();
    }
    
    
    /*
     * times.. 
     */
    doCR();
    strftime(dtstr, 79, cfg.vdatestamp, 0l);
    mPrintf(" 3It is:    0%s", dtstr); doCR();
    if (verbose)
    {
        mPrintf(" 3Up for:  0"); diffstamp(uptimestamp); doCR();
    }
    if (gotCarrier())
    {
        mPrintf(" 3Online:  0"); diffstamp(conntimestamp); doCR();
    }
    mPrintf(" 3Loggedin:0");  diffstamp(logtimestamp); doCR();

    
    /*
     * Userlog info.. 
     */
    doCR();
    mPrintf(" 3Userlog stats:0"); doCR();
    mPrintf(" %d log entries", cfg.MAXLOGTAB); doCR();
    active = aides = sysops = problem = perm = nodes = 0;
    for (i=0; i < cfg.MAXLOGTAB; i++)
    {
        if (logTab[i].ltflags.L_INUSE)
        {
            active++;
            if (logTab[i].ltflags.AIDE)         aides++;
            if (logTab[i].ltflags.SYSOP)        sysops++;
            if (logTab[i].ltflags.PROBLEM)      problem++;
            if (logTab[i].ltflags.PERMANENT)    perm++;
            if (logTab[i].ltflags.NODE)         nodes++;
        }
    }
    mPrintf(" %d active", active); doCR();
    mPrintf(" %s calls", ltoac(cfg.callno));   doCR();
    if (verbose)
    {
        mPrintf(" %d nodes", nodes);
        if (aide || sysop)
            mPrintf(", %d aides, %d sysops, %d problem users, and %d permanent",
                    aides, sysops, problem, perm);  doCR();
    }
    
    /*
     * Room file info.. 
     */
    doCR();
    mPrintf(" 3Room stats:0");            doCR();
    mPrintf(" %d rooms", MAXROOMS);         doCR();
                                            
    active = public = private = directory = 
    group = shared = moderated = anon = 0;
    
    for (i=0; i<MAXROOMS; i++)
    {
        if (roomTab[i].rtflags.INUSE)
        {
            active++;
            if (roomTab[i].rtflags.PUBLIC)        public++;
            else                                    private++;
            if (roomTab[i].rtflags.MSDOSDIR)      directory++;
            if (roomTab[i].rtflags.GROUPONLY)     group++;
            if (roomTab[i].rtflags.SHARED)        shared++;
            if (roomTab[i].rtflags.MODERATED)     moderated++;
            if (roomTab[i].rtflags.ANON)          anon++;
        }
    }
    
    mPrintf(" %d active", active);          doCR(); 
    
    if (verbose)
    {
        mPrintf(" %d public, %d private, %d directory,"
                " %d moderated, %d group only, %d networked, and %d anon.", 
                public, private, directory, moderated, group, shared, anon);
        doCR();
    
    }
   
    /*
     * Message(s) status.. 
     */
    doCR();
    strcpy(dtstr, cfg.msg_nym);
    dtstr[0] = (char)toupper(dtstr[0]);
    mPrintf(" 3%s status:0", dtstr); doCR();
    
    /* stop before the message count if already aborted.. */
    if (outFlag != OUTOK)
        return;
        
    if (verbose)
    {
        public = private = group = moderated = problem = 0;
        
        for( i = 0; i < (int)sizetable(); ++i)
        {
                 if (msgTab1[i].mtmsgflags.PROBLEM  ) problem++;
                 if (msgTab1[i].mtmsgflags.MODERATED) moderated++;
                 if (msgTab1[i].mtmsgflags.LIMITED  ) group++  ;
           else  if (msgTab1[i].mtmsgflags.MAIL     ) private++;
           else                                       public++ ;

                 /* Bug: this will not count origin message # 65536 */
                 /* does anyone really care? */
                 /* should probably add a network bit in the message */
                 /* attribute byte. */
                 if (msgTab8[i].mtomesg)              networked++;
        }
    }
    
    mPrintf(" %s entered", ltoac(cfg.newest)); doCR();
    mPrintf(" %s online, #%lu to %lu",
    ltoac(cfg.newest - cfg.oldest + 1), cfg.oldest, cfg.newest); 
     doCR();

    if ( (cfg.mtoldest - cfg.oldest) > 0 && aide)
    {
        mPrintf(" %d missing", (int)(cfg.mtoldest - cfg.oldest)); doCR();
    }

    if (verbose)
    {
        mPrintf(" %d public, %d networked, %d private, %d moderated, ", 
                public, networked, private, moderated);
    
        if (!aide) mPrintf("and ");
    
        mPrintf("%d group only", group);
    
        if (aide)  mPrintf(", and %d problem user", problem, cfg.msgs_nym);
        doCR();
    
        mPrintf(" %dK %s space", cfg.messagek, cfg.msg_nym); doCR();
    }

    if (cfg.oldest > 1)  work = (long)((long)cfg.messagek * 1024l);
    else                 work = cfg.catLoc;

    if (cfg.oldest > 1)  average = (work) / (cfg.newest - cfg.oldest + 1);
    else                 average = (work) / (cfg.newest);

    mPrintf(" %s bytes average %s length", ltoac(average), cfg.msg_nym); doCR();

    
    /*
     * System and debugging.. 
     */
    if (sysop && verbose)
    {
        r.h.ah = 0x48;   /* allocate memory              */
        r.h.al = 0;
        r.x.bx = 0xffff; /* ask for an impossible amount */

        intdos(&r, &r);

        doCR();
        mPrintf(" 3System status / debugging:0"); doCR();
        mPrintf(" Host system is an IBM or compatible.");  doCR();
        mPrintf(" DOS version %d.%d", _osmajor, _osminor); doCR();
#ifdef ALPHA_TEST        
     /* mPrintf(" %u bytes stack space", _stklen );        doCR(); */
        mPrintf(" %u bytes stack space", stackavail() );        doCR();
#endif
        mPrintf(" %uK system memory", _bios_memsize());       doCR();
        mPrintf(" %ld bytes free system memory", (long)r.x.bx*16L); doCR();
/* mPrintf(" %s bytes free system memory", ltoac(farcoreleft()) ); doCR(); */
        strcpy(dtstr, ltoac(received));
        mPrintf(" %s characters transmitted, %s characters received",
                ltoac(transmitted), dtstr ); doCR();
    }
}


