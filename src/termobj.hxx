/************************************************************************
************************************************************************/

/*@@@
File:		termobj.hxx
Version:	1.00
Description:	Class TERMOBJ - Search Term Base Class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef TERMOBJ_HXX
#define TERMOBJ_HXX

#include "defs.hxx"
#include "string.hxx"
#include "operand.hxx"

class TERMOBJ : public OPERAND {
public:
  TERMOBJ() { }
  t_Operand GetOperandType() const { return TypeTerm; }
  virtual ~TERMOBJ() { }
};

typedef TERMOBJ* PTERMOBJ;

#endif
