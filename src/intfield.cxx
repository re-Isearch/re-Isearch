#pragma ident  "%Z%%Y%%M%  %I% %G% %U% BSN"
/*@@@
File:		intfield.cxx
Version:	$Revision: 1.1 $
Description:	Class INTFIELD - Numeric interval data object
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/


#include "intfield.hxx"

INTERVALFLD::INTERVALFLD()
{
  GlobalStart = (GPTYPE)-1;
  StartValue  = 0.0;
  EndValue    = 0.0;
}


// Copy Constructor
INTERVALFLD::INTERVALFLD(const INTERVALFLD& OtherField)
{
  GlobalStart = OtherField.GlobalStart;
  StartValue  = OtherField.StartValue;
  EndValue    = OtherField.EndValue;
}


INTERVALFLD INTERVALFLD::operator=(const INTERVALFLD& OtherField)
{
  GlobalStart = OtherField.GlobalStart;
  StartValue  = OtherField.StartValue;
  EndValue    = OtherField.EndValue;

  return *this;
}


void INTERVALFLD::Dump(ostream& os) const
{
  os << *this << endl;
}

ostream& operator<<(ostream& os, const INTERVALFLD& Fld)
{
 return  os << "ptr: " << Fld.GlobalStart << " [" << Fld.StartValue << "," << Fld.EndValue << "]";
}

INTERVALFLD::~INTERVALFLD()
{
}


void INTERVALFLD::Write(PFILE Fp) const
{
  ::Write(GlobalStart, Fp);
  ::Write(StartValue, Fp);
  ::Write(EndValue, Fp);
}

void INTERVALFLD::Write(PFILE Fp, GPTYPE Offset) const
{
  ::Write(GlobalStart+Offset, Fp);
  ::Write(StartValue, Fp);
  ::Write(EndValue, Fp);
}


void Write(const INTERVALFLD& s, PFILE Fp)
{
  s.Write(Fp);
}


void INTERVALFLD::Read(FILE *Fp)
{
  ::Read(&GlobalStart, Fp);
  ::Read(&StartValue, Fp);
  ::Read(&EndValue, Fp);
}

void Read(INTERVALFLD *s, FILE *Fp)
{
  s->Read(Fp);
} 


