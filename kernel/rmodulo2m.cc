/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/* $Id$ */
/*
* ABSTRACT: numbers modulo 2^m
*/

#include <string.h>
#include "mod2.h"

#ifdef HAVE_RINGS
#include <mylimits.h>
#include "structs.h"
#include "febase.h"
#include "omalloc.h"
#include "numbers.h"
#include "longrat.h"
#include "mpr_complex.h"
#include "ring.h"
#include "rmodulo2m.h"
#include "si_gmp.h"

int nr2mExp;

/*
 * Multiply two numbers
 */
number nr2mMult (number a,number b)
{
  if (((NATNUMBER)a == 0) || ((NATNUMBER)b == 0))
    return (number)0;
  else
    return nr2mMultM(a,b);
}

/*
 * Give the smallest non unit k, such that a * x = k = b * y has a solution
 */
number nr2mLcm (number a,number b,ring r)
{
  NATNUMBER res = 0;
  if ((NATNUMBER) a == 0) a = (number) 1;
  if ((NATNUMBER) b == 0) b = (number) 1;
  while ((NATNUMBER) a % 2 == 0)
  {
    a = (number) ((NATNUMBER) a / 2);
    if ((NATNUMBER) b % 2 == 0) b = (number) ((NATNUMBER) b / 2);
    res++;
  }
  while ((NATNUMBER) b % 2 == 0)
  {
    b = (number) ((NATNUMBER) b / 2);
    res++;
  }
  return (number) (1L << res);  // (2**res)
}

/*
 * Give the largest non unit k, such that a = x * k, b = y * k has
 * a solution.
 */
number nr2mGcd (number a,number b,ring r)
{
  NATNUMBER res = 0;
  if ((NATNUMBER) a == 0 && (NATNUMBER) b == 0) return (number) 1;
  while ((NATNUMBER) a % 2 == 0 && (NATNUMBER) b % 2 == 0)
  {
    a = (number) ((NATNUMBER) a / 2);
    b = (number) ((NATNUMBER) b / 2);
    res++;
  }
//  if ((NATNUMBER) b % 2 == 0)
//  {
//    return (number) ((1L << res));// * (NATNUMBER) a);  // (2**res)*a    a ist Einheit
//  }
//  else
//  {
    return (number) ((1L << res));// * (NATNUMBER) b);  // (2**res)*b    b ist Einheit
//  }
}

/*
 * Give the largest non unit k, such that a = x * k, b = y * k has
 * a solution.
 */
number nr2mExtGcd (number a, number b, number *s, number *t)
{
  NATNUMBER res = 0;
  if ((NATNUMBER) a == 0 && (NATNUMBER) b == 0) return (number) 1;
  while ((NATNUMBER) a % 2 == 0 && (NATNUMBER) b % 2 == 0)
  {
    a = (number) ((NATNUMBER) a / 2);
    b = (number) ((NATNUMBER) b / 2);
    res++;
  }
  if ((NATNUMBER) b % 2 == 0)
  {
    *t = NULL;
    *s = nr2mInvers(a);
    return (number) ((1L << res));// * (NATNUMBER) a);  // (2**res)*a    a ist Einheit
  }
  else
  {
    *s = NULL;
    *t = nr2mInvers(b);
    return (number) ((1L << res));// * (NATNUMBER) b);  // (2**res)*b    b ist Einheit
  }
}

void nr2mPower (number a, int i, number * result)
{
  if (i==0)
  {
    *(NATNUMBER *)result = 1;
  }
  else if (i==1)
  {
    *result = a;
  }
  else
  {
    nr2mPower(a,i-1,result);
    *result = nr2mMultM(a,*result);
  }
}

/*
 * create a number from int
 */
number nr2mInit (int i, const ring r)
{
  long ii = i;
  while (ii < 0) ii += r->nr2mModul ;
  while ((ii>1) && (ii >= r->nr2mModul )) ii -= r->nr2mModul ;
  return (number) ii;
}

/*
 * convert a number to int (-p/2 .. p/2)
 */
