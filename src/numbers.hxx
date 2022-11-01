/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef NUMBERS_HXX
# define NUMBERS_HXX 1

class NUMERICOBJ {
 friend class NUMERICALRANGE;
public:
  NUMERICOBJ ();
  NUMERICOBJ (const STRING& s);
  NUMERICOBJ (NUMBER x) { val = x; }

  bool Ok() const;

  NUMERICOBJ& operator=(const NUMERICOBJ& newVal) { val = newVal.val; return *this; }

  NUMERICOBJ& operator +=(const NUMERICOBJ& x) {
   val += x.val;
   return *this;
  }
  NUMERICOBJ& operator -=(const NUMERICOBJ& x) {
   val -= x.val;
   return *this;
  }
  NUMERICOBJ& operator *=(const NUMERICOBJ& x) {
   val *= x.val;
   return *this;
  }
  NUMERICOBJ& operator /=(const NUMERICOBJ& x) {
   val /= x.val;
   return *this;
  }

  friend NUMERICOBJ operator+(const NUMERICOBJ& val1,  const NUMERICOBJ& val2);
  friend NUMERICOBJ operator-(const NUMERICOBJ& val1,  const NUMERICOBJ& val2);
  friend NUMERICOBJ operator*(const NUMERICOBJ& val1,  const NUMERICOBJ& val2);
  friend NUMERICOBJ operator/(const NUMERICOBJ& val1,  const NUMERICOBJ& val2);

  friend NUMERICOBJ operator+(const NUMERICOBJ& val1,  const DOUBLE val2);
  friend NUMERICOBJ operator-(const NUMERICOBJ& val1,  const DOUBLE val2);
  friend NUMERICOBJ operator*(const NUMERICOBJ& val1,  const DOUBLE val2);
  friend NUMERICOBJ operator/(const NUMERICOBJ& val1,  const DOUBLE val2);



  operator long double() const { return (long double)val; }
  operator double() const { return (double)val; }
  operator int()    const { return (int)val;  }
  operator long()   const { return (long)val; }

  bool operator== (const NUMERICOBJ& Other) const { return val == Other.val; }
  bool operator!= (const NUMERICOBJ& Other) const { return val != Other.val; }
  bool operator>= (const NUMERICOBJ& Other) const { return val >= Other.val; }
  bool operator<= (const NUMERICOBJ& Other) const { return val <= Other.val; }
  bool operator>  (const NUMERICOBJ& Other) const { return val >  Other.val; }
  bool operator<  (const NUMERICOBJ& Other) const { return val <  Other.val; }

  void        Write(FILE *Fp) const;
  bool Read(FILE *Fp);

  friend void Write(const NUMERICOBJ& s, FILE *Fp);
  friend bool Read(NUMERICOBJ *p, PFILE Fp);

  // STD IO Streams
  friend ostream& operator <<(ostream& os, const NUMERICOBJ& val) { return os << (long double) val.val; }
  friend istream& operator >>(istream& os, NUMERICOBJ& val) {
	long double value; istream& res =  os >> value; val.val = value; return res; }

private:
  NUMBER val;
};

#if 0

/* Telephone numbers?

UK:
Min. 10 or 11 digits (the specification indicates 11 digits, but the author knows of some 10 digit numbers)
Valid full UK telephone numbers must start with a 0

Telephone Formats:

  +Code Number
 (+Code) Number
  0Code Number
  00Code Number
  Number

Number Formats:
  (nnn) nnn-nnnn
  nnn-nnn-nnnn

 where - can be space or no-space or -

0 may be prefixed

Extention:
  #nnnnn

*/
class  TELEPHONE_NUM {
public:

  TELEPHONE_NUM ()

private:
  UINT4 CountryCode;
  UINT4 AreaCode;
  UINT8 Number;
  UINT4 Extension;
};

/*
CARD TYPE 	Prefix 	Length 	Check digit algorithm
MASTERCARD	51-55	16 	mod 10
VISA	4	13, 16 	mod 10
AMEX	34
37	15 	mod 10
Diners Club/
Carte Blanche	300-305
36
38 	14	mod 10
Discover	6011	16 	mod 10
enRoute	2014
2149 	15	any
JCB	3	16 	mod 10
JCB	2131
1800	15 	mod 10
*/

class CREDITCARD_NUM {


}


/* IBAN: LLPZIIDBAN

2-place, alphabetical country code (LL)
2-place, numberical checksum (PZ) over the entire IBAN 
Maximal 30-place Basic Bank Account Number (BBAN):
	Bank Identification (IID) Account id (BAN)

Name                                                                Example 
Country Code	alpha constant                                        DE
Checksun        PZ        2-place numeric, Modulus 97-10 (ISO 7064)   21
Bank Id         IID  BLZ  Konstant 8-place alphanumeric               30120400
Account Id      BAN  KTO  Konstant 10-stellig (with leading zeros)    15228


*/


#endif


class NUMERICALRANGE
{
public:
  NUMERICALRANGE();
  NUMERICALRANGE(const NUMERICOBJ& Start, const NUMERICOBJ& End) {
    d_start = Start;
    d_end   = End;
  }
  NUMERICALRANGE(const NUMERICOBJ& NewPoint) {
    *this = NewPoint;
  }
  NUMERICALRANGE(const STRING& RangeString);

  // Setting
  bool SetRange(const STRING& RangeString);
  bool SetRange(const NUMERICOBJ& Start, const NUMERICOBJ& End) {
    d_start = Start;
    d_end   = End;
    return Ok();
  }

