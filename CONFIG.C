/************************************************************************/
/*                              confg.c                                 */
/*      configuration program for Citadel bulletin board system.        */
/************************************************************************/

#include <direct.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ctdl.h"
#include "proto.h"
#include "keywords.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*      buildhalls()            builds hall-table (all rooms in Maint.) */
/*      buildroom()             builds a new room according to msg-buf  */
/*      clearaccount()          sets all group accounting data to zero  */
/*      configcit()             the main configuration for citadel      */
/*      illegal()               abort config.exe program                */
/*      initfiles()             opens & initalizes any missing files    */
/*      logInit()               indexes log.dat                         */
/*      logSort()               Sorts 2 entries in logTab               */
/*      readaccount()           reads grpdata.cit values into grp struct*/
/*      readconfig()            reads config.cit values                 */
/*      RoomTabBld()            builds room.tab, index's room.dat       */
/*      zapGrpFile()            initializes grp.dat                     */
/*      zapHallFile()           initializes hall.dat                    */
/*      zapLogFile()            initializes log.dat                     */
/*      zapRoomFile()           initializes room.dat                    */
/************************************************************************/

/************************************************************************/
/*                External variable declarations in CONFG.C             */
/************************************************************************/
char  *grpFile, *hallFile, *logFile, msgFile[64], *roomFile;

char   resizeMsg = FALSE,               /* Resize Msg-file?             */
       resizeLog = FALSE;               /* Resize Log-file?             */

void checkresize(void);
void resizemsgfile(void);
void resizelogfile(void);
BOOL resize_putMessage(void);
          
/************************************************************************/
/*      buildhalls()  builds hall-table (all rooms in Maint.)           */
/************************************************************************/
void buildhalls(void)
{
    int i;

    doccr(); cPrintf("Building hall file "); doccr();

    for (i = 4; i < MAXROOMS; ++i)
    {
        if (roomTab[i].rtflags.INUSE)
        {
            hallBuf->hall[1].hroomflags[i].inhall = 1;  /* In Maintenance */
            hallBuf->hall[1].hroomflags[i].window = 0;  /* Not a Window   */
        }
    }
    putHall();
}

/************************************************************************/
/*      buildroom()  builds a new room according to msg-buf             */
/************************************************************************/
void buildroom(void)
{
    int roomslot;

    if (*msgBuf->mbcopy) return;
    roomslot = msgBuf->mbroomno;

    if (msgBuf->mbroomno < MAXROOMS)
    {
        getRoom(roomslot);

        if ((strcmp(roomBuf.rbname, msgBuf->mbroom) != SAMESTRING)
        || (!roomBuf.rbflags.INUSE))
        {
            if (msgBuf->mbroomno > 3)
            {
                roomBuf.rbflags.INUSE     = TRUE;
                roomBuf.rbflags.PERMROOM  = FALSE;
                roomBuf.rbflags.MSDOSDIR  = FALSE;
                roomBuf.rbflags.GROUPONLY = FALSE;
                roomBuf.rbroomtell[0]     = '\0';
                roomBuf.rbflags.PUBLIC    = TRUE;
            }
            strcpy(roomBuf.rbname, msgBuf->mbroom);

            putRoom(thisRoom);
        }
    }
}

/************************************************************************/
/*      clearaccount()  initializes all group data                      */
/************************************************************************/
void clearaccount(void)
{
    int i;
    int groupslot;

    for (groupslot = 0; groupslot < MAXGROUPS; groupslot++)
    {
        /* init days */
        for ( i = 0; i < 7; i++ )
            accountBuf.group[groupslot].account.days[i] = 1;

        /* init hours & special hours */
        for ( i = 0; i < 24; i++ )
        {
            accountBuf.group[groupslot].account.hours[i]   = 1;
            accountBuf.group[groupslot].account.special[i] = 0;
        }

        accountBuf.group[groupslot].account.have_acc      = FALSE;
        accountBuf.group[groupslot].account.dayinc        = 0.f;
        accountBuf.group[groupslot].account.sp_dayinc     = 0.f;
        accountBuf.group[groupslot].account.maxbal        = 0.f;
        accountBuf.group[groupslot].account.priority      = 0.f;
        accountBuf.group[groupslot].account.dlmult        = -1;
        accountBuf.group[groupslot].account.ulmult        =  1;

    }
}

/************************************************************************/
/*      configcit() the <main> for configuration                        */
/************************************************************************/
void configcit(void)
{
    fcloseall();

    /* read config.cit */
    readconfig(0);

    /* move to home-path */
    changedir(cfg.homepath);

    /* initialize & open any files */
    initfiles();

    /* allocate the tables here so readconfig() can be called from sysop menu*/
    allocateTables();

    if (logTab == NULL)
        illegal("Error allocating logTab \n");
    if (msgTab1 == NULL || msgTab2 == NULL || /* msgTab3 == NULL || */
        msgTab4 == NULL || msgTab5 == NULL || msgTab6 == NULL ||
        msgTab7 == NULL || msgTab8 == NULL /* || msgTab9 == NULL */)
        illegal("Error allocating msgTab \n");

    if (msgZap )  zapMsgFile();
    if (roomZap)  zapRoomFile();
    if (logZap )  zapLogFile();
    if (grpZap )  zapGrpFile();
    if (hallZap)  zapHallFile();

    if (roomZap && !msgZap)  roomBuild = TRUE;
    if (hallZap && !msgZap)  hallBuild = TRUE;

    logInit();
    msgInit();
    RoomTabBld();

    if (hallBuild)  buildhalls();

    fcloseall();

    doccr();
    cPrintf("Config Complete");
    doccr();

    if (resizeLog)  resizelogfile();
    if (resizeMsg)  resizemsgfile();

    if (resizeLog || resizeMsg)
    {
        doccr();
        printf("Please Restart");
        doccr();
        exit(7);
    }
}


#ifdef GOODBYE
/***********************************************************************/
/*    illegal() Prints out configur error message and aborts           */
/***********************************************************************/
void illegal(char *errorstring)
{
    doccr();
    cPrintf("%s", errorstring);
    doccr();
    cPrintf("Fatal error. Aborting."); doccr();
    exit(200);
}
#endif

/***********************************************************************/
/*    illegal() Prints out configur error message and aborts           */
/***********************************************************************/
void illegal(const char *fmt, ...)
{
    char buff[256];
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);
    
    doccr();
    cPrintf("%s", buff);
    doccr();
    cPrintf("Fatal error in configuration. Aborting."); doccr();
    curson();
    exit(7);
}



