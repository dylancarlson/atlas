/* -------------------------------------------------------------------- */
/*  MSG3.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                       Overlayed message code                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  copymessage()   copies specified message # into specified room      */
/*  deleteMessage() deletes message for pullIt()                        */
/*  insert()        aide fn: to insert a message                        */
/*  makeMessage()   is menu-level routine to enter a message            */
/*  markIt()        is a sysop special to mark current message          */
/*  markmsg()       marks a message for insertion and/or visibility     */
/*  pullIt()        is a sysop special to remove a message from a room  */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/*  copyindex()     copies msg index source to message index dest w/o   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  02/26/91    (PAT)   Rearanged message code for overlays.            */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  copymessage()   copies specified message # into specified room      */
/* -------------------------------------------------------------------- */
void copymessage(ulong id, uchar roomno)
{
    unsigned char attr;
    char copy[20];
    int slot;

    slot = indexslot(id);

    /* load in message to be inserted */
    fseek(msgfl, msgTab2[slot].mtmsgLoc, 0);
    getMessage();

    /* retain vital information */
    attr    = msgBuf->mbattr;
    strcpy(copy, msgBuf->mbId);
    
    clearmsgbuf();
    msgBuf->mbtext[0] = '\0';

    strcpy(msgBuf->mbcopy, copy);
    msgBuf->mbattr   = attr;
    msgBuf->mbroomno = roomno;    
    
    putMessage();
    noteMessage();
}

/* -------------------------------------------------------------------- */
/*  deleteMessage() deletes message for pullIt()                        */
/* -------------------------------------------------------------------- */
void deleteMessage(void)
{
    ulong id;

    id = originalId;

    if (!(*msgBuf->mbx))
    {
        markmsg();    /* Mark it for possible insertion elsewhere */
    }
    
    changeheader(id, DUMP, 255);
    
    if (thisRoom != AIDEROOM && thisRoom != DUMP)
    {
        /* note in Aide): */
        sprintf(msgBuf->mbtext, "Following %s deleted by %s:",
                cfg.msg_nym, logBuf.lbname);

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

        copymessage(id, AIDEROOM); 
        if (!logBuf.lbroom[AIDEROOM].lvisit)
            talleyBuf->room[AIDEROOM].new--;
    }
}

