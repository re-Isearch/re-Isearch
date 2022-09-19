#pragma ident  "@(#)operand.cxx  1.4 10/13/99 12:55:23 BSN"

/************************************************************************
************************************************************************/

/*@@@
File:		operand.cxx
Version:	1.00
Description:	Class OPERAND - Query Operand
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "operand.hxx"

OPERAND::OPERAND() {
}

OPOBJ& OPERAND::operator=(const OPOBJ& OtherOp) {
  OtherOp.GetAttributes(&Attributes);
  return *this;
}

void OPERAND::SetAttributes(const ATTRLIST& NewAttributes) {
  Attributes = NewAttributes;
}

void OPERAND::GetAttributes(PATTRLIST AttributesBuffer) const {
  *AttributesBuffer = Attributes;
}

OPERAND::~OPERAND() {
}
