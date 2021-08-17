/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		sterm.hxx
Description:	Class STERM - String Search Term
@@@*/

#ifndef STERM_HXX
#define STERM_HXX

#include "defs.hxx"
#include "string.hxx"
#include "termobj.hxx"

class STERM : public TERMOBJ {
friend class SEARCHTERM;
public:
  STERM();
  OPOBJ*  Duplicate() const;
  OPOBJ&  operator=(const OPOBJ& OtherOp);
  void    SetTerm(const STRING& NewTerm)      { Term = NewTerm;       }
  void    GetTerm(STRING *StringBuffer) const { *StringBuffer = Term; }
  const   STRING& GetTerm() const             { return Term;          }
  ~STERM();
private:
  STRING Term;
};

typedef STERM* PSTERM;

#endif
