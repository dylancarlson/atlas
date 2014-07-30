/* -------------------------------------------------------------------- */
/*  CONSOLE.C                Dragon Citadel                   >>IBM<<   */
/* -------------------------------------------------------------------- */
/*  This code handels the console on the IBM PC and compatible          */
/*  computers. This code will need heavy modification for use with      */
/*  other computers.                                                    */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <conio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  INPUT:                                                              */
/*  doccr()         Do CR on console, used to not scroll the window     */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/* -------------------------------------------------------------------- */
/*  OUTPUT:                                                             */
/*  fkey()          Deals with function keys from console               */
/*  KBReady()       returns TRUE if a console char is ready             */
/*  outCon()        put a character out to the console                  */
/*  cPrintf()       send formatted output to console                    */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  04/25/90    (PAT)   Created                                         */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  #defines                                                            */
/* -------------------------------------------------------------------- */
#define ALT_A   30  /* */
#define ALT_B   48  /* */
#define ALT_C   46  /* */
#define ALT_D   32  /* */
#define ALT_E   18  /* */
#define ALT_F   33
#define ALT_G   34
#define ALT_H   35
#define ALT_I   23
#define ALT_J   36
#define ALT_K   37
#define ALT_L   38  /* */
#define ALT_M   50
#define ALT_N   49
#define ALT_O   24
#define ALT_P   25  /* */
#define ALT_Q   16
#define ALT_R   19
#define ALT_S   31
#define ALT_T   20  /* */
#define ALT_U   22
#define ALT_V   47  /* */
#define ALT_W   17
#define ALT_X   45  /* */
#define ALT_Y   21
#define ALT_Z   44

#define F1      59  /* */
#define F2      60  /* */
#define F3      61  /* */
#define F4      62  /* */
#define F5      63  /* */
#define F6      64  /* */
#define F7      65  /* */
#define F8      66  /* */
#define F9      67  /* */
#define F10     68  /* */

#define ALT_F1  104
#define ALT_F2  105
#define ALT_F3  106
#define ALT_F4  107
#define ALT_F5  108
#define ALT_F6  109 /* */
#define ALT_F7  110
#define ALT_F8  111
#define ALT_F9  112
#define ALT_F10 113

#define SFT_F1  84
#define SFT_F2  85
#define SFT_F3  86
#define SFT_F4  87
#define SFT_F5  88
#define SFT_F6  89  /* */
#define SFT_F7  90
#define SFT_F8  91
#define SFT_F9  92
#define SFT_F10 93

#define PGUP    73  /* */
#define PGDN    81  /* */


/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */
BOOL    getkey = FALSE;                 /* key in buffer?               */
extern  char prtf_buff[512];

/* -------------------------------------------------------------------- */
/*  doccr()         Do CR on console, used to not scroll the window     */
/* -------------------------------------------------------------------- */
void doccr(void)
{ 
    unsigned char row, col;

    if (!console) return; 

    if (!anyEcho) return;

    readpos( &row, &col);

    if (row == (uchar)(scrollpos + 1) )
    {
        position(0,0);     /* clear screen if we hit our window */
    }

    if (row >= scrollpos)
    {
        scroll( scrollpos, 1, cfg.attr);
        position( scrollpos, 0);
    }
    else 
    {
        col=0;
        row++;
        position(row, col);
    }
}

/* -------------------------------------------------------------------- */
/*  fkey()          Deals with function keys from console               */
/* -------------------------------------------------------------------- */
void fkey(void)
{            
    char key;
    int oldIO; 
    label string;


    key = (char)getch();

    if (strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)    
    if (ConLock == TRUE && key == ALT_L &&
        strcmpi(cfg.f6pass, "disabled") != SAMESTRING)
    {
        ConLock = FALSE;

        oldIO = whichIO;
        whichIO = CONSOLE;
        onConsole = TRUE;
        update25();
        string[0] = 0;
        getNormStr("System Password", string, NAMESIZE, NO_ECHO);
        if (strcmpi(string, cfg.f6pass) != SAMESTRING)
            ConLock = TRUE;
        whichIO = (BOOL)oldIO;
        onConsole = (BOOL)(whichIO == CONSOLE);
        update25();
        givePrompt();
        return;
    }

    if (ConLock && !sysop && strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)
        return;

    switch(key)
    {
    case F1:
        drop_dtr();
        break;

    case F2:
        Initport();
        break;

    case F3:
        sysReq = (BOOL)(!sysReq);
        break;

    case F4:
        ScreenFree();
        anyEcho = (BOOL)(!anyEcho);
        break;

    case F5: 
        if  (whichIO == CONSOLE) whichIO = MODEM;
        else                     whichIO = CONSOLE;

        onConsole = (BOOL)(whichIO == CONSOLE);
        break;

    case SFT_F6:
        if (!ConLock)
            aide = (BOOL)(!aide);
        break;

    case ALT_F6:
        if (!ConLock)
            sysop = (BOOL)(!sysop);
        break;

    case F6:
        if (sysop || !ConLock)
            sysopkey = TRUE;
        break;

    case F7:
        cfg.noBells = (uchar)!cfg.noBells;
        break;

    case ALT_C:
    case F8:
        chatkey = (BOOL)(!chatkey);   /* will go into chat from main() */
        break;

    case F9:
        cfg.noChat = (uchar)!cfg.noChat;
        chatReq = FALSE;
        break;
    
    case F10:
#ifdef OLDHELP        
        help();
#else
        altF10();
#endif /* OLDHELP */
        break;

    case ALT_B:
        backout = (BOOL)(!backout);
        break;

    case ALT_D:
        debug = (BOOL)(!debug);
        break;

    case ALT_E:
        eventkey = TRUE;
        break;

    case ALT_L:
        if (cfg.f6pass[0] && strcmpi(cfg.f6pass, "f6disabled") != SAMESTRING)
          ConLock = (BOOL)(!ConLock);
        break;

    case ALT_P:
        if (printing)
        {
            printing=FALSE;
            fclose(printfile);
        }else{
            printfile=fopen(cfg.printer, "a");
            if (printfile)
            {
                printing=TRUE;
            } else {
                printing=FALSE;
                fclose(printfile);
            }
        }
        break;

    case ALT_T:
        twit = (BOOL)(!twit);
        break;

    case ALT_X:
        doCR();
        doCR();
        mPrintf("Exit to MS-DOS"); doCR();
        ExitToMsdos = TRUE;
        break;
    
    case ALT_V:
        logBuf.VERIFIED = (BOOL)(!logBuf.VERIFIED);
        break;

    case ALT_F10:
        altF10();
        break;
        
    case ALT_A:
        logBuf.lbflags.NOACCOUNT = !logBuf.lbflags.NOACCOUNT;
        break;
    
    case PGUP:
        logBuf.credits += 5;
        break;
    
    case PGDN:
        logBuf.credits -= 5;
        break;
        
    default:
        break;
    }

    update25();
}

