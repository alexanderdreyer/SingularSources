#include "mod2.h"

#ifdef HAVE_VANISHING_IDEAL

#include <kernel/structs.h>
#include <kernel/polys.h>
#include <kernel/ideals.h>
#include <kernel/febase.h>
#include "VanishingIdeal.h"
#include "FactoredNumber.h"

using namespace std;

/* assumes that there are valid entries alpha[0 .. n-1], where
   n is the number of variables in the current ring */
poly p_alpha_a (const int* alpha, const int a)
{
  int n = currRing->N;
  poly result = pISet(a);
  for (int i = 1; i <= n; i++)
    {
      poly x_i = pISet(1);
      pSetExp(x_i, i, 1);
      pSetm(x_i);
      for (int j = 1; j <= alpha[i - 1]; j++)
      {
        poly temp = p_Add_q(pCopy(x_i), pNeg(pISet(j)), currRing);   // this is 'x_i - j'
        result = p_Mult_q(result, temp, currRing);
      }
      pDelete(&x_i);
    }
  return result;
}

int getM()
{
  int m = 0;
  if      (rField_is_Ring_ModN(currRing)) m = (int)mpz_get_ui(currRing->ringflaga);
  else if (rField_is_Ring_2toM(currRing)) m = FactoredNumber::power(2, (int)(unsigned long)currRing->ringflagb);
  else if (rField_is_Ring_PtoM(currRing)) m = FactoredNumber::power((int)mpz_get_ui(currRing->ringflaga),
                                                                    (int)(unsigned long)currRing->ringflagb);
  return m;
}

poly normalForm (const poly f, const bool printOperationDetails)
{
  int n = currRing->N;
  int m = getM();

  poly h = NULL;
  poly ff = pCopy(f);
  poly rTimesXToTheAlpha = NULL;
  int a;
  int c;
  int k;
  int r;
  int alpha[n];
  while (ff != NULL)
  {
    a = n_Int(pGetCoeff(ff), currRing);
    for (int i = 0; i < n; i++)
      alpha[i] = pGetExp(ff, i + 1);
    FactoredNumber fn = FactoredNumber::factorial(n, alpha);
    FactoredNumber mm = FactoredNumber(m);
    c = mm.cancel(fn.gcd(mm)).getInt();
    r = a % c;   // remainder of division of a by c in N
    ff = p_Add_q(ff, pNeg(p_alpha_a(alpha, a - r)), currRing);                   // f = f - p(alpha, a - r);
    if (r != 0)
    {
      rTimesXToTheAlpha = pISet(r);
      for (int i = 0; i < n; i++) pSetExp(rTimesXToTheAlpha, i + 1, alpha[i]);
      pSetm(rTimesXToTheAlpha);
      h = p_Add_q(h, pCopy(rTimesXToTheAlpha), currRing);                        // h = h + r * x^alpha
      ff = p_Add_q(ff, pNeg(rTimesXToTheAlpha), currRing);                       // f = f - r * x^alpha
    }
  }
  return h;
}

/* expects m >= 1 */
int smarandache (const int m, const bool printOperationDetails)
{
  return FactoredNumber(m).smarandache();
}

/* check for minimality of alpha and,
   check whether entry already present in iii,
   in case all checks are positive, p_alpha_a
   will be inserted in iii */
void checkedInsert (ideal& iii, const int n, const FactoredNumber& m,
                    const int* alpha, const FactoredNumber& a, const FactoredNumber& facForAlpha)
{
  bool alphaMinimal = true;
  int i = 0;
  /*
  printf("\nalpha = (");
  for (int j = 0; j < n; j++) { if (j != 0) printf(", "); printf("%d", alpha[j]); }
  printf("), a = %s", a.toString());
  */
  while ((i < n) && (alphaMinimal))
  {
    if ((alpha[i] > 0) && (m.cancel(facForAlpha.cancel(FactoredNumber(alpha[i]))) == a))
      alphaMinimal = false;
    i++;
  }
  /* if (alphaMinimal) printf(" ===> candidate"); */
  if (alphaMinimal)
    idInsertPolyNoDuplicates(iii, p_alpha_a(alpha, a.getInt()));
  return;
}

