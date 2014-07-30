#ifndef MULTICIT

/* -------------------------------------------------------------------- */
/*  MSGMAKE.C                Dragon Citadel                             */
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
/*  makeMessage()   is menu-level routine to enter a message            */
/*  aideMessage()   save auto message in Aide>                          */
/*  specialMessage()    saves a special message in curent room          */
/*  putMessage()    stores a message to disk                            */
/*  putMsgChar()    writes character to message file                    */
/*  notelogmessage() notes private message into recip.'s log entry      */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/*  dPrintf()       sends formatted output to message file              */
/*  overwrite()     checks for any overwriting of old messages          */
/*  putMsgStr()     writes a string to the message file                 */
/*  noteMessage()   puts message in mesgBuf into message index          */
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
/*  makeMessage()   is menu-level routine to enter a message            */
/* -------------------------------------------------------------------- */
BOOL makeMessage(void)
{
#ifdef NETWORK
    int              logNo2;
#endif
    char             *pc, allUpper;
    int              logNo;
    
    char             recipient[NAMESIZE + NAMESIZE + NAMESIZE + NAMESIZE + 3];
    label            rnode;
    label            rregion;
    label            rcountry;
    
    label            forward;
    label            groupname;
    int              groupslot;
    label            replyId;
    char             filelink[64];
    char             subject[81];
    time_t           t;
    
    if (oldFlag && heldMessage)
    {
        memcpy( msgBuf, msgBuf2, sizeof(struct msgB) );
    }

    *rnode     = '\0';      /* null out some temp strings.. */
    *rregion   = '\0';
    *rcountry  = '\0';
    *recipient = '\0';
    *forward   = '\0';
    *filelink  = '\0';
    *rnode     = '\0';
    *groupname = '\0';
    *subject   = '\0';
    
    /* limited-access message, ask for group name */
    if (limitFlag)
    {
        getString("group", groupname, NAMESIZE, FALSE, ECHO, "");
        
        if (!(*groupname))
            groupslot = 0;
        else
            groupslot = partialgroup(groupname);

        
        if ( groupslot == ERROR || !ingroup(groupslot) )
        {
            mPrintf("\n No such group.");
            return FALSE;
        }
        /* makes it look prettier */
        strcpy(groupname, grpBuf.group[groupslot].groupname);
    }
    
    /* if replying, author becomes recipient */
    /* also record message which is being replied to */
    if (replyFlag)
    {
        strcpy(recipient, msgBuf->mbauth);
        strcpy(replyId,   *msgBuf->mbsrcId ? msgBuf->mbsrcId : msgBuf->mbId);
        strcpy(rnode,     msgBuf->mboname);
        strcpy(subject,   msgBuf->mbsub);
    }
    else
    {
        if (roomBuf.rbflags.SUBJECT)
        {
            getNormStr("subject", subject, 50, ECHO);
        }
    }

    /* clear message buffer 'cept when entring old message */
    if (!oldFlag)
    {
        memset(msgBuf, 0, sizeof(struct msgB));
    }

    strcpy(msgBuf->mbsub, subject);

    /* user not logged in, sending exclusive to sysop */
    if (mailFlag && !replyFlag && !loggedIn)
    {
        doCR();
        mPrintf(" Private mail to '%s'", (cfg.sysop[0]) ? cfg.sysop :"Sysop");
        strcpy(recipient, (cfg.sysop[0]) ? cfg.sysop : "Sysop");
    }

    /* sending exclusive mail which is not a reply */
    if (mailFlag && !replyFlag && loggedIn)
    {
        getNormStr("recipient", recipient, NAMESIZE + NAMESIZE + 1, ECHO);
        if (!strlen(recipient))
        {
            return FALSE;
        }

#ifdef NETWORK
        parseNetAddress(recipient, forward, rnode, rregion, rcountry);
        *forward = 0;
        
        if (!(*recipient))
        {
            strcpy(recipient, "sysop");
        }
#endif

    }

    if (mailFlag)
    {
#ifdef NETWORK
        if (*rnode) alias(rnode);

        logNo = findPerson(*rnode ? rnode : recipient, &lBuf);
        
        if (logNo == ERROR)
        {
            if (*rnode)    
            {
                label temp;
                strcpy(temp, rnode);
                route(temp);
                
                if (!getnode(temp))
                {
                    if (!*rregion)
                    {
                        mPrintf("Dont know how to reach '%s'", rnode);
                        return FALSE;
                    }
                }
            }
            else
            {
                if (
                      ( hash(recipient) != hash("Sysop"))
                   && ( hash(recipient) != hash("Aide"))
                   )
                {   
                    mPrintf("No '%s' known", recipient);
                    return FALSE;
                }
            }
        }
        else
        {
            if (lBuf.lbflags.NODE && !rnode[0])
            {
                mPrintf(" %s forwarded to Sysop on %s\n", cfg.msg_nym, recipient);
                strcpy(rnode, recipient);
                strcpy(recipient, "SysOp");
            }
        }
#else
        logNo = findPerson(recipient, &lBuf);

        if ( (logNo == ERROR) && ( hash(recipient) != hash("Sysop"))
           && ( hash(recipient) != hash("Aide")) )
        {
             mPrintf("No '%s' known", recipient);
             return FALSE;
        }
#endif
    
        if ( (logNo != ERROR) && lBuf.forward[0] && !*rnode)
        {
            mPrintf(" %s forwarded to ", cfg.msg_nym);

            logNo2 = findPerson(lBuf.forward, lBuf2);

            if (logNo2 != ERROR)
            {
                mPrintf("%s", lBuf2->lbname);
                strcpy(forward, lBuf2->lbname);
            }
            doCR();
        }
    
    }

    if (linkMess)
    {
        getNormStr("file", filelink, 64, ECHO);
        if ( !strlen(filelink))
        {
            return FALSE;
        }
    }

    /* copy groupname into the message buffer */
    strcpy(msgBuf->mbgroup, groupname);

    strcpy(msgBuf->mbzip,  rnode);
    strcpy(msgBuf->mbrzip, rregion);
    strcpy(msgBuf->mbczip, rcountry);

    strcpy(msgBuf->mbsub, subject);
    
    /* moderated messages */
    if (
         (
           roomBuf.rbflags.MODERATED 
           || (roomTab[thisRoom].rtflags.SHARED && !logBuf.lbflags.NETUSER)
         )
         && !mailFlag
       )
    {
        strcpy(msgBuf->mbx, "M");
    }

    /* problem user message */
    if (twit && !mailFlag)
    {
        strcpy(msgBuf->mbx, "Y");
    }
 
    /* copy message Id of message being replied to */
    if (replyFlag)
    {
        strcpy(msgBuf->mbreply, replyId);
    }        

    /* finally it's time to copy recipient to message buffer */
    if (*recipient)
    {
        strcpy(msgBuf->mbto, recipient);
    }
    else
    {
        msgBuf->mbto[0] = '\0';
    }

    /* finally it's time to copy forward addressee to message buffer */
    if (*forward)
    {
        strcpy(msgBuf->mbfwd, forward);
    }
    else
    {
        msgBuf->mbfwd[0] = '\0';
    }

    if (*filelink)
    {
        strcpy(msgBuf->mblink, filelink);
    }
    else
    {
        msgBuf->mblink[0] = '\0';
    }

    if (roomBuf.rbflags.SHARED)
    {
        strcpy(msgBuf->mbsig, cfg.nodeSignature);
    }
    
    /* lets handle .Enter old-message */
    if (oldFlag)
    {
        if (!heldMessage)
        {
            mPrintf("\n No aborted %s\n ", cfg.msg_nym);
            return FALSE;
        }
        else
        {
            if (!getYesNo("Use aborted message", 1))
                /* clear only the text portion of message buffer */
                memset( msgBuf->mbtext , 0, sizeof msgBuf->mbtext);
        }
    }

    /* clear our flags */
    heldMessage = FALSE;

    /* copy author name into message buffer */
    strcpy(msgBuf->mbauth,  logBuf.lbname);
    strcpy(msgBuf->mbsur,   logBuf.surname);
    strcpy(msgBuf->mbtitle, logBuf.title);

    /* set room# and attribute byte for message */
    msgBuf->mbroomno = (uchar)thisRoom;
    msgBuf->mbattr   = 0;

    
    if (roomBuf.rbflags.ANON)
    {
        t = time(NULL);
        t = (t / (60*60*24)) * (60*60*24); /* only the day.. */
        strcpy(msgBuf->mbauth,  "****");
        strcpy(msgBuf->mbsur,   "");
        strcpy(msgBuf->mbtitle, "");
        sprintf(msgBuf->mbtime, "%ld", t);
    }
    
    if (!linkMess)
    {
        if (getText())
        {
            for (pc=msgBuf->mbtext, allUpper=TRUE; *pc && allUpper;  pc++)
            {
                if (toupper(*pc) != *pc)  allUpper = FALSE;
            }

            if (allUpper)   fakeFullCase(msgBuf->mbtext);
        }
        else
        {
            oldFlag = FALSE;
            return FALSE;
        }
    }
    else
    {
        doCR();
        putheader(TRUE);
        
        msgBuf->mbtext[0] = '\0';
    }
    
    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

    strcpy(msgBuf->mboname, "");
    strcpy(msgBuf->mboreg,  "");

    strcpy(msgBuf->mbcreg,  "");
    strcpy(msgBuf->mbccont, "");


    if (sysop && msgBuf->mbx[0] == 'M')
    {
        if (getYesNo("Release message", 1))
        {
            msgBuf->mbx[0] = '\0' /*NULL*/;
        }
    }

#ifdef NETWORK
    if (*msgBuf->mbzip)  /* save it for netting... */
    {
        /* room names get included in networked mail */
        strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);

        if (!save_mail())
        {
            heldMessage = TRUE;
            memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );
        
            mPrintf("Error mailing to %s, message in old buffer.", msgBuf->mbzip); 
            doCR();
        
            return FALSE;
        }
    }
