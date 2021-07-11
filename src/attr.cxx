/*@@@
File:		attr.cxx
Version:	1.00
Description:	Class ATTR - Attribute
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "attr.hxx"
#include "magic.hxx"

#pragma ident  "@(#)attr.cxx  1.5 04/20/99 14:24:13 BSN"


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

GDT_BOOLEAN ATTR::Read(PFILE Fp)
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
