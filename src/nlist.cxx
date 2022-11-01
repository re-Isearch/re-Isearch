/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/* $Id: nlist.cxx,v 1.3 2007/07/04 06:21:59 edz Exp edz $ */

/*@@@
File:		nlist.cxx
Version:	1.00
$Revision: 1.3 $
Description:	Class NUMERICLIST
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/

#include <stdlib.h>
#include <errno.h>
#include "common.hxx"
#include "nlist.hxx"
#include "magic.hxx"


#ifndef LINEAR_MAX
# define LINEAR_MAX 4
#endif

#define DEBUG 0
#define NEW_CODE 1

typedef UINT4  _Count_t;

#define SIZEOF_HEADER  sizeof(_Count_t)+1 /* size of magic */

void NUMERICLIST::Clear()
{
  Count      = 0;
  FileName.Empty();
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}

void NUMERICLIST::Empty()
{
  if(table) delete [] table;
  MaxEntries = 0;
  table      = NULL;

  Count      = 0;
  FileName.Empty();   
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}



NUMERICLIST::NUMERICLIST()
{
  Ncoords    = 2;
  MaxEntries = 0;
  table      = NULL;
  Count      = 0;
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}


NUMERICLIST::NUMERICLIST(INT n)
{
  Ncoords    = n;
  MaxEntries = 0;
  table      = NULL; 
  Count      = 0;
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}


void NUMERICLIST::ResetHitPosition()
{
  Pointer = ( (Relation != 1) ?  StartIndex : 0 );
}


GPTYPE NUMERICLIST::GetNextHitPosition()
{
  GPTYPE Value;

#if DEBUG
  cerr << "GetNextPosition:  StartIndex=" << StartIndex << "  EndIndex=" << EndIndex << endl;
#endif

  if(StartIndex == EndIndex) // was StartIndex == -1
    return((GPTYPE)-1);
  if(Relation != 1) {
    if(Pointer>EndIndex)
      return((GPTYPE)-1);
    Value=table[Pointer].GetGlobalStart();
    ++Pointer;
    return(Value);
  } else {
    if(Pointer >= StartIndex)
      while(Pointer <= EndIndex)
	++Pointer;
    if(Pointer >= Count)
      return((GPTYPE)-1);
    Value=table[Pointer].GetGlobalStart();
    ++Pointer;
    return(Value);
  }
}


