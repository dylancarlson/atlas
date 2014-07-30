/* -------------------------------------------------------------------- */
/*                                NET.H                                 */
/*                                                                      */
/*              Define some network specific types, &tc.                */
/* -------------------------------------------------------------------- */

typedef struct 
{
    char        tempName[13];
    char        fileName[13];
    time_t      timeStamp;
    label       uploader;
    char        description[65];
    label       routeNode;
    label       forNode[1];
}
NET_FILE;


