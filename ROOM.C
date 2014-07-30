/************************************************************************/
/*                               room.c                                 */
/*              room code for Citadel bulletin board system             */
/************************************************************************/

#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      canseeroom()            returns TRUE if user can see a room     */
/*      dumpRoom()              tells us # new messages etc             */
/*      gotoRoom()              handles "g(oto)" command for menu       */
/*      indexslot()             returns index slot# for a message#      */
/*      listRooms()             lists known rooms                       */
/*      partialExist()          returns slot# of partially named room   */
/*      printroom()             displays name of specified room         */
/*      roomdescription()       prints out room description             */
/*      roomExists()            returns slot# of named room else ERROR  */
/*      roomtalley()            talleys up total,messages & new         */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*      canseeroom() returns TRUE if user has access to room            */
/************************************************************************/
int roomPosSlot(int room)
{
    int i;
    
    for (i=0; i<MAXROOMS; i++)
    {
        if (roomPos[i] == (uchar)room)
            return (i);
    }

    crashout("Room missing in table!");
    
    return 0;
}

/************************************************************************/
/*      canseeroom() returns TRUE if user has access to room            */
/************************************************************************/
int canseeroom(int roomslot)
{ 
    if ( 
        /* is room in use              */
            roomTab[roomslot].rtflags.INUSE
        /* and room's in this hall     */
        &&  roominhall(roomslot)
    
        /* and group can see this room */
        &&  groupseesroom(roomslot)
    
        /* only aides go to aide room  */ 
        &&   ( roomslot != AIDEROOM || aide) )
        return(TRUE);

    return(FALSE);
}

/************************************************************************/
/*      dumpRoom() tells us # new messages etc                          */
/************************************************************************/
void dumpRoom(void)
{
    int   total, messages, new;
    
    total    = talleyBuf->room[thisRoom].total;
    messages = talleyBuf->room[thisRoom].messages;
    new      = talleyBuf->room[thisRoom].new;
    
    doCR();

    if (    cfg.roomtell 
         && roomBuf.rbroomtell[0] 
         && logBuf.lbroom[thisRoom].lvisit
       )
    {
        roomdescription();
    }
    else if (*roomBuf.descript)
    {
        mPrintf(" 3%s0", roomBuf.descript);
        doCR();
    }

    if (aide)
    {
        mPrintf(" %d total,",  total);
    }

    mPrintf(" %d %s", messages, (messages == 1)? cfg.msg_nym: cfg.msgs_nym);

    if (new)
    {
        mPrintf(", 4%d new0", new);
    }
    
    mPrintf(".");

    if /* (logBuf.lbroom[thisRoom].mail) */
        (talleyBuf->room[thisRoom].hasmail)
    {
        doCR();
        mPrintf(" 3You have Exclusive message(s) here.0");
    }
    
    doCR();
}

