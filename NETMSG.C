/* -------------------------------------------------------------------- */
/*  NETMSG.C                 Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*      Networking libs for the Citadel bulletin board system           */
/*              Networking message handling rutines                     */
/* -------------------------------------------------------------------- */
#ifdef NETWORK

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"
/* The following includes can be removed when my RAM file handling system works.  */
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <errno.h>

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  GetStr()         gets a null-terminated string from a file          */
/*  PutStr()         puts a null-terminated string to a file            */
/*  GetMessage()     Gets a message from a file, returns sucess         */
/*  PutMessage()     Puts a message to a file                           */
/*  NewRoom()        Puts all new messages in a room to a file          */
/*  saveMessage()    saves a message to file if it is netable           */
/*  ReadMsgFl()      Reads a message file into thisRoom                 */
/*  nodeSharesRoom() Does the ROUTE.CIT file say we share this room?    */
/*  netcanseeroom()  Can the node see this room?                        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  09/17/90    (PAT)   Split from NET.C for overlays                   */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  GetStr()        gets a null-terminated string from a file           */
/* -------------------------------------------------------------------- */
void GetStr(FILE *fl, char *str, int mlen)
{
    int   l = 0;
    uchar ch = 1;
  
    while(!feof(fl) && ch)
    {
	ch=(uchar)fgetc(fl);
	if ((ch != 0xFF && ch != '\r')/* tfilter[ch]*/ && l < mlen)
	{
	    str[l]=ch /* tfilter[ch] */;
	    l++;
	}
    }
    str[l]='\0';
}

/* -------------------------------------------------------------------- */
/*  GetFStr()       gets a null-terminated FILTERED string from a file  */
/*                  or Totalitarian Communist string processing.        */
/*                  fLevel      0 = filters ^B codes only               */
/*                              1 = filters ^A codes & ^B codes.        */
/*                              2 = IBM extended                        */
/* -------------------------------------------------------------------- */
void GetFStr(FILE *fl, char *str, int mlen, int fLevel)
{
    int   l = 0;
    uchar ch = 1;
    BOOL  ctrla = FALSE;
    BOOL  ctrlb = FALSE;
    
    while(!feof(fl) && ch)
    {
	ch=(uchar)fgetc(fl);
	if (   (ch != 0xFF && ch != '\r')       /* no FFs, or CRs (LFs only) */
	    && ( tfilter[ch] || fLevel < 2)      /* No IBM extended */
	    && l < mlen)                        /* Not past end of str */
	{
	    if (ch == CTRL_A /* CTRL-A */)
	    {
		ctrla = TRUE;
	    }
	    else
	    if (ch == CTRL_B /* CTRL-A */)
	    {
		ctrlb = TRUE;
	    }
	    else
	    if (ctrlb || (ctrla && fLevel > 0))
	    {
		ctrla = TRUE;
		ctrlb = FALSE;
	    }
	    else
	    {
		if (ctrla)
		{
		    str[l]=CTRL_A;
		    l++;     
		    ctrla=FALSE;
		}
		
		str[l]=ch;
		l++;
	    }
	}
    }
    str[l]='\0';
}

/* -------------------------------------------------------------------- */
/*  PutStr()        puts a null-terminated string to a file             */
/* -------------------------------------------------------------------- */
void PutStr(FILE *fl, char *str)
{
    fwrite(str, sizeof(char), (strlen(str) + 1), fl);
}

