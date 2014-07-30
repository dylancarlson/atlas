/* -------------------------------------------------------------------- */
/*  MSGCFG.C                 Dragon Citadel                             */
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
/*  msgInit()       builds message table from msg.dat                   */
/*  zapMsgFile()    initializes msg.dat                                 */
/*  slidemsgTab()   frees slots at beginning of msg table               */
/*  buildcopies()   copies appropriate msg index members                */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  02/26/91    (PAT)   Rearanged message code for overlays.            */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  msgInit()       sets up lowId, highId, cfg.catSector and cfg.catChar*/
/*                  by scanning over message.buf                        */
/* -------------------------------------------------------------------- */
void msgInit(void)
{
    ulong first, here;
    int makeroom;
    int skipcounter = 0;   /* maybe need to skip a few . Dont put them in
                              message index */
    int slot;
    int result;

    doccr(); doccr();
    cPrintf("Building message table"); doccr();

    /* start at the beginning */
    fseek(msgfl, 0l, 0);

    getMessage();

    /* get the ID# */
    sscanf(msgBuf->mbId, "%ld", &first);
    
    cPrintf("Message #%lu\r", first);

    /* put the index in its place */
    /* mark first entry of index as a reference point */

    cfg.mtoldest = first;
    
    indexmessage(first);

    cfg.newest = cfg.oldest = first;

    cfg.catLoc = ftell(msgfl);

    for(;;)  /* while (TRUE) */
    {
        /* result is zero if getmessage encounters 10000 blank characters */
        result = getMessage();

        sscanf(msgBuf->mbId, "%ld", &here);
                               
        if ((here == first) || !result) break;

        if (!(here % 10))
            cPrintf("Message #%lu\r", here);

        /* find highest and lowest message IDs: */
        /* >here< is the dip pholks             */
        if (here < cfg.oldest)
        {
            slot = ( indexslot(cfg.newest) + 1 );

            makeroom = (int)(cfg.mtoldest - here);

            /* check to see if our table can hold  remaining msgs */
            if (cfg.nmessages < (makeroom + slot))
            {
                skipcounter = (makeroom + slot) - cfg.nmessages;

                slidemsgTab(makeroom - skipcounter);

                cfg.mtoldest = (here + (ulong)skipcounter);
 
            }
            /* nmessages can handle it.. Just make room */
            else
            {
                slidemsgTab(makeroom);
                cfg.mtoldest = here;
            }
            cfg.oldest = here;
        }

        if (here > cfg.newest)
        {
            cfg.newest = here;

            /* read rest of message in and remember where it ends,      */
            /* in case it turns out to be the last message              */
            /* in which case, that's where to start writing next message*/
            getMsgStr(msgBuf->mbtext, MAXTEXT); 

            cfg.catLoc = ftell(msgfl);
        }

        /* check to see if our table is big enough to handle it */
        if ( (int)(here - cfg.mtoldest) >= cfg.nmessages)
        {
            crunchmsgTab(1);
        }

        if (skipcounter) 
        {
            skipcounter--;
        }
        else
        {
            indexmessage(here);
        }
    }    
    /* buildcopies(); */
}

/* -------------------------------------------------------------------- */
/*  zapMsgFl()  initializes message.buf                                 */
/* -------------------------------------------------------------------- */
zapMsgFile()
{
    int i;
    unsigned sect;
    char buff[1024];

    /* Clear it out just in case */
    for (i = 0;  i < 1024;  i++) buff[i] = 0;

    /* put null message in first sector... */
    buff[0]  = 0xFF; /*                                   */
    buff[1]  = DUMP; /*  \  To the dump                   */
    buff[2]  = '\0'; /*  /  Attribute                     */
    buff[3]  =  '1'; /*  >                                */
    buff[4]  = '\0'; /*  \  Message ID "1" MS-DOS style   */
    buff[5]  =  'M'; /*  /         Null messsage          */
    buff[6]  = '\0'; /*                                   */
                                                  
    cfg.newest = cfg.oldest = 1l;

    cfg.catLoc = 7l;

    if (fwrite(buff, 1024, 1, msgfl) != 1)
    {
        cPrintf("zapMsgFil: write failed"); doccr();
    }

    for (i = 0;  i < 7;  i++) buff[i] = 0;

    doccr();  doccr();
    cPrintf("MESSAGEK=%d", cfg.messagek);  doccr();
    for (sect = 1;  sect < cfg.messagek;  sect++)
    {
        cPrintf("Clearing block %u\r", sect);
        if (fwrite(buff, 1024, 1, msgfl) != 1)
        {
            cPrintf("zapMsgFil: write failed");  doccr();
        }
    }
    return TRUE;
}

/************************************************************************/
/*      slidemsgTab() frees >howmany< slots at the beginning of the     */
/*      message table.                                                  */
/************************************************************************/
void slidemsgTab(int howmany)
{
    uint numnuked;
   
    numnuked = cfg.nmessages - howmany;
   
    _fmemmove(&msgTab1[howmany], msgTab1,
            (uint)(numnuked * (sizeof(*msgTab1)) ));
    _fmemmove(&msgTab2[howmany], msgTab2,
            (uint)(numnuked * (sizeof(*msgTab2)) ));
/*  _fmemmove(&msgTab3[howmany], msgTab3,
            (uint)(numnuked * (sizeof(*msgTab3)) )); */
    _fmemmove(&msgTab4[howmany], msgTab4,
            (uint)(numnuked * (sizeof(*msgTab4)) ));
    _fmemmove(&msgTab5[howmany], msgTab5,
            (uint)(numnuked * (sizeof(*msgTab5)) ));
    _fmemmove(&msgTab6[howmany], msgTab6,
            (uint)(numnuked * (sizeof(*msgTab6)) ));
    _fmemmove(&msgTab7[howmany], msgTab7,
            (uint)(numnuked * (sizeof(*msgTab7)) ));
    _fmemmove(&msgTab8[howmany], msgTab8,
            (uint)(numnuked * (sizeof(*msgTab8)) ));
/*  _fmemmove(&msgTab9[howmany], msgTab9,
            (uint)(numnuked * (sizeof(*msgTab9)) )); */
}

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  slidemsgTab()   frees slots at beginning of msg table               */
/* -------------------------------------------------------------------- */
void slidemsgTab(int howmany)
{
    hmemcpy(&msgTab[howmany], &msgTab[0],(long)
      ((long)( (long)cfg.nmessages - (long)howmany) * (long)(sizeof(*msgTab)) )
    );
}
#endif

#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  buildcopies()   copies appropriate msg index members                */
/* -------------------------------------------------------------------- */
void buildcopies(void)
{
    int i;

    for( i = 0; i < (int)sizetable(); ++i)
    {
        if (msgTab1[i].mtmsgflags.COPY)
        {
        /*  if ((int)msgTab3[i].mtoffset <= i) */
            if ((int)msgTab8[i].mtomesg <= i)
            {
              /*  copyindex( i, (i - msgTab3[i].mtoffset)); */
                  copyindex( i, (i - msgTab8[i].mtomesg)); 
            }
        }
    }
}

#endif
