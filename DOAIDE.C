/* -------------------------------------------------------------------- */
/*  DOAIDE.C                 Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*        Code for doAide() and some function implemetations.           */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  doAide()        handles the aide-only menu                          */
/*  msgNym()        Aide message nym setting function                   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*   5/14/91    (PAT)   Code moved from SYSOP1.C to shrink that file    */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  doAide()        handles the aide-only menu                          */
/* -------------------------------------------------------------------- */
void doAide(char moreYet, char first)
/* char moreYet; */
/* char first; */    /* first parameter if TRUE              */
{
    int  roomExists();
    char oldchat;
    char ich;

    if (moreYet)   first = '\0';

    mPrintf("Aide special fn: ");

    switch (toupper( first ? (char)first : (char)(ich=(char)iCharNE()) ))
    {
    case 'A':
        mPrintf("Attributes ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else attributes();
        break;

    case 'C':
        chatReq = TRUE;
        oldchat = (char)cfg.noChat;
        cfg.noChat = FALSE;
        mPrintf("Chat\n ");
        if (whichIO == MODEM)   ringSysop();
        else                    chat() ;
        cfg.noChat = oldchat;
        break;

    case 'E':
        mPrintf("Edit room\n  \n");
        renameRoom();
        break;

    case 'F':
        mPrintf("File set\n  \n");
        batchinfo(TRUE);
        break;
    
    case 'G':
        mPrintf("Group membership\n  \n");
        groupfunc();
        break;

    case 'H':
        mPrintf("Hallway changes\n  \n");
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            hallfunc();
        }
        break;

    case 'I':
        mPrintf("Insert %s\n ", cfg.msg_nym);
        insert();
        break;

    case 'K':
        mPrintf("Kill room\n ");
        killroom();
        break;

    case 'L':
        mPrintf("List group ");
        listgroup();
        break;

    case 'M':
        mPrintf("Move file ");
        moveFile();
        break;

    case 'N':
        mPrintf("Name Messages");
        msgNym();
        break;

    case 'R':
        mPrintf("Rename file ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            renamefile();
        }
        break;

    case 'S':
        mPrintf("Set file info\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            setfileinfo();
        }
        break;

    case 'U':
        mPrintf("Unlink file\n ");
        if (roomBuf.rbflags.MSDOSDIR != 1)
        {
            if (expert) mPrintf("? ");
            else        mPrintf("\n Not a directory room.");
        }
        else
        {
            unlinkfile();
        }
        break;

    case 'V':
        mPrintf("View Help Text File\n ");
        tutorial("aide.hlp");
        break;

    case 'W':
        mPrintf("Window into hallway\n ");
        
        if (cfg.floors)
        {
            doCR();
            mPrintf("-- System in floor mode, no efect."); doCR();
            return;
        }
        
        if (!cfg.aidehall && !sysop)
        {
            mPrintf(" Must be a Sysop!\n");
        }
        else
        {
            windowfunc();
        }
        break;

    case '-':
        moveRoom(-1);
        break;
        
    case '+':
        moveRoom(1);
        break;
        
    case '?':
        oChar('?');
        tutorial("aide.mnu");
        break;

    default:
        oChar(ich);
        if (!expert)   mPrintf("\n '?' for menu.\n " );
        else           mPrintf(" ?\n "               );
        break;
    }
}


/* -------------------------------------------------------------------- */
/*  msgNym()        Aide message nym setting function                   */
/* -------------------------------------------------------------------- */
void msgNym(void)
{
    doCR();
    if (!cfg.msgNym)
    {
        doCR();
        printf(" Message nyms not enabled!");
        doCR();
        return;
    }

    getString("name (SINGLE)", cfg.msg_nym,  LABELSIZE, FALSE, ECHO, "");
    getString("name (PLURAL)", cfg.msgs_nym, LABELSIZE, FALSE, ECHO, "");
    getString("what to do to message", 
               cfg.msg_done, LABELSIZE, FALSE, ECHO, "");

    sprintf(msgBuf->mbtext, "\n Message nym changed by %s to\n "
                            "Single:   %s\n "
                            "Plural:   %s\n "
                            "Verb  :   %s\n ",
                            logBuf.lbname, 
                            cfg.msg_nym, cfg.msgs_nym, cfg.msg_done );
    aideMessage();
    doCR();
}


