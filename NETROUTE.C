/* -------------------------------------------------------------------- */
/*  NETROUTE.C                    Atlas                                 */
/* -------------------------------------------------------------------- */
/*                      Autorouting handling routines                   */
/*                                                                      */
/*               NOTE: * To compile as standalone DOS application,      */
/*                       define STANDALONE and NETWORK (/DSTANDALONE    */
/*                       /DNETWORK).                                    */
/*                                                                      */
/* -------------------------------------------------------------------- */

#ifdef NETWORK

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */

#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <errno.h>
#include <string.h>

/* Some headers can't be used in STANDALONE, because I must define stuff.  */

#ifndef STANDALONE

#include "keywords.h"
#include "ctdl.h"
#include "proto.h"
#include "global.h"

#endif


#ifdef STANDALONE

#include "netroute.inc"
#include <time.h>

#define MAXBUF    512
#define ACCEL_VAL 100

#endif

#define TOPCOUNT  30     /* Upper limit of Route Goodness field.        */
#define MAXNODES 200     /* Max # of indirect-connect nodes allowed.    */
#define MAXALIAS   5     /* Max # of chars in an alias.                 */
#define MOSTLENS  30     /* Width of boardname fields in #ROUTE line.   */

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  addRoute()      appends the route of a good net message to tmp file */
/*  updateRoutes()  adjusts ROUTE.CIT based on routes in tmp file       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  11/12/91     (JRD)   Created                                        */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* This is a debug thang.  */

#include <stdarg.h>
#include <stdio.h>

extern  char prtf_buff[512];

void d (char *fmt, ... )
{
    register char *buf = prtf_buff;
    register char *path ;
    va_list ap;
    int hFile, pos ;

    va_start(ap, fmt);
    vsprintf(prtf_buff, fmt, ap);
    va_end(ap);

    path = (char *) malloc (80 * sizeof (char)) ;
    sprintf (path, "%s\\ROUTE.LOG", cfg.homepath) ;
    hFile = sopen (path,
		   O_CREAT | O_TEXT | O_APPEND | O_WRONLY,
		   SH_DENYWR,
		   S_IREAD | S_IWRITE) ;

    /* Replace CR's with dorky underscores.  */

    for (pos = 0; pos < (int)strlen (buf); pos++)
	if ('\n' == buf[pos])
	   buf[pos] = '_' ;

    write (hFile, buf, strlen (buf)) ;

    write (hFile, "\n", 1) ;
    close (hFile) ;
    free (path) ;
}



/* -------------------------------------------------------------------- *
 *
 * Autorouter Design Specifications.
 *
 * The autorouter operates transparently.  It balances operations find the
 * most expedient route possible to any one node.  It also allows the sysop
 * to manually manage routing.
 *
 * The format of the ROUTE.CIT file has changed slightly.  A third field has
 * been added to the #ROUTE keyword, and the #AVOID keyword has been added.
 *
 * The third field in #ROUTE is in integer from 0 to TOPCOUNT which rates the
 * "goodness" of a particular route.  For every public message integrated
 * through a direct-connect from Node X, this Route Count is incremented if
 * the direct-connect is Node X's route, decremented if it is not.  If the
 * Route Count reaches zero, then whatever direct-connect provides the next
 * message from Node X is made X's #route, with a Route Count of zero.  The
 * Route Count isn't incremented above TOPCOUNT.  This is an arbitrary # and
 * in unusual networking situations (TC+ messages per network, for example)
 * this method will not provide optimal performance.  But it won't break,
 * either.
 *
 * If not present, the Route Count field will be added at 0.
 *
 * If the keyword LOCKED (or LOCK) appears in place of the Route Count, the
 * route won't be messed with.
 *
 * There are two conditions in which a Route Count won't be incremented even
 * though a message passing through the direct-connect was received.  They
 * are:
 * 1) If the route to Node X contains a node specified in an #AVOID keyword,
 *    or
 * 2) If the route to Node X contains a ".." (path lost).
 *
 * -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  addRoute()      appends the route of a good net message to tmp file */
/* -------------------------------------------------------------------- */

/*
 * If there is no ROUTE.TMP in the HOME directory, addRoute creates it.
 * It then appends the msgPath to this file.
 *
 */

void addRoute (char *msgPath)
{
    int  hOutFile ;
    char path[256] ;

    sprintf (path, "%s\\ROUTE.TMP", cfg.homepath) ;

    hOutFile = sopen (path,
		   O_CREAT | O_TEXT | O_APPEND | O_WRONLY,
		   SH_DENYWR,
		   S_IREAD | S_IWRITE) ;

    if (-1 == hOutFile)
	crashout("Can't create ROUTE.TMP!");

    msgPath = strcat (msgPath, "\n") ;
    write (hOutFile, msgPath, strlen (msgPath)) ;

    close (hOutFile) ;
}


/* -------------------------------------------------------------------- */
/* missingLinks() scans a path for a !..! sequence.                     */
/* -------------------------------------------------------------------- */

