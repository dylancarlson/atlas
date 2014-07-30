/*
 *  Copyright (C) 1990 by Marty Del Vecchio
 *
 *  This was writen by Marty Del Vecchio, and is avalible with
 *  documentation in the file SWAP300.ZIP. If you want to use
 *  this in your programs, I suggest you get it. Many thanks
 *  to this man for this wonderful function.
 */

/* This conditional extern statement is required when compiling with
   Turbo C++
 */
#ifdef __cplusplus
extern "C" {
#endif

int swap (unsigned char *program_name,
          unsigned char *command_line,
          unsigned char *exec_return,
          unsigned char *swap_fname);

#ifdef __cplusplus
}
#endif

int ems4_installed (void);
int xms_installed (void);

/* The return code from swap() will be one of the following.  Codes other    */
/* than SWAP_OK (0) indicate that an error occurred, and thus the program    */
/* has NOT been swapped, and the new program has NOT been executed.          */

#define SWAP_OK         (0)         /* Swapped OK and returned               */
#define SWAP_NO_SHRINK  (1)         /* Could not shrink DOS memory size      */
#define SWAP_NO_SAVE    (2)         /* Could not save program to XMS/EMS/disk*/
#define SWAP_NO_EXEC    (3)         /* Could not execute new program         */


/* If swap() returns 3, SWAP_NO_EXEC, the byte/char pointed to by the        */
/* parameter exec_return will be one of the following standard DOS error     */
/* codes, as specified in the DOS technical reference manuals.               */

#define BAD_FUNC        (0x01)   /* Bad DOS function number--unlikely          */
#define FILE_NOT_FOUND  (0x02)   /* File not found--couldn't find program_name */
#define ACCESS_DENIED   (0x05)   /* Access denied--couldn't open program_name  */
#define NO_MEMORY       (0x08)   /* Insufficient memory to run program_name    */
#define BAD_ENVIRON     (0x0A)   /* Invalid environment segment--unlikely      */
#define BAD_FORMAT      (0x0B)   /* Format invalid--unlikely                   */

