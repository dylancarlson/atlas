/* -------------------------------------------------------------------- */
/*  MSG.C                    Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*               This is the high level message code.                   */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  clearmsgbuf()   this clears the message buffer out                  */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/*  indexslot()     give it a message # and it returns a slot#          */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/*  sizetable()     returns # messages in table                         */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  06/02/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code.                                      */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  clearmsgbuf()   this clears the message buffer out                  */
/* -------------------------------------------------------------------- */
void clearmsgbuf(void)
{
    /* clear msgBuf out */
    msgBuf->mbroomno    =   0 ;
    msgBuf->mbattr      =   0 ;
    msgBuf->mbauth[ 0]  = '\0';
    msgBuf->mbtitle[0]  = '\0';
    msgBuf->mbocont[0]  = '\0';
    msgBuf->mbfpath[0]  = '\0';
    msgBuf->mbtpath[0]  = '\0';
    msgBuf->mbczip[ 0]  = '\0';
    msgBuf->mbcopy[ 0]  = '\0';
    msgBuf->mbfwd[  0]  = '\0';
    msgBuf->mbgroup[0]  = '\0';
    msgBuf->mbtime[ 0]  = '\0';
    msgBuf->mbId[   0]  = '\0';
    msgBuf->mbsrcId[0]  = '\0';
    msgBuf->mboname[0]  = '\0';
    msgBuf->mboreg[ 0]  = '\0';
    msgBuf->mbreply[0]  = '\0';
    msgBuf->mbroom[ 0]  = '\0';
    msgBuf->mbto[   0]  = '\0';
    msgBuf->mbsur[  0]  = '\0';
    msgBuf->mblink[ 0]  = '\0';
    msgBuf->mbx[    0]  = '\0';
    msgBuf->mbzip[  0]  = '\0';
    msgBuf->mbrzip[ 0]  = '\0';
    msgBuf->mbusig[ 0]  = '\0';
    msgBuf->mbsub[  0]  = '\0';
    msgBuf->mbsig[  0]  = '\0';
    msgBuf->mbsoft[ 0]  = '\0';
    msgBuf->mbcreg[ 0]  = '\0';
    msgBuf->mbccont[0]  = '\0';
}

/* -------------------------------------------------------------------- */
/*  mAbort()        returns TRUE if the user has aborted typeout        */
/* -------------------------------------------------------------------- */
BOOL mAbort(BOOL pause)
{
    char c;
    char toReturn = FALSE;
    char oldEcho;
    int  i;
    BOOL more = FALSE;
    
    /*
     * Can not abort IMPERVIOUS
     */
    if (outFlag == IMPERVIOUS)
    {
	return FALSE;
    }
    
    /*
     * Carrier loss and not on Console
     */
    if (!CARRIER)
    {
	outFlag = OUTSKIP;
	return TRUE;
    }

    /*
     *  Check for keypress..
     */
    if (BBSCharReady() || pause)
    {
	oldEcho  = echo;
    
	if (pause)
	{
	    c = 'P';
	    more = TRUE;
	}
	else
	{
	    echo = NEITHER;
	    c = (char)toupper(iChar());
	}

	if (c == 'P' || c == 19 /* XOFF */)       /* Pause! */
	{
	    echo = oldEcho;
	    setio(whichIO, echo, outFlag);
	    
	    if (more)
		putWord((uchar *)"<More>");
	   
	    echo = NEITHER;
	    
	    c = (char)toupper(iChar());             /* wait to resume */
	    
	    if (more)
		for(i=0; i<6; i++)
		    doBS();
	}
	
	echo = oldEcho;
	
	setio(whichIO, echo, outFlag);
	
	if (outFlag == NOSTOP)  
	    return FALSE;
	
	switch (c)
	{
	case 'C':
	    dotoMessage = COPY_IT;
	    toReturn    = FALSE;
	    break;
	    
	case 'J':                            /* jump paragraph:*/
	    doCR();
	    termCap(TERM_BOLD);
	    putWord((uchar *)"<Jump>");
	    termCap(TERM_NORMAL);
	    doCR();
	    outFlag     = OUTPARAGRAPH;
	    toReturn    = FALSE;
	    break;
	    
	case 'K':                            /* kill:          */
	    if (    CAN_MODERATE()
		 || (cfg.kill && (strcmpi(logBuf.lbname, 
					  msgBuf->mbauth) == SAMESTRING))
	       )
	    {   
		dotoMessage = PULL_IT;
		
		doCR();
		termCap(TERM_BOLD);
		putWord((uchar *)"<Kill>");
		termCap(TERM_NORMAL);
		outFlag     = OUTNEXT;
		toReturn    = TRUE;
	    }
	    else
	    {
		toReturn               = FALSE;
	    }
	    break;
	    
	case 'M':                            /* mark:          */
	    if (aide)  dotoMessage = MARK_IT;
	    toReturn               = FALSE;
	    break;
	    
	case 'N':                            /* next:          */
	    doCR();
	    termCap(TERM_BOLD);
	    putWord((uchar *)"<Next>");
	    termCap(TERM_NORMAL);
	    doCR();
	    outFlag     = OUTNEXT;
	    toReturn    = TRUE;
	    break;
	    
	case 'S':                            /* skip:          */
	    doCR();
	    termCap(TERM_BOLD);
	    putWord((uchar *)"<Stop>");
	    termCap(TERM_NORMAL);
	    doCR();
	    outFlag     = OUTSKIP;
	    toReturn    = TRUE;
	    break;
	
	case 'R':
	    dotoMessage = REVERSE_READ;
	    toReturn    = FALSE;
	    break;
	
	default:
	    toReturn    = FALSE;
	    break;
	}
    }
    
    return toReturn;
}


