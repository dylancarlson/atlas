/************************************************************************/
/*                              room2.c                                 */
/*              room code for Citadel bulletin board system             */
/************************************************************************/
#include <direct.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <errno.h>

#include "ctdl.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                              Contents                                */
/*                                                                      */
/*      renameRoom()            sysop special to rename rooms           */
/*      messWithShareList()     shows nodes sharing room and optionally */
/*                              lets people change who shared the room. */
/*                                                                      */
/************************************************************************/
static int directory_l(char *str);

/* ------------------------------------------------------------------------ */
/*  directory_l()   returns wether a directory is locked                    */
/* ------------------------------------------------------------------------ */
static int directory_l(char *str)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    char path[80];

    sprintf(path, "%s\\external.cit", cfg.homepath);
    
    if ((fBuf = fopen(path, "r")) == NULL)  /* ASCII mode */
    {  
	crashout("Can't find route.cit!");
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
	if (line[0] != '#') continue;
   
	if (strnicmp(line, "#DIRE", 5) != SAMESTRING) continue;
     
	parse_it( words, line);

	if (strcmpi(words[0], "#DIRECTORY") == SAMESTRING)
	{
	    if (u_match(str, words[1]))
	    {
		fclose(fBuf);
		return TRUE;
	    }
	}
    }
    fclose(fBuf);
    return FALSE;
}

/***************************************************************************/
/*                                                                         */
/* addRoomIfShould is for saveNewReality's use only.                       */
/*                                                                         */
/***************************************************************************/

void addRoomIfShould (char *curnode,
		      BOOL shouldBePresent,
		      BOOL present,
		      char *room,
		      int  hOutFile)
{
    char line[80], roomwquotes[LABELSIZE+3] ;

    if (curnode)
	if (shouldBePresent && !present)
	    {
	    sprintf (roomwquotes, "\"%s\"", room) ;

	    sprintf (line, "#ROOM    %s ", roomwquotes) ;
	    while (strlen (line) < LABELSIZE+8)
		strcat (line, " ") ;

	    sprintf (line, "%s%s\n", line, roomwquotes) ;

	    write (hOutFile, line, strlen (line)) ;
	    }
}

/* Data structure used my messWithShareList.  Eventually, move it in with  */
/* other data structures.                                                  */

struct messNode          /* for use with messWithShareList */
{
    label nodeName ;
    char  fShared ;
    char  bHotKey ;
    uchar bHotKeyPos ;
} ;


/***************************************************************************/
/*                                                                         */
/* Passed the array from messWithShareList, saveNewReality opens           */
/* NODES.CIT and reads through the file, adding the appropriate #ROOM line */
/* if it needs to be added, removing it if it needs to be removed.         */
/*                                                                         */
/***************************************************************************/

