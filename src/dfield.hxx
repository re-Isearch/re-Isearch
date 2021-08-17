/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		dfield.hxx
Description:	Class DATEFLD - Data structures for date 
@@@*/

#ifndef DATEFLD_HXX
#define DATEFLD_HXX

#include "gdt.h"
#include "common.hxx"
#include "date.hxx"

class DATEFLD {
public:
  DATEFLD();
  DATEFLD(const GPTYPE& Start, const SRCH_DATE& Date);
  DATEFLD(const DATEFLD& Other);

  DATEFLD& operator=(const DATEFLD& Other);
  operator  STRING () const;

  GPTYPE    GetGlobalStart() const { return GlobalStart;};
  void      SetGlobalStart(GPTYPE x) { GlobalStart = x;};
  SRCH_DATE GetValue() const { return Value;} ;
  void      SetValue(const SRCH_DATE& x) { Value = x;};
  void      Dump(ostream& os = cout) const;
  void      Write(FILE *fp) const;
  void      Write(FILE *fp, const GPTYPE Offset) const;

  void      Read(FILE *fp);

  friend ostream& operator<<(ostream& os, const DATEFLD& Fld);

  ~DATEFLD();

  friend void Write(const DATEFLD& d, FILE *fp);
  friend void Read(DATEFLD *ptr, FILE *fp);

private:
  GPTYPE   GlobalStart;
  SRCH_DATE Value;
};


void Write(const DATEFLD& d, FILE *fp);
void Read(DATEFLD *ptr, FILE *fp);



// Date Range Field
class DATERNGFLD {
public:
  DATERNGFLD();
  DATERNGFLD(const GPTYPE& Start, const DATERANGE& Daterange);
  DATERNGFLD(const DATERNGFLD& Other);

  DATERNGFLD& operator=(const DATERNGFLD& Other);
  operator  STRING () const;

  GPTYPE     GetGlobalStart() const { return GlobalStart;};
  void       SetGlobalStart(GPTYPE x) { GlobalStart = x;};
  DATERANGE  GetValue() const { return Value;} ;
  void       SetValue(const DATERANGE& x) { Value = x;};
  void       Dump(ostream& os = cout) const;
  void       Write(FILE *fp) const;
  void       Write(FILE *fp, GPTYPE Offset) const;
  void       Read(FILE *fp);

  ~DATERNGFLD();

  friend void Write(const DATERNGFLD& d, FILE *fp);
  friend void Read(DATERNGFLD *ptr, FILE *fp);

private:
  GPTYPE    GlobalStart;
  DATERANGE Value;
};


typedef DATEFLD* PDATEFLD;

#endif
