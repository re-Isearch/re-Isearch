/*@@@
File:		intlist.cxx
Version:	1.00
$Revision: 1.2 $
Description:	Class INTERVALLIST
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
                Derived from class NLIST by J. Fullton
@@@*/

#include <stdlib.h>
#include <time.h>

#include "common.hxx"
#include "intlist.hxx"
#include "magic.hxx"

typedef UINT4  _Count_t;

#define SIZEOF_HEADER  sizeof(_Count_t)+1 /* size of magic */


// Prototypes
static INT SortStartCmp(const void* x, const void* y);
static INT SortEndCmp(const void* x, const void* y);
static INT SortGPCmp(const void* x, const void* y);

#define DEBUG 0

static const size_t BASIC_CHUNK=50;

void INTERVALLIST::Clear()
{
  Count      = 0;
  FileName.Empty();   
  Pointer    = 0;
  StartIndex = EndIndex = -1;
}

void INTERVALLIST::Empty()
{
  if(table)
    delete [] table;
  MaxEntries = BASIC_CHUNK*Ncoords;
  table      = new INTERVALFLD[ MaxEntries ];
  Count      = 0;
  FileName.Empty();
  Pointer    = 0;
  StartIndex = EndIndex = -1;
}

INTERVALLIST::INTERVALLIST()
{
  Ncoords    = 3;
  MaxEntries = BASIC_CHUNK*Ncoords;
  table      = new INTERVALFLD[ MaxEntries ];
  Count      = 0;
//  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  Truncate   = GDT_FALSE;
}


INTERVALLIST::INTERVALLIST(INT n)
{
  Ncoords    = n;
  MaxEntries = BASIC_CHUNK*Ncoords;
  table      = new INTERVALFLD[ MaxEntries ];
  Count      = 0;
  FileName   = "";
  Pointer    = 0;
  StartIndex = EndIndex = -1;
  Truncate   = GDT_FALSE;
}


INT4 INTERVALLIST::LoadTable(INT4 Start, INT4 End)
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // This is only called when the table has been written out during the
  // index creation phase, before the file has been sorted (with both
  // components dumped)
  //
  // Return the number of items read, just in case anyone's interested
{
  FILE  *fp;
  INT4   nRecs=0;

  if (!(FileName.GetLength())) {
    logf (LOG_PANIC, "IntervalList FileName not set");
    return 0;
  }

  // Count = 0; // This was missing???

  fp = fopen(FileName,"rb");
  if (fp) {

     if (End < 0)
	End = GetFileSize(fp)/sizeof(INTERVALFLD);
     if (Start < 0)
	Start = 0;

    if (fseek(fp, (off_t)Start*sizeof(INTERVALFLD), SEEK_SET) == -1)
      {
	logf (LOG_ERRNO, "INTERVALLIST could not seek on '%s' (Start=%ld)",
		FileName.c_str(), (long)Start);
	return 0;
      }

    Expand(Count + End - Start + 1);

    for (INT4 i=Start, x=0; i<=End; i++) {
      Read(&table[Count], fp);
      nRecs++;
      if(++Count == MaxEntries)
	Expand();
    }
    fclose(fp);
  }
  return nRecs;
}


INT4 INTERVALLIST::LoadTable(INT4 Start, INT4 End, IntBlock Offset)
{
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // Return the actual number of items read in this load

  FILE  *fp;
  GPTYPE high;
  INT4   x, i;
  DOUBLE fStart, fEnd;
  size_t nRecs=0;
  off_t   MoveTo;
  
  if (!(FileName.GetLength())) {
    logf (LOG_PANIC, "Intervallist FileName not set");
    return 0;
  }
  
  fp = fopen(FileName,"rb");
  if (fp) {
    _Count_t  nCount;
    // Bump past Count, then offset to the right starting point
    // Note - there are two tables in the file - one sorted by starting
    // value, one sorted by ending value.  There are Count entries in each
    // version.  
    x = fread((char*)&nCount, 1, sizeof(UINT4), fp); // explicit cast

    if ((End >= nCount) || (End < 0))
      End = nCount-1;
    if (Start < 0)
      Start = 0;
  
    if (x != 0) {
      if (Offset == START_BLOCK)
	MoveTo = SIZEOF_HEADER + (Start * sizeof(INTERVALFLD));
      else if (Offset == END_BLOCK)
	MoveTo = SIZEOF_HEADER + ((nCount+Start) * sizeof(INTERVALFLD));
     else
	MoveTo = SIZEOF_HEADER + ((2*nCount)+Start) * sizeof(INTERVALFLD);

      if (fseek(fp, MoveTo, SEEK_SET) == -1)
	{
	  logf (LOG_ERROR, "INTERVALLIST could not seek to '%s'(%ld))",
                FileName.c_str(), (long)MoveTo);
	  return 0;
	}

      nRecs = 0;

      Expand(Count + End - Start + 1);

      for (i=Start; i<=End; i++) {
	Read(&table[Count], fp);
	nRecs++;
	if(++Count == MaxEntries)
	  Expand();
      }
#if DEBUG
      cerr << "Done (got " << nRecs << " records" << endl;
#endif
    }
    fclose(fp);
  } else
    logf (LOG_ERRNO, "Could not open '%s'", FileName.c_str());
  return nRecs;
}


