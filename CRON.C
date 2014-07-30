/* -------------------------------------------------------------------- */
/*  CRON.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*  This file contains all the code to deal with the cron events        */
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
/*                                                                      */
/*  readcron()	    reads cron.cit values into events structure 	*/
/*  writecrontab()  updates cron.tab entries				*/
/*  do_cron()       called when the system is ready to do an event      */
/*  cando_event()   Can we do this event?                               */
/*  do_event()      Actualy do this event                               */
/*  list_event()    List all events                                     */
/*  cron_commands() Sysop Fn: Cron commands                             */
/*  zap_event()     Zap an event out of the cron list                   */
/*  reset_event()   Reset an even so that it has not been done          */
/*  done_event()    Set event so it seems to have been done             */
/*  did_net()       Set all events for a node to done                   */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  05/07/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code. Also added F6CAll Done               */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static  void do_event(int evnt);
static  void zap_event(void );
static  void reset_event(void );
static  void done_event(void );
static  void force_event(void);
static  void next_event_set(void);

static struct event events[MAXCRON];
static int on_event = 0;
static int numevents= 0;
static char *event_0_equals_none = "event (0 = None)";

/* -------------------------------------------------------------------- */
/*  External data                                                       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*      readcron()     reads cron.cit values into events structure      */
/* -------------------------------------------------------------------- */
void readcron(void)
{                          
    FILE *fBuf;
    char line[90];
    char *words[256];
    int  i, j, k, l, count;
    int cronslot = ERROR;
    int hour;
#ifdef CRON
    struct event tabentry;
    long t;
#endif

   
    /* move to home-path */
    changedir(cfg.homepath);

    if ((fBuf = fopen("cron.cit", "r")) == NULL)  /* ASCII mode */
    {  
	cPrintf("Can't find CRON.CIT!"); doccr();
        exit(200);
    }

    while (fgets(line, 90, fBuf) != NULL)
    {
        if (line[0] != '#')  continue;

        count = parse_it( words, line);

        for (i = 0; cronkeywords[i] != NULL; i++)
        {
            if (strcmpi(words[0], cronkeywords[i]) == SAMESTRING)
            {
                break;
            }
        }

        switch(i)
        {
        case CR_DAYS:              
            if (cronslot == ERROR)  break;

            /* init days */
            for ( j = 0; j < 7; j++ )
               events[cronslot].e_days[j] = 0;

            for (j = 1; j < count; j++)
            {
                for (k = 0; daykeywords[k] != NULL; k++)
                {
                    if (strcmpi(words[j], daykeywords[k]) == SAMESTRING)
                    {
                        break;
                    }
                }
                if (k < 7)
                    events[cronslot].e_days[k] = TRUE;
                else if (k == 7)  /* any */
                {
                    for ( l = 0; l < MAXGROUPS; ++l)
                        events[cronslot].e_days[l] = TRUE;
                }
                else
                {
                    doccr();
		    cPrintf("CRON.CIT - Warning: Unknown day %s ", words[j]);
                    doccr();
                }
            }
            break;

        case CR_DO:
            cronslot = (cronslot == ERROR) ? 0 : (cronslot + 1);

            if (cronslot > MAXCRON)
            {
                doccr();
		illegal("CRON.CIT - too many entries");
	    }

	    events[cronslot].e_type = ERROR;
            for (k = 0; crontypes[k] != NULL; k++)
            {
              if (strcmpi(words[1], crontypes[k]) == SAMESTRING)
		events[cronslot].e_type = (char) k;
	    }

	    if (ERROR == events[cronslot].e_type)
		cPrintf("CRON.CIT - unknown event type %s\n", words[1]);

            strcpy(events[cronslot].e_str, words[2]);
            events[cronslot].l_sucess  = (long)0;
            events[cronslot].l_try     = (long)0;
            break;
            
        case CR_HOURS:             
            if (cronslot == ERROR)  break;

            /* init hours */
            for ( j = 0; j < 24; j++ )
                events[cronslot].e_hours[j]   = 0;

            for (j = 1; j < count; j++)
            {
                if (strcmpi(words[j], "Any") == SAMESTRING)
                {
                    for (l = 0; l < 24; ++l)
                        events[cronslot].e_hours[l] = TRUE;
                }
                else
                {
                    hour = atoi(words[j]);

                    if ( hour > 23 || hour < 0) 
                    {
                        doccr();
			cPrintf("CRON.CIT - Warning: Invalid hour %d ", hour);
                        doccr();
                    }
                    else
                    {
                       events[cronslot].e_hours[hour] = TRUE;
                    }
                }
            }
            break;

        case CR_REDO:
            if (cronslot == ERROR)  break;
            
            events[cronslot].e_redo = atoi(words[1]);
            break;

        case CR_RETRY:
            if (cronslot == ERROR)  break;
            
            events[cronslot].e_retry = atoi(words[1]);
            break;

        default:
	    cPrintf("CRON.CIT - Warning: Unknown variable %s", words[0]);
            doccr();
            break;
        }
    }
    fclose(fBuf);

    numevents = cronslot;

    /* these defaults apply if we have no record in cron.tab of the event,
       or if cron.tab does not exist */
    for (i=0; i<MAXCRON; i++)
    {
        events[i].l_sucess  = (long)0;
        events[i].l_try     = (long)0;
    }

#ifdef CRON

    /* load cron.tab -- dcf 4/92 */

    if ((fBuf = fopen("cron.tab", "r")) != NULL)
    {
	while (!feof(fBuf))
	{
	    if (!(fgets(line, 90, fBuf)))
		break;
	    count = parse_it(words, line);
	    if (count != 4)
		continue;

	    tabentry.e_type = ERROR;
	    for (k = 0; crontypes[k] != NULL; k++)
	    {
		  if (stricmp(words[0], crontypes[k]) == SAMESTRING)
		    tabentry.e_type = (char) k;
	    }
	    strcpy(tabentry.e_str, words[1]);

	    tabentry.l_sucess = atol(words[2]);
	    tabentry.l_try = atol(words[3]);

	    /* if any time in this file is ahead of the current clock
	       setting, then invalidate the whole thing */

	    if ((tabentry.l_sucess > time(&t)) || (tabentry.l_try > time(&t)))
	    {
		cPrintf("Time in CRON.TAB is past current clock time.\n");
		cPrintf("CRON.TAB entries are being ignored.\n");
		for (i = 0; i <= numevents; i++)
		{
		    events[i].l_sucess = (long) 0;
		    events[i].l_try = (long) 0;
		}
		break;
	    }

	    for (i = 0; i <= numevents; i++)
		if (!(stricmp(events[i].e_str, tabentry.e_str)) &&
		    (events[i].e_type == tabentry.e_type))
		{
		    events[i].l_sucess = tabentry.l_sucess;
		    events[i].l_try = tabentry.l_try;
		}

	}

	fclose(fBuf);
	unlink("cron.tab");
    }

#endif
}

