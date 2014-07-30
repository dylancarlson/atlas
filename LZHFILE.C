/* -------------------------------------------------------------------- */
/* Archives.c                                                           */
/* LZH File handling routines                                           */
/* -------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/* Contents                                                             */
/*   lzhview()     This routine handles viewing LZH-Files               */
/* -------------------------------------------------------------------- */
/* static  char lzhview(char *filename,char verbose); */

/* -------------------------------------------------------------------- */
/*	 LZH-Files structures												*/
/* -------------------------------------------------------------------- */
struct lzhlfh {
	char unknown1[2];
	char method[5]  ;
	long csize      ;
	long fsize      ;
	int ftime       ;
	int fdate       ;
	char fattr      ;
	char unknown2   ;
	char namelen    ;
/*  char *fname     ;       * Deleted due to non-essential, called by   */
/*  int crc         ;       * Other method.                             */
};


/* -------------------------------------------------------------------- */
/*   lzhview()                                                          */
/*              Added by Turtle! (SDJ)                                  */
/* -------------------------------------------------------------------- */
char lzhview(char *filename, char verbose)
{
	FILE            *fptr;
	struct lzhlfh   header;
    char            fname[128];
	int             crc;
	char            method[7];
	char            s1[9];
	char            s2[9];
	char            ok = 1;
	char            i;
	int             num;
    int             pct;
	long            t_fsize = 0;
	long            t_csize = 0;

	outFlag = OUTOK;

	if((fptr = fopen(filename, "rb")) ==NULL)
	{
		mPrintf("\n No LZH-File: %s", filename);
		return ERROR;
	}

	doCR();
	doCR();
	mPrintf(" Contents of: %s", filename);
	doCR();
	doCR();

	fseek(fptr, 0L, SEEK_SET);
	num = 0;

    if (verbose)
    {
        mPrintf("FileName      Orignal   Packed    Pct  Date      Time      Type   CRC ");  doCR();
        mPrintf("------------  --------  --------  ---  --------  --------  -----  ----"); doCR();
    }
    else
    {
        mPrintf("FileName      Orignal   Date      Time    "); doCR();
        mPrintf("------------  --------  --------  --------"); doCR();
    }

	while (ok) {
		ok = (char)(fread(&header, sizeof(header), 1, fptr) == 1);
        if (!ok || !header.namelen) 
            break;
            
		fread(fname, header.namelen, 1, fptr);
		*(fname+header.namelen) = 0;
        normalizeString(fname);
        
        fread(&crc, sizeof(short), 1, fptr);
		for (i = 0; i < 5; i++)
            method[i] = header.method[i];
        method[5] = 0;
		num++;	/* increment total number of files */
		sprintf(s1, "%02.2d/%02d/%02d",
			(header.fdate & 0x01e0) / 32,
			(header.fdate & 0x001f),
			(header.fdate & 0xfe00) / 512 + 80);
		sprintf(s2, "%02d:%02d:%02d",
			(header.ftime & 0xfc00) / 2048,
			(header.ftime & 0x07e0) / 32,
			(header.ftime & 0x001f));
		t_fsize += header.fsize;
		t_csize += header.csize;
        
        if (verbose)
        {
            if (header.fsize == 0L) header.fsize = 1L;
            pct = (int)(100 - (((long)header.csize * 100L) / header.fsize));
            mPrintf("%-12s  %8ld  %8ld  %2d%%  %s  %s  %5s  %04X",
                     fname, header.fsize, header.csize, pct, s1, s2, method, crc);
            doCR();
        }
        else
        {
            mPrintf("%-12s  %8ld  %s  %s",
                     fname, header.fsize, s1, s2);
            doCR();
        }
        
        if((outFlag == OUTNEXT) || (outFlag == OUTSKIP)) 
        {
            fclose(fptr);
            return FALSE;
        }
        
        fseek(fptr, header.csize, SEEK_CUR);
	}
    
    if (verbose)
    {
        mPrintf("------------  --------  --------  ---"); doCR();
        mPrintf("%3d File(s)   %8ld  %8ld  %2d%%", num,t_fsize, t_csize, 100 - (char)((t_csize * 100) / t_fsize)); doCR();
    }  
    else
    {            
        mPrintf("------------  --------  "); doCR();
        mPrintf("%3d File(s)   %8ld", num, t_fsize); doCR();
    }
    
    fclose(fptr);
        
	return TRUE;
}

/* -------------------------------------------------------------------- */
/*		readlzh()  menu level .vl  HIGH level routine					*/
/* -------------------------------------------------------------------- */
void readlzh(char verbose)
{
    int i;
    label filename;

    if (changedir(roomBuf.rbdirname) == ERROR) return;
    
    getNormStr("", filename, NAMESIZE, ECHO);
             
    if (!filename[0])
        strcpy(filename, "*.lzh");

    if(!strchr(filename, '.'))
            strcat(filename, ".lzh");

    if (ambig(filename))
    {
        /* fill our directory array according to filename */
        filldirectory(filename, 0 );

        /* show contents of ambigous files */
        for (i = 0; filedir[i].entry[0] && !mAbort(FALSE) && (outFlag != OUTSKIP) &&
                (lzhview(filedir[i].entry, verbose) != ERROR); i++) ;

		if (!i) mPrintf("\n No LZH-File: %s", filename);
    }
    else
        lzhview(filename, verbose);

    sprintf(msgBuf->mbtext, "LZH-View of file %s in room %s]",
            filename, roomBuf.rbname);

    trap(msgBuf->mbtext, T_DOWNLOAD);

    changedir(cfg.homepath);
}

