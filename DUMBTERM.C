/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#ifdef TERM

#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

BOOL termCarr = FALSE;
void displayTerm(void);

int gotCarrier(void)
{
    return termCarr;
}

extern void         RAISE_DTR(void);

void Hangup(void)
{
    termCarr = FALSE;
    RAISE_DTR();
}

int carrier(void)
{
    if (onConsole)
    {
        return FALSE;
    }
    
    if (!termCarr)
    {
        if (MIReady())
            termCarr = TRUE;
    }
    
    if (termCarr != modStat)                        /* carrier changed   */
    {
        if (termCarr)                               /* new carrier present,*/
        {                                           /* detect baud rate    */
            baud(cfg.initbaud);
            update25();
            carrdetect();
            outstring("HJ");
            return(TRUE);
        } 
        else                                        /* Carrier droped? */
        {
            carrloss();
            outstring("HJPress any key to start system...");
            
            displayTerm();
            
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

int whatFile    = 0;

void displayTerm(void)
{
    char    path[80];
    char    line[128];
    FILE    *fl;
    time_t  t;
    
    do
    {
        whatFile++;
        
        sprintf(path, "%s\\dterm%d.txt", cfg.helppath, whatFile);
        if ((fl = fopen(path, "rt")) == NULL)
        {
            whatFile = 0;
            continue;
        }
        
        outstring("HJ");
        cls();
        while (fgets(line, 126, fl) != NULL)
        {
            line[strlen(line)-1] = 0;
            outstring(line);
            outstring("\r\n");
            cPrintf(line); doccr();
        }
           
        fclose(fl);
        
        t = time(NULL);
        while (t+20 > time(NULL) && !MIReady() && !KBReady())
            ;
    }
    while(!MIReady() && !KBReady());
    
    if (KBReady())
        ciChar();
}

#endif