#endif
    
    putMessage();

    if (!replyFlag)
    {
        MessageRoom[msgBuf->mbroomno]++;
    }

    noteMessage();

    limitFlag = 0;  /* keeps Aide) messages from being grouponly */

#ifdef GOODBYE
    /* 
     * if its mail, note it in recipients log entry 
     */
    if (mailFlag) notelogmessage(msgBuf->mbto);
    /* note in forwarding addressee's log entry */
    if (*forward)  notelogmessage(msgBuf->mbfwd);
#endif

    msgBuf->mbto[   0] = '\0';
    msgBuf->mbgroup[0] = '\0';
    msgBuf->mbfwd[  0] = '\0';

    oldFlag     = FALSE;

    return TRUE;
}


/* -------------------------------------------------------------------- */
/*  putMessage()    stores a message to disk                            */
/* -------------------------------------------------------------------- */
BOOL putMessage(void)
{
    long timestamp;
    char stamp[20];

    time(&timestamp);
    sprintf(stamp, "%ld", timestamp);

    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

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

    /* setup time/datestamp: */
    if (!msgBuf->mbcopy[0])
    {
        if(!*msgBuf->mbtime)
        {
            strcpy(msgBuf->mbtime, stamp);
        }
    }
    else
    {
        *msgBuf->mbtime = 0;
    }

    /* write room name out:     */

/*    if (!*msgBuf->mboname) */
/*    { */
        if (!msgBuf->mbcopy[0]) 
        { 
            strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);
        }
        else
        {
            *msgBuf->mbroom = 0;
        }