int nr2mInt(number &n, const ring r)
{
  if ((NATNUMBER)n > (r->nr2mModul  >>1)) return (int)((NATNUMBER)n - r->nr2mModul );
  else return (int)((NATNUMBER)n);
}

number nr2mAdd (number a, number b)
{
  return nr2mAddM(a,b);
}

number nr2mSub (number a, number b)
{
  return nr2mSubM(a,b);
}

BOOLEAN nr2mIsUnit (number a)
{
  return ((NATNUMBER) a % 2 == 1);
}

number  nr2mGetUnit (number k)
{
  if (k == NULL)
    return (number) 1;
  NATNUMBER tmp = (NATNUMBER) k;
  while (tmp % 2 == 0)
    tmp = tmp / 2;
  return (number) tmp;
}

BOOLEAN nr2mIsZero (number  a)
{
  return 0 == (NATNUMBER)a;
}

BOOLEAN nr2mIsOne (number a)
{
  return 1 == (NATNUMBER)a;
}

BOOLEAN nr2mIsMOne (number a)
{
  return (currRing->nr2mModul  == (NATNUMBER)a + 1) 
        && (currRing->nr2mModul != 2);
}

BOOLEAN nr2mEqual (number a,number b)
{
  return nr2mEqualM(a,b);
}

BOOLEAN nr2mGreater (number a,number b)
{
  return nr2mDivBy(a, b);
}

BOOLEAN nr2mDivBy (number a,number b)
{
  if (a == NULL)
    return (currRing->nr2mModul  % (NATNUMBER) b) == 0;
  else
    return ((NATNUMBER) a % (NATNUMBER) b) == 0;
  /*
  if ((NATNUMBER) a == 0) return TRUE;
  if ((NATNUMBER) b == 0) return FALSE;
  while ((NATNUMBER) a % 2 == 0 && (NATNUMBER) b % 2 == 0)
  {
    a = (number) ((NATNUMBER) a / 2);
    b = (number) ((NATNUMBER) b / 2);
}
  return ((NATNUMBER) b % 2 == 1);
  */
}