void saveNewReality (struct messNode *nodeArray, char *room, int iTopNode)
{
    int   hInFile, hOutFile, i ;
    char  buf[256], originalBuf[256], *words[16], *curnode = NULL ;
    BOOL  shouldBePresent, present ;

    sprintf (buf, "%s\\NODES.CIT", cfg.homepath) ;
    hInFile = sopen (buf, O_TEXT | O_RDONLY, SH_DENYNO) ;

    sprintf (buf, "%s\\NODES.!!!", cfg.homepath) ;
    unlink (buf) ;

    hOutFile = sopen (buf, O_CREAT | O_TEXT | O_WRONLY,
			   SH_DENYNO,
			   S_IREAD | S_IWRITE) ;

    if (-1 == hInFile)
	crashout ("Problem opening NODES.CIT.  Snorp.") ;

    if (-1 == hOutFile)
	crashout ("Problem opening NODES.!!!.  Foom.") ;

    /* When we identify a #NODE, a BOOL tells us whether that #NODE shares */
    /* room.  If not, and the room is found, it's not passed through to    */
    /* the temporary file.  If so, and the roomname ISN'T found before the */
    /* next #NODE, it is ADDED just before the next #NODE (or EOF).        */

    originalBuf[0] = '\0' ;

    while ( !eof(hInFile) )
	{
	/* In every loop, the first thing we do is write the line from the */
	/* PREVIOUS loop around.                                           */

	write (hOutFile, originalBuf, strlen (originalBuf)) ;

	if (!sfgets (buf, 255, hInFile))
	    break ;				    /* Error reading file. */

	strcpy (originalBuf, buf) ;

	if ('#' != buf[0])			    /* It's not a keyword. */
	    continue ;

	parse_it (words, buf) ;

	/* We are only concerned about #NODE and #ROOM.  If we're reading  */
	/* #NODE line, first check whether a #ROOM line needs to be added  */
	/* for the previous node.                                          */

	if (SAMESTRING == stricmp (words[0], "#NODE"))
	    {
	    addRoomIfShould (curnode, shouldBePresent, present,
			     room, hOutFile) ;

	    /* In any event, find the node's memory bro and fill in BOOLs. */

	    for (i = 0; i <= iTopNode; i++)
		if (SAMESTRING == stricmp (nodeArray[i].nodeName, words[1]))
		    break ;

	    if (i > iTopNode)            /* Couldn't find node in memory.  */
		{
		curnode = NULL ;
		continue ;
		}

	    present = FALSE ;
	    shouldBePresent = nodeArray[i].fShared ;
	    curnode = nodeArray[i].nodeName ;
	    }


	/* If present is TRUE, we don't need to worry about parsing the    */
	/* rest of the #ROOMs, but instead will sit around until another   */
	/* #NODE comes along.  But if present is false, is this line the   */
	/* room we're looking for?  And should we remove it?               */

	if (present)
	    continue ;

	if (SAMESTRING == stricmp (words[0], "#ROOM"))
	    if (SAMESTRING == stricmp (words[1], room))
		{
		present = TRUE ;

		/* If we've found it and wished we hadn't, nuke the buffer */
		/* and end, so nothing gets written.                       */

		if (!shouldBePresent)
		    originalBuf[0] = '\0' ;
		}
	}

    /* Now that we're out of the file, we need to call addRoomIfShould     */
    /* once more to add a #ROOM line to the last node in the nodes file,   */
    /* if necessary.  And finally, write the last line of the file	   */

    addRoomIfShould (curnode, shouldBePresent, present, room, hOutFile) ;
    write (hOutFile, originalBuf, strlen (originalBuf)) ;

    close (hInFile) ;

    if (0 == close (hOutFile))     /* SUccessful close.  Rename... */
	{
	sprintf (buf, "%s\\NODES.CIT", cfg.homepath) ;
	sprintf (originalBuf, "%s\\NODES.!!!", cfg.homepath) ;

	unlink (buf) ;
	rename (originalBuf, buf) ;
	}
    else
	cPrintf ("Unable to close NODES.!!!  Spork.") ;
}


/************************************************************************/
/*      messWithShareList() is an aide special function called from     */
/*      renameRoom which lets aides view a room's share list (the nodes */
/*      we are currently sharing it with), and lets sysops modify this  */
/*      information.                                                    */
/*      messWithShareList can be called with one of two parameters.     */
/*      SHORT_SEE_NO_MODIFY spits out a concatenated list of nodes      */
/*      sharing roomname.  This is ideal for .RVC or something.         */
/*      SEE_WITH_MODIFY spits out a list of nodes and lets sysops alter */
/*      the room's share status with another node.  It also makes the   */
/*      necessary changes to NODES.CIT.                                 */
/************************************************************************/

/* For now, only sysops can modify this list.   Ultimately, I think it     */
/* should be sysop-configurable whether the aides can modify the share     */
/* list or not.                                                            */


#define SHORT_SEE_NO_MODIFY 1
#define SEE_WITH_MODIFY     2
#define MAXDIRECT           50 /* No more than 50 direct-connects */
#define FILLED       0xFF
#define NOT_IN_STRING        0xFA

