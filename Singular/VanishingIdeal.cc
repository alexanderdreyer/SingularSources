#include "mod2.h"

#ifdef HAVE_VANISHING_IDEAL

#include <kernel/structs.h>
#include <kernel/polys.h>
#include <kernel/ideals.h>
#include <kernel/febase.h>
#include "VanishingIdeal.h"
#include "FactoredNumber.h"

using namespace std;

ideal gBForVanishingIdealDirect (const bool printOperationDetails)
{
  ideal iii = idInit(1, 0);
  idInsertPoly(iii, pISet(17)); // will include po only if it is not the zero polynomial
  idSkipZeroes(iii);  // remove zero generators (resulting from block-wise allocation of memory)
  return iii;
}

ideal gBForVanishingIdealRecursive (const bool printOperationDetails)
{
  ideal iii = idInit(1, 0);
  idInsertPoly(iii, pISet(3)); // will include po only if it is not the zero polynomial
  idSkipZeroes(iii);  // remove zero generators (resulting from block-wise allocation of memory)
  return iii;
}

poly normalForm (const poly f, const bool printOperationDetails)
{
  poly p = NULL;
  return p;
}

bool isZeroFunction (const poly f, const bool printOperationDetails)
{
  return false;
}

/* expects a >= 1, b >= 1 */
int helper_gcd (const int a, const int b)
{
  if ((a == 1) || (b == 1)) return 1;
  int m = (a > b) ? a : b;
  int n = (a > b) ? b : a;
  int temp = 0;
  /* m >= n */
  while ((n != 1) && (n != 0))
  {
    temp = m - n;
    m = (temp > n) ? temp : n;
    n = (temp > n) ? n : temp;
  }
  if (n == 1) return 1;
  if (n == 0) return m;
}

/* expects m >= 1 */
int smarandache (const int m, const bool printOperationDetails)
{
  return FactoredNumber(m).smarandache();
}

#endif
/* HAVE_VANISHING_IDEAL */
