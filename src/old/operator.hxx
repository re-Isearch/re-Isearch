/************************************************************************
************************************************************************/

/*@@@
File:		operator.hxx
Version:	1.00
Description:	Class OPERATOR - Query Operator
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef OPERATOR_HXX
# define OPERATOR_HXX

#include "defs.hxx"
#include "string.hxx"
#include "opobj.hxx"

class OPERATOR : public OPOBJ {
public:
  OPERATOR();
  OPERATOR(const t_Operator& Op);
  OPERATOR(const OPOBJ& OtherOp);
  t_OpType GetOpType() const { return TypeOperator; };
  t_Operand GetOperandType() const { return TypeNil; };
  OPOBJ* Duplicate() const;
  virtual OPOBJ& operator=(const OPOBJ& OtherOp);
  void SetOperatorType(const t_Operator OperatorType);
  t_Operator GetOperatorType() const;
  void SetOperatorMetric(const FLOAT Metric);
  FLOAT GetOperatorMetric() const;
  void SetOperatorString(const STRING& Arg);
  STRING GetOperatorString() const;
  ~OPERATOR();
private:
  t_Operator OperatorType;
  FLOAT      OperatorMetric;
  STRING     OperatorString;
};

typedef OPERATOR* POPERATOR;

#endif