BOOL missingLinks (char *path)
{
    unsigned int  i ;
	     char *pch ;

    d ("      +++ Entering missingLinks with %s.", path) ;

    pch = path ;

    for (i = 0; i < (strlen (path) - 4); i++)
	{
	if (SAMESTRING == strncmp (pch, "!..!", 4))
	    {
	    d ("      +++ CONTAINS a missing link.") ;
	    return TRUE ;
	    }
	pch++ ;
	}
    d ("      +++ No missing linke found.") ;
    return FALSE ;
}

/* -------------------------------------------------------------------- */
/* nodeInPath () makes sure the path doesn't contain any avoided nodes, */
/*               overlooking the origin node.                           */
/* -------------------------------------------------------------------- */

BOOL nodeInPath (char *path, char *avoids[])
{
    int  i ;
    char *pch ;

    d ("      ^^^ Entering nodeInPath...") ;

    for (i = 0; avoids[i]; i++)
	{
	pch = strstr (path, avoids[i]) ;

	/* To be >in< the path, the node must be present, and must NOT be  */
	/* the node of origin (first entry).  You still route TO avoids.   */
	/* You just don't route THROUGH them.                              */

	if ((NULL != pch) && (pch != path))
	    {
	    d ("      ^^^ Exiting nodeInPath with TRUE.") ;
	    return TRUE ;
	    }
	}
    d ("      ^^^ Exiting nodeInPath with FALSE.") ;
    return FALSE ;
}


typedef struct routeEntrytag
{  

    label    targetNode ;
    label    throughNode ;
    char     alias[MAXALIAS] ;
    unsigned fLocked   : 1 ;
    unsigned fChanged  : 1 ;  /* If bRouteCount is changed or node is new. */
    unsigned fSaved    : 1 ;
    unsigned fNewAlias : 1 ;
    uchar bRouteCount ;
} routeEntry ;


/* -------------------------------------------------------------------- */
/* format() writes a #ROUTE line to buf in fields.  For STANDALONE,     */
/* don't write the last field.                                          */
/* -------------------------------------------------------------------- */


void format (char *buf, routeEntry *node)
{
    char spaces[] = "                                   ",
	 *target, *through ;

    d ("            ENTERING format with %s...", buf) ;

    target = (char *) malloc (80 * sizeof (char)) ;
    through = (char *) malloc (80 * sizeof (char)) ;

    sprintf (target, "\"%s\"", node->targetNode) ;
    if (strlen(target) < MOSTLENS)
	strncat (target, spaces, (MOSTLENS - strlen(target))) ;

    sprintf (through, "\"%s\"", node->throughNode) ;
    if (strlen(through) < MOSTLENS)
	strncat(through, spaces, (MOSTLENS - strlen(through))) ;

#ifdef STANDALONE
    sprintf (buf, "#ROUTE %s %s\n", target, through) ;
#else
    sprintf (buf, "#ROUTE %s %s %2d\n", target, through, node->bRouteCount) ;
#endif

    free (target) ;
    free (through) ;
    d ("            EXITING format.") ;

}



/* ------------------------------------------------------------------------*
 *                                                                         *
 * findBang finds the next exclamation in the string and returns the       *
 * offset to it.  Returns -1 if no further bangs.                          *
 *                                                                         *
/* ------------------------------------------------------------------------*/

int findBang (char *ch)
{
    int i = 0 ;
    char *pch ;

    pch = strchr (ch, '!') ;
    if (NULL == pch)
	{
	d ("   === There is no bang in %s.", ch) ;
	return (-1) ;
	}
    else
	{
	d ("   === There is a bang in %s, at char #%d", ch, pch - ch) ;
	return (pch - ch) ;
	}
}



/* ------------------------------------------------------------------------*
 *                                                                         *
 * remove_word (char *stri, int n) removes nth word from string stri, and  *
 * returns the number of words remaining in the string.                    *
 *                                                                         *
/* ------------------------------------------------------------------------*/

int remove_word (char *stri, int n)
{
    char *buf, *words[256] ;
    int  i, tot ;

    d ("      *** Removing word number %d from %s", n, stri) ;

    buf = malloc (256 * sizeof (char)) ;

    if (strlen(stri) > 256)
	{
	free (buf) ;
	d ("      *** String is too long.  Failed.") ;
	return -1 ;                          /* Failed.  */
	}

    strcpy (buf, stri) ;
    tot = parse_it (words, buf) ;

    if (n > tot)
	{
	free (buf) ;
	d ("      *** There aren't that many words.  Failed.") ;
	return tot ;                        /* Failed.  */
	}

    stri[0] = '\0' ;

    for (i = 0; i < tot; i++)
	if (i != n)
	    {
	    strcat (stri, words[i]) ;
	    strcat (stri, " ") ;
	    }

    d ("      *** The resulting string is %s.", stri) ;
    free (buf) ;
    d ("      *** Exiting word remover.") ;
    return (tot - 1) ;
}

/* ------------------------------------------------------------------------*/
/*                                                                         */
/* accept() makes sure the proposed alias hasn't been used.  If not, it    */
/* saves it to the file, plops it in the route datastructure, and returns  */
/* TRUE.                                                                   */
/*                                                                         */
/* ------------------------------------------------------------------------*/

