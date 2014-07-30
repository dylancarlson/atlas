/* -------------------------------------------------------------------- */
/*  HALLEDIT.C               Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                 Code for the aide/sop to edit halls..                */
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
/*  force()         sysop special to force access into a hallway        */
/*  globalhall()    adds/removes rooms from current hall                */
/*  hallfunc()      adds/removes room from current hall                 */
/*  xhallfunc()     called by hallfunc() for each hall                  */
/*  killhall()      sysop special to kill a hall                        */
/*  listhalls()     sysop special to list all hallways                  */
/*  newhall()       sysop special to add a new hall                     */
/*  renamehall()    sysop special to rename a hallway                   */
/*  windowfunc()    windows/unwindows room from current hall            */
/*  moveHall()      Moves a hall up or down in the hall list            */
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
/*  force()         sysop special to force access into a hallway        */
/* -------------------------------------------------------------------- */
void force(void)
{
    label hallname;
    int slot;

    getString("hallway", hallname, NAMESIZE, FALSE, ECHO, "");

    slot = partialhall(hallname);

    if ( (slot == ERROR) || !strlen(hallname) )
    {
        mPrintf("\n No such hall.");
        return;
    }

    thisHall = (unsigned char)slot;

    sprintf(msgBuf->mbtext,
    "Access forced to hallway %s", hallBuf->hall[thisHall].hallname );

    mPrintf("%s", msgBuf->mbtext); doCR();

    trap(msgBuf->mbtext, T_SYSOP);
}

/* -------------------------------------------------------------------- */
/*  globalhall()    adds/removes rooms from current hall                */
/* -------------------------------------------------------------------- */
int changed_room;

void globalhall(void)
{
    int roomslot;

    changed_room = FALSE;

    for (roomslot = 0; roomslot < MAXROOMS; roomslot++)
      if (roomTab[roomslot].rtflags.INUSE)
      {
        mPrintf(" %s", roomTab[roomslot].rtname);
        if (xhallfunc(roomslot, 3, TRUE)==FALSE)
          break;
      }

    if (changed_room)
    {
        amPrintf("\n By %s, in hall %s\n", logBuf.lbname,
                hallBuf->hall[thisHall].hallname);

        SaveAideMess();
    }

    putHall();
}       

/* -------------------------------------------------------------------- */
/*  hallfunc()      adds/removes room from current hall                 */
/* -------------------------------------------------------------------- */
void hallfunc(void)
{
    label roomname;
    int roomslot;

    getString("room name", roomname, NAMESIZE, FALSE, ECHO, "");

    roomslot = roomExists(roomname);
    if (roomslot == ERROR) roomslot = partialExist(roomname);


    if ((roomslot) == ERROR || !strlen(roomname) )
    {
        mPrintf("\n No %s room", roomname);
        return;
    }

    xhallfunc(roomslot, 0, FALSE);

    putHall();
}       

/* -------------------------------------------------------------------- */
/*  xhallfunc()     called by hallfunc() for each hall                  */
/* -------------------------------------------------------------------- */
xhallfunc(int roomslot, int xyn, int fl)
{
    int yn;

    if (hallBuf->hall[thisHall].hroomflags[roomslot].inhall)
    {

        yn=getYesNo("Exclude from hall", (char)xyn);
        if (yn == 2) return FALSE;
        if (yn)
        {
            hallBuf->hall[thisHall].hroomflags[roomslot].inhall = FALSE;

            sprintf(msgBuf->mbtext,
            "Room %s excluded from hall %s by %s",
             roomTab[roomslot].rtname,
             hallBuf->hall[thisHall].hallname,
             logBuf.lbname );

            trap(msgBuf->mbtext, T_AIDE);

            if (!fl)
            {
                aideMessage();
            }else{              
                amPrintf(" Excluded %s\n",
                       roomTab[roomslot].rtname,
                       hallBuf->hall[thisHall].hallname);
                changed_room = TRUE;
            }
        }
    }else{
      yn=getYesNo("Add to hall", (char)xyn);
      if (yn == 2) return FALSE;
      if (yn)
      {
          hallBuf->hall[thisHall].hroomflags[roomslot].inhall = TRUE;

          sprintf(msgBuf->mbtext,
             "Room %s added to hall %s by %s",
             roomTab[roomslot].rtname,
             hallBuf->hall[thisHall].hallname,
             logBuf.lbname );

          trap(msgBuf->mbtext, T_AIDE);

            if (!fl)
            {
                aideMessage();
            }else{
                amPrintf(" Added    %s\n",
                       roomTab[roomslot].rtname);
                changed_room = TRUE;
            }
      }
    }
    return TRUE;
}


