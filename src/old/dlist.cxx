/*@@@
File:		dlist.cxx
Version:	1.00
$Revision: 1.2 $
Description:	Class DATELIST
Author:	 	Edward Zimmermann edz@bsn.com
@@@*/

#include <stdlib.h>
#include <errno.h>

#include "dlist.hxx"
#include "magic.hxx"

#define DEBUG 0


typedef UINT4  _Count_t;
#define SIZEOF_HEADER  sizeof(_Count_t)+1 /* size of magic */


static const size_t BASIC_CHUNK=50;


void DATELIST::Clear()
{
  Count      = 0;
  FileName.Empty();
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}

void DATELIST::Empty()
{
  if(table) delete [] table;
  MaxEntries = 0;
  table      = NULL;

  Count      = 0;
  FileName.Empty();   
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}



DATELIST::DATELIST()
{
  Ncoords    = 2;
  MaxEntries = 0;
  table      = NULL; 
  Count      = 0;
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}


DATELIST::DATELIST(INT n)
{
  Ncoords    = n;
  MaxEntries = 0;
  table      = NULL; 
  Count      = 0;
  Pointer    = 0;
  StartIndex = EndIndex = 0; // was -1;
}


void DATELIST::ResetHitPosition()
{
  Pointer = ( (Relation != 1) ?  StartIndex : 0 );
}


GPTYPE DATELIST::GetNextHitPosition()
{
  GPTYPE Value;

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


static int SortCmp(const void* x, const void* y) 
{
  return Compare( (*((PDATEFLD)x)).GetValue(), (*((PDATEFLD)y)).GetValue() );
}


static int SortCmpGP(const void* x, const void* y) 
{
  const GPTYPE gp1 = (*((PDATEFLD)x)).GetGlobalStart();
  const GPTYPE gp2 = (*((PDATEFLD)y)).GetGlobalStart();

  if (gp1 > gp2)
    return 1;
  if (gp1 < gp2)
    return -1;
  return 0;
}

GDT_BOOLEAN DATELIST::Expand(size_t Entries)
{
  if (Entries < MaxEntries)
    return GDT_TRUE;
  return Resize(Entries +  (BASIC_CHUNK*Ncoords)); 
}


GDT_BOOLEAN DATELIST::Resize(size_t Entries)
{
  if (Entries == 0)
    {
      Clear();
      return GDT_TRUE;
    }

  DATEFLD    *temp;

  try {
    temp =new DATEFLD[Entries];
  } catch (...) {
    logf (LOG_PANIC, "Memory allocation failure. Can't build Date field array!");
    return GDT_FALSE;
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
  return GDT_TRUE;
}


void DATELIST::Sort()
{
  QSORT((void *)table, Count, sizeof(DATEFLD), SortCmp);
}


void DATELIST::SortByGP()
{
  QSORT((void *)table, Count, sizeof(DATEFLD), SortCmpGP);
}


SearchState DATELIST::Matcher(const SRCH_DATE& Key,
	const SRCH_DATE& A, const SRCH_DATE& B, const SRCH_DATE& C, 
	INT4 Relation, INT4 Type)
{
//cerr << "Matcher: Key, A, B, C, Type // Rel=" << (int)Relation <<endl;
  switch (Relation) {
  case ZRelGE:			// greater than or equals
    if ((B>=Key) && (Type==-1 || A<Key))
      {
        return(MATCH);		// exact place - lower boundary
      }
    else if (A>=Key)
      {
	return(TOO_LOW);		// key too low
      }
    else
      {
        return(TOO_HIGH);		// key too high
      }

  case ZRelGT:			// greater than
    if ((B>Key) && (Type==-1 || A<=Key))
      {
        return(MATCH);		// exact place - lower boundary
      }
    else if (A>Key)
      {
	return(TOO_LOW);		// key too low
      }
    else
      {
        return(TOO_HIGH);		// key too high
      }

  case ZRelLE:			// less than or equals
    if ((B<=Key) && (Type==0 || C>Key))
      return(MATCH);		// exact place - upper boundary
    else if (C<=Key)
      return(TOO_HIGH);
    else
      return(TOO_LOW);

  case ZRelLT:			// less than
    if ((B<Key) && (Type==0 || C>=Key))
      {
        return(MATCH);		// exact place - upper boundary
      }
    else if (C<Key)
      {
        return(TOO_HIGH);
      }
    else
      {
	return(TOO_LOW);
      }
    default: break;
  }

  logf (LOG_PANIC, "Hideous Matching Error");
  return(NO_MATCH);
}


// Gp Search
SearchState DATELIST::Matcher(GPTYPE Key, GPTYPE A, GPTYPE B, GPTYPE C, 
		     INT4 Relation, INT4 Type)
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
  }

  logf (LOG_PANIC, "Hideous Matching Error");
  return(NO_MATCH);
}


