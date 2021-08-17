/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		termobj.hxx
Description:	Class TERMOBJ - Search Term Base Class
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
