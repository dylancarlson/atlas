/* -------------------------------------------------------------------- */
/*  INIT.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  INIT Citadel code                                                   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
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
/*  initCitadel()   Load up data files and open everything.             */
/*  openFile()      Special to open a .cit file                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  08/04/90    (PAT)   File created                                    */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data definitions                                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  initCitadel()   Load up data files and open everything.             */
/* -------------------------------------------------------------------- */
void initCitadel(void)
{
    static char prompt[92];
    char *envprompt;

    char *grpFile, *hallFile, *logFile, *msgFile, *roomFile;
    char scratch[80];

/**** new startup ****/    
    setscreen();

    logo();
    if (start_p)
        pause(300);
    
    if (time(NULL) < 607415813L)
    {
        cPrintf("\n\nPlease set your time and date!\n");
        exit(200);
    }

/*
    if (cfg.f6pass[0])
    {
        ConLock = TRUE;
    }
*/

    update25();

    onConsole = TRUE;
/**** end new startup ****/    
    
#ifdef TERM
    modStat = TRUE;
#endif    
    
#ifdef IBM    
    /* lets ignore ^C's  */
    signal(SIGINT, ctrl_c);
#endif
    
    asciitable();

    envprompt = getenv("PROMPT");
    sprintf(prompt, "PROMPT=%s[Cit] ", envprompt);
    putenv(prompt);

    elistlen = conRows - 15;
    
    if (
          ((lBuf2   = _fcalloc(1,          sizeof(struct logBuffer ))) == NULL)
       || ((talleyBuf=_fcalloc(1,          sizeof(struct talleyBuffer))) == NULL)
       || ((doors   = _fcalloc(MAXDOORS,   sizeof(struct ext_door  ))) == NULL)
       || ((edit    = _fcalloc(MAXEXTERN,  sizeof(struct ext_editor))) == NULL)
       || ((hallBuf = _fcalloc(1,          sizeof(struct hallBuffer))) == NULL)
       || ((extrn   = _fcalloc(MAXEXTERN,  sizeof(struct ext_prot  ))) == NULL)
       || ((roomTab = _fcalloc(MAXROOMS,   sizeof(struct rTable    ))) == NULL)
       || ((msgBuf  = _fcalloc(1,          sizeof(struct msgB      ))) == NULL)
       || ((msgBuf2 = _fcalloc(1,          sizeof(struct msgB      ))) == NULL)
       || ((eventlist=_fcalloc(elistlen,   sizeof(EVENT            ))) == NULL)
       || ((roomPos = _fcalloc(MAXROOMS,   1                        )) == NULL)
       )
    {
        crashout("Can not allocate space for tables");
    }

    /* most .tab files are input here */
    /* cron.tab is input in readcron() along with cron.cit -- dcf 4/92 */
    if (!readTables())
    {
        cls();
        cCPrintf("Etc.dat not found!"); doccr();
        configcit();
    }
    else
    if (readconfigcit)
        readconfig(1);   /* forced to read in config.cit */

    if (cfg.f6pass[0])
    {
        ConLock = TRUE;
    }

    portInit();

    setscreen();

    update25();

    cCPrintf("Starting system."); doccr(); doccr();
    
    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(scratch, "%s\\%s", cfg.msgpath, "msg.dat");

    /* open message files: */
    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    msgFile     =  scratch  ;
    roomFile    = "room.dat";
    
    /* move to home-path */
    changedir(cfg.homepath);

    openFile(grpFile,  &grpfl );
    openFile(hallFile, &hallfl);
    openFile(logFile,  &logfl );
    openFile(msgFile,  &msgfl );
    openFile(roomFile, &roomfl);

    /* open Trap file */
    trapfl = fopen(cfg.trapfile, "a+");

    trap("Citadel Started", T_SYSOP);

    getGroup();
    getHall();

    if (cfg.accounting)
    {
        readaccount();    /* read in accounting data */
    }
    readprotocols();
    readcron();

    getRoom(LOBBY);     /* load Lobby>  */
    if (!slv_slave)
    {
        Initport();
        Initport();
    }
    whichIO = MODEM;

    /* record when we put system up */
    time(&uptimestamp);

    setdefaultconfig();
    setalloldrooms();
    roomtalley();
}

/* -------------------------------------------------------------------- */
/*  openFile()      Special to open a .cit file                         */
/* -------------------------------------------------------------------- */
void openFile(char *filename, FILE **fd)
{
    char str[80];
    
    /* open message file */
    if ((*fd = fopen(filename, "r+b")) == NULL)
    {
        sprintf(str, "%s file missing! (%d / %s)", filename, errno);
        crashout(str);
    }
}
