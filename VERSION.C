/* ------------------------------------------------------------------------- */
/*  Makes us able to change the version at a glance       VERSION.C          */
/* ------------------------------------------------------------------------- */

#define VERSION "2.00.05a"

char version[]      = VERSION;
char programName[]  = "Atlas";

char compilerName[] = "Microsoft C"; 

#ifdef ALPHA_TEST
char testsite[] = "Alpha Test Site";
#else
#  ifdef BETA_TEST
char testsite[] = "Beta Test Site";
#  else
char testsite[] = "";
#  endif
#endif

char cmpDate[] = __DATE__;
char cmpTime[] = __TIME__;

/* failing to leave these at 3 and 2 lines will cause the waiting for
   call screen to mess up. See logo() in MISC2.C to modify this. */

char *welcome[] = {    /* 3 LINES!!!!!! \" */
      "All the traditions of dead generations",
      "lay on the minds of the living",
      "like a nightmare.",
    0
};

char *copyright[] = {   /* 2 LINES!!!! */
       "Atlas",
       "Created using Microsoft C Optimizing Compiler Version 6.00A",
    0
};