void messWithShareList (uchar bUsage, label roomname)
{
    struct messNode  nodeArray[MAXDIRECT] ;
	   int       i, j, iNode, iTopNode, iOn ;
	   uchar     fKeyFound, fAnyPrinted, fQuit, fDisplayMenu ;
	   char      ch, oldEcho,
		     path[255], buf[255], *lpch, *lpstr,
		     aHotKeys[] = "BCDEFGHIJKLMNOPQRTUVWXYZ1234567890!#$%&*(",
		     *words[256];
	   int       hFile ;

    /* Initialize the array.  */

    for (i = 0; i < MAXDIRECT; i++)
	{
	strcpy (nodeArray[i].nodeName, '\0') ;
	nodeArray[i].fShared = FALSE ;
	nodeArray[i].bHotKey = (uchar) 0 ;
	nodeArray[i].bHotKeyPos = (uchar) 0 ;
	}

    /* In one single pass, we're going to read the NODES file and fill out */
    /* our array based on all the information we find there!               */

    sprintf (path, "%s\\NODES.CIT", cfg.homepath) ;

    hFile = sopen (path, O_TEXT | O_RDONLY, SH_DENYNO) ;

    if (-1 == hFile)
	crashout ("Problem opening NODES.CIT.") ;

    iTopNode = -1 ; 

    while ( !eof(hFile) )
	{
	if (!sfgets (buf, 255, hFile))
	    break ;                            /* Error reading file. */

	if ('#' != buf[0])            /* It's not a keyword. */
	    continue ;

	if (strnicmp (buf, "#NODE", 5) == SAMESTRING)
	    {
	    parse_it (words, buf) ;

	    if (words[1] == NULL)         /* Invalid #NODE entry. */
		continue ;

	    /* We have a new node and its name! */

	    ++iTopNode ;
	    strcpy (nodeArray[iTopNode].nodeName, words[1]) ;
	    }
	else
	    {
	    if (strnicmp (buf, "#ROOM", 5) != SAMESTRING)
		continue ;

	    if (-1 == iTopNode)       /* We don't have a node yet.  */
		continue ;

	    parse_it (words, buf) ;

	    if (stricmp (words[1], roomname) == SAMESTRING)
		nodeArray[iTopNode].fShared = TRUE ;
	    }
	}
    close (hFile) ;

    /* For the short, non-interactive output, just spew it out and quit.  */

    if (SHORT_SEE_NO_MODIFY == bUsage)
	{
	if (-1 == iTopNode)
	    mPrintf ("(No nodes in NODES.CIT.)") ;
	else
	    {
	    iOn = 0 ;
	    fAnyPrinted = FALSE ;

	    while (iOn <= iTopNode)
		{
		if (TRUE == nodeArray[iOn].fShared)
		    {
		    if (fAnyPrinted)
			mPrintf ("3, 0%s", nodeArray[iOn].nodeName) ;
		    else
			{
			mPrintf ("%s", nodeArray[iOn].nodeName) ;
			fAnyPrinted = TRUE ;
			}
		    }
		++iOn ;
		}
	    if (fAnyPrinted)
		mPrintf ("3.0") ;
	    }
	return ;
	}

    if (SEE_WITH_MODIFY == bUsage)
	{
	/* If we found more nodes than we can hotkey, say so and abort.  */

	if (iTopNode >= (int) strlen (aHotKeys))
	    {
	    mPrintf ("Uh-oh!  There are too many nodes in the userlog to use this function.") ;
	    doCR() ;
	    return ;
	    }

	/* Hotkeys are determined by a priority system. First, we try to   */
	/* get the first character of the node name.  Second, we try for   */
	/* the first character following every space in the name.  Third,  */
	/* we try every character in the name.  Last, we scan the entire   */
	/* hotkey string until we find an unused key.                      */

	for (iNode = 0; iNode <= iTopNode; iNode++)
	    {
	    fKeyFound = FALSE ;

	    /* PLAN ONE: Try for the first character.  */

	    ch = (char) toupper (*nodeArray[iNode].nodeName) ;

	    lpch = strchr (aHotKeys, ch) ;

	    if (lpch)                         /* If char = valid hotkey... */
		{
		nodeArray[iNode].bHotKey = *lpch ;
		nodeArray[iNode].bHotKeyPos = 0 ;
		*lpch = FILLED ;                      /* Ptr into aHotKeys */

		continue ;                      /* Do the next node.  */
		}

	    /* PLAN TWO: Try for the first letter of every word.  Since i  */
	    /* starts at one, it's ok to check the previous character for  */
	    /* a space.                                                    */

	    lpstr = nodeArray[iNode].nodeName ;

	    i = 1 ;

	    while (lpstr[i])      /* While we're not on the final NULL...  */
		{
		if ( ' ' == lpstr[i-1] )          /* Is the prev char a space? */
		    {
		    ch = (char) toupper (lpstr[i]) ;
		    lpch = strchr (aHotKeys, ch) ;
		    if (lpch)                                /* Valid chr? */
			{
			nodeArray[iNode].bHotKey = *lpch ;
			nodeArray[iNode].bHotKeyPos = (uchar) i ;
			*lpch = FILLED ;              /* Ptr into aHotKeys */

			fKeyFound = TRUE ; /* Signal release from for iter.*/
			break ;            /* Releases from while loop.    */
			}
		    }
		++i ;
		}

	    /* If the above loop found a key, move on to the next node.  */

	    if (fKeyFound)
		continue ;

	    /* PLAN THREE:  Check every single character in the string for */
	    /* a valid hot key.                                            */

	    i = 0 ;

	    while (lpstr[i])      /* While we're not on the final NULL...  */
		{
		ch = (char) toupper (lpstr[i]) ;
		lpch = strchr (aHotKeys, ch) ;
		if (lpch)                                    /* Valid chr? */
		    {
		    nodeArray[iNode].bHotKey = *lpch ;
		    nodeArray[iNode].bHotKeyPos = (uchar) i ;
		    *lpch = FILLED ;                  /* Ptr into aHotKeys */
		    fKeyFound = TRUE ;    /* Signal release from for iter. */
		    break ;                   /* Releases from while loop. */
		    }
		++i ;
		}

	    /* If the above loop found a key, move on to the next node. */

	    if (fKeyFound)
		continue ;

	    /* PLAN FOUR: Give the node the first available hot key.       */
	    /* Checking done previously assures that we'll get one.        */

	    i = 0 ;

	    while (FILLED == aHotKeys[i])
		++i ;

	    nodeArray[iNode].bHotKey = aHotKeys[i] ;
	    nodeArray[iNode].bHotKeyPos = NOT_IN_STRING ;
	    aHotKeys[i] = FILLED ;
	    }

	fDisplayMenu = TRUE ;
	fQuit = FALSE ;

	for (;;)
	    {
	    if (fDisplayMenu)
		{
		doCR () ;

		for (iNode = 0; iNode <= iTopNode; iNode++)
		    {
		    strcpy (buf, ".........................") ;
		    lpstr = nodeArray[iNode].nodeName ;
		    i = j = 0 ;
		    while (lpstr[i] != '\0')    /* Doesn't copy the NULL */
			{
			    if ( i == (int) nodeArray[iNode].bHotKeyPos)        /* Turn on bold */
				{
				buf[j] = '' ;
				buf[j+1] = '3' ;
				buf[j+2] = lpstr[i] ;
				buf[j+3] = '' ;
				buf[j+4] = '0' ;
				j += 5 ;
				}
			    else
				{
				buf[j] = lpstr[i] ;
				++j ;
				}
			    ++i ;
			}

		    mPrintf ("<3%c0> %s %s", nodeArray[iNode].bHotKey,
			  buf,
			  nodeArray[iNode].fShared ? "YES" : "No") ;
		    doCR() ;
		    }
		}

	    /* Take user input. */

	    doCR() ;
	    mPrintf("<3S0> to save, <3A0> to abort."); doCR();
	    
	    fDisplayMenu = (BOOL)(!expert) ;

	    outFlag = IMPERVIOUS;

	    doCR();
	    mPrintf("2Add/Drop:0 ");
	
	    oldEcho = echo;
	    echo         = NEITHER;
	    ch  = (char) iChar();
	    echo         = oldEcho;

	    if (!((whichIO == CONSOLE) || gotCarrier()))
		return;

	    switch(toupper(ch))
		{
		case 'A':
		    mPrintf("Abort");   doCR();
		    if (getYesNo("Abort", TRUE))
			return;
		    break;
    
		case 'S':
		    mPrintf("Save"); doCR();
		    if (getYesNo("Save", TRUE))
			{
			saveNewReality(nodeArray, roomname, iTopNode) ;
			return ;
			}
		    break ;

		case '\r':
		case '\n':
		case '?' :
		    mPrintf("Menu"); doCR();
		    fDisplayMenu = TRUE;
		    break;

		default:        /* Check if for hotkey and toggle right entry. */
		    for (i = 0; i <= iTopNode; i++)
			if (toupper(ch) == nodeArray[i].bHotKey)
			    {
			    nodeArray[i].fShared = (uchar) !nodeArray[i].fShared ;
			    mPrintf ("%s %s", nodeArray[i].nodeName,
			       nodeArray[i].fShared ? "added." : "dropped.") ;
			    doCR() ;
			    break ;
			    }

		    /* If it fails to break above, it's not valid input. */

		    if (i > iTopNode)
			{
			mPrintf("%c ? for help", ch); doCR();
			break ;
			}
		}
	    }  /* This is in the right place.  */
	}
}