/***********************************************************************/
/*     listRooms() lists known rooms                                   */
/***********************************************************************/
void listRooms(unsigned int what, char verbose, char numMess)
{
    int   i, j; 
    char  firstime;
    char  string[NAMESIZE+NAMESIZE];
    label groupname;
    int   groupslot;
    
    if (what == LIMRMS)
    {
        getNormStr("group", groupname, NAMESIZE, ECHO);
        groupslot = partialgroup(groupname);
        if ( groupslot != ERROR && !ingroup(groupslot) )
        {
            groupslot = ERROR;
        }
        if (groupslot != ERROR)
        {
            doCR();
            mPrintf("Rooms for %s group.", grpBuf.group[groupslot].groupname);
            doCR();
        }
    }

    outFlag = OUTOK;

    showdir    = 0;
    showhidden = 0;
    showgroup  = 0;

    /* criteria for NEW rooms */
 
    if (what == NEWRMS || what == OLDNEW)
    {
        termCap(TERM_BOLD);
        mPrintf("\n Rooms with unread %s along %s:", cfg.msgs_nym,
                hallBuf->hall[thisHall].hallname);
        termCap(TERM_NORMAL);
        doCR();
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&  (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen)
            &&   talleyBuf->room[roomPos[i]].new
            &&  !logBuf.lbroom[roomPos[i]].xclude )
            {
                printroomVer(roomPos[i], verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;

    /* for dir rooms */

    if (what == DIRRMS || what == APLRMS || what == LIMRMS || what == SHRDRM 
        || what == NOTSHRDRM)
    {
        firstime = TRUE;

        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&  (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen)
            &&  ( (roomTab[roomPos[i]].rtflags.MSDOSDIR  && what == DIRRMS)
               || (roomTab[roomPos[i]].rtflags.APLIC     && what == APLRMS)
               || (roomTab[roomPos[i]].rtflags.SHARED    && what == SHRDRM)
               || (!roomTab[roomPos[i]].rtflags.SHARED   && what == NOTSHRDRM)
               || (roomTab[roomPos[i]].rtflags.GROUPONLY && what == LIMRMS
     && (groupslot == ERROR || roomTab[roomPos[i]].grpno == (uchar)groupslot) )
               ) )
            {
                if (firstime)
                {
                    termCap(TERM_BOLD);
                    mPrintf("\n %s room:", 
                       what == DIRRMS ?    "Directory"      :
                       what == LIMRMS ?    "Limited Access" :
                       what == NOTSHRDRM ? "Local"          :
                       what == SHRDRM ?    "Shared"         : "Application");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                    prtList(LIST_START);
                }
                printroomVer(roomPos[i], verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
 
    /* criteria for MAIL rooms */

    if (what == NEWRMS || what == OLDNEW || what == MAILRM) 
    {
        firstime = TRUE;
 
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&   (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen)
            &&  /* logBuf.lbroom[roomPos[i]].mail */
                  talleyBuf->room[roomPos[i]].hasmail
              )
            {
                if (firstime)
                {
                    termCap(TERM_BOLD);
                    mPrintf("\n You have exclusive message(s) in:");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                    prtList(LIST_START);
                }
                printroomVer(roomPos[i], verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
 
    /* criteria for OLD rooms */
 
    if (what == OLDNEW || what == OLDRMS)
    {
        termCap(TERM_BOLD);
        mPrintf("\n No unseen %s in:", cfg.msgs_nym);
        termCap(TERM_NORMAL);
        doCR();
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&  (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen)
            &&  !talleyBuf->room[roomPos[i]].new )
            {
                printroomVer(roomPos[i], verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;
 
    /* criteria for EXCLUDED rooms */
 
    if (what == OLDNEW || what == XCLRMS)
    {
        firstime = TRUE;
 
        prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&   (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen) 
            &&   logBuf.lbroom[roomPos[i]].xclude
            &&   (talleyBuf->room[roomPos[i]].new || what == XCLRMS) )
            {
                if (firstime) 
                {
                    termCap(TERM_BOLD);
                    mPrintf("\n Excluded rooms:");
                    termCap(TERM_NORMAL);
                    doCR();

                    firstime = FALSE;
                }
                printroomVer(roomPos[i], verbose, numMess);
            }
        }
        prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;

    /* criteria for WINDOWS */
 
    if ( what == WINDWS )
    {
        termCap(TERM_BOLD);
        mPrintf("\n Rooms exiting to other halls:");
        termCap(TERM_NORMAL);
        doCR();
        
        if (!verbose) prtList(LIST_START);
        for ( i = 0; i < MAXROOMS && (outFlag != OUTSKIP); i++)
        {
            if ( canseeroom(roomPos[i])
            &&   (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen) 
            &&   iswindow(roomPos[i]) )
            {
                if (verbose)
                {
                    mPrintf("  %s : ", makeRoomName(roomPos[i], string));
                
                }
                else
                {
                    printroomVer(roomPos[i], FALSE, FALSE);
                }

                if (verbose)
                {
                    prtList(LIST_START);
                    
                    for (j = 0 ; j < MAXHALLS; j++)
                    {
                        if ( hallBuf->hall[j].hroomflags[roomPos[i]].window 
                        &&   hallBuf->hall[j].h_inuse
                        &&   groupseeshall(j) )
                        {
                            prtList(hallBuf->hall[j].hallname);
                        }
                    }
                    prtList(LIST_END);
                }
            }
        }
        if (!verbose) prtList(LIST_END);
    }
    if (outFlag == OUTSKIP) return;

    if (!expert)
    {
        termCap(TERM_BOLD);
        if (showhidden)                   mPrintf("\n ) => hidden room"    );
        if (showgroup)                    mPrintf("\n : => group only room");
        if (showdir && what != DIRRMS)    mPrintf("\n ] => directory room" );
        termCap(TERM_NORMAL);
    }
}

/* ------------------------------------------------------------------------ */
/*  RoomStatus() Shows the status of a room...                              */
/* ------------------------------------------------------------------------ */
void RoomStatus(void)
{
    char buff[500];
    int j;
    
    doCR();
    mPrintf(" Hallway %s", hallBuf->hall[thisHall].hallname);
    if (hallBuf->hall[thisHall].owned) 
    {
        mPrintf(", owned by group %s",
            grpBuf.group[ hallBuf->hall[thisHall].grpno ].groupname);
    }
    doCR();
    
    doCR();
    formatSummary(buff);
    mPrintf(buff);
    doCR();
    doCR();
   
    mPrintf("Windowed in Halls:");
    doCR();
  
    prtList(LIST_START);
    for (j = 0; j < MAXHALLS; j++)
    {
        if ( hallBuf->hall[j].hroomflags[thisRoom].window 
          && hallBuf->hall[j].h_inuse
          && groupseeshall(j) )
        {
            prtList(hallBuf->hall[j].hallname);
        }
    }
    prtList(LIST_END);
    readhalls();
    doCR();
} 

/************************************************************************/
/*      partialExist() the list looking for a partial match             */
/************************************************************************/
int partialExist(char *roomname)
{ 
    int i, j, length;

    length = strlen(roomname);

    j = thisRoom + 1;

    for (i = 0; i < MAXROOMS; ++i, ++j)
    {
        if ( j == MAXROOMS ) j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
        {
            if ((strnicmp(roomTab[j].rtname, roomname, length) == SAMESTRING) 
                && roominhall(j)
                && canseeroom(j) )
                return(j);
        } 
    }

    for (i = 0, j = thisRoom + 1; i < MAXROOMS; ++i, ++j)
    {
        if ( j == MAXROOMS ) j = 0;

        if (roomTab[j].rtflags.INUSE &&
        (roomTab[j].rtgen == logBuf.lbroom[j].lbgen) )
        {
            if (substr(roomTab[j].rtname, roomname)
                && roominhall(j)
                && canseeroom(j) )
                return(j);
        } 
    }
    return(ERROR);
}

/***********************************************************************/
/*     printroom()  displays name of specified room.                   */
/*     Called by listRooms  Sets global flags.                         */
/***********************************************************************/
void printrm(int room)
{  
    char string[NAMESIZE + 10];

    if  (roomTab[room].rtflags.SHARED)
    {
        strcpy(string, cfg.netPrefix);
    }
    else
    {
        string[0] = '\0';
    }
    
    strcat(string, roomTab[room].rtname);

    if  (roomTab[room].rtflags.MSDOSDIR)
        strcat(string, "]");
    if  (roomTab[room].rtflags.GROUPONLY)
        strcat(string, ":");
    if (!roomTab[room].rtflags.PUBLIC)
        strcat(string, ")");
    else if (!roomTab[room].rtflags.GROUPONLY && 
             !roomTab[room].rtflags.MSDOSDIR)
        strcat(string, ">");
    if (iswindow(room)) strcat(string, ">");

    if ( roomTab[room].rtflags.GROUPONLY)showgroup   = TRUE;
    if ( roomTab[room].rtflags.MSDOSDIR) showdir     = TRUE;
    if (!roomTab[room].rtflags.PUBLIC)   showhidden  = TRUE;

    mPrintf("%s",string);
}

/***********************************************************************/
/*     printroom()  displays name of specified room.                   */
/*     Called by listRooms  Sets global flags.                         */
/***********************************************************************/
void printroomVer(int room, int verbose, char numMess)
{  
    char string[NAMESIZE+NAMESIZE];
    int oldRoom;

    makeRoomName(room, string);

    if ( roomTab[room].rtflags.GROUPONLY)showgroup   = TRUE;
    if ( roomTab[room].rtflags.MSDOSDIR) showdir     = TRUE;
    if (!roomTab[room].rtflags.PUBLIC)   showhidden  = TRUE;

    if (!verbose && !numMess)
    {
        /*
        mPrintf("%s ",string);
        */
        prtList(string);
    }
    else
    {
        oldRoom = thisRoom;
        getRoom(room);
        mPrintf("3%-33s0 ", string);

        if (numMess)
        {
            if (aide)
            {
                mPrintf("%3d total, ", talleyBuf->room[room].total);
            }
            mPrintf("%3d %s, %3d new", talleyBuf->room[room].messages,
                                       cfg.msgs_nym,
                                       talleyBuf->room[room].new );

            if (talleyBuf->room[room].new  && /* logBuf.lbroom[room].mail */
                talleyBuf->room[room].hasmail
                )
            {
                mPrintf(", (Mail)");
            }
            
            doCR();

            if (*roomBuf.descript && verbose)
            {
                mPrintf("    ");
            }
        }

        if (verbose)
        {
            if (*roomBuf.descript)
                mPrintf("%s", roomBuf.descript);
            if (!numMess || *roomBuf.descript)
                doCR();
        }

        if (verbose && numMess)  doCR();

        getRoom(oldRoom);
    }
}

/* -------------------------------------------------------------------- */
/*  makeRoomName()  Room name...                                        */
/* -------------------------------------------------------------------- */
char *makeRoomName(int room, char *string)
{
    if  (roomTab[room].rtflags.SHARED)
    {
        strcpy(string, cfg.netPrefix);
    }
    else
    {
        string[0] = '\0';
    }
    
    strcat(string, roomTab[room].rtname);

    if  (roomTab[room].rtflags.MSDOSDIR)
        strcat(string, "]");
    if  (roomTab[room].rtflags.GROUPONLY)
        strcat(string, ":");
    if (!roomTab[room].rtflags.PUBLIC)
        strcat(string, ")");
    else if (!roomTab[room].rtflags.GROUPONLY && 
             !roomTab[room].rtflags.MSDOSDIR)
        strcat(string, ">");
    if (iswindow(room)) strcat(string, ">");

    return string;
}

/************************************************************************/
/*      roomdescription()  prints out room description                  */
/************************************************************************/
void roomdescription(void)
{
    outFlag     = OUTOK;

    if (!roomtell) return;

    if (!roomBuf.rbroomtell[0]) return;

    /* only do room description upon first visit this call to a room    */
    if (!logBuf.lbroom[thisRoom].lvisit) return;

    if (changedir(cfg.roompath) == -1 ) return;

    /* no bad files */
    if (checkfilename(roomBuf.rbroomtell, 0) == ERROR)
    {
        changedir(cfg.homepath);
        return;
    }

    if (!filexists(roomBuf.rbroomtell))
    {
        mPrintf("No room description %s", roomBuf.rbroomtell);
        doCR();
        doCR();
        changedir(cfg.homepath);
        return;
    }

    if (!expert) 
    {
        doCR();
        mPrintf(" <J>ump <N>ext <P>ause <S>top");
        doCR();
    }

    /* print it out */
    dumpf(roomBuf.rbroomtell);

    /* go to our home-path */
    changedir(cfg.homepath);

    outFlag = OUTOK;

    doCR();
}

/************************************************************************/
/*      roomExists() returns slot# of named room else ERROR             */
/************************************************************************/
int roomExists(char *room)
{
    int i;

    for (i = 0;  i < MAXROOMS;  i++)
    {
        if (roomTab[i].rtflags.INUSE == 1   &&
            strcmpi(room, roomTab[i].rtname) == SAMESTRING )
        {
            return(i);
        }
    }
    return(ERROR);
}


/************************************************************************/
/*   roomtalley()  talleys up total,messages & new for every room       */
/************************************************************************/
void roomtalley(void)
{
    register int i;
    int          num;
    register int room;
    int          slot;

    for (room = 0; room < MAXROOMS; room++)
    {
        talleyBuf->room[room].total    = 0;
        talleyBuf->room[room].messages = 0;
        talleyBuf->room[room].new      = 0;
        talleyBuf->room[room].hasmail  = 0;
    }

    num = sizetable();

    for (i = 0, slot=0; i < num; ++i, slot++)
    {
        room = msgTab4[slot].mtroomno;

        if (!msgTab1[slot].mtmsgflags.COPY)
            talleyBuf->room[room].total++;
        else
        {
            if (msgTab8[slot].mtomesg <= (uint)i)
                talleyBuf->room[room].total++;

        }

        if (mayseeindexmsg(i))
        {
            talleyBuf->room[room].messages++;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
            {
                talleyBuf->room[room].new++;

                /* check to see if its private mail and set flag if so */
                if (msgTab1[i].mtmsgflags.MAIL)
                    talleyBuf->room[room].hasmail = TRUE;
            }
        }
    }
}

/************************************************************************/
/*      givePrompt() prints the usual "CURRENTROOM>" prompt.            */
/************************************************************************/
void givePrompt(void)
{
    int dummy;

    Mflush();  /* eat up excess garbage */

    outFlag   = IMPERVIOUS;
    echo      = BOTH;
    onConsole = (char)(whichIO == CONSOLE);

    ansiattr = cfg.attr;

    doCR();

    if (roomTab[thisRoom].rtflags.SHARED) 
        mPrintf(cfg.netPrefix);

    /* turn off net-prefix for known rooms listing */
    dummy = roomTab[thisRoom].rtflags.SHARED;
    roomTab[thisRoom].rtflags.SHARED = 0;

    termCap(TERM_REVERSE); 
    printrm(thisRoom);
    termCap(TERM_NORMAL); 
    mPrintf(" ");

    roomTab[thisRoom].rtflags.SHARED = dummy;

    if (strcmp(roomBuf.rbname, roomTab[thisRoom].rtname) != SAMESTRING)
    {
        crashout("Room table mismatch!");
    }
    outFlag = OUTOK;
}

/************************************************************************/
/*      noteRoom() -- enter room into RAM index array.                  */
/************************************************************************/
void noteRoom(void)
{
    strcpy(roomTab[thisRoom].rtname     , roomBuf.rbname) ;
    roomTab[thisRoom].rtgen             = roomBuf.rbgen   ;
    roomTab[thisRoom].grpno             = roomBuf.rbgrpno ;
 /* roomTab[thisRoom].grpgen            = roomBuf.rbgrpgen; */

    /* dont YOU like ansi C? */
    roomTab[thisRoom].rtflags           = roomBuf.rbflags;
}

/************************************************************************/
/*     stepRoom()  1 for next 0, 0 for previous room                    */
/************************************************************************/
void stepRoom(int direction)
{
    int i, rp;
    char done = 0;

    i = thisRoom;
    oldroom   = thisRoom;
    ug_hall   = thisHall;
    ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
    ug_bypass = logBuf.lbroom[thisRoom].bypass;
    ug_new    = talleyBuf->room[thisRoom].new;

    logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen;
    logBuf.lbroom[thisRoom].lvisit   = 0;

    /* logBuf.lbroom[thisRoom].mail     = 0; */
    talleyBuf->room[thisRoom].hasmail  = 0;

    /* zero new count in talleybuffer */
    talleyBuf->room[thisRoom].new  = 0;

    rp = roomPosSlot(thisRoom);
    i = rp;
    
    do
    {
        if (direction == 1) ++i;
             else           --i;

              if ( (direction == 1) && (i == MAXROOMS) ) i = 0; 
        else  if ( (direction == 0) && (i == -1      ) ) i = MAXROOMS - 1;

        if ( (canseeroom(roomPos[i]) || i == rp)
        /* and is it K>nown                             */
        /* sysops can plus their way into hidden rooms! */
        && ( (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen)
        ||   logBuf.lbflags.SYSOP ))
        {
            mPrintf("%s", roomTab[roomPos[i]].rtname); doCR();

            getRoom(roomPos[i]);
 
            dumpRoom();

            done = 1;
        }
    }                                              
    while ( done != 1 );
}

/************************************************************************/
/*     unGotoRoom()                                                     */
/************************************************************************/
void unGotoRoom(void)
{
    int i;

    mPrintf("Jump back to ");

    i = oldroom;

    if (hallBuf->hall[ug_hall].h_inuse)
    {
        thisHall = (uchar)ug_hall;
    }
    
    if ( canseeroom(i) && i != thisRoom) 
    {
        mPrintf("%s", roomTab[i].rtname); doCR();

        getRoom(i);

        logBuf.lbroom[thisRoom].bypass  = ug_bypass;
        logBuf.lbroom[thisRoom].lvisit  = ug_lvisit;
        talleyBuf->room[thisRoom].new   = ug_new;

        dumpRoom();
    }                                              
}

/************************************************************************/
/*      gotoRoom() is the menu fn to travel to a new room               */
/*      returns TRUE if room is Lobby>, else FALSE                      */
/************************************************************************/
int gotoRoom(char *roomname)
{
    int  i, j, check, rp;
    int  roomNo  = ERROR;
    int  oldHall = thisHall;
    
    rp = roomPosSlot(thisRoom);
    
    /* set generation number on exit */
    logBuf.lbroom[thisRoom].lbgen    = roomBuf.rbgen; 

    if (!strlen(roomname))
    {
        /* leaves us in Lobby> if nothing found */

        for (i = 0; i < MAXROOMS && roomNo == ERROR; i++)
        {
            /* can user access this room?         */
            if ( canseeroom(roomPos[i])

            /* does it have new messages,         */
            && (  talleyBuf->room[roomPos[i]].new

               /* or is it a window?              */
               || (
                   iswindow(roomPos[i]) && i > rp 
                   && thisHall != 1 && !cfg.floors 
                   && cfg.subhubs==2     /* DragCit-style, empties visited */

                  )
               )


            /* is it NOT excluded                 */
            &&  (!logBuf.lbroom[roomPos[i]].xclude 
                 || /* logBuf.lbroom[roomPos[i]].mail */
                     talleyBuf->room[roomPos[i]].hasmail
                )
            
            /* is it NOT bypassed                 */
            &&  (!logBuf.lbroom[roomPos[i]].bypass)
            
            /* we dont come back to current room  */
            &&   i != rp

            /* and is it K>nown                   */
            && (roomTab[roomPos[i]].rtgen == logBuf.lbroom[roomPos[i]].lbgen) )
            {
                roomNo = roomPos[i];
            }
        }

        if ( roomNo == ERROR && !cfg.floors && cfg.subhubs)
        {
            for (j = rp + 1, i = 0; 
                 j != rp && roomNo == ERROR && i < MAXROOMS; 
                 j++, i++)
            { 
                if (j == MAXROOMS) j = 0;

                if (iswindow(roomPos[j]) 

                /* can user access this room?     */
                && canseeroom(roomPos[j])

                /* is it NOT excluded             */
                &&  !logBuf.lbroom[roomPos[j]].xclude 
                
                /* is it NOT bypassed             */
                &&  !logBuf.lbroom[roomPos[j]].bypass 

                /* and is it K>nown               */
                && (roomTab[roomPos[j]].rtgen 
                    == logBuf.lbroom[roomPos[j]].lbgen) 
                   )
                {
                    roomNo  = roomPos[j];
                }
            }
        }

        if (cfg.floors && roomNo == ERROR)
            roomNo = LOBBY;
            
        if (roomNo != ERROR)
        {
            mPrintf("%s", roomTab[roomNo].rtname, roomTab[roomNo].rtname);
        }
    }
    else
    {
        check = roomExists(roomname);

        if (!canseeroom(check)) check = ERROR;

        if (check == ERROR) check = partialExist(roomname);

        if (check != ERROR && canseeroom(check))
        {
            roomNo = check;
        }
        else
        {
            mPrintf(" No '%s' room\n ", roomname);
        }
    }
    
    if (roomNo != ERROR)
    {
        if (roomNo != thisRoom)
        {
            /*
             * Set flags for ungoto
             */
            oldroom   = thisRoom;
            ug_hall   = oldHall;
            ug_lvisit = logBuf.lbroom[thisRoom].lvisit;
            ug_bypass = logBuf.lbroom[thisRoom].bypass;
            ug_new    = talleyBuf->room[thisRoom].new;
                
            /*
             * If not bypassing room, then clear new, mail, and last visit
             */
            if (!skiproom)
            {
                logBuf.lbroom[thisRoom].lvisit   = 0; 

                talleyBuf->room[thisRoom].hasmail  = 0;
                /*logBuf.lbroom[thisRoom].mail     = 0; */
                
                talleyBuf->room[thisRoom].new     = 0;
            }
            else
            {
                logBuf.lbroom[thisRoom].bypass = TRUE;
            }
        }
        
        /*
         * Get room and display it.
         */
        getRoom(roomNo);
            
        if (!strlen(roomname))    
        {
            if  (
                  (
                    (!cfg.floors && iswindow(roomNo))
                    || 
                    (cfg.floors && roomNo == LOBBY)
                  ) 
                  && 
                  logBuf.NEXTHALL
                )
            {
                mPrintf(", Next Hall: "); 
                stephall(1);
            }
            
            doCR();
        }
        dumpRoom();  
        
        /*
         * May have been unknown... if so, note it:          
         */
        if ((logBuf.lbroom[thisRoom].lbgen ) != roomBuf.rbgen)
        {
            logBuf.lbroom[thisRoom].lbgen  = roomBuf.rbgen;
            logBuf.lbroom[thisRoom].lvisit = (MAXVISIT - 1);
        }
    }

    skiproom = FALSE;
    
    return roomNo == ERROR;
}

