#include "mod2.h"

#ifdef HAVE_VANISHING_IDEAL

#include <kernel/structs.h>
#include <kernel/polys.h>
#include <kernel/ideals.h>
#include <kernel/febase.h>
#include "VanishingIdeal.h"
#include "FactoredNumber.h"

#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
/* define a special indentation string which precedes any
   output that is due to PRINT_VANISHING_IDEAL_OPERATIONS */
char* myPrintIndent = "   ~~~>";
int counterCancel = 0;
int counterCheckedInsert = 0;
int counterTimes = 0;
int counterDivides = 0;
int counterPAlphaA = 0;
int counterPolyAdd = 0;
int counterPolyMult = 0;
int counterGcd = 0;
#endif

using namespace std;

void usageVanishingIdealCode ()
{
  printf("\nType 'system(\"vanishingIdeal\", \"direct\");' to run the algorithm");
  printf("\nfor the direct generation of a strong Groebner basis of the ideal");
  printf("\nof vanishing polynomials in the given ring. This ring is assumed to");
  printf("\nbe a polynomial ring with global ordering over Z/m for some m >= 2.");
  printf("\nThe command returns the ideal generated by this strong Groebner");
  printf("\nbasis.\n");
  printf("\nType 'system(\"vanishingIdeal\", \"recursive\");' to run the algorithm");
  printf("\nfor the recursive generation of a strong Groebner basis of the ideal");
  printf("\nof vanishing polynomials in the given ring. This ring is assumed to");
  printf("\nbe a polynomial ring with global ordering over Z/m for some m >= 2.");
  printf("\nThe command returns the ideal generated by this strong Groebner");
  printf("\nbasis which should be the same as the result of the first command.");
  printf("\nWith this algorithm, the Groebner basis will be constructed along");
  printf("\nthe prime factorization of m.\n");
  printf("\nType 'system(\"vanishingIdeal\", \"normalForm\", f);' to obtain the");
  printf("\nnormal form of a given polynomial f with respect to the strong");
  printf("\nGroebner basis as computed by the first two commands. Note that");
  printf("\nthis method does not explicitely generate the Groebner basis.\n");
  printf("\nType 'system(\"vanishingIdeal\", \"isZeroFunction\", f);' to obtain");
  printf("\n1 if and only if the given polynomial f evaluates to zero for all");
  printf("\ntuples of values. Otherwise, 0 is returned. Instead of computing");
  printf("\nthe normal form and checking whether it is zero, this command");
  printf("\nmakes use of an alternative algorithm first proposed by P. Kalla.\n");
  printf("\nType 'system(\"vanishingIdeal\", \"nonZeroTuple\", f);' to obtain");
  printf("\na value for each ring variable such that the given polynomial f");
  printf("\ndoes not evaluate to zero for this tuple. The tuple is returned");
  printf("\nas an intvec. If there is no such tuple, i.e., if f is a zero");
  printf("\nfunction, then an intvec with each entry equal to '-1' will be");
  printf("\nreturned. This algorithm works similar to Kalla's algorithm and");
  printf("\nmay thus be used instead of the above 'isZeroFunction'-command.\n");
  printf("\nType 'system(\"vanishingIdeal\", \"smarandache\", n);' to obtain");
  printf("\nthe value of the Smarandache function for the given natural");
  printf("\nnumber n, n >= 1. This function computes, for a given n, the");
  printf("\nsmallest natural number k such that n divides k!.\n\n");
}