/*  } */

    if (!msgBuf->mbcopy[0])  { dPrintf("A%s", msgBuf->mbauth);      }
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

    talleyBuf->room[msgBuf->mbroomno].total++;

    if (mayseemsg()) 
    {
        talleyBuf->room[msgBuf->mbroomno].messages++;
        talleyBuf->room[msgBuf->mbroomno].new++;
    }

    return  TRUE;
}

/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/* -------------------------------------------------------------------- */
void aideMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = AIDEROOM;

    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf->room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  specialMessage()    saves a special message in curent room          */
/* -------------------------------------------------------------------- */
void specialMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = (uchar)thisRoom;
    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[thisRoom].lvisit)
        talleyBuf->room[thisRoom].new--;
}

/* -------------------------------------------------------------------- */
/*  putMsgChar()    writes character to message file                    */
/* -------------------------------------------------------------------- */
void putMsgChar(char c)
{
    if (ftell(msgfl) >= (long)((long)cfg.messagek * 1024l))
    {
        /* scroll to the beginning */
        fseek(msgfl, 0l, 0);
    }

    /* write character out */
    fputc(c, msgfl);
}
#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  notelogmessage() notes private message into recip.'s log entry      */
/* -------------------------------------------------------------------- */
void notelogmessage(char *name)
{
    int logNo;

    if (    strcmpi( msgBuf->mbto, "Sysop") == SAMESTRING
         || strcmpi( msgBuf->mbto, "Aide") == SAMESTRING
         || (
                 msgBuf->mbzip[0] 
              && strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING
            )
       ) return;

    if ((logNo = findPerson(name, lBuf2)) == ERROR)
    {
        return;
    }
        
    if (lBuf2->lbflags.L_INUSE)
    {
        /* if room unknown and public room, make it known */
        if ( 
             (lBuf2->lbroom[thisRoom].lbgen != roomBuf.rbgen)
             && roomBuf.rbflags.PUBLIC 
           )
            lBuf2->lbroom[thisRoom].lbgen = roomBuf.rbgen;

        lBuf2->lbroom[thisRoom].mail = TRUE;  /* Note there's mail */
        putLog(lBuf2, logNo);
    }
}
#endif
/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;
    uint total = (uint)(cfg.nmessages - howmany);

    for (i = 0; i < howmany; ++i)
    {
        room = msgTab4[i].mtroomno;

        talleyBuf->room[room].total--;

        if (mayseeindexmsg(i))
        {
            talleyBuf->room[room].messages--;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf->room[room].new--;
        }
    }

    _fmemmove(msgTab1, &(msgTab1[howmany]),
            ( total * (unsigned)sizeof(*msgTab1)) );
    _fmemmove(msgTab2, &(msgTab2[howmany]),
            ( total * (unsigned)sizeof(*msgTab2)) );