// Search for the GP INT4 values
// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState DATELIST::Find(const STRING& Fn, GPTYPE Key, INT4 Relation, INT4 *Index)
{
  SetFileName(Fn);

  return DiskFind(Fn, Key, Relation, Index);
}



// Ultimately, this routine will try to load the table in one chunk of
// memory.  If it succeeds, it'll call MemFind to do the search in memory.
// Otherwise, it'll call DiskFind to do the search on disk.
SearchState DATELIST::Find(const STRING& Fn, const SRCH_DATE& Key, INT4 Relation, INT4 *Index)
{
  SetFileName(Fn);

  return DiskFind(Fn, Key, Relation, Index);
}


SearchState DATELIST::Find(const SRCH_DATE& Key, INT4 Relation, INT4 *Index)
{
  return DiskFind(FileName, Key, Relation, Index);
}

SearchState DATELIST::Find(GPTYPE Key, INT4 Relation, INT4 *Index)
{
  return DiskFind(FileName, Key, Relation, Index);
}


SearchState DATELIST::MemFind(const SRCH_DATE& Key, INT4 Relation, INT4 *Index)
{
  return NO_MATCH;
}


SearchState DATELIST::DiskFind(STRING Fn, const SRCH_DATE& Key, INT4 Relation, INT4 *Index)
{
  //  INT4 B;
  PFILE  Fp = fopen(Fn, "rb");

  if (!Fp) {
    logf (LOG_ERRNO, "Can't open date index '%s'", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {
    INT4        Total;
    INT         Low, High, X, OX;
    SearchState State;
    INT         Type=0;

    if (getObjID(Fp) != objDLIST)
      {
	fclose(Fp);
	if (feof(Fp))
	  logf (LOG_INFO, "Empty index: '%s'", Fn.c_str());
	else
	  logf (LOG_PANIC, "%s not a date index??", Fn.c_str());
        *Index = -1;
        return NO_MATCH;
      }
    Read(&Total, Fp);

    Low = 0;
    High = Total - 1;
    X = High / 2;

    DATEFLD lowerBound;
    DATEFLD upperBound;
    DATEFLD value;

    do {
      OX = X;
//cerr << "X = " << X << endl;

      if ((X > 0) && (X < High)) {
	fseek(Fp, SIZEOF_HEADER + (X-1) * sizeof(DATEFLD), SEEK_SET);
	Type=INSIDE;
      } else if (X <= 0) {
//cerr << "Reading from Start" << endl;
	fseek(Fp, SIZEOF_HEADER + X * sizeof(DATEFLD), SEEK_SET);
	Type=AT_START;
      } else if (X >= High) {
	fseek(Fp, SIZEOF_HEADER + (X-1) * sizeof(DATEFLD), SEEK_SET);
	Type=AT_END;
      }
	
      if (Type != AT_START)
        {
	  Read(&lowerBound, Fp);
//cerr << "Got lowerBound = " << lowerBound.GetValue() << endl;
        }

      Read(&value, Fp); // Gps, NumericValue
// cerr << "Got value = " << value.GetValue() << endl;
	
      // If we're at the start, we need to read the first value into
      // NumericValue, but we don't want to leave LowerBound 
      // uninitialized.  This will also handle the case when we only
      // have two values in the index.
      if (Type == AT_START)
	lowerBound = value;

//cerr << "lowerBound = " << lowerBound.GetValue() << endl;
//cerr << "value      = " << value.GetValue() << endl;
	
      if(Type != AT_END)
	{
	  Read(&upperBound, Fp); // Dummy, UpperBound
//cerr << "upperBound = " << upperBound.GetValue() << endl;
	}
	
      // Similarly, if we're at the end and can't read in a new value
      // for UpperBound, we don't want it uninitialized, either.
      if (Type == AT_END)
	upperBound = value;

//cerr << "Relation = " << (int)Relation << endl;

      State = Matcher(Key, lowerBound.GetValue(), value.GetValue(), upperBound.GetValue(), Relation, Type);
	
      if (State == MATCH) {
//cerr << "We Got a match" << endl;
	// We got a match
	fclose(Fp);
	*Index = X;
	return MATCH;
      } else if ((State == TOO_HIGH) && (Type == AT_END)) {
	// We didn't get a match, but we ran off the upper end, so
	// the key is bigger than anything indexed
//cerr << "Ran off the upper end" << endl;
	fclose(Fp);
	*Index = -1;
	return State;
      } else if ((State == TOO_LOW) && (Type == AT_START)) {
	// We didn't get a match, but we ran off the lower end, so 
	// the key is smaller than anything indexed
//cerr << "Ran off the lower end" << endl;
	fclose(Fp);
	*Index = -1;
	return State;
      } else if (Low >= High) {
	// If Low is >= High, there aren't any more values to check
	// so we're done whether we got a match or not, and if we got
	// here, there wasn't a match.  This probably won't happen - 
	// at least, we expect that these conditions will be caught
	// by one of the preceeding, but it pays to be safe.
//cerr << "Low >= High ??" << endl;
	fclose(Fp);
	*Index = -1;
	return NO_MATCH;
      }

      if (State == TOO_LOW)
	High = X;
      else
	Low = X + 1;
	
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



// This one searches for Gps!!!
SearchState DATELIST::DiskFind(STRING Fn, GPTYPE Key, INT4 Relation, INT4 *Index)
{
  PFILE  Fp = fopen(Fn, "rb");

  if (!Fp) {
    logf (LOG_ERRNO, "Datelist Index open faolure '%s'", Fn.c_str());
    *Index = -1;
    return NO_MATCH;

  } else {

    _Count_t    Total;
    INT         Low, High, X, OX;
    SearchState State;
    INT         Type=0;
    DOUBLE      Hold;         // This is just a dummy - we don't use it
    off_t       Offset;       // Offset needed to read the element
    size_t      ElementSize = sizeof(DATEFLD); 


    if (getObjID(Fp) != objDLIST)
      {
	if (feof(Fp))
	  logf (LOG_INFO, "Empty index: '%s'", Fn.c_str());
	else
	  logf (LOG_PANIC, "%s not a date index??", Fn.c_str());
        *Index = -1;
        return NO_MATCH;
      }

    Read(&Total,Fp);

    Low = 0;
    High = Total - 1;
    X = High / 2;

    DATEFLD Value, Lower, Upper;

    do {
      OX = X;

      if ((X > 0) && (X < High)) {
	Offset = SIZEOF_HEADER + (Total+X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=INSIDE;

      } else if (X <= 0) {
	Offset = SIZEOF_HEADER + (Total+X) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
	Type=AT_START;

      } else if (X >= High) {
	Offset = SIZEOF_HEADER + (Total+X-1) * ElementSize;
	fseek(Fp, (long)Offset, SEEK_SET);
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
	Lower.GetGlobalStart(),
	Value.GetGlobalStart(),
	Upper. GetGlobalStart(),
	Relation, Type);
	
      if (State == MATCH) {
	// We got a match

//cerr << "===========" << endl << "MATCH:" << endl;
//cerr << "Key   = " << Key << endl;
//cerr << "Lower = " << Lower.GetGlobalStart() << endl;
//cerr << "Value = " << Value.GetGlobalStart() << endl;
//cerr << "Upper = " << Upper.GetGlobalStart() << endl;



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



void DATELIST::Dump(ostream& os) const
{
  for(_Count_t x=0; x<Count; x++)
    table[x].Dump(os);
}


void DATELIST::Dump(INT4 start, INT4 end, ostream& os) const
{
  if (start < 0)
    start = 0;
  if (end > Count)
    end = Count;

  for(INT x=start; x<end; x++)
   table[x].Dump(os);
}

size_t DATELIST::LoadTable(INT4 Start, SRCH_DATE date)
{
  off_t      MoveTo = SIZEOF_HEADER + (Start*sizeof(DATEFLD));
  size_t     nRecs = 0;
  _Count_t   nCount;
  FILE      *fp;

  if ((fp=fopen(FileName,"rb")) != NULL) {
   DATEFLD datefield;
   SRCH_DATE sdate;

   Read(&nCount, fp);
   fseek(fp, MoveTo, SEEK_SET);
   errno = 0;
   Resize(Count+ nCount-Start +1);
   for (size_t i=Start; i<nCount; i++) {
     if (feof(fp))
       break;
      ::Read(&datefield, fp);
      sdate = datefield.GetValue();
      if (sdate < date)
	continue;

     if (sdate > date)
	{
	  break;
	}
      if(Count == MaxEntries)
	Expand();
      table[Count++] = datefield;
      nRecs++;
   }
   fclose(fp);
 }

  return nRecs;
}


size_t DATELIST::LoadTable(INT4 Start, INT4 End)
{
  FILE   *fp;
  size_t  nRecs=0;

  if ( FileName.IsEmpty() ) {
    logf (LOG_PANIC, "DATELIST::LoadTable: FileName not set");
    return 0;
  }

  const long   Elements = (long)(GetFileSize(FileName)/sizeof(DATEFLD));

  if (Elements == 0)
    {
//      logf (LOG_WARN, "DATELIST: '%s' is empty!", FileName.c_str());
      return nRecs;
    }
  if (Start > Elements)
    {
      logf (LOG_WARN, "DATELIST: Start %d > element count (%ld). Nothing to load.",
	(int)Start, Elements);
      return nRecs;
    }
  
  if ((fp=fopen(FileName,"rb")) != NULL) {

    if (Start == -1)                     Start=0;
    if ((End  == -1) || (End>=Elements) ) End = (INT4)(Elements - 1);

    if (Start > 0)
      if (fseek(fp, (off_t)Start*sizeof(DATEFLD), SEEK_SET) == -1)
	logf (LOG_ERRNO, "DATELIST: Seek error on '%s'", FileName.c_str());

    Resize(Count + End-Start + 1); // Make sure there is some room

    errno = 0;
    for (size_t i=Start;i<=End;i++){
      if (feof(fp))
	{
	  logf (
		(errno ? LOG_ERRNO : LOG_ERROR),
		"Premature date list read-failure in '%s' [%d into (%d,%d) of %ld]",
		FileName.c_str(), (int)i, (int)Start, (int)End, Elements);
	  break;
	}
      ::Read(&table[Count], fp);
//cerr << "Read [" << Count << "] " << table[Count] << endl;
      nRecs++;
      if(++Count==MaxEntries)
	Expand();
    }
    fclose(fp);
  } else
    logf (LOG_ERROR, "Could not open '%s'", FileName.c_str());
  return nRecs;
}


size_t DATELIST::LoadTable(INT4 Start, INT4 End, NumBlock Offset)
{
  // Start is the starting index (i.e., 0 based), and End is the ending
  // index, so this will load End-Start+1 items into table[Start] through
  // table[End]
  //
  // Return the actual number of items read in this load
  size_t nRecs = 0;

  if (FileName.IsEmpty() ) {
    logf (LOG_PANIC, "Numeric List FileName not set");
    return 0;
  }

  if (GetFileSize(FileName) == 0) return 0; // Empty index
  
  FILE *fp = fopen(FileName,"rb");
  if (fp) {
    // Bump past Count, then offset to the right starting point
    // Note - there are two tables in the file - one sorted by starting
    // value, one sorted by ending value.  There are Count entries in each
    // version.  
    _Count_t nCount;
    off_t MoveTo;

    if (getObjID(fp)!= objDLIST)
      {
	fclose(fp);
	logf (LOG_PANIC, "%s not a date list??", FileName.c_str());
        return 0;
      }

    Read(&nCount, fp);

    if ((End >= nCount) || (End < 0))
      End = nCount-1;
    if (Start < 0)
      Start = 0;

//if (Start>0) Start--; //@@@@??
  
    if ((Start < nCount) && !feof(fp)) {
      if (Offset == VAL_BLOCK) {
	MoveTo = SIZEOF_HEADER + ((Start)*sizeof(DATEFLD));
      } else if (Offset == GP_BLOCK) {
	MoveTo = SIZEOF_HEADER + ((nCount+Start)*sizeof(DATEFLD));
      } else {
	MoveTo = 0;
      }

#if DEBUG
      cerr << "DATELIST: Moving " << MoveTo << " bytes into the file and reading "
	   << nCount << " elements starting at table[" << Count << "]"
	   << endl;
#endif

      if (MoveTo != SIZEOF_HEADER)
	if (fseek(fp, MoveTo, SEEK_SET) == -1)
	  logf (LOG_ERRNO, "Can't seek to %ld in '%s'", (long)MoveTo, FileName.c_str());

      Resize(Count + End-Start + 1);
      errno = 0;
      for (size_t i=Start; i<=End; i++) {
	if (feof(fp))
	  {
	    logf (LOG_ERRNO, "Premature date list read-failure in '%s' [%d in (%d,%d)]",
		FileName.c_str(), i, Start, End);
	    break;
	  }
	::Read(&table[Count], fp);
#if DEBUG
	cerr << "table[" << Count << "] = " << table[Count] << endl;
#endif
	nRecs++;
	if(++Count == MaxEntries)
	  Expand();
      }
    }
    fclose(fp);
  } else
    logf (LOG_ERROR, "Could not open '%s'", FileName.c_str());

#if DEBUG
  cerr << "returning " << nRecs << endl;
#endif
  return nRecs;
}


void DATELIST::WriteTable()
{
  FILE  *fp;
  
  if ( FileName.IsEmpty() ) 
    return;

  if ((fp=fopen(FileName,"wb")) != NULL)
    {
      for(_Count_t x=0; x<Count; x++)
	Write(table[x], fp);
      fclose(fp);
    }
}


void DATELIST::WriteTable(INT Offset)
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
	  // Write the header
	  putObjID(objDLIST, fp);
	  Write((_Count_t)Count, fp);
	}

      // Now, go to the specified offset and write the table
      off_t  MoveTo = (off_t)(Offset*sizeof(DATEFLD)) + SIZEOF_HEADER;

#if DEBUG
      cerr << Count << " elements, Offsetting " << MoveTo << " bytes into the file." << endl;
#endif

      if (MoveTo != SIZEOF_HEADER)
	{
	  if (fseek(fp, MoveTo, SEEK_SET) == -1)
	    logf (LOG_ERRNO, "Can't seek to %ld in '%s'", (long)MoveTo, FileName.c_str());
	}

      for (_Count_t x=0; x<Count; x++)
	{
#if DEBUG
	  cerr << "  write: " << table[x] << endl;
#endif
	  Write(table[x], fp);
	}
      fclose(fp);
   }
}


DATELIST::~DATELIST()
{
  if (table)
    delete [] table;
}


void DATELIST::WriteIndex(const STRING& Fn)
{
#if DEBUG
  cerr << "WriteIndex(" << Fn << ")" << endl;
#endif
  SetFileName(Fn);
  LoadTable(0,-1);
#if DEBUG
  cerr << "Count = " << Count << endl;
#endif
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
FILE *DATELIST::OpenForAppend(const STRING& Fn)
{
  // New file?
  if (GetFileSize(Fn) == 0)
    return fopen(Fn, "wb");

  FILE *Fp = fopen(Fn, "rb"); // Read and write

  if (Fp == NULL)
   {
      logf (LOG_ERRNO, "DATELIST:: Can't open '%s' for reading", Fn.c_str());
      return NULL;
   }
  if (getObjID(Fp)!= objDLIST)
    {
#if DEBUG
      cerr << "OpenForAppend = a+b...." << endl;
#endif
      fclose(Fp);
      return fopen(Fn, "a+b"); // Append
    }

  _Count_t Total; // This MUST match the type of Count!!!
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
	  logf (LOG_ERRNO, "Can't create a temporary list '%s'", Fn.c_str());
	  fclose(Fp);
	  return NULL;
	}
      TmpName = TempName; // Set it
    }

  // Copy over
  DATEFLD fld;
  for (_Count_t i=0; i< Total; i++)
    {
      fld.Read(Fp);
#if DEBUG
      cerr << "DATELIST [" << i << "] = " << fld << endl;
#endif
      fld.Write(oFp);
    }
  fclose(Fp);
  fclose(oFp);
  if (::remove(Fn) == -1)
    logf (LOG_ERRNO, "Can't remove '%s'", Fn.c_str());
  if (RenameFile(TmpName, Fn) == -1)
    logf (LOG_ERRNO, "Can't rename '%s' to '%s'", TmpName.c_str(), Fn.c_str());

  // Now open for append
  if ((Fp = fopen(Fn, "a+b")) == NULL)
    logf (LOG_ERRNO, "Could not open '%s' for date list append", Fn.c_str());
  else
    logf (LOG_DEBUG, "Opening '%s' for date list append", Fn.c_str());
  return Fp;
}
