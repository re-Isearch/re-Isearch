/*@@@
File:		intfield.hxx
Version:	$Revision: 1.1 $
Description:	Class INTFIELD - Numeric interval data object
@@@*/

#ifndef INTERVALFLD_HXX
#define INTERVALFLD_HXX

#include "gdt.h"
#include "nfield.hxx"


class INTERVALFLD : public NUMERICFLD {

public:
  INTERVALFLD();
  INTERVALFLD(const INTERVALFLD& OtherField);
  INTERVALFLD operator=(const INTERVALFLD& OtherField);
  GPTYPE GetGlobalStart() const  { return GlobalStart; }
  void   SetGlobalStart(GPTYPE x){ GlobalStart = x; }
  DOUBLE GetStartValue() const   { return StartValue; }
  void   SetStartValue(DOUBLE x) { StartValue = x; }
  DOUBLE GetEndValue() const     { return EndValue; }
  void   SetEndValue(DOUBLE x)   { EndValue = x; }
  void   Write(FILE *fp) const;
  void   Write(FILE *Fp, GPTYPE Offset) const;
  void   Dump(ostream& os = cout) const;

  void   Read(FILE *fp);

  ~INTERVALFLD();
 

  friend void Write(const INTERVALFLD& s, FILE *Fp);
  friend void Read(INTERVALFLD *s, FILE *Fp);


  friend inline ostream& operator<<(ostream& os, const INTERVALFLD& Fld);


private:
  GPTYPE GlobalStart;
  DOUBLE StartValue;
  DOUBLE EndValue;
};



typedef INTERVALFLD* PINTERVALFLD;

#endif
