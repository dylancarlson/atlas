/* -------------------------------------------------------------------- */
/*  EDIT.C                   Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*                Message editor and related code.                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include "ctdl.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  editText()      handles the end-of-message-entry menu.              */
/*  putheader()     prints header for message entry                     */
/*  getText()       reads a message from the user                       */
/*  matchString()   searches for match to given string.                 */
/*  replaceString() corrects typos in message entry                     */
/*  wordcount()     talleys # lines, word & characters message containes*/
/*  fakeFullCase()  converts a message in uppercase-only to a           */
/*  xPutStr()       Put a string to a file w/o trailing blank           */
/*  GetFileMessage()    gets a null-terminated string from a file       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  06/06/89    (PAT)   Made history, cleaned up comments, reformated   */
/*                      icky code.                                      */
/*  06/18/89    (LWE)   Added wordwrap to message entry                 */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  editText()      handles the end-of-message-entry menu.              */
/*      return TRUE  to save message to disk,                           */
/*             FALSE to abort message, and                              */
/*             ERROR if user decides to continue                        */
/* -------------------------------------------------------------------- */
int editText(char *buf, int lim)
{
    char ch, x;
    FILE *fd;
    char stuff[100];
    label tmp1, tmp2;
    char ich;
    int i;

    strcpy(gprompt, "2Entry cmd:0 ");
    dowhat = PROMPT;

    do
    {
        outFlag = IMPERVIOUS;
        while (MIReady()) getMod();
        doCR();
        mPrintf("2Entry cmd:0 ");
        switch ( ch = (char)toupper(ich = (char)iCharNE()) )
        {
        case 'A':
            mPrintf("Abort\n ");
            if  (!strlen(buf)) return FALSE;
            if (getYesNo(confirm, 0))
            {
                heldMessage = TRUE;

                memcpy( msgBuf2, msgBuf, sizeof(struct msgB) );

                dowhat = DUNO;
                return FALSE;
            }
            break;

#ifdef GOODBYE
        case 'C':
            mPrintf("Continue");
            doCR();
            return ERROR;
#endif
        case 'C':
            mPrintf("Continue");
            for( i = strlen(buf); i > 0 && buf[i-1] == 10; i--)
                buf[i-1] = 0;
            doCR();
            doCR();
            outFlag = OUTOK;
            putheader(FALSE);
            doCR();
            mFormat(buf);
            outFlag = IMPERVIOUS;
            return ERROR;

        case 'F':
            mPrintf("Find & Replace text\n ");
            replaceString(buf, lim, TRUE);
            break;

        case 'P':
            mPrintf("Print formatted\n ");
            doCR();
            outFlag = OUTOK;
            putheader(FALSE);
            doCR();
            mFormat(buf);
            if ((msgBuf->mbsig[0] || msgBuf->mbusig[0]) && cfg.nodeSignature[0])
            {
                termCap(TERM_BOLD);
                doCR(); mFormat("------");
                termCap(TERM_NORMAL);
                if (*msgBuf->mbusig)
                {
                    doCR();
                    mFormat(msgBuf->mbusig);
                }
                if (*msgBuf->mbsig)
                {
                    doCR();
                    mFormat(msgBuf->mbsig);
                }
            }
            termCap(TERM_NORMAL);
            outFlag = IMPERVIOUS;
            doCR();
            break;

        case 'R':
            mPrintf("Replace text\n ");
            replaceString(buf, lim, FALSE);
            break;

        case 'S':
            if (cfg.msgNym)
            {
                mPrintf("%s %s", cfg.msg_done, cfg.msg_nym);
                doCR();
            }
            else
            {
                mPrintf("Save");
                wordcount(buf);
            }
            entered++;  /* increment # messages entered */
            dowhat = DUNO;
            return TRUE;

        case 'W':
            mPrintf("Word count\n ");
            wordcount(buf);
            break;

        case '?':
            oChar('?');
            tutorial("edit.mnu");
            break;

        case '!':
            mPrintf("Subject / Signature\n ");
            getString("subject",   msgBuf->mbsub,  NAMESIZE, FALSE, ECHO, "");
            getString("signature", msgBuf->mbusig, NAMESIZE, FALSE, ECHO, "");
            break;
        
        case '@':
            mPrintf("Address\n ");
            getString("destinaton user", msgBuf->mbto,  NAMESIZE, FALSE, ECHO, "");
            getString("node", msgBuf->mbzip, NAMESIZE, FALSE, ECHO, "");
            getString("region", msgBuf->mbrzip, NAMESIZE, FALSE, ECHO, "");
            break;
            
        case '*':
            if (sysop)
            {
                mPrintf("Name Change"); doCR();

                strcpy(stuff, msgBuf->mbtitle);
                getString("title", msgBuf->mbtitle, 
                          NAMESIZE, FALSE, ECHO, stuff);

                strcpy(stuff, msgBuf->mbauth);
                getString("name", msgBuf->mbauth, 
                          NAMESIZE, FALSE, ECHO, stuff);
                
                strcpy(stuff, msgBuf->mbsur);
                getString("surname", msgBuf->mbsur, 
                          NAMESIZE, FALSE, ECHO, stuff);
                
                break;
            }
            /* fall through */

        default:
            if ( (x = (char)strpos((char)tolower(ch), editcmd)) != 0 )
            {
                x--;
                mPrintf("%s", edit[x].ed_name); doCR();
                if (edit[x].ed_local && !onConsole)
                {
                    mPrintf("\n Local editor only!\n ");
                }
                else
                {
                    changedir(cfg.aplpath);
                    if ((fd = fopen("message.apl", "wb")) != NULL)
                    {
                        xPutStr(fd, msgBuf->mbtext);
                        fclose(fd);
                    }
                    sprintf(tmp1, "%d", cfg.mdata);
                    sprintf(tmp2, "%d", bauds[speed]);
                    if (onConsole)
                    {
                        strcpy(tmp1, "LOCAL");
                    }
                    sformat(stuff, edit[x].ed_cmd, "fpsa", "message.apl",
                            tmp1, tmp2, cfg.aplpath);
                    readMessage = FALSE;
                    
                    apsystem(stuff);
                    
                    
                    if ((fd = fopen("message.apl", "rb")) != NULL)
                    {
                        GetFileMessage(fd, msgBuf->mbtext, cfg.maxtext);
                        fclose(fd);
                        unlink("message.apl");
                    }
                    if (debug) cPrintf("(%s)", stuff);
                    changedir(cfg.homepath);
                }
                break;
            }
            oChar(ich);
            if (!expert) tutorial("edit.mnu");
            else mPrintf("\n '?' for menu.\n \n");
            break;
        }
    }
    while (CARRIER);
    dowhat = DUNO;
    return FALSE;
}