/************************************************************************/
/*      initfiles() -- initializes files, opens them                    */
/************************************************************************/
void initfiles(void)
{
    static char it_will_be_initialized[] = " It will be initialized. ";
    static char not_found[]              = " %s not found. ";

    chdir(cfg.homepath);

    if (cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  == '\\')
        cfg.msgpath[ (strlen(cfg.msgpath) - 1) ]  =  '\0';

    sprintf(msgFile, "%s\\%s", cfg.msgpath, "msg.dat");

    grpFile     = "grp.dat" ;
    hallFile    = "hall.dat";
    logFile     = "log.dat" ;
    roomFile    = "room.dat";

    checkresize();

    /* open group file */
    if ((grpfl = fopen(grpFile, "r+b")) == NULL)
    {
        cPrintf(not_found, grpFile);  doccr();
        if ((grpfl = fopen(grpFile, "w+b")) == NULL)
            illegal("Can't create the group file!");
        {
            cPrintf(it_will_be_initialized); doccr();
            grpZap = TRUE;
        }
    }

    /* open hall file */
    if ((hallfl = fopen(hallFile, "r+b")) == NULL)
    {
        cPrintf(not_found, hallFile); doccr();
        if ((hallfl = fopen(hallFile, "w+b")) == NULL)
            illegal("Can't create the hall file!");
        {
            cPrintf(it_will_be_initialized);  doccr();
            hallZap = TRUE;
        }
    }

    /* open log file */
    if ((logfl = fopen(logFile, "r+b")) == NULL)
    {
        cPrintf(not_found, logFile); doccr();
        if ((logfl = fopen(logFile, "w+b")) == NULL)
            illegal("Can't create log file!");
        {
            cPrintf(it_will_be_initialized);  doccr();
            logZap = TRUE;
        }
    }

    /* open message file */
    if ((msgfl = fopen(msgFile, "r+b")) == NULL)
    {
        cPrintf(not_found, "msg.dat");  doccr();
        if ((msgfl = fopen(msgFile, "w+b")) == NULL)
            illegal("Can't create message file!");
        {
            cPrintf(it_will_be_initialized);  doccr();
            msgZap = TRUE;
        }
    }

    /* open room file */
    if ((roomfl = fopen(roomFile, "r+b")) == NULL)
    {
        cPrintf(not_found, roomFile);  doccr();
        if ((roomfl = fopen(roomFile, "w+b")) == NULL)
            illegal("Can't create room file!");
        {
            cPrintf(it_will_be_initialized); doccr();
            roomZap = TRUE;
        }
    }
}

/************************************************************************/
/*      logInit() indexes log.dat                                       */
/************************************************************************/
void logInit(void)
{
    int i;
    int count = 0;

    doccr(); doccr();
    cPrintf("Building log table "); doccr();

    cfg.callno = 0l;

    rewind(logfl);
    
    /* clear logTab */
    for (i = 0; i < cfg.MAXLOGTAB; i++) 
        memset(&logTab[i], 0, sizeof(&logTab[i]));

    /* load logTab: */
    for (thisLog = 0;  thisLog < cfg.MAXLOGTAB;  thisLog++)
    {
  
        cPrintf("log#%d\r",thisLog);

        getLog(&logBuf, thisLog);

        if (logBuf.callno > cfg.callno)  cfg.callno = logBuf.callno;

        /* 
         * count valid entries:             
         */
        if (logBuf.lbflags.L_INUSE == 1)  count++;
          
        /* 
         * copy relevant info into index:   
         */
        log2tab(&logTab[thisLog], &logBuf);
        logTab[thisLog].ltlogSlot= thisLog;
        
#ifdef OLD        
        logTab[thisLog].ltcallno = logBuf.callno;
        logTab[thisLog].permanent = logBuf.lbflags.PERMANENT;
        if (logBuf.lbflags.L_INUSE == 1)
        {
            logTab[thisLog].ltnmhash = hash(logBuf.lbname);
            logTab[thisLog].ltinhash = hash(logBuf.lbin  );
            logTab[thisLog].ltpwhash = hash(logBuf.lbpw  );
        }
        else
        {
            logTab[thisLog].ltnmhash = 0;
            logTab[thisLog].ltinhash = 0;
            logTab[thisLog].ltpwhash = 0;
        }
#endif        
    
    }
    doccr();
    cPrintf("%lu calls.", cfg.callno);
    doccr();
    cPrintf("%d valid log entries.", count);  

    qsort(logTab, (size_t)cfg.MAXLOGTAB, (unsigned)sizeof(*logTab), logSort);
}

/************************************************************************/
/*      logSort() Sorts 2 entries in logTab                             */
/************************************************************************/
int logSort(struct lTable *s1, struct lTable *s2)
{
    if (s1->ltnmhash == 0 && s2->ltnmhash == 0)
        return 0;
    if (s1->ltnmhash == 0 && s2->ltnmhash != 0)
        return 1;
    if (s1->ltnmhash != 0 && s2->ltnmhash == 0)
        return -1;
    if (s1->ltcallno < s2->ltcallno)
        return 1;
    if (s1->ltcallno > s2->ltcallno)
        return -1;
    return 0;
}