/* -------------------------------------------------------------------- */
/*  killhall()      sysop special to kill a hall                        */
/* -------------------------------------------------------------------- */
void killhall(void)
{
    int empty = TRUE, i;

    if (thisHall == 0 || thisHall == 1)
    {
        mPrintf("\nThe Main and Maintenance hallways cannot be killed.");
        return;
    }
    
    /* Check hall for any rooms */
    for (i = 0; i < MAXROOMS; i++)
    {
        if ( hallBuf->hall[thisHall].hroomflags[i].inhall
          && roomTab[i].rtflags.INUSE) empty = FALSE;
    }

    if (!empty) 
    {
        mPrintf("\n Hall still has rooms.");
        return;
    }

    if (getYesNo(confirm, 0))
    {
        hallBuf->hall[thisHall].h_inuse = FALSE;
        hallBuf->hall[thisHall].owned   = FALSE;
        putHall();

       sprintf(msgBuf->mbtext,
       "Hallway %s deleted", hallBuf->hall[thisHall].hallname );

       trap(msgBuf->mbtext, T_SYSOP);
    }
}

/* -------------------------------------------------------------------- */
/*  listhalls()     sysop special to list all hallways                  */
/* -------------------------------------------------------------------- */
void listhalls(void)
{
    int i;

    doCR();
    doCR();
    prtList(LIST_START);
    for (i = 0; i < MAXHALLS; i++)
    {
        if (hallBuf->hall[i].h_inuse)
        {
            prtList(hallBuf->hall[i].hallname);
        }
    }
    prtList(LIST_END);
}


/* -------------------------------------------------------------------- */
/*  newhall()       sysop special to add a new hall                     */
/* -------------------------------------------------------------------- */
void newhall(void)
{
    label hallname, groupname;
    int i, slot = 0, groupslot;


    getString("hall", hallname, NAMESIZE, FALSE, ECHO, "");

    if ( (hallexists(hallname) != ERROR) || !strlen(hallname) )
    {
        mPrintf("\n We already have a %s hall.", hallname);
        return;
    }

    /* search for a free hall slot */

    for (i = 0; i < MAXHALLS && !slot ; i++)
    {
        if (!hallBuf->hall[i].h_inuse)  slot = i;
    }

    if (!slot)
    {
        mPrintf("\n Hall table full.");
        return;
    }

    strcpy(hallBuf->hall[slot].hallname, hallname);
    hallBuf->hall[slot].h_inuse = TRUE;


    getString("group for hall", groupname, NAMESIZE, FALSE, ECHO, "");

    if (!strlen(groupname)) hallBuf->hall[slot].owned = FALSE;

    else
    {
        groupslot = partialgroup(groupname);

        if ( groupslot == ERROR  )
        {
            mPrintf("\n No such group.");
            getHall();
            return;
        }

        else
        {
            hallBuf->hall[slot].owned  = TRUE;
            hallBuf->hall[slot].grpno  = (unsigned char)groupslot;
        }
    }

    /* make current room a window into current hallway */
    /* so we can get back                              */
    hallBuf->hall[thisHall].hroomflags[thisRoom].window = TRUE;

    /* put current room in hall */
    hallBuf->hall[slot].hroomflags[thisRoom].inhall = TRUE;

    /* make current room a window into new hallway */
    hallBuf->hall[slot].hroomflags[thisRoom].window = TRUE;


    if (getYesNo(confirm, 0))
    {
        putHall();

        sprintf(msgBuf->mbtext,
        "Hall %s created", hallBuf->hall[slot].hallname );

        trap(msgBuf->mbtext, T_SYSOP);
    }
    else  getHall();

}


