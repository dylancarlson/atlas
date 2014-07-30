/***************************************************************************/
/*									   */
/* FUNCTION: Capture () 						   */
/*									   */
/* PURPOSE:  Takes control of GLOBAL.DAT if we do not already have it,	   */
/*	     identifies if any changes have been made, updates global data */
/*	     if changes have been made, and increments the lock count on   */
/*	     GLOBAL.DAT.						   */
/*									   */
/* RETURNS:  TRUE if the function executed successfully.		   */
/*									   */
/***************************************************************************/

BOOL Capture( void )
{
}


/***************************************************************************/
/*									   */
/* FUNCTION:   Release ()						   */
/*									   */
/* PURPOSE:    Release decrements the lock count.  If the count reaches    */
/*	       zero, Release notifies the global file of any changes made  */
/*	       since the lock began.  If no changes have been made,	   */
/*	       GLOBAL.DAT is not modified.  If changes have been made,	   */
/*	       timestamps in GLOBAL.DAT are incremented.  As well, the	   */
/*	       appropriate .DAT files are locked and the changes are saved */
/*	       into them.  Lastly, the .DAT files and GLOBAL.DAT are	   */
/*	       unlocked.						   */
/*									   */
/* PARAMETERS: While changes should be explicitly reported, Release also   */
/*	       can scan different areas for changes.  The following flags  */
/*	       tell Release what areas to look for changes in:		   */
/*									   */
/*		    MOD_ALL	-- Check all data for modification.  Slow.
		    MOD_USER	-- Check the userlog for modifications.
		    MOD_ROOM	-- Check rooms for a modification.
		    MOD_MESSAGE -- Check the message memory for a change.
		    MOD_GROUP	-- ??? Hmmm...
		    MOD_NONE	-- Check no areas.  Changes were explicitly
				   reported (see ExpMod).		   */
/*									   */
/***************************************************************************/

void Release ( void )
{
}

/***************************************************************************/
/*									   */
/* FUNCTION: Change ()							   */
/*									   */
/* PURPOSE:  Tells us whether GLOBAL.DAT has been modified since we last   */
/*	     read it.  We use a timestamp to accomplish this.		   */
/*									   */
/* RETURNS:  TRUE if the file has been changed. 			   */
/*									   */
/***************************************************************************/

BOOL Change ( void )
{
}


/***************************************************************************/
/*									   */
/* FUNCTION:   ExpMod ()						   */
/*									   */
/* PURPOSE:    Tells Release exactly what information has been modified.   */
/*	       When Release is called, these timestamps WILL be updated.   */
/*									   */
/* PARAMETERS: Flag defining the kind of data modified, and the slot #.    */
/*									   */
/***************************************************************************/

void ExpMod ()
{
}





BOOL CaptureMsgFl (int hFile) ??? I might be able to keep FILE's if I lock
				entire files rather than portions.  GLOBAL.DAT
				could be a handle thing.
{
}
