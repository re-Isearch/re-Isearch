/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)sterm.cxx"

/*@@@
File:		sterm.cxx
Description:	Class STERM - String Search Term
@@@*/

#include "sterm.hxx"

STERM::STERM() {
}

OPOBJ* STERM::Duplicate() const {
  POPOBJ Temp = new STERM();
  *Temp = *this;
  return Temp;
}

OPOBJ& STERM::operator=(const OPOBJ& OtherOp) {
  OPERAND::operator=(OtherOp);
  OtherOp.GetTerm(&Term);
  return *this;
}

STERM::~STERM() {
}