/* -------------------------------------------------------------------- */
/*  GetMessage()    Gets a message from a file, returns sucess          */
/* -------------------------------------------------------------------- */
BOOL GetMessage(FILE *fl)
{
    char c;

    /* clear message buffer out */
    clearmsgbuf();

    /* find start of message */
    do
    {
	c = (uchar)fgetc(fl);
    } while (c != -1 && !feof(fl));

    if (feof(fl))
	return FALSE;

    /* get message's attribute byte */
    msgBuf->mbattr = (uchar)fgetc(fl);

    GetStr(fl, msgBuf->mbId, LABELSIZE);

    do 
    {
	c = (uchar)fgetc(fl);
	switch (c)
	{
	case 'A':     GetFStr(fl, msgBuf->mbauth,  LABELSIZE, 0);    break;
	case 'B':     GetFStr(fl, msgBuf->mbsub,   79       , 0);    break;
	case 'D':     GetStr(fl,  msgBuf->mbtime,  LABELSIZE);       break;
	case 'F':     GetFStr(fl, msgBuf->mbfwd,   LABELSIZE, 0);    break;
	case 'G':     GetFStr(fl, msgBuf->mbgroup, LABELSIZE, 0);    break;
	case 'I':     GetStr(fl,  msgBuf->mbreply, LABELSIZE);       break;
	case 'J':     GetFStr(fl, msgBuf->mbcreg,  LABELSIZE, 0);    break;
	case 'j':     GetFStr(fl, msgBuf->mbccont, LABELSIZE, 0);    break;
	case 'M':     /* will be read off disk later */              break;
	case 'N':     GetFStr(fl, msgBuf->mbtitle, LABELSIZE, 0);    break;
	case 'n':     GetFStr(fl, msgBuf->mbsur,   LABELSIZE, 0);    break;
	case 'O':     GetFStr(fl, msgBuf->mboname, LABELSIZE, 0);    break;
	case 'o':     GetFStr(fl, msgBuf->mboreg,  LABELSIZE, 0);    break;
	case 'P':     GetFStr(fl, msgBuf->mbfpath, 256      , 0);    break;
	case 'p':     GetFStr(fl, msgBuf->mbtpath, 256      , 0);    break;
	case 'Q':     GetFStr(fl, msgBuf->mbocont, LABELSIZE, 0);    break;
	case 'q':     GetFStr(fl, msgBuf->mbczip,  LABELSIZE, 0);    break;
	case 'R':     GetFStr(fl, msgBuf->mbroom,  LABELSIZE, 0);    break;
	case 'S':     GetStr(fl,  msgBuf->mbsrcId, LABELSIZE);       break;
	case 's':     GetFStr(fl, msgBuf->mbsoft,  LABELSIZE, 0);    break;
	case 'T':     GetFStr(fl, msgBuf->mbto,    LABELSIZE, 0);    break;
/*      case 'X':     GetStr(fl,  msgBuf->mbx,     LABELSIZE);       break; */
	case 'Z':     GetFStr(fl, msgBuf->mbzip,   LABELSIZE, 0);    break;
	case 'z':     GetFStr(fl, msgBuf->mbrzip,  LABELSIZE, 0);    break;
	case '.':     GetFStr(fl, msgBuf->mbsig,   90       , 0);    break;
	case '_':     GetFStr(fl, msgBuf->mbusig,  90       , 0);    break;
	
	default:
	    GetStr(fl, msgBuf->mbtext, cfg.maxtext);  /* discard unknown field  */
	    msgBuf->mbtext[0]    = '\0';
	    break;
	}
    } while (c != 'M' && !feof(fl));

    if (feof(fl))
    {
	return FALSE;
    }

    GetFStr(fl, msgBuf->mbtext, cfg.maxtext, 0);  /* get the message field  */
    
    if (!*msgBuf->mboname)
    {
	strcpy(msgBuf->mboname, node.ndname);
    }

    if (!*msgBuf->mboreg)
    {
	strcpy(msgBuf->mboreg, node.ndregion);
    }

    if (!*msgBuf->mbsrcId)
    {
	strcpy(msgBuf->mbsrcId, msgBuf->mbId);
    }

    /*
     * If the other node did not set up a from path, do it.
     */
    if (!*msgBuf->mbfpath)
    {
	if (strcmpi(msgBuf->mboname, node.ndname) == 0)
	{
	    strcpy(msgBuf->mbfpath, msgBuf->mboname);
	}
	else
	{
	    /* last node did not originate, make due with what we got... */
	    strcpy(msgBuf->mbfpath, msgBuf->mboname);
	    strcat(msgBuf->mbfpath, "!..!");
	    strcat(msgBuf->mbfpath, node.ndname);
	}
    }


    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  PutMessage()    Puts a message to a file                            */
/* -------------------------------------------------------------------- */
void PutMessage(FILE *fl)
{
    /* write start of message */
    fputc(0xFF, fl);

    /* put message's attribute byte */
    msgBuf->mbattr = (uchar)(msgBuf->mbattr & (ATTR_RECEIVED|ATTR_REPLY));
    fputc(msgBuf->mbattr, fl);

    /* put local ID # out */
    PutStr(fl, msgBuf->mbId);

    if (!msgBuf->mbsrcId[0])
    {
	strcpy(msgBuf->mboname, cfg.nodeTitle);
	strcpy(msgBuf->mboreg,  cfg.nodeRegion);
	strcpy(msgBuf->mbocont, cfg.nodeContry);
	strcpy(msgBuf->mbsrcId, msgBuf->mbId);
	strcpy(msgBuf->mbsoft,  programName);
	strcat(msgBuf->mbsoft,  " ");
	strcat(msgBuf->mbsoft,  version);


	strcpy(msgBuf->mbcreg,  cfg.twitRegion);
	strcpy(msgBuf->mbccont, cfg.twitCountry);

    }
    
    if (*msgBuf->mbfpath)
    {
	strcat(msgBuf->mbfpath, "!");
    }
    strcat(msgBuf->mbfpath, cfg.nodeTitle);

    if (!msgBuf->mbtime[0])
    {
	sprintf(msgBuf->mbtime, "%ld", time(NULL));
    }
    
    fputc('A', fl); PutStr(fl, msgBuf->mbauth);
    fputc('D', fl); PutStr(fl, msgBuf->mbtime);
    fputc('O', fl); PutStr(fl, msgBuf->mboname);
    fputc('o', fl); PutStr(fl, msgBuf->mboreg);
    fputc('S', fl); PutStr(fl, msgBuf->mbsrcId);
    fputc('P', fl); PutStr(fl, msgBuf->mbfpath);
    
    if (msgBuf->mbsub[0])   { fputc('B', fl); PutStr(fl, msgBuf->mbsub);   }
    if (msgBuf->mbfwd[0])   { fputc('F', fl); PutStr(fl, msgBuf->mbfwd);   }
    if (msgBuf->mbgroup[0]) { fputc('G', fl); PutStr(fl, msgBuf->mbgroup); }
    if (msgBuf->mbreply[0]) { fputc('I', fl); PutStr(fl, msgBuf->mbreply); }
    if (msgBuf->mbcreg[0])  { fputc('J', fl); PutStr(fl, msgBuf->mbcreg);  }
    if (msgBuf->mbccont[0]) { fputc('j', fl); PutStr(fl, msgBuf->mbccont); }
    if (msgBuf->mbtitle[0]) { fputc('N', fl); PutStr(fl, msgBuf->mbtitle); }
    if (msgBuf->mbsur[0])   { fputc('n', fl); PutStr(fl, msgBuf->mbsur);   }
    if (msgBuf->mbtpath[0]) { fputc('p', fl); PutStr(fl, msgBuf->mbtpath); }
    if (msgBuf->mbocont[0]) { fputc('Q', fl); PutStr(fl, msgBuf->mbocont); }
    if (msgBuf->mbczip[0])  { fputc('q', fl); PutStr(fl, msgBuf->mbczip);  }
    if (msgBuf->mbroom[0])  { fputc('R', fl); PutStr(fl, msgBuf->mbroom);  }
    if (msgBuf->mbsoft[0])  { fputc('s', fl); PutStr(fl, msgBuf->mbsoft);  }
    if (msgBuf->mbto[0])    { fputc('T', fl); PutStr(fl, msgBuf->mbto);    }
    if (msgBuf->mbzip[0])   { fputc('Z', fl); PutStr(fl, msgBuf->mbzip);   }
    if (msgBuf->mbrzip[0])  { fputc('z', fl); PutStr(fl, msgBuf->mbrzip);  }
    if (msgBuf->mbsig[0])   { fputc('.', fl); PutStr(fl, msgBuf->mbsig);   }
    if (msgBuf->mbusig[0])  { fputc('_', fl); PutStr(fl, msgBuf->mbusig);  }

    /* put the message field  */
    fputc('M', fl); PutStr(fl, msgBuf->mbtext);
}

/* -------------------------------------------------------------------- */
/*  NewRoom()       Puts all new messages in a room to a file           */
/* -------------------------------------------------------------------- */
void NewRoom(int room, char *filename)
{
    int   i, h;
    char str[100];
    ulong lowLim, highLim, msgNo;
    FILE *file;

    lowLim  = logBuf.lbvisit[ logBuf.lbroom[room].lvisit ] + 1;
    highLim = cfg.newest;

    logBuf.lbroom[room].lvisit = 0;

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim)  lowLim = cfg.oldest;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "ab");
    if (!file)
    {
	return;
    }

    h = hash(cfg.nodeTitle);
    
    for (i = 0; i != (int)sizetable(); i++)
    {
	msgNo = (ulong)(cfg.mtoldest + i);
	
	if ( msgNo >= lowLim && highLim >= msgNo )
	{
	    /* skip messages not in this room */
	    if (msgTab4[i].mtroomno != (uchar)room) continue;
    
	    /* no open messages from the system */
	    if (msgTab6[i].mtauthhash == h) continue;
    
	    /* skip mail */
	    if (msgTab1[i].mtmsgflags.MAIL) continue;
    
	    /* No problem user shit */
	    if (
		(msgTab1[i].mtmsgflags.PROBLEM || msgTab1[i].mtmsgflags.MODERATED) 
	    && !(msgTab1[i].mtmsgflags.MADEVIS || msgTab1[i].mtmsgflags.MADEVIS)
	       )
	    { 
		continue;
	    }

	    saveMessage( msgNo, file );
	    mread ++;
	}
    }
    fclose(file);
}

