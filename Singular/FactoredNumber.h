#ifndef FACTORED_NUMBER_H
#define FACTORED_NUMBER_H

#ifdef HAVE_VANISHING_IDEAL

#include <list>

/*! \class FactoredNumber
    \brief Class FactoredNumber may be used to represent the prime factorization
           of a given natural number n, n >= 1.

    The class uses two private member fields to store the primes which divide n,
    in ascending order, and the corresponding exponents. This implementation
    assumes that n has only "small" prime factors, i.e., only prime factors
    which appear in a SINGULAR-internal array of primes that can be accessed
    via the method <c>cf_getSmallPrime(int)</c>.<br>
    The class FactoredNumber implements basic operations between natural
    numbers such as <c>gcd, lcm, *</c>, check for divisibility and cancelation
    of prime factors (given by a second instance of FactoredNumber).
    \author Frank Seelisch, http://www.mathematik.uni-kl.de/~seelisch
*/
class FactoredNumber
{
  private:
    /** private store for all primes appearing in the prime factorization
      * of the represented natural number;
      * the primes are stored in ascending order
      */
    std::list<int> m_primes;    // prime factors sorted in ascending order
    /** private store for the exponents with which the primes appear in the
      * prime factorization of the represented natural number;
      * the exponent at position k corresponds to the k-th prime stored in
      * m_primes
      */
    std::list<int> m_exponents; // corresponding exponents in prime factorization
    /**
      * A static method for inserting a prime into the list m_primes.
      * This will work regardless whether p is already present in m_primes or
      * not. The exponent corresponding to p will be set/adjusted accordingly.
      * @param primes a list of prime numbers
      * @param exponents a list of corresponding exponents
      * @param p the prime to be inserted
      */
    static void insertPrime (std::list<int>& primes, std::list<int>& exponents, const int p);
    /**
      * A static method for computing the power b^e, for given ints b >= 1 and e >= 0.
      * This method goes along the binary representation of e, thus only computing
      * the necessary powers b^1, b^2, b^4, b^8, etc.
      * @param b the base of the power to be computed
      * @param e the exponent of the power to be computed
      * @result the power b^e
      */
    static int FactoredNumber::power (const int b, const int e);
  public:
    /**
      * A constructor which expects a natural number n, n >= 1, which has
      * only "small" primes in its prime factorization. The parameter n will
      * be prime-factorized by this constructor to fill the private members
      * accordingly.<br>
      * (For all n < 1, a representation of 1 will be returned.)
      * @param n the natural number to be represented, n >= 1
      */
    FactoredNumber (const int n);
    /**
      * A constructor which expects "small" prime and some exponent >= 0.
      * The resulting instance represents the natural number p^exponent.
      * In the case exponent == 0, this number will be 1.
      * @param p a "small" prime
      * @param exponent the exponent for p
      */
    FactoredNumber (const int p, const int exponent);
    /**
      * The copy constructor.
      * This creates a deep copy of the given argument.
      * @param fn a reference to some FactoredNumber
      */
    FactoredNumber (const FactoredNumber& fn);
    /**
      * The destructor.
      */
    ~FactoredNumber ();
    /**
      * A method for computing the product of the FactoredNumber
      * given by <c>*this</c> and the argument. The result is a new
      * instance of FactoredNumber.
      * @param fn a reference to some FactoredNumber
      * @return the product of *this and fn
      */
    FactoredNumber operator* (const FactoredNumber& fn) const;
    /**
      * A method for computing the cancelation of the FactoredNumber
      * given by <c>*this</c> with respect to the argument.
      * The result is a new instance of FactoredNumber which is defined
      * as follows: Let p^e be the highest p-power in <c>*this</c>. Then<br>
      * (1) If p does not appear in fn, then p^e is also the highest p-power
      * in the result.<br>
      * (2) Let p^d be the highest p-power in fn.<br>
      * (2.1) If e > d then p^(e-d) is the highest p-power in the result.<br>
      * (2.2) If e <= d then p does not appear in the result.
      * @param fn a reference to some FactoredNumber
      * @return the cancelation of *this w.r.t. fn
      */
    FactoredNumber cancel (const FactoredNumber& fn) const;
    /**
      * The assignment operator. This implements a deep copy.
      * @param fn a reference to some FactoredNumber
      */
    FactoredNumber& operator= (const FactoredNumber& fn);
    /**
      * A method for computing the greatest common divisor (gcd)
      * of the FactoredNumber given by <c>*this</c> and the argument.
      * The result is a new instance of FactoredNumber.
      * @param fn a reference to some FactoredNumber
      * @return the gcd of *this and fn
      */
    FactoredNumber gcd (const FactoredNumber& fn) const;
    /**
      * A method for computing the least common multiple (lcm)
      * of the FactoredNumber given by <c>*this</c> and the argument.
      * The result is a new instance of FactoredNumber.
      * @param fn a reference to some FactoredNumber
      * @return the lcm of *this and fn
      */
    FactoredNumber lcm (const FactoredNumber& fn) const;
    /**
      * A static method for computing the factorial of a
      * given multi-index of natural numbers.
      * The result is a new instance of FactoredNumber which
      * represents the natural number
      * <c>n[0]! * n[1]! * ... * n[nn-1]!</c>.<br>
      * <c>nn</c> is assumed to be at least 1.
      * @param nn the number of indices in the multi-index
      * @param n the multi-index
      * @return the factorial of the multi-index given by n
      */
    static FactoredNumber factorial (int nn, int* n);
    /**
      * A method for checking whether the FactoredNumber
      * given by <c>*this</c> divides the argument.
      * @param fn a reference to some FactoredNumber
      * @return true iff *this divides fn
      */
    bool divides (const FactoredNumber& fn) const;
    /**
      * A method for checking whether the FactoredNumber
      * given by <c>*this</c> and the argument are equal.
      * @param fn a reference to some FactoredNumber
      * @return true iff *this equals fn
      */
    bool operator== (const FactoredNumber& fn) const;
    /**
      * A method for checking whether the FactoredNumber
      * given by <c>*this</c> and the argument are not equal.
      * @param fn a reference to some FactoredNumber
      * @return true iff *this does not equal fn
      */
    bool operator!= (const FactoredNumber& fn) const;
    /**
      * A method for computing the value of the Smarandache function
      * for the FactoredNumber given by <c>*this</c>.
      * For an integer n, n >= 1, the Smarandache function yields the
      * smallest natural number k such that n divies k!.
      * @return the value of the Smarandache function applied to *this
      */
    int smarandache () const;
    /**
      * A method for obtaining the integer value of the FactoredNumber
      * given by *this.
      * This method may encounter overflows; hence the result may be
      * incorrect.
      * @return the integer value of *this
      */
    int getInt () const;
    /**
      * A method for obtaining the smallest prime sitting inside the FactoredNumber
      * given by *this. If there is no prime, then this method returns 1.
      * @return the smallest prime in *this, or 1 if there is none
      */
    int getSmallestP () const;
    /**
      * A method for obtaining the exponent with which a given prime
      * appears in the FactoredNumber given by *this.
      * If the prime is not present, then the correct exponent 0 is
      * returned.
      * @param p the prime
      * @return the exponent of p in *this
      */
    int getExponent (const int p) const;
    /**
      * A method for obtaining a printable string representation of the
      * FactoredNumber given by *this.
      * The caller of this method is responsible for deleting the obtained
      * char*.
      * @return a string representation of *this
      */
    char* toString () const;
    /**
      * A method for printing a string representation of the
      * FactoredNumber given by *this.
      */
    void print () const;
    /**
      * A method for printing a string representation of the
      * FactoredNumber given by *this, followed by a line feed.
      */
    void printLn () const;
};

#endif
/* HAVE_VANISHING_IDEAL */

#endif
/* FACTORED_NUMBER_H */
