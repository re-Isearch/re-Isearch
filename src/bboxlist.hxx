/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		bboxlist.hxx
Description:	Class BBOXLIST - Utilities for lists of spatial bouding boxes 
@@@*/

#ifndef BBOXLIST_HXX
#define BBOXLIST_HXX

#include <stdlib.h>
#include <time.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "fc.hxx"
//#include "nlist.hxx"
#include "bboxfield.hxx"

// IntBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
//enum IntBlock { START_BLOCK, END_BLOCK, PTR_BLOCK };


class BBOXLIST 
{
public:
  BBOXLIST();
  BBOXLIST(INT n);

  void     Write(FILE *fp);
  INT      Read(FILE *fp);

  void     Dump(ostream& os = cout) const;

  void     SetFileName(STRING s)  { FileName = s; }
  void     SetFileName(CHR *s)    { FileName = s; }
  STRING   GetFileName()          { return FileName; }

  void     SetCount(size_t x)     { Count = x; }
  size_t   GetCount()             { return Count; }

  BBOXFLD *GetByGp(const FC& Fc) const;
  BBOXFLD *GetByGp(GPTYPE GpStart, GPTYPE GpEnd) const;
  BBOXFLD *GetByIndex(size_t x) const;

  void     Clear();
  void     Empty();

  // make room for more entries
  void     Expand();

  // collapse size to total entries
  void     Cleanup()              { Resize(Count); }
  void     Resize(size_t Entries); // resize table to Entries size

  void     WriteIndex(const STRING& Fn);
  FILE    *OpenForAppend(const STRING& Fn);

  GDT_BOOLEAN  Close(FILE *fp) { return (fp && fclose((FILE *)fp) == 0); };

  ///
  BBOXFLD  operator[](signed n) const   {return table[n]; }
  BBOXFLD  operator[](unsigned n) const {return table[n]; }
  BBOXFLD& operator[](unsigned n)       {return table[n]; }

  ~BBOXLIST();

  // This calls the old DiskFind and, now, MemFind
  //  SearchState  Find(STRING Fn, DOUBLE Key, INT4 Relation, 
  //		    IntBlock FindBlock, INT4 *Index);
  //  SearchState  Find(STRING Fn, INT4 Key, INT4 Relation, 
  //		    IntBlock FindBlock, INT4 *Index);
  //
  //  SearchState  Find(DOUBLE Key, INT4 Relation, IntBlock FindBlock, 
  //		    INT4 *Index);
  //  SearchState  Find(INT4 Key, INT4 Relation, IntBlock FindBlock, 
  //		    INT4 *Index);

private:
  void   WriteTable();
  void   WriteTable(INT Offset);
  INT    LoadRawTable();
  INT    LoadTable(STRING& Fn);

  void   SortByGp();
  void   SortByNorth();
  void   SortByEast();
  void   SortByWest();
  void   SortBySouth();
  void   SortByExtent();

  BBOXFLD    *table;      // the table of attribute/numeric data
  size_t      Count;      // count of items in table
  STRING      FileName;   // the file which attributes/numeric data are in
  size_t      MaxEntries; // current maximum size of table - see Resize()
  enum {Unsorted, ByGp, ByNorth, ByEast, ByWest, BySouth, ByExtent}  Sorted;
};

typedef BBOXLIST* PBBOXLIST;
#endif



