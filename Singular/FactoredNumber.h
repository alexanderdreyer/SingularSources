#ifndef FACTORED_NUMBER_H
#define FACTORED_NUMBER_H

#ifdef HAVE_VANISHING_IDEAL

#include <list>

class FactoredNumber
{
  private:
    std::list<int> m_primes;    // prime factors sorted in ascending order
    std::list<int> m_exponents; // corresponding exponents in prime factorization
    static void insertPrime (std::list<int>& primes, std::list<int>& exponents, const int p);
  public:
    FactoredNumber (const int n);
    FactoredNumber (const int p, const int exponent);
    FactoredNumber (const FactoredNumber& fn);
    ~FactoredNumber ();
    FactoredNumber operator* (const FactoredNumber& fn) const;
    FactoredNumber cancel (const FactoredNumber& fn) const;
    FactoredNumber& operator= (const FactoredNumber& fn);
    FactoredNumber gcd (const FactoredNumber& fn) const;
    FactoredNumber lcm (const FactoredNumber& fn) const;
    static FactoredNumber factorial (int nn, int* n);
    static int power (const int b, const int e);
    bool divides (const FactoredNumber& fn) const;
    bool operator== (const FactoredNumber& fn) const;
    bool operator!= (const FactoredNumber& fn) const;
    int smarandache () const;
    int getInt () const;
    int getSmallestP () const;
    int getExponent (const int p) const;
    char* toString () const;
    void print () const;
    void printLn () const;
};

#endif
/* HAVE_VANISHING_IDEAL */

#endif
/* FACTORED_NUMBER_H */