/************************************************************************/
/*      readaccount()  reads grpdata.cit values into group structure    */
/************************************************************************/
void readaccount(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  i, j, k, l, count;
    int groupslot = ERROR;
    int hour;
   
    clearaccount();   /* initialize all accounting data */

    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("grpdata.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Grpdata.cit!"); doccr();
        exit(200);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; grpkeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], grpkeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        switch(i)
        {
            case GRK_DAYS:              
                if (groupslot == ERROR)  break;

                /* init days */
                for ( j = 0; j < 7; j++ )
                    accountBuf.group[groupslot].account.days[j] = 0;

                for (j = 1; j < count; j++)
                {
                    for (k = 0; daykeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                        {
                            break;
                        }
                    }
                    if (k < 7)
                        accountBuf.group[groupslot].account.days[k] = TRUE;
                    else if (k == 7)  /* any */
                    {
                        for ( l = 0; l < MAXGROUPS; ++l)
                            accountBuf.group[groupslot].account.days[l] = TRUE;
                    }
                    else
                    {
                        doccr();
                   cPrintf("Grpdata.Cit - Warning: Unknown day %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case GRK_GROUP:             
                groupslot = groupexists(words[1]);
                if (groupslot == ERROR)
                {
                    doccr();
                    cPrintf("Grpdata.Cit - Warning: Unknown group %s", words[1]);
                    doccr();
                }
                accountBuf.group[groupslot].account.have_acc = TRUE;
                break;

            case GRK_HOURS:             
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.hours[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.hours[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 ) 
                        {
                            doccr();
                            cPrintf("Grpdata.Cit - Warning: Invalid hour %d ",
                            hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.hours[hour] = TRUE;
                    }
                }
                break;

            case GRK_DAYINC:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dayinc);   /* float */
                }
                break;

            case GRK_DLMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.dlmult);   /* float */
                }
                break;

            case GRK_ULMULT:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.ulmult);   /* float */
                }
                break;

            case GRK_PRIORITY:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.priority);  /* float */
                }

                break;

            case GRK_MAXBAL:
                if (groupslot == ERROR)  break;

                if (count > 1)
                {
                    sscanf(words[1], "%f ",
                    &accountBuf.group[groupslot].account.maxbal);   /* float */
                }

                break;



            case GRK_SPECIAL:           
                if (groupslot == ERROR)  break;

                /* init hours */
                for ( j = 0; j < 24; j++ )
                    accountBuf.group[groupslot].account.special[j]   = 0;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            accountBuf.group[groupslot].account.special[l] = TRUE;
                    }
                    else
                    {
                        hour = atoi(words[j]);

                        if ( hour > 23 )
                        {
                            doccr();
                            cPrintf("Grpdata.Cit - Warning: Invalid special hour %d ", hour);
                            doccr();
                        }
                        else
                       accountBuf.group[groupslot].account.special[hour] = TRUE;
                    }

                }
                break;
        }

    }
    fclose(fBuf);
}

/************************************************************************/
/*      readprotocols() reads protocol.cit values into ext   structure  */
/************************************************************************/
void readprotocols(void)
{                          
    FILE *fBuf;
    char line[128];
    char *words[256];
    int  j, count;

    numDoors    = 0;
    extrncmd[0] = '\0' /* NULL */;
    editcmd[0]  = '\0' /* NULL */;
   
    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("external.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find external.cit!"); doccr();
        exit(200);
    }

    while (fgets(line, 125, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        if (strcmpi("#PROTOCOL", words[0]) == SAMESTRING)
        {
            j = strlen(extrncmd);

            if (strlen( words[1] ) > 19 )
              illegal("Protocol name to long; must be less than 20");
            if (strlen( words[3] ) > 39 )
              illegal("Recive command to long; must be less than 40");
            if (strlen( words[4] ) > 39 )
              illegal("Send command to long; must be less than 40");
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Batch field bad; must be 0 or 1");
            if (atoi(words[3]) < 0 || atoi(words[3]) > 10 * 1024)
              illegal("Block field bad; must be 0 to 10K");
            if (j >= MAXEXTERN)
              illegal("To many external proticals");
    
            strcpy(extrn[j].ex_name, words[1]);
            extrn[j].ex_batch = (char)atoi(words[2]);
            extrn[j].ex_block = atoi(words[3]);
            strcpy(extrn[j].ex_rcv,  words[4]);
            strcpy(extrn[j].ex_snd,  words[5]);
            extrncmd[j]   = (char)tolower(*words[1]);
            extrncmd[j+1] = '\0';
        }
        if (strcmpi("#EDITOR", words[0]) == SAMESTRING)
        {
            j = strlen(editcmd);

            if (strlen( words[1] ) > 19 )
              illegal("Protocol name to long; must be less than 20");
            if (strlen( words[3] ) > 29 )
              illegal("Command line to long; must be less than 30");
            if (atoi(words[2]) < 0 || atoi(words[2]) > 1)
              illegal("Local field bad; must be 0 or 1");
            if (j > 19)
              illegal("Only 20 external editors");
    
            strcpy(edit[j].ed_name,  words[1]);
            edit[j].ed_local  = (char)atoi(words[2]);
            strcpy(edit[j].ed_cmd,   words[3]);
            editcmd[j]    = (char)tolower(*words[1]);
            editcmd[j+1]                = '\0';
        }
        if (strcmpi("#DOOR", words[0]) == SAMESTRING)
        {
            if (count < 8)
              illegal("To few arguments for #OTHER command");
            if (strlen( words[1] ) > NAMESIZE )
              illegal("Door name to long, must be less than 31");
            if (strlen( words[2] ) > 40 )
              illegal("Door command to long, must be less than 41");
            if (strlen( words[3] ) > NAMESIZE )
              illegal("Door group to long, must be less than 31");
            if (numDoors >= (MAXDOORS) )
              illegal("To many #DOORs");
    
            strcpy(doors[numDoors].name,   words[1]);
            strcpy(doors[numDoors].cmd ,   words[2]);
            strcpy(doors[numDoors].group,  words[3]);
            doors[numDoors].CON     = atoi(words[4]);
            doors[numDoors].SYSOP   = atoi(words[5]);
            doors[numDoors].AIDE    = atoi(words[6]);
            doors[numDoors].DIR     = atoi(words[7]);
            numDoors++;
        }
    }
    fclose(fBuf);
}

/*
 * count the lines that start with keyword...
 *
int keyword_count(key, filename)
char *key;
char *filename;
{
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  count = 0;
   
    changedir(cfg.homepath);

    if ((fBuf = fopen(filename, "r")) == NULL) 
    {  
        cPrintf("Can't find %s!", filename); doccr();
        exit(200);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        parse_it( words, line);

        if (strcmpi(key, words[0]) == SAMESTRING)
          count++;
   }

   fclose(fBuf);

   return (count == 0 ? 1 : count);
} */