BOOL accept (char *tmpbuf, int entry, int iMax,
				    routeEntry *routes, int hOutFile)
{
    int i, j, len ;
    char buf [80] ;

    d ("      Entering accept code.") ;

    if (strlen (tmpbuf) > MAXALIAS)
	{
	printf ("ACK!  Error in NETROUTE.\n") ;
	d ("      ACK.  buffer too long.") ;
	return FALSE ;
	}

    d ("      Checking if it has been used...") ;
    for (i = 0; i <= iMax; i++)
	{
	if (SAMESTRING == stricmp (routes[i].alias, tmpbuf))
	    return FALSE ;
	}
    d ("      Apparently, it has not been used." ) ;

    len = strlen (tmpbuf) ;

    for (j = 0; j < len; j++)
	tmpbuf[j] = (char)toupper(tmpbuf[j]) ;

    buf[0] = '\0' ;

    d ("      Intend to write the alias %s.", buf) ;

    sprintf (buf, "#ALIAS   \"%s\"", tmpbuf) ;
    write (hOutFile, buf, strlen (buf)) ;
    write (hOutFile, "           ", 10 - strlen (tmpbuf)) ;
    sprintf (buf, "\"%s\"\n", routes[entry].targetNode) ;
    write (hOutFile, buf, strlen (buf)) ;

    strcpy (routes[entry].alias, tmpbuf) ;
    d ("      Alias line written.   Exiting accept code." ) ;
    return TRUE ;
}


/* ------------------------------------------------------------------------*/
/*                                                                         */
/* oneword() tries to make an alias from a single word passed to it.  If   */
/* the word contains less than the target number of characters, or if      */
/* accept() says the string isn't unique, oneword() fails.  If the word    */
/* contains more than target characters, only target characters are passed */
/* to accept().                                                            */
/*                                                                         */
/* ------------------------------------------------------------------------*/

BOOL oneword (char *name, uchar target, int entry, int iMax,
				   routeEntry *routes, int hOutFile)
{
    label tbuf ;

    if (strlen (name) < target)
	return FALSE ;

    strncpy (tbuf, name, target) ;
    tbuf[target] = '\0' ;

    return (accept (tbuf, entry, iMax, routes, hOutFile)) ;
}



/* ------------------------------------------------------------------------*
 *                                                                         *
 * writeAliases() makes up a unique alias for nodes which need one, and    *
 * writes the new alias to hOutFile (which is already open).               *
 *                                                                         *
/* ------------------------------------------------------------------------*/


void writeAliases (int hOutFile, routeEntry *routes, int iMax)
{
    int   i, j, iWords ;
    label tbuf, name ;
    char  **words, *tmpAlias ;

    d ("   Entering alias writer.") ;

    words    = (char **) malloc (20 * sizeof (char *)) ;
    tmpAlias = (char *)  malloc (15 * sizeof (char)) ;

    for (i = 0; i <= iMax; i++)
	if (routes[i].fNewAlias)                /* Needs a new alias.  */
	    {
	    d ("   The node %s needs a new alias.", routes[i].targetNode) ;
	    strcpy (name, routes[i].targetNode) ;
	    strcpy (tbuf, name) ;
	    iWords = parse_it (words, tbuf) ;

	    /* Strip out all the "THE"'s and "BBS"'s from the word, unless */
	    /* it's the last word left.                                    */

	    for (j = 0; ( (j < iWords) && (iWords > 1) ) ; j++)
		if ( (SAMESTRING == stricmp (words[j], "THE")) ||
		     (SAMESTRING == stricmp (words[j], "BBS"))
		   )
		    {
		    iWords = remove_word (name, j) ;
		    j = 0 ;       /* Reset it to the first word.        */
		    continue ;
		    }

	    /* If the BBS has a single-word name, try to give it an ALIAS  */
	    /* with three or more letters, two letters, or one letter.     */

	    if (iWords == 1)
		{
		if (oneword (name, 3, i, iMax, routes, hOutFile))
		    continue ;
		if (oneword (name, 2, i, iMax, routes, hOutFile))
		    continue ;
		if (oneword (name, 1, i, iMax, routes, hOutFile))
		    continue ;
		}

	    /* If the BBS has more than one word, form a tmp alias with    */
	    /* the first letters of each word.  If this alias is three     */
	    /* letters long, try to accept it.  Else, try to accept any    */
	    /* three-letter word.  Else, pass the first word to oneword    */
	    /* with a 3 letter target.  Else, try to accept the whole      */
	    /* temporary alias, up to four letters.  Else, pass the second */
	    /* through the last word to oneword with a 3 letter target.    */
	    /* Else, pass all words in order to oneword with a 4 letter    */
	    /* target.  If any of these are accepted, move to the next     */
	    /* name.  If none of these combinations work, well, that's     */
	    /* tough luck.                                                 */

	    else
		{
		strcpy (tbuf, name) ;
		parse_it (words, tbuf) ;

		for (j = 0; j < iWords; j++)
		    tmpAlias[j] = words[j][0] ;
		tmpAlias[j] = '\0' ;
		tmpAlias[4] = '\0' ;     /* No more than 4 allowed.  */

		if (3 == strlen (tmpAlias))
		    if (accept(tmpAlias, i, iMax, routes, hOutFile))
			continue ;

		for (j = 0; j < iWords; j++)
		    if (3 == strlen (words[j]))
			if (accept (words[j], i, iMax, routes, hOutFile))
			    break ;
		if (j < iWords) continue ;

		if (oneword (words[0], 3, i, iMax, routes, hOutFile))
		    continue ;

		if (accept (tmpAlias, i, iMax, routes, hOutFile))
		    continue ;

		for (j = 1; j < iWords; j++)
		    if (oneword (words[j], 3, i, iMax, routes, hOutFile))
			break ;
		if (j < iWords) continue ;

		for (j = 0; j < iWords; j++)
		    if (oneword (words[j], 4, i, iMax, routes, hOutFile))
			break ;
		if (j < iWords) continue ;
		}
	    }
    free (words) ;
    free (tmpAlias) ;
    d ("   Exiting alias writer.") ;
}