/*  _fmemmove(msgTab3, &(msgTab3[howmany]),
            ( total * (unsigned)sizeof(*msgTab3)) ); */
    _fmemmove(msgTab4, &(msgTab4[howmany]),
            ( total * (unsigned)sizeof(*msgTab4)) );
    _fmemmove(msgTab5, &(msgTab5[howmany]),
            ( total * (unsigned)sizeof(*msgTab5)) );
    _fmemmove(msgTab6, &(msgTab6[howmany]),
            ( total * (unsigned)sizeof(*msgTab6)) );
    _fmemmove(msgTab7, &(msgTab7[howmany]),
            ( total * (unsigned)sizeof(*msgTab7)) );
    _fmemmove(msgTab8, &(msgTab8[howmany]),
            ( total * (unsigned)sizeof(*msgTab8)) );
/*  _fmemmove(msgTab9, &(msgTab9[howmany]),
            ( total * (unsigned)sizeof(*msgTab9)) ); */

    cfg.mtoldest += howmany;
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;

    for (i = 0; i < howmany; ++i)
    {
        room = msgTab[i].mtroomno;

        talleyBuf->room[room].total--;

        if (mayseeindexmsg(i))
        {
            talleyBuf->room[room].messages--;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf->room[room].new--;
        }
    }

    hmemcpy(&(msgTab[0]), &(msgTab[howmany]),
            ( (ulong)(cfg.nmessages - howmany) * (ulong)sizeof(*msgTab)) );

    cfg.mtoldest += howmany;
}
#endif

/* -------------------------------------------------------------------- */
/*  dPrintf()       sends formatted output to message file              */
/* -------------------------------------------------------------------- */
void dPrintf(char *fmt, ... )
{
    char buff[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    putMsgStr(buff);
}


/* -------------------------------------------------------------------- */
/*  overwrite()     checks for any overwriting of old messages          */
/* -------------------------------------------------------------------- */
void overwrite(int bytes)
{
    long pos;
    int i;

    pos = ftell(msgfl);

    fseek(msgfl, 0l, SEEK_CUR);

    for ( i = 0; i < bytes; ++i)
    {
        if (getMsgChar() == 0xFF /* -1 */) /* obliterating a message */
        {
            logBuf.lbvisit[(MAXVISIT-1)]    = ++cfg.oldest;
        }
    }

    fseek(msgfl, pos, SEEK_SET);
}

/* -------------------------------------------------------------------- */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */
void putMsgStr(char *string)
{
    char *s;

    /* check for obliterated messages */
    overwrite(strlen(string) + 1); /* the '+1' is for the null */

    for (s = string;  *s;  s++) putMsgChar(*s);

    /* null to tie off string */
    putMsgChar(0);
}

/* -------------------------------------------------------------------- */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */
void noteMessage(void)
{
    ulong id /* ,copy */;
    int crunch = 0;
 /* int slot, copyslot; */

    logBuf.lbvisit[0]   = ++cfg.newest;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* mush up any obliterated messages */
    if (cfg.mtoldest < cfg.oldest)
    {
        crunch = ((ushort)(cfg.oldest - cfg.mtoldest));
    }

    /* scroll index at #nmessages mark */
    if ( (ushort)(id - cfg.mtoldest) >= (ushort)cfg.nmessages)
    {
        crunch++;
    }

    if (crunch)
    {
        crunchmsgTab(crunch);
    }

    /* now record message info in index */
    indexmessage(id);

#ifdef GOODBYE
    /* special for duplicated messages */
    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        copyslot = indexslot(copy);
        slot     = indexslot(id);

        if (copyslot != ERROR)
        {
            copyindex(slot, copyslot);
        }
    }
#endif
}

#endif /* Not MULTICIT.  */

#ifdef MULTICIT

/* 4-21-92 Review started (JRD).  */

