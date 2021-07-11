/*@@@
File:		nfield.cxx
Version:	1.00
$Revision: 1.1 $
Description:	Class NUMERICFLD - Data structures for numeric data
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

#include "nfield.hxx"
#include "common.hxx"
#include "magic.hxx"

void NUMERICFLD::Dump(ostream& os) const
{
  os << *this << endl;
}


ostream& operator<<(ostream& os, const NUMERICFLD& Fld)
{
  return os << "start: " << Fld.GlobalStart << " value: " << Fld.NumericValue;
}


void NUMERICFLD::Write(FILE *fp) const
{
  putObjID(objNUMFLD, fp);
  ::Write(GlobalStart, fp);
  ::Write(NumericValue, fp);
}

void NUMERICFLD::Write(FILE *fp, GPTYPE Offset) const
{
  putObjID(objNUMFLD, fp);
  ::Write(GlobalStart+Offset, fp);
  ::Write(NumericValue, fp);
}


void Write(const NUMERICFLD& d, FILE *fp)
{
  d.Write(fp);
}

void NUMERICFLD::Read(FILE *fp)
{
  if (getObjID(fp) != objNUMFLD)
    {
      PushBackObjID( objNUMFLD, fp);
    }
//  GlobalStart = 0;
//  NumericValue = 0;
  ::Read(&GlobalStart, fp);
  ::Read(&NumericValue, fp);
}

void Read(NUMERICFLD *ptr, FILE *fp)
{
  ptr->Read(fp);
}