/* -------------------------------------------------------------------- */
/*  insert()        aide fn: to insert a message                        */
/* -------------------------------------------------------------------- */
void insert(void)
{
    if ( thisRoom   == AIDEROOM  ||  markedMId == 0l )
    {
        mPrintf("Not here.");
        return;
    }
    copymessage(markedMId, (uchar)thisRoom); 
    
    sprintf(msgBuf->mbtext, "Following %s inserted in %s> by %s",
        cfg.msg_nym, roomBuf.rbname, logBuf.lbname );

    trap(msgBuf->mbtext, T_AIDE);

    aideMessage();

    copymessage(markedMId, AIDEROOM); 
    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf->room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  markIt()        is a sysop special to mark current message          */
/* -------------------------------------------------------------------- */
BOOL markIt(void)
{
    ulong id;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* confirm that we're marking the right one: */
    outFlag = OUTOK;
    printMessage( id, (char)0 );

    outFlag = OUTOK;

    doCR();

    if (getYesNo("Mark",1)) 
    {
        markmsg();
        return TRUE;
    }
    else return FALSE;
}

/* -------------------------------------------------------------------- */
/*  markmsg()       marks a message for insertion and/or visibility     */
/* -------------------------------------------------------------------- */
void markmsg(void)
{
    ulong id;
    uchar attr;

    markedMId = originalId;
    id        = originalId;

    if (msgBuf->mbx[0])
    {
        if (!copyflag)  attr = msgBuf->mbattr;
        else            attr = originalattr;

        attr = (uchar)(attr ^ ATTR_MADEVIS);

        if (!copyflag)  msgBuf->mbattr = attr;
        else            originalattr  = attr;

        changeheader(id, 255, attr);

        if ((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
            copymessage( id, (uchar)thisRoom);
    }
}


/* -------------------------------------------------------------------- */
/*  pullIt()        is a sysop special to remove a message from a room  */
/* -------------------------------------------------------------------- */
BOOL pullIt(void)
{
    ulong id;

    id = originalId;
    
    /* confirm that we're removing the right one: */
    outFlag = OUTOK;

    printMessage( id,  (char)0 );

    outFlag = OUTOK;

    doCR();

    if (getYesNo("Pull",0)) 
    {
        deleteMessage();
        return TRUE;
    }
    else return FALSE;
}


/* -------------------------------------------------------------------- */
/*  changeheader()  Alters room# or attr byte in message base & index   */
/* -------------------------------------------------------------------- */
void changeheader(ulong id, uchar roomno, uchar attr)
{
    long loc;
    int  slot;
    int  c;
    long pos;
    int  room;

    pos = ftell(msgfl);
    slot = indexslot(id);

    /*
     * Change the room # for the message
     */
    if (roomno != 255)
    {
        /* determine room # of message to be changed */
        room = msgTab4[slot].mtroomno;

        /* fix the message tallys from */
        talleyBuf->room[room].total--;
        if (mayseeindexmsg(slot))
        {
            talleyBuf->room[room].messages--;
            if  ((ulong)(cfg.mtoldest + slot) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf->room[room].new--;
        }

        /* fix room tallys to */
        talleyBuf->room[roomno].total++;
        if (mayseeindexmsg(slot))
        {
            talleyBuf->room[roomno].messages++;
            if  ((ulong)(cfg.mtoldest + slot) >
                logBuf.lbvisit[ logBuf.lbroom[roomno].lvisit ])
                talleyBuf->room[room].new++;
        }
    }

    loc  = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    fseek(msgfl, loc, SEEK_SET);

    /* find start of message */
    do c = getMsgChar(); while (c != 0xFF);

    if (roomno != 255)
    {
        overwrite(1);
        /* write room #    */
        putMsgChar(roomno);

        msgTab4[slot].mtroomno = roomno;
    }
    else
    {
        getMsgChar();
    }

    if (attr != 255)
    {
        overwrite(1);
        /* write attribute */
        putMsgChar(attr);  

        msgTab1[slot].mtmsgflags.RECEIVED
            = ((attr & ATTR_RECEIVED) == ATTR_RECEIVED);

        msgTab1[slot].mtmsgflags.REPLY
            = ((attr & ATTR_REPLY)    == ATTR_REPLY   );

        msgTab1[slot].mtmsgflags.MADEVIS
            = ((attr & ATTR_MADEVIS)  == ATTR_MADEVIS );
    }

    fseek(msgfl, pos, SEEK_SET);
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  copyindex()     copies msg index source to message index dest w/o   */
/*                  certain fields (attr, room#)                        */
/* -------------------------------------------------------------------- */
void copyindex(int dest, int source)
{
    msgTab5[dest].mttohash             =     msgTab5[source].mttohash;
    msgTab8[dest].mtomesg              =     msgTab8[source].mtomesg;
/*  msgTab9[dest].mtorigin             =     msgTab9[source].mtorigin; */
    msgTab6[dest].mtauthhash           =     msgTab6[source].mtauthhash;
    msgTab7[dest].mtfwdhash            =     msgTab7[source].mtfwdhash;
    msgTab1[dest].mtmsgflags.MAIL      =     msgTab1[source].mtmsgflags.MAIL;
    msgTab1[dest].mtmsgflags.LIMITED   =     msgTab1[source].mtmsgflags.LIMITED;
    msgTab1[dest].mtmsgflags.PROBLEM   =     msgTab1[source].mtmsgflags.PROBLEM;

    msgTab1[dest].mtmsgflags.COPY    = TRUE;
}
#endif
