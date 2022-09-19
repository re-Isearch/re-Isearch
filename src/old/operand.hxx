/************************************************************************
************************************************************************/

/*@@@
File:		operand.hxx
Version:	1.00
Description:	Class OPERAND - Query Operand
Author:		Nassib Nassar, nrn@cnidr.org
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