/* -------------------------------------------------------------------- */
/*  MSGMAKE.C                Dragon Citadel                             */
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
/*  makeMessage()   is menu-level routine to enter a message            */
/*  aideMessage()   save auto message in Aide>                          */
/*  specialMessage()    saves a special message in curent room          */
/*  putMessage()    stores a message to disk                            */
/*  putMsgChar()    writes character to message file                    */
/*  notelogmessage() notes private message into recip.'s log entry      */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/*  dPrintf()       sends formatted output to message file              */
/*  overwrite()     checks for any overwriting of old messages          */
/*  putMsgStr()     writes a string to the message file                 */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  02/26/91	(PAT)	Rearanged message code for overlays.		*/
/*  E-Day 92	(JRD)	Multicit compatability started. 		*/
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  makeMessage()   is menu-level routine to enter a message            */
/* -------------------------------------------------------------------- */
BOOL makeMessage(void)
{
#ifdef NETWORK
    int              logNo2;
#endif
    char             *pc, allUpper;
    int              logNo;
    
    char             recipient[NAMESIZE + NAMESIZE + NAMESIZE + NAMESIZE + 3];
    label            rnode;
    label            rregion;
    label            rcountry;
    
    label            forward;
    label            groupname;
    int              groupslot;
    label            replyId;
    char             filelink[64];
    char             subject[81];
    time_t           t;
    
    if (oldFlag && heldMessage)
    {
        memcpy( msgBuf, msgBuf2, sizeof(struct msgB) );
    }

    *rnode     = '\0';      /* null out some temp strings.. */
    *rregion   = '\0';
    *rcountry  = '\0';
    *recipient = '\0';
    *forward   = '\0';
    *filelink  = '\0';
    *rnode     = '\0';
    *groupname = '\0';
    *subject   = '\0';
    
    /* limited-access message, ask for group name */
    if (limitFlag)
	{
        getString("group", groupname, NAMESIZE, FALSE, ECHO, "");
        
        if (!(*groupname))
            groupslot = 0;
        else
            groupslot = partialgroup(groupname);

        
        if ( groupslot == ERROR || !ingroup(groupslot) )
	    {
            mPrintf("\n No such group.");
            return FALSE;
	    }

        /* makes it look prettier */
        strcpy(groupname, grpBuf.group[groupslot].groupname);
	}
    
    /* if replying, author becomes recipient */
    /* also record message which is being replied to */

    if (replyFlag)
	{
        strcpy(recipient, msgBuf->mbauth);
        strcpy(replyId,   *msgBuf->mbsrcId ? msgBuf->mbsrcId : msgBuf->mbId);
        strcpy(rnode,     msgBuf->mboname);
        strcpy(subject,   msgBuf->mbsub);
	}
    else
	{
        if (roomBuf.rbflags.SUBJECT)
	    {
            getNormStr("subject", subject, 50, ECHO);
	    }
	}

    /* clear message buffer except when entring an old message */
    if (!oldFlag)
	{
        memset(msgBuf, 0, sizeof(struct msgB));
	}

    strcpy(msgBuf->mbsub, subject);

    /* user not logged in, sending exclusive to sysop */

    if (mailFlag && !replyFlag && !loggedIn)
	{
        doCR();
        mPrintf(" Private mail to '%s'", (cfg.sysop[0]) ? cfg.sysop :"Sysop");
        strcpy(recipient, (cfg.sysop[0]) ? cfg.sysop : "Sysop");
	}

    /* sending exclusive mail which is not a reply */
    if (mailFlag && !replyFlag && loggedIn)
	{
        getNormStr("recipient", recipient, NAMESIZE + NAMESIZE + 1, ECHO);
        if (!strlen(recipient))
	    {
            return FALSE;
	    }

#ifdef NETWORK
        parseNetAddress(recipient, forward, rnode, rregion, rcountry);
        *forward = 0;
        
        if (!(*recipient))
	    {
            strcpy(recipient, "sysop");
	    }
#endif
	}

    if (mailFlag)
	{
#ifdef NETWORK
        if (*rnode) alias(rnode);

        logNo = findPerson(*rnode ? rnode : recipient, &lBuf);
        
        if (logNo == ERROR)
	    {
            if (*rnode)    
		{
                label temp;
                strcpy(temp, rnode);
                route(temp);
                
                if (!getnode(temp))
		    {
                    if (!*rregion)
			{
                        mPrintf("Dont know how to reach '%s'", rnode);
                        return FALSE;
			}
		    }
		}
            else
		{
                if (
                      ( hash(recipient) != hash("Sysop"))
                   && ( hash(recipient) != hash("Aide"))
                   )
		    {
                    mPrintf("No '%s' known", recipient);
                    return FALSE;
		    }
		}
	    }
        else
        {
            if (lBuf.lbflags.NODE && !rnode[0])
            {
                mPrintf(" %s forwarded to Sysop on %s\n", cfg.msg_nym, recipient);
                strcpy(rnode, recipient);
                strcpy(recipient, "SysOp");
            }
        }
#else
        logNo = findPerson(recipient, &lBuf);

        if ( (logNo == ERROR) && ( hash(recipient) != hash("Sysop"))
           && ( hash(recipient) != hash("Aide")) )
        {
             mPrintf("No '%s' known", recipient);
             return FALSE;
        }
#endif
    
        if ( (logNo != ERROR) && lBuf.forward[0] && !*rnode)
        {
            mPrintf(" %s forwarded to ", cfg.msg_nym);

            logNo2 = findPerson(lBuf.forward, lBuf2);

            if (logNo2 != ERROR)
            {
                mPrintf("%s", lBuf2->lbname);
                strcpy(forward, lBuf2->lbname);
            }
            doCR();
        }
    }

    if (linkMess)
    {
        getNormStr("file", filelink, 64, ECHO);
        if ( !strlen(filelink))
        {
            return FALSE;
        }
    }

    /* copy groupname into the message buffer */
    strcpy(msgBuf->mbgroup, groupname);

    strcpy(msgBuf->mbzip,  rnode);
    strcpy(msgBuf->mbrzip, rregion);
    strcpy(msgBuf->mbczip, rcountry);

    strcpy(msgBuf->mbsub, subject);
    
    /* moderated messages */
    if (
         (
           roomBuf.rbflags.MODERATED 
           || (roomTab[thisRoom].rtflags.SHARED && !logBuf.lbflags.NETUSER)
         )
         && !mailFlag
       )
    {
        strcpy(msgBuf->mbx, "M");
    }

    /* problem user message */
    if (twit && !mailFlag)
    {
        strcpy(msgBuf->mbx, "Y");
    }
 
    /* copy message Id of message being replied to */
    if (replyFlag)
    {
        strcpy(msgBuf->mbreply, replyId);
    }        

    /* finally it's time to copy recipient to message buffer */
    if (*recipient)
    {
        strcpy(msgBuf->mbto, recipient);
    }
    else
    {
        msgBuf->mbto[0] = '\0';
    }

    /* finally it's time to copy forward addressee to message buffer */
    if (*forward)
    {
        strcpy(msgBuf->mbfwd, forward);
    }
    else
    {
        msgBuf->mbfwd[0] = '\0';
    }

    if (*filelink)
    {
        strcpy(msgBuf->mblink, filelink);
    }
    else
    {
        msgBuf->mblink[0] = '\0';
    }

    if (roomBuf.rbflags.SHARED)
    {
        strcpy(msgBuf->mbsig, cfg.nodeSignature);
    }
    
    /* lets handle .Enter old-message */
    if (oldFlag)
    {
        if (!heldMessage)
        {
            mPrintf("\n No aborted %s\n ", cfg.msg_nym);
            return FALSE;
        }
        else
        {
            if (!getYesNo("Use aborted message", 1))
                /* clear only the text portion of message buffer */
                memset( msgBuf->mbtext , 0, sizeof msgBuf->mbtext);
        }
    }

    /* clear our flags */
    heldMessage = FALSE;

    /* copy author name into message buffer */
    strcpy(msgBuf->mbauth,  logBuf.lbname);
    strcpy(msgBuf->mbsur,   logBuf.surname);
    strcpy(msgBuf->mbtitle, logBuf.title);

    /* set room# and attribute byte for message */
    msgBuf->mbroomno = (uchar)thisRoom;
    msgBuf->mbattr   = 0;

    
    if (roomBuf.rbflags.ANON)
    {
        t = time(NULL);
        t = (t / (60*60*24)) * (60*60*24); /* only the day.. */
        strcpy(msgBuf->mbauth,  "****");
        strcpy(msgBuf->mbsur,   "");
        strcpy(msgBuf->mbtitle, "");
        sprintf(msgBuf->mbtime, "%ld", t);
    }
    
    if (!linkMess)
    {
        if (getText())
        {
            for (pc=msgBuf->mbtext, allUpper=TRUE; *pc && allUpper;  pc++)
            {
                if (toupper(*pc) != *pc)  allUpper = FALSE;
            }

            if (allUpper)   fakeFullCase(msgBuf->mbtext);
        }
        else
        {
            oldFlag = FALSE;
            return FALSE;
        }
    }
    else
    {
        doCR();
        putheader(TRUE);
        
        msgBuf->mbtext[0] = '\0';
    }
    
    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

    strcpy(msgBuf->mboname, "");
    strcpy(msgBuf->mboreg,  "");

    strcpy(msgBuf->mbcreg,  "");
    strcpy(msgBuf->mbccont, "");


    if (sysop && msgBuf->mbx[0] == 'M')
    {
        if (getYesNo("Release message", 1))
        {
            msgBuf->mbx[0] = '\0' /*NULL*/;
        }
    }

#ifdef NETWORK
    if (*msgBuf->mbzip)  /* save it for netting... */
    {
        /* room names get included in networked mail */
        strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);

        if (!save_mail())
        {
            heldMessage = TRUE;
            memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );
        
            mPrintf("Error mailing to %s, message in old buffer.", msgBuf->mbzip); 
            doCR();
        
            return FALSE;
        }
    }