/* -------------------------------------------------------------------- */
/*  saveMessage()   saves a message to file if it is netable            */
/* -------------------------------------------------------------------- */
#define msgstuff  msgTab1[slot].mtmsgflags  
void saveMessage(ulong id, FILE *fl)
{
    ulong here;
    ulong loc;
    int   slot;
    FILE *fl2;
    
    slot = indexslot(id);
    
    if (slot == ERROR) return;

    if (msgTab1[slot].mtmsgflags.COPY)
    {
	copyflag     = TRUE;
	originalId   = id;
	originalattr = 0;

	originalattr = (uchar)
		       (originalattr | (msgstuff.RECEIVED)?ATTR_RECEIVED :0 );
	originalattr = (uchar)
		       (originalattr | (msgstuff.REPLY   )?ATTR_REPLY : 0 );
	originalattr = (uchar)
		       (originalattr | (msgstuff.MADEVIS )?ATTR_MADEVIS : 0 );

#ifdef GOODBYE                       
	if (msgTab3[slot].mtoffset <= (ushort)slot)
	    saveMessage( (ulong)(id - (ulong)msgTab3[slot].mtoffset), fl);
#endif
	if (msgTab8[slot].mtomesg  <= (ushort)slot)
	    saveMessage( (ulong)(id - (ulong)msgTab8[slot].mtomesg ), fl);

	return;
    }

    /* in case it returns without clearing buffer */
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbto[   0]  = '\0';

    loc = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    if (copyflag)  slot = indexslot(originalId);

    if (!mayseeindexmsg(slot) && !msgTab1[slot].mtmsgflags.NET) return;

    fseek(msgfl, loc, 0);

    getMessage();
    getMsgStr(msgBuf->mbtext, cfg.maxtext);

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if ((int)here == 1) return;

    if (!mayseemsg() && !msgTab1[slot].mtmsgflags.NET) return;

    if (here != id )
    {
	cPrintf("Can't find message. Looking for %lu at byte %ld!\n ",
		 id, loc);
	return;
    }

    if (msgBuf->mblink[0])
    {
	if ((fl2 = fopen(msgBuf->mblink, "rt")) == NULL)
	{
	    return;
	}
	GetFileMessage(fl2, msgBuf->mbtext, cfg.maxtext);
	fclose(fl2);
    }

#ifdef HENGE    
    if (node.network == NET_HENGE)
    {
	HengePutMessage(fl);
    }
    else
    {
#endif         
	PutMessage(fl);
#ifdef HENGE    
    }
#endif         
}

