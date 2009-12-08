#include "mod2.h"

#ifdef HAVE_VANISHING_IDEAL

#include "factory.h"
#include <list>
#include "FactoredNumber.h"

using namespace std;

void FactoredNumber::insertPrime (list<int>& primes, list<int>& exponents, const int p)
{
  list<int>::iterator itP = primes.begin();
  list<int>::iterator itExp = exponents.begin();
  while ((itP != primes.end()) && (*itP < p))
  {
    itP++;
    itExp++;
  }
  if (*itP == p)
  {
    /* increment exponent of p at current position */
    int exp = *itExp;
    itExp = exponents.erase(itExp);
    exponents.insert(itExp, exp + 1);
  }
  else
  {
    /* we have itP == primes.end();
       thus insert p^1 at the end with exponent = 1*/
    primes.insert(itP, p);
    exponents.insert(itExp, 1);
  }
}

FactoredNumber::FactoredNumber (const int n)
{
  m_primes.clear();
  m_exponents.clear();
  int nn = n;
  int i = 0;
  int p;
  /* Note that for nn <= 1, the created FactoredNumber will anyway
     represent the natural number 1. */
  while (nn > 1)
  {
    if (nn % 2 == 0)
    {
      insertPrime(m_primes, m_exponents, 2);
      nn = nn / 2;
    }
    else
    {
      i = 0;
      p = cf_getSmallPrime(0);
      while (nn % p != 0) {
        i++;
        p = cf_getSmallPrime(i);
      }
      insertPrime(m_primes, m_exponents, p);
      nn = nn / p;
    }
  }
}

FactoredNumber::FactoredNumber (const int p, const int exponent)
{
  m_primes.clear();
  m_exponents.clear();
  if (exponent > 0)
  {
    m_primes.insert(m_primes.begin(), p);
    m_exponents.insert(m_exponents.begin(), exponent);
  }
}

FactoredNumber::FactoredNumber (const FactoredNumber& fn)
{
  /* This implements a deep copy. */
  m_primes.clear();
  m_exponents.clear();
  list<int>::const_iterator fnItP;
  list<int>::const_iterator fnItExp = fn.m_exponents.begin();
  list<int>::iterator itP = m_primes.begin();
  list<int>::iterator itExp = m_exponents.begin();
  for (fnItP = fn.m_primes.begin(); fnItP != fn.m_primes.end(); fnItP++)
  {
    itP   = m_primes.insert(itP,   *fnItP);   itP++;
    itExp = m_primes.insert(itExp, *fnItExp); itExp++;
    fnItExp++;
  }
}

FactoredNumber::~FactoredNumber ()
{
  m_primes.clear();
  m_exponents.clear();
}

FactoredNumber FactoredNumber::operator* (const FactoredNumber& fn) const
{
  FactoredNumber result(1);
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  list<int>::iterator rItP = result.m_primes.begin();
  list<int>::iterator rItExp = result.m_exponents.begin();
  while ((itP != m_primes.end()) || (fItP != fn.m_primes.end()))
  {
    if ((itP == m_primes.end()) || (*fItP < *itP))
    {
      rItP   = result.m_primes.insert(rItP,   *fItP);   rItP++;   fItP++;
      rItExp = result.m_primes.insert(rItExp, *fItExp); rItExp++; fItExp++;
    }
    else if ((fItP == fn.m_primes.end()) || (*itP < *fItP))
    {
      rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;
      rItExp = result.m_primes.insert(rItExp, *itExp); rItExp++; itExp++;
    }
    else
    {
      rItP   = result.m_primes.insert(rItP,   *itP);             rItP++;   itP++;   fItP++;
      rItExp = result.m_primes.insert(rItExp, *itExp + *fItExp); rItExp++; itExp++; fItExp++;
    }
  }
  return result;
}

