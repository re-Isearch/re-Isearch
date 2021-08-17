/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		bboxfield.hxx
Description:	Class BBOXFIELD - Spatial bounding box data object
@@@*/

#include <stdio.h>

#include "gdt.h"
#include "nfield.hxx"

#ifndef BBOXFLD_HXX
#define BBOXFLD_HXX

/*
Extent: A derived GEO Profile element whose value is the number of square degrees of coverage
for a given data set. It is calculated as
  (North Bounding Coordinate - South Bounding Coordinate) * (East Bounding Coordinate - West Bounding Coordinate).
Extent provides the ability to locate data sets of small, regional, continental, and global extent.
*/

class BBOXFLD 
{
public:
  enum Directions { north=0, west, south, east };

  BBOXFLD();
  BBOXFLD(const char *String);
  BBOXFLD(const BBOXFLD& OtherField);
  BBOXFLD& operator=(const BBOXFLD& OtherField);

  operator   STRING () const;

  GPTYPE     GetGlobalStart() const   { return GlobalStart; }
  void       SetGlobalStart(GPTYPE x) { GlobalStart = x;    }

  // Bounded by -180/+180/-90/+90
  int        Set(const char * value);

  DOUBLE     getNorth() const { return _getVal(Vertices[north]);}
  DOUBLE     getWest() const  { return _getVal(Vertices[west]); }
  DOUBLE     getSouth() const { return _getVal(Vertices[south]);}
  DOUBLE     getEast() const  { return _getVal(Vertices[east]); }

  void       setNorth(DOUBLE val) { Vertices[north] = _setVal(val); }
  void       setWest(DOUBLE val)  { Vertices[west]  = _setVal(val); } 
  void       setSouth(DOUBLE val) { Vertices[south] = _setVal(val); } 
  void       setEast(DOUBLE val)  { Vertices[east]  = _setVal(val); } 

  DOUBLE     setExtent(DOUBLE val=0.0);
  DOUBLE     getExtent() const;

  void       Dump(ostream& os = cout) const;
  void       Write(FILE *fp) const;
  void       Write(FILE *fp, GPTYPE Offset) const;
  int        Read(FILE *fp);

  friend void Write(const BBOXFLD& s, FILE *Fp);
  friend void Read(BBOXFLD *s, FILE *Fp);

  ~BBOXFLD() {};

private:
  INT4    _setVal(DOUBLE val) const { return (INT4)(1000*val);    };
  DOUBLE  _getVal(INT4   val) const { return (DOUBLE)(val/1000.0);};
  GPTYPE  GlobalStart;
  INT4    Vertices[4];
  UINT4   Extent;
};


#endif