/* -------------------------------------------------------------------- */
/*  ReadMsgFile()   Reads a message file into thisRoom                  */
/* -------------------------------------------------------------------- */
int ReadMsgFl(int room, char *filename, char *here, char *there)
{
    FILE *file, *fl;
    char str[100];
    ulong oid, loc;
    long l;
    int oauth, i, bad, /* oname, */ temproom, lp, goodmsg = 0;

    expired = 0;   duplicate = 0;

    sprintf(str, "%s\\%s", cfg.temppath, filename);

    file = fopen(str, "rb");

    if (!file)
	return -1;

    while(GetMessage(file) == TRUE)
	{
	msgBuf->mbroomno = (uchar)room;

	sscanf(msgBuf->mbsrcId, "%ld", &oid);
	/* oname = hash(msgBuf->mboname); */
	oauth = hash(msgBuf->mbauth);

	memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );

	bad = FALSE;

	if (strcmpi(cfg.nodeTitle, msgBuf->mboname) == SAMESTRING)
	    {
	    bad = TRUE; 
	    duplicate++; 
	    }

	if (*msgBuf->mbzip) /* is mail */
	    {
	    /* not for this system */
	    if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING)
		{
		if (!save_mail())
		    {
		    clearmsgbuf();
		    strcpy(msgBuf->mbauth, "Sysop");
		    strcpy(msgBuf->mbto,   msgBuf2->mbauth);
		    strcpy(msgBuf->mbzip,  msgBuf2->mboname);
		    strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
		    strcpy(msgBuf->mbroom, msgBuf2->mbroom);
		    sprintf(msgBuf->mbtext, 
			   " \n Can not find route to '%s'.", msgBuf2->mbzip);
		    amPrintf( 
			" Can not find route to '%s' in message from '%s'.\n",
			msgBuf2->mbzip, msgBuf2->mboname);
		    netError = TRUE;
		
		    save_mail();
		    duplicate++;
		    }
		else
		    {
		    expired++;
		    }
		bad = TRUE;
		}
	    else 
		{
		/* for this system  */
		
		if (strcmpi(msgBuf->mbto, cfg.nodeTitle) == SAMESTRING)
		    {
		    /* Special command..  */

		    sprintf(str, "%s\\OUTPUT.NET", cfg.transpath);
		    if ((fl = fopen(str, "ab")) != NULL)
			{
			PutMessage(fl);
			fclose(fl);
			}

		    bad = TRUE;
		    expired++;
		    }
		else
		    if (*msgBuf->mbto && personexists(msgBuf->mbto) == ERROR
			&& strcmpi(msgBuf->mbto, "Sysop") != SAMESTRING)
			{
			clearmsgbuf();
			strcpy(msgBuf->mbauth, "Sysop");
			strcpy(msgBuf->mbto,   msgBuf2->mbauth);
			strcpy(msgBuf->mbzip,  msgBuf2->mboname);
			strcpy(msgBuf->mbrzip, msgBuf2->mboreg);
			strcpy(msgBuf->mbroom, msgBuf2->mbroom);
			sprintf(msgBuf->mbtext,
			    " \n No '%s' user found on %s.", msgBuf2->mbto,
			    cfg.nodeTitle);
			save_mail();
			bad = TRUE;
			duplicate++;
			}
		}
	    }
	else 
	    {
	    /* is public */
	    if (!bad)
	    {
		for (i = sizetable(); i != -1 && !bad; i--)
		{

		/* just check origin id and author hash */
		/*  if (msgTab9[i].mtorigin == oname */
		    if (msgTab6[i].mtauthhash == oauth
		       && (uint)oid == msgTab8[i].mtomesg)
		    {
			loc = msgTab2[i].mtmsgLoc;
			fseek(msgfl, loc, 0);
			getMessage();
			if (strcmpi(msgBuf->mbauth, msgBuf2->mbauth)   
								== SAMESTRING
			 && strcmpi(msgBuf->mboname, msgBuf2->mboname)
								== SAMESTRING
			 && strcmpi(msgBuf->mbtime, msgBuf2->mbtime)  
								== SAMESTRING
			 && strcmpi(msgBuf->mbsrcId, msgBuf2->mbsrcId)  
								== SAMESTRING
			   )
			{
			    bad = TRUE; 
			    duplicate++; 
			}
		    }
		}
	    }

	    memcpy( msgBuf, msgBuf2, sizeof(struct msgB) );
    
	    /* fix group only messages, or discard them! */
	    if (*msgBuf->mbgroup && !bad)
	    {
		bad = TRUE;
		for (i=0; node.ndgroups[i].here[0]; i++)
		{
		    if (strcmpi(node.ndgroups[i].there, msgBuf->mbgroup) == SAMESTRING)
		    {
			strcpy(msgBuf->mbgroup, node.ndgroups[i].here);
			bad = FALSE;
		    }
		}
		/* put it in RESERVED_2 */
		if (bad)
		{
		    bad = FALSE;
		    sprintf(str, " \n 3 Old group was %s. 0", msgBuf->mbgroup);
		    strcat(msgBuf->mbtext, str);
		    strcpy(msgBuf->mbgroup, grpBuf.group[1].groupname);
		}
	    }
    
	    /* Expired? */
	    if ( atol(msgBuf2->mbtime) 
		< (time(&l) - ((long)node.ndexpire *60*60*24)) ) 
	    {
		bad = TRUE;
		expired++;
	    }
    }

	/* If it's good, save it!  */
	if (!bad)
	    {
	    temproom = room;

	    if (strcmpi(msgBuf->mbroom, there) == SAMESTRING)
		strcpy(msgBuf->mbroom, here);

	    if (*msgBuf->mbto)
		temproom = NfindRoom(msgBuf->mbroom);

	    msgBuf->mbroomno = (uchar)temproom;

	    putMessage();
	    noteMessage();
	    addRoute (msgBuf->mbfpath) ;  /* For autorouting.  NETROUTE.C. */
	    goodmsg++;

	    if (*msgBuf->mbto)
	    {
		lp = thisRoom;
		thisRoom = temproom;
		/* notelogmessage(msgBuf->mbto); */
		thisRoom = lp;
	    }
	}
    }
    fclose(file);

    return goodmsg;
}