/************************************************************************/
/*      readconfig() reads config.cit values                            */
/*      ignore == 0 normal call before reconfiguring system             */
/*      ignore == 1 force read at startup with CTDL -E cmd line option  */
/*      ignore == 2 force read from sysop menu  '&'                     */
/************************************************************************/
void readconfig(char ignore) 
{
    FILE *fBuf;
    char line[256];
    char *words[256];
    int  i, j, k, l, count, att;
    char notkeyword[20];
    char valid = FALSE;
    char found[K_NWORDS+2];
    int  lineNo = 0;

    static char nochange[] = "%s invalid: can't change to '%s' directory";

    strcpy(cfg.msg_nym,  "message");
    strcpy(cfg.msgs_nym, "messages");
    strcpy(cfg.msg_done, "Saving");
    cfg.version = 3120000L;
    
    for (i=0; i <= K_NWORDS; i++)
        found[i] = FALSE;

    if ((fBuf = fopen("config.cit", "r")) == NULL)  /* ASCII mode */
    {  
        cPrintf("Can't find Config.cit!"); doccr();
        exit(200);
    }

    while (fgets(line, 254, fBuf) != NULL)
    {
        lineNo++;

        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; keywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], keywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        if (keywords[i] == NULL)
        {
            cPrintf("CONFIG.CIT (%d) Warning: Unknown variable %s ", lineNo, 
                words[0]);
            doccr();
            continue;
        }
        else
        {
            if (found[i] == TRUE)
            {
                cPrintf("CONFIG.CIT (%d) Warning: %s mutiply defined!", lineNo, 
                    words[0]);
                doccr();
            }else{
                found[i] = TRUE;
            }
        }

        switch(i)
        {
            case K_ACCOUNTING:
                cfg.accounting = (uchar)atoi(words[1]);
                break;

            case K_IDLE_WAIT:
                cfg.idle = atoi(words[1]);
                break;

            case K_ATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.attr = (uchar)att;
                break;

            case K_WATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.wattr = (uchar)att;
                break;

            case K_CATTR:
                sscanf(words[1], "%x ", &att); /* hex! */
                cfg.cattr = (uchar)att;
                break;

            case K_BATTR:
                sscanf(words[1], "%x ", &att);    /* hex! */
                cfg.battr = (uchar)att;
                break;

            case K_UTTR:
                sscanf(words[1], "%x ", &att);     /* hex! */
                cfg.uttr = (uchar)att;
                break;

            case K_INIT_BAUD:
                cfg.initbaud = (uchar)atoi(words[1]);
                break;
            
            case K_READOLD:
                cfg.readOld = (uchar)atoi(words[1]);
                break;
            
            case K_LINES_SCREEN:
                cfg.linesScreen = (char)atoi(words[1]);
                break;

            case K_BIOS:
                cfg.bios = (uchar)atoi(words[1]);
                break;

            case K_DUMB_MODEM:
                cfg.dumbmodem    = (uchar)atoi(words[1]);
                break;

            case K_READLLOG:
                cfg.readluser    = (uchar)atoi(words[1]);
                break;

            case K_DATESTAMP:
                if (strlen( words[1] ) > 63 )
                    illegal("#DATESTAMP too long; must be less than 64");

                strcpy( cfg.datestamp, words[1] );
                break;
            
            case K_ENTER_NAME:
                if (strlen( words[1] ) > NAMESIZE )
                {
                    illegal("#ENTER_NAME too long; must be less than 30");
                }
                strcpy( cfg.enter_name, words[1] );
                break;

            case K_VDATESTAMP:
                if (strlen( words[1] ) > 63 )
                    illegal("#VDATESTAMP too long; must be less than 64");

                strcpy( cfg.vdatestamp, words[1] );
                break;

            case K_MODERATE: 
                cfg.moderate     = (uchar)atoi(words[1]);
                break;

            case K_NETMAIL:
                cfg.netmail = (uchar)atoi(words[1]);
                break;

            case K_HELPPATH:  
                if (strlen( words[1] ) > 63 )
                    illegal("helppath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy( cfg.helppath, words[1] );  
                break;
            
            case K_NET_PREFIX:  
                if (strlen( words[1] ) > 10 )
                    illegal("#NET_PREFIX too long; must be less than 11");

                strcpy( cfg.netPrefix, words[1] );  
                break;
            
            case K_TRANSPATH:  
                if (strlen( words[1] ) > 63 )
                    illegal("#TRANSPATH too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy( cfg.transpath, words[1] );  
                break;

            case K_TEMPPATH:
                if (strlen( words[1] ) > 63 )
                illegal("temppath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);


                strcpy( cfg.temppath, words[1] );
                break;


            case K_HOMEPATH:
                if (strlen( words[1] ) > 63 )
                    illegal("homepath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.homepath, words[1] );  
                break;

            case K_KILL:
                cfg.kill = (uchar)atoi(words[1]);
                break;

            case K_LINEFEEDS:
                cfg.linefeeds = (uchar)atoi(words[1]);
                break;
            
            case K_LOGINSTATS:
                cfg.loginstats = (uchar)atoi(words[1]);
                break;

            case K_MAXBALANCE:
                sscanf(words[1], "%f ", &cfg.maxbalance); /* float */
                break;

            case K_MAXLOGTAB:
                if (ignore) break;

                cfg.MAXLOGTAB    = atoi(words[1]);

                break;

            case K_MESSAGE_ROOM:
                cfg.MessageRoom = (char)atoi(words[1]);
                break;

            case K_NEWUSERAPP:
                if (strlen( words[1] ) > 12 )
                illegal("NEWUSERAPP too long; must be less than 13");

                strcpy( cfg.newuserapp, words[1] );
                break;

            case K_MAXTEXT:
                cfg.maxtext = atoi(words[1]);
                break;

            case K_MAX_WARN:
                cfg.maxwarn = (char)atoi(words[1]);
                break;

            case K_MDATA:

                if (ignore == 2) break;

                cfg.mdata   = atoi(words[1]);

                if ( (cfg.mdata < 1) || (cfg.mdata > 4) )
                {
                    illegal("MDATA port can only currently be 1, 2, 3 or 4");
                }
                break;

            case K_MAXFILES:
                cfg.maxfiles = atoi(words[1]);
                break;

            case K_MSGPATH:
                if (strlen(words[1]) > 63)
                    illegal("msgpath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.msgpath, words[1]);  
                break;

            case K_F6PASSWORD:
                if (strlen(words[1]) > 19)
                    illegal("f6password too long; must be less than 20");

                strcpy(cfg.f6pass, words[1]);  
                break;

            case K_APPLICATIONS:
                if (strlen(words[1]) > 63)
                    illegal("applicationpath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.aplpath, words[1]);  
                break;

            case K_MESSAGEK:
                if (ignore) break;
                cfg.messagek = atoi(words[1]);
                break;

            case K_MODSETUP:
                if (strlen(words[1]) > 63)
                    illegal("Modsetup too long; must be less than 64");

                strcpy(cfg.modsetup, words[1]);  
                break;
                
            case K_DIAL_INIT:
                if (strlen(words[1]) > 63)
                    illegal("Dial_Init too long; must be less than 64");

                strcpy(cfg.dialsetup, words[1]);  
                break;
                
            case K_DIAL_PREF:
                if (strlen(words[1]) > 20)
                    illegal("Dial_Prefix too long; must be less than 20");

                strcpy(cfg.dialpref, words[1]);  
                break;

            case K_NEWBAL:
                sscanf(words[1], "%f ", &cfg.newbal);  /* float */
                break;

            case K_AIDEHALL:
                cfg.aidehall = (uchar)atoi(words[1]);
                break;

            case K_NMESSAGES:
                if (ignore) break;

                cfg.nmessages  = atoi(words[1]);

                break;

            case K_NODENAME:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("nodeName too long; must be less than 30");

                strcpy(cfg.nodeTitle, words[1]);  
                break;

            case K_SYSOP:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("sysop too long; must be less than 30");

                strcpy(cfg.sysop, words[1]);  
                break;
            
            case K_NODESIG:
                if (strlen(words[1]) > 90)
                    illegal("Signature too long; must be less than 90");

                strcpy(cfg.nodeSignature, words[1]);  
                break;

            case K_NODEREGION:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("nodeRegion too long; must be less than 30");

                strcpy(cfg.nodeRegion, words[1]);
                break;
            
            case K_NODECONTRY:
                if (strlen(words[1]) > NAMESIZE)
                {
                    illegal("#nodecontry too long; must be less than 30");
                }

                strcpy(cfg.nodeContry, words[1]);
                break;

            case K_NOPWECHO:
                cfg.nopwecho = (unsigned char)atoi(words[1]);
                break;

            case K_NULLS:
                cfg.nulls = (uchar)atoi(words[1]);
                break;

            case K_OFFHOOK:
                cfg.offhook = (uchar)atoi(words[1]);
                break;

            case K_OLDCOUNT:
                cfg.oldcount = atoi(words[1]);
                break;

            case K_PRINTER:
                if (ignore == 2) break;

                if (strlen(words[1]) > 63)
                    illegal("printer too long; must be less than 64");

                strcpy(cfg.printer, words[1]);  
                break;

            case K_ROOMOK:
                cfg.nonAideRoomOk = (uchar)atoi(words[1]);
                break;

            case K_ROOMTELL:
                cfg.roomtell = (uchar)atoi(words[1]);
                break;

            case K_ROOMPATH:
                if (strlen(words[1]) > 63)
                    illegal("roompath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.roompath, words[1]);  
                break;
            
            case K_FLOORS:
                cfg.floors = (uchar)atoi(words[1]);
                break;

            case K_SUBHUBS:
                cfg.subhubs = (char)atoi(words[1]);
                break;

            case K_TABS:
                cfg.tabs = (uchar)atoi(words[1]);
                break;
            
            case K_TIMEOUT:
                cfg.timeout = (char)atoi(words[1]);
                break;

            case K_TRAP:
                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; trapkeywords[k] != NULL; k++)
                    {
                        sprintf(notkeyword, "!%s", trapkeywords[k]);

                        if (strcmpi(words[j], trapkeywords[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = TRUE;
                            }
                            else cfg.trapit[k] = TRUE;
                        }
                        else if (strcmpi(words[j], notkeyword) == SAMESTRING)
                        {
                            valid = TRUE;

                            if ( k == 0)  /* ALL */
                            {
                                for (l = 0; l < 16; ++l) cfg.trapit[l] = FALSE;
                            }
                            else cfg.trapit[k] = FALSE; 
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TRAP parameter %s ", words[j]);
                        doccr();
                    }
                }
                break;

            case K_TRAP_FILE:

                if (ignore == 2) break;

                if (strlen(words[1]) > 63)
                    illegal("Trap filename too long; must be less than 64");
  
                strcpy(cfg.trapfile, words[1]);  

                break;

            case K_UNLOGGEDBALANCE:
                sscanf(words[1], "%f ", &cfg.unlogbal);  /* float */
                break;

            case K_UNLOGTIMEOUT:
                cfg.unlogtimeout = (char)atoi(words[1]);
                break;

            case K_UPPERCASE:
                cfg.uppercase = (uchar)atoi(words[1]);
                break;

            case K_USER:
                for ( j = 0; j < 5; ++j)  cfg.user[j] = 0;

                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; userkeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], userkeywords[k]) == SAMESTRING)
                        {
                           valid = TRUE;

                           cfg.user[k] = TRUE;
                        }
                    }

                    if (!valid)
                    {
                        doccr();
                   cPrintf("Config.Cit - Warning: Unknown #USER parameter %s ",
                        words[j]);
                        doccr();
                    }
                }
                break;

            case K_WIDTH:
                cfg.width = (uchar)atoi(words[1]);
                break;

            case K_TWIT_FEATURES:
                cfg.msgNym     = FALSE;
                cfg.borders    = FALSE;
                cfg.titles     = FALSE;
                cfg.nettitles  = FALSE;
                cfg.surnames   = FALSE;
                cfg.netsurname = FALSE;
                cfg.entersur   = FALSE;
                cfg.colors     = FALSE;

                for (j = 1; j < count; j++)
                {
                    valid = FALSE;

                    for (k = 0; twitfeatures[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], twitfeatures[k]) == SAMESTRING)
                        {
                            valid = TRUE;

                            switch (k)
                            {
                            case 0:     /* MESSAGE NYMS */
                                cfg.msgNym = TRUE;
                                break;

                            case 1:     /* BOARDERS */
                                cfg.borders = TRUE;
                                break;
                            
                            case 2:     /* TITLES */
                                cfg.titles = TRUE;
                                break;
                            
                            case 3:     /* NET_TITLES */
                                cfg.nettitles = TRUE;
                                break;
                            
                            case 4:     /* SURNAMES */
                                cfg.surnames = TRUE;
                                break;
                            
                            case 5:     /* NET_SURNAMES */
                                cfg.netsurname = TRUE;
                                break;
                            
                            case 6:     /* ENTER_TITLES */
                                cfg.entersur = TRUE;
                                break;

                            case 7:     /* COLORS */
                                cfg.colors = TRUE;
                                break;

                            default:
                                break;
                            }
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #TWIT_FEATURES parameter %s ",
                                words[j]);
                        doccr();
                    }
                }
                break;
            
            case K_LOGIN:
                cfg.l_closedsys   = FALSE;
                cfg.l_verified    = TRUE;
                cfg.l_questionare = FALSE;
                cfg.l_application = FALSE;
                cfg.l_sysop_msg   = FALSE;
                cfg.l_create      = TRUE;
                
                for (j = 1; j < count; j++)
                {
                    for (k = 0, valid = FALSE; newuserkeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], newuserkeywords[k]) == SAMESTRING)
                        {
                            valid = TRUE;
                            l = TRUE;
                        }
                        else
                        if (words[j][0] == '!' &&
                            strcmpi((words[j])+1, newuserkeywords[k]) == SAMESTRING)
                        {
                            valid = TRUE;
                            l = FALSE;
                        }
                            
                        if (valid)    
                        {
                            switch (k)
                            {
                            case L_CLOSED:
                                cfg.l_closedsys = (uchar)l;
                                break;
                            
                            case L_VERIFIED:
                                cfg.l_verified = (uchar)l;
                                break;
                            
                            case L_QUESTIONS:
                                cfg.l_questionare = (uchar)l;
                                break;
                            
                            case L_APPLICATION:
                                cfg.l_application = (uchar)l;
                                break;
                            
                            case L_SYSOP_MESSAGE:
                                cfg.l_sysop_msg = (uchar)l;
                                break;
                            
                            case L_NEW_ACCOUNTS:
                                cfg.l_create = (uchar)l;
                                break;
                                
                            default:
                                doccr();
                                cPrintf("Config.Cit - Warning:"
                                        " Unknown #LOGIN parameter %s ",
                                        words[j]);
                                doccr();
                                break;
                            }
                            break;
                        }
                    }

                    if ( !valid )
                    {
                        doccr();
                        cPrintf("Config.Cit - Warning:"
                                " Unknown #LOGIN parameter %s ",
                                words[j]);
                        doccr();
                    }
                }
                break;