/* -------------------------------------------------------------------- */
/*  putheader()     prints header for message entry                     */
/* -------------------------------------------------------------------- */
void putheader(BOOL first)
{
    char dtstr[80];
    int oldEcho = echo;
    
    if (first) echo = BOTH;
    
    strftime(dtstr, 79, cfg.datestamp, 0l);

    termCap(TERM_BOLD);
    mPrintf("    %s", dtstr);
    
    mPrintf(" From ");
    
    if (!roomBuf.rbflags.ANON)
    {
        if (msgBuf->mbtitle[0] && logBuf.DISPLAYTS
           && (
                (cfg.titles && !(msgBuf->mboname[0])) 
                || cfg.nettitles
              )
           )
        {
             mPrintf( "[%s] ", msgBuf->mbtitle);
        }
        
        mPrintf("4%s03", msgBuf->mbauth);
        
        if (msgBuf->mbsur[0] && logBuf.DISPLAYTS
           && (
                (cfg.surnames && !(msgBuf->mboname[0])) 
                || cfg.netsurname
              )
           )
        {
             mPrintf( " [%s]", msgBuf->mbsur);
        }
    }
    else
    {
        /* mPrintf("****"); */
        mPrintf("4%s03", cfg.anonauthor);
    }
        
    if (msgBuf->mbto[0])    mPrintf(" To %s", msgBuf->mbto);
    if (msgBuf->mbzip[0])   mPrintf(" @ %s", msgBuf->mbzip);
    if (msgBuf->mbrzip[0])  mPrintf(", %s", msgBuf->mbrzip);
    if (msgBuf->mbczip[0])  mPrintf(", %s", msgBuf->mbczip);
    if (msgBuf->mbfwd[0])   mPrintf(" Forwarded to %s", msgBuf->mbfwd);
    if (msgBuf->mbgroup[0]) mPrintf(" (%s Only)", msgBuf->mbgroup);
    if (msgBuf->mbsub[0])
    {
        doCR();
        mPrintf("    Subject: 4%s0", msgBuf->mbsub);
    }
    termCap(TERM_NORMAL);
    
    echo = (uchar)oldEcho;
}

