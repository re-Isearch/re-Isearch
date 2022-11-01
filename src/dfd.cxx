/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*-@@@
File:		dfd.cxx
Description:	Class DFD - Data Field Definition
@@@*/

#include "common.hxx"
#include "dfd.hxx"

#pragma ident  "@(#)dfd.cxx"


DFD::DFD()
{
  FileNumber = 0;
}

DFD::DFD(const DFD& OtherDfd)
{
  FileNumber = OtherDfd.FileNumber;
  Attributes = OtherDfd.Attributes;
}

DFD& DFD::operator=(const DFD& OtherDfd)
{
  FileNumber = OtherDfd.FileNumber;
  Attributes = OtherDfd.Attributes;
  return *this;
}

#if 0
bool DFD::GetFieldName(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldName(StringBuffer);
}

bool DFD::GetFieldType(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldType(StringBuffer);
}
#endif

DFD::~DFD()
{
}