/* New stuff */

            case K_DIRPATH:
                if (strlen(words[1]) > 63)
                    illegal("dirpath too long; must be less than 64");

                if (changedir(words[1]) == ERROR)
                    illegal(nochange, words[0], words[1]);

                strcpy(cfg.dirpath, words[1]);
                break;


            case K_DIAL_RING:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("dial_ring too long; must be less than 30");

                strcpy(cfg.dialring, words[1]);  
                break;

            case K_UP_DAYS:              
                /* init days */
                for ( j = 0; j < 7; j++ )
                    cfg.updays[j] = 0;

                for (j = 1; j < count; j++)
                {
                    for (k = 0; daykeywords[k] != NULL; k++)
                    {
                        if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                        {
                            break;
                        }
                    }
                    if (k < 7)
                        cfg.updays[k] = TRUE;
                    else if (k == 7)  /* any */
                    {
                        for ( l = 0; l < 7; ++l)
                            cfg.updays[l] = TRUE;
                    }
                    else
                    {
                        doccr();
                            cPrintf("Invalid CONFIG.CIT %s: %s",
                                    words[0], words[j]);
                        doccr();
                    }
                }
                break;

            case K_UP_HOURS:
                /* init hours */
                for ( j = 0; j < 24; j++ )
                    cfg.uphours[j] = FALSE;

                for (j = 1; j < count; j++)
                {
                    if (strcmpi(words[j], "Any") == SAMESTRING)
                    {
                        for (l = 0; l < 24; l++)
                            cfg.uphours[l] = TRUE;
                    }
                    else
                    {
                        l = atoi(words[j]);

                        if ( l > 23 ) 
                        {
                            doccr();
                            cPrintf("Invalid CONFIG.CIT %s: %s",
                                    words[0], words[j]);
                            doccr();
                        }
                        else
                            cfg.uphours[l] = TRUE;
                    }
                }
                break;

            case K_TWITREGION:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("twitregion too long; must be less than 30");

                strcpy(cfg.twitRegion, words[1]);
                break;

            case K_TWITCOUNTRY:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("twitcountry too long; must be less than 30");

                strcpy(cfg.twitCountry, words[1]);
                break;

            case K_ANONAUTHOR:
                if (strlen(words[1]) > NAMESIZE)
                    illegal("anonauthor too long; must be less than 30");

                strcpy(cfg.anonauthor, words[1]);
                break;

