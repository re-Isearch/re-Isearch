/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:		attr.cxx
Version:	1.00
Description:	Class ATTR - Attribute
@@@*/

#include "attr.hxx"
#include "magic.hxx"

#pragma ident  "@(#)attr.cxx"


ATTR::ATTR()
{
  SetId = NulString;
  AttrType = 0;
  AttrValue = NulString;
}

ATTR::ATTR(const ATTR& OtherAttr)
{
  SetId = OtherAttr.SetId;
  AttrType = OtherAttr.AttrType;
  AttrValue = OtherAttr.AttrValue;
}

ATTR& ATTR::operator=(const ATTR& OtherAttr)
{
  SetId = OtherAttr.SetId;
  AttrType = OtherAttr.AttrType;
  AttrValue = OtherAttr.AttrValue;
  return *this;
}

void ATTR::Write(PFILE Fp) const {
  putObjID (objATTR,Fp);
  ::Write(SetId,     Fp);
  ::Write(AttrType,  Fp);
  ::Write(AttrValue, Fp);
}

bool ATTR::Read(PFILE Fp)
{
  obj_t obj = getObjID (Fp);
  if (obj != objATTR)
    {
      PushBackObjID (obj, Fp);
    }
  else
    {
      ::Read(&SetId,     Fp);
      ::Read(&AttrType,  Fp);
      ::Read(&AttrValue, Fp);
    }
  return obj == objATTR;
}

ATTR::~ATTR()
{
}