/************************************************************************/
/*      renameRoom() is sysop special fn                                */
/*      Returns:        TRUE on success else FALSE                      */
/************************************************************************/
void renameRoom(void)
{ 
    char    pathname[64];
    char    summary[500];
    label   roomname;
    label   oldname;
    label   groupname;
    char    line[80];
    char    waspublic;
    int     groupslot;
    char    description[13];
    int     roomslot;
    BOOL    prtMess = TRUE;
    BOOL    quit    = FALSE;
    int     c;
    char    oldEcho;
   
    strcpy(oldname,roomBuf.rbname);
    if (!roomBuf.rbflags.MSDOSDIR)
    {
	roomBuf.rbdirname[0] = '\0';
    }

    doCR();

    do 
    {
	if (prtMess)
	{
	    doCR();
	    outFlag = OUTOK;
	    mPrintf("<3N0> Name.............. %s", roomBuf.rbname);   doCR();
	    mPrintf("<3I0> Infoline.......... %s", roomBuf.descript); doCR();
	    mPrintf("<3D0> Directory......... %s",
			     roomBuf.rbflags.MSDOSDIR
			     ? roomBuf.rbdirname : "None");             doCR();
	    
	    mPrintf("<3L0> Application....... %s",
			     roomBuf.rbflags.APLIC
			     ? roomBuf.rbaplic   : "None");             doCR();
	    
	    mPrintf("<3F0> Description File.. %s", 
			     roomBuf.rbroomtell[0]
			     ? roomBuf.rbroomtell : "None");            doCR();
	    
	    mPrintf("<3G0> Access Group...... %s", 
			     roomBuf.rbflags.GROUPONLY
			     ? grpBuf.group[roomBuf.rbgrpno].groupname
			     : "None");                                 doCR();
	    
	    mPrintf("<3V0> PriVileges Group.. %s",
			     roomBuf.rbflags.PRIVGRP
			     ? grpBuf.group[roomBuf.rbPgrpno].groupname
			     : "None");                                 doCR();
			     
	    if (roomBuf.rbflags.PRIVGRP)
	    {
		mPrintf("    Download only..... %s", 
			     roomBuf.rbflags.DOWNONLY ? "Yes" : "No" ); doCR();
		
		mPrintf("    Upload only....... %s", 
			     roomBuf.rbflags.UPONLY   ? "Yes" : "No" ); doCR();
		
		mPrintf("    Read Only......... %s", 
			     roomBuf.rbflags.READONLY ? "Yes" : "No" ); doCR();
		
		mPrintf("    Group moderates... %s", 
			     roomBuf.rbflags.GRP_MOD  ? "Yes" : "No" ); doCR();
	    }
	    
	    mPrintf("<3H0> Hidden............ %s", 
			     roomBuf.rbflags.PUBLIC ? "No" : "Yes" );   doCR();
	    
	    mPrintf("<3Y0> Anonymous......... %s", 
			     roomBuf.rbflags.ANON ? "Yes" : "No" );     doCR();
	    
	    mPrintf("<3O0> BIO............... %s", 
			     roomBuf.rbflags.BIO ? "Yes" : "No" );      doCR();
						
	    mPrintf("<3M0> Moderated......... %s", 
			     roomBuf.rbflags.MODERATED ? "Yes" : "No" );doCR();
#ifdef NETWORK
	    mPrintf("<3E0> Networked/Shared.. %s", 
			     roomBuf.rbflags.SHARED ? "Yes" : "No" );   doCR();

	    if (roomBuf.rbflags.SHARED)
		{
		mPrintf ("<3W0> Shared With....... ") ;
		messWithShareList (SHORT_SEE_NO_MODIFY, oldname) ;
		doCR() ;
		}
#endif /* NETWORK */

	    mPrintf("<3P0> Permanent......... %s", 
			     roomBuf.rbflags.PERMROOM ? "Yes" : "No" ); doCR();
	    
	    mPrintf("<3U0> Subject........... %s", 
			     roomBuf.rbflags.SUBJECT ? "Yes" : "No" ); doCR();
	    
	    doCR();
	    mPrintf("<3S0> to save, <3A0> to abort."); doCR();
	    
	    prtMess = (BOOL)(!expert);
	}
	
	outFlag = IMPERVIOUS;

	doCR();
	mPrintf("2Change:0 ");
	
	oldEcho = echo;
	echo    = NEITHER;
	c       = iChar();
	echo    = oldEcho;

	if (!CARRIER)
	    return;

	switch(toupper(c))
	{
	case 'L':
	    mPrintf("Application"); doCR();
	    
	    if (sysop && onConsole)
	    {
		if ( getYesNo("Application", (uchar)(roomBuf.rbflags.APLIC) ) )
		{
		    getString("Application filename", description, 13, FALSE,
			    ECHO, (roomBuf.rbaplic[0]) ? roomBuf.rbaplic : "");

		    strcpy(roomBuf.rbaplic, description);

		    roomBuf.rbflags.APLIC = TRUE;
		}
		else
		{
		    roomBuf.rbaplic[0] = '\0';
		    roomBuf.rbflags.APLIC = FALSE;
		}
	    }
	    else
	    {
		mPrintf("Must be Sysop at console to enter application.");
		doCR();
	    }
	    break;
   
	case 'N':
	    mPrintf("Name"); doCR();
	    
	    getString("New room name", roomname, NAMESIZE, FALSE, ECHO, 
		      roomBuf.rbname);
	    normalizeString(roomname);
	    roomslot = roomExists(roomname);
	    if (roomslot >= 0  &&  roomslot != thisRoom)
	    {
		mPrintf("A \"%s\" room already exists.\n", roomname);
	    }
	    else 
	    {
		strcpy(roomBuf.rbname, roomname); /* also in room itself */
	    }
	    break;
    
	case 'I':
	    mPrintf("Info-line \n ");
	    getNormStr("New room Info-line", roomBuf.descript, 79, ECHO);
	    break;
    
	case 'D':
	    mPrintf("Directory"); doCR();

	    if (sysop)
	    {
		if (getYesNo("Directory room", (uchar)roomBuf.rbflags.MSDOSDIR))
		{
		    roomBuf.rbflags.MSDOSDIR = TRUE;

		    if (!roomBuf.rbdirname[0])
			mPrintf(" No drive and path");
		    else
			mPrintf(" Now space %s",roomBuf.rbdirname);

		    doCR();
		    getString("Path", pathname, 63, FALSE, ECHO,
		     (roomBuf.rbdirname[0]) ? roomBuf.rbdirname : cfg.dirpath);
		    pathname[0] = (char)toupper(pathname[0]);

		    doCR();
		    mPrintf("Checking pathname \"%s\"", pathname);
		    doCR();
		    
		    if (directory_l(pathname) && !onConsole)
		    {
			logBuf.VERIFIED = TRUE;

			sprintf(msgBuf->mbtext, 
				"Security violation on dirctory %s by %s\n "
				"User unverified.", pathname, logBuf.lbname);
			aideMessage();

			doCR();
			mPrintf("Security violation, your account is being "
				"held for sysop's review"); 
			doCR();
			Hangup();

			getRoom(thisRoom);
			return;
		    }

		    if (changedir(pathname) != -1)
		    {
			mPrintf(" Now space %s", pathname);
			doCR();
			strcpy(roomBuf.rbdirname, pathname);
		    }
		    else
		    {
			mPrintf("%s does not exist! ", pathname);
			if (getYesNo("Create", 0))
			{
			    if (mkdir(pathname) == -1)
			    {
				mPrintf("mkdir() ERROR!");
				strcpy(roomBuf.rbdirname, cfg.temppath);
			    }
			    else
			    {
				strcpy(roomBuf.rbdirname, pathname);
				mPrintf(" Now space %s",roomBuf.rbdirname);
				doCR();
			    }
			}
			else
			{
			    strcpy(roomBuf.rbdirname, cfg.temppath);
			}
		    }

		    if (roomBuf.rbflags.PRIVGRP && roomBuf.rbflags.MSDOSDIR)
		    {
			roomBuf.rbflags.DOWNONLY =
			    getYesNo("Download only", 
				    (uchar)roomBuf.rbflags.DOWNONLY);

			if (!roomBuf.rbflags.DOWNONLY)
			{
			    roomBuf.rbflags.UPONLY   =  getYesNo("Upload only", 
						 (uchar)roomBuf.rbflags.UPONLY);
			}
		    }
		}
		else
		{
		    roomBuf.rbflags.MSDOSDIR = FALSE;
		    roomBuf.rbflags.DOWNONLY = FALSE;
		}
		changedir(cfg.homepath);
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make directories.");
		doCR();
	    }
	    break;
    
	case 'F':
	    mPrintf("Description File"); doCR();

	    if (cfg.roomtell && sysop)
	    {
		if ( getYesNo("Display room description File",
			(uchar)(roomBuf.rbroomtell[0] != '\0') ) )
		{
		    getString("Description Filename", description, 13, FALSE,
		    ECHO, (roomBuf.rbroomtell[0]) ? roomBuf.rbroomtell : "");
		    strcpy(roomBuf.rbroomtell, description);
		}
		else roomBuf.rbroomtell[0] = '\0';
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop and have Room descriptions configured.");
		doCR();
	    }
	    break;
    
	case 'G':
	    mPrintf("Access Group"); doCR();
	    
	    if ((thisRoom > 2) || (thisRoom > 0 && sysop))
	    {
		if (getYesNo("Change Group", 0))
		{
		    getString("Group for room <CR> for no group",
				    groupname, NAMESIZE, FALSE, ECHO, "");

		    roomBuf.rbflags.GROUPONLY = TRUE;

		    groupslot = partialgroup(groupname);

		    if (!strlen(groupname) || (groupslot == ERROR) )
		    {
			roomBuf.rbflags.GROUPONLY = 0;

			if (groupslot == ERROR && strlen(groupname))
			    mPrintf("No such group.");
		    }

		    if (roomBuf.rbflags.GROUPONLY)
		    {
			roomBuf.rbgrpno  = (unsigned char)groupslot;
		     /* roomBuf.rbgrpgen = grpBuf.group[groupslot].groupgen;*/
		    }
		}
	    }
	    else
	    {
		if(thisRoom > 0)
		{
		    doCR();
		    mPrintf("Must be Sysop to change group for Mail> or Aide)");
		    doCR();
		}
		else
		{
		    doCR();
		    mPrintf("Lobby> can never be group only");
		    doCR();
		}
	    }
	    break;
	
	case 'V':
	    mPrintf("Privileges Group"); doCR();
	    
	    if (getYesNo("Change Group", 0))
	    {
		getString("Group for room <CR> for no group",
				groupname, NAMESIZE, FALSE, ECHO, "");

		roomBuf.rbflags.PRIVGRP = TRUE;

		groupslot = partialgroup(groupname);

		if (!strlen(groupname) || (groupslot == ERROR) )
		{
		    roomBuf.rbflags.PRIVGRP   = FALSE;
		    roomBuf.rbflags.READONLY  = FALSE;
		    roomBuf.rbflags.DOWNONLY  = FALSE;
		    roomBuf.rbflags.UPONLY    = FALSE;
		    roomBuf.rbflags.GRP_MOD   = FALSE;

		    if (groupslot == ERROR && strlen(groupname))
			mPrintf("No such group.");
		}

		if (roomBuf.rbflags.PRIVGRP )
		{
		    roomBuf.rbPgrpno  = (unsigned char)groupslot;
		 /* roomBuf.rbPgrpgen = grpBuf.group[groupslot].groupgen; */
		}
	    }
	    
	    if (roomBuf.rbflags.PRIVGRP)
	    {
		roomBuf.rbflags.READONLY =
		    getYesNo("Read only", (uchar)roomBuf.rbflags.READONLY);
		
		roomBuf.rbflags.GRP_MOD  =
		    getYesNo("Group Moderates", (uchar)roomBuf.rbflags.GRP_MOD);
	    
		if (roomBuf.rbflags.MSDOSDIR)
		{    
		    roomBuf.rbflags.DOWNONLY =
			getYesNo("Download only", 
				(uchar)roomBuf.rbflags.DOWNONLY);
    
		    if (!roomBuf.rbflags.DOWNONLY)
		    {
			roomBuf.rbflags.UPONLY   =  getYesNo("Upload only", 
					     (uchar)roomBuf.rbflags.UPONLY);
		    }
		}
	    }
	    
	    break;
	    
	case 'H':
	    mPrintf("Hidden Room"); doCR();
	    
	    if ((thisRoom > 2) || (thisRoom>0 && sysop))
	    {
		waspublic = (uchar)roomBuf.rbflags.PUBLIC;

		roomBuf.rbflags.PUBLIC =
		    !getYesNo("Hidden room", (uchar)(!roomBuf.rbflags.PUBLIC));

		if (waspublic && (!roomBuf.rbflags.PUBLIC))
		{
		    roomBuf.rbgen = (uchar)((roomBuf.rbgen +1) % MAXGEN);
		    logBuf.lbroom[thisRoom].lbgen = roomBuf.rbgen;
		}
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) hidden.");
		doCR();
	    }
	    break;
    
	case 'Y':
	    mPrintf("Anonymous Room"); doCR();
	    
	    if ((thisRoom > 2) || (thisRoom>0 && sysop))
	    {
		roomBuf.rbflags.ANON =
		     getYesNo("Anonymous room", (uchar)(roomBuf.rbflags.ANON));
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) Anonymous.");
		doCR();
	    }
	    break;
	
	case 'O':
	    mPrintf("BIO Room"); doCR();
	    
	    if ((thisRoom > 2) || (thisRoom>0 && sysop))
	    {
		roomBuf.rbflags.BIO =
		    getYesNo("BIO room", (uchar)(roomBuf.rbflags.BIO));
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make Lobby>, Mail> or Aide) BIO.");
		doCR();
	    }
	    break;
	    
	case 'M':
	    mPrintf("Moderated"); doCR();
	    
	    if (sysop)
	    {
		if (getYesNo("Moderated", (uchar)(roomBuf.rbflags.MODERATED) ))
		    roomBuf.rbflags.MODERATED = TRUE;
		else
		    roomBuf.rbflags.MODERATED = FALSE;
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make Moderated rooms.");
		doCR();
	    }
	    break;

