/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
// $Id: gpolylist.hxx,v 1.1 2007/05/15 15:47:20 edz Exp $

/*@@@
File:		gpolylist.hxx
Version:	1.00
$Revision: 1.1 $
Description:	Class GPOLYLIST - Utilities for lists of spatial polygons
@@@*/

#ifndef GPOLYLIST_HXX
#define GPOLYLIST_HXX

#include <stdlib.h>
#include <time.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "fc.hxx"
//#include "nlist.hxx"
#include "gpolyfield.hxx"

// IntBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
//enum IntBlock { START_BLOCK, END_BLOCK, PTR_BLOCK };


class GPOLYLIST 
{
private:
    
  GPOLYFLD    *table;      // the table of attribute/numeric data
  INT4         Count;      // count of items in table
  STRING       FileName;   // the file which attributes/numeric data are in
  INT4         MaxEntries; // current maximum size of table - see Resize()
  bool  Sorted;     // Have we done the optimization or not?

public:
  GPOLYLIST();
  GPOLYLIST(INT n);
  void   Write(FILE *fp);
  void   WriteTable();

  INT    Read(FILE *fp);
  INT    LoadRawTable();
  INT    LoadTable(STRING& Fn);
  void   Dump(ostream& os = cout) const;
  void   Sort();

  void   SetFileName(STRING s)  { FileName = s; }
  void   SetFileName(CHR *s)    { FileName = s; }
  STRING GetFileName()          { return FileName; }

  void   SetCount(INT x)        { Count = x; }
  INT    GetCount()             { return Count; }

  GPOLYFLD *GetGpolyByGp(const FC& Fc) const;
  GPOLYFLD *GetGpolyByGp(GPTYPE GpStart, GPTYPE GpEnd) const;
  GPOLYFLD *GetGpolyByIndex(INT x) const;


  void   Clear();
  void   Empty();

  // make room for more entries
  void   Expand();
  // collapse size to total entries
  void   Cleanup()              { Resize(Count); }
  void   Resize( INT4 Entries);      // resize table to Entries size

  void         WriteIndex(const STRING& Fn);
  FILE        *OpenForAppend(const STRING& Fn);

  ///
//GPOLYFLD  operator[](signed n) const   {return table[n]; }
  GPOLYFLD  operator[](unsigned n) const {return table[n]; }
  GPOLYFLD& operator[](unsigned n)       {return table[n]; }

  ~GPOLYLIST();

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

};

typedef GPOLYLIST* PGPOLYLIST;
#endif



