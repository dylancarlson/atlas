/************************************************************************/
/*  MCFILE.C                      Atlas                        14Jan92  */
/*               File handling routines for MultiCitadel                */
/*                                                                      */
/* Since MultiCitadel capacity insists that a section of a file may not */
/* be available at any one time, the file handling routines need to be  */
/* flexable enough to deal with this.  This means you should never      */
/* denyreadwrite on a file.  If you need control of it, lock it.  In    */
/* addition to providing multicit-compatable file-handle-based I/O      */
/* functions, this module also provides the file RAM caching routines   */
/* which accelerate cool stuff with NODES.CIT and ROUTE.CIT lots.       */
/*                                                                      */
/************************************************************************/

#include <string.h>
#include <malloc.h>
#include <dos.h>
#include <errno.h>
#include <io.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/************************************************************************/
/*                          contents                                    */
/*                                                                      */
/*  sfgets()            reads a line from a file, given a handle.       */
/*  msfgets()           reads a line from a potentially cached file.    */
/*  mopen()             opens a file and caches it, if possible.        */
/*  mclose()            closes a cached file.                           */
/*  mmalloc()           mallocs memory, freeing caches, if necessary.   */
/*                      Should be used instead of malloc().             */
/************************************************************************/

/************************************************************************/
/*      sfgets()        reads a line from a file, given a handle        */
/************************************************************************/

/*
 * sfgets() reads a string from the input handle file and stores it in str.
 * Characters are read from the current file position up to the first newline
 * character ('\n'), up to the end of the file, or until the number of
 * characters read is equal to n-1 (a null goes into the last position).
 * The result is stored in str, and a \0 is appended.   The newline
 * character, if read, IS included in str.  If n is equal to 1, str is empty.
 *
 */

char *sfgets (char *str, int n, int hFile)
{
    register int   r ;
    register int   pos = 0 ;
    register char  tries = 0 ;
	     char  ch ;
	     char  error = FALSE ;

    if (n == 1)
	{
	str[0] = '\0' ;
	return (str) ;
	}

    for (;;)
	{
	r = read (hFile, &ch, 1) ;

	if (0 == r)       /* EOF */
	    if (pos)      /* If any characters gotten, pass them back.  */
		break ;
	    else                 /* If no characters, it's an error.   */
		return (NULL) ;

	if (-1 == r)                            /* Problem with the file. */
	    {
	    ++tries ;
	    if (tries > 5)                      /* Timeout. */
		{
		error = TRUE ;                  /* It's a serious error. */
		break ;
		}

	    pause (100) ;                       /* Pause 1 second and */
	    continue ;                          /* try again.         */
	    }

	str[pos] = ch ;
	++pos ;

	if ( (LF == ch) || (pos == n-1) )       /* Two reasons to quit. */
	    break ;
	}

    if (!error)
	{
	str[pos] = '\0' ;
	return (str) ;
	}
    else
	{
	doCR() ;
	mPrintf ("A timeout error has occured in sfgets.") ;  doCR() ;
	mPrintf ("Please tell the sysop about the error and the situation.") ;  doCR() ;
	doCR() ;
	return (NULL) ;
	}
}

#ifdef FUCKOFFFORNOW

/* timestamp takes a fullname and returns the file's timestamp, or -1 if   */
/* error.                                                                  */

long timestamp (char *fullname)
{
    struct stat filestat ;

    if (0 == stat (fullname, &filestat))
	{
	return ((long) &filestat.st_atime) ;
	}
    return (-1) ;
}


/* The file caching functions depend on a small global array of pointers.  */
/* These pointers identify file headers on the global heap, which in turn  */
/* point to RAM copies of files.                                           */

/* fullname is the file's full name.  timestamp is its timestamp.          */
/* lastAccess is the Current Time of its last access, used to decide which */
/* cached file to toss out if necessary.  keywords is AN ALPHABETIZED      */
/* VERSION of the double-NULL-terminated string seive through which this   */
/* file was originally loaded.  text is the cached file, which ends with a */
/* single NULL.                                                            */

struct memFileTAG
{
    char fullname[128] ;
    long timestamp ;
    long lastAccess ;
    char *keywords ;
    char *text ;
} memFile ;

/* ptrmFile is a variable-length array of long pointers which point to     */
/* memFile structures.  The last ptrmFile pointer is NULL.                 */

memFile *ptrmFile = NULL ;

/***************************************************************************/
/*                                                                         */
/* mopen() opens a TEXT file and caches it, if possible.  mopen() should   */
/* be used when you just want to examine a text file, and need neither     */
/* exclusive access during your visit, nor write capacity.  Since mopen()  */
/* keeps files cached until they are changed or the memory is needed, it   */
/* greatly enhances the speed of text file access.                         */
/*                                                                         */
/* PARAMETERS:                                                             */
/*                                                                         */
/* fullname: Complete filename of the file.                                */
/*                                                                         */
/* keywords:  TRIPLE-NULL terminated string containing single-null         */
/*            terminated "first words" in separated from one another by    */
/*            single nulls.  Did you catch that?  For example:             */
/*            #ROOM\0Bob's Room\0\0#NODE\0\0\0 would cause any line        */
/*            starting with either #NODE, or #ROOM in field one, "Bob's    */
/*            Room" in field two to be cached.  If keywords is NULL, cache */                             
/*            the entire freaking file.                                    */
/*                                                                         */
/* fOthersOK: This is TRUE if you will accept a "superset" of your         */
/*            keywords.  IF TRUE, an existing cached file may accomodate   */
/*            you, but you will have to check if the data you get back are */
/*            actually keywords.  IF FALSE, you will get the fastest       */
/*            possible cache (once it's loaded off disk), and need not     */
/*            check if each line is a keyword.                             */
/*                                                                         */
/***************************************************************************/

int mopen (char *fullname, char *keywords, BOOL fOthersOK)
{
    int i ;

    /* Scan the mFile array to see if this file has been opened before.    */
    /* Since the mFile array is initialized with zeroes, it's ok to refer  */
    /* to data fields.  Just expect them to be full of zeros if invalid.   */

    for (i = 0; ptrmFile[i]; i++)
	if (SAMESTRING == stricmp (ptrmFile[i]->fullname, fullname))
	    break ;

    /* If the file is in the cache, check that the timestamp has not       */
    /* changed.                                                            */

    if (!ptrmFile[i])                            /* File is in the cache.  */
	{
	if (mFile[i]->timestamp == timestamp (fullname))
........blah.
	}
}

/***************************************************************************/
/*                                                                         */
/* msfgets() reads a line from a potentially cached file.  It behaves just */
/* like sfgets(), which behaves almost just like fgets().                  */
/*                                                                         */
/***************************************************************************/

char *msfgets (char *str, int n, int hFile)
{
}

/***************************************************************************/
/*                                                                         */
/* mclose() closes a potentially cached file.                              */
/*                                                                         */
/***************************************************************************/

int mclose (int handle)
{
}

/***************************************************************************/
/*                                                                         */
/* mmalloc() satisfies memory allocations, pushing cached files out of     */
/* memory if necessary.                                                    */
/*                                                                         */
/***************************************************************************/



void *mmalloc (size_t size)
{
}



#endif /* FUCKOFFFORNOW */