/* End of New stuff */


            default:
                cPrintf("Config.Cit - Warning: Unknown variable %s", words[0]);
                doccr();
                break;
        }
    }
    fclose(fBuf);

    for (i = 0, valid = TRUE; i <= K_NWORDS; i++)
    {
        if (!found[i])
        {
            cPrintf("CONFIG.CIT : ERROR: can not find %s keyword!\n",
                keywords[i]);
            valid = FALSE;
        }
    }

    if (!valid)
        illegal("");

    if (strcmpi(cfg.homepath, cfg.temppath) == SAMESTRING)    
    {
        illegal("#HOMEPATH may not equal #TEMPPATH");
    }
}

/************************************************************************/
/*      RoomTabBld() -- build RAM index to ROOM.DAT, displays stats.    */
/************************************************************************/
void RoomTabBld(void)
{
    int  slot;
    int  roomCount = 0;

    doccr(); doccr();
    cPrintf("Building room table"); doccr();

    for (slot = 0;  slot < MAXROOMS;  slot++)
    {
        getRoom(slot);

        cPrintf("Room No: %d\r", slot);

        if (roomBuf.rbflags.INUSE)  ++roomCount;
   
        noteRoom();
        putRoom(slot);
    }
    doccr();
    cPrintf(" %d of %d rooms in use", roomCount, MAXROOMS); doccr();

}

/************************************************************************/
/*      zapGrpFile(), erase & reinitialize group file                   */
/************************************************************************/
void zapGrpFile(void)
{
    doccr();
    cPrintf("Writing group table."); doccr();

    memset(&grpBuf, 0, sizeof grpBuf);

    /* To fucking hell with calling it "Null".  I'm tempted to nuke        */
    /* Reserved_2 into something different, too, but I want to know what   */
    /* function it serves (if any) first.                                  */

    strcpy( grpBuf.group[0].groupname, "Local");
    grpBuf.group[0].g_inuse  = 1;
    grpBuf.group[0].groupgen = 1;      /* Group Null's gen# is one      */
                                       /* everyone's a member at log-in */

    strcpy( grpBuf.group[1].groupname, "Reserved_2");
    grpBuf.group[1].g_inuse   = 1;
    grpBuf.group[1].groupgen  = 1;

    putGroup();
}