void INTERVALLIST::WriteTable()
{
  FILE  *fp;
  
  if (!(FileName.GetLength())) {
    logf (LOG_PANIC, "INTervalList FileName not set");
    return;
  }

  if ((fp = fopen(FileName,"wb")) != NULL)
    {
      for (INT4 x=0; x<Count; x++)
        Write (table[x], fp);
      fclose(fp);
    }
}


void INTERVALLIST::WriteTable(INT Offset)
{
  FILE  *fp;
  
  if (!(FileName.GetLength())) {
    logf (LOG_PANIC, "INTervalList FileName not set (offset)");
    return;
  }

  // Offset = 0 is start of file
  if ((fp = fopen (FileName, (Offset == 0) ? "wb" : "a+b")) != NULL) {
    // First, write out the header
    if (Offset == 0) {
      putObjID(objINTLIST, fp);
      Write( (_Count_t)Count, fp);
    }

    // Now, go to the specified offset and write the table
    size_t nBytes = sizeof(INTERVALFLD);
    off_t MoveTo = SIZEOF_HEADER + ((off_t)Offset*nBytes);

    //    cout << "Offsetting " << MoveTo << " bytes into the file." << endl;

    if (fseek(fp, MoveTo, SEEK_SET) == 0) {
      for (off_t x=0; x<Count; x++)
	Write(table[x], fp);
    }
    fclose(fp);
  }
}


void INTERVALLIST::SortByStart()
{
  QSORT((void *)table,Count, sizeof(INTERVALFLD),SortStartCmp);
}


void INTERVALLIST::SortByEnd()
{
  QSORT((void *)table,Count, sizeof(INTERVALFLD),SortEndCmp);
}


void INTERVALLIST::SortByGP()
{
  QSORT((void *)table,Count, sizeof(INTERVALFLD),SortGPCmp);
}


