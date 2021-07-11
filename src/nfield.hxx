/*@@@
File:		nfield.hxx
@@@*/

#ifndef NUMERICFLD_HXX
#define NUMERICFLD_HXX

#include "gdt.h"
#include "defs.hxx"

class NUMERICFLD {
public:
  NUMERICFLD() { }
  NUMERICFLD(GPTYPE x, NUMBER v)
    {
      GlobalStart = x;
      NumericValue = v;
    }

  NUMERICFLD(const NUMERICFLD& Other)
    {
      GlobalStart  = Other.GlobalStart;
      NumericValue = Other.NumericValue;
    }
  NUMERICFLD& operator=(const NUMERICFLD& Other)
    {
      GlobalStart  = Other.GlobalStart;
      NumericValue = Other.NumericValue;
      return *this;
    }

  operator    STRING () const { return NumericValue; }

  NUMERICFLD& operator+=(const GPTYPE Offset) { IncrementStart(Offset); return *this;}
  void        IncrementStart(GPTYPE x)  { GlobalStart += x; }

  GPTYPE      GetGlobalStart() const    { return GlobalStart; };
  void        SetGlobalStart(GPTYPE x)  { GlobalStart = x;    };
  NUMBER      GetNumericValue() const   { return NumericValue;} ;
  void        SetNumericValue(NUMBER x) { NumericValue = x;   };

  void        Dump(ostream& os = cout) const;

  void        Write(FILE *fp) const;
  void        Write(FILE *fp, GPTYPE Offset) const;
  void        Read(FILE *fp);

  friend void Write(const NUMERICFLD& d, FILE *fp);
  friend void Read(NUMERICFLD *ptr, FILE *fp);

  friend ostream& operator<<(ostream& os, const NUMERICFLD& Fld);

  ~NUMERICFLD() { }

  size_t Sizeof () const { return sizeof(GPTYPE) + sizeof(NUMBER) + 1; }

private:
  GPTYPE   GlobalStart;
  NUMBER   NumericValue;
};


void Write(const NUMERICFLD& d, FILE *fp);
void Read(NUMERICFLD *ptr, FILE *fp);


typedef NUMERICFLD* PNUMERICFLD;

#endif