/* -------------------------------------------------------------------- */
/*  renamehall()    sysop special to rename a hallway                   */
/* -------------------------------------------------------------------- */
void renamehall(void)
{
    label hallname, newname, groupname;
    int groupslot, hallslot;

    getString("hall", hallname, NAMESIZE, FALSE, ECHO, "");
    
    hallslot = hallexists(hallname);

    if ( hallslot == ERROR || !strlen(hallname) )
    {
        mPrintf("\n No hall %s", hallname);
        return;
    }

    getString("new name for hall", newname, NAMESIZE, FALSE, ECHO, "");

    if (strlen(newname) && (hallexists(newname) != ERROR))
    {
        mPrintf("\n A %s hall already exists", newname);
        return;
    }

    if(hallBuf->hall[hallslot].owned)
        mPrintf("\n Owned by group %s\n ",
                    grpBuf.group[ hallBuf->hall[hallslot].grpno ].groupname);

    if (!cfg.nonAideRoomOk)
    {
        hallBuf->hall[hallslot].enterRoom = 
  getYesNo("Can users create rooms?", (char)hallBuf->hall[hallslot].enterRoom);
    }
                    
    if (getYesNo("Description", (char)hallBuf->hall[hallslot].described))
    {
        getString("Description Filename", hallBuf->hall[hallslot].htell, 13,
                    FALSE, ECHO, hallBuf->hall[hallslot].htell);

        if (hallBuf->hall[hallslot].htell[0])
        {
            hallBuf->hall[hallslot].described = TRUE;
        } else {
            hallBuf->hall[hallslot].described = FALSE;
        }
    } else {
        hallBuf->hall[hallslot].described = FALSE;
    }

    if (getYesNo("Change group",0))
    {
        getString("new group for hall", groupname, NAMESIZE, FALSE, ECHO, "");

        if (!strlen(groupname))
        {
            hallBuf->hall[hallslot].owned = FALSE;
        } else {
            groupslot = partialgroup(groupname);

            if ( groupslot == ERROR )
            {
                mPrintf("\n No such group.");
                getHall();
                return;
            }

            hallBuf->hall[hallslot].owned = TRUE;
            hallBuf->hall[hallslot].grpno = (unsigned char)groupslot;
        }
    }

    if (strlen(newname))
    {
        strcpy( hallBuf->hall[hallslot].hallname, newname );
    }

    if (getYesNo(confirm, 0))
    {
        putHall();
        sprintf(msgBuf->mbtext,
        "Hall %s renamed %s", hallname, newname);

        trap(msgBuf->mbtext, T_SYSOP);
    } else {
        getHall();
    }
    
}

/* -------------------------------------------------------------------- */
/*  windowfunc()    windows/unwindows room from current hall            */
/* -------------------------------------------------------------------- */
void windowfunc(void)
{
    if (hallBuf->hall[thisHall].hroomflags[thisRoom].window)
    {
        if (getYesNo("Unwindow",0))
        {
             hallBuf->hall[thisHall].hroomflags[thisRoom].window = FALSE;

             sprintf(msgBuf->mbtext,
             "Hall %s made invisible from room %s> by %s",
             hallBuf->hall[thisHall].hallname,
             roomBuf.rbname,
             logBuf.lbname );

             trap(msgBuf->mbtext, T_AIDE);

            aideMessage();

        }
    }
    else

    if (getYesNo("Window",0))
    {
        hallBuf->hall[thisHall].hroomflags[thisRoom].window = TRUE;

        sprintf(msgBuf->mbtext,
        "Hall %s made visible from room %s> by %s",
         hallBuf->hall[thisHall].hallname,
         roomBuf.rbname,
         logBuf.lbname );

         trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

    }
    putHall();
}

/* -------------------------------------------------------------------- */
/*  moveHall()      Moves a hall up or down in the hall list            */
/* -------------------------------------------------------------------- */
void moveHall(int offset)
{
    struct hall_buffer tmp;
    
    mPrintf("Move hall"); doCR(); doCR();
    
    if  (   
            (thisHall > (2)        || offset ==  1)    
         && (thisHall < (MAXHALLS) || offset == -1)
        )
    {
        tmp                             = hallBuf->hall[thisHall];
        hallBuf->hall[thisHall]         = hallBuf->hall[thisHall+offset];
        hallBuf->hall[thisHall+offset]  = tmp;
        
        thisHall += offset;
        
        mPrintf("Hall moved to just after %s.",
                hallBuf->hall[thisHall-1].hallname);  doCR();
        
        putHall();
    }
}