int binaryPowerMod (const int a, const int b, const int m)
{
  /* computes a^b mod m according to the binary representation of b,
     i.e., a^7 = a^4 * a^2 * a^1. This saves some multiplications. */
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

poly p_alpha_a (const int* alpha, const int a)
{
  /* assumes that there are valid entries alpha[0 .. n-1], all >= 0,
     where n is the number of variables in the current ring */
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

poly normalForm (const poly f)
{
  int n = currRing->N;
  int m = rChar(currRing);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  counterTimes = 0;
  counterGcd = 0;
  counterCancel = 0;
  counterPolyAdd = 0;
  counterPAlphaA = 0;
#endif

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
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    for (int i = 0; i < n; i++) if (alpha[i] >= 2) counterTimes += alpha[i] - 1;
    counterGcd++;
    counterCancel++;
#endif
    r = a % c;   // remainder of division of a by c in N
    ff = p_Add_q(ff, pNeg(p_alpha_a(alpha, a - r)), currRing);                   // f = f - p(alpha, a - r);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    counterPAlphaA++;
    counterPolyAdd++;
#endif
    if (r != 0)
    {
      rTimesXToTheAlpha = pISet(r);
      for (int i = 0; i < n; i++) pSetExp(rTimesXToTheAlpha, i + 1, alpha[i]);
      pSetm(rTimesXToTheAlpha);
      h = p_Add_q(h, pCopy(rTimesXToTheAlpha), currRing);                        // h = h + r * x^alpha
      ff = p_Add_q(ff, pNeg(rTimesXToTheAlpha), currRing);                       // f = f - r * x^alpha
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
      counterPolyAdd += 2;
#endif
    }
  }
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterTimes,   "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterGcd,     "FactoredNumber gcd (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterCancel,  "FactoredNumber cancel (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterPolyAdd, "p_Add_q");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterPAlphaA, "p_alpha_a");
  printf("\n");
#endif
  return h;
}

/* expects m >= 1 */
int smarandache (const int m)
{
  int s = FactoredNumber(m).smarandache();
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, s, "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, s, "FactoredNumber::divides (const FactoredNumber& fn) const");
  printf("\n");
#endif
  return s;
}

void checkedInsert (ideal& iii, const int n, const FactoredNumber& m,
                    const int* alpha, const FactoredNumber& a, const FactoredNumber& facForAlpha)
{
  /* check for minimality of alpha (i.e. there is no smaller beta such that m already divides a * beta!) and,
     check whether entry already present in iii,
     in case all checks are positive, p_alpha_a will be inserted in iii */
  bool alphaMinimal = true;
  int i = 0;
  while ((i < n) && (alphaMinimal))
  {
    if ((alpha[i] > 0) && (m.cancel(facForAlpha.cancel(FactoredNumber(alpha[i]))) == a))
      alphaMinimal = false;
    i++;
  }
  if (alphaMinimal)
    idInsertPolyWithTests(iii, IDELEMS(iii), p_alpha_a(alpha, a.getInt()), false, false);
  return;
}

void checkedInsertTerm (ideal& iii, poly& term, const int n, const FactoredNumber& m,
                        const int* alpha, const FactoredNumber& a, const FactoredNumber& facForAlpha)
{
  /* check for minimality of alpha (i.e. there is no smaller beta such that m already divides a * beta!) and,
     check whether entry already present in iii,
     in case all checks are positive, the given term will be inserted in iii */
  bool alphaMinimal = true;
  int i = 0;
  while ((i < n) && (alphaMinimal))
  {
    if ((alpha[i] > 0) && (m.cancel(facForAlpha.cancel(FactoredNumber(alpha[i]))) == a))
      alphaMinimal = false;
    i++;
  }
  if (alphaMinimal)
    idInsertPolyWithTests(iii, IDELEMS(iii), pCopy(term), false, false);
  return;
}

void gBForVanishingIdealDirect_Helper (ideal& iii, const int n, const FactoredNumber& m, const int s, int varIndex, int* alpha)
{
  FactoredNumber oldA(m);
  FactoredNumber fac = FactoredNumber::factorial(n, alpha);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  for (int i = 0; i < n; i++) if (alpha[i] >= 2) counterTimes += alpha[i] - 1;
#endif
  FactoredNumber a(1);
  while (alpha[varIndex] <= s)
  {
    a = m.cancel(fac);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    counterCancel++;
#endif
    if ((a != m) && (a != oldA))
      checkedInsert(iii, n, m, alpha, a, fac);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    counterCheckedInsert++;
#endif
    if (a == FactoredNumber(1)) break;
    else if (varIndex + 1 < n)
    {
      gBForVanishingIdealDirect_Helper(iii, n, m, s, varIndex + 1, alpha);
      alpha[varIndex + 1] = 0;
    }
    oldA = a;
    alpha[varIndex]++;
    fac = fac * FactoredNumber(alpha[varIndex]);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    counterTimes++;
#endif
  }
}

ideal gBForVanishingIdealDirect ()
{
  int n = currRing->N;
  int mm = rChar(currRing);
  FactoredNumber m(mm);
  int s = FactoredNumber::FactoredNumber(mm).smarandache();
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  counterTimes = s;
  counterDivides = s;
  counterCheckedInsert = 0;
  counterCancel = 0;
#endif
  int alpha[n];
  for (int i = 0; i < n; i++) alpha[i] = 0;

  ideal iii = idInit(1, 0);
  gBForVanishingIdealDirect_Helper(iii, n, m, s, 0, alpha);
  /* remove zero generators (resulting from block-wise allocation of memory): */
  idSkipZeroes(iii);

#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterTimes,         "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterDivides,       "FactoredNumber::divides (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterCancel,        "FactoredNumber cancel (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterCheckedInsert, "checked insert into target ideal");
  printf("\n");
#endif
  return iii;
}

ideal gBForVanishingIdealRecursive_Helper (const int n, const FactoredNumber& m)
{
  /* this method returns an ideal with one-term generators;
     these terms contain all information to generate the appropriate
     p(alpha, a), i.e. they have the coefficient a and the exponent vector alpha */
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
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
    counterCancel++;
#endif
    ideal jjj = gBForVanishingIdealRecursive_Helper(n, mDividedByQ);
    for (int j = 0; j < IDELEMS(jjj); j++)
    {
      poly myTerm = jjj->m[j];
      int a = n_Int(pGetCoeff(myTerm), currRing);
      int alpha[n];
      for (int i = 0; i < n; i++) alpha[i] = pGetExp(myTerm, i + 1);
      FactoredNumber aa(a);
      FactoredNumber fac = FactoredNumber::factorial(n, alpha);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
      for (int i = 0; i < n; i++) if (alpha[i] >= 2) counterTimes += alpha[i] - 1;
      counterTimes++;
      counterDivides++;
#endif
      if (m.divides(aa * fac))
      {
        checkedInsertTerm(iii, myTerm, n, m, alpha, aa, fac);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
        counterCheckedInsert++;
#endif
      }
      else
      {
        poly pp = p_Mult_q(pCopy(myTerm), pISet(q), currRing);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
        counterPolyMult++;
#endif
        checkedInsertTerm(iii, pp, n, m, alpha, aa, fac);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
        counterCheckedInsert++;
#endif
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
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
            counterTimes++;
#endif
          }
          FactoredNumber bb = m.cancel(fac1);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
          counterCancel++;
#endif
          poly myNewTerm = pISet(bb.getInt());
          for (int t = 0; t < n; t++)
            pSetExp(myNewTerm, t + 1, alpha[t]);
          pSetm(myNewTerm);
          checkedInsertTerm(iii, myNewTerm, n, m, alpha, bb, fac1);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
          counterCheckedInsert++;
#endif
          pDelete(&myNewTerm);
          alpha[i] = alpha_i;
        }
      }
    }
    idDelete(&jjj);
  }

  /* remove zero generators (resulting from block-wise allocation of memory): */
  idSkipZeroes(iii);
  return iii;
}

