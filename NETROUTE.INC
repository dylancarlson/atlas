/* -------------------------------------------------------------------- */
/*  NETROUTE.C                    Atlas                                 */
/* -------------------------------------------------------------------- */
/*                   Stuff necessary for NETROUTE to compile as a       */
/*                   STANDALONE DOS application.                        */
/*                                                                      */
/* NOTE NOTE NOTE: THIS FILE IS INCLUDED BY NETROUTE.C WHEN COMPILING   */
/*                 AS A STANDALONE DOS APPLICATION.  IT IS NOT PART OF  */
/*                 CITADEL, AND IS NOT A COMPILABLE C PROGRAM.          */
/*                                                                      */
/* -------------------------------------------------------------------- */

#include <string.h>
#include <dos.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <direct.h>
#define cPrintf printf
#define crashout( s1 ) printf( s1 )
#define doccr() printf("\n")
#define LABELSIZE 30
#define SAMESTRING 0
#define LF (10)
#define TRUE 1
#define FALSE 0
typedef char label[31];
typedef unsigned char BOOL  ;
typedef unsigned char uchar ;
struct config {  char homepath[64];  } ;
struct config cfg ;  /* Global variable to make STANDALONE compatable. */

BOOL filexists(char *filename)
{
    return (BOOL)((access(filename, 4) == 0) ? TRUE : FALSE);
}

/* -------------------------------------------------------------------- */
/*  pause()         busy-waits N/100 seconds                            */
/* -------------------------------------------------------------------- */
void pause(register int ptime)
{
    union REGS in, out;
    register int i, j=0;
    in.h.ah=0x2C;
    intdos(&in, &out);
    i = out.h.dl;
    while(j < ptime)
    {
	in.h.ah=0x2C;
	intdos(&in, &out);
	if(out.h.dl < (uchar)i)
	    j += (100+out.h.dl)-i;
	else
	    j += out.h.dl-i;
	i = out.h.dl;
    }
}


/************************************************************************/
/*      sfgets()        reads a line from a file, given a handle        */
/************************************************************************/

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
	printf ("Disk error.") ;
	return (NULL) ;
	}
}

/* -------------------------------------------------------------------- */
/*  qtext()         Consumes quoted strings and expands escape chars    */
/* -------------------------------------------------------------------- */
int  qtext(char *buf, char *line, char end)
{
    int index = 0;
    int slash = 0;
    char chr;

    while (line[index] != '\0' && (line[index] != end || slash != 0)) 
    {
	if (slash == 0) 
	{
	    if (line[index] == '\\') 
	    {
		slash = 1;
	    } 
	    else 
	    if (line[index] == '^') 
	    {
		slash = 2;
	    }
	    else 
	    {
		*(buf++) = line[index];
	    }
	} 
	else 
	if (slash == 1) 
	{
	    switch (line[index]) 
	    {
		default:
		    *(buf++) = line[index];
		    break;
		case 'n':                       /* newline */
		    *(buf++) = '\n';
		    break;
		case 't':                       /* tab */
		    *(buf++) = '\t';
		    break;
		case 'r':                       /* carriage return */
		    *(buf++) = '\r';
		    break;
		case 'f':                       /* formfeed */
		    *(buf++) = '\f';
		    break;
		case 'b':                       /* backspace */
		    *(buf++) = '\b';
		    break;
	    }
	    slash = 0;
	} else /* if (slash == 2 ) */
	{
	    if (line[index] == '?') 
	    {
		chr = 127;
	    } 
	    else 
	    if (line[index] >= 'A' && line[index] <= 'Z') 
	    {
		chr = (char)(line[index] - 'A' + 1);
	    }
	    else 
	    if (line[index] >= 'a' && line[index] <= 'z') 
	    {
		chr = (char)(line[index] - 'a' + 1);
	    }
	    else 
	    {
		chr = line[index];
	    }

	    *(buf++) = chr;
	    slash = 0;
	}

	index++;
    }

    *buf = 0;
    return line[index] == end ? index + 1 : index;
}


/* -------------------------------------------------------------------- */
/*  parse_it()      routines to parse strings separated by white space  */
/* -------------------------------------------------------------------- */
int parse_it(char *words[], char input[])
{
/* states ofmachine... */
#define INWORD          0
#define OUTWORD         1
#define INQUOTES        2

/* characters */
#undef  TAB
#define TAB             9
#define BLANK           ' '
#define QUOTE           '\"'
#define QUOTE2          '\''
#define MXWORD         128

    int  i,state,thisword;

    input[strlen(input)+1] = 0;         /* double-null */

    for (state = OUTWORD, thisword = i = 0; input[i]; i++) 
    {
	switch (state) 
	{
	    case INWORD:
		if (isspace(input[i])) 
		{
		    input[i] = 0;
		    state = OUTWORD;
		}
		break;
	    case OUTWORD:
		if (input[i] == QUOTE || input[i] == QUOTE2) 
		{
		    state = INQUOTES;
		} else if (!isspace(input[i])) 
		{
		    state = INWORD;
		}

		/* if we are now in a string, setup, otherwise, break */

		if (state != OUTWORD) 
		{
		    if (thisword >= MXWORD)
		    {
			return thisword;
		    }

		    if (state == INWORD) 
		    {
			words[thisword++] = (input + i);
		    } else {
			words[thisword++] = (input + i + 1);
		    }
		}
		break;
	    case INQUOTES:
		i += qtext(input + i, input + i,input[i - 1]);
		state = OUTWORD;
		break;
	}
    }
    return thisword;
}