#endif
    
    putMessage();

    if (!replyFlag)
    {
        MessageRoom[msgBuf->mbroomno]++;
    }

    noteMessage();

    limitFlag = 0;  /* keeps Aide) messages from being grouponly */

#ifdef GOODBYE
    /* 
     * if its mail, note it in recipients log entry 
     */
    if (mailFlag) notelogmessage(msgBuf->mbto);
    /* note in forwarding addressee's log entry */
    if (*forward)  notelogmessage(msgBuf->mbfwd);
#endif

    msgBuf->mbto[   0] = '\0';
    msgBuf->mbgroup[0] = '\0';
    msgBuf->mbfwd[  0] = '\0';

    oldFlag     = FALSE;

    return TRUE;
}


/* -------------------------------------------------------------------- */
/*  putMessage()    stores a message to disk                            */
/* -------------------------------------------------------------------- */
BOOL putMessage(void)
{
    /* MCNOTE: During the writing of a message, GLOBAL.DAT and the target */
    /* file are captured.						  */

    long timestamp;
    char stamp[20];

    time(&timestamp);
    sprintf(stamp, "%ld", timestamp);

    Capture () ;
    CaptureMsgfl (msgfl) ;

    sprintf(msgBuf->mbId, "%lu", (unsigned long)(cfg.newest + 1) );

    /* record start of message to be noted */
    msgBuf->mbheadLoc = (long)cfg.catLoc;


    /* tell putMsgChar where to write   */
    fseek(msgfl, cfg.catLoc, 0);

    overwrite(1);
    putMsgChar((char)0xFF);		  /* start-of-message		  */

    overwrite(1);
    putMsgChar(msgBuf->mbroomno);	  /* room #		 */

    overwrite(1);
    putMsgChar(msgBuf->mbattr); 	  /* attribute byte	*/

    dPrintf("%s", msgBuf->mbId);	  /* message ID */

    if (!msgBuf->mbcopy[0])		  /* setup time/datestamp: */
	{
        if(!*msgBuf->mbtime)
	    {
            strcpy(msgBuf->mbtime, stamp);
	    }
	}
    else
	{
        *msgBuf->mbtime = 0;
	}

    /* room name */

    if (!msgBuf->mbcopy[0])
	strcpy(msgBuf->mbroom, roomTab[msgBuf->mbroomno].rtname);
    else
	*msgBuf->mbroom = 0;

    if (!msgBuf->mbcopy[0])  { dPrintf("A%s", msgBuf->mbauth);      }
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

    overwrite(1);
    putMsgChar('M');
    putMsgStr(msgBuf->mbtext);			/* Message 'M' and text. */

    fflush(msgfl);

    /* record where to begin writing next message */

    cfg.catLoc = ftell(msgfl);


    talleyBuf->room[msgBuf->mbroomno].total++;

    if (mayseemsg()) 
    {
        talleyBuf->room[msgBuf->mbroomno].messages++;
        talleyBuf->room[msgBuf->mbroomno].new++;
    }

    ReleaseMsgFl () ;
    Release () ;

    return  TRUE;
}