int nr2mDivComp(number as, number bs)
{
  NATNUMBER a = (NATNUMBER) as;
  NATNUMBER b = (NATNUMBER) bs;
  assume(a != 0 && b != 0);
  while (a % 2 == 0 && b % 2 == 0)
  {
    a = a / 2;
    b = b / 2;
  }
  if (a % 2 == 0)
  {
    return -1;
  }
  else
  {
    if (b % 2 == 1)
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
}

BOOLEAN nr2mGreaterZero (number k)
{
  return ((NATNUMBER) k !=0) && ((NATNUMBER) k <= (currRing->nr2mModul >>1));
}

//#ifdef HAVE_DIV_MOD
#if 1 //ifdef HAVE_NTL // in ntl.a
//extern void XGCD(long& d, long& s, long& t, long a, long b);
#include <NTL/ZZ.h>
#ifdef NTL_CLIENT
NTL_CLIENT
#endif
#else
void XGCD(long& d, long& s, long& t, long a, long b)
{
   long  u, v, u0, v0, u1, v1, u2, v2, q, r;

   long aneg = 0, bneg = 0;

   if (a < 0) {
      a = -a;
      aneg = 1;
   }

   if (b < 0) {
      b = -b;
      bneg = 1;
   }

   u1=1; v1=0;
   u2=0; v2=1;
   u = a; v = b;

   while (v != 0) {
      q = u / v;
      r = u % v;
      u = v;
      v = r;
      u0 = u2;
      v0 = v2;
      u2 =  u1 - q*u2;
      v2 = v1- q*v2;
      u1 = u0;
      v1 = v0;
   }

   if (aneg)
      u1 = -u1;

   if (bneg)
      v1 = -v1;

   d = u;
   s = u1;
   t = v1;
}
#endif

NATNUMBER InvMod(NATNUMBER a)
{
   long d, s, t;

   XGCD(d, s, t, a, currRing->nr2mModul );
   assume (d == 1);
   if (s < 0)
      return s + currRing->nr2mModul ;
   else
      return s;
}
//#endif

inline number nr2mInversM (number c)
{
  // Table !!!
  NATNUMBER inv;
  inv = InvMod((NATNUMBER)c);
  return (number) inv;
}

number nr2mDiv (number a,number b)
{
  if ((NATNUMBER)a==0)
    return (number)0;
  else if ((NATNUMBER)b%2==0)
  {
    if ((NATNUMBER)b != 0)
    {
      while ((NATNUMBER) b%2 == 0 && (NATNUMBER) a%2 == 0)
      {
        a = (number) ((NATNUMBER) a / 2);
        b = (number) ((NATNUMBER) b / 2);
      }
    }
    if ((NATNUMBER) b%2 == 0)
    {
      WerrorS("Division not possible, even by cancelling zero divisors.");
      WerrorS("Result is integer division without remainder.");
      return (number) ((NATNUMBER) a / (NATNUMBER) b);
    }
  }
  return (number) nr2mMult(a, nr2mInversM(b));
}

number nr2mMod (number a, number b)
{
  /*
    We need to return the number r which is uniquely determined by the
    following two properties:
      (1) 0 <= r < |b| (with respect to '<' and '<=' performed in Z x Z)
      (2) There exists some k in the integers Z such that a = k * b + r.
    Consider g := gcd(2^m, |b|). Note that then |b|/g is a unit in Z/2^m.
    Now, there are three cases:
      (a) g = 1
          Then |b| is a unit in Z/2^m, i.e. |b| (and also b) divides a.
          Thus r = 0.
      (b) g <> 1 and g divides a
          Then a = (a/g) * (|b|/g)^(-1) * b (up to sign), i.e. again r = 0.
      (c) g <> 1 and g does not divide a
          Let's denote the division with remainder of a by g as follows:
          a = s * g + t. Then t = a - s * g = a - s * (|b|/g)^(-1) * |b|
          fulfills (1) and (2), i.e. r := t is the correct result. Hence
          in this third case, r is the remainder of division of a by g in Z.
    This algorithm is the same as for the case Z/n, except that we may
    compute the gcd of |b| and 2^m "by hand": We just extract the highest
    power of 2 (<= 2^m) that is contained in b.
  */
  NATNUMBER g = 1;
  NATNUMBER b_div = (NATNUMBER)b;
  if (b_div < 0) b_div = - b_div; // b_div now represents |b|
  NATNUMBER r = 0;
  while ((g < currRing->nr2mModul ) && (b_div > 0) && (b_div % 2 == 0))
  {
    b_div = b_div >> 1;
    g = g << 1;
  } // g is now the gcd of 2^m and |b|

  if (g != 1) r = (NATNUMBER)a % g;
  return (number)r;
}

number nr2mIntDiv (number a,number b)
{
  if ((NATNUMBER)a==0)
  {
    if ((NATNUMBER)b==0)
      return (number) 1;
    if ((NATNUMBER)b==1)
      return (number) 0;
    return (number) (currRing->nr2mModul  / (NATNUMBER) b);
  }
  else
  {
    if ((NATNUMBER)b==0)
      return (number) 0;
    return (number) ((NATNUMBER) a / (NATNUMBER) b);
  }
}

number  nr2mInvers (number c)
{
  if ((NATNUMBER)c%2==0)
  {
    WerrorS("division by zero divisor");
    return (number)0;
  }
  return nr2mInversM(c);
}

number nr2mNeg (number c)
{
  if ((NATNUMBER)c==0) return c;
  return nr2mNegM(c);
}

number nr2mMapMachineInt(number from)
{
  NATNUMBER i = ((NATNUMBER) from) % currRing->nr2mModul ;
  return (number) i;
}

number nr2mMapZp(number from)
{
  long ii = (long) from;
  while (ii < 0) ii += currRing->nr2mModul ;
  while ((ii>1) && (ii >= currRing->nr2mModul )) ii -= currRing->nr2mModul ;
  return (number) ii;
}

number nr2mMapQ(number from)
{
  int_number erg = (int_number) omAlloc(sizeof(MP_INT)); // evtl. spaeter mit bin
  mpz_init(erg);

  nlGMP(from, (number) erg);
  mpz_mod_ui(erg, erg, currRing->nr2mModul );
  number r = (number) mpz_get_ui(erg);

  mpz_clear(erg);
  omFree((ADDRESS) erg);
  return (number) r;
}

number nr2mMapGMP(number from)
{
  int_number erg = (int_number) omAlloc(sizeof(MP_INT)); // evtl. spaeter mit bin
  mpz_init(erg);

  mpz_mod_ui(erg, (int_number) from, currRing->nr2mModul );
  number r = (number) mpz_get_ui(erg);

  mpz_clear(erg);
  omFree((ADDRESS) erg);
  return (number) r;
}

nMapFunc nr2mSetMap(const ring src, const ring dst)
{
  if (rField_is_Ring_2toM(src)
     && (src->ringflagb >= dst->ringflagb))
  {
    return nr2mMapMachineInt;
  }
  if (rField_is_Ring_Z(src))
  {
    return nr2mMapGMP;
  }
  if (rField_is_Q(src))
  {
    return nr2mMapQ;
  }
  if (rField_is_Zp(src)
     && (src->ch == 2)
     && (dst->ringflagb == 1))
  {
    return nr2mMapZp;
  }
  if (rField_is_Ring_PtoM(src) || rField_is_Ring_ModN(src))
  {
    // Computing the n of Z/n
    int_number modul = (int_number) omAlloc(sizeof(MP_INT)); // evtl. spaeter mit bin
    mpz_init(modul);
    mpz_set(modul, src->ringflaga);
    mpz_pow_ui(modul, modul, src->ringflagb);
    if (mpz_divisible_2exp_p(modul, dst->ringflagb))
    {
      mpz_clear(modul);
      omFree((ADDRESS) modul);
      return nr2mMapGMP;
    }
    mpz_clear(modul);
    omFree((ADDRESS) modul);
  }
  return NULL;      // default
}

/*
 * set the exponent (allocate and init tables) (TODO)
 */

void nr2mSetExp(int m, const ring r)
{
  if (m>1)
  {
    nr2mExp = m;
    r->nr2mModul  = 2;
    for (int i = 1; i < m; i++)
    {
      r->nr2mModul  = r->nr2mModul  * 2;
    }
  }
  else
  {
    nr2mExp=2;
    r->nr2mModul =4;
  }
}

void nr2mInitExp(int m, const ring r)
{
  nr2mSetExp(m, r);
  if (m<2) WarnS("nInitExp failed: using Z/2^2");
}

#ifdef LDEBUG
BOOLEAN nr2mDBTest (number a, const char *f, const int l)
{
  if (((NATNUMBER)a<0) || ((NATNUMBER)a>currRing->nr2mModul ))
  {
    return FALSE;
  }
  return TRUE;
}
#endif

void nr2mWrite (number &a, const ring r)
{
  if ((NATNUMBER)a > (r->nr2mModul  >>1))
     StringAppend("-%d",(int)(r->nr2mModul -((NATNUMBER)a)));
  else                          StringAppend("%d",(int)((NATNUMBER)a));
}

static const char* nr2mEati(const char *s, int *i)
{

  if (((*s) >= '0') && ((*s) <= '9'))
  {
    (*i) = 0;
    do
    {
      (*i) *= 10;
      (*i) += *s++ - '0';
      if ((*i) >= (MAX_INT_VAL / 10)) (*i) = (*i) % currRing->nr2mModul ;
    }
    while (((*s) >= '0') && ((*s) <= '9'));
    (*i) = (*i) % currRing->nr2mModul ;
  }
  else (*i) = 1;
  return s;
}

const char * nr2mRead (const char *s, number *a)
{
  int z;
  int n=1;

  s = nr2mEati(s, &z);
  if ((*s) == '/')
  {
    s++;
    s = nr2mEati(s, &n);
  }
  if (n == 1)
    *a = (number)z;
  else
      *a = nr2mDiv((number)z,(number)n);
  return s;
}
#endif
