// $Id: gpolyfield.hxx,v 1.1 2007/05/15 15:47:20 edz Exp $
/*@@@
File:		gpolyfield.hxx
Version:	1.0
$Revision: 1.1 $
Description:	Class GPOLYFIELD - Spatial polygon data object
@@@*/

#include <stdio.h>

#include "gdt.h"
#include "nfield.hxx"
#include "bboxfield.hxx"

#ifndef GPOLYFLD_HXX
#define GPOLYFLD_HXX

class GPOLYFLD 
{
public:
  GPOLYFLD();
  GPOLYFLD(INT n);
  GPOLYFLD(const BBOXFLD& OtherField);
  GPOLYFLD(const GPOLYFLD& OtherField);

  GPOLYFLD operator=(const GPOLYFLD& OtherField);

  DOUBLE operator[](size_t n) const;

  operator    STRING () const;

  GPTYPE      GetGlobalStart()        { return GlobalStart; }
  void        SetGlobalStart(GPTYPE x){ GlobalStart = x; }
  size_t      GetVertexCount()        { return nVertices; }
  void        SetVertexCount(size_t x);

  // Start count with 0
  GDT_BOOLEAN GetVertexN(size_t x, DOUBLE* value) const;
  void        SetVertexN(size_t x, DOUBLE value);

  // 0 if undefined (eg. nVertices != 4 (not a box)
  DOUBLE     getExtent() const { return Extent; }
  DOUBLE     setExtent(DOUBLE Val=0.0); // Val==0 -> Use Vertices to calculate it

  void       Dump(ostream& os = cout) const;
  void       Write(FILE *fp) const;
  int        Read(FILE *fp);
  void       Resize(size_t x);

  friend void Write(const GPOLYFLD& s, FILE *Fp);
  friend void Read(GPOLYFLD *s, FILE *Fp);

  //  DOUBLE GetEndValue()           { return EndValue; }
  //  void   SetEndValue(DOUBLE x)   { EndValue = x; }
  ~GPOLYFLD();

private:
  GPTYPE  GlobalStart;
  UINT2   nVertices;
  DOUBLE *Vertices;
  DOUBLE  Extent;
};

typedef GPOLYFLD* PGPOLYFLD;

#endif