static INT SortStartCmp(const void* x, const void* y) 
{
  DOUBLE a=((*((PINTERVALFLD)x)).GetStartValue()) - ((*((PINTERVALFLD)y)).GetStartValue()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT SortEndCmp(const void* x, const void* y) 
{
  DOUBLE a=((*((PINTERVALFLD)x)).GetEndValue()) - ((*((PINTERVALFLD)y)).GetEndValue()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


static INT SortGPCmp(const void* x, const void* y) 
{
  long long a= (long long)((*((PINTERVALFLD)x)).GetGlobalStart()) -
    (long long)((*((PINTERVALFLD)y)).GetGlobalStart()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


SearchState INTERVALLIST::IntervalMatcher(DOUBLE Key, DOUBLE LowerBound, DOUBLE Mid, 
			      DOUBLE UpperBound, INT4 Relation, IntType Type)
  // Return TOO_LOW if the key value is less than the lower bound
  // Return TOO_HIGH if the key value is greater than the upper bound
  // Return MATCH if the key value is between the bounds
{
  if (Truncate)
    {
      return IntervalMatcher((DOUBLE)((long)Key), (DOUBLE)((long)LowerBound),
	(DOUBLE)((long)Mid), (DOUBLE)((long)UpperBound), Relation, Type) ;
    }
	
  switch (Relation)
    {
    case ZRelGE:		        // greater than or equals
      if ((Mid>=Key) && (Type==AT_START || LowerBound<Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>=Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelGT:		        // greater than
      if ((Mid>Key) && (Type==AT_START || LowerBound<=Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelLE:		        // less than or equals
      //      if ((Mid<=Key) && (Type==AT_END || UpperBound>Key))
      if (UpperBound<Key)
	return(TOO_HIGH);
      else if ((Type==AT_END) && (Mid<=Key))
	return(MATCH);
      else if ((Mid<=Key) && (UpperBound>Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<=Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);

    case ZRelLT:		        // less than
      if ((Mid<Key) && (Type==AT_END || UpperBound>=Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);
    }

  logf (LOG_PANIC, "INTERVALLIST: Hideous Matching Error");
  return(NO_MATCH);
}


SearchState INTERVALLIST::IntervalMatcher(GPTYPE Key,  GPTYPE LowerBound, GPTYPE Mid, 
	GPTYPE UpperBound, INT4 Relation, IntType Type)
  // Return TOO_LOW if the key value is less than the lower bound
  // Return TOO_HIGH if the key value is greater than the upper bound
  // Return MATCH if the key value is between the bounds
{

//cerr << "Key = " << Key << "  LB=" << LowerBound << " Mid=" << Mid << " UB=" << UpperBound << endl;
	
  switch (Relation)
    {
    case ZRelGE:		        // greater than or equals
      if ((Mid>=Key) && (Type==AT_START || LowerBound<Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>=Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelGT:		        // greater than
      if ((Mid>Key) && (Type==AT_START || LowerBound<=Key))
	return(MATCH);		        // exact place - lower boundary
      else if (LowerBound>Key)
	return(TOO_LOW);		// key too low
      else
	return(TOO_HIGH);		// key too high

    case ZRelLE:		        // less than or equals
      //      if ((Mid<=Key) && (Type==AT_END || UpperBound>Key))
      if (UpperBound<Key)
	return(TOO_HIGH);
      else if ((Type==AT_END) && (Mid<=Key))
	return(MATCH);
      else if ((Mid<=Key) && (UpperBound>Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<=Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);

    case ZRelLT:		        // less than
      if ((Mid<Key) && (Type==AT_END || UpperBound>=Key))
	return(MATCH);		        // exact place - upper boundary
      else if (UpperBound<Key)
	return(TOO_HIGH);
      else
	return(TOO_LOW);
    }

  logf (LOG_PANIC, "Hideous Matching Error");
  return(NO_MATCH);
}


// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState 
INTERVALLIST::Find(STRING Fn, DOUBLE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SetFileName(Fn);

  return DiskFind(Fn, Key, Relation, FindBlock, Index);
}


SearchState 
INTERVALLIST::Find(STRING Fn, GPTYPE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  SetFileName(Fn);

  return DiskFind(FileName, Key, Relation, FindBlock, Index);
}


SearchState INTERVALLIST::Find(DOUBLE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  return DiskFind(FileName, Key, Relation, FindBlock, Index);
}


SearchState INTERVALLIST::Find(GPTYPE Key, INT4 Relation, 
		   IntBlock FindBlock, INT4 *Index)
{
  return DiskFind(FileName, Key, Relation, FindBlock, Index);
}


SearchState INTERVALLIST::MemFind(DOUBLE Key, INT4 Relation, 
		      IntBlock FindBlock, INT4 *Index)
{
  return NO_MATCH;
}


SearchState INTERVALLIST::MemFind(GPTYPE Key, INT4 Relation, 
		      IntBlock FindBlock, INT4 *Index)
{
  return NO_MATCH;
}


SearchState INTERVALLIST::DiskFind(STRING Fn, DOUBLE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index)
{
  PFILE  Fp = fopen(Fn, "rb");
  
  SearchState State;
  IntType     Type=INSIDE;

  INTERVALFLD  lowerBound, upperBound, middelValue;

  if (!Fp) {
    logf (LOG_ERRNO, "Could not open %s (Interval list index)", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {
    _Count_t  Total;
    INT Low = 0;
    INT High, X, OX;

    if (getObjID(Fp) != objINTLIST)
      {
        logf (LOG_PANIC, "%s not a interval index??", Fn.c_str());
        *Index = -1;
        return NO_MATCH;
      }
    Read(&Total, Fp);
    High = Total - 1;
    X = High / 2;

    switch(FindBlock) {

    case START_BLOCK:
      // Search the index sorted by starting interval values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  fseek(Fp, SIZEOF_HEADER + (X-1) * sizeof(INTERVALFLD), SEEK_SET);
	  Type=INSIDE; // We're inside the interval
	} else if (X <= 0) {
	  fseek(Fp, SIZEOF_HEADER + X * sizeof(INTERVALFLD), SEEK_SET);
	  Type=AT_START; // We're at the lower boundary
	} else if (X >= High) {
	  fseek(Fp, SIZEOF_HEADER + (X-1) * sizeof(INTERVALFLD), SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START)
	  Read(&lowerBound,  Fp);

	Read(&middelValue, Fp);

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  lowerBound = middelValue;

	if (Type != AT_END)
	  Read(&upperBound, Fp);

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  upperBound = middelValue;

	State = IntervalMatcher(Key,
		lowerBound.GetStartValue(),
		middelValue.GetStartValue(),
		upperBound.GetStartValue(), Relation, Type);

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
      break;
      //    } else {
    case END_BLOCK:
      // Search the index sorted by ending interval values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  fseek(Fp, SIZEOF_HEADER + (Total+X-1) * sizeof(INTERVALFLD), SEEK_SET);
	  Type=INSIDE; // We're inside the interval

	} else if (X <= 0) {
	  fseek(Fp, SIZEOF_HEADER + (Total+X) * sizeof(INTERVALFLD), SEEK_SET);
	  Type=AT_START; // We're at the lower boundary

	} else if (X >= High) {
	  fseek(Fp, SIZEOF_HEADER + (Total+X-1) * sizeof(INTERVALFLD), SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START)
	  Read(&lowerBound, Fp);

	Read(&middelValue, Fp);

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  lowerBound = middelValue;

	if (Type != AT_END)
	  Read(&upperBound, Fp);

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  upperBound = middelValue;
      
#if DEBUG
	cerr << "Comparing key " << Key << " with " << LowerBound 
	     << ", " << NumericValue << " and " << UpperBound << endl;
#endif

	State = IntervalMatcher(Key,
		lowerBound.GetEndValue(),
		middelValue.GetEndValue(),
		upperBound.GetEndValue(), Relation, Type);
#if DEBUG
	cerr << State << endl;
#endif

	if (State == MATCH) {
	  // We found a match
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
      break;

    case PTR_BLOCK:
      // Search the index sorted by global pointer values
      break;
    }
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


SearchState INTERVALLIST::DiskFind(STRING Fn, GPTYPE Key, INT4 Relation, 
		       IntBlock FindBlock, INT4 *Index)
{
  PFILE       Fp = fopen(Fn, "rb");
  INT         ElementSize;
  SearchState State;
  IntType     Type=INSIDE;
  INT4        Hold;         // This is the interval boundary - we don't use it
  off_t       Offset;       // Offset needed to read the element


  INTERVALFLD lowerBound, upperBound, middelValue;


  // These count entries, not offsets
  _Count_t  Total;
  INT Low = 0;
  INT High, X, OX;
  
  if (!Fp) {
    logf (LOG_ERRNO, "Could not open %s (Int Index)", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {

    if (getObjID(Fp) != objINTLIST)
      {
        logf (LOG_PANIC, "%s not a interval index??", Fn.c_str());
        *Index = -1;
        return NO_MATCH;
      }
    Read(&Total, Fp);

    ElementSize = sizeof(INTERVALFLD);
    High = Total - 1;
    X = High / 2;

    //    if (ByStart) {
    switch(FindBlock) {

    case START_BLOCK:
      // Search the index sorted by starting interval values
      break;

    case END_BLOCK:
      // Search the index sorted by ending interval values
      break;

    case PTR_BLOCK:
      // Search the index sorted by global pointer values
      do {
	OX = X;
	if ((X > 0) && (X < High)) {
	  Offset = SIZEOF_HEADER + ((2*Total)+(X-1)) * ElementSize;
	  fseek(Fp, Offset, SEEK_SET);
	  Type=INSIDE; // We're inside the interval

	} else if (X <= 0) {
	  Offset = SIZEOF_HEADER + ((2*Total)+X) * ElementSize;
	  fseek(Fp, Offset, SEEK_SET);
	  Type=AT_START; // We're at the lower boundary

	} else if (X >= High) {
	  Offset = SIZEOF_HEADER + ((2*Total)+(X-1)) * ElementSize;
	  fseek(Fp, Offset, SEEK_SET);
	  Type=AT_END; // We're at the upper boundary
	}

	if (Type != AT_START)
	  Read(&lowerBound,  Fp);

	Read(&middelValue,  Fp);

	// If we're at the start, we need to read the first value into
	// NumericValue, but we don't want to leave LowerBound 
	// uninitialized.  This will also handle the case when we only
	// have two values in the index.
	if (Type == AT_START)
	  lowerBound = middelValue;

	if (Type != AT_END)
	  Read(&upperBound, Fp);

	// Similarly, if we're at the end and can't read in a new value
	// for UpperBound, we don't want it uninitialized, either.
	if (Type == AT_END)
	  upperBound = middelValue;

	State = IntervalMatcher(Key,
		lowerBound.GetGlobalStart(),
		middelValue.GetGlobalStart(),
		upperBound.GetGlobalStart(), 
		Relation, Type);
#if DEBUG
	cerr << State << endl;
#endif

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
      break;
    }
  }
  fclose(Fp);
  *Index = -1;
  return NO_MATCH;
}


void INTERVALLIST::Dump(ostream& os) const
{
  INTERVALLIST::Dump((INT4)0, (INT4)Count, os);
}


void INTERVALLIST::Dump(INT4 start, INT4 end, ostream& os) const
{
  if (end > Count)
    end = Count;

  for(INT4 x= (start < 0 ? 0 : start); x<end; x++)
    table[x].Dump(os);
}


void INTERVALLIST::Expand(size_t Entries)
{
  if (Entries < MaxEntries)
    return;
  Resize(Entries +  (BASIC_CHUNK*Ncoords));
}




void INTERVALLIST::Resize(INT4 Entries)
{
  if (Entries == MaxEntries)
     return; // Don't wast time

  INT4 CopyCount;

  if(Entries>Count)
    {
      CopyCount=Count;
    }
  else
    {
      // Trim
      CopyCount=Entries;
      Count=Entries;
    }

  PINTERVALFLD temp=new INTERVALFLD[Entries];
  for(INT4 x=0; x<CopyCount; x++)
    temp[x]=table[x];

  if(table)
    delete [] table;
  table=temp;
  MaxEntries=Entries;
}


INTERVALLIST::~INTERVALLIST()
{
  if(table)
    delete [] table;
}

void INTERVALLIST::WriteIndex(const STRING& Fn)
{
  SetFileName(Fn);
  LoadTable(0,-1);
  if (Count > 0) { // was > 1
    ::remove(Fn);

    SortByStart();
    WriteTable(0);
    SortByEnd();
    WriteTable(Count);
    SortByGP();
    WriteTable(2*Count);
 }
}


// Reduce the two tables down to one to allow for a simple
// write append
//
FILE *INTERVALLIST::OpenForAppend(const STRING& Fn)
{
  // New file?
  if (GetFileSize(Fn) == 0)
    return fopen(Fn, "wb");

  FILE *Fp = fopen(Fn, "rb");

  if (Fp == NULL)
   {
      logf (LOG_ERRNO, "INTERVALLIST:: Can't open '%s' for reading.", Fn.c_str());
      return NULL;
   }
  if (getObjID(Fp)!= objINTLIST)
    {
      fclose(Fp);
      return fopen(Fn, "a+b");
    }

  _Count_t  Total; // This MUST match the type of Count!!!
  Read(&Total, Fp);
  if (Total == 0) 
    {
      // Nothing in there so
      fclose(Fp);
      return fopen(Fn, "wb"); // Can start from scratch
    }

  STRING TmpName = Fn + "~";

  for (size_t i =0; FileExists(TmpName); i++)
    {
      TmpName.form ("%s.%d", Fn.c_str(), (int)i);
    }
  FILE *oFp = fopen(TmpName, "wb");
  if (oFp == NULL)
    {
      // Fall into scatch (we might not have writing permission
      char     scratch[ L_tmpnam+1];
      char    *TempName = tmpnam( scratch ); 

      logf (LOG_WARN, "Could not create '%s', trying tmp '%s'", TmpName.c_str(),
	TempName);
      if ((oFp = fopen(TempName, "wb")) == NULL)
	{
	  logf (LOG_ERRNO, "Can't create a temporary interval list '%s'", Fn.c_str());
	  fclose(Fp);
	  return NULL;
	}
      TmpName = TempName; // Set it
    }

  // Copy over
  INTERVALFLD fld;
  for (UINT4 i=0; i< Total; i++)
    {
      fld.Read(Fp);
      fld.Write(oFp);
    }
  fclose(Fp);
  fclose(oFp);
  if (remove(Fn) == -1)
    logf (LOG_ERRNO, "Can't remove '%s'", Fn.c_str());
  if (RenameFile(TmpName, Fn) == -1)
    logf (LOG_ERRNO, "Can't rename '%s' to '%s'", TmpName.c_str(), Fn.c_str());

  // Now open for append
  if ((Fp = fopen(Fn, "a+b")) == NULL)
    logf (LOG_ERRNO, "Could not open '%s' for interval append", Fn.c_str());
  else
    logf (LOG_DEBUG, "Opening '%s' for interval append", Fn.c_str());
  return Fp;
}



#ifdef NEVER
main()
{
  NUMERICLIST list;
  STRING n;
  INT4 Start,End;
  DOUBLE val;
  n="test.62";
  printf("DOUBLE is %d bytes\n",sizeof(DOUBLE));
  list.TempLoad();
  End=list.DiskFind(n, (double)88.0,5); // 5 GT
  printf("\n===\n");
  Start=list.DiskFind(n,(double)88.0,1); // 1 LT
  
  if(End-Start<=1)
    printf("No Match!\n");
  list.SetFileName(n.NewCString());
  list.LoadTable(Start,End);
    
}
#endif
