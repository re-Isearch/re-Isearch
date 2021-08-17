/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef EUDEX_H
#define EUDEX_H

#include "gdt.h"
#include <iostream>
#include <stdint.h>

typedef uint64_t eudex_t;


// This class expects UCS-2/4 wide character input
//
// If the caller is using a UTF8 encoded or 8-bit character set
// it needs to prepare a wchar_t array to pass here
//
class EUDEX 
{
public:
  EUDEX(const wchar_t *input) { eudex = hash(input); };
  EUDEX(eudex_t val) { eudex = val; };
  EUDEX(NUMBER number);

  operator const uint64_t() const { return eudex; }
  operator const int64_t() const  { return (int64_t)eudex; }
  operator const NUMBER() const   { return conv_to_number(); }

  EUDEX& operator=(const EUDEX& other) {
    eudex = other.eudex;
    return *this;
  }

  eudex_t          value() const { return eudex; }
  int64_t          distance(const EUDEX& other) const { return distance(eudex, other.eudex); };

  friend std::ostream& operator <<(std::ostream& os, const EUDEX& val) { return os << val.eudex; };
private:
  NUMBER            conv_to_number() const;
  eudex_t           hash(const wchar_t *);
  int64_t           distance(const uint64_t, const uint64_t) const;
  eudex_t           eudex;
} ;

// Comparison operators
inline int64_t operator - (const EUDEX& s1, const EUDEX& s2) { return s1.distance(s2); }


inline bool operator== (const EUDEX& s1, const EUDEX& s2) { return s1.distance(s2) == 0; }
inline bool operator< (const EUDEX& s1, const EUDEX& s2)  { return s1.distance(s2) <  0; }
inline bool operator> (const EUDEX& s1, const EUDEX& s2)  { return s1.distance(s2) >  0; }

#endif