  NUMERICALRANGE& operator  =(const NUMERICALRANGE& Range) {
    d_start = Range.d_start;
    d_end   = Range.d_end;
    return *this;
  }
  // Only set when the string makes sense!
  NUMERICALRANGE& operator  =(const STRING& RangeString) {
    const NUMERICALRANGE Range(RangeString);
    if (Range.Ok()) *this = Range; // Set only if OK
    return *this;
  }
  // Arithmetic
  NUMERICALRANGE& operator +=(const NUMERICALRANGE& Range) {
   d_start += Range.d_start;
   d_end   += Range.d_end;
   return *this;
  }
  NUMERICALRANGE& operator -=(const NUMERICALRANGE& Range) {
    d_start -= Range.d_start;
    d_end   -= Range.d_end;
    return *this;
  }
  NUMERICALRANGE& operator ++() {return *this += NUMERICOBJ(1); }
  NUMERICALRANGE& operator --() {return *this -= NUMERICOBJ(1); }

  // typecasts
  operator    STRING () const;

  friend ostream &operator << (ostream &os, const NUMERICALRANGE &dt);
  friend STRING  &operator << (STRING &String, const NUMERICALRANGE &dt); 
 
  NUMERICOBJ  GetStart() const { return d_start; }
  NUMERICOBJ  GetEnd()   const { return d_end; }

  void  SetStart(const NUMERICOBJ& NewStart) { d_start = NewStart; }
  void  SetEnd(const NUMERICOBJ& NewEnd)  { d_end = NewEnd; }

  bool Ok() const;
  bool Defined() const;
  bool Contains(const NUMERICOBJ& Test) const;
  bool Contains(const NUMERICALRANGE& OtherRange) const;

  void        Clear()  { d_start = 0; d_end = 0; }

  // Comparison operators
  bool operator ==(const NUMERICALRANGE& OtherRange) const;
  bool operator !=(const NUMERICALRANGE& OtherRange) const;
  bool operator  >(const NUMERICALRANGE& OtherRange) const;
  bool operator  <(const NUMERICALRANGE& OtherRange) const;
  bool operator >=(const NUMERICALRANGE& OtherRange) const;
  bool operator <=(const NUMERICALRANGE& OtherRange) const;

  // I/O
  bool Read(PFILE Fp);
  void Write(PFILE Fp) const;
  
  ~NUMERICALRANGE();
  
private:
  NUMERICOBJ d_start;
  NUMERICOBJ d_end;
};
  
  
// Common functions....
inline void Write(const NUMERICALRANGE& Range, FILE *Fp)  { Range.Write(Fp);         }
inline bool Read(NUMERICALRANGE *Range, FILE *Fp)  { return Range->Read(Fp);  }


class MONETARYOBJ {
public:
  MONETARYOBJ ();
  MONETARYOBJ (const STRING& s);
  MONETARYOBJ (const NUMBER x);
  MONETARYOBJ (const NUMERICOBJ& x);
/*
  MONETARYOBJ (UINT4 amount, UINT2 fract) {
    Amount = amount;
    Fract  = fract;
  }
*/
  bool Ok() const { return Fract < 50000 ? true : false; }

  bool Set(const NUMBER  x);
  bool Set(const STRING& s);

  MONETARYOBJ& operator=(const MONETARYOBJ& newVal) {
   Amount = newVal.Amount;
   Fract  = newVal.Fract;
   return *this;
  }
  MONETARYOBJ& operator=(const NUMBER x) {
   Set(x);
   return *this;
  }

  operator long double() const {
    long double val = Amount;
    if (Ok()) val += Fract/50000.0;
    else      val += (Fract - 50001)/50000.0;
    return val;
  }

  operator NUMERICOBJ() const { return NUMERICOBJ((long double)*this); }
  operator double() const { return (double)((long double)*this); }
  operator int()    const { return (int)Amount;  }
  operator long()   const { return (long)Amount; }

  bool  RoundToNearest100th() {
    if (!Ok()) return false;
    // 500 = 1 cent (1/100 of currency unit)
    Fract = (500 * ((Fract+250)/500));
    return true;
  }

  // Resolves to 0.000002
  bool operator== (const MONETARYOBJ& Other) const {
    return Amount == Other.Amount && Fract == Other.Fract;
  }
  bool operator!= (const MONETARYOBJ& Other) const {
    return Amount != Other.Amount && Fract != Other.Fract;
  }
  bool operator>= (const MONETARYOBJ& Other) const {
    return (Amount > Other.Amount || (Amount == Other.Amount && Fract >= Other.Fract));
  }
  bool operator<= (const MONETARYOBJ& Other) const {
    return (Amount < Other.Amount || (Amount == Other.Amount && Fract <= Other.Fract));
  }
  bool operator>  (const MONETARYOBJ& Other) const {
    return (Amount > Other.Amount || (Amount == Other.Amount && Fract > Other.Fract));
  }
  bool operator<  (const MONETARYOBJ& Other) const {
    return (Amount < Other.Amount || (Amount == Other.Amount && Fract < Other.Fract));
  }

  void        Write(FILE *Fp) const;
  bool Read(FILE *Fp);

  friend void Write(const MONETARYOBJ& s, FILE *Fp);
  friend bool Read(MONETARYOBJ *p, PFILE Fp);

  // STD IO Streams
  friend ostream& operator <<(ostream& os, const MONETARYOBJ& val) {
    return os << val.Amount << "." << val.Fract;
  }
  friend istream& operator >>(istream& os, MONETARYOBJ& val) {
    STRING   tmp;
    istream& o = os >> tmp;
    val = MONETARYOBJ(tmp);
    return o;
  }

private:
  UINT4 Amount;
  UINT2 Fract; // nearlest 1/500 th cent, e.g. Amount/500*100 = 50,000
};

#endif