/************************************************************************/
/* -------------------------------------------------------------------- */
/*  updateRoutes()  adjusts ROUTE.CIT based on routes in tmp file       */
/* -------------------------------------------------------------------- */
/************************************************************************/

void updateRoutes(char *connectname)
{
    routeEntry *routes ;
    label      origin ;
    char       **avoids, *pch, *originalBuf, *buf, **words, *filepath ;
    int        hOutFile, hInFile, i, iTopRoute, iTopAvoid ;
    BOOL       fAliasesWritten = FALSE ;

    routes      = (routeEntry *) malloc (MAXNODES * sizeof (routeEntry)) ;
    originalBuf = (char *)       malloc (256      * sizeof (char)) ;
    buf         = (char *)       malloc (256      * sizeof (char)) ;
    filepath    = (char *)       malloc (256      * sizeof (char)) ;
    avoids      = (char **)      malloc (MAXNODES * sizeof (char *)) ;
    words       = (char **)      malloc (256      * sizeof (char *)) ;

    sprintf (filepath, "%s\\ROUTE.LOG", cfg.homepath) ;
    hOutFile = sopen (filepath,
		   O_CREAT | O_TEXT | O_WRONLY | O_TRUNC,
		   SH_DENYWR,
		   S_IREAD | S_IWRITE) ;

#define binky "\nSTARTING AUTOROUTER\n"

    write (hOutFile, binky, strlen (binky) ) ;

    close (hOutFile) ;

    sprintf (filepath, "%s\\ROUTE.TMP", cfg.homepath) ;

    if (FALSE == filexists (filepath))         /* If no ROUTE.TMP, abort. */
	goto endUpdate ;

    /* Read ROUTE.CIT's #ROUTEs and #AVOIDs into array.  If no Count, zero */

    sprintf (filepath, "%s\\ROUTE.CIT", cfg.homepath) ;

    /* If no ROUTE.CIT, create one.  */

    if (FALSE == filexists (filepath))
	{
	hOutFile = sopen (filepath, O_TEXT | O_CREAT, SH_DENYNO) ;
	close (hOutFile) ;
	}

    hInFile = sopen (filepath, O_TEXT | O_RDONLY, SH_DENYNO) ;

    if (-1 == hInFile)
	crashout ("Problem opening ROUTE.CIT.") ;

    for (i = 0; i < MAXNODES; i++)
	{
	avoids[i] = NULL ;
	routes[i].fSaved = FALSE ;

	/* We're looking for an alias by default.  Only if one already     */
	/* exists will this bit be cleared.  If this were changed to       */
	/* FALSE, new #ALIASes would only be calculated for brand spanking */
	/* new nodes.                                                      */

	routes[i].fNewAlias = TRUE ;
	}

    /* ----------------------------------------------------------------- */
    /* PART ONE: READ THE ROUTE.CIT FILE FOR #AVOIDS and #ROUTES.        */
    /* ----------------------------------------------------------------- */

    iTopRoute = -1 ;
    iTopAvoid = -1 ;

    while ( !eof(hInFile) )
	{
	if (!sfgets (buf, 255, hInFile))
	    break ;                            /* Error reading file. */

	if ('#' != buf[0])                     /* It's not a keyword. */
	    continue ;

	parse_it (words, buf) ;

	if (SAMESTRING == stricmp (words[0], "#ROUTE"))
	    {
	    if (words[2] == NULL)         /* Invalid #ROUTE entry. */
		continue ;

	    /* We have a new target and its route! */

	    ++iTopRoute ;
	    strcpy (routes[iTopRoute].targetNode, words[1]) ;
	    strcpy (routes[iTopRoute].throughNode, words[2]) ;
	    routes[iTopRoute].bRouteCount = (uchar) atoi(words[3]) ;
	    routes[iTopRoute].fChanged = FALSE ;

	    if ('L' == toupper(words[3][0]))
		routes[iTopRoute].fLocked = TRUE ;
	    else
		routes[iTopRoute].fLocked = FALSE ;
	    }

	if (SAMESTRING == stricmp (words[0], "#AVOID"))
	    {
	    if (words[1] == NULL)         /* Invalid #AVOID entry. */
		continue ;

	    /* We have a new node to avoid. */

	    ++iTopAvoid ;
	    avoids[iTopAvoid] = (char *) malloc (LABELSIZE) ;
	    strcpy (avoids[iTopAvoid], words[1]) ;
	    }
	}

    d ("Number of #AVOIDs in ROUTE.CIT: %d.", iTopAvoid+1) ;
    d ("Number of #ROUTEs in ROUTE.CIT: %d.", iTopRoute+1) ;

    /* ---------------------------------------------------------- */
    /* PART ONE POINT FIVE: READ THE ROUTE.CIT FILE FOR #ALIASes. */
    /* ---------------------------------------------------------- */

    lseek (hInFile, 0, SEEK_SET) ;    /* Go back to start of file.  */

    while ( !eof(hInFile) )
	{
	if (!sfgets (buf, 255, hInFile))
	    break ;                            /* Error reading file. */

	if ('#' != buf[0])                     /* It's not a keyword. */
	    continue ;

	parse_it (words, buf) ;

	if (SAMESTRING == stricmp (words[0], "#ALIAS"))
	    {
	    if (words[2] == NULL)       /* Invalid #ALIAS entry.  */
		continue ;

	    /* We have an alias.  Try to find its corresponding node.  If  */
	    /* no corresponding node exists, oh fucking well.  Just hope   */
	    /* the autoaliaser doesn't create a new alias that's the same. */
	    /* If it does, the new alias will be listed first in           */
	    /* ROUTE.CIT, so it's not like the world will end, is it?      */

	    for (i = 0; i <= iTopRoute; i++)
		{
		if (SAMESTRING == stricmp(words[2], routes[i].targetNode))
		    {
		    strncpy (routes[i].alias, words[1], MAXALIAS) ;
		    routes[i].alias[MAXALIAS - 1] = '\0' ;
		    routes[i].fNewAlias = FALSE ;
		    d ("Alias '%s' found for node '%s'.", routes[i].alias, routes[i].targetNode) ;
		    break ;
		    }
		}
	    }
	}
    close (hInFile) ;

    /* ------------------------------------------- */
    /* PART TWO: READ THE ROUTE.TMP FILE.          */
    /* ------------------------------------------- */

    sprintf (filepath, "%s\\ROUTE.TMP", cfg.homepath) ;

    hInFile = sopen (filepath, O_TEXT | O_RDONLY, SH_DENYNO) ;

    if (-1 == hInFile)
	crashout ("Problem opening ROUTE.TMP.") ;

    d ("ROUTE.TMP opened.") ;

    while ( !eof(hInFile) )
	{
	if (!sfgets (buf, 512, hInFile))
	    break ;                            /* Error reading file. */

	d ("The ROUTE.TMP line reads: %s", buf) ;

/* If this is a standalone router, the connectname passed to us is just a  */
/* blank buffer, and isn't a node name (because, obviously, we're not      */
/* connected to anything).  The connectname must instead be derived from   */
/* the last field in the path.  I hear that some Cits don't do things this */                                             
/* way, and leave the origin node out of the path.  I could read the       */
/* origin name out of MSG.DAT, but I won't for now.                        */

#ifdef STANDALONE
	strcpy (connectname, buf) ;
	pch = connectname ;
	pch[strlen(pch) - 1] = '\0' ;   /* Nuke the CR. */

	/* Step backwards until you hit a bang or the beginning.  */

	pch = &( pch[strlen(pch)] ) ;

	while ( (*pch != '!') && (pch != connectname) )
	    --pch ;

	if (*pch == '!')
	    ++pch ;

	strcpy (connectname, pch) ;

#endif /* STANDALONE */

	pch = buf ;

	/* If the route contains no bangs, it is a direct-connect.  Mail   */
	/* to a direct-connect should be routed directly.  But there are   */
	/* situations when this may not be all that great; what if, for    */
	/* example, you net once a week?  If it's a direct-connect route,  */
	/* I route mail to it through it, with a goodness count of         */
	/* TOPCOUNT.  Why?  Well, why would people have a direct-connect   */
	/* that doesn't provide enough new messages to influence the       */
	/* auto-router, if they didn't establish the connection primarily  */
	/* for mail purposes?  But if they get more than TOPCOUNT messages */
	/* from this node through other sources before getting one         */
	/* directly, well that's just tough luck.                          */

	i = findBang (pch) ;

	if (-1 == i)                            /* From direct-connect.  */
	    strcpy (origin, connectname) ;
	else                                /* not from direct-connect. */
	    {
	    strncpy (origin, pch, i) ;
	    origin[i] = '\0' ;
	    }

	for (i = 0; i <= iTopRoute; i++)
	    if (SAMESTRING == stricmp (origin, routes[i].targetNode))
		break ;

	/* If we exited the last loop the "normal" way, the node is        */
	/* unknown, and must be added.  Otherwise, see if the RouteCount   */
	/* should be changed.                                              */

	d ("Original line: %s.  Connect: %s.  Origin: %s.",
				       buf, connectname, origin) ;

	if (i > iTopRoute)                  /* Node unknown!  Autonode it! */
	    {
	    d ("'%s' shows the new node '%s'.  Autonoding...", buf, origin) ;
	    cPrintf ("    %s is a new node.", origin) ; doccr() ;

	    trap ("New node: %s", T_NETWORK) ;

	    ++iTopRoute ;
	    strcpy (routes[iTopRoute].targetNode, origin) ;
	    strcpy (routes[iTopRoute].throughNode, connectname) ;
	    routes[iTopRoute].fLocked = FALSE ;
	    routes[iTopRoute].fChanged = TRUE ;
	    routes[iTopRoute].bRouteCount = 0 ;
	    routes[iTopRoute].fNewAlias = TRUE ;
	    d ("Autonoding complete.") ;
	    }
	else   /* Known node. */
	    {
	    d ("Node '%s' is known.", origin) ;

	    if (routes[i].fLocked)          /* Don't touch. */
		{
		d ("This node is locked, so leaving it alone.") ;
		cPrintf ("    %s is a locked node.\n", origin) ;
		continue ;
		}

	    /* If the current connect is the same as the message's origin, */
	    /* make the dest and through equal, and force the goodness     */
	    /* count to TOPCOUNT.  See above for details.                  */

	    if (SAMESTRING == stricmp (connectname, origin))
		{
		d ("'%s' is a direct-connect.", connectname) ;
		strcpy (routes[i].throughNode, connectname) ;
		routes[i].fLocked = FALSE ;
		routes[i].fChanged = TRUE ;
		routes[i].bRouteCount = TOPCOUNT ;
		}

	    /* If the current connect isn't the Origin's normal route, and */
	    /* if the count is > 0, decrement it, but don't note a change  */
	    /* in STANDALONE version.  If the count is already at zero,    */
	    /* switch routes!                                              */

	    if (SAMESTRING != stricmp (connectname, routes[i].throughNode))
		{
		d ("This route decreases the node's current goodness count.") ;

		if (routes[i].bRouteCount > 0)
		    {
		    d ("Decrementing the count.") ;
		    routes[i].bRouteCount-- ;
#ifndef STANDALONE
		    routes[i].fChanged = TRUE ;         
#endif
		    }
		else
		    {
		    d ("Changing the route.") ;
		    strcpy (routes[i].throughNode, connectname) ;
		    routes[i].fChanged = TRUE ;
		    }
		}

	    /* If the current connect IS the Origin's normal route, and if */
	    /* the Route Count < TOPCOUNT, and if the path has no ".."'s,  */
	    /* and if the path it took to get here has has no #AVOIDs,     */
	    /* increment the Route Count.  But do NOT note this change in  */
	    /* STANDALONE, as it doesn't really change its ROUTE.CIT.      */

	    else
		{
		d ("This route reinforces the goodness of %s's #ROUTE.", origin) ;
		if ( (routes[i].bRouteCount < TOPCOUNT)   &&
		     (FALSE == missingLinks (buf)     )   &&
		     (FALSE == nodeInPath (buf, avoids)) )
		    {
		    d ("Incrementing the goodness of this route.") ;
		    routes[i].bRouteCount++ ;
#ifndef STANDALONE  
		    routes[i].fChanged = TRUE ;      
#endif
		    }
		}
	    }
	}
    close (hInFile) ;

    d ("ROUTE.TMP is closed.") ;

    /* Deallocate #AVOID memory.  */

    for (i = 0; avoids[i] != NULL; i++)
	free (avoids[i]) ;

    /* ---------------------------------------------------------- */
    /* PART THREE: IF ROUTE INFO HAS CHANGED, REWRITE ROUTE.CIT.  */
    /* ---------------------------------------------------------- */

    for (i = 0; i <= iTopRoute; i++)
	if (routes[i].fChanged)
	    break ;

    if (i > iTopRoute)
	{
	/* In any event, delete ROUTE.TMP.  */
	sprintf (buf, "%s\\ROUTE.TMP", cfg.homepath) ;
	unlink (buf) ;
	goto endUpdate ;
	}

    sprintf (filepath, "%s\\ROUTE.TMP", cfg.homepath) ;

    unlink (filepath) ;

    hOutFile = sopen (filepath,
		   O_CREAT | O_TEXT | O_WRONLY,
		   SH_DENYNO ,
		   S_IREAD | S_IWRITE) ;

    if (-1 == hOutFile)
	crashout("Can't create ROUTE.TMP!") ;

    d ("ROUTE.TMP has been opened for writing.") ;

    sprintf (filepath, "%s\\ROUTE.CIT", cfg.homepath) ;

    hInFile = sopen (filepath, O_TEXT | O_RDONLY, SH_DENYNO) ;

    if (-1 == hInFile)
	crashout ("Problem opening ROUTE.CIT.") ;

    d ("ROUTE.CIT has been opened for reading.") ;

    while ( !eof(hInFile) )
	{
	if (!sfgets (buf, 255, hInFile))
	    break ;                            /* Error reading file. */

	strcpy (originalBuf, buf) ;

	d ("Transferring line: %s.", originalBuf) ;

	/* If this is an #ALIAS or #ROUTE and we have not written the new  */
	/* aliases yet, call writeAliases.  This will keep the #ALIASes at */
	/* the top of the file.  Then pass the buf on to be written or     */
	/* modified (as with #ROUTEs).                                     */

	if ( (strnicmp(buf, "#ALIAS", 6) == SAMESTRING) && !fAliasesWritten)
	    {
	    writeAliases (hOutFile, routes, iTopRoute) ;
	    fAliasesWritten = TRUE ;
	    }

	if (strnicmp (buf, "#ROUTE", 6) == SAMESTRING)
	    {

	    if (!fAliasesWritten)
		{
		writeAliases (hOutFile, routes, iTopRoute) ;
		fAliasesWritten = TRUE ;
		}

	    parse_it (words, buf) ;

	    if (words[2] == NULL)         /* Invalid #ROUTE entry. */
		continue ;

	    /* We have a target and its route!  Its memory brother is      */
	    /* guaranteed to exist.  If we have found the memory brother,  */
	    /* and the route info has changed, and the route is not        */
	    /* locked, create a new line to write to ROUTE.CIT.  Then      */
	    /* clear the changed flag so we can idenfity new nodes later.  */

	    for (i = 0; i <= iTopRoute; i++)
		{
		if (    (SAMESTRING == strcmp (routes[i].targetNode, words[1]))
		     && (routes[i].fChanged)
		     && (!routes[i].fLocked)
		   )
		    {
		    d ("Formatting a new line...") ;
		    format (originalBuf, &(routes[i])) ;
		    routes[i].fChanged = FALSE ;
		    break ;
		    }
		}
	    }
	d ("Line being written: %s.", originalBuf) ;
	write (hOutFile, originalBuf, strlen (originalBuf)) ;
	}

    close (hInFile) ;

    /* Check for fChanged flags which denote nodes new to us this call.  */

    for (i = 0; i <= iTopRoute; i++)
	if (routes[i].fChanged)
	    {
	    format (buf, &(routes[i])) ;
	    d ("Node being tacked on to the end with: %s.", buf) ;
	    write (hOutFile, buf, strlen (buf)) ;
	    }

    if (0 == close (hOutFile))     /* SUccessful close.  Rename... */
	{
	d ("ROUTE.TMP closed.") ;
	sprintf (filepath, "%s\\ROUTE.CIT", cfg.homepath) ;
	sprintf (buf, "%s\\ROUTE.TMP", cfg.homepath) ;
	unlink (filepath) ;
	rename (buf, filepath) ;
	d ("ROUTE.TMP renamed to ROUTE.CIT") ;
	}
    else
	cPrintf ("Unable to close ROUTE.TMP!") ;

/* We're outta here.  Free up memory.  */

endUpdate:
    free (routes) ;
    free (originalBuf) ;
    free (buf) ;
    free (filepath) ;
    free (avoids) ;
    free (words) ;
    d ("EXITING AUTOROUTER.") ;
}

