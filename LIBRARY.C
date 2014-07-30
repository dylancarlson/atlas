/************************************************************************/
/*                            library.c                                 */
/*                                                                      */
/*                  Routines used by Ctdl & Confg                       */
/************************************************************************/

/* TURBO C */
#include <direct.h>
#include <malloc.h>

/* Citadel */
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/*      getGroup()              loads groupBuffer                       */
/*      putGroup()              stores groupBuffer to disk              */
/*                                                                      */
/*      getHall()               loads hallBuffer                        */
/*      putHall()               stores hallBuffer to disk               */
/*                                                                      */
/*      getLog()                loads requested CTDLLOG record          */
/*      putLog()                stores a logBuffer into citadel.log     */
/*                                                                      */
/*      getRoom()               load given room into RAM                */
/*      putRoom()               store room to given disk slot           */
/*                                                                      */
/*      writeTables()           writes all system tables to disk        */
/*	readTables()		loads all tables except cron.tab	*/
/*                                                                      */
/*      allocateTables()        allocate table space with halloc        */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*      getGrooup() loads group data into RAM buffer                    */
/************************************************************************/
void getGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fread(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("getGroup-EOF detected!");
    }
}

/************************************************************************/
/*      putGroup() stores group data into grp.cit                       */
/************************************************************************/
void putGroup(void)
{
    fseek(grpfl, 0L, 0);

    if (fwrite(&grpBuf, sizeof grpBuf, 1, grpfl) != 1)
    {
        crashout("putGroup-write fail!");
    }
    fflush(grpfl);
}

/************************************************************************/
/*      getHall() loads hall data into RAM buffer                       */
/************************************************************************/
void getHall(void)
{
    int i;
    
    fseek(hallfl, 0L, 0);

    if (fread(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("getHall-EOF detected!");
    }
    
    if (fread(roomPos, MAXROOMS, 1, hallfl) != 1)
    {
        cPrintf("\nCreating room position table.\n");
        
        for (i=0; i<MAXROOMS; i++)
            roomPos[i] = (char)i;
            
        putHall();
    }
}

/************************************************************************/
/*	putHall() stores hall data into hall.cit			*/
/************************************************************************/
void putHall(void)
{
    fseek(hallfl, 0L, 0);

    if ( fwrite(hallBuf, sizeof (struct hallBuffer), 1, hallfl) != 1)
    {
        crashout("putHall-write fail!");
    }
    
    if ( fwrite(roomPos, MAXROOMS, 1, hallfl) != 1)
    {
        crashout("putHall-write fail!");
    }
    
    fflush(hallfl);
}

/************************************************************************/
/*      getLog() loads requested log record into RAM buffer             */
/************************************************************************/
void getLog(struct logBuffer *lBuf, int n)
{
    long int s;

    if (lBuf == &logBuf)  thisLog = n;

    s = (long)n * (long)sizeof logBuf;

    fseek(logfl, s, 0);

    if (fread(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("getLog-EOF detected!");
    }
}

/************************************************************************/
/*      putLog() stores given log record into log.cit                   */
/************************************************************************/
void putLog(struct logBuffer *lBuf, int n)
{
    long int s;
    int i;
    
    s = (long)n * (long)(sizeof(struct logBuffer));

    fseek(logfl, s, 0);  

    if (fwrite(lBuf, sizeof logBuf, 1, logfl) != 1)
    {
        crashout("putLog-write fail!");
    }
    fflush(logfl);
    
    for ( i = 0;  i < cfg.MAXLOGTAB;  i++)
    {
        if (n == logTab[i].ltlogSlot)
        {
            log2tab(&logTab[i], lBuf);
            logTab[i].ltlogSlot = n;
            break;
        }
    }
}

/************************************************************************/
/*      getRoom()                                                       */
/************************************************************************/
void getRoom(int rm)
{
    long int s;

    /* load room #rm into memory starting at buf */
    thisRoom    = rm;
    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fread(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)  
    {
        crashout("getRoom-EOF detected!");
    }
    
    if (roomBuf.rbflags.MSDOSDIR)
    {
        if (changedir(roomBuf.rbdirname) == -1)
        {
            roomBuf.rbflags.MSDOSDIR = FALSE;
            roomBuf.rbflags.DOWNONLY = FALSE;
        
            noteRoom();
            putRoom(rm);
            
            sprintf(msgBuf->mbtext, 
                    "%s>'s directory unfound.\n Directory: %s\n", 
                    roomBuf.rbname, 
                    roomBuf.rbdirname);
            aideMessage();
        }
    }
}

/************************************************************************/
/*      putRoom() stores room in buf into slot rm in room.buf           */
/************************************************************************/
void putRoom(int rm)
{
    long int s;

    s = (long)rm * (long)sizeof roomBuf;

    fseek(roomfl, s, 0);
    if (fwrite(&roomBuf, sizeof roomBuf, 1, roomfl) != 1)
    {
        crashout("putRoom-write failed!");
    }
    fflush(roomfl);
}

/************************************************************************/
/*	readTables()  loads most tables into ram			*/
/*		      cron.tab is read by readcron() in cron.c		*/
/************************************************************************/
readTables()
{
    FILE  *fd;

    getcwd(etcpath, 64);

    /*
     * ETC.DAT
     */
    if ((fd  = fopen("etc.dat" , "rb")) == NULL)
        return(FALSE);
    if (!fread(&cfg, sizeof cfg, 1, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("etc.dat");

    changedir(cfg.homepath);

    allocateTables();

    if (logTab == NULL)
        crashout("Error allocating logTab \n");
    if (msgTab1 == NULL || msgTab2 == NULL || /* msgTab3 == NULL || */
        msgTab4 == NULL || msgTab5 == NULL || msgTab6 == NULL ||
        msgTab7 == NULL || msgTab8 == NULL /*|| msgTab9 == NULL */)
        crashout("Error allocating msgTab \n");

    /*
     * LOG.TAB
     */
    if ((fd  = fopen("log.tab" , "rb")) == NULL)
        return(FALSE);
    if (!fread(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("log.tab" );

    /*
     * MSG.TAB
     */
    if (readMsgTab() == FALSE)  return FALSE;

    /*
     * ROOM.TAB
     */
    if ((fd = fopen("room.tab", "rb")) == NULL)
        return(FALSE);
    if (!fread(roomTab, sizeof(struct rTable), MAXROOMS, fd))
    {
        fclose(fd);
        return FALSE;
    }
    fclose(fd);
    unlink("room.tab");

    return(TRUE);
}

/************************************************************************/
/*      writeTables()  stores all tables to disk                        */
/************************************************************************/
void writeTables(void)
{
    FILE  *fd;
    int i;

    changedir(etcpath);

    if ((fd     = fopen("etc.dat" , "wb")) == NULL)
    {
	crashout("Can't make ETC.DAT");
    }
    /* write out ETC.DAT */
    fwrite(&cfg, sizeof cfg, 1, fd);
    fclose(fd);

    changedir(cfg.homepath);

    if ((fd  = fopen("log.tab" , "wb")) == NULL)
    {
	crashout("Can't make LOG.TAB");
    }
    /* write out LOG.TAB */
    fwrite(logTab, sizeof(struct lTable), cfg.MAXLOGTAB, fd);
    fclose(fd);
 
    writeMsgTab();

    if ((fd = fopen("room.tab", "wb")) == NULL)
    {
	crashout("Can't make ROOM.TAB");
    }
    /* write out ROOM.TAB */
    fwrite(roomTab, sizeof(struct rTable), MAXROOMS, fd);
    fclose(fd);

#ifdef CRON
    writecrontab();
#endif

    changedir(etcpath);

}


/************************************************************************/
/*    allocateTables()   allocate msgTab and logTab                     */
/************************************************************************/
void allocateTables(void)
{
    logTab =  _fcalloc(cfg.MAXLOGTAB+1, sizeof(struct lTable));
    msgTab1 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable1));
    msgTab2 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable2));
/*  msgTab3 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable3)); */
    msgTab4 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable4));
    msgTab5 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable5));
    msgTab6 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable6));
    msgTab7 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable7));
    msgTab8 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable8));