#ifdef NETWORK

	case 'E':
	    mPrintf("Networked/Shared"); doCR();
	    
	    if (sysop)
	    {
		/* Perhaps rooms made shareable should also automatically */
		/* become permenant, as well.                             */

		BOOL fShared = (BOOL) roomBuf.rbflags.SHARED ;

		roomBuf.rbflags.SHARED = getYesNo("Networked/Shared room",
					 (uchar)fShared) ;

		if ((roomBuf.rbflags.SHARED) && (!fShared))
		    roomBuf.rbflags.PERMROOM = TRUE ;
	    }
	    else
	    {
		doCR();
		mPrintf("Must be Sysop to make Shared rooms.");
		doCR();
	    }
	    break;

	case 'W':
	    mPrintf ("Shared With");  doCR();

	    if (roomBuf.rbflags.SHARED)
		messWithShareList (SEE_WITH_MODIFY, oldname) ;
	    else
		{
		doCR() ;
		mPrintf ("Unshared rooms don't have share lists.") ;
		doCR() ;
		}
	    break ;

#endif /* NETWORK */

	case 'P':
	    mPrintf("Permanent");
	    doCR();
	    if (thisRoom > DUMP)
	    {
		if (!roomBuf.rbflags.MSDOSDIR)
		{
		    roomBuf.rbflags.PERMROOM =
			getYesNo("Permanent", (uchar)roomBuf.rbflags.PERMROOM);
		}
		else
		{
		    roomBuf.rbflags.PERMROOM = 1;
		    doCR();
		    mPrintf("Directory rooms are always Permanent.");
		    doCR();
		}
	    }
	    else
	    {
		doCR();
		mPrintf("Lobby> Mail> Aide) or Dump> always Permanent.");
		doCR();
	    }
	    break;
   
	case 'U':
	    mPrintf("Subject"); doCR();
	    
	    roomBuf.rbflags.SUBJECT = getYesNo("Ask for subject in room",
				     (uchar)roomBuf.rbflags.SUBJECT);
	    break;
	
	case 'S':
	    mPrintf("Save");  doCR();
	    if (getYesNo("Save changes", FALSE))
	    {
		noteRoom();
		putRoom(thisRoom);

		/* trap file line */
		sprintf(line, "Room \'%s\' changed to \'%s\' by %s",
				oldname, roomBuf.rbname, logBuf.lbname);
		trap(line, T_AIDE);

		/* Aide room */
		formatSummary(summary);
		sprintf(msgBuf->mbtext, "%s \n%s", line, summary);
		aideMessage();

		return;
	    }
	    break;
	
	case 'A':
	    mPrintf("Abort");  doCR();
	    if (getYesNo("Abort", TRUE))
	    {
		getRoom(thisRoom);
		return;
	    }
	    break;
    
	case '\r':
	case '\n':
	case '?':
	    mPrintf("Menu"); doCR();
	    prtMess = TRUE;
	    break;
    
	default:
	    mPrintf("%c ? for help", c); doCR();
	    break;
	
	}
    } while (!quit);
}