/* -------------------------------------------------------------------- */
/* writecrontab()							*/
/* -------------------------------------------------------------------- */

#ifdef CRON
void writecrontab(void)
{
    FILE *fp;
    int i;

    if (!(fp = fopen("cron.tab", "w")))
    {
	cPrintf("Couldn't open CRON.TAB\n");
	perror("CRON.TAB");
	return;
    }

    for (i = 0; i <= numevents; i++)
	switch (events[i].e_type)
	{
	case CR_SHELL_1:
	case CR_SHELL_2:
	case CR_NET:
	    fprintf(fp, "%s \"%s\" %ld %ld\n", crontypes[events[i].e_type],
		events[i].e_str, events[i].l_sucess, events[i].l_try);
	}

    fclose(fp);
}
#endif

/* -------------------------------------------------------------------- */
/*  do_cron()       called when the system is ready to do an event      */
/* -------------------------------------------------------------------- */
int do_cron(int why_called)
{
    int was_event, done;

    why_called = why_called; /* to prevent a -W3 warning, the varible will
                              be used latter */
    was_event = on_event;
    done = FALSE;

    do
    {
        if (cando_event(on_event))
        {
            cls();
            do_event(on_event);
            done = TRUE;  
        }

        on_event = on_event > numevents ? 0 : on_event + 1;
    }
    while(!done && on_event != was_event);  

    if (!done)
    {
        if (debug) cPrintf("No Job> ");
        Initport();
        return FALSE;
    }

    doCR();
    return TRUE;  
}

/* -------------------------------------------------------------------- */
/*  cando_event()   Can we do this event?                               */
/* -------------------------------------------------------------------- */
int cando_event(int evnt)
{
    long l;

    /* not a valid (posible zaped) event */
    if (events[evnt].e_type == ERROR)
        return FALSE;

    /* not right time || day */
    if (!events[evnt].e_hours[hour()] || !events[evnt].e_days[dayofweek()])
        return FALSE;

    /* already done, wait a little longer */
    if ((time(&l) - events[evnt].l_sucess)/(long)60 < (long)events[evnt].e_redo)
        return FALSE;  
    
    /* didnt work, give it time to get un-busy */
    if ((time(&l) - events[evnt].l_try)/(long)60 < (long)events[evnt].e_retry)
        return FALSE;  

    return TRUE;
}