/* check for minimality of alpha and,
   check whether entry already present in iii,
   in case all checks are positive, the given
   term will be inserted in iii */
void checkedInsertTerm (ideal& iii, poly& term, const int n, const FactoredNumber& m,
                        const int* alpha, const FactoredNumber& a, const FactoredNumber& facForAlpha)
{
  bool alphaMinimal = true;
  int i = 0;
  /*
  printf("\nalpha = (");
  for (int j = 0; j < n; j++) { if (j != 0) printf(", "); printf("%d", alpha[j]); }
  printf("), a = %s", a.toString());
  */
  while ((i < n) && (alphaMinimal))
  {
    if ((alpha[i] > 0) && (m.cancel(facForAlpha.cancel(FactoredNumber(alpha[i]))) == a))
      alphaMinimal = false;
    i++;
  }
  /* if (alphaMinimal) printf(" ===> candidate"); */
  if (alphaMinimal)
    idInsertPolyNoDuplicates(iii, pCopy(term));
  return;
}

void gBForVanishingIdealDirect_Helper (ideal& iii, const int n, const FactoredNumber& m, const int s, int varIndex, int* alpha)
{
  FactoredNumber oldA(m);
  FactoredNumber fac = FactoredNumber::factorial(n, alpha);
  FactoredNumber a(1);
  while (alpha[varIndex] <= s)
  {
    a = m.cancel(fac);
    if ((a != m) && (a != oldA))
      checkedInsert(iii, n, m, alpha, a, fac);
    if (a == FactoredNumber(1)) break;
    else if (varIndex + 1 < n)
    {
      gBForVanishingIdealDirect_Helper(iii, n, m, s, varIndex + 1, alpha);
      alpha[varIndex + 1] = 0;
    }
    oldA = a;
    alpha[varIndex]++;
    fac = fac * FactoredNumber(alpha[varIndex]);
  }
}

ideal gBForVanishingIdealDirect (const bool printOperationDetails)
{
  int n = currRing->N;
  int mm = getM();
  FactoredNumber m(mm);
  int s = smarandache(mm, 0);
  int alpha[n];
  for (int i = 0; i < n; i++) alpha[i] = 0;

  ideal iii = idInit(1, 0);
  gBForVanishingIdealDirect_Helper(iii, n, m, s, 0, alpha);
  idSkipZeroes(iii);  /* remove zero generators (resulting from block-wise allocation of memory) */

  return iii;
}

/* returns an ideal with one-term generators */
ideal gBForVanishingIdealRecursive_Helper (const int n, const FactoredNumber& m)
{
  ideal iii = idInit(1, 0);
  int q = m.getSmallestP();
  if (m == FactoredNumber(q))
  {
    for (int i = 1; i <= n; i++)
    {
      poly myMonomial = pISet(1);
      pSetExp(myMonomial, i, q);
      pSetm(myMonomial);
      /* Here, we fill in the first polynomials, and they are
         guaranteed to form a GB for I0 in Z/q. In particular, no
         further checks are needed. */
      idInsertPoly(iii, myMonomial);
    }
  }
  else
  {
    FactoredNumber mDividedByQ = m.cancel(FactoredNumber(q));
    ideal jjj = gBForVanishingIdealRecursive_Helper(n, mDividedByQ);
    for (int j = 0; j < IDELEMS(jjj); j++)
    {
      poly myTerm = jjj->m[j];
      int a = n_Int(pGetCoeff(myTerm), currRing);
      int alpha[n];
      for (int i = 0; i < n; i++) alpha[i] = pGetExp(myTerm, i + 1);
      FactoredNumber aa(a);
      FactoredNumber fac = FactoredNumber::factorial(n, alpha);
      if (m.divides(aa * fac))
        checkedInsertTerm(iii, myTerm, n, m, alpha, aa, fac);
      else
      {
        poly pp = p_Mult_q(pCopy(myTerm), pISet(q), currRing);
        checkedInsertTerm(iii, pp, n, m, alpha, aa, fac);
        pDelete(&pp);
        for (int i = 0; i < n; i++)
        {
          int alpha_i = alpha[i];
          FactoredNumber fac1(fac);
          int e = fac1.getExponent(q);
          while (fac1.getExponent(q) == e)
          {
            alpha[i]++;
            fac1 = fac1 * FactoredNumber(alpha[i]);
          }
          FactoredNumber bb = m.cancel(fac1);
          poly myNewTerm = pISet(bb.getInt());
          for (int t = 0; t < n; t++)
            pSetExp(myNewTerm, t + 1, alpha[t]);
          pSetm(myNewTerm);
          checkedInsertTerm(iii, myNewTerm, n, m, alpha, bb, fac1);
          pDelete(&myNewTerm);
          alpha[i] = alpha_i;
        }
      }
    }
    idDelete(&jjj);
  }

  idSkipZeroes(iii);  /* remove zero generators (resulting from block-wise allocation of memory) */
  return iii;
}

