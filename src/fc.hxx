/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		fc.hxx
Description:	Class FC - Field Coordinates
@@@*/

#ifndef FC_HXX
#define FC_HXX

#include "defs.hxx"

class FC {
public:
  FC();
  FC(const FC& Fc);
  FC(const GPTYPE Gp);
  FC(const GPTYPE GpPair[2]);
  FC(const GPTYPE Start, const GPTYPE End);

  void   SetFieldStart(const GPTYPE NewFieldStart) { FieldStart = NewFieldStart;};
  void   SetFieldEnd(const GPTYPE NewFieldEnd) { FieldEnd = NewFieldEnd;};
  GPTYPE GetFieldStart() const { return FieldStart;};
  GPTYPE GetFieldEnd() const { return FieldEnd;};

  GPTYPE      GetLength() const { return FieldEnd-FieldStart+1; }
  GDT_BOOLEAN IsEmpty() const   { return FieldStart == FieldEnd && FieldStart == 0; }

  operator STRING() const;

  FC& operator =(const FC& Fc);
  FC& operator =(const GPTYPE Gp);
  FC& operator =(const GPTYPE GpPair[2]);

// Arithmetic
  FC& operator+=(const GPTYPE Offset);
  FC& operator+=(const FC& Fc);
  FC& operator-=(const GPTYPE Offset);
  FC& operator-=(const FC& Fc);
// 
  friend FC operator+(int offset, const FC& Fc);
  friend FC operator-(int offset, const FC& Fc);
  friend FC operator+(const FC& Fc, int offset);
  friend FC operator-(const FC& Fc, int offset);

// Comparison
  INT Compare (const FC& Fc) const;
  GDT_BOOLEAN Contains (const FC& Fc) const {
	return (Fc.FieldStart >= FieldStart && Fc.FieldEnd <= FieldEnd);};
  GDT_BOOLEAN Contains (const GPTYPE Gp) const {
	return (Gp >= FieldStart && Gp <= FieldEnd);};
  GDT_BOOLEAN operator ==(const FC& Fc) const {
	return (FieldStart == Fc.FieldStart && FieldEnd == Fc.FieldEnd);};
  GDT_BOOLEAN operator !=(const FC& Fc) const {
        return (FieldStart != Fc.FieldStart || FieldEnd != Fc.FieldEnd);};
  GDT_BOOLEAN operator <=(const FC& Fc) const {
	return (FieldStart <= Fc.FieldStart && FieldEnd <= Fc.FieldEnd);};
  GDT_BOOLEAN operator >=(const FC& Fc) const {
	return (FieldStart >= Fc.FieldStart && FieldEnd >= Fc.FieldEnd);};
  GDT_BOOLEAN operator <(const FC& Fc) const {
	return (FieldStart < Fc.FieldStart  && FieldEnd < Fc.FieldEnd) ;};
  GDT_BOOLEAN operator >(const FC& Fc) const {
	 return (FieldStart > Fc.FieldStart && FieldEnd > Fc.FieldEnd);};
  GDT_BOOLEAN operator <(const GPTYPE Gp) const {
	return FieldStart < Gp;};
  GDT_BOOLEAN operator >(const GPTYPE Gp) const {
	return FieldEnd > Gp;};

  friend GDT_BOOLEAN operator >(const GPTYPE Gp, const FC& Fc) {
	return Gp > Fc.FieldEnd;};
  friend GDT_BOOLEAN operator <(const GPTYPE Gp, const FC& Fc) {
	return Gp < Fc.FieldStart;};
  friend INT Compare(const FC& Fc1, const FC& Fc2) {
	 return Fc1.Compare(Fc2);}; 

// I/O
  void Write (PFILE fp, const GPTYPE Offset) const;
  void Write(PFILE fp) const;
  GDT_BOOLEAN Read(PFILE fp);

  friend ostream& operator<<(ostream& os, const FC& Fc);

// Misc
  void FlipBytes();

  ~FC();
private:
  GPTYPE FieldStart;
  GPTYPE FieldEnd;
};

typedef FC* PFC;

void Write(const FC& Fc, PFILE Fp);
GDT_BOOLEAN Read(PFC FcPtr, PFILE Fp);


#endif