ideal gBForVanishingIdealRecursive ()
{
  int n = currRing->N;
  int mm = rChar(currRing);
  FactoredNumber m(mm);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  counterTimes = 0;
  counterCheckedInsert = 0;
  counterCancel = 0;
  counterDivides = 0;
  counterPolyMult = 0;
#endif
  /* The next line computes an ideal with one-term generators.
     They just have the right coefficients and exponent vectors
     but we still need to build the right p_alpha_a's from these. */
  ideal iii = gBForVanishingIdealRecursive_Helper(n, m);
  /* We now have in iii terms of the form a*x^alpha.
     These data will now be used to create the right p(alpha, a)
     and finally set up the desired Groebner basis in jjj. */
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
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterTimes,         "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterDivides,       "FactoredNumber::divides (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterCancel,        "FactoredNumber cancel (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterPolyMult,      "p_Mult_q");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterCheckedInsert, "checked insert into target ideal");
  printf("\n");
#endif
  return jjj;
}

poly substitute (const poly& p, const int varIndex, const int varValue, const int m)
{
  /* substitutes a value for a variable and returns the resulting
     polynomial in (at least) one less variable;
     All computations are in Z/m.
     Let x denote the variable, then the powers of x are computed
     according to the binary representation of the exponent, i.e.
     x^7 = x^4 * x^2 * x^1. This saves some multiplications.
     The method assumes 1 <= varIndex <= number of ring variables. */
  poly result = NULL;
  poly pp = pCopy(p);
  int exp = 0;
  int power = 0;
  int coeff = 0;
  poly newTerm = NULL;
  while (pp != NULL)
  {
    exp = pGetExp(pp, varIndex);
    power = binaryPowerMod(varValue, exp, m);
    coeff = n_Int(pGetCoeff(pp), currRing);
    coeff = (coeff * power) % m;
    if (coeff != 0)
    {
      newTerm = pCopy(pHead(pp));
      pSetCoeff(newTerm, nInit(coeff));
      pSetExp(newTerm, varIndex, 0);
      pSetm(newTerm);
      result = p_Add_q(result, newTerm, currRing);
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
      counterPolyAdd++;
#endif
    }
    pp = pNext(pp);
  }
  return result;
}

bool isZeroFunction_Helper(const poly& f, const int n, const int m, const int lambda, int* values, int* nonZero, const int varIndex)
{
  /* This method assumes 1 <= varIndex <= n. */
  if      (f == NULL)    return true;
  else if (varIndex > n) return false;
  else
  {
    bool isZeroFct = true;
    while ((values[varIndex - 1] < lambda) && isZeroFct)
    {
      poly p = substitute(f, varIndex, values[varIndex - 1], m);
      isZeroFct = isZeroFunction_Helper(p, n, m, lambda, values, nonZero, varIndex + 1);
      pDelete(&p);
      nonZero[varIndex - 1] = values[varIndex - 1];
      values[varIndex - 1]++;
    }
    values[varIndex - 1] = 0;
    return isZeroFct;
  }
}

bool isZeroFunction (const poly f)
{
  /* This method implements Kalla's algorithm.
     Basically, a big-enough set of tuples must be tested. If f
     vanishes for all of these, then f is guaranteed to be the zero function. */
  int m = rChar(currRing);
  int n = currRing->N;
  int lambda = FactoredNumber::FactoredNumber(m).smarandache();
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  counterTimes = lambda;
  counterDivides = lambda;
  counterPolyAdd = 0;
#endif
  int* values  = new int[n]; for (int i = 0; i < n; i++) values[i]  = 0;
  int* nonZero = new int[n]; /* not used in this method */
  bool result = isZeroFunction_Helper(f, n, m, lambda, values, nonZero, 1);
  delete [] values;
  delete [] nonZero;
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterTimes,   "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterDivides, "FactoredNumber::divides (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterPolyAdd, "p_Add_q");
  printf("\n");
#endif
  return result;
}

int* nonZeroTuple (const poly f)
{ /* This method implements Kalla's algorithm but - this time - returns
     the first tuple it finds, for which f does not vanish. */
  int m = rChar(currRing);
  int n = currRing->N;
  int lambda = FactoredNumber::FactoredNumber(m).smarandache();
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  counterTimes = lambda;
  counterDivides = lambda;
  counterPolyAdd = 0;
#endif
  int* values =  new int[n]; for (int i = 0; i < n; i++) values[i]  = 0;
  int* nonZero = new int[n]; for (int i = 0; i < n; i++) nonZero[i] = 0;
  bool result = isZeroFunction_Helper(f, n, m, lambda, values, nonZero, 1);
  delete [] values;
  if (result)
  {
    /* i.e. f is the zero function; hence, there is no such tuple
       and we return the special int array with all entries = -1: */
    for (int i = 0; i < n; i++) nonZero[i] = -1;
  }
#ifdef PRINT_VANISHING_IDEAL_OPERATIONS
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterTimes,   "FactoredNumber::operator* (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterDivides, "FactoredNumber::divides (const FactoredNumber& fn) const");
  printf("\n%sperformed %d invocations of '%s'", myPrintIndent, counterPolyAdd, "p_Add_q");
  printf("\n");
#endif
  return nonZero;
}

#endif
/* HAVE_VANISHING_IDEAL */
