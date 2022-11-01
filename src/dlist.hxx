/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		dlist.hxx
Description:	Class DATELIST - Utilities for lists of numeric values
@@@*/

#ifndef DATELIST_HXX
#define DATELIST_HXX

#include <stdlib.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "dfield.hxx"

#ifndef NUMERICLIST_HXX

// SearchState is used to indicate the results of the matcher 
enum SearchState { NO_MATCH=-1, TOO_LOW, TOO_HIGH, MATCH };

// IntType tells the matcher where we are in the block of values
enum IntType { AT_START, INSIDE, AT_END };

// NumBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
enum NumBlock { NONE, VAL_BLOCK, GP_BLOCK };

typedef NumBlock blktype;

#endif


class DATELIST {
private:
  DATEFLD     *table;      // the table of attribute/numeric data
  size_t       Count;      // count of items in table
  INT          Attribute;  // which USE attribute this table maps
  size_t       Pointer;    // current position
  size_t       MaxEntries; // current maximum size of table - see Resize()
  size_t       StartIndex;
  size_t       EndIndex;
  INT          Relation;   // what relation generated StartIndex, EndIndex
  STRING       FileName;   // the file which attributes/numeric data are in
  INT          Ncoords;    // Number of values in an entry

  void         WriteTable();
  void         WriteTable(INT Offset);

  SearchState  DiskFind(STRING Fn, const SRCH_DATE& Key, INT4 Relation, INT4 *Index);
  SearchState  MemFind(const SRCH_DATE& Key, INT4 Relation, INT4 *Index);

  SearchState  DiskFind(STRING Fn, GPTYPE Key, INT4 Relation, INT4 *Index);


  SearchState  Matcher(const SRCH_DATE& Key, const SRCH_DATE& A, const SRCH_DATE& B,
	const SRCH_DATE& C, INT4 Relation, INT4 Type);

  SearchState Matcher(GPTYPE Key, GPTYPE A, GPTYPE B, GPTYPE C,   
	INT4 Relation, INT4 Type);

public:
  DATELIST();
  DATELIST(INT n);

  void         Clear();
  void         Empty();

  void         SetRelation(INT r)      { Relation = r; };
  INT          GetRelation()           { return(Relation); };
  SRCH_DATE    GetValue(INT i)         { return(table[i].GetValue()); };
  // return the USE attribute for this table
  INT          GetAttribute()          { return(Attribute); };
  // set the USE attribute
  void         SetAttribute(INT x)     { Attribute = x; }; 
  // get number of items in table
  INT4         GetCount()              { return(Count); };
  // make room for more entries
  bool  Expand(size_t amount); 
  bool  Expand()                { return Resize(Count + (50*Ncoords)); };
  // collapse size to total entries
  void         Cleanup()               { Resize(Count); };
  GPTYPE       GetGlobalStart(INT4 i)  { return(table[i].GetGlobalStart()); };
  // set file name to load from
  void         SetFileName(STRING s)   { FileName = s; };
  // set file name to load from
  void         SetFileName(PCHR s)     { FileName = s; };
  void         SetCoords(INT n)        { Ncoords = n; };
  INT          GetCoords()             { return Ncoords; };

  SearchState  Find(const STRING& Fn, const SRCH_DATE& Key, INT4 Relation, INT4 *Index);
  SearchState  Find(const SRCH_DATE& Key, INT4 Relation, INT4 *Index);
  SearchState  Find(const STRING& Fn, GPTYPE Key, INT4 Relation, INT4 *Index);
  SearchState  Find(GPTYPE Key, INT4 Relation, INT4 *Index);

  /// 
  DATEFLD  operator[](signed n) const    {return table[n]; }
  DATEFLD  operator[](unsigned n) const  {return table[n]; }
  DATEFLD& operator[](unsigned n)        {return table[n]; }

  void         WriteIndex(const STRING& Fn);
  FILE        *OpenForAppend(const STRING& Fn);

  void         Sort();                     // sort numeric field items
  void         SortByGP();                 // sort by global ptr
  void         Dump(ostream& os = cout) const;    // dump all numeric field data
  void         Dump(INT4 start, INT4 end, ostream& os = cout) const; // dump numeric field data
  bool  Resize(size_t Entries);       // resize table to Entries size
  void         TempLoad();                 // test routine
  size_t       LoadTable(INT4 Start, INT4 End);
  size_t       LoadTable(INT4 Start, INT4 End, NumBlock Offset);
  size_t       LoadTable(INT4 Start, SRCH_DATE date);

  void         ResetHitPosition();
  GPTYPE       GetNextHitPosition();
  ~DATELIST();
};

typedef DATELIST* PDATELIST;
#endif