/* -------------------------------------------------------------------- */
/*  aideMessage()   save auto message in Aide>                          */
/* -------------------------------------------------------------------- */
void aideMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = AIDEROOM;

    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[AIDEROOM].lvisit)
        talleyBuf->room[AIDEROOM].new--;
}

/* -------------------------------------------------------------------- */
/*  specialMessage()    saves a special message in curent room          */
/* -------------------------------------------------------------------- */
void specialMessage(void)
{
    /* clear out message buffer */
    clearmsgbuf();

    msgBuf->mbroomno = (uchar)thisRoom;
    strcpy(msgBuf->mbauth,  cfg.nodeTitle);

    putMessage();

    noteMessage();

    if (!logBuf.lbroom[thisRoom].lvisit)
        talleyBuf->room[thisRoom].new--;
}
// !!!!!!!!!!!!!!!







/* -------------------------------------------------------------------- */
/*  putMsgChar()    writes character to message file                    */
/* -------------------------------------------------------------------- */
void putMsgChar(char c)
{
    if (ftell(msgfl) >= (long)((long)cfg.messagek * 1024l))
    {
        /* scroll to the beginning */
        fseek(msgfl, 0l, 0);
    }

    /* write character out */
    fputc(c, msgfl);
}
#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  notelogmessage() notes private message into recip.'s log entry      */
/* -------------------------------------------------------------------- */
void notelogmessage(char *name)
{
    int logNo;

    if (    strcmpi( msgBuf->mbto, "Sysop") == SAMESTRING
         || strcmpi( msgBuf->mbto, "Aide") == SAMESTRING
         || (
                 msgBuf->mbzip[0] 
              && strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING
            )
       ) return;

    if ((logNo = findPerson(name, lBuf2)) == ERROR)
    {
        return;
    }
        
    if (lBuf2->lbflags.L_INUSE)
    {
        /* if room unknown and public room, make it known */
        if ( 
             (lBuf2->lbroom[thisRoom].lbgen != roomBuf.rbgen)
             && roomBuf.rbflags.PUBLIC 
           )
            lBuf2->lbroom[thisRoom].lbgen = roomBuf.rbgen;

        lBuf2->lbroom[thisRoom].mail = TRUE;  /* Note there's mail */
        putLog(lBuf2, logNo);
    }
}
#endif
/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;
    uint total = (uint)(cfg.nmessages - howmany);

    for (i = 0; i < howmany; ++i)
    {
        room = msgTab4[i].mtroomno;

        talleyBuf->room[room].total--;

        if (mayseeindexmsg(i))
        {
            talleyBuf->room[room].messages--;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf->room[room].new--;
        }
    }

    _fmemmove(msgTab1, &(msgTab1[howmany]),
            ( total * (unsigned)sizeof(*msgTab1)) );
    _fmemmove(msgTab2, &(msgTab2[howmany]),
            ( total * (unsigned)sizeof(*msgTab2)) );