/*  msgTab9 = _fcalloc(cfg.nmessages+1, sizeof(struct messagetable9)); */
}



/* -------------------------------------------------------------------- */
/*  readMsgTable()     Avoid the 64K limit. (stupid segments)           */
/* -------------------------------------------------------------------- */
int readMsgTab(void)
{
    FILE *fd;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    if ((fd  = fopen(temp , "rb")) == NULL)
        return(FALSE);

    if (!fread(msgTab1, sizeof(*msgTab1), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab2, sizeof(*msgTab2), cfg.nmessages, fd)) return(FALSE);
/*  if (!fread(msgTab3, sizeof(*msgTab3), cfg.nmessages, fd)) return(FALSE);*/
    if (!fread(msgTab4, sizeof(*msgTab4), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab5, sizeof(*msgTab5), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab6, sizeof(*msgTab6), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab7, sizeof(*msgTab7), cfg.nmessages, fd)) return(FALSE);
    if (!fread(msgTab8, sizeof(*msgTab8), cfg.nmessages, fd)) return(FALSE);
/*  if (!fread(msgTab9, sizeof(*msgTab9), cfg.nmessages, fd)) return(FALSE);*/
    
    fclose(fd);
    unlink(temp);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  writeMsgTable()     Avoid the 64K limit. (stupid segments)          */
/* -------------------------------------------------------------------- */
void writeMsgTab(void)
{
    FILE *fd;
    char temp[80];

    sprintf(temp, "%s\\%s", cfg.homepath, "msg.tab");

    if ((fd  = fopen(temp , "wb")) == NULL)
        return;

    fwrite(msgTab1, sizeof(*msgTab1), cfg.nmessages , fd);
    fwrite(msgTab2, sizeof(*msgTab2), cfg.nmessages , fd);
/*  fwrite(msgTab3, sizeof(*msgTab3), cfg.nmessages , fd); */
    fwrite(msgTab4, sizeof(*msgTab4), cfg.nmessages , fd);
    fwrite(msgTab5, sizeof(*msgTab5), cfg.nmessages , fd);
    fwrite(msgTab6, sizeof(*msgTab6), cfg.nmessages , fd);
    fwrite(msgTab7, sizeof(*msgTab7), cfg.nmessages , fd);
    fwrite(msgTab8, sizeof(*msgTab8), cfg.nmessages , fd);
/*  fwrite(msgTab9, sizeof(*msgTab9), cfg.nmessages , fd); */

    fclose(fd);
}
