/* -------------------------------------------------------------------- */
/*  NET.C                    Dragon Citadel                             */
/* -------------------------------------------------------------------- */
/*      Networking libs for the Citadel bulletin board system           */
/* -------------------------------------------------------------------- */
#ifdef NETWORK

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "keywords.h"
#include "proto.h"
#include "global.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  dc15network()   During network master code                          */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  HISTORY:                                                            */
/*                                                                      */
/*  09/17/90    (PAT)   Created to hold DragNet 1.5 special code        */
/*                                                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Static Data                                                         */
/* -------------------------------------------------------------------- */
static void sendRequest(void);
static void reciveRequest(void);
static void makeSendFile(void);
static void sendFiles(void);
static void reciveFiles(void);
static netFailed = FALSE;

/* -------------------------------------------------------------------- */
/*  dc15master()    During network master code                          */
/* -------------------------------------------------------------------- */
BOOL dc15network(BOOL master)
{
    char line[100], line2[100];
    label here, there;
    FILE *file;
    int i, rms;
    time_t t, t2=0;
    BOOL    done = FALSE;
    
    netFailed = FALSE;
    
    if (!gotCarrier()) return FALSE;

    sprintf(line, "%s\\mesg.tmp", cfg.temppath);
    unlink(line);
    
    sprintf(line, "%s\\mailin.tmp", cfg.temppath);
    unlink(line);

    sprintf(line, "%s\\roomreq.in", cfg.temppath);
    unlink(line);
    
    sprintf(line, "%s\\roomreq.out", cfg.temppath);
    unlink(line);

    if((file = fopen(line, "ab")) == NULL)
    {
        perror("Error opening roomreq.out");
        return FALSE;
    }

    for (i=get_first_room(here, there), rms=0;
         i;
         i=get_next_room(here, there), rms++)
    {
        PutStr(file, there);
    }

    PutStr(file, "");
    fclose(file);

    if (master)
    {
        sendRequest();
        if (!gotCarrier()) return FALSE;
        reciveRequest();
    }
    else
    {
        reciveRequest();
        if (!gotCarrier()) return FALSE;
        sendRequest();
    }
    
    if (!gotCarrier() || netFailed) return FALSE;
    
    if (master)
    {
        /* clear the buffer */
        while (gotCarrier() && MIReady())
        {
            getMod();
        }
    }
    
    makeSendFile();
    
    if (!gotCarrier() || netFailed) return FALSE;
        
    /*
     * wait for them to get their shit together 
     */
    cPrintf(" Waiting for transfer.");
    
    outMod('X');
    t2 = 0;
    t = time(NULL); 
    while (gotCarrier() && !done)
    {
        if (time(NULL) > (t + (35 * 60))) /* only wait 35 minutes */
        {
            drop_dtr();
            netFailed = TRUE;
        }
        
        if (MIReady())
        {
            i = getMod();
            if (i == 'X' || node.network != NET_DCIT16)
            {
                done = TRUE;
            }
            else
            {
                if (debug)
                {
                    cPrintf("<%c>", i);
                }
            }
                
        }
        
        /* wake them up! (every second) */
        if (time(NULL) != t2)
        {
            outMod('X');
            t2 = time(NULL);
        }
    }
    
    /* wake them up! */
    for (i=0; i<10; i++)
        outMod('X');
    
    doccr();

    if (!gotCarrier() || netFailed) return FALSE;

    if (master)
    {
        reciveFiles();
        if (!gotCarrier() || netFailed) return FALSE;
        sendFiles();
    }
    else
    {
        sendFiles();
        if (!gotCarrier() || netFailed) return FALSE;
        reciveFiles();
    }
    
    if (netFailed) return FALSE;
    
    cPrintf(" Hangup.");
    doccr();
    
    drop_dtr();

    cPrintf(" Uncompressing message files.");
    doccr();
         
    sformat(line, node.unzip, "d", roomdatain);
    apsystem(line);
         
    unlink(roomdatain);
    
    for (i=0; i<rms; i++)
    {
        sprintf(line,  "room.%d",   i);
        sprintf(line2, "roomin.%d", i);
        rename(line, line2);
    }
        
    sprintf(line,  "%s\\mesg.tmp",   cfg.temppath);
    sprintf(line2, "%s\\mailin.tmp", cfg.temppath);
    rename(line, line2);

    return TRUE;
}

