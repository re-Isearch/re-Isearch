/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		operand.hxx
Description:	Class OPERAND - Query Operand
@@@*/

#ifndef OPERAND_HXX
#define OPERAND_HXX

#include "defs.hxx"
#include "string.hxx"
#include "opobj.hxx"

class OPERAND : public OPOBJ {
public:
OPERAND();
  t_OpType GetOpType() const { return TypeOperand; };
  virtual OPOBJ& operator=(const OPOBJ& OtherOp);
  virtual void SetAttributes(const ATTRLIST& NewAttributes);
  virtual void GetAttributes(ATTRLIST *AttributesBuffer) const;
  virtual ~OPERAND();
private:
  ATTRLIST Attributes;
};

typedef OPERAND* POPERAND;

#endif