/* -------------------------------------------------------------------- */
/*  indexslot()     give it a message # and it returns a slot#          */
/* -------------------------------------------------------------------- */
int indexslot(ulong msgno)
{ 
    if (msgno < cfg.mtoldest)
    {
	return(ERROR);
    }

    return((int)(msgno - cfg.mtoldest));
}

/* -------------------------------------------------------------------- */
/*  mayseemsg()     returns TRUE if person can see message. 100%        */
/* -------------------------------------------------------------------- */
BOOL mayseemsg(void)
{
    int i;
    uchar attr;

    if (!copyflag) attr = msgBuf->mbattr;
    else           attr = originalattr;


    /* mfUser From Acit */
    if ( mf.mfUser[0] )
    {
	stripansi(mf.mfUser);

	if (   !u_match(deansi(msgBuf->mbto),   mf.mfUser)
	    && !u_match(deansi(msgBuf->mbauth), mf.mfUser)
	    && !u_match(deansi(msgBuf->mbfwd),  mf.mfUser)
	    && !u_match(deansi(msgBuf->mboname),mf.mfUser)
	    && !u_match(deansi(msgBuf->mboreg), mf.mfUser)
	    && !u_match(deansi(msgBuf->mbocont),mf.mfUser)
	    && !u_match(deansi(msgBuf->mbzip),  mf.mfUser)
	    && !u_match(deansi(msgBuf->mbrzip), mf.mfUser)
	    && !u_match(deansi(msgBuf->mbczip), mf.mfUser)
	       ) return (FALSE);
    }


#ifdef GOODBYE
    /* mfUser */
    if ( mf.mfUser[0] )
    {
	if (!u_match(msgBuf->mbto, mf.mfUser)
	    && !u_match(msgBuf->mbauth, mf.mfUser) )
	    return (FALSE);
    }
#endif

    /* check for PUBLIC non problem user messages first */
    if ( !msgBuf->mbto[0] && !msgBuf->mbx[0] && !msgBuf->mbgroup[0])
	return(TRUE);

    if (msgBuf->mbx[0])
    {
	if ( strcmpi(msgBuf->mbauth,  logBuf.lbname) == SAMESTRING )
	{
	    /* problem users cant see copys of their own messages */
	    if (copyflag) return(FALSE);
	}
	else
	{
	    /* but everyone else cant see the orignal if it has been released */
	    if (   !copyflag 
		&& ((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
	       ) 
		return(FALSE);
	    
	    /* problem user message... */    
	    if (   msgBuf->mbx[0] == 'Y'
		&& !/* CAN_MODERATE() */ (sysop || aide)
		&& !((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
	       )
	    {   
		return(FALSE);
	    }
	    
	    /* moderated message... */    
	    if (   msgBuf->mbx[0] == 'M' 
		&& !((attr & ATTR_MADEVIS) == ATTR_MADEVIS)
	       )
	    {   
		if ( !(   sysop 
		       || (aide && !cfg.moderate) 
		       /* || (roomTab[thisRoom].rtflags.GRP_MOD&&pgroupseesroom()) */
		      ) 
		   )
		{   
		    return(FALSE);
		}
	    }
	}
    }


    if ( msgBuf->mbto[0] )
	{
	/* 1-23-92 BUG: Related to the long comment below, it is also      */
	/* necessary to shortcircuit this test of msgBufmbsrcId when the   */
	/* darn message doesn't even have one.  If there IS an             */
	/* msgBufmbsrcId, and if it doesn't match the local ID, don't show */
	/* the message.  Otherwise, go ahead and show it, cuz it's local.  */

	/* author can see his own private messages */
	/* but ONLY on local system. */

	/* If the names match, */

	if ( (strcmpi(msgBuf->mbauth,  logBuf.lbname) == SAMESTRING)

	   /* and if there's no source id OR if the id's match, */

	   && ( ('\0' == msgBuf->mbsrcId[0])
		||  (strcmpi(msgBuf->mbsrcId, msgBuf->mbId) == SAMESTRING)
	      )
	   )
	    return(TRUE);

	/* I bet the removal of this line caused this bug:  */
	/* !*msgBuf->mbsrcId */ /* cant see if it has been netted */


	/* recipient can see private messages      */
	if (strcmpi(msgBuf->mbto, logBuf.lbname) == SAMESTRING)
	return(TRUE);          

	/* forwardee can see private messages      */
	if (strcmpi(msgBuf->mbfwd, logBuf.lbname) == SAMESTRING)
	return(TRUE);
	    
	/* sysops see messages to 'Sysop'           */
	if ( sysop && ( strcmpi(msgBuf->mbto, "Sysop") == SAMESTRING) )
	return(TRUE);

	/* aides see messages to 'Aide'           */
	if ( aide && ( strcmpi(msgBuf->mbto, "Aide") == SAMESTRING) )
	return(TRUE);

	/* none of those so cannot see message     */
	return(FALSE);
    }

    if ( msgBuf->mbgroup[0] )
    {
	for (i = 0 ; i < MAXGROUPS; ++i)
	{
	    /* check to see which group message is to */
	    if (strcmpi(grpBuf.group[i].groupname, msgBuf->mbgroup) == SAMESTRING)
	    {
		/* if in that group */
		if (logBuf.groups[i] == grpBuf.group[i].groupgen )
		{
		    return(TRUE);
		}
		else
		{
		    return(FALSE);
		}
	    }
	} /* group can't see message, return false */
	
	if (sysop)
	    mPrintf("    Unknown group %s!", msgBuf->mbgroup); doCR();
	return(sysop);
    }

    return(TRUE);
}

/* -------------------------------------------------------------------- */
/*  mayseeindexmsg() Can see message by slot #. 99%                     */
/* -------------------------------------------------------------------- */
BOOL mayseeindexmsg(int slot)
{
    int i;
    int oslot;
    char copy = 0;

    oslot = slot;

/*  if (msgTab3[slot].mtoffset > (unsigned short)slot)  return(FALSE); */

    /* seek out original message table entry */
    while (msgTab1[slot].mtmsgflags.COPY)
    {
       copy = TRUE;
       /* copy has scrolled */
       if (msgTab8[slot].mtomesg  > (unsigned int)slot)
	   return(FALSE);

       else  /* look at original message index */
	   slot = slot - (int)msgTab8[slot].mtomesg;
    }

    /* check for PUBLIC non problem user messages first */
    if (    !msgTab5[slot].mttohash 
	 && !msgTab1[slot].mtmsgflags.PROBLEM 
	 && !msgTab1[slot].mtmsgflags.LIMITED)
    {
	return(TRUE);
    }

    
    if (msgTab1[slot].mtmsgflags.PROBLEM)
    {
	if (copy)
	{

	    /* problem users can not see copys of their messages */
	    if (msgTab6[slot].mtauthhash == (int)hash(logBuf.lbname))
		{
		return FALSE;
		}
	}
	else
	{
	    /* If you not an aide or a sysop, and if the message has not   */
	    /* been made visible, and if, apparently, you're not the       */
	    /* author, or something like this, then you can't see it.      */

	    if (
		  !(
			/* if you are a aide/sop and it is not MADEVIS */
			(aide || sysop) 
		     || msgTab1[oslot].mtmsgflags.MADEVIS
		   )
		&& msgTab6[slot].mtauthhash != (int)hash(logBuf.lbname)
	       )
		{
		return FALSE;
		}
	}
    }   

    if (msgTab1[slot].mtmsgflags.MAIL)
    {


/*****************************/
#ifdef GOODBYE

	/* author can see his own private messages */
	if (msgTab6[slot].mtauthhash == (int)hash(logBuf.lbname)
	    && (msgTab9[slot].mtorigin == 0 ||
	    msgTab9[slot].mtorigin == (int)hash(cfg.nodeTitle))
	     )
	{
	      return(TRUE);
	}

#endif

	/* author can see his own private messages */
	/* author can see his own net private message ONLY on local system */

	if (msgTab6[slot].mtauthhash == (int)hash(logBuf.lbname)

	    /* if the source id is equal to the local id then message */
	    /* is thought to be not netted */
	    /* It would be possible however by vast cooincidence that a */
	    /* message netted out would have the same local and src id */
	    /* fucking unlikely */
	    /* it still won't get through mayseemsg tho. */

	    && (msgTab8[slot].mtomesg == (unsigned int)(cfg.mtoldest + slot)) 
	    
	     )
	{
	      return(TRUE);
	}

/*****************************/


	/* recipient can see private messages      */
	if (msgTab5[slot].mttohash == (int)hash(logBuf.lbname)
	    && !msgTab1[slot].mtmsgflags.NET)   return(TRUE);

	/* forwardee can see private messages      */
	if (msgTab7[slot].mtfwdhash == (int)hash(logBuf.lbname))  return(TRUE);
	    
	/* sysops see messages to 'Sysop'           */
	if ( sysop && (msgTab5[slot].mttohash == (int)hash("Sysop")) )
	return(TRUE);

	/* aides see messages to 'Aide'           */
	if ( aide && (msgTab5[slot].mttohash == (int)hash("Aide")) )
	return(TRUE);

	/* none of those so cannot see message     */
	return(FALSE);
    }

    if (msgTab1[slot].mtmsgflags.LIMITED)
    {
	for (i = 0 ; i < MAXGROUPS; ++i)
	{
	    /* check to see which group message is to */
	    if ((int)hash(grpBuf.group[i].groupname) == msgTab5[slot].mttohash)
	    {
		/* if in that group */
		if (logBuf.groups[i] == grpBuf.group[i].groupgen )
		{
		    return(TRUE);
		}
		else
		{
		    return(FALSE);
		}
	    }
	} /* group can't see message, return false */
	return(sysop);
    }
    return(TRUE);
}

/* -------------------------------------------------------------------- */
/*  sizetable()     returns # messages in table                         */
/* -------------------------------------------------------------------- */
uint sizetable(void)
{
    return (int)((cfg.newest - cfg.mtoldest) + 1);
}


/* -------------------------------------------------------------------- */
/*  indexmessage()  builds one message index from msgBuf                */
/* -------------------------------------------------------------------- */
void indexmessage(ulong here)
{
    ushort slot;
    ulong copy;
    ulong oid;
    
    slot = indexslot(here);

    msgTab2[slot].mtmsgLoc            = (long)0;

    msgTab1[slot].mtmsgflags.MAIL     = 0;
    msgTab1[slot].mtmsgflags.RECEIVED = 0;
    msgTab1[slot].mtmsgflags.REPLY    = 0;
    msgTab1[slot].mtmsgflags.PROBLEM  = 0;
    msgTab1[slot].mtmsgflags.MADEVIS  = 0;
    msgTab1[slot].mtmsgflags.LIMITED  = 0;
    msgTab1[slot].mtmsgflags.MODERATED= 0;
    msgTab1[slot].mtmsgflags.RELEASED = 0;
    msgTab1[slot].mtmsgflags.COPY     = 0;
    msgTab1[slot].mtmsgflags.NET      = 0;

    msgTab6[slot].mtauthhash  = 0;
    msgTab5[slot].mttohash    = 0;
    msgTab7[slot].mtfwdhash   = 0;
/*  msgTab3[slot].mtoffset    = 0; */
/*  msgTab9[slot].mtorigin    = 0; */
    msgTab8[slot].mtomesg     = /* (long) */ 0;

    msgTab4[slot].mtroomno    = DUMP;

    /* --- */
    
    msgTab2[slot].mtmsgLoc    = msgBuf->mbheadLoc;

    /* 1-23-92 BUG: You can't see local private e-mail you send.  This is  */
    /* because mayseeindexmessage asks, "is msgTab8[slot].mtomesg the same */
    /* value as the local message number of this message?"  But  for       */
    /* local messages, no msgTab8[slot].mtomesg is assigned-- this value   */
    /* is read from a field on the disk.  For remote nodes, it's the       */
    /* original message number.  I could add it locally so the message     */
    /* number of a local message is stored not only at the very beginning, */
    /* but also in a field.  But HOW DUMB that is.  Instead, if            */
    /* msgBuf->mbsrcId has nothing in it (field wasn't in header), set     */
    /* msgTab8[slot].mtomesg to here (a variable), which equals the        */
    /* message number.  This may go insane if remote nodes stop sending    */
    /* an origin #.  If so, check on networking if no origin # is present, */
    /* and make one up if not present.                                     */
    
    if (*msgBuf->mbsrcId)
	{
	sscanf(msgBuf->mbsrcId, "%ld", &oid);
	msgTab8[slot].mtomesg = (unsigned int)oid;
	}
    else
	msgTab8[slot].mtomesg = (uint) here ;


    if (*msgBuf->mbauth)  msgTab6[slot].mtauthhash =  hash(msgBuf->mbauth);

    if (*msgBuf->mbto)
    {
	msgTab5[slot].mttohash   =  hash(msgBuf->mbto);    

	msgTab1[slot].mtmsgflags.MAIL = 1;

	if (*msgBuf->mbfwd)  msgTab7[slot].mtfwdhash = hash(msgBuf->mbfwd);
    }

    if (*msgBuf->mbgroup)
    {
	msgTab5[slot].mttohash   =  hash(msgBuf->mbgroup);
	msgTab1[slot].mtmsgflags.LIMITED = 1;
    }

/*  if (*msgBuf->mboname)
      msgTab9[slot].mtorigin = hash(msgBuf->mboname); */

    if (strcmpi(msgBuf->mbzip, cfg.nodeTitle) != SAMESTRING && *msgBuf->mbzip)
    {
	msgTab1[slot].mtmsgflags.NET = 1;
	msgTab5[slot].mttohash       = hash(msgBuf->mbzip);
    }

    if (*msgBuf->mbx)  msgTab1[slot].mtmsgflags.PROBLEM = 1;

    msgTab1[slot].mtmsgflags.RECEIVED = 
	((msgBuf->mbattr & ATTR_RECEIVED) == ATTR_RECEIVED);

    msgTab1[slot].mtmsgflags.REPLY    = 
	((msgBuf->mbattr & ATTR_REPLY   ) == ATTR_REPLY   );

    msgTab1[slot].mtmsgflags.MADEVIS  = 
	((msgBuf->mbattr & ATTR_MADEVIS ) == ATTR_MADEVIS );

    msgTab4[slot].mtroomno = msgBuf->mbroomno;

    /* This is special. */
    if  (*msgBuf->mbcopy)
    {
	msgTab1[slot].mtmsgflags.COPY = 1;

	/* get the ID# */
	sscanf(msgBuf->mbcopy, "%ld", &copy);

	/* msgTab3[slot].mtoffset = (ushort)(here - copy); */
	msgTab8[slot].mtomesg = (uint)(here - copy); 
    }

    if (roomBuild) buildroom();
}
