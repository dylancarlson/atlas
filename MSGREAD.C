/* -------------------------------------------------------------------- */
/*  MSGREAD.C                Dragon Citadel                             */
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
/*  printMessage()  prints message on modem and console                 */
/*  stepMessage()   find the next message in DIR                        */
/*  showMessages()  is routine to print roomful of msgs                 */
/*  printheader()   prints current message header                       */
/*  getMessage()    reads a message off disk into RAM.                  */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/*  getMsgChar()    reads a character from msg file, curent position    */
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
ulong   copyOf;


/* -------------------------------------------------------------------- */
/*  printMessage()  prints message on modem and console                 */
/* -------------------------------------------------------------------- */
#define msgstuff  msgTab1[slot].mtmsgflags  

void printMessage(ulong id, char verbose)
{
    ulong   here;
    long    loc;
    int     slot;
    static  level = 0;

    if ((slot = indexslot(id)) == ERROR) return;

    if (!level)
    {
	originalId   = id;
    }
	
    if (msgTab1[slot].mtmsgflags.COPY)
    {
	copyflag     = TRUE;
	originalattr = 0;

	if (level == 0)
	    copyOf = (ulong)(id - (ulong)msgTab8[slot].mtomesg);
	/*  copyOf = (ulong)(id - (ulong)msgTab3[slot].mtoffset); */
	
	originalattr = (uchar)
       (originalattr | (msgstuff.RECEIVED)?ATTR_RECEIVED :0 );

	originalattr = (uchar)
       (originalattr | (msgstuff.REPLY   )?ATTR_REPLY : 0 );

	originalattr = (uchar)
       (originalattr | (msgstuff.MADEVIS )?ATTR_MADEVIS : 0 );

	level ++;

	if (level > 20)
	{
	    level = 0;
	    return;
	}

#ifdef GOODBYE        
	if ((int)msgTab3[slot].mtoffset <= slot)
	    printMessage( (ulong)(id - (ulong)msgTab3[slot].mtoffset), verbose);
#endif
	if ((int)msgTab8[slot].mtomesg <= slot)
	    printMessage( (ulong)(id - (ulong)msgTab8[slot].mtomesg), verbose);

	level = 0;

	return;
    }

    /* in case it returns without clearing buffer */
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbto[   0]  = '\0';

    loc = msgTab2[slot].mtmsgLoc;
    if (loc == ERROR) return;

    if (copyflag)  slot = indexslot(originalId);

    if (!mayseeindexmsg(slot) )
    {
	return;
    }

    fseek(msgfl, loc, 0);

    getMessage();

    dotoMessage = NO_SPECIAL;

    sscanf(msgBuf->mbId, "%lu", &here);

    /* cludge to return on dummy msg #1 */
    if ((int)here == 1 || !mayseemsg())
    {
	return;
    }

    if (here != id )
    {
	mPrintf("Can't find message. Looking for %lu at byte %ld!\n ",
		 id, loc);
	return;
    }
    
    getMsgStr(msgBuf->mbtext, cfg.maxtext); 

    if (mf.mfSearch[0])
    {
	if (   !u_match(msgBuf->mbtext, mf.mfSearch) )
	    return;
    }
    
    mread++; /* Increment # messages read */
    
    printheader( id, verbose, slot);

    seen = TRUE;

    if (msgBuf->mblink[0])
    {
	dumpf(msgBuf->mblink);
    }
    else
    {
	/* NEW MESSAGE READ */
	mFormat(msgBuf->mbtext);
	if (outFlag == OUTNEXT)     /* If <N>ext, extra line */
	{
	    doCR();
	}
    }
    if ((msgBuf->mbsig[0] || msgBuf->mbusig[0]) && logBuf.SIGNATURES
      /* && cfg.nodeSignature[0] */)
    {
	termCap(TERM_BOLD);
	doCR(); mFormat("------");
	termCap(TERM_NORMAL);
	if (*msgBuf->mbusig)
	{
	    doCR();
	    mFormat(msgBuf->mbusig);
	}
	if (*msgBuf->mbsig)
	{
	    doCR();
	    mFormat(msgBuf->mbsig);
	}
    }
    termCap(TERM_NORMAL);
    doCR();
    echo = BOTH;
}