#ifdef STANDALONE


/*-------------------------------------------------------------------------*/
/*                                                                         */
/* findAPath cues the file pointer up to a '/0' + 'P', which is            */
/* probably followed by a path.  If Full Mode is off, no speed balancing   */
/* is performed.  If two consecutive NULLs are found, we skip ahead to     */
/* accelerate running on new BBS's.                                        */
/* RETURNS : If we're cued up to a path, return TRUE.                      */
/*           If EOF, return FALSE.                                         */
/*                                                                         */
/*-------------------------------------------------------------------------*/

BOOL findAPath (int hFile, BOOL fFull, BOOL fVerbose)
{
    /* lSkipAhead is the number of bytes being skipped per path or when we */
    /* have hit mega-NULLs.  After each path, if we're not in Full Mode,   */
    /* the percent completion of the message file is compared with the     */
    /* percent elapsed of the target five minute scan time.  If we're      */
    /* behind, lSkipAhead is incremented by ACCEL_VAL.  If we're ahead,    */
    /* lSkipAhead is decremented by ACCEL_VAL, if above zero.              */

    static long  lSkipAhead = 0 ;
    static float rStartTime, rFileLen ;
    static int   hLastFile = -1 ;

	   long  lCurPos ;
	   float rFilePercent, rTimePercent ;
	   int   err ;
	   uchar ch ;

    /* On the first pass of a file different from the file used on the     */
    /* last pass, vars for file length and start time are initialized.     */

    if (hLastFile != hFile)
	{
	rFileLen = (float) filelength (hFile) ;
	rStartTime = (float) time (NULL) ;
	hLastFile = hFile ;
	}

    /* This loop is returned out of w/ TRUE or FALSE at various points     */
    /* within it.                                                          */

    for (;;)
	{
	err = read (hFile, &ch, 1) ;
	if (err <= 0)
	    return FALSE ;

	/* If we're on a NULL, and the next char is a 'P', return TRUE.    */
	/* If the next char is also a NULL and we're not in Full Mode,     */
	/* "speed up" twice so no decrement can bring the accel value back */
	/* to zero in the subsequent adjustment.  If the next char is      */
	/* anything else, just move on, adjusting speed if necessary.      */

	if ('\0' == ch)
	    {
	    err = read (hFile, &ch, 1) ;
	    if (err <= 0)
		return FALSE ;

	    switch (ch)
		{
		case 'P':
		    return TRUE ;

		case '\0':
		    lSkipAhead += (ACCEL_VAL + ACCEL_VAL) ;

		default:
		    if (!fFull)
			{
			/* If % file done < % time done, inc lSkipAhead.  */

			lCurPos = tell (hFile) ;

			rFilePercent = (float)lCurPos / rFileLen ;
			rTimePercent = ( (float)time(NULL) - rStartTime) /
					  (float) (5 * 60) ;

			if (rFilePercent < rTimePercent)
			    lSkipAhead += ACCEL_VAL ;
			else
			    if (lSkipAhead)
				lSkipAhead -= ACCEL_VAL ;

			if ((lSkipAhead + lCurPos) > (long)rFileLen)
			    {
			    lseek (hFile, 0, SEEK_END) ;
			    return FALSE ;
			    }

			lseek (hFile, lSkipAhead, SEEK_CUR) ;
			}
		}
	    }
	}
}


