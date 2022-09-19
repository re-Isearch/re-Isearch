/************************************************************************
************************************************************************/

/*@@@
File:		sterm.hxx
Version:	1.00
Description:	Class STERM - String Search Term
Author:		Nassib Nassar, nrn@cnidr.org
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