/* -------------------------------------------------------------------- */
/*  getText()       reads a message from the user                       */
/*                  Returns TRUE if user decides to save it, else FALSE */
/* -------------------------------------------------------------------- */
BOOL getText(void)
{
    char temp, done, d, c = 0, *buf, beeped = FALSE;
    int  i, toReturn, lim, wsize = 0,j;
    char /* lastCh, */ word[50];

    if (!expert)
    {
        tutorial("entry.blb");
        outFlag = OUTOK;
        doCR();
        mPrintf(" You have up to %d characters.", cfg.maxtext );
        mPrintf("\n Enter %s (end with empty line).", cfg.msg_nym);
    }

    outFlag = IMPERVIOUS;

    doCR();
    putheader(TRUE);
    doCR();

    buf     = msgBuf->mbtext;
    
    if (oldFlag) 
    {
        int i;

        outFlag = OUTOK;
      
        for( i = strlen(buf); i > 0 && buf[i-1] == 10; i--)
            buf[i-1] = 0;
        mFormat(msgBuf->mbtext);

        outFlag = IMPERVIOUS;
    }

    /* lastCh  = 10 */ /* NEWLINE */;

    lim     = cfg.maxtext - 1;

    done = FALSE;

    asciitable();
    for (i=127; i<256; i++)
        tfilter[i] = (char)i;
    
    do
    {
        i = strlen(buf);
        /* if (i) lastCh = buf[i]; */

        while (!done && i < lim && CARRIER )
        {
            /* if (i) lastCh = c; */
            c = (char)iChar();

            switch(c)  /* Analyse what they typed       */
            {
            case CTRL_A:                /* CTRL-A>nsi   */
                temp = echo;
                echo = NEITHER;
                d = (char)iChar();
                echo = temp;

                if (
                      (d >= '0' && d <= '4') 
                   || (d >= 'a' && d <= 'h')
                   || (d >= 'A' && d <= 'H')
                   )
                {
                    termCap(d);
                    buf[i++]   = 0x01;
                    buf[i++]   = d;
                }
                else 
                {
                    oChar(7);
                }
                break;
                
             case  10:                  /* NEWLINE      */
                if ( /* (lastCh == 10) || */ (i == 0) ||
                     (i && buf[i-1] == 10 )) done = TRUE;
                if (!done) buf[i++] = 10; /* A CR might be nice   */
                break;
                
             case  ESC:                 /* An ESC? no, No, NO!  */
                oChar(7);    
                break;
                
             case  26:                  /* CTRL-Z       */
                done = TRUE;
                entered++;  /* increment # messages entered */
                break;  
                
             case '\b':                 /* Backspace */
                if (i > 0 && buf[i-1] == '\t')  /* TAB  */
                {
                    i--;
                    while ( (crtColumn % 8) != 1)  doBS();
                }
                else
                    if (i > 0 && buf[i-1] != 10)
                    {
                        i--;
                        if(wsize > 0)  wsize--;
                    }
                    else
                    {
                        oChar(32);
                        oChar(7);
                    }
                break;
             
             default:
                if (c == '\r' || c == '\n' || c == ' ' || c == '\t' || wsize == 50)
                {
                    wsize = 0;
                }
                else
                {
                    if (crtColumn >= (uchar)(termWidth-1))
                    {
                        if (wsize)
                        {
                            for (j = 0; j < (wsize+1); j++)
                                doBS();
                            doCR();
                            for (j = 0; j < wsize; j++)
                                echocharacter(word[j]);
                            echocharacter(c);
                        }
                        else
                        {
                            doBS();
                            doCR();
                            echocharacter(c);
                        }

                        wsize = 0;
                    }
                    else
                    {
                        word[wsize] = (char)c;
                        wsize ++;
                    }
                }

                if (c != 0) buf[i++]   = c;
                if (i > cfg.maxtext - 80 && !beeped)
                {
                    beeped = TRUE;
                    oChar(7 /* BELL */);
                }
                break;
            }
           
            buf[i] = 0x00;              /* null to terminate message     */
            /* if (i) lastCh = buf[i-1]; */

            if (i == lim)   mPrintf(" Buffer overflow.\n ");
        }
        done = FALSE;                   /* In case they Continue         */
        
        termCap(TERM_NORMAL);

        if (c == 26 && i != lim)        /* if was CTRL-Z        */
        {
           buf[i++] = 10;               /* end with NEWLINE     */
           buf[i] = 0x00;
           toReturn = TRUE;             /* yes, save it         */
           doCR();
           if (cfg.msgNym)
           {
               mPrintf(" %s %s", cfg.msg_done, cfg.msg_nym);
               doCR();
           }
           else
           {
               mPrintf(" Saved ");
               wordcount(buf);
           }
        }
        else                           /* done or lost carrier */
        {
            asciitable();
            toReturn = editText(buf, lim);
            asciitable();
            for (i=127; i<256; i++)
                tfilter[i] = (char)i;
        }

    }   
    while ((toReturn == ERROR)  &&  CARRIER);
    /* ERROR returned from editor means continue    */

    asciitable();
    
    if (toReturn == TRUE || toReturn == ERROR)     /* Filter null messages */
    {   
        toReturn = FALSE;
        for (i = 0; buf[i] != 0 && !toReturn; i++)
            toReturn = (buf[i] > ' ');
    }
    return  (BOOL)toReturn;
}