/* -------------------------------------------------------------------- */
/*  stepMessage()   find the next message in DIR                        */
/* -------------------------------------------------------------------- */
BOOL stepMessage(ulong *at, int dir)
{
    int i;

    for (i = indexslot(*at), i += dir; i > -1 && i < (int)sizetable(); i += dir)
    {
	/* skip messages not in this room */
	if (msgTab4[i].mtroomno != (uchar)thisRoom) continue;

	/* skip by special flag */
	if (mf.mfMai && !msgTab1[i].mtmsgflags.MAIL) continue;
	if (mf.mfLim && !msgTab1[i].mtmsgflags.LIMITED) continue;
	if (mf.mfPub && 
	   (msgTab1[i].mtmsgflags.LIMITED || msgTab1[i].mtmsgflags.MAIL ))
	   continue;

	if (mayseeindexmsg(i))
	{
	    *at = (ulong)(cfg.mtoldest + i);
	    return TRUE;
	}
    }
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  showMessages()  is routine to print roomful of msgs                 */
/* -------------------------------------------------------------------- */
void showMessages(char whichMess, char revOrder, char verbose)
{
    int   increment, i ;
    ulong lowLim, highLim, msgNo, start;
    unsigned char attr;
    BOOL  done;
    char  save[64];

    if (mf.mfLim)
    {
	getgroup();
	if (!mf.mfLim)
	    return;
    }
    else 
    {
      doCR();
    }

    if (mf.mfUser[0])
	getNormStr("user", mf.mfUser, NAMESIZE, ECHO);
    
    if (mf.mfSearch[0])
    {
	getNormStr("search text", mf.mfSearch, NAMESIZE-2, ECHO);
	if (mf.mfSearch[0] != '*')
	{
	    sprintf(save, "*%s", mf.mfSearch);
	    strcpy(mf.mfSearch, save);
	}
	if (mf.mfSearch[strlen(mf.mfSearch)-1] != '*')
	{
	    strcat(mf.mfSearch, "*");
	}
	mPrintf("Searching for %s.", mf.mfSearch); doCR();
    }

    outFlag = OUTOK;

    if (!expert )  mPrintf("\n <J>ump <N>ext <P>ause <S>top");

    switch (whichMess)  
    {
    case NEWoNLY:
	lowLim  = logBuf.lbvisit[ logBuf.lbroom[thisRoom].lvisit ] + 1;
	highLim = cfg.newest;

	/* print out last new message */
	if (!revOrder && oldToo && (highLim >= lowLim))
	    stepMessage(&lowLim, -1);
	break;

    case OLDaNDnEW:
	lowLim  = cfg.oldest;
	highLim = cfg.newest;
	break;

    case OLDoNLY:
	lowLim  = cfg.oldest;
	highLim = logBuf.lbvisit[ logBuf.lbroom[thisRoom].lvisit  ];
	break;
    }

    /* stuff may have scrolled off system unseen, so: */
    if (cfg.oldest  > lowLim)  lowLim = cfg.oldest;

    /* Allow for reverse retrieval: */
    if (!revOrder)
    {
	start       = lowLim;
	increment   = 1;
    }else{
	start       = highLim;
	increment   = -1;
    }

    start -= (long)increment;
    done = (BOOL)(!stepMessage(&start, increment));

    for (msgNo = start;
	 !done 
	 && msgNo >= lowLim 
	 && msgNo <= highLim 
	 && (CARRIER);
	 done = (BOOL)(!stepMessage(&msgNo, increment)) )
    {
	if (BBSCharReady()) mAbort(FALSE);

	if (outFlag != OUTOK)
	{
	    if (outFlag == OUTNEXT || outFlag == OUTPARAGRAPH)
	    {
		outFlag = OUTOK;
	    }
	    else if (outFlag == OUTSKIP)  
	    {
		echo = BOTH;
		memset(&mf, 0, sizeof(mf));
		return;
	    }
	}

	copyflag = FALSE;
	seen = FALSE;

	printMessage( msgNo, verbose );

	if (outFlag != OUTSKIP)
	{
	    switch(dotoMessage)
	    {
		case COPY_IT:
		    if (!sysop || !onConsole) break;
		    getNormStr("save path", save, 64, ECHO);
		    if (*save)
		    {
			/*copyMessage(i, save); save message to file */
		    }
		    break;
    
		case PULL_IT:
		    /* Pull current message from room if flag set */
		    pullIt();
		    outFlag = OUTOK;
		    break;
    
		case MARK_IT:
		    /* Mark current message from room if flag set */
		    markIt();
		    outFlag = OUTOK;
		    break;

#ifdef GOODBYE    
		case REVERSE_READ:
		    increment = (increment == 1) ? -1 : 1;
		    doCR();
		    mPrintf("3<Reversed>0");
		    doCR();
		    break;
#endif
		case REVERSE_READ:
		    increment = -increment;
		    doCR();
		    mPrintf("3<Reversed %c>0", (increment == 1) ? '+' : '-');
		    lowLim = cfg.oldest;
		    highLim= cfg.newest;     /* reevaluate for Livia */
		    doCR();
		    break;
    
		case NO_SPECIAL:

		    /* Release (Y/N)[N] */

		    if (outFlag == OUTNEXT)
			break;
		    
		    if (   *msgBuf->mbx 
			&& CAN_MODERATE() 
			&& seen 
			&& ( msgBuf->mbattr & ATTR_MADEVIS) != ATTR_MADEVIS
			&& CANOUTPUT() 
		       )
			if (getYesNo("Release", 0))
			    {
			    markmsg();
			    outFlag = OUTOK;
			    }
    
		    /* Ask about replying to mail if (1) it's to you, or   */
		    /* (2) it's forwarded to you, or (3) it's to "Sysop"   */
		    /* at this node, and you're a sysop, (4) it's to Aide  */
		    /* at this node, and you're an aide, all ONLY IF it    */
		    /* has NOT already been replied to.			   */
		    /* When checking to see if it is to this node, accept  */
		    /* both no target node, and our BBS name as target.    */

		    i = indexslot(msgNo) ;

		    if ( whichMess == NEWoNLY

		      && !msgTab1[i].mtmsgflags.REPLY

		      && (   (strcmpi(msgBuf->mbto,  logBuf.lbname) == SAMESTRING)

			  || (strcmpi(msgBuf->mbfwd, logBuf.lbname) == SAMESTRING)

			  || (	 (strcmpi(msgBuf->mbto, "Sysop") == SAMESTRING)

			      && (    (! *msgBuf->mbzip)
				   || (stricmp(msgBuf->mbzip, cfg.nodeTitle) == SAMESTRING) )

			      && sysop )

			  || (	 (strcmpi(msgBuf->mbto, "Aide") == SAMESTRING)

			      && (    (! *msgBuf->mbzip)
				   || (stricmp(msgBuf->mbzip, cfg.nodeTitle) == SAMESTRING) )

			      && aide )
			 )
		       )
			{
		       outFlag = OUTOK;
		       doCR();


		       if (getYesNo("Respond", 1)) 
		       {
			   replyFlag = 1;
			   mailFlag  = 1;
			   linkMess  = FALSE;
    
			   if (!copyflag)  attr = msgBuf->mbattr;
			   else            attr = originalattr;
    
			   if (whichIO != CONSOLE)  echo = CALLER;
    
			   if  (makeMessage()) 
			   {
			       attr = (uchar)(attr | ATTR_REPLY);
    
			       if (!copyflag)  msgBuf->mbattr = attr;
			       else            originalattr  = attr;
    
			       if (!copyflag)  changeheader(msgNo,      255, attr);
			       else            changeheader(originalId, 255, attr);
			   }
    
			   replyFlag = 0;
			   mailFlag  = 0;
    
			   /* Restore privacy zapped by make... */
			   if (whichIO != CONSOLE)  echo = BOTH;
    
			   outFlag = OUTOK;
    
			   if (cfg.oldest  > lowLim)
			   {
			       lowLim = cfg.oldest;
			       if (msgNo < lowLim) msgNo = lowLim;
			   }
		       }
		    }
		    break;
		    
		default:
		    break;
	    }
	}

	copyflag     = FALSE;
	originalId   = 0;
	originalattr = 0;
    }
    echo = BOTH;
    memset(&mf, 0, sizeof(mf));
}
/* -------------------------------------------------------------------- */
/*  getMessage()    reads a message off disk into RAM.                  */
/* -------------------------------------------------------------------- */
BOOL getMessage(void)
{
    char c;
    int i;

    /* clear message buffer out */
    clearmsgbuf();

    /* look 10000 chars for start message */
    for (i = 0; i < 10000; ++i)
    {
	c = (uchar)getMsgChar();
	if (c == -1) break;
    }
    if (i == 10000) return(FALSE);

#ifdef GOODBYE
    /* find start of message */
    do
    {
	c = (uchar)getMsgChar();
    } while (c != -1);
#endif

    /* record exact position of start of message */
    msgBuf->mbheadLoc  = (long)(ftell(msgfl) - (long)1);

    /* get message's room #         */
    msgBuf->mbroomno   = (uchar)getMsgChar();

    /* get message's attribute byte */
    msgBuf->mbattr     = (uchar)getMsgChar();

    getMsgStr(msgBuf->mbId, LABELSIZE);
    
    do 
    {
	c = (char)getMsgChar();
	
	switch (c)
	{
	case 'A':     getMsgStr(msgBuf->mbauth,  LABELSIZE);    break;
	case 'B':     getMsgStr(msgBuf->mbsub,   80       );    break;
	case 'C':     getMsgStr(msgBuf->mbcopy,  LABELSIZE);    break;
	case 'D':     getMsgStr(msgBuf->mbtime,  LABELSIZE);    break;
	case 'F':     getMsgStr(msgBuf->mbfwd,   LABELSIZE);    break;
	case 'G':     getMsgStr(msgBuf->mbgroup, LABELSIZE);    break;
	case 'I':     getMsgStr(msgBuf->mbreply, LABELSIZE);    break;
	case 'J':     getMsgStr(msgBuf->mbcreg,  LABELSIZE);    break;
	case 'j':     getMsgStr(msgBuf->mbccont, LABELSIZE);    break;
	case 'L':     getMsgStr(msgBuf->mblink,  63);           break;
	case 'M':     /* will be read off disk later */         break;
	case 'N':     getMsgStr(msgBuf->mbtitle, LABELSIZE);    break;
	case 'n':     getMsgStr(msgBuf->mbsur,   LABELSIZE);    break;
	case 'O':     getMsgStr(msgBuf->mboname, LABELSIZE);    break;
	case 'o':     getMsgStr(msgBuf->mboreg,  LABELSIZE);    break;
	case 'P':     getMsgStr(msgBuf->mbfpath, 128     );     break;
	case 'p':     getMsgStr(msgBuf->mbtpath, 128     );     break;
	case 'Q':     getMsgStr(msgBuf->mbocont, LABELSIZE);    break;
	case 'q':     getMsgStr(msgBuf->mbczip,  LABELSIZE);    break;
	case 'R':     getMsgStr(msgBuf->mbroom,  LABELSIZE);    break;
	case 'S':     getMsgStr(msgBuf->mbsrcId, LABELSIZE);    break;
	case 's':     getMsgStr(msgBuf->mbsoft,  LABELSIZE);    break;
	case 'T':     getMsgStr(msgBuf->mbto,    LABELSIZE);    break;
	case 'X':     getMsgStr(msgBuf->mbx,     LABELSIZE);    break;
	case 'Z':     getMsgStr(msgBuf->mbzip,   LABELSIZE);    break;
	case 'z':     getMsgStr(msgBuf->mbrzip,  LABELSIZE);    break;
	case '.':     getMsgStr(msgBuf->mbsig,   90       );    break;
	case '_':     getMsgStr(msgBuf->mbusig,  90       );    break;

	default:
	    getMsgStr(msgBuf->mbtext, cfg.maxtext); /* discard unknown field  */
	    msgBuf->mbtext[0]    = '\0';
	    break;
	}
    } while (c != 'M');

    return(TRUE);
}

/* -------------------------------------------------------------------- */
/*  getMsgStr()     reads a NULL terminated string from msg file        */
/* -------------------------------------------------------------------- */
void getMsgStr(char *dest, int lim)
{
    char c;

    while ((c = (char)getMsgChar()) != 0)    /* read the complete string     */
    {
	if (lim)                        /* if we have room then         */
	{
	    lim--;
	    *dest++ = c;                /* copy char to buffer          */
	}
    }
    *dest = '\0';                       /* tie string off with null     */
}

/* -------------------------------------------------------------------- */
/*  getMsgChar()    reads a character from msg file, curent position    */
/* -------------------------------------------------------------------- */
int getMsgChar(void)
{
    int c;

    c = fgetc(msgfl);

    if (c == ERROR)
    {
	/* check for EOF */
	if (feof(msgfl))
	{
	    clearerr(msgfl);
	    fseek(msgfl, 0l, SEEK_SET);
	    c = fgetc(msgfl);
	}
    }
    return c;
}

/* -------------------------------------------------------------------- */
/*  printheader()   prints current message header                       */
/* -------------------------------------------------------------------- */
void printheader(ulong id, char verbose, int slot)
{
    char dtstr[80];
    uchar attr;
    long timestamp;

    if (outFlag == OUTNEXT) outFlag = OUTOK;

    if (*msgBuf->mbtime)
    {
	sscanf(msgBuf->mbtime, "%ld", &timestamp);
	strftime(dtstr, 79, 
		 verbose ? cfg.vdatestamp : cfg.datestamp, timestamp);
    }


    if (verbose && (strcmpi(msgBuf->mbauth, "****") != SAMESTRING || sysop)) 
    {
	doCR();
	termCap(TERM_BOLD);
	mPrintf("    # %lu of %lu", originalId, cfg.newest);
	if (copyflag && aide)
	    mPrintf(" (Duplicate id # %lu, %s)", copyOf, msgBuf->mbId);
	if (*msgBuf->mbsrcId) 
	{
	    doCR();
	    mPrintf("    Source id # is:  %s", msgBuf->mbsrcId);
	}              
	if (*msgBuf->mblink && sysop) 
	{
	    doCR();
	    mPrintf("    Linked file is:  %s", msgBuf->mblink);
	}
	if (*msgBuf->mbfpath)
	{
	    doCR();
	    mPrintf("    Path followed:   %s!%s", msgBuf->mbfpath, cfg.nodeTitle);
	}
	if (*msgBuf->mbsoft)
	{
	    doCR();
	    mPrintf("    Source software: %s", msgBuf->mbsoft);
	}
	if (*msgBuf->mbtpath)
	{
	    doCR();
	    mPrintf("    Forced path:     %s", msgBuf->mbtpath);
	}
    }
    
    doCR();
    termCap(TERM_BOLD);
    mPrintf("    %s", dtstr);
    
    if (msgBuf->mbauth[ 0])
    {
	mPrintf(" From ");
	
	if (!roomBuf.rbflags.ANON 
	  && strcmpi(msgBuf->mbauth, "****") != SAMESTRING)
	{
	    if (msgBuf->mbtitle[0] && logBuf.DISPLAYTS
	       && (
		    (cfg.titles && !(msgBuf->mboname[0])) 
		    || cfg.nettitles
		  )
	       )
	    {
		 mPrintf( "[%s] ", msgBuf->mbtitle);
	    }
	    
	    
	    termCap(TERM_UNDERLINE);
	    mPrintf("%s", msgBuf->mbauth);
	    termCap(TERM_NORMAL);
	    termCap(TERM_BOLD);
	    
	    if (msgBuf->mbsur[0] && logBuf.DISPLAYTS 
	       && (
		    (cfg.surnames && !(msgBuf->mboname[0])) 
		    || cfg.netsurname
		  )
	       )
	    {
		 mPrintf( " [%s]", msgBuf->mbsur);
	    }
	}
	else
	{
	    /* mPrintf("****"); */
	    mPrintf("4%s03", cfg.anonauthor);
	    
	    if (sysop && strcmpi(msgBuf->mbauth, "****") != SAMESTRING)
	    {
		mPrintf(" (%s)", msgBuf->mbauth);
	    }
	}
    }
    
    termCap(TERM_BOLD);

    if (msgBuf->mboname[0]
	&& (strcmpi(msgBuf->mboname, cfg.nodeTitle) != SAMESTRING
	  || strcmpi(msgBuf->mboreg, cfg.nodeRegion) != SAMESTRING)
	    && strcmpi(msgBuf->mbauth, msgBuf->mboname) != SAMESTRING)
	     mPrintf(" @ %s", msgBuf->mboname);


    if (msgBuf->mboreg[0] &&
	strcmpi(msgBuf->mboreg, cfg.nodeRegion) != SAMESTRING)
	{
	   mPrintf(", %s", msgBuf->mboreg);

	   if (verbose && *msgBuf->mbcreg)
	       mPrintf(" {%s}", msgBuf->mbcreg);
	}
    
    if (msgBuf->mbocont[0] && verbose &&
	strcmpi(msgBuf->mbocont, cfg.nodeContry) != SAMESTRING)
	{
	   mPrintf(", %s", msgBuf->mbocont);

	   if (verbose && *msgBuf->mbccont)
	       mPrintf(" {%s}", msgBuf->mbccont);
	}

    if (msgBuf->mbto[0])
    {
	mPrintf(" To %s", msgBuf->mbto);

	if (msgBuf->mbzip[0]
	      && strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING)
		 mPrintf(" @ %s", msgBuf->mbzip);

	if (msgBuf->mbrzip[0] &&
	    strcmpi(msgBuf->mbrzip, cfg.nodeRegion))
	      mPrintf(", %s", msgBuf->mbrzip);

	if (msgBuf->mbfwd[0])
	    mPrintf(" Forwarded to %s", msgBuf->mbfwd );

	if (msgBuf->mbreply[0])
	{
	    if (verbose)
		mPrintf(" [reply to %s]", msgBuf->mbreply);
	    else
		mPrintf(" [reply]");
	}
	if ( msgstuff.RECEIVED)  mPrintf(" [received]");
	if ( msgstuff.REPLY)     mPrintf(" [reply sent]");

	if ( (msgBuf->mbto[0])
	   && !(strcmpi(msgBuf->mbauth, logBuf.lbname) == SAMESTRING ))
	{

	    if (!copyflag)  attr = msgBuf->mbattr;
	    else            attr = originalattr;

	    if (!(attr & ATTR_RECEIVED))
	    {
		attr = (uchar)(attr | ATTR_RECEIVED);

		if (!copyflag)  msgBuf->mbattr = attr;
		else            originalattr  = attr;

		if (!copyflag)  changeheader(id,         255, attr);
		else            changeheader(originalId, 255, attr);

	    }
	}
    }

    if (strcmpi(msgBuf->mbroom, roomBuf.rbname) != SAMESTRING)
    {
	mPrintf(" In %s>",  msgBuf->mbroom );
    }

    if (msgBuf->mbgroup[0])
    {
	mPrintf(" (%s only)", msgBuf->mbgroup);
    }

    /* Aides can't release moderated messages, but they can see them.  So  */
    /* ask if the user is an aide, or if the user CAN_MODERATE.            */

    if ( (aide || CAN_MODERATE()) && msgBuf->mbx[0] )
	{
	if (!msgstuff.MADEVIS)
	    {
	    if (msgBuf->mbx[0] == 'Y')
		mPrintf(" 1[problem user]03") ;
	    else
		mPrintf(" [moderated]") ;
	    }
	else
	    mPrintf(" [viewable %s]", msgBuf->mbx[0] == 'Y' ?
			 "problem user" : "moderated" ) ;
	}

    if ((aide || sysop) && msgBuf->mblink[0])
	mPrintf(" [file-linked]");

    doCR();

    if (msgBuf->mbsub[0] && logBuf.SUBJECTS)
    {
	mPrintf("    Subject: 4%s0", msgBuf->mbsub);
	doCR();
    }
    
    termCap(TERM_NORMAL);
    
    if (*msgBuf->mbto && whichIO != CONSOLE) echo = CALLER;
}
