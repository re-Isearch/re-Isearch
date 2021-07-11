/*-@@@
File:		dfd.cxx
Version:	1.00
Description:	Class DFD - Data Field Definition
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "common.hxx"
#include "dfd.hxx"

#pragma ident  "@(#)dfd.cxx  1.10 05/08/01 21:36:58 BSN"


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
GDT_BOOLEAN DFD::GetFieldName(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldName(StringBuffer);
}

GDT_BOOLEAN DFD::GetFieldType(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldType(StringBuffer);
}
#endif

DFD::~DFD()
{
}
