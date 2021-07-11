// $Id: nlist.hxx,v 1.1 2007/05/15 15:47:20 edz Exp $

/*@@@
File:		nlist.hxx
Version:	1.00
$Revision: 1.1 $
Description:	Class NLIST - Utilities for lists of numeric values
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

#ifndef NUMERICLIST_HXX
#define NUMERICLIST_HXX

#include <stdlib.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"

#ifndef DATELIST_HXX
// SearchState is used to indicate the results of the matcher 
enum SearchState { NO_MATCH=-1, TOO_LOW, TOO_HIGH, MATCH };

// IntType tells the matcher where we are in the block of values
enum IntType { AT_START, INSIDE, AT_END };

// NumBlock tells LoadTable which sorted block to load - the blocks are
// sorted by start value, end value and global pointer
enum NumBlock { NONE, VAL_BLOCK, GP_BLOCK };
typedef NumBlock blktype;

#endif


#ifndef HAVE_SORTTYPE
# define HAVE_SORTTYPE 1
  enum stype {EQ,LT,LE,GT,GE,NE};
  typedef enum stype SortType;
#else
  extern stype SortType;
#endif

#ifndef HAVE_STATUS
# define HAVE_STATUS 1
  enum mstatus {MATCH_FAILED=-1,  /* failed for unknown reasons */
                MATCH_GOOD,       /* finished the search */
                MATCH_LOWER,      /* key matches lower bound */
                MATCH_MID,        /* key is in range */
                MATCH_UPPER};     /* key matches upper bound */
  typedef mstatus Status;
#else
  extern mstatus Status;
#endif

class NUMERICLIST {
private:
  NUMERICFLD  *table;      // the table of attribute/numeric data
  size_t       Count;      // count of items in table
  INT          Attribute;  // which USE attribute this table maps
  size_t       Pointer;    // current position
  size_t       MaxEntries; // current maximum size of table - see Resize()
  size_t       StartIndex;
  size_t       EndIndex;
  ZRelation_t  Relation;   // what relation generated StartIndex, EndIndex
  STRING       FileName;   // the file which attributes/numeric data are in
  INT          Ncoords;    // Number of values in an entry
  blktype      table_type; // how is the data in table sorted?

  SearchState  MemFind(NUMBER Key, ZRelation_t Relation, INT4 *Index);
  SearchState  MemFind(GPTYPE Key, ZRelation_t Relation, INT4 *Index);
#if 1
  SearchState  MemFindGp(GPTYPE Key, ZRelation_t Relation, INT4 *Index);
  SearchState  MemFindIndexes(NUMBER Key, ZRelation_t Relation, INT4 *StartIndex, INT4 *EndIndex);
#endif
  SearchState  DiskFind(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *Index);
  SearchState  DiskFind(STRING Fn, GPTYPE Key, ZRelation_t Relation, INT4 *Index);

  SearchState  Matcher(NUMBER Key, NUMBER A, NUMBER B, NUMBER C, 
		       ZRelation_t Relation, INT4 Type);
  SearchState  Matcher(GPTYPE Key, GPTYPE A, GPTYPE B, GPTYPE C, 
		       ZRelation_t Relation, INT4 Type);

#if 1
  Status        n_compare(NUMBER val, NUMBER key);
  Status        LinearMatchUpGT(NUMBER Key, int bottom,int top,int* Index);
  Status        LinearMatchDownLT(NUMBER Key,int bottom,int top,int* Index);
  Status        LinearMatchLE(NUMBER Key,int bottom,int top,int* Index);
  Status        LinearMatchGE(NUMBER Key,int bottom,int top,int* Index);
  Status        BestMatchLE(NUMBER Key,int first,int last, int* Index);
  Status        BestMatchGE(NUMBER Key,int first,int last, int* Index);
  Status        Match(SortType Relation, NUMBER Key,int* Index);
  Status        MatchGp(SortType Relation,int Key,int* Index);
#endif
public:
  NUMERICLIST();
  NUMERICLIST(INT n);

  void         Clear();
  void         Empty();

  void         SetRelation(ZRelation_t r) { Relation = r; };
  INT          GetRelation()              { return(Relation); };
  NUMBER       GetNumericValue(INT i)     { return(table[i].GetNumericValue()); };
  // return the USE attribute for this table
  INT          GetAttribute()             { return(Attribute); };
  // set the USE attribute
  void         SetAttribute(INT x)        { Attribute = x; }; 
  // get number of items in table
  INT4         GetCount()                 { return(Count); };
  // make room for more entries
  GDT_BOOLEAN  Expand(size_t Entries);
  GDT_BOOLEAN  Expand()                   { return Resize(Count + (50*Ncoords)); };
  // collapse size to total entries
  void         Cleanup()                  { Resize(Count); };
  GPTYPE       GetGlobalStart(INT4 i)     { return(table[i].GetGlobalStart()); };
  // set file name to load from
  void         SetFileName(STRING s)      { FileName = s; };
  // set file name to load from
  void         SetFileName(PCHR s)        { FileName = s; };
  void         SetCoords(INT n)           { Ncoords = n; };
  INT          GetCoords()                { return Ncoords; };

  void         WriteIndex(const STRING& Fn);
  FILE        *OpenForAppend(const STRING& Fn);

  ///
  NUMERICFLD  operator[](signed n) const   {return table[n]; }
  NUMERICFLD  operator[](unsigned n) const {return table[n]; }
  NUMERICFLD& operator[](unsigned n)       {return table[n]; }

  SearchState  Find(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *Index);
  SearchState  Find(STRING Fn, GPTYPE Key, ZRelation_t Relation, INT4 *Index);
  SearchState  Find(NUMBER Key, ZRelation_t Relation, INT4 *Index);
  SearchState  Find(GPTYPE Key, ZRelation_t Relation, INT4 *Index);

  SearchState  FindIndexes(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *StartIndex, INT4 *EndIndex);
  void         Sort();                     // sort numeric field items
  void         SortByGP();                 // sort by global ptr
  void         Dump(ostream& os = cout) const;   // dump all numeric field data
  void         Dump(INT4 start, INT4 end, ostream& os=cout) const; // dump numeric field data
  GDT_BOOLEAN  Resize(size_t Entries);       // resize table to Entries size
  void         TempLoad();                 // test routine
  size_t       LoadTable(NumBlock Offset);
  size_t       LoadTable(INT4 Start, INT4 End);
  size_t       LoadTable(INT4 Start, INT4 End, NumBlock Offset);
  void         WriteTable();
  void         WriteTable(INT Offset);
  void         ResetHitPosition();
  GPTYPE       GetNextHitPosition();

  ~NUMERICLIST();
};

typedef NUMERICLIST* PNUMERICLIST;
#endif