/* -------------------------------------------------------------------- */
/*  KBReady()       returns TRUE if a console char is ready             */
/* -------------------------------------------------------------------- */
BOOL KBReady(void)
{
    int c;

    if (getkey) return(TRUE);
  
    if (kbhit())
    {
        c = getch();
 
        if (!c)
        {
            fkey();
            return(FALSE);
        }
        else ungetch(c);

        getkey = 1;
       
        return(TRUE);
    }
    else return(FALSE);
}

/* -------------------------------------------------------------------- */
/*  ciChar()        Get character from console                          */
/* -------------------------------------------------------------------- */
int ciChar(void)
{
    int c;
    
    c = getch();
    getkey = 0;
    ++received;  /* increment received char count */

    return c;
}

/* -------------------------------------------------------------------- */
/*  outCon()        put a character out to the console                  */
/* -------------------------------------------------------------------- */
void outCon(char c)
{
    unsigned char row, col;
    static   char escape = FALSE;

    if (!console) return; 

    if (c == 7   /* BELL */  && cfg.noBells)  return;
    if (c == 27 || escape) /* ESC || ANSI sequence */
    {
        escape = ansi(c);
        return;
    }
    if (c == 26) /* CT-Z */                   return;

    if (!anyEcho)  return;

    /* if we dont have carrier then count what goes to console */
    if (!gotCarrier()) transmitted++;

    if (c == '\n')                                      /* Newline */
    {
        doccr();
    }
    else
    if (c == '\r')                                      /* Return */
    {
        readpos(&row, &col);
        position(row, 0);
    }
    else 
    if (c == 7)                                         /* Bell */
    {
        putchar(c);
    }
    else
    if (c == '\b')                                      /* Backspace */
    {
        readpos(&row, &col);
        if (col == 0 && prevChar != 10)
        {
            row--;
            col = conCols;
        }
        position(row, (uchar)(col-1));
        (*charattr)(' ', ansiattr);
        position(row, (uchar)(col-1));
    } 
    else                                                /* Other Character */
    {
        readpos(&row, &col);
        (*charattr)(c, ansiattr);
        if (col == (uchar)(conCols-1))
        {
            position(row,col);
            doccr();
        }
    }
}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
void cPrintf(char *fmt, ... )
{
    register char *buf = prtf_buff;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(prtf_buff, fmt, ap);
    va_end(ap);

    while(*buf) {
        outCon(*buf++);
    }
}

/* -------------------------------------------------------------------- */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */
void cCPrintf(char *fmt, ... )
{
    va_list ap;
    int i;
    uchar row, col;

    va_start(ap, fmt);
    vsprintf(prtf_buff, fmt, ap);
    va_end(ap);

    i = (conCols - strlen(prtf_buff)) / 2;

    strrev(prtf_buff);

    while(i--)
    {
        strcat(prtf_buff, " ");
    }

    strrev(prtf_buff);

    readpos(&row, &col);
    (*stringattr)(row, prtf_buff, cfg.attr);
    /* (*stringattr)(wherey()-1, prtf_buff, cfg.attr); */
}

/* -------------------------------------------------------------------- */
/*  ctrl_c()        Used to catch CTRL-Cs                               */
/* -------------------------------------------------------------------- */
void ctrl_c(void)
{
    uchar row, col;

    signal(SIGINT, ctrl_c);
    readpos( &row, &col);
    position((uchar)(row-1), 19);
    ungetch('\r');
    getkey = TRUE;
}
 
/* -------------------------------------------------------------------- */
/*  cGets()     get a string from the console.                          */
/* -------------------------------------------------------------------- */
void cGets(char *buff)
{
    cgets(buff);
}

