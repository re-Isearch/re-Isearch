/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		df.cxx
Description:	Class DF - Data Field
@@@*/

#include "common.hxx"
#include "df.hxx"
#include "magic.hxx"

#pragma ident  "@(#)df.cxx  1.11 06/27/00 20:29:06 BSN"


DF::DF() {
}

DF::DF(const DF& OtherDf)
{
  *this = OtherDf;
}

DF& DF::operator=(const DF& OtherDf)
{
  FieldName = OtherDf.FieldName;
  Fct = OtherDf.Fct;
  return *this;
}


void DF::GetFieldName(PSTRING StringBuffer) const
{
  if (StringBuffer)
    *StringBuffer = FieldName;
}

void DF::Write(PFILE fp) const
{
  putObjID (objDF, fp);
  FieldName.Write(fp);
  Fct.Write(fp);
}

bool DF::Read(PFILE fp) {
  obj_t obj = getObjID (fp);
  if (obj != objDF)
    {
      PushBackObjID (obj, fp);
    }
  else
    {
      FieldName.Read(fp);
      Fct.Read(fp);
    }
  return obj == objDF;
}

DF::~DF() {
}