/* -------------------------------------------------------------------- */
/*  matchString()   searches for match to given string.                 */
/*                  Runs backward  through buffer so we get most recent */
/*                  error first. Returns loc of match, else ERROR       */
/* -------------------------------------------------------------------- */
char *matchString(char *buf, char *pattern, char *bufEnd, char ver)
{
    char *loc, *pc1, *pc2;
    char subbuf[10];
    char foundIt;

    for (loc = bufEnd, foundIt = FALSE;  !foundIt && --loc >= buf;)
    {
        for (pc1 = pattern, pc2 = loc,  foundIt = TRUE ;  *pc1 && foundIt;)
        {
            if (! (tolower(*pc1++) == tolower(*pc2++)))   foundIt = FALSE;
        }
        if (ver && foundIt)
        {
          doCR();
          strncpy(subbuf,
             buf + 10 > loc ? buf : loc - 10,
             (unsigned)(loc - buf) > 10 ? 10 : (unsigned)(loc - buf));
          subbuf[(unsigned)(loc - buf) > 10 ? 10 : (unsigned)(loc - buf)] = 0;
          mPrintf("%s", subbuf);
          if (*term.bold)
              termCap(TERM_BOLD);
          else
              mPrintf(">");
          mPrintf("%s", pattern);
          if (*term.bold)
              termCap(TERM_NORMAL);
          else
              mPrintf("<");
          strncpy(subbuf, loc + strlen(pattern), 10 );
          subbuf[10] = 0;
          mPrintf("%s", subbuf);
          if (!getYesNo("Replace", 0))
            foundIt = FALSE;
        }
    }
    return   foundIt  ?  loc  :  NULL;
}