/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Following is a main() included so I can easily compile standalone       */
/* versions and updates based on the same code.  The standalone version    */
/* reads all the routes out of MSG.DAT and into ROUTES.TMP, and then calls */
/* updateRoutes above.                                                     */
/*                                                                         */
/* The standalone version has a few command-line switches:                 */
/*                                                                         */
/* /V           Verbose mode.  The user gets to see decisions being made.  */
/* /F           Full mode.  Gets paths from ENTIRE message base, no matter */
/*                  how long it takes.  Normally, MSG.DAT scan reading     */
/*                  dynamically adjusts to take 5 minutes.                 */
/* /H           Help.  Lists the commands.                                 */
/* /D           Skip the MSG.DAT read, and just work with ROUTE.TMP.       */
/*                                                                         */
/*-------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    char ch, buf[MAXBUF], *bufptr, *endbufptr ;
    int  hInFile, hOutFile ;
    BOOL fVerbose = FALSE, fFull = FALSE ;

    printf ("\nAtlas Autorouter v1.06b, by Angela Davis.\n") ;
    printf ("Run NETROUTE in Cit's home directory.\n") ;
    printf ("Enter NETROUTE /H for help.\n\n") ;

    /* Set cfg.homepath equal to whatever directory we're in.  */

    if (NULL == getcwd (cfg.homepath, 64))
	{
	printf ("Unable to read current directory.  Aborting.\n") ;
	return (-4) ;
	}

    while (argc > 1)
	{
	char ch ;

	--argc ;
	ch = (char) toupper(argv[argc][1]) ;

	if ('H' == ch)
	    {
	    printf ("There are two command-line switches:\n\n") ;
	    printf ("/V turns on Verbose Mode.\n") ;
	    printf ("/F turns on Full Mode.  This causes a complete scan of your message file.\n") ;
	    printf ("   Otherwise, a fast scan is made, which does not include every route.\n\n") ;
	    return (0) ;
	    }

	if ('V' == ch)
	    {
	    printf ("Verbose mode ON.\n") ;
	    fVerbose = TRUE ;
	    }

	if ('F' == ch)
	    {
	    printf ("Full mode ON.\n") ;
	    fFull = TRUE ;
	    }

	if ('D' == ch)
	    {
	    updateRoutes ("Nobody's Home") ;
	    return (1) ;
	    }
	}

    if (filexists ("ROUTE.TMP"))
	{
	printf ("There is a ROUTE.TMP file in this directory. The autorouter needs to create\n") ;
	printf ("its own ROUTE.TMP, so you'll need to get rid of this one to proceed.\n") ;
	return (-1) ;
	}

    if (!(filexists ("MSG.DAT")))
	{
	printf ("Can't find MSG.DAT.  That's the big file with all your messages in it.\n") ;
	printf ("I gotta have MSG.DAT to run.\n") ;
	return (-2) ;
	}

    /* Open MSG.DAT and read all the routes out of it into ROUTE.TMP.  */

    hInFile = sopen ("MSG.DAT", O_BINARY | O_RDONLY, SH_DENYNO) ;

    if (-1 == hInFile)
	{
	printf ("Can't open MSG.DAT.  Aborting.\n") ;
	return (-3) ;
	}

    hOutFile = sopen ("ROUTE.TMP", O_CREAT | O_TEXT | O_WRONLY, SH_DENYNO,
		      S_IREAD | S_IWRITE) ;

    if (-1 == hOutFile)
	{
	printf ("Can't open ROUTE.TMP.  Aborting.\n") ;
	return (-4) ;
	}

    printf ("Reading the MSG.DAT file...\n\n") ;

    /* Set up start time & file length. */


    while (!eof (hInFile))
	if (findAPath (hInFile, fFull, fVerbose))
	    {
	    /* Copy the path until a \0, eof, or overflow.  */
	    
	    bufptr = buf ;
	    endbufptr = &buf[MAXBUF] ;

	    do
		{
		read (hInFile, &ch, 1) ;
		*bufptr = ch ;
		bufptr++ ;
		}
	    while ( (ch) && (!eof(hInFile)) && (bufptr != endbufptr) ) ;

	    /* If not eof and not filled buffer, it's a valid path.  */

	    if (eof(hInFile))
		printf ("END OF FILE.\n") ;

	    if (bufptr == endbufptr)
		printf ("BUFFER OVERFLOW.  But don't worry.\n") ;

	    if ((!eof(hInFile)) && (bufptr != endbufptr))
		{
		write (hOutFile, buf, strlen (buf)) ;
		write (hOutFile, "\n", 1) ;
		if (fVerbose)
		    printf ("ROUTE: %s\n", buf) ;
		}
	    }
    printf ("Done reading MSG.DAT.  Calculating routes...\n") ;
    close (hInFile) ;
    close (hOutFile) ;
    updateRoutes (buf) ;
    return (1) ;
}
#endif /* STANDALONE */

#endif  /* NETWORK */