FactoredNumber FactoredNumber::cancel (const FactoredNumber& fn) const
{
  FactoredNumber result(1);
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  list<int>::iterator rItP = result.m_primes.begin();
  list<int>::iterator rItExp = result.m_exponents.begin();
  int newExp = 0;
  while ((itP != m_primes.end()) && (fItP != fn.m_primes.end()))
  {
    if (*fItP < *itP)
    {
      fItP++;
      fItExp++;
    }
    else if (*itP < *fItP)
    {
      rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;
      rItExp = result.m_primes.insert(rItExp, *itExp); rItExp++; itExp++;
    }
    else
    {
      newExp = *itExp - *fItExp;
      if (newExp > 0)
      {
        rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;
        rItExp = result.m_primes.insert(rItExp, newExp); rItExp++;
      }
      itP++;   fItP++;
      itExp++; fItExp++;
    }
  }
  while (itP != m_primes.end())
  {
    rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;
    rItExp = result.m_primes.insert(rItExp, *itExp); rItExp++; itExp++;
  }
  return result;
}

FactoredNumber FactoredNumber::gcd (const FactoredNumber& fn) const
{
  FactoredNumber result(1);
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  list<int>::iterator rItP = result.m_primes.begin();
  list<int>::iterator rItExp = result.m_exponents.begin();
  int minExp = 0;
  while ((itP != m_primes.end()) && (fItP != fn.m_primes.end()))
  {
    if (*fItP < *itP)
    {
      fItP++;
      fItExp++;
    }
    else if (*itP < *fItP)
    {
      itP++;
      itExp++;
    }
    else
    {
      minExp = (*itExp < *fItExp) ? *itExp : *fItExp;
      rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;   fItP++;
      rItExp = result.m_primes.insert(rItExp, minExp); rItExp++; itExp++; fItExp++;
    }
  }
  return result;
}

FactoredNumber FactoredNumber::lcm (const FactoredNumber& fn) const
{
  FactoredNumber result(1);
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  list<int>::iterator rItP = result.m_primes.begin();
  list<int>::iterator rItExp = result.m_exponents.begin();
  int maxExp = 0;
  while ((itP != m_primes.end()) || (fItP != fn.m_primes.end()))
  {
    if ((itP == m_primes.end()) || (*fItP < *itP))
    {
      rItP   = result.m_primes.insert(rItP,   *fItP);   rItP++;   fItP++;
      rItExp = result.m_primes.insert(rItExp, *fItExp); rItExp++; fItExp++;
    }
    else if ((fItP == fn.m_primes.end()) || (*itP < *fItP))
    {
      rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;
      rItExp = result.m_primes.insert(rItExp, *itExp); rItExp++; itExp++;
    }
    else
    {
      maxExp = (*itExp > *fItExp) ? *itExp : *fItExp;
      rItP   = result.m_primes.insert(rItP,   *itP);   rItP++;   itP++;   fItP++;
      rItExp = result.m_primes.insert(rItExp, maxExp); rItExp++; itExp++; fItExp++;
    }
  }
  return result;
}

FactoredNumber& FactoredNumber::operator= (const FactoredNumber& fn)
{
  /* This implements a deep copy. */
  m_primes.clear();
  m_exponents.clear();
  list<int>::const_iterator fnItP;
  list<int>::const_iterator fnItExp = fn.m_exponents.begin();
  list<int>::iterator itP = m_primes.begin();
  list<int>::iterator itExp = m_exponents.begin();
  for (fnItP = fn.m_primes.begin(); fnItP != fn.m_primes.end(); fnItP++)
  {
    itP   = m_primes.insert(itP,   *fnItP);   itP++;
    itExp = m_primes.insert(itExp, *fnItExp); itExp++;
    fnItExp++;
  }
  return *this;
}

bool FactoredNumber::divides (const FactoredNumber& fn) const
{
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  if (m_primes.size() > fn.m_primes.size()) return false;
  while ((itP != m_primes.end()) && (fItP != fn.m_primes.end()))
  {
    if (*itP < *fItP) return false;
    else if (*fItP < *itP)
    {
      fItP++;
      fItExp++;
    }
    else
    {
      if (*itExp > *fItExp) return false;
      itP++;   fItP++;
      itExp++; fItExp++;
    }
  }
  return (itP == m_primes.end());
}