/* -------------------------------------------------------------------- */
/*  nodeSharesRoom() Does the ROUTE.CIT file say we share this room?    */
/* -------------------------------------------------------------------- */
BOOL nodeSharesRoom (char *node, int roomslot)
{
    int   hInFile ;
    char  buf[256], *words[20] ;
    BOOL  fPresent = FALSE ;

    sprintf (buf, "%s\\NODES.CIT", cfg.homepath) ;
    hInFile = sopen (buf, O_TEXT | O_RDONLY, SH_DENYNO) ;

    if (-1 == hInFile)
	crashout ("Problem opening ROUTE.CIT.") ;

    while ( !eof (hInFile) )
	{
	if (!sfgets (buf, 255, hInFile))
	    break ;                            /* Error reading file. */

	if ('#' != buf[0])
	    continue ;

	if (SAMESTRING != strnicmp (buf, "#NODE", 5))
	    continue ;

	parse_it (words, buf) ;

	if (SAMESTRING == stricmp (words[1], node))
	    break ;
	}

    while ( !eof (hInFile) )
	{
	if (!sfgets (buf, 255, hInFile))
	    break ;

	if ('#' != buf[0])
	    continue ;

	if (SAMESTRING != strnicmp (buf, "#ROOM", 5))
	    continue ;

	parse_it (words, buf) ;

	if (SAMESTRING == stricmp (words[1], roomTab[roomslot].rtname))
	    {
	    fPresent = TRUE ;
	    break ;
	    }
	}
    close (hInFile) ;
    return fPresent ;
}


/* -------------------------------------------------------------------- */
/*  nodecanseeroom() Can the node see this room?                        */
/* -------------------------------------------------------------------- */
BOOL nodecanseeroom(char *nodename, int roomslot)
{
  /* DOESN"T WORK YET.  */
    /* If the room shared bit is not set, but the NODES.CIT file says we   */
    /* share the room with the node, flip the SHARED bit.                  */

    if (FALSE == roomTab[roomslot].rtflags.SHARED)
	{
	roomTab[roomslot].rtflags.SHARED =
	    nodeSharesRoom (nodename, roomslot) ;
	}

	/* is room in use              */
    if ( roomTab[roomslot].rtflags.INUSE

	/* and it is shared            */
	&& roomTab[roomslot].rtflags.SHARED 

	/* and group can see this room */
	&& (groupseesroom(roomslot)
	|| roomTab[roomslot].rtflags.READONLY
	|| roomTab[roomslot].rtflags.DOWNONLY )       

	/* only aides go to aide room  */ 
	&&   ( roomslot != AIDEROOM || aide) )
    {
	return TRUE;
    }

    return FALSE;
}

#endif /* NETWORK */
