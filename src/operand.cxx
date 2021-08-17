/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)operand.cxx"

/************************************************************************
************************************************************************/

/*@@@
File:		operand.cxx
Description:	Class OPERAND - Query Operand
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
