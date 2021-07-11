/*@@@
File:		dfield.cxx
Version:	1.00
$Revision: 1.1 $
Description:	Class DATECFLD - Data structures for date 
@@@*/

#include "defs.hxx"
#include "dfield.hxx"


DATEFLD::DATEFLD() { }


DATEFLD::DATEFLD(const GPTYPE& Start, const SRCH_DATE& Date)
{
  GlobalStart = Start;
  Value       = Date;
}


DATEFLD& DATEFLD::operator=(const DATEFLD& Other)
{
  GlobalStart = Other.GlobalStart;
  Value       = Other.Value;
  return *this;
}

DATEFLD::DATEFLD(const DATEFLD& Other)
{
  *this = Other;
}

DATEFLD::operator STRING () const
{
  return Value;
}

void DATEFLD::Write(FILE *fp) const
{
  ::Write(GlobalStart, fp);
  ::Write(Value, fp);
}

void DATEFLD::Write(FILE *fp, GPTYPE Offset) const
{
  ::Write(GlobalStart+Offset, fp);
  ::Write(Value, fp);
}


ostream& operator<<(ostream& os, const DATEFLD& Fld)
{
  return os << "(" << Fld.GlobalStart << ", " << Fld.Value << ")";
}

void Write(const DATEFLD& d, FILE *fp)
{
  d.Write(fp);
}

void DATEFLD::Read(FILE *fp)
{
  ::Read(&GlobalStart, fp);
  ::Read(&Value, fp);
}

void Read(DATEFLD *ptr, FILE *fp)
{
  ptr->Read(fp);
}

void DATEFLD::Dump(ostream& os) const
{
  os << "start: " << GlobalStart << " value: " << Value.ISOdate() << endl;
}

DATEFLD::~DATEFLD() { }


DATERNGFLD::DATERNGFLD() { }

DATERNGFLD::DATERNGFLD(const GPTYPE& Start, const DATERANGE& Daterange) 
{ 
  GlobalStart = Start; 
  Value       = Daterange; 
} 


DATERNGFLD& DATERNGFLD::operator=(const DATERNGFLD& Other)
{
  GlobalStart = Other.GlobalStart;
  Value       = Other.Value;
  return *this;
}

DATERNGFLD::DATERNGFLD(const DATERNGFLD& Other)
{
  *this = Other;
}

DATERNGFLD::operator STRING () const
{
  return Value;
}

void DATERNGFLD::Write(FILE *fp) const
{
  ::Write(GlobalStart, fp);
  ::Write(Value, fp);
}

void DATERNGFLD::Write(FILE *fp, GPTYPE Offset) const
{
  ::Write(GlobalStart+Offset, fp);
  ::Write(Value, fp);
}

void Write(const DATERNGFLD& d, FILE *fp)
{
  d.Write(fp);
}

void DATERNGFLD::Read(FILE *fp)
{
  ::Read(&GlobalStart, fp);
  ::Read(&Value, fp);
}

void Read(DATERNGFLD *ptr, FILE *fp)
{
  ptr->Read(fp);
}

void DATERNGFLD::Dump(ostream& os) const
{
  os << "start: " << GlobalStart << " value: " << (STRING)Value << endl;
}

DATERNGFLD::~DATERNGFLD() { }


