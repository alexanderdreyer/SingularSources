// emacs edit mode for this file is -*- C++ -*-
// $Id: fglmvec.h,v 1.6 1998-05-19 09:43:47 pohl Exp $

/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/*
* ABSTRACT - The FGLM-Algorithm
*   header file for fglmvec.cc. See fglmvec.cc for details.
*/

#ifndef FGLMVEC_H
#define FGLMVEC_H

#include "mod2.h"
#include "numbers.h"

class fglmVectorRep;

class fglmVector
{
protected:
    fglmVectorRep * rep;
    void makeUnique();
    fglmVector( fglmVectorRep * rep );
public:
    fglmVector();
    fglmVector( int size );
    fglmVector( int size, int basis );
    fglmVector( const fglmVector & v );
    ~fglmVector();
#ifdef __NO_CONSTR__
    void mac_constr( const fglmVector & v);
    void mac_constr_i( int size);
#endif
    int size() const;
    int numNonZeroElems() const;

    void nihilate( const number fac1, const number fac2, const fglmVector v );
    fglmVector & operator = ( const fglmVector & v );

    int operator == ( const fglmVector & );
    int operator != ( const fglmVector & );
    int isZero();
    int elemIsZero( int i );

    fglmVector & operator += ( const fglmVector & );
    fglmVector & operator -= ( const fglmVector & );
    fglmVector & operator *= ( const number & );
    fglmVector & operator /= ( const number & );
    friend fglmVector operator - ( const fglmVector & v );
    friend fglmVector operator + ( const fglmVector & lhs, const fglmVector & rhs );
    friend fglmVector operator - ( const fglmVector & lhs, const fglmVector & rhs );
    friend fglmVector operator * ( const fglmVector & v, const number n );
    friend fglmVector operator * ( const number n, const fglmVector & v );

    const number getconstelem( int i ) const;
    number & getelem( int i );
    void setelem( int i, number & n );

    number gcd() const;
    number clearDenom();
};

#endif
