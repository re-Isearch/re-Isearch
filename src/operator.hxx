/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		operator.hxx
Description:	Class OPERATOR - Query Operator
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

 // friend ostream& operator <<(ostream& os, const OPERATOR& op) {
 //   return os << op.GetOperatorType(); 
 // }

private:
  t_Operator OperatorType;
  FLOAT      OperatorMetric;
  STRING     OperatorString;
};

typedef OPERATOR* POPERATOR;

#endif
