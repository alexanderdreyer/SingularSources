/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/*
* ABSTRACT:
*/

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "mod2.h"
#include <mylimits.h>
#include "omalloc.h"
#include "structs.h"
#include "options.h"
#include "febase.h"
//#include "cntrlc.h"
#include "kstd1.h"
#include "timer.h"
#include "intvec.h"
#include "ring.h"
#include "omSingularConfig.h"
#include "p_Procs.h"
#include "kversion.h"

#define SI_DONT_HAVE_GLOBAL_VARS

//#ifdef HAVE_LIBPARSER
//#  include "libparse.h"
//#endif /* HAVE_LIBPARSER */

#ifdef HAVE_FACTORY
#include <factory.h>
/* libfac version strings */
  extern const char * libfac_version;
  extern const char * libfac_date;
#endif

#include <si_gmp.h>
#ifdef HAVE_MPSR
#include <MP_Config.h>
#endif

/* init bins from structs.h */

omBin char_ptr_bin = omGetSpecBin(sizeof(char_ptr));

/*0 implementation*/

/*2
* the global exit routine of Singular
*/
#ifdef HAVE_MPSR
void (*MP_Exit_Env_Ptr)()=NULL;
#endif
extern "C" {

void m2_end(int i)
{
  fe_reset_input_mode();
  #ifdef PAGE_TEST
  mmEndStat();
  #endif
  #ifdef HAVE_TCL
  if (tclmode)
  {
    PrintTCL('Q',0,NULL);
  }
  #endif
  fe_reset_input_mode();
  if (i<=0)
  {
    #ifdef HAVE_TCL
    if (!tclmode)
    #endif
      if (TEST_V_QUIET)
      {
        if (i==0)
          printf("Auf Wiedersehen.\n");
        else
          printf("\n$Bye.\n");
      }
    //#ifdef sun
    //  #ifndef __svr4__
    //    _cleanup();
    //    _exit(0);
    //  #endif
    //#endif
    exit(0);
  }
  else
  {
    #ifdef HAVE_TCL
    if (!tclmode)
    #endif
      printf("\nhalt %d\n",i);
  }
  #ifdef HAVE_MPSR
  if (MP_Exit_Env_Ptr!=NULL) (*MP_Exit_Env_Ptr)();
  #endif
  exit(i);
}
}

/*2
* the renice routine for very large jobs
* works only on unix machines,
* testet on : linux, HP 9.0
*
*#include <sys/times.h>
*#include <sys/resource.h>
*extern "C" int setpriority(int,int,int);
*void very_nice()
*{
*#ifndef NO_SETPRIORITY
*  setpriority(PRIO_PROCESS,0,19);
*#endif
*  sleep(10);
*}
*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#ifdef buildin_rand
/*
 *
 *  A prime modulus multiplicative linear congruential
 *  generator (PMMLCG), or "Lehmer generator".
 *  Implementation directly derived from the article:
 *
 *        S. K. Park and K. W. Miller
 *        Random Number Generators: Good Ones are Hard to Find
 *        CACM vol 31, #10. Oct. 1988. pp 1192-1201.
 *
 *  Using the following multiplier and modulus, we obtain a
 *  generator which:
 *
 *        1)  Has a full period: 1 to 2^31 - 2.
 *        2)  Is testably "random" (see the article).
 *        3)  Has a known implementation by E. L. Schrage.
 */


#define  A        16807        /*  A "good" multiplier          */
#define  M   2147483647        /*  Modulus: 2^31 - 1          */
#define  Q       127773        /*  M / A                  */
#define  R         2836        /*  M % A                  */


int siSeed = 1;


int siRand()
{
  siSeed = A * (siSeed % Q) - R * (siSeed / Q);

  if ( siSeed < 0 )
    siSeed += M;

  return( siSeed );
}
#endif