ideal gBForVanishingIdealRecursive (const bool printOperationDetails)
{
  int n = currRing->N;
  int mm = getM();
  FactoredNumber m(mm);
  /* The next line computes an ideal with one-term generators.
     They just have the right coefficients and exponent vectors
     but we still need to build the right p_alpha_a's from these! */
  ideal iii = gBForVanishingIdealRecursive_Helper(n, m);
  ideal jjj = idInit(1, 0);
  for (int i = 0; i < IDELEMS(iii); i++)
  {
    poly myTerm = iii->m[i];
    int a = n_Int(pGetCoeff(myTerm), currRing);
    int alpha[n];
    for (int i = 0; i < n; i++) alpha[i] = pGetExp(myTerm, i + 1);
    idInsertPoly(jjj, p_alpha_a(alpha, a));
  }
  idSkipZeroes(jjj);  /* remove zero generators (resulting from block-wise allocation of memory) */
  return jjj;
}

/* computes a^b mod m according to the binary representation of b,
   i.e. a^7 = a^4 * a^2 * a^1. This saves some multiplications. */
int binaryPower (const int a, const int b, const int m)
{
  int result = 1;
  int factor = a;
  int bb = b;
  while (bb != 0)
  {
    if (bb % 2 != 0) result = (result * factor) % m;
    bb = bb / 2;
    factor = (factor * factor) % m;
  }
  return result;
}

/* substitutes a value for a variable and returns the resulting
   polynomial in (at least) one less variable;
   All computations are in Z/m.
   Let x denote the variable, then the powers of x are computed
   according to the binary representation of the exponent, i.e.
   x^7 = x^4 * x^2 * x^1. This saves some multiplications.
   1 <= varIndex <= number of variables */
poly substitute (const poly& p, const int varIndex, const int varValue, const int m)
{
  poly result = NULL;
  poly pp = pCopy(p);
  int exp = 0;
  int power = 0;
  int coeff = 0;
  poly newTerm = NULL;
  while (pp != NULL)
  {
    exp = pGetExp(pp, varIndex);
    power = binaryPower(varValue, exp, m);
    coeff = n_Int(pGetCoeff(pp), currRing);
    coeff = (coeff * power) % m;
    if (coeff != 0)
    {
      newTerm = pCopy(pHead(pp));
      pSetCoeff(newTerm, nInit(coeff));
      pSetExp(newTerm, varIndex, 0);
      pSetm(newTerm);
      result = p_Add_q(result, newTerm, currRing);
    }
    pp = pNext(pp);
  }
  return result;
}

/* Kalla's algorithm;
   1 <= varIndex <= n */
bool isZeroFunction_Helper(const poly f, const int n, const int m, const int varIndex)
{
  if (varIndex > n) return (f == NULL);
  else
  {
    
  }
}

bool isZeroFunction (const poly f, const bool printOperationDetails)
{
  int m = getM();
  int n = currRing->N;
  return isZeroFunction_Helper(f, n, m, 1);
}

#endif
/* HAVE_VANISHING_IDEAL */