/************************************************************************/
/*      zapHallFile(), erase & reinitialize hall file                   */
/************************************************************************/
void zapHallFile(void)
{
    int i;
    
    doccr();
    cPrintf("Writing hall table.");  doccr();

    strcpy( hallBuf->hall[0].hallname, "Main");
    hallBuf->hall[0].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[0].h_inuse = 1;
    hallBuf->hall[0].hroomflags[0].inhall = 1;  /* Lobby> in Root        */
    hallBuf->hall[0].hroomflags[1].inhall = 1;  /* Mail>  in Root        */
    hallBuf->hall[0].hroomflags[2].inhall = 1;  /* Aide)  in Root        */

    strcpy( hallBuf->hall[1].hallname, "Maintenance");
    hallBuf->hall[1].owned = 0;                 /* Hall is not owned     */

    hallBuf->hall[1].h_inuse = 1;
    hallBuf->hall[1].hroomflags[0].inhall = 1;  /* Lobby> in Maintenance */
    hallBuf->hall[1].hroomflags[1].inhall = 1;  /* Mail>  in Maintenance */
    hallBuf->hall[1].hroomflags[2].inhall = 1;  /* Aide)  in Maintenance */


    hallBuf->hall[0].hroomflags[2].window = 1;  /* Aide) is the window   */
    hallBuf->hall[1].hroomflags[2].window = 1;  /* Aide) is the window   */

    /*
     * Init the room position table..
     */
    for (i=0; i<MAXROOMS; i++)
        roomPos[i] = (uchar)i;
    
    putHall();
}

/************************************************************************/
/*      zapLogFile() erases & re-initializes userlog.buf                */
/************************************************************************/
zapLogFile()
{
    int  i;

    /* clear RAM buffer out:                    */
    memset(&logBuf, 0, sizeof(logBuf));
 
    doccr();  
    doccr();
    cPrintf("MAXLOGTAB=%d",cfg.MAXLOGTAB);  doccr();

    /* write empty buffer all over file;        */
    for (i = 0; i < cfg.MAXLOGTAB;  i++)
    {
        cPrintf("Clearing log entry %d\r", i);
        /* logTab[i].ltlogSlot = i; */
        putLog(&logBuf, i);
    }
    doccr();
    return TRUE;
}

/************************************************************************/
/*      zapRoomFile() erases and re-initailizes ROOM.DAT                */
/************************************************************************/
zapRoomFile()
{
    int i;

    memset(&roomBuf, 0, sizeof(roomBuf));
    
    doccr();  doccr();
    cPrintf("MAXROOMS=%d", MAXROOMS); doccr();

    for (i = 0;  i < MAXROOMS;  i++)
    {
        cPrintf("Clearing room %d\r", i);
        putRoom(i);
        noteRoom();
    }

    /* Lobby> always exists -- guarantees us a place to stand! */
    thisRoom            = 0          ;
    strcpy(roomBuf.rbname, "Lobby")  ;
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(LOBBY);
    noteRoom();

    /* Mail> is also permanent...       */
    thisRoom            = MAILROOM      ;
    strcpy(roomBuf.rbname, "Exclusive Messages");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(MAILROOM);
    noteRoom();

    /* Aide) also...                    */
    thisRoom            = AIDEROOM;
    strcpy(roomBuf.rbname, "Aide");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = FALSE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(AIDEROOM);
    noteRoom();

    /* Dump> also...                    */
    thisRoom            = DUMP;
    strcpy(roomBuf.rbname, "Dump");
    roomBuf.rbflags.PERMROOM = TRUE;
    roomBuf.rbflags.PUBLIC   = TRUE;
    roomBuf.rbflags.INUSE    = TRUE;

    putRoom(DUMP);
    noteRoom();

    return TRUE;
}

/************************************************************************/
/**************************  Resize Stuff *******************************/
/************************************************************************/

int    newmessagek, newmaxlogtab;       /* New size values              */


/************************************************************************/
/*      resizelogfile() -- resizes log file                             */
/************************************************************************/
void resizelogfile(void)
{
    int i;
    int result;
    int dummy;
    char *logFile2;

    chdir(cfg.homepath);

    fclose(logfl);

    logFile2 = "log.tmp";
    
    if ((logfl = fopen(logFile2, "w+b")) == NULL)
        illegal("Can't create temp log file!");

    dummy = cfg.MAXLOGTAB;
    cfg.MAXLOGTAB = newmaxlogtab;
    zapLogFile();
    cfg.MAXLOGTAB = dummy;
    doccr();

    fclose(logfl);

    for (i = 0;  ((i < cfg.MAXLOGTAB) && (i < newmaxlogtab)); i++)
    {
        cPrintf("Copying log entry %d  \r", logTab[i].ltlogSlot);

        /* open first log file */
        if ((logfl = fopen(logFile, "r+b")) == NULL)
        {
            printf("Can't open log file");
            doccr();
        }

        /* get source log entry  */
        getLog(&logBuf, logTab[i].ltlogSlot);

        /* close first log file */
        fclose(logfl);

        /* open temp log file */
        if ((logfl = fopen(logFile2, "r+b")) == NULL)
        {
            printf("Can't open temp. log file");
            doccr();
        }

        /* write destination log entry */ 
        putLog(&logBuf, i);

        /* Close temp log file */
        fclose(logfl);
    }
    doccr();

    /* clear RAM buffer out: */
    memset(&logBuf, 0, sizeof(logBuf));

    result = unlink(logFile);
    if (result == -1)
    {
        printf("Cannot delete log.dat");
        doccr();
    }

    result = rename(logFile2, logFile);
    if (result == -1)
    {
        printf("Cannot rename log.tmp");
        doccr();
    }
}