/* -------------------------------------------------------------------- */
/*  replaceString() corrects typos in message entry                     */
/* -------------------------------------------------------------------- */
void replaceString(char *buf, int lim, char ver)
{
    char oldString[260];
    char newString[260];
    char *loc, *textEnd;
    char *pc;
    int  incr, length;
    char *matchString();
                                                  /* find terminal null */
    for (textEnd = buf, length = 0;  *textEnd;  length++, textEnd++);

    getString("text",      oldString, 256, FALSE, ECHO, "");
    if (!*oldString)
    {
        mPrintf(" Text not found.\n");
        return;
    }

    if ((loc=matchString(buf, oldString, textEnd, ver)) == NULL)
    {
        mPrintf(" Text not found.\n ");
        return;
    }

    getString("replacement text", newString, 256, FALSE, ECHO, "");
    if    (   strlen(newString) > strlen(oldString)
    && (((int)strlen(newString) - strlen(oldString))  >=  (lim - length)))
    {
        mPrintf(" Buffer overflow.\n ");
        return;
    }

    /* delete old string: */
    for (pc=loc, incr=strlen(oldString); (*pc=*(pc+incr)) != 0;  pc++);
    textEnd -= incr;

    /* make room for new string: */
    for (pc=textEnd, incr=strlen(newString);  pc>=loc;  pc--)
    {
        *(pc+incr) = *pc;
    }

    /* insert new string: */
    for (pc=newString;  *pc;  *loc++ = *pc++);
}

/* -------------------------------------------------------------------- */
/*  wordcount()     talleys # lines, word & characters message containes*/
/* -------------------------------------------------------------------- */
void wordcount(char *buf)
{
    char *counter;
    int lines = 0, words = 0, chars;

    counter = buf;

    chars = strlen(buf);

    while(*counter++)
    {
         if (*counter == ' ')
         {
             if ( ( *(counter - 1) != ' ') && ( *(counter - 1) != '\n') ) 
             words++;
         }

         if (*counter == '\n')
         {
             if ( ( *(counter - 1) != ' ') && ( *(counter - 1) != '\n') ) 
             words++;
             lines++;
         } 

    }
    mPrintf(" %d lines, %d words, %d characters", lines, words, chars);
    doCR();
}

/* -------------------------------------------------------------------- */
/*  fakeFullCase()  converts a message in uppercase-only to a           */
/*      reasonable mix.  It can't possibly make matters worse...        */
/*      Algorithm: First alphabetic after a period is uppercase, all    */
/*      others are lowercase, excepting pronoun "I" is a special case.  */
/*      We assume an imaginary period preceding the text.               */
/* -------------------------------------------------------------------- */
void fakeFullCase(char *text)
{
    char *c;
    char lastWasPeriod;
    char state;

    for(lastWasPeriod=TRUE, c=text;   *c;  c++)
    {
        if ( (*c != '.') && (*c != '?') && (*c != '!') )
        {
            if (isalpha(*c))
            {
                if (lastWasPeriod)  *c = (char)toupper(*c);
                else                *c = (char)tolower(*c);
                lastWasPeriod          = FALSE;
            }
        } else
        {
            lastWasPeriod       = TRUE ;
        }
    }

    /* little state machine to search for ' i ': */
    #define NUTHIN          0
    #define FIRSTBLANK      1
    #define BLANKI          2
    for (state=NUTHIN, c=text;  *c;  c++)
    {
        switch (state)
        {
        case NUTHIN:
            if (isspace(*c))  state   = FIRSTBLANK;
            else              state   = NUTHIN    ;
            break;
        case FIRSTBLANK:
            if (*c == 'i')    state   = BLANKI    ;
            else              state   = NUTHIN    ;
            break;
        case BLANKI:
            if (isspace(*c))  state   = FIRSTBLANK;
            else              state   = NUTHIN    ;

            if (!isalpha(*c)) *(c-1)  = 'I';
            break;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  xPutStr()       Put a string to a file w/o trailing blank           */
/* -------------------------------------------------------------------- */
void xPutStr(FILE *fl, char *str)
{
    while(*str)
    {
        fputc(*str, fl);
        str++;
    }
}

/* -------------------------------------------------------------------- */
/*  GetFileMessage()    gets a null-terminated string from a file       */
/* -------------------------------------------------------------------- */
void GetFileMessage(FILE *fl, char *str, int mlen)
{
    int l = 0, i;
    char ch;
 
    asciitable();
    for (i=127; i<256; i++)
        tfilter[i] = (char)i;
    tfilter['\r']  = '\0';
    tfilter[10  ]  = 10;
    tfilter[0xFF]  = '\0';
 
    while(!feof(fl) && l < mlen)
    {
        if ((ch = tfilter[(uchar)fgetc(fl)]) != '\0' && !feof(fl))
        {
            str[l] = ch;
            l++;
        }
    }
 
    str[l]='\0';
 
    asciitable();
}