static INT SortCmp(const void* x, const void* y) 
{
  NUMBER a=((*((PNUMERICFLD)x)).GetNumericValue()) - ((*((PNUMERICFLD)y)).GetNumericValue()) ;
//   return Compare( (*((PDATEFLD)x)).GetValue(), (*((PDATEFLD)y)).GetValue() );
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT SortCmpGP(const void* x, const void* y) 
{
  const GPTYPE gp1 = (*((PNUMERICFLD)x)).GetGlobalStart();
  const GPTYPE gp2 = (*((PNUMERICFLD)y)).GetGlobalStart();

  if (gp1 > gp2) return  1;
  if (gp1 < gp2) return -1;
  return 0;

}


bool NUMERICLIST::Expand(size_t Entries)
{
  if (Entries < MaxEntries)
    return true;
  return Resize(Entries +  (50*Ncoords));
}


bool NUMERICLIST::Resize(size_t Entries)
{
  if (Entries == 0)
    {
      Empty();
      return true;
    }

  NUMERICFLD *temp;

  try {
    temp =new NUMERICFLD[Entries];
  } catch(...) {
    message_log (LOG_PANIC|LOG_ERRNO, "Could not allocate numerical list (%ld)",
	(long)Entries);
    return false;
  }
  size_t      CopyCount;

  if(Entries>Count)
    CopyCount=Count;
  else
    Count = (CopyCount=Entries);

  for(size_t x=0; x<CopyCount; x++)
    temp[x]=table[x];

  if(table)
    delete [] table;

  table=temp;
  MaxEntries=Entries;
  return true;
}


void NUMERICLIST::Sort()
{
  QSORT((void *)table, Count, sizeof(table[0]),SortCmp);
}


void NUMERICLIST::SortByGP()
{
  QSORT((void *)table, Count, sizeof(table[0]),SortCmpGP);
}


SearchState NUMERICLIST::Matcher(NUMBER Key, NUMBER A, NUMBER B, NUMBER C, 
	ZRelation_t Relation, INT4 Type)
{
	
  switch (Relation) {
  case ZRelGE:			// greater than or equals
    if ((B>=Key) && (Type==-1 || A<Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>=Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelGT:			// greater than
    if ((B>Key) && (Type==-1 || A<=Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelLE:			// less than or equals
    if ((B<=Key) && (Type==0 || C>Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<=Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

  case ZRelLT:			// less than
    if ((B<Key) && (Type==0 || C>=Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);
  default:
    message_log (LOG_DEBUG, "No semantics for relation=%d", Relation);
    break;
  }

  message_log (LOG_PANIC, "Hideous Matching Error");
  return(NO_MATCH);
}


// A:= Lower B:= Value C:= Upper
SearchState NUMERICLIST::Matcher(GPTYPE Key, GPTYPE A, GPTYPE B, GPTYPE C, 
		     ZRelation_t Relation, INT4 Type)
{
  switch (Relation) {
  case ZRelGE:			// greater than or equals
    if ((B>=Key) && (Type==-1 || A<Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>=Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelGT:			// greater than
    if ((B>Key) && (Type==-1 || A<=Key))
      return(MATCH);		// exact place - lower boundary
    else if (A>Key)
      return(TOO_LOW);		// key too low
    else
      return(TOO_HIGH);		// key too high

  case ZRelLE:			// less than or equals
    if ((B<=Key) && (Type==0 || C>Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<=Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

  case ZRelLT:			// less than
    if ((B<Key) && (Type==0 || C>=Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

   default:
     message_log (LOG_DEBUG, "No semantics for relation=%d", Relation);
     break;
  }

  message_log (LOG_PANIC, "Hideous Matching Error");
  return(NO_MATCH);
}


// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState NUMERICLIST::Find(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *Index)
{
  SetFileName(Fn);

#if DEBUG
  cerr << "Find(" << Fn << ", " << Key << ", " << Relation << ")" << endl;
#endif

#if NEW_CODE
  if (LoadTable(GP_BLOCK) >= 0)
    return MemFind(Key, Relation, Index);
#endif
  return DiskFind(Fn, Key, Relation, Index);
}


SearchState NUMERICLIST::Find(NUMBER Key, ZRelation_t Relation, INT4 *Index)
{
#if NEW_CODE
  INT nRecs = 0;
  if (Count <= 0)
    nRecs = LoadTable(GP_BLOCK);
  if (nRecs >0)
    return MemFind(Key, Relation, Index);
#endif
  return DiskFind(FileName, Key, Relation, Index);
}

#if 0
SearchState NUMERICLIST::MemFind(NUMBER Key, ZRelation_t Relation, INT4 *Index)
{
  return NO_MATCH;
}
#else

SearchState NUMERICLIST::MemFind(NUMBER Key, ZRelation_t Relation, INT4 *Index) {
  SearchState fStatus = NO_MATCH;
  Status search_status = MATCH_FAILED;

  INT4   index = -1;

  // Linear search for now
  switch (Relation) {

  case ZRelEQ:  // find all table values which equal Key
    //    search_status=Match(GE,Key,&first);
    //    if (search_status == MATCH_GOOD)
    //      search_status=Match(LE,Key,&last);
    break;

  case ZRelLT:  // find all table values which are less than Key
    search_status=Match(LT,Key,&index);
    break;

  case ZRelLE:  // find all table values which are less than or equal to Key
    search_status=Match(LE,Key,&index);
    break;

  case ZRelGT:  // find all table values which are greater than Key
    search_status=Match(GT,Key,&index);
    break;

  case ZRelGE:  // find all table values which are greater than or equal to Key
    search_status=Match(GE,Key,&index);
    break;

  case ZRelNE:  // not going there...
    break;

  default:
    message_log (LOG_DEBUG, "No semantics for relation=%d", Relation);
    break;

  }

  if (search_status == MATCH_FAILED)
    return NO_MATCH;

  else if (index < 0) // no lower bound
    fStatus = TOO_LOW;

  else if (index > Count) // no upper bound
    fStatus = TOO_HIGH;

  else
    fStatus = MATCH;

  *Index = index;

  return fStatus;
}

SearchState NUMERICLIST::MemFindGp(GPTYPE Key, ZRelation_t Relation, INT4 *Index)
{
  SearchState Status = NO_MATCH;
  INT4        x,found;
  GPTYPE      val;

  if (Count == 0)
   {
     *Index = 0;
     return NO_MATCH;
   }

  found = -1;

  // Linear search for now
  switch (Relation) {

  case ZRelEQ:  // find first index for which table value equals Key
    for(x=0; x<Count; x++) {
      val = table[x].GetGlobalStart();
      // Find the first matching lower bound, then stop looking for those
      if ((found < 0) && (val == Key)) {
        found=x;
      }
    }
    break;

  case ZRelLT:  // find index of first table value which is less than Key
    x=0;
      val = table[x].GetGlobalStart();
    while (val<Key && x<Count) {
      found = x;
      x++;
      val = table[x].GetGlobalStart();
    }
    break;

  case ZRelLE:  // find index of table value which is less than or equal to Key
    x=0;
      val = table[x].GetGlobalStart();
    while (!(val>Key) && x<Count) {
      found = x;
      x++;
      val = table[x].GetGlobalStart();
    }
    break;

  case ZRelGT:  // find index of table value which is greater than Key
    x=Count-1;
    val = table[x].GetGlobalStart();
    while (val>Key && x>0) {
      found = x--;
      val = table[x].GetGlobalStart();
    }
    break;

  case ZRelGE:  // find index of table values which is greater than or equal to Key
    x=Count-1;
    val = table[x].GetGlobalStart();
    while (!(val<Key) && x>0) {
      found = x--;
      val = table[x].GetGlobalStart();
    }
    break;

  case ZRelNE:  // not going there...
    break;

   default:
     message_log (LOG_DEBUG, "No semantics for relation=%d", Relation);
     break;
  }

  if (found < 0) // nothing matched
    Status = NO_MATCH;
  else if (found >= Count) // no upper bound
    Status = NO_MATCH;
  else
    Status = MATCH;

  *Index = found;
  return Status;
}

SearchState NUMERICLIST::FindIndexes(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *StartIndex, INT4 *EndIndex)
{
  SearchState Status = NO_MATCH;
  INT nRecs;

  SetFileName(Fn);
  nRecs = LoadTable(VAL_BLOCK); // Load up the data

  if (nRecs >= 0) {
    Status =
      MemFindIndexes(Key, Relation, StartIndex, EndIndex);
  } else {
    // Hmmm... failed to lead the table into memory, so try disk search
    //    Status =
    DiskFind(FileName, Key, Relation, StartIndex);
  }
  return Status;
}

SearchState NUMERICLIST::MemFindIndexes(NUMBER Key, ZRelation_t Relation, INT4 *StartIndex, INT4 *EndIndex)
{
  SearchState fStatus = NO_MATCH;
  Status      search_status = MATCH_FAILED;
  INT4        first = -1;
  INT4        last  = -1;

  // Linear search for now
  switch (Relation) {

  case ZRelEQ:  // find all table values which equal Key
    search_status=Match(GE,Key,&first);
    if (search_status == MATCH_GOOD)
      search_status=Match(LE,Key,&last);
    break;

  case ZRelLT:  // find all table values which are less than Key
    search_status=Match(LT,Key,&last);
    if (last >= 0)
      first=0;

    break;

  case ZRelLE:  // find all table values which are less than or equal to Key
    search_status=Match(LE,Key,&last);
    if (last >= 0)
      first=0;
    break;

  case ZRelGT:  // find all table values which are greater than Key
    search_status=Match(GT,Key,&first);
    if (first >= 0)
      last=Count-1;
    break;

  case ZRelGE:  // find all table values which are greater than or equal to Key
    search_status=Match(GE,Key,&first);
    if (first >= 0)
      last=Count-1;
    break;

  case ZRelNE:  // not going there...
    break;

  default:
   message_log (LOG_DEBUG, "No semantics for relation=%d", Relation);
   break;
  }

  if (search_status == MATCH_FAILED)
    return NO_MATCH;

  if ((first < 0) && (last < 0)) // nothing matched
    fStatus = NO_MATCH;

  else if (first < 0) // no lower bound
    fStatus = TOO_LOW;

  else if (last < 0) // no upper bound
    fStatus = TOO_HIGH;

  else
    fStatus = MATCH;

  *StartIndex = first;
  *EndIndex = last;
  return fStatus;
}

#endif


SearchState NUMERICLIST::DiskFind(STRING Fn, NUMBER Key, ZRelation_t Relation, INT4 *Index)
{
  //  INT4 B;
  PFILE  Fp = fopen(Fn, "rb");

  if (Fp == NULL) {

    message_log (LOG_ERRNO, "Can't open numeric list index '%s'", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {
    NUMERICFLD lowerBound, upperBound, numericValue;
    _Count_t     Total;
    const size_t ElementSize = numericValue.Sizeof(); // sizeof(GPTYPE) + sizeof(NUMBER);

    if (getObjID(Fp) != objNLIST)
      {
	fclose(Fp);
        message_log (LOG_PANIC, "%s not a numerical index??", Fn.c_str());
        *Index = -1;
        return NO_MATCH;
      }

    Read(&Total, Fp);
#if DEBUG
    cerr << Total << " elements" << endl;
#endif

    if (Total == 0) return NO_MATCH;

    long Low = 0;
    long High = Total - 1;
    long OX, X = High / 2;

    enum IntType Type=AT_START;

    do {
      OX = X;

      if ((X > 0) && (X < High)) {
	if (-1 == fseek(Fp, SIZEOF_HEADER + (X-1) * ElementSize, SEEK_SET))
	  message_log (LOG_ERRNO, "Seek failure on %s [X-1//X=%ld]", Fn.c_str());
	Type=INSIDE;

      } else if (X <= 0) {
	if (-1 == fseek(Fp, SIZEOF_HEADER + X * ElementSize, SEEK_SET))
	  message_log (LOG_ERRNO, "Seek failure on %s [X=%ld]", Fn.c_str(), (long)X);
	Type=AT_START;

      } else if (X >= High) {
	if (-1 == fseek(Fp, SIZEOF_HEADER + (X-1) * ElementSize, SEEK_SET))
	  message_log (LOG_ERRNO, "Seek failure on %s [X-1//X=%ld]", Fn.c_str(), (long)X);
	Type=AT_END;
      }
	
      if (Type != AT_START)
	lowerBound.Read(Fp);

      numericValue.Read(Fp);

      // If we're at the start, we need to read the first value into
      // NumericValue, but we don't want to leave LowerBound 
      // uninitialized.  This will also handle the case when we only
      // have two values in the index.
      if (Type == AT_START)
	lowerBound = numericValue;
	
      if(Type != AT_END)
	upperBound.Read(Fp);

	
      // Similarly, if we're at the end and can't read in a new value
      // for UpperBound, we don't want it uninitialized, either.
      if (Type == AT_END)
	upperBound = numericValue;

      SearchState State = Matcher(Key, lowerBound.GetNumericValue(),
	numericValue.GetNumericValue(),
	upperBound.GetNumericValue(),
	Relation, Type);
	
      if (State == MATCH) {
	// We got a match
	fclose(Fp);
	*Index = X;
	return MATCH;
      } else if ((State == TOO_HIGH) && (Type == AT_END)) {
	// We didn't get a match, but we ran off the upper end, so
	// the key is bigger than anything indexed
	fclose(Fp);
	*Index = -1;
	return State;
      } else if ((State == TOO_LOW) && (Type == AT_START)) {
	// We didn't get a match, but we ran off the lower end, so 
	// the key is smaller than anything indexed
	fclose(Fp);
	*Index = -1;
	return State;
      } else if (Low >= High) {
	// If Low is >= High, there aren't any more values to check
	// so we're done whether we got a match or not, and if we got
	// here, there wasn't a match.  This probably won't happen - 
	// at least, we expect that these conditions will be caught
	// by one of the preceeding, but it pays to be safe.
	fclose(Fp);
	*Index = -1;
	return NO_MATCH;
      }

      if (State == TOO_LOW) {
	High = X;
      } else {
	Low = X + 1;
      }
	
      X = (Low + High) / 2;
      if (X < 0) {
	X = 0;
      } else {
	if (X >= Total) {
	  X = Total - 1;
	}
      }
    } while (X != OX);
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


// Search for the GP INT4 values
// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState NUMERICLIST::Find(STRING Fn, GPTYPE Key, ZRelation_t Relation, INT4 *Index)
{
  SetFileName(Fn);

#if NEW_CODE
  if (LoadTable(GP_BLOCK) >= 0)
    return MemFindGp(Key, Relation, Index);
#endif

  return DiskFind(Fn, Key, Relation, Index);
}


SearchState NUMERICLIST::Find(GPTYPE Key, ZRelation_t Relation, INT4 *Index)
{
#if NEW_CODE
  INT nRecs = 0;   
  if (Count <= 0)
    nRecs = LoadTable(GP_BLOCK);
  if (nRecs >0)
    return MemFindGp(Key, Relation, Index);
#endif
  return  DiskFind(FileName, Key, Relation, Index);
}


SearchState NUMERICLIST::MemFind(GPTYPE Key, ZRelation_t Relation, INT4 *Index)
{
#if NEW_CODE
  return MemFindGp(Key, Relation, Index);
#else
  return NO_MATCH;
#endif
}


// This one searches for Gps!!!
SearchState NUMERICLIST::DiskFind(STRING Fn, GPTYPE Key, ZRelation_t Relation, INT4 *Index)
{
  PFILE  Fp = fopen(Fn, "rb");

#if DEBUG
  cerr << "DiskFind (" << Fn << ", " << Key << ", " << Relation << ")" << endl;
#endif

  if (!Fp) {
    message_log (LOG_ERRNO, "Index open faolure '%s'", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {

    INT4         Total;
    INT          Low, High, X, OX;
    SearchState  State;
    enum IntType Type=AT_START;

    if (getObjID(Fp) != objNLIST)
      {
	fclose(Fp);
        message_log (LOG_PANIC, "%s not a numerical index??", FileName.c_str());
	*Index = -1;
        return NO_MATCH;
      }

    Read(&Total,Fp);

#if DEBUG
   cerr << "Total = " << Total << endl;
#endif

    Low = 0;
    High = Total - 1;
    X = High / 2;

    NUMERICFLD Value, Lower, Upper;
    const size_t  ElementSize = Value.Sizeof();


    do {
      OX = X;

      if ((X > 0) && (X < High)) {
	fseek(Fp, SIZEOF_HEADER + (Total+X-1) * ElementSize, SEEK_SET);
	Type=INSIDE;

      } else if (X <= 0) {
	fseek(Fp, SIZEOF_HEADER + (Total+X) * ElementSize, SEEK_SET);
	Type=AT_START;

      } else if (X >= High) {
	fseek(Fp, SIZEOF_HEADER + (Total+X-1) * ElementSize, SEEK_SET);
	Type=AT_END;
      }
	
      if (Type != AT_START)
	Read(&Lower, Fp);
	
      Read(&Value, Fp);

      // If we're at the start, we need to read the first value into
      // NumericValue, but we don't want to leave LowerBound 
      // uninitialized.  This will also handle the case when we only
      // have two values in the index.
      if (Type == AT_START)
	Lower = Value;
	
      if(Type != AT_END)
	Read(&Upper,Fp);
	
      // Similarly, if we're at the end and can't read in a new value
      // for UpperBound, we don't want it uninitialized, either.
      if (Type == AT_END)
	Upper = Value;

      State = Matcher(Key,
	(GPTYPE)Lower.GetGlobalStart(), (GPTYPE)Value.GetGlobalStart(), (GPTYPE)Upper.GetGlobalStart(),
	Relation, Type);
	
      if (State == MATCH) {
	// We got a match
	fclose(Fp);
	*Index = X;
	return MATCH;
      } else if ((State == TOO_HIGH) && (Type == AT_END)) {
	// We didn't get a match, but we ran off the upper end, so
	// the key is bigger than anything indexed
	fclose(Fp);
	*Index = -1;
	return State;
      } else if ((State == TOO_LOW) && (Type == AT_START)) {
	// We didn't get a match, but we ran off the lower end, so 
	// the key is smaller than anything indexed
	fclose(Fp);
	*Index = -1;
	return State;
      } else if (Low >= High) {
	// If Low is >= High, there aren't any more values to check
	// so we're done whether we got a match or not, and if we got
	// here, there wasn't a match.  This probably won't happen - 
	// at least, we expect that these conditions will be caught
	// by one of the preceeding, but it pays to be safe.
	fclose(Fp);
	*Index = -1;
	return NO_MATCH;
      }

      if (State == TOO_LOW) {
	High = X;
      } else {
	Low = X + 1;
      }
	
      X = (Low + High) / 2;
      if (X < 0) {
	X = 0;
      } else {
	if (X >= Total) {
	  X = Total - 1;
	}
      }
    } while (X != OX);
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}




void NUMERICLIST::Dump(ostream& os) const
{
  for(_Count_t x=0; x<Count; x++)
    table[x].Dump(os);
}


void NUMERICLIST::Dump(INT4 start, INT4 end, ostream& os) const
{
  if (start < 0)
    start = 0;
  if (end > Count)
    end = Count;

  for(_Count_t x=start; x<end; x++)
    table[x].Dump(os);
}


// Load the entire table in one gulp
size_t NUMERICLIST::LoadTable(NumBlock Offset)
{
  if (FileName.IsEmpty()) {
    message_log (LOG_PANIC, "Numeric List FileName not set");
    return 0;
  }
  if (table_type == Offset)
    return Count;

  FILE  *fp; 
  size_t nRecs=0;
  if ((fp = fopen(FileName,"rb")) != NULL) {
    // Bump past Count, then offset to the right starting point
    // Note - there are two tables in the file - one sorted by starting
    // value, one sorted by ending value.  There are Count entries in each
    // version.
    _Count_t nCount;
    ::Read(&nCount, fp);
    nRecs = LoadTable(0,nCount-1,Offset);
    fclose(fp);
    table_type = Offset;
  } else {
    table_type = NONE;
  }
  return nRecs;
}


size_t NUMERICLIST::LoadTable(INT4 Start, INT4 End)
{
  FILE   *fp;
  size_t  nRecs=0;
  
 if (FileName.IsEmpty() ) {
    message_log (LOG_PANIC, "Numeric List FileName not set");
    return 0;
  }

#if DEBUG
  cerr << "NUMERICLIST::LoadTable(" << Start << "," << End << ")" << endl; 
#endif

  const off_t  fileSize = GetFileSize(FileName);
  const size_t size     = table[0].Sizeof();
  const long   Elements = fileSize/size;

  if (Elements == 0)
    {
      if (!FileExists(FileName))
	message_log (LOG_ERROR, "NUMERICLIST: '%s' does not exist?", FileName.c_str());
      else if (fileSize == 0)
	message_log (LOG_WARN, "NUMERICLIST: '%s' is empty (%lu/%u)!", FileName.c_str(),
		(unsigned long)fileSize, size);
      else
	message_log (LOG_ERROR, "NUMERICLIST: Short write (%lu/%u)?",
		(unsigned long)fileSize, size); 
      return nRecs;
    }
  if (Start > Elements)
    {
      message_log (LOG_WARN, "NUMERICLIST: Start %d > element count (%ld). Nothing to load.",
        (int)Start, Elements);
      return nRecs;
    }

  
  if ((fp=fopen(FileName,"rb")) != NULL) {

    if ((End==-1) || (End>=Elements))   End  = (INT4)(Elements - 1);
    if (Start==-1)                      Start= 0;

    if (Start)
      {
	if (fseek(fp, (off_t)Start*size, SEEK_SET) != 0)
	  message_log (LOG_ERRNO, "Can't seek in '%s'[%ld]", FileName.c_str(), (long) Start*size);
      }
    Resize(Count + End-Start + 1);

    for (size_t i=Start;i<=End;i++){
      if (feof(fp))
	break;
#if DEBUG
      cerr << "Read element " << Count << endl;
#endif
      ::Read(&table[Count], fp);
      nRecs++;
      if(++Count==MaxEntries)
	{
	  if (Expand() == false)
	    break;
	}
    }
    fclose(fp);
    //Cleanup();
  } else
    message_log (LOG_ERROR, "Could not open '%s'", FileName.c_str());

  return nRecs;
}


size_t NUMERICLIST::LoadTable(INT4 Start, INT4 End, NumBlock Offset)
{
#if DEBUG
  cerr << "NUMERICLIST::LoadTable(" << Start << "," << End << "," << (int)Offset << ")" << endl;
#endif

  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // Return the actual number of items read in this load
  size_t nRecs = 0;
  const size_t size = table[0].Sizeof();

  if (FileName.IsEmpty()) {
    message_log (LOG_PANIC, "Numeric List FileName not set");
    return 0;
  }
  if (GetFileSize(FileName) == 0)
    return 0; // Empty

  FILE *fp = fopen(FileName,"rb");
  if (fp) {

    if (getObjID(fp) != objNLIST)
      {
	fclose(fp);
        message_log (LOG_PANIC, "%s not a Numerical list??", FileName.c_str());
	return 0;
      }

    // Bump past Count, then offset to the right starting point
    // Note - there are two tables in the file - one sorted by starting
    // value, one sorted by ending value.  There are Count entries in each
    // version.  
    _Count_t nCount;

    Read(&nCount, fp);

    if ((End >= nCount) || (End < 0))
      End = nCount-1;

    if (--Start < 0) // Was 2007 Dec 10:  if (Start < 0)  we now decrement
      Start = 0;

#if DEBUG
    cerr << "Start = " << Start << "  End =" << End << "   nCount= " << nCount << endl;
#endif
  
    if ((Start < nCount) && !feof(fp)) {
      off_t MoveTo;

      if (Offset == VAL_BLOCK)
	MoveTo = SIZEOF_HEADER + (Start * size);
      else if (Offset == GP_BLOCK)
	MoveTo = SIZEOF_HEADER + ((nCount+Start) * size);
      else 
	MoveTo = 0;

#if DEBUG
      cerr << "Moving " << MoveTo << " bytes into the file and reading "
	   << (End - Start +1) << " elements (of " << nCount << ") starting at table[" << Count << "]"
	   << endl;
#endif
      if (MoveTo != SIZEOF_HEADER)
	if (-1 == fseek(fp, MoveTo, SEEK_SET))
	  message_log (LOG_ERRNO, "Seek failure to %lu on '%s'", (unsigned long)MoveTo, FileName.c_str());

      errno = 0;
      Resize (Count + End - Start + 1);
      for (size_t i=Start; i<=End; i++) {
	if (feof(fp))
	  {
	    message_log (LOG_ERRNO, "Premature numeric list read-failure in '%s' (%d of %d)",
		FileName.c_str(), i, End);
	    break;
	  }
	::Read(&table[Count],  fp);
#if DEBUG
	cerr << "Read " << table[Count] << endl;
#endif

	nRecs++;
	if(++Count == MaxEntries)
	  if (Expand() == false)
	    break;
      }
    }
    fclose(fp);
  } else
    message_log (LOG_ERROR, "Could not open '%s'", FileName.c_str());
#if DEBUG
  cerr <<"Read " << nRecs << endl;
#endif
  return nRecs;
}


// First generation write
void NUMERICLIST::WriteTable()
{
  FILE  *fp;

  if ( FileName.IsEmpty() ) 
    return;

#if DEBUG
  cerr << "First generation WriteTable" << endl;
#endif

  if ((fp=fopen(FileName,"wb")) != NULL)
    {
      for(size_t i=0; i<Count; i++)
	Write(table[i], fp);
      fclose(fp);
    }
}


// Write the combined table
//
void NUMERICLIST::WriteTable(INT Offset)
{
  FILE  *fp;

  if ( FileName.IsEmpty())
    return;

  // Offset = 0 is start of file
  if ((fp = fopen (FileName, (Offset == 0) ? "wb" : "a+b")) != NULL)
    {
      // First, write out the count
      if (Offset == 0)
	{
	  putObjID(objNLIST, fp);
	  Write((_Count_t)Count, fp);
	}

      // Now, go to the specified offset and write the table
      off_t  MoveTo = (off_t)(Offset* table[0].Sizeof()) + SIZEOF_HEADER;

      // cout << "Offsetting " << MoveTo << " bytes into the file." << endl;

      if (fseek(fp, MoveTo, SEEK_SET) == 0)
	{
	  for (_Count_t x=0; x<Count; x++)
	    Write(table[x], fp);
	}
      else message_log (LOG_ERRNO, "Seek error in NUMERICLIST");
      fclose(fp);
   }
}


void NUMERICLIST::WriteIndex(const STRING& Fn)
{
  SetFileName(Fn);

#if DEBUG
  cerr << "Load table... " << endl;
#endif
  LoadTable(0,-1);
  if (Count > 0)
    {
      Sort();  
      WriteTable(0);
      SortByGP();
      WriteTable(Count);
    }
}


// Reduce the two tables down to one to allow for a simple
// write append
//
FILE *NUMERICLIST::OpenForAppend(const STRING& Fn)
{
  // New file?
  if (GetFileSize(Fn) == 0)
    return fopen(Fn, "a+b");

  FILE *Fp = fopen(Fn, "rb"); // Read and write

  if (Fp == NULL)
   {
      message_log (LOG_ERRNO, "NUMERICLIST:: Can't open '%s' for reading/writing", Fn.c_str());
      return NULL;
   }
  if (getObjID(Fp)!= objNLIST)
    {
      fclose(Fp);
      return fopen(Fn, "a+b");
    }

  _Count_t Total;
  Read(&Total, Fp);
  if (Total == 0) 
    {
      // Nothing in there so
      fclose(Fp);
      return fopen(Fn, "wb"); // Can start from scratch
    }

  STRING TmpName = Fn + "~";

  // Look for a non-existing....
  for (size_t i =0; FileExists(TmpName); i++)
    {
      TmpName.form ("%s.%d", Fn.c_str(), (int)i);
    }
  // Open it... (Racing condition??)
  FILE *oFp = fopen(TmpName, "wb");
  if (oFp == NULL)
    {
      // Fall into scatch (we might not have writing permission
      STRING tmp = GetTempFilename(Fn, true) ;

      message_log (LOG_WARN, "Could not create '%s', trying tmp '%s'", TmpName.c_str(), tmp.c_str());
      if ((oFp = tmp.fopen("wb")) == NULL)
	{
	  message_log (LOG_ERRNO, "Can't create a temporary numlist '%s'", Fn.c_str());
	  fclose(Fp);
	  return NULL;
	}
      TmpName = tmp; // Set it
    }

  // Copy over
  NUMERICFLD fld;
  for (_Count_t i=0; i< Total; i++)
    {
      fld.Read(Fp);
      fld.Write(oFp);
    }
  fclose(Fp);
  fclose(oFp);
  if (::remove(Fn) == -1)
    message_log (LOG_ERRNO, "Can't remove '%s'", Fn.c_str());
  if (RenameFile(TmpName, Fn) == -1)
    message_log (LOG_ERRNO, "Can't rename '%s' to '%s'", TmpName.c_str(), Fn.c_str());

  // Now open for append
  if ((Fp = fopen(Fn, "a+b")) == NULL)
    message_log (LOG_ERRNO, "Could not open '%s' for nlist append", Fn.c_str());
  else
    message_log (LOG_DEBUG, "Opening '%s' for nlist append", Fn.c_str());
  return Fp;
}



NUMERICLIST::~NUMERICLIST()
{
  if (table)
    delete [] table;
}

#if 1
// This is all the new search code

// Checks the test value (key) against the array value (val) to see
// on whether key is greater than, less than or equal to val.
Status NUMERICLIST::n_compare(NUMBER val, NUMBER key)
{
  if (key<val) // key is below the comparison value
    return MATCH_LOWER;

  else if (key>val) // key is above the comparison value
    return MATCH_UPPER;

  else // key must equal the comparison value
    return MATCH_MID;
}


// Matches by walking up the array from the bottom until it finds
// a value that is strictly larger than the Key
Status NUMERICLIST::LinearMatchLE(NUMBER Key,int bottom,int top,int* Index)
{
  int i=bottom;
  if (Key < table[bottom].GetNumericValue()) // Nothing in table matches
    return MATCH_FAILED;
  else if (Key >= table[top].GetNumericValue()) { // Everything in table matches
    *Index=top;
    return MATCH_GOOD;
  }

  while ((table[i].GetNumericValue() <= Key) && (i<=top)) {
    *Index=i;
    i++;
  }
  return MATCH_GOOD;
}


/* Matches by walking down the array from the top until it finds
 * a value that is strictly smaller than the Key
 */
Status NUMERICLIST::LinearMatchDownLT(NUMBER Key,int bottom,int top,int* Index)
{
  int i=top;
  Status ret;
  ret = MATCH_FAILED;

  if (Key<=table[bottom].GetNumericValue()) // Nothing in table matches
    return MATCH_FAILED;
  else if (Key>table[top].GetNumericValue()) { // Everything in table matches
    *Index=top;
    return MATCH_GOOD;
  }

  while ((Key<=table[i].GetNumericValue()) && (i>=bottom)) {
    i--;
  }

  if (i>=0) {
    *Index=i;
    ret = MATCH_GOOD;
  }
  return ret;
}


/* Matches by walking down the array from the top until it finds
 * a value that is strictly smaller than the Key
 */
Status NUMERICLIST::LinearMatchGE(NUMBER Key,int bottom,int top,int* Index)
{
  int i=top;
  if (Key<table[bottom].GetNumericValue()) { // Nothing in table matches
    return MATCH_FAILED;
  } else if (Key>table[top].GetNumericValue()) { // Everything in table matches
    *Index=top;
    return MATCH_GOOD;
  }

  while ((table[i].GetNumericValue()>=Key) && (i>=bottom)) {
    *Index=i;
    i--;
  }
  return MATCH_GOOD;
}

Status NUMERICLIST::LinearMatchUpGT(NUMBER Key,int bottom,int top,int* Index)
{
  int i=bottom;
  Status ret;
  ret = MATCH_FAILED;

#if DEBUG
  cerr << "LinearMatchUpGT" << endl;
#endif

  if (Key>=table[top].GetNumericValue()) // Nothing in table matches
    return MATCH_FAILED;
  else if (Key<table[bottom].GetNumericValue()) { // Everything in table matches
    *Index=bottom;
    return MATCH_GOOD;
  }

  while ((Key >= table[i].GetNumericValue()) && (i<=top)) {
    i++;
  }
  if (i>bottom) {
    *Index=i;
    ret = MATCH_GOOD;
  }

  return ret;
}


/* 
 * Does a binary search in table, looking for the largest
 * array index i for which table[i] < Key, and for which
 * table[i+1]>=Key
 */
Status NUMERICLIST::BestMatchLE(NUMBER Key,int first, int last, int* Index)
{
  Status found;
  int mid;

  if (table[first].GetNumericValue() > Key)
    return MATCH_FAILED;
  if (table[last].GetNumericValue() <= Key) {
    *Index=last;
    return MATCH_GOOD;
  }

  found = MATCH_FAILED;
  mid = (first + last) / 2;
  while (found != MATCH_GOOD && last-first>LINEAR_MAX) {
    found=n_compare(table[mid].GetNumericValue(),Key);
    if (found == MATCH_MID) {
      // bail and start the linear search
      found = MATCH_GOOD;
    } else {
      if (found == MATCH_LOWER) {
	last = mid;
      } else {
	first = mid;
      }
      mid = (first + last) / 2;
    }
  }

  // To match strict inequality, when we match the midpoint or when
  // we get indexes close enough, we can just do the linear search
  found = LinearMatchLE(Key,first,last,Index);
  return found;
}


/* 
 * Does a binary search in table, looking for the smallest
 * array index i for which table[i]>Key and for which
 * table[i-1]<=Key
 */
Status NUMERICLIST::BestMatchGE(NUMBER Key,int first, int last, int* Index)
{
  Status found;
  int mid;

  if (table[last].GetNumericValue()<Key) // Nothing matches
    return MATCH_FAILED;
  else if (table[first].GetNumericValue()>=Key) { // Everything matches
    *Index=first;
    return MATCH_GOOD;
  }

  found = MATCH_FAILED;

  mid = (first + last) / 2;
  while (found != MATCH_GOOD && last-first>LINEAR_MAX) {
    found=n_compare(table[mid].GetNumericValue(),Key);
    if (found == MATCH_MID) {
      found = MATCH_GOOD;
    } else {
      if (found == MATCH_LOWER) {
	last = mid;
      } else {
	first = mid;
      }
      mid = (first + last) / 2;
    }
  }

  // To match strict inequality, when we match the midpoint or when
  // we get indexes close enough, we can just do the linear search
  found = LinearMatchGE(Key,first,last,Index);
  return found;
}


// Finds the index of table which best matches Key against the
// NumericValue for the given Relation.  Best match is defined as
// the index in the table which is closest to the actual value.
// 
// This is a little complicated because we do not always have
// the key value in the table.  We just try to get as close as we
// can.  Because the table may have repeated values, as well, we
// have to find a close match, then do a linear walk of the table
// in the appropriate direction to account for the repeats.
//
// Also, for LT and GT, we have to make sure that we find the first
// value which strictly matches.
//
// Obviously, we cannot handle NE, which would require the return of
// two ranges of indexes.

Status NUMERICLIST::Match(SortType Relation, NUMBER Key, int* Index)
{
  Status  ret = MATCH_FAILED;
  int Len;

  Len=Count;

  *Index=-1; // Just in case, at least it's defined
  
  switch (Relation) {
  case LE: // find biggest index for which array value is less than Key
    ret = BestMatchLE(Key,0,Len-1,Index);
    break;

  case GE: // find smallest index for which array value is greater than Key
    ret = BestMatchGE(Key,0,Len-1,Index);
    break;

  case LT:
    ret = BestMatchLE(Key,0,Len-1,Index);

    // When we get here, we know table[Index] is no bigger than Key.
    // It might be equal, though.  If it is equal, we have to walk
    // down table from table[Index] to find the first value which is
    // actually less than Key.
    if ((ret == MATCH_GOOD) 
	&& (*Index >= 0) 
	&& (table[*Index].GetNumericValue()==Key)) {
	// We are not at the bottom of the list
	ret = LinearMatchDownLT(Key,0,*Index,Index);
    }
    break;

  case GT:
    ret = BestMatchGE(Key,0,Len-1,Index);

    // When we get here, we know table[Index] is not smaller than Key.
    // It might be equal, though.  If it is equal, we have to walk
    // down table from table[Index] to find the first value which is
    // actually smaller than Key.
    if ((ret == MATCH_GOOD) 
	&& (*Index >= 0) 
	&& (table[*Index].GetNumericValue()==Key)) {
      // We are not at the bottom of the list
      ret = LinearMatchUpGT(Key,*Index,Len-1,Index);
    }
    break;

  case EQ:
  case NE:
    break;
  }
  return ret;
}

#endif