/* -------------------------------------------------------------------- */
/*  sendRequest()   Send the room request file                          */
/* -------------------------------------------------------------------- */
static void sendRequest(void)
{
    cPrintf(" Sending room request file.");
    doccr();

    wxsnd(cfg.temppath, roomreqout, 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    
    unlink(roomreqout);
}

/* -------------------------------------------------------------------- */
/*  reciveRequest() Recive the room request file                        */
/* -------------------------------------------------------------------- */
static void reciveRequest(void)
{
    cPrintf(" Receiving room request file.");
    doccr();

    wxrcv(cfg.temppath, roomreqin, 
            (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
            
    if (!filexists(roomreqin))
    {
        drop_dtr();
        netFailed = TRUE;
    }
}

/* -------------------------------------------------------------------- */
/*  makeSendFile()  Make the file to send to remote..                   */
/* -------------------------------------------------------------------- */
static void makeSendFile(void)
{
    char line[100], line2[100];
    label troo;
    label fn;
    FILE *file;
    int i = 0, rm;
    
    if ((file = fopen(roomreqin, "rb")) == NULL)
    {
        perror("Error opening roomreq.in");
    }

    doccr();
    cPrintf(" Fetching:");
    doccr();

    GetStr(file, troo, LABELSIZE);
    
    while(strlen(troo) && !feof(file))
    {
        if ((rm = roomExists(troo)) != ERROR)
        {
	    if (nodecanseeroom(node.ndname, rm))
            {
                sprintf(fn, "room.%d", i);
                cPrintf(" %-20s  ", troo);
                if( !((i+1) % 3) ) doccr();
                NewRoom(rm, fn);
            }
            else
            {
                doccr();
                cPrintf(" No access to %s room.", troo);
                doccr();
                amPrintf(" '%s' room not available to remote.\n", troo);
                netError = TRUE;
            }
        }
        else
        {
            doccr();
            cPrintf(" No %s room on system.", troo);
            doccr();
            amPrintf(" '%s' room not found for remote.\n", troo);
            netError = TRUE;
        }

        i++;
        GetStr(file, troo, LABELSIZE);
    }
    doccr();
    fclose(file);
    unlink(roomreqin);

    cPrintf(" Copying mail file.");
    doccr();
    sprintf(line,  "%s\\%s",         cfg.transpath, node.ndmailtmp);
    sprintf(line2, "%s\\mesg.tmp",   cfg.temppath);
    if ((file = fopen(line2, "wb")) != NULL)
    {
        fclose(file);
    }
    copyfile(line, line2);
    
    cPrintf(" Compressing message files.");
    doccr();
    
    /* 
     * Zip them up
     */
    sformat(line, node.zip, "df", roomdataout, "mesg.tmp room.*");
    apsystem(line);
    
    /* 
     * Remove them.
     */
    ambigUnlink("room.*",   FALSE);
    unlink("mesg.tmp");
}

/* -------------------------------------------------------------------- */
/*  sendFiles()     Send the data files                                 */
/* -------------------------------------------------------------------- */
static void sendFiles(void)
{
    cPrintf(" Sending mail and rooms.");
    doccr();

    wxsnd(cfg.temppath, roomdataout, 
         (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));
    
    unlink(roomdataout);
}

/* -------------------------------------------------------------------- */
/*  reciveFiles()   Recive the date files                               */
/* -------------------------------------------------------------------- */
static void reciveFiles(void)
{
    cPrintf(" Receiving mail and rooms.");
    doccr();

    wxrcv(cfg.temppath, roomdatain, 
            (char)strpos((char)tolower(node.ndprotocol[0]), extrncmd));

    if (!filexists(roomdatain))
    {
        drop_dtr();
        netFailed = TRUE;
    }
}

#endif /* NETWORK */