/************************************************************************/
/*      resizemsgfile() -- resizes message file                         */
/************************************************************************/
void resizemsgfile(void)
{
    int i;
    int result;
    char msgFile2[64];
    int dummy;
    long loc;
    int tablesize;
    ulong here;
    ulong oldest;

    oldest    = cfg.mtoldest;
    tablesize = sizetable();

    sprintf(msgFile2, "%s\\%s", cfg.msgpath, "msg.tmp");

    fclose(msgfl);
    
    if ((msgfl = fopen(msgFile2, "w+b")) == NULL)
        illegal("Can't create the message file!");

    dummy = cfg.messagek;
    cfg.messagek = newmessagek;
    zapMsgFile();
    cfg.catLoc = 0l;

    cfg.messagek = dummy;
    doccr();
    doccr();

    fclose(msgfl);

    for (i = 0; i < tablesize; i++)
    {
        loc = msgTab2[i].mtmsgLoc;

        /* open first message file */
        if ((msgfl = fopen(msgFile, "r+b")) == NULL)
        {
            printf("Can't open msg file");
            doccr();
        }

        fseek(msgfl, loc, 0);

        getMessage();
        getMsgStr(msgBuf->mbtext, cfg.maxtext); 

        /* close first msg file */
        fclose(msgfl);

        sscanf(msgBuf->mbId, "%ld", &here);

        /* Don't bother with the null message */
        if (here == 1L)
            continue;

        if (here != (oldest + (ulong)i))
            continue;

        cPrintf("Copying Message #%lu\r", here);

        /* open temp message file */
        if ((msgfl = fopen(msgFile2, "r+b")) == NULL)
        {
            printf("Can't open temp. msg file");
            doccr();
        }

        resize_putMessage();

        /* close temp msg file */
        fclose(msgfl);
    }
    doccr();

    result = unlink(msgFile);
    if (result == -1)
    {
        printf("Cannot delete msg.dat");
        doccr();
    }

    result = rename(msgFile2, msgFile);
    if (result == -1)
    {
        printf("Cannot rename msg.tmp");
        doccr();
    }
}

/************************************************************************/
/*      checkresize() -- checks to see if message file and/or log       */
/*      need to be resized.                                             */
/************************************************************************/
void checkresize(void)
{
    struct stat buf;
    int fh, result;

    /* save old values for later */
    newmessagek  = cfg.messagek;
    newmaxlogtab = cfg.MAXLOGTAB;

    fh = open(msgFile, O_RDONLY);

    if (fh != -1)
    {
        result = fstat(fh, &buf);

        if (buf.st_size != ( (long)cfg.messagek * 1024l))
        {
            resizeMsg = TRUE;
            printf("Must resize msg.dat");
            doccr();

            /* set messagek to actual value */
            cfg.messagek = (int)(buf.st_size / 1024l);
        }

        close(fh);
    }

    fh = open(logFile, O_RDONLY);

    if (fh != -1)
    {
        result = fstat(fh, &buf);

        if (buf.st_size != ( (long)cfg.MAXLOGTAB *  (long)sizeof logBuf))
        {
            resizeLog = TRUE;
            printf("Must resize log.dat");
            doccr();

            /* set MAXLOGTAB to actual value */
            cfg.MAXLOGTAB = (int)(buf.st_size / (long)sizeof logBuf);
        }
        close(fh);
    }
    if (resizeMsg || resizeLog)  pause(200);
}

/* -------------------------------------------------------------------- */
/*  resize_putMessage()    stores a message to disk                     */
/* -------------------------------------------------------------------- */
BOOL resize_putMessage(void)
{
    /* record start of message to be noted */
    msgBuf->mbheadLoc = (long)cfg.catLoc;

    /* tell putMsgChar where to write   */
    fseek(msgfl, cfg.catLoc, 0);
 
    /* start-of-message              */
    overwrite(1);
    putMsgChar((char)0xFF);

    /* write room #                  */
    overwrite(1);
    putMsgChar(msgBuf->mbroomno);

    /* write attribute byte  */
    overwrite(1);
    putMsgChar(msgBuf->mbattr);  

    /* write message ID */
    dPrintf("%s", msgBuf->mbId);         

    if (msgBuf->mbauth[0])   { dPrintf("A%s", msgBuf->mbauth);      }
    if (msgBuf->mbsub[0])    { dPrintf("B%s", msgBuf->mbsub);       }
    if (msgBuf->mbcopy[0])   { dPrintf("C%s", msgBuf->mbcopy);      }
    if (msgBuf->mbtime[0])   { dPrintf("D%s", msgBuf->mbtime);      }
    if (msgBuf->mbfwd[0])    { dPrintf("F%s", msgBuf->mbfwd);       }
    if (msgBuf->mbgroup[0])  { dPrintf("G%s", msgBuf->mbgroup);     }
    if (msgBuf->mbreply[0])  { dPrintf("I%s", msgBuf->mbreply);     }
    if (msgBuf->mbcreg[0])   { dPrintf("J%s", msgBuf->mbcreg);      }
    if (msgBuf->mbccont[0])  { dPrintf("j%s", msgBuf->mbccont);     }
    if (msgBuf->mblink[0])   { dPrintf("L%s", msgBuf->mblink);      }
    if (msgBuf->mbtitle[0])  { dPrintf("N%s", msgBuf->mbtitle);     }
    if (msgBuf->mbsur[0])    { dPrintf("n%s", msgBuf->mbsur);       }
    if (msgBuf->mboname[0])  { dPrintf("O%s", msgBuf->mboname);     }
    if (msgBuf->mboreg[0])   { dPrintf("o%s", msgBuf->mboreg);      }
    if (msgBuf->mbfpath[0])  { dPrintf("P%s", msgBuf->mbfpath);     }
    if (msgBuf->mbtpath[0])  { dPrintf("p%s", msgBuf->mbtpath);     }
    if (msgBuf->mbocont[0])  { dPrintf("Q%s", msgBuf->mbocont);     }
    if (msgBuf->mbczip[0])   { dPrintf("q%s", msgBuf->mbczip);      }
    if (msgBuf->mbroom[0])   { dPrintf("R%s", msgBuf->mbroom);      }
    if (msgBuf->mbsrcId[0])  { dPrintf("S%s", msgBuf->mbsrcId);     }
    if (msgBuf->mbsoft[0])   { dPrintf("s%s", msgBuf->mbsoft);      }
    if (msgBuf->mbto[0])     { dPrintf("T%s", msgBuf->mbto);        }
    if (msgBuf->mbx[0])      { dPrintf("X%s", msgBuf->mbx);         }
    if (msgBuf->mbzip[0])    { dPrintf("Z%s", msgBuf->mbzip);       }
    if (msgBuf->mbrzip[0])   { dPrintf("z%s", msgBuf->mbrzip);      }
    if (msgBuf->mbsig[0])    { dPrintf(".%s", msgBuf->mbsig);       }
    if (msgBuf->mbusig[0])   { dPrintf("_%s", msgBuf->mbusig);      }

    /* M-for-message. */
    overwrite(1);
    putMsgChar('M'); putMsgStr(msgBuf->mbtext);

    /* now finish writing */
    fflush(msgfl);

    /* record where to begin writing next message */
    cfg.catLoc = ftell(msgfl);

    return  TRUE;
}