/*  _fmemmove(msgTab3, &(msgTab3[howmany]),
            ( total * (unsigned)sizeof(*msgTab3)) ); */
    _fmemmove(msgTab4, &(msgTab4[howmany]),
            ( total * (unsigned)sizeof(*msgTab4)) );
    _fmemmove(msgTab5, &(msgTab5[howmany]),
            ( total * (unsigned)sizeof(*msgTab5)) );
    _fmemmove(msgTab6, &(msgTab6[howmany]),
            ( total * (unsigned)sizeof(*msgTab6)) );
    _fmemmove(msgTab7, &(msgTab7[howmany]),
            ( total * (unsigned)sizeof(*msgTab7)) );
    _fmemmove(msgTab8, &(msgTab8[howmany]),
            ( total * (unsigned)sizeof(*msgTab8)) );
/*  _fmemmove(msgTab9, &(msgTab9[howmany]),
            ( total * (unsigned)sizeof(*msgTab9)) ); */

    cfg.mtoldest += howmany;
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  crunchmsgTab()  obliterates slots at the beginning of table         */
/* -------------------------------------------------------------------- */
void crunchmsgTab(int howmany)
{
    int i;
    int room;

    for (i = 0; i < howmany; ++i)
    {
        room = msgTab[i].mtroomno;

        talleyBuf->room[room].total--;

        if (mayseeindexmsg(i))
        {
            talleyBuf->room[room].messages--;

            if  ((ulong)(cfg.mtoldest + i) >
                logBuf.lbvisit[ logBuf.lbroom[room].lvisit ])
                talleyBuf->room[room].new--;
        }
    }

    hmemcpy(&(msgTab[0]), &(msgTab[howmany]),
            ( (ulong)(cfg.nmessages - howmany) * (ulong)sizeof(*msgTab)) );

    cfg.mtoldest += howmany;
}
#endif

/* -------------------------------------------------------------------- */
/*  dPrintf()       sends formatted output to message file              */
/* -------------------------------------------------------------------- */
void dPrintf(char *fmt, ... )
{
    char buff[256];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    putMsgStr(buff);
}


/* -------------------------------------------------------------------- */
/*  overwrite()     checks for any overwriting of old messages          */
/* -------------------------------------------------------------------- */
void overwrite(int bytes)
{
    long pos;
    int i;

    pos = ftell(msgfl);

    fseek(msgfl, 0l, SEEK_CUR);

    for ( i = 0; i < bytes; ++i)
    {
        if (getMsgChar() == 0xFF /* -1 */) /* obliterating a message */
        {
            logBuf.lbvisit[(MAXVISIT-1)]    = ++cfg.oldest;
        }
    }

    fseek(msgfl, pos, SEEK_SET);
}

/* -------------------------------------------------------------------- */
/*  putMsgStr()     writes a string to the message file                 */
/* -------------------------------------------------------------------- */
void putMsgStr(char *string)
{
    char *s;

    /* check for obliterated messages */
    overwrite(strlen(string) + 1); /* the '+1' is for the null */

    for (s = string;  *s;  s++) putMsgChar(*s);

    /* null to tie off string */
    putMsgChar(0);
}

/* -------------------------------------------------------------------- */
/*  noteMessage()   puts message in mesgBuf into message index          */
/* -------------------------------------------------------------------- */
void noteMessage(void)
{
    ulong id /* ,copy */;
    int crunch = 0;
 /* int slot, copyslot; */

    logBuf.lbvisit[0]   = ++cfg.newest;

    sscanf(msgBuf->mbId, "%lu", &id);

    /* mush up any obliterated messages */
    if (cfg.mtoldest < cfg.oldest)
    {
        crunch = ((ushort)(cfg.oldest - cfg.mtoldest));
    }

    /* scroll index at #nmessages mark */
    if ( (ushort)(id - cfg.mtoldest) >= (ushort)cfg.nmessages)
    {
        crunch++;
    }

    if (crunch)
    {
        crunchmsgTab(crunch);
    }

    /* now record message info in index */
    indexmessage(id);

#ifdef GOODBYE
    /* special for duplicated messages */
    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
        /* get the ID# */
        sscanf(msgBuf->mbcopy, "%ld", &copy);

        copyslot = indexslot(copy);
        slot     = indexslot(id);

        if (copyslot != ERROR)
        {
            copyindex(slot, copyslot);
        }
    }
#endif
}

#endif /* MULTICIT  */
