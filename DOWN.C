/* -------------------------------------------------------------------- */
/*  DOWN.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  Code to bring citadel down                                          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
/* MSC */
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* DragCit */
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/*  exitcitadel()   Done with cit, time to leave                        */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  08/04/90    (PAT)   File created                                    */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data definitions                                             */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  crashout()      Fatal system error                                  */
/* -------------------------------------------------------------------- */
void crashout(char *message)
{
    FILE *fd;           /* Record some crash data */

    Hangup();

    fcloseall();

    fd = fopen("crash.cit", "w");
    fprintf(fd, message);
    fclose(fd);

    writeTables();

    cfg.attr = 7;   /* exit with white letters */

    position(0,0);
    cPrintf("Fatal System Crash: %s\n", message);

    drop_dtr();

    portExit();

    _ffree((void far *)msgTab1);
    _ffree((void far *)msgTab2);
/*  _ffree((void far *)msgTab3); */
    _ffree((void far *)msgTab4);
    _ffree((void far *)msgTab5);
    _ffree((void far *)msgTab6);
    _ffree((void far *)msgTab7);
    _ffree((void far *)msgTab8);
/*  _ffree((void far *)msgTab9); */

    _ffree((void far *)roomPos  );
    _ffree((void far *)eventlist);
    _ffree((void far *)msgBuf2  );
    _ffree((void far *)msgBuf   );
    _ffree((void far *)roomTab  );
    _ffree((void far *)extrn    );
    _ffree((void far *)hallBuf  );
    _ffree((void far *)edit     );
    _ffree((void far *)doors    );
    _ffree((void far *)talleyBuf);
    _ffree((void far *)lBuf2    );
    
    exit(199);
}

/* -------------------------------------------------------------------- */
/*  exitcitadel()   Done with cit, time to leave                        */
/* -------------------------------------------------------------------- */
void exitcitadel(void)
{
    if (!slv_door)
        drop_dtr();   /* turn DTR off */

    putGroup();       /* save group table */
    putHall();        /* save hall table  */

    writeTables(); 

    trap("Citadel Terminated", T_SYSOP);

    /* close all files */
    fcloseall();

    cfg.attr = 7;   /* exit with white letters */
    cls();

    portExit();

    _ffree((void far *)msgTab1);
    _ffree((void far *)msgTab2);
/*  _ffree((void far *)msgTab3); */
    _ffree((void far *)msgTab4);
    _ffree((void far *)msgTab5);
    _ffree((void far *)msgTab6);
    _ffree((void far *)msgTab7);
    _ffree((void far *)msgTab8);
/*  _ffree((void far *)msgTab9); */

    _ffree((void far *)roomPos  );
    _ffree((void far *)eventlist);
    _ffree((void far *)msgBuf2  );
    _ffree((void far *)msgBuf   );
    _ffree((void far *)roomTab  );
    _ffree((void far *)extrn    );
    _ffree((void far *)hallBuf  );
    _ffree((void far *)edit     );
    _ffree((void far *)doors    );
    _ffree((void far *)talleyBuf);
    _ffree((void far *)lBuf2    );

    if (gmode() != 7)
    {
        outp(0x3d9,0);
    }

    exit(return_code);
}


