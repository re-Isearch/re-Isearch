/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)operator.cxx"

/*@@@
File:		operator.cxx
Description:	Class OPERATOR - Query Operator
@@@*/

#include "operator.hxx"

OPERATOR::OPERATOR()
{
  OperatorType = OperatorNoop;
  OperatorMetric = 0;
}

OPERATOR::OPERATOR(const t_Operator& Op)
{
  OperatorType = Op;
  OperatorMetric = 0;
}


OPERATOR::OPERATOR(const OPOBJ& OtherOp)
{
  *this = OtherOp;
}

void OPERATOR::SetOperatorType(const t_Operator NewOperatorType)
{
  OperatorType = NewOperatorType;
}

OPOBJ* OPERATOR::Duplicate() const
{
  POPOBJ Temp = new OPERATOR();
  *Temp = *this;
  return Temp;
}

OPOBJ& OPERATOR::operator=(const OPOBJ& OtherOp)
{
  OperatorType = OtherOp.GetOperatorType();
  OperatorMetric = OtherOp.GetOperatorMetric();
  OperatorString = OtherOp.GetOperatorString();
  return *this;
}

t_Operator OPERATOR::GetOperatorType() const
{
  return OperatorType;
}

void OPERATOR::SetOperatorMetric(const FLOAT Metric)
{
  OperatorMetric = Metric;
}

FLOAT OPERATOR::GetOperatorMetric() const
{
  return OperatorMetric;
}

void OPERATOR::SetOperatorString(const STRING& Arg)
{
  OperatorString = Arg;
}

STRING OPERATOR::GetOperatorString() const
{
  return OperatorString;
}


OPERATOR::~OPERATOR() {
}