/* -------------------------------------------------------------------- */
/*  list_event()    List all events                                     */
/*  This function borrowed from Acit.                                   */
/* -------------------------------------------------------------------- */
void list_event(void)
{
    int i;
    char dtstr[20];
  
    termCap(TERM_BOLD);
    mPrintf(" ##   " "   Type   " "           String     "
            "    Redo  Retry   Succeeded    Attempted");
    termCap(TERM_NORMAL);
    doCR();
  
    for (i=0; i<=numevents; i++)
    {
        if (events[i].e_type != ERROR) 
        {
            mPrintf(" %02d%c%c %10s%22s%8d%7d", i+1,
                on_event == i  ? '<' : ' ',
                cando_event(i) ? '+' : 
                    (events[i].l_sucess != events[i].l_try) ? '-' : ' ',
                crontypes[events[i].e_type], events[i].e_str,
                events[i].e_redo, events[i].e_retry);
            if (events[i].l_sucess)
            {
                strftime(dtstr, 19, "  %b%D %H:%M", events[i].l_sucess);
                mPrintf("%s", dtstr);
            }
            else
            {
                mPrintf("      N/A    ");
            }
            if (events[i].l_try)
            {
                strftime(dtstr, 19, "  %b%D %H:%M", events[i].l_try);
                mPrintf("%s", dtstr);
            }
            else
            {
                mPrintf("      N/A");
            }
            doCR();
        }
    }
}


#ifdef GOODBYE
/* -------------------------------------------------------------------- */
/*  list_event()    List all events                                     */
/* -------------------------------------------------------------------- */
void list_event(void)
{
    int i;
    char dtstr[20];
  
    mPrintf(" ##        Type                String  Redo Retry Last");
    doCR();
  
    for (i=0; i<=numevents; i++)
    {
        if (events[i].e_type != ERROR) 
        {
            mPrintf(" %02d%c%c%10s, %20s, %4d, %4d ", i+1,
                on_event == i ? '*' : ' ', cando_event(i) ? '+' : ' ', 
                crontypes[events[i].e_type], events[i].e_str, 
                events[i].e_redo, events[i].e_retry);
            if (events[i].l_try)
            {
                strftime(dtstr, 19, "%X %y%b%D", events[i].l_try);
                mPrintf("%s", dtstr);
            }
            else
            {
                mPrintf("N/A");
            }
            doCR();
        }
    }
}

#endif