int FactoredNumber::power (const int b, const int e)
{
  int result = 1;
  int factor = b;
  int ee = e;
  while (ee != 0)
  {
    if (ee % 2 != 0) result = result * factor;
    ee = ee / 2;
    factor = factor * factor;
  }
  return result;
}

int FactoredNumber::getInt () const
{
  list<int>::const_iterator itP;
  list<int>::const_iterator itExp = m_exponents.begin();
  int result = 1;
  for (itP = m_primes.begin(); itP != m_primes.end(); itP++)
  {
    result = result * power(*itP, *itExp);
    itExp++;
  }
  return result;
}

int FactoredNumber::getSmallestP () const
{
  list<int>::const_iterator itP = m_primes.begin();
  if (itP != m_primes.end()) return *itP;
  return 1;
}

int FactoredNumber::getExponent (const int p) const
{
  list<int>::const_iterator itP;
  list<int>::const_iterator itExp = m_exponents.begin();
  for (itP = m_primes.begin(); itP != m_primes.end(); itP++)
  {
    if (*itP == p) return *itExp;
    itExp++;
  }
  return 0;
}

char* FactoredNumber::toString () const
{
  /* The caller of this method is responsible for deleting
     the following char*. */
  char* h = new char[1000];
  if (m_primes.size() == 0)
  {
    strcpy(h, "1");
  }
  else
  {
    strcpy(h, "");
    list<int>::const_iterator itP;
    list<int>::const_iterator itExp = m_exponents.begin();
    bool firstTime = true;
    for (itP = m_primes.begin(); itP != m_primes.end(); itP++)
    {
      if (!firstTime) strcat(h, " * ");
      char s[10];
      sprintf(s, "%d^%d", *itP, *itExp);
      strcat(h, s);
      itExp++;
      firstTime = false;
    }
  }
  return h;
}

bool FactoredNumber::operator== (const FactoredNumber& fn) const
{
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  if (m_primes.size() != fn.m_primes.size()) return false;
  while (itP != m_primes.end())
  {
    if ((*itP != *fItP) || (*itExp != *fItExp)) return false;
    itP++;   fItP++;
    itExp++; fItExp++;
  }
  return true;
}

bool FactoredNumber::operator!= (const FactoredNumber& fn) const
{
  list<int>::const_iterator itP = m_primes.begin();
  list<int>::const_iterator itExp = m_exponents.begin();
  list<int>::const_iterator fItP = fn.m_primes.begin();
  list<int>::const_iterator fItExp = fn.m_exponents.begin();
  if (m_primes.size() != fn.m_primes.size()) return true;
  while (itP != m_primes.end())
  {
    if ((*itP != *fItP) || (*itExp != *fItExp)) return true;
    itP++;   fItP++;
    itExp++; fItExp++;
  }
  return false;
}

int FactoredNumber::smarandache () const
{
  int result = 1;
  FactoredNumber fac(1);
  /* We simply multiply until we have divisibility: */
  while (!this->divides(fac))
  {
    result++;
    fac = fac * FactoredNumber(result);
  }
  return result;
}

FactoredNumber FactoredNumber::factorial (int nn, int* n)
{
  FactoredNumber fac(1);
  for (int i = 0; i < nn; i++)
    for (int j = 2; j <= n[i]; j++)
      fac = fac * FactoredNumber(j);
  return fac;
}

void FactoredNumber::print () const
{
  char* temp = this->toString();
  printf("%s", temp);
  delete temp;
}

void FactoredNumber::printLn () const
{
  char* temp = this->toString();
  printf("%s\n", temp);
  delete temp;
}

#endif
/* HAVE_VANISHING_IDEAL */
