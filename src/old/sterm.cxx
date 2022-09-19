#pragma ident  "@(#)sterm.cxx  1.3 10/13/99 12:55:09 BSN"
/************************************************************************
************************************************************************/

/*@@@
File:		sterm.cxx
Version:	1.00
Description:	Class STERM - String Search Term
Author:		Nassib Nassar, nrn@cnidr.org
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