/* -------------------------------------------------------------------- */
/*  cron_commands() Sysop Fn: Cron commands                             */
/* -------------------------------------------------------------------- */
void cron_commands(void)
{
    int i;
    char ich;
    
    switch (toupper( ich=(char)iCharNE() ))
    {
    case 'A':
        mPrintf("All Done\n ");
        doCR();
        mPrintf("Setting all events to done...");
        for (i=0; i<MAXCRON; i++)
        {
            events[i].l_sucess  = time(NULL);
            events[i].l_try     = time(NULL);
        }
        doCR();
        break;
    case 'D':
        mPrintf("Done event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        done_event();
        break;
    case 'E':
        mPrintf("Enter Cron file\n ");
        readcron();
        break;
    case 'L':
        mPrintf("List events"); doCR(); doCR();
        list_event();
        break;
    case 'R':
        mPrintf("Reset event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        reset_event();
        break;
    case 'Z':
        mPrintf("Zap event\n ");
        if (numevents == ERROR)
        {
            mPrintf("\n NO CRON EVENTS!\n ");
            return;
        }
        zap_event();
        break;
    case 'N':
        mPrintf("Next event set\n\n");
        next_event_set();
        break;
    case 'F':
        mPrintf("Force event\n ");
        force_event();
        break;
    case '?':
        oChar('?');
        doCR();
        doCR();
        mPrintf(" 3A0>ll done\n 3D0>one event\n 3E0>nter Cron file\n ");
        mPrintf("3F0>orce event\n 3L0>ist event\n 3N0>ext event set\n 3R0>eset event\n 3Z0>ap event\n 3?0> -- this menu\n ");
        break;
    default:
        oChar(ich);
        if (!expert)  mPrintf("\n '?' for menu.\n "  );
        else          mPrintf(" ?\n "                );
        break;
    }
}  

/* -------------------------------------------------------------------- */
/*  zap_event()     Zap an event out of the cron list                   */
/* -------------------------------------------------------------------- */
static void zap_event(void)
{
    int i;

    i = (int)getNumber(event_0_equals_none, 0L, (long)numevents+1, (long)ERROR);
    if (i == ERROR || i == 0) return;
    events[i-1].e_type = ERROR;
}

/* -------------------------------------------------------------------- */
/*  reset_event()   Reset an even so that it has not been done          */
/* -------------------------------------------------------------------- */
static void reset_event(void)
{
    int i;

    i = (int)getNumber(event_0_equals_none, 0L, (long)numevents+1, (long)ERROR);
    if (i == ERROR || i == 0) return;
    events[i-1].l_sucess = 0L;
    events[i-1].l_try    = 0L;
}

/* -------------------------------------------------------------------- */
/*  done_event()    Set event so it seems to have been done             */
/* -------------------------------------------------------------------- */
static void done_event(void)
{
    int i;
    long l, time();

    i = (int)getNumber(event_0_equals_none, 0L, (long)numevents+1, (long)ERROR);
    if (i == ERROR || i == 0) return;
    events[i-1].l_sucess = time(&l);
    events[i-1].l_try    = time(&l);
}

/* -------------------------------------------------------------------- */
/*  did_net()       Set all events for a node to done                   */
/* -------------------------------------------------------------------- */
void did_net(char *callnode)
{
    int i;
    long l, time();
  
    for (i=0; i<=numevents; i++)
    {
        if( strcmpi(events[i].e_str, callnode) == SAMESTRING
                && events[i].e_type == CR_NET)
        {
            events[i].l_sucess = time(&l);
            events[i].l_try    = time(&l);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  do_event()	    Actually do this event				*/
/* -------------------------------------------------------------------- */
static void do_event(int evnt)
{
    long time(), l;

    switch(events[evnt].e_type)
    {
    case CR_SHELL_1:
    case CR_SHELL_2:
        cPrintf("SHELL: \"%s\"", events[evnt].e_str);
        if (changedir(cfg.aplpath) == ERROR)
        {
            cPrintf("  -- Can't find application directory.\n\n");
            changedir(cfg.homepath);
            return;
        }
        apsystem(events[evnt].e_str);
        changedir(cfg.homepath);
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        Hangup();
        postEvent("SHELL: %s", events[evnt].e_str);
        break;

    case CR_CHAT_OFF:
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        cfg.noChat = TRUE;
        cPrintf("CHAT OFF");
        postEvent("CHAT OFF");
        break;
        
    case CR_CHAT_ON:
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        cfg.noChat = FALSE;
        cPrintf("CHAT ON");
        postEvent("CHAT ON");
        break;
        
    case CR_SHUTDOWN:    
        events[evnt].l_sucess = time(&l);
        events[evnt].l_try    = time(&l);
        cPrintf("SHUTDOWN CITADEL, ERRORLEVEL %s", events[evnt].e_str);
        return_code = atoi(events[evnt].e_str);
        ExitToMsdos = TRUE;
        break;
        
#ifdef NETWORK
    case CR_NET:
        cPrintf("NETWORK: with \"%s\"", events[evnt].e_str);
        if (net_callout(events[evnt].e_str))
        {
            did_net(events[evnt].e_str);
            postEvent("Net-Callout to %s, %d new", events[evnt].e_str, entered);
        }
        else
        {
            postEvent("Net-Callout to %s, failed", events[evnt].e_str, entered);
        }
        events[evnt].l_try       = time(&l);
        break;
#endif

    default:
        cPrintf(" Unknown event type %d, slot %d\n ", events[evnt].e_type, evnt);
        break;  
    }
}

/* -------------------------------------------------------------------- */
/*  force_event()   Force an event to occur                             */
/* -------------------------------------------------------------------- */
static void force_event(void)
{
    int i;

    i = (int)getNumber(event_0_equals_none, 0L, (long)numevents+1, (long)ERROR);
    if (i == ERROR || i == 0) return;

    if (i != ERROR)
        do_event(i - 1);
}

/* -------------------------------------------------------------------- */
/*  next_event_set() Set the next event pointer.                        */
/* -------------------------------------------------------------------- */
void next_event_set(void)
{
    int i;

    i = (int)getNumber(event_0_equals_none, 0L, (long)numevents+1, (long)ERROR);
    if (i == ERROR || i == 0) return;

    if (i != ERROR)
        on_event = i - 1;
}
      
