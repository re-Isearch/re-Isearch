/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:		intlist.hxx
Version:	1.00
$Revision: 1.1 $
Description:	Class INTERVALLIST - Utilities for lists of numeric intervals
@@@*/

#ifndef INTERVALLIST_HXX
#define INTERVALLIST_HXX

#include <stdlib.h>
#include <time.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "dlist.hxx"
#include "intfield.hxx"

// IntBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
enum IntBlock { START_BLOCK, END_BLOCK, PTR_BLOCK };


class INTERVALLIST 
  : public NUMERICLIST 
{
private:
    
  PINTERVALFLD table;      // the table of attribute/numeric data
  size_t       Count;      // count of items in table
  INT          Attribute;  // which USE attribute this table maps
  INT4         Pointer;    // current position
  size_t       MaxEntries; // current maximum size of table - see Resize()
  INT4         StartIndex;
  INT4         EndIndex;
  INT          Relation;   // what relation generated StartIndex, EndIndex
  STRING       FileName;   // the file which attributes/numeric data are in
  INT          Ncoords;    // Number of values in an entry
  GDT_BOOLEAN  Truncate;
    
  //  These are called internally by Find
  //  SearchState  MemFind(DOUBLE Key, INT4 Relation, 
  //		       GDT_BOOLEAN ByStart, INT4 *Index);
  //  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
  //			GDT_BOOLEAN ByStart, INT4 *Index);
  SearchState  MemFind(DOUBLE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index);
  SearchState  DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
			IntBlock FindBlock, INT4 *Index);
  SearchState  MemFind(GPTYPE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index);
  SearchState  DiskFind(STRING Fn, GPTYPE Key, INT4 Relation, 
			IntBlock FindBlock, INT4 *Index);
  SearchState  IntervalMatcher(DOUBLE Key, DOUBLE LowerBound, 
			       DOUBLE Mid, DOUBLE UpperBound,
			       INT4 Relation, IntType Type);
  SearchState  IntervalMatcher(GPTYPE Key, GPTYPE LowerBound, 
			       GPTYPE Mid, GPTYPE UpperBound,
			       INT4 Relation, IntType Type);

public:
  INTERVALLIST();
  INTERVALLIST(INT n);

  void   Clear();
  void   Empty();

  void   SetTruncatedSearch(GDT_BOOLEAN State=GDT_TRUE) {
    Truncate = State;
  }

  void   SortByStart();              // sort numeric field items
  void   SortByEnd();                // sort numeric field items
  void   SortByGP();                 // sort numeric field items
  INT4   LoadTable(INT4 Start, INT4 End);
  INT4   LoadTable(INT4 Start, INT4 End, IntBlock Offset);
  void   WriteTable();
  void   WriteTable(INT Offset);
  GPTYPE GetGlobalStart(INT4 i) { return(table[i].GetGlobalStart()); }
  DOUBLE GetStartValue(INT i)   { return(table[i].GetStartValue()); }
  DOUBLE GetEndValue(INT i)     { return(table[i].GetEndValue()); }
  void   SetCoords(INT n)       { Ncoords = n; }
  INT    GetCoords()            { return Ncoords; }
  // set file name to load from
  void   SetFileName(STRING s)  { FileName = s; }
  // set file name to load from
  void   SetFileName(CHR *s)    { FileName = s; }
  // get number of items in table
  INT4   GetCount() { return(Count); }
  // make room for more entries
  void   Expand(size_t amount);
  void   Expand()                { Resize(Count + (50*Ncoords)); };
  // collapse size to total entries
  void   Cleanup() { Resize(Count); }
  void   Resize( INT4 Entries);      // resize table to Entries size

  void         WriteIndex(const STRING& Fn);
  FILE        *OpenForAppend(const STRING& Fn);

  // This calls the old DiskFind and, now, MemFind
  SearchState  Find(STRING Fn, DOUBLE Key, INT4 Relation, 
		    IntBlock FindBlock, INT4 *Index);
  SearchState  Find(STRING Fn, GPTYPE Key, INT4 Relation, 
		    IntBlock FindBlock, INT4 *Index);

  SearchState  Find(DOUBLE Key, INT4 Relation, IntBlock FindBlock, 
		    INT4 *Index);
  SearchState  Find(GPTYPE Key, INT4 Relation, IntBlock FindBlock, 
		    INT4 *Index);

  void         Dump(ostream& os = cout) const;                  // dump numeric field data
  void         Dump(INT4 start, INT4 end, ostream& os = cout) const; // dump numeric field data
  ~INTERVALLIST();
};

typedef INTERVALLIST* PINTERVALLIST;
#endif



