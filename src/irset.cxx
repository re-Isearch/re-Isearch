#pragma ident  "@(#)irset.cxx"
static const char RCS_Id[] = "$Id: irset.cxx,v 1.8 2007/11/18 19:41:28 edz Exp edz $";


/************************************************************************
 History:
 $Log: irset.cxx,v $
 Revision 1.8  2007/11/18 19:41:28  edz
 *** empty log message ***

 Revision 1.7  2007/11/07 09:29:16  edz
 Added Join and JoinO

 Revision 1.6  2007/06/20 08:08:31  edz
 sync

 Revision 1.5  2007/06/18 16:05:25  edz
 Wildcard "*"

 Revision 1.4  2007/05/20 08:41:15  edz
 *** empty log message ***

 Revision 1.3  2007/05/16 12:38:32  edz
 *** empty log message ***

 Revision 1.2  2007/05/16 12:27:50  edz
 Caching HitTotal
 ./

 Revision 1.1  2007/05/15 15:47:23  edz
 Initial revision

// Revision 1.12  1996/05/02  23:14:24  edz
// Changed AddEntry call in IRSET::Or to add to the hit-table. The
// logic is that a record could contain both terms but the record
// hit-tables would resp. only have the coordinates of each term.. So
// by adding the hit coordinates we can get a highlighted record that
// show both terms.
//
************************************************************************/

/*-@@@
File:		irset.cxx
Version:	2.00
Description:	Class IRSET - Internal Search Result Set
Author:		Edward C. Zimmermann from some code originally by Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include "common.hxx"
#include "irset.hxx"
#include "magic.hxx"
#include "lang-codes.hxx"
#include "index.hxx"

#include <float.h>
#ifndef MAXFLOAT
# ifndef FLT_MAX
#  define FLT_MAX 3.402823466E+18
# endif
#define MAXFLOAT FLT_MAX
#endif

#if 1
# define SORT QSORT
#elif 0
# define SORT heapsort 
#else
# define SORT mergesort 
#endif


#ifndef FIELD_WILD_MATCH
#define FIELD_WILD_MATCH(_x, _y)  (_x).FieldMatch(_y)
#endif


enum NormalizationMethods defaultNormalization = CosineNormalization;

#define AWWW 1
#define STORE_PROX 1

#define INCREMENT 4096 
// (Parent->GetTotalRecords ()  > 1000 ? 1000 :  Parent->GetTotalRecords ()  )



static GDT_BOOLEAN _compLTE(const FC& Fc1, const FC& Fc2) { return Fc1 <= Fc2; }
static GDT_BOOLEAN _compGTE(const FC& Fc1, const FC& Fc2) { return Fc1 >= Fc2; } 

///
/// NOTE: Instead of GetMdtIndex() use GetIndex()
///

static int IrsetIndexCompare(const void* x, const void* y)
{
  INDEX_ID s1 = (*((PIRESULT)x)).GetIndex();
  INDEX_ID s2 = (*((PIRESULT)y)).GetIndex();
  return s1.Compare(s2);
}   

static int iIndexCompare(const void *x, const void *y)
{
  return (int)(*((_index_id_t *)x) - (_index_id_t)(((IRESULT *)y)->GetIndex()) );
}

atomicIRSET::atomicIRSET (const PIDBOBJ DbParent, size_t Reserve)
{
  allocs           = 0;
  TotalEntries     = 0;
  MaxEntries       = 0;
  MaxEntriesAdvice = 0;

  Table = NULL;
  Increment = Reserve ? Reserve : INCREMENT;
  ComputedS = Unnormalized;
  HitTotal  = 0;
  IrsetCompar = 0;
  userData    = NULL;

  Parent = DbParent;

  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
  SortRequest = Unsorted;
}

atomicIRSET::atomicIRSET (const OPOBJ& Irset)
{
  allocs           = 0;
  TotalEntries     = 0;
  MaxEntries       = 0;
  Table            = NULL;
  Increment        = INCREMENT;
  MaxEntriesAdvice = 0;

  *this = Irset;
}


/*
atomicIRSET::operator const RSET *() const
{
  return GetRset ();
}
*/

OPOBJ& atomicIRSET::operator =(const OPOBJ& OtherIrset)
{
#if 1
  const OPOBJ  *OtherPtr = &OtherIrset;
  Set ((atomicIRSET *)OtherPtr);
#else
  const atomicIRSET  *OtherPtr = (atomicIRSET *)&OtherIrset;
  Set (OtherPtr);
#endif
  return *this;
}

atomicIRSET& atomicIRSET::operator = (const atomicIRSET& Other)
{
  Set (&Other);
  return *this;
}

void atomicIRSET::Set(const atomicIRSET *OtherPtr)
{
  if (OtherPtr == NULL)
    {
      logf (LOG_PANIC, "atomicIRSET::Set(\"Null pointer\") passed");
      if (Parent) Parent->SetErrorCode(1); // Permanent Error
      return; // Passed Nul Pointer!
    }

  const size_t  OtherTotal = OtherPtr->TotalEntries;

  Parent           = OtherPtr->Parent;
  MaxScore         = OtherPtr->MaxScore;
  MinScore         = OtherPtr->MinScore;
  Sort             = OtherPtr->Sort;
  SortRequest      = OtherPtr->SortRequest;
  ComputedS        = OtherPtr->ComputedS;
  allocs           = OtherPtr->allocs;
  HitTotal         = OtherPtr->HitTotal;
  IrsetCompar      = OtherPtr->IrsetCompar;
  userData         = OtherPtr->userData;
  MaxEntriesAdvice = OtherPtr->MaxEntriesAdvice;

//cerr << "SET: Now add to table //  Count = " << __IB_IRESULT_allocated_count << endl;;


  // Expand (or shrink) if we need to..
  if (OtherTotal > MaxEntries || (MaxEntries - OtherTotal) > (Increment/2) || Table == NULL)
    {
//cerr << "SET: Expand.. // "  << __IB_IRESULT_allocated_count << endl;

      IRESULT *oldTable = Table;
      IRESULT *newTable;
      try {
	newTable = new IRESULT[OtherTotal];
      } catch (...) {
	if (errno == 0) errno = ENOMEM;
        logf (LOG_ERRNO, "IRSET::Set() Could not allocate space for %ld IRESULTs!", (long)OtherTotal);  
	if (Parent) Parent->SetErrorCode(2); // Temp Error
	return;
      }
      Table        = newTable;
      MaxEntries   = OtherTotal;
      TotalEntries = 0;
      if (oldTable) delete[] oldTable;
      allocs = 0;
    }
  // Now add to table..
//cerr << "SET: Now add to table " << OtherTotal << " Count = " << __IB_IRESULT_allocated_count << endl;

  for (TotalEntries = 0; TotalEntries < OtherTotal; TotalEntries++)
    Table[TotalEntries] = OtherPtr->Table[TotalEntries];

//cerr << "-> " << __IB_IRESULT_allocated_count << endl;;

}


atomicIRSET& atomicIRSET::Concat(const atomicIRSET& OtherIrset)
{
  if ((TotalEntries + OtherIrset.TotalEntries) >= MaxEntries)
    {
      Resize(TotalEntries + OtherIrset.TotalEntries + Increment);
    }

  for (size_t i=0; i < OtherIrset.TotalEntries; i++)
    {
//cerr << "Adding " << i << "/" <<
// OtherIrset.Table[i].GetMdt()->c_str() << "  = " << OtherIrset.Table[i].GetMdtIndex() << endl;
      FastAddEntry (OtherIrset.Table[i]);
    }
  return *this;

}

OPOBJ& atomicIRSET::Cat(const OPOBJ& OtherIrset, GDT_BOOLEAN AddHitCounts)
{
  return Cat(OtherIrset, OtherIrset.GetTotalEntries (), AddHitCounts);
}

OPOBJ& atomicIRSET::Cat(const OPOBJ& OtherIrset, size_t Total, GDT_BOOLEAN AddHitCounts)
{
  const size_t y = OtherIrset.GetTotalEntries ();
  if (Total == 0 || Total > y)
    Total = y;
  if (Total)
    {
      // Save an allocation/copy (maybe)
      if ((Total + TotalEntries) >= MaxEntries)
	Resize(Total + TotalEntries + Increment);
      IRESULT iresult;
      for (size_t x = 1; x <= Total; x++)
	{
	  OtherIrset.GetEntry (x, &iresult);
	  AddEntry (iresult, AddHitCounts);
	}
    }
  return *this;
}

OPOBJ& atomicIRSET::operator +=(const OPOBJ& OtherIrset)
{
  return Cat(OtherIrset, OtherIrset.GetTotalEntries (), GDT_FALSE);
}

OPOBJ *atomicIRSET::Duplicate () const
{
//cerr << "IRSET::Duplicate!!" << endl;
  POPOBJ Temp = new atomicIRSET (Parent);
  if (Temp)
    {
      *Temp = *this;
    }
  return Temp;
}

atomicIRSET *atomicIRSET::Duplicate()
{
//cerr << "IRSET:Duplicate(2)!!" << endl;
  atomicIRSET * Temp= new atomicIRSET(Parent);
  if (Temp)
    {
      *Temp=*((OPOBJ*)this);
    }
  return(Temp);
}

void atomicIRSET::MergeEntries(const GDT_BOOLEAN AddHitCounts)
{
  size_t  CurrentItem = 0;

  if (TotalEntries == 0) return;

  SortByIndex();

  // TODO: Sort to do a faster merge!
  INDEX_ID idx     = Table[0].GetIndex();
  INDEX_ID old_idx = 0;
  Sort = ByIndex;
  DOUBLE score;
  long   adds = 0;

  for (size_t x = 1; x < TotalEntries; x++)
    {
      if (idx == Table[x].GetIndex())
	{
	  if (CurrentItem != x)
	    {
	      adds++;
	      if (AddHitCounts)
		{
		  Table[CurrentItem].IncHitCount(Table[x].GetHitCount());
		  Table[CurrentItem].AddToHitTable(Table[x]);
		}
	      if (ComputedS)
		{
		  score = Table[CurrentItem].IncScore(Table[x].GetScore());
		  if (score > MaxScore) MaxScore = score;
		  if (score < MinScore) MinScore = score;
		}
	    }
	}
      else
	{
	  if (++CurrentItem != x)
	    {
	      idx = (Table[CurrentItem] = Table[x]).GetIndex();
	      if (Sort == ByIndex && idx < old_idx)
		Sort = Unsorted;
	    }
	  else
	    idx = Table[x].GetIndex();
	  old_idx = idx;
	}
    }
  CurrentItem++;
  logf (LOG_DEBUG, "MergeEntries: %ld adds from %ld -> %ld entries", adds, TotalEntries, CurrentItem);

  TotalEntries = CurrentItem;
}


void atomicIRSET::SetVirtualIndex(const UCHR NewvIndex)
{
  for (size_t i=0; i<TotalEntries; i++)
    Table[i].SetVirtualIndex( NewvIndex );
}

UCHR atomicIRSET::GetVirtualIndex(size_t i) const
{
  if (i > 0 && i < TotalEntries)
    {
      return Table[i-1].GetVirtualIndex();
    }
  return 0;
}

// Fixup the MDT pointers
void atomicIRSET::SetMdt(const IDBOBJ *Idb)
{
  if (Idb)
    {
      for (size_t i=0; i<TotalEntries; i++)
	{
	  if (Table[i].GetMdt() == NULL)
	    Table[i].SetMdt( Idb->GetMainMdt ( Table[i].GetVirtualIndex() ) );
	}
    }
}

void atomicIRSET::SetMdt(const MDT *NewMdt)
{
  const MDT *MdtPtr = NewMdt ? NewMdt : (Parent ? Parent->GetMainMdt () : NULL);
  if (MdtPtr)
    {
      for (size_t i=0; i<TotalEntries; i++)
	Table[i].SetMdt(MdtPtr);
    }
}

const MDT *atomicIRSET::GetMdt(size_t i) const
{
  if (i > 0 && i <= TotalEntries)
    {
      const MDT *Mdt = Table[i-1].GetMdt();
      if (Mdt)
	return Mdt;
      return Parent ? Parent->GetMainMdt ( Table[i-1].GetVirtualIndex() ) : NULL;
    }
  return Parent ? Parent->GetMainMdt ( ) : NULL;
}

GDT_BOOLEAN atomicIRSET::GetMdtrec(size_t i, MDTREC *Mdtrec) const
{
  if (i > 0 && i <= TotalEntries)
    {
      const MDT *Mdt = GetMdt(i);
//cerr << "GetMdtrec(" << i << ")=";
      if (Mdt) {
//cerr << "Mdt->Name=" <<  Mdt->c_str() << endl;
        INT idx = Table[i-1].GetMdtIndex();
        if (idx) return Mdt->GetEntry(idx, Mdtrec);
        else logf (LOG_PANIC, "::Getmdtrec MDT position undefined (ZERO)");
      }
    }
  return GDT_FALSE;
}


void atomicIRSET::LoadTable (const STRING& FileName)
{
  GDT_BOOLEAN ok = GDT_FALSE;
  TotalEntries = 0;
  PFILE fp = FileName.Fopen ("rb");
  if (fp)
    {
      ok = Read(fp);
      fclose(fp);
    }
  if (ok == GDT_FALSE && Parent)
    Parent->SetErrorCode(30); // "Specified result set does not exist"
}

GDT_BOOLEAN atomicIRSET::Read (PFILE fp)
{
  TotalEntries = 0;
  obj_t obj = getObjID(fp);
  if (obj != objIRSET)
    {
       PushBackObjID(obj, fp);
    }
  else
    {
      UINT4 NewTotal;
      ::Read(&NewTotal, fp);
      Resize (NewTotal);
      for (UINT4 i=0; i < NewTotal; i++)
	Table[i].Read(fp);
      TotalEntries = NewTotal;

      float Min, Max;
      ::Read(&Max, fp);
      ::Read(&Min, fp);
      MaxScore = Max;
      MinScore = Min;
      
      UCHR x;
      ::Read(&x, fp);
      Sort = SortRequest = (enum SortBy)x;

      ::Read(&x, fp);
      ComputedS = (enum NormalizationMethods)x;

      SRCH_DATE TimeStamp;
      ::Read(&TimeStamp, fp);

      STRING path, stem;
      path.Read(fp);
      if (Parent && PathCompare(path.c_str(), (stem = Parent->GetDbFileStem()).c_str()))
	logf (LOG_WARN, "IRSET parent instance name mis-match '%s' != '%s'",
		path.c_str(), stem.c_str());

      if (Parent && TimeStamp <  Parent->DateLastModified() )
	return GDT_FALSE; // Expired
    }
  return obj == objIRSET;
}

void atomicIRSET::SaveTable (const STRING& FileName) const
{
  if (TotalEntries)
    {
      PFILE fp = FileName.Fopen("wb");
      if (fp)
	{
	  Write(fp);
	  fclose(fp);
	}
    }
  else
    FileName.Unlink ();
}

/*
Format:
  <MAGIC>
  <DB STEM NAME>
  <DB Options>
  <TOTAL ENTRIES>
  <TABLE>
  <MAX SCORE>
  <MIN SCORE>
  <SORT>
*/
void atomicIRSET::Write (PFILE fp) const
{
  size_t i;

  putObjID(objIRSET, fp);
  ::Write((UINT4)TotalEntries, fp);
  for (i=0; i < TotalEntries; i++)
    {
      Table[i].Write(fp);
    }
  ::Write((float)MaxScore, fp);
  ::Write((float)MinScore, fp);
  ::Write((UCHR)Sort, fp);
  ::Write((UCHR)ComputedS, fp);
  ::Write(SRCH_DATE("Now"), fp);
  if (Parent)
    Parent->GetDbFileStem().Write(fp);
  else
    NulString.Write(fp);
}


void atomicIRSET::FastAddEntry(const IRESULT& ResultRecord)
{
  HitTotal = 0;
  if (TotalEntries > 1)
    {
      const INDEX_ID index = ResultRecord.GetIndex ();
      if (Table[TotalEntries-1].GetIndex () == index)
        {
          Table[TotalEntries-1].IncHitCount (ResultRecord.GetHitCount ());
          Table[TotalEntries-1].AddToHitTable (ResultRecord);
	  if (ComputedS > NoNormalization)
	    {
	      const DOUBLE score = Table[TotalEntries-1].IncScore ( ResultRecord.GetScore() );
	      if (score < MinScore) MinScore = score;
	      if (score > MaxScore) MaxScore = score;
	      // if (Sort == ByScore) Sort = Unsorted;
	    }
	  return;
	}
      if (Sort == ByIndex)
	{
	  if (Table[TotalEntries-1].GetIndex() > index)
	    {
	      // New index is not in order
	      if (TotalEntries == 1)
		{
		  // Since we'll only have 2 entries can swap!
		  Table[TotalEntries++] = Table[0];
		  Table[0] = ResultRecord;
		  if ( ComputedS > NoNormalization)
		    {
		      const DOUBLE score = ResultRecord.GetScore();
		      if (score < MinScore) MinScore = score;
		      if (score > MaxScore) MaxScore = score;
		    }
		  return;
		}
	      else
		{
		  logf (LOG_DEBUG, "atomicIRSET::FastAddEntry new record out of index order");
		  Sort = Unsorted;
		}
	    }
	}
    }

  if ( ComputedS > NoNormalization )
    {
      const DOUBLE score = ResultRecord.GetScore();
      if (score < MinScore) MinScore = score;
      if (score > MaxScore) MaxScore = score;
      if (Sort == ByScore)
	{
	  if (TotalEntries && Table[TotalEntries-1].GetScore() < ResultRecord.GetScore ())
	    Sort = Unsorted;
	}
    }

  if (TotalEntries == MaxEntries)
    Expand();

  // SANITY CHECK: Make sure we have room
  if (TotalEntries < MaxEntries)
    Table[TotalEntries++] = ResultRecord;
}

size_t atomicIRSET::FindByMdtIndex(size_t Index) const
{
  if (Sort == ByIndex && TotalEntries > 3)
    {
      // Binary search...
      IRESULT *ptr = (IRESULT *)bsearch(&Index, (const void *)Table, TotalEntries, sizeof(IRESULT), iIndexCompare);
      if (ptr)
	{
	  return (size_t)(ptr - Table) + 1;
	}
    }
  // linear
  for (size_t x = 0; x < TotalEntries; x++)
    {
      if (Table[x].GetIndex ().Equals(Index))
	return x + 1;
    }
  return 0; // NOT FOUND
}

void atomicIRSET::AddEntry (const IRESULT& ResultRecord, const GDT_BOOLEAN AddHitCounts)
{
  const INDEX_ID Index = ResultRecord.GetIndex (); 
  size_t pos;

  HitTotal = 0;
  if (Table[TotalEntries-1].GetIndex () == Index)
    pos = TotalEntries;
  else
    pos = FindByMdtIndex( Index );
  if (pos)
    {
      pos--; // Into array
      if (AddHitCounts)
	{
	  Table[pos].IncHitCount (ResultRecord.GetHitCount ());
	  Table[pos].AddToHitTable (ResultRecord);
	}
#if USE_GEOSCORE
      Table[pos].SetGscore(ResultRecord.GetGscore());
#endif
      if (ComputedS > NoNormalization)
	{
	  const DOUBLE score = Table[pos].IncScore (ResultRecord.GetScore ());
	  if (score < MinScore) MinScore = score;
	  if (score > MaxScore) MaxScore = score;
	  if (Sort == ByScore)
	    {
	      if (((pos+1) < TotalEntries) && (score > Table[pos+1].GetScore()))
		Sort = Unsorted;
	    }
	}
      return; // Done
    }

  // NOT in table, so add..
  if (Sort == ByIndex)
    {
      // Did we break the sort (increasing index number)?
      if (TotalEntries && Table[TotalEntries-1].GetIndex() > Index) 
	{
	  Sort = Unsorted;
	}
    }
  else if (Sort == ByScore)
    {
      // Did we break the sort (best score to lowest order)?
      if (TotalEntries && Table[TotalEntries-1].GetScore() < ResultRecord.GetScore ())
	Sort = Unsorted;
    }
  else
    Sort = Unsorted;
  if (TotalEntries == MaxEntries)
    Expand ();
  // SANITY CHECK: Enough memory?
  if (TotalEntries < MaxEntries)
    Table[TotalEntries++] = ResultRecord;
}

GDT_BOOLEAN atomicIRSET::GetEntry (const size_t Index, PIRESULT ResultRecord) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *ResultRecord = Table[Index - 1];
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

IRESULT atomicIRSET::GetEntry (const size_t Index) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    return Table[Index - 1];
  return IRESULT();
}


PRSET atomicIRSET::GetRset() const
{
  return GetRset (TotalEntries);
}

PRSET atomicIRSET::GetRset(size_t Total) const
{
  return GetRset (0, Total);
}


// First element is 0
PRSET atomicIRSET::GetRset(size_t Start, size_t End) const
{
  return Fill(Start+1, End, NULL);
}

// First element is 1.
PRSET atomicIRSET::Fill(size_t Start, size_t End, PRSET set) const
{
  if (TotalEntries == 0 || (Start == 0 && End == 0)) 
    return new RSET();

  if (Start == 0) Start = 1;

  size_t TotalHits = set ? set->GetHitTotal() : 0;
  if(End == 0 || End>TotalEntries)
    End=TotalEntries;

  if (ComputedS == preCosineMetricNormalization)
    {
      logf (LOG_WARN, "CosineMetricNormalization still in pre-Form!");
      //ComputeScoresCosineMetricNormalization (1);
    }

  if (ComputedS == Unnormalized)
    logf (LOG_ERROR, "IRSET Scores have not been converted before fill.");

  if (Start > End)
    {
      logf (LOG_PANIC, "RSET Fill Range End (%d) > Start (%d). Swap values?", End, Start);
      size_t tmp = Start;
      Start = End;
      End   = tmp;
    }

  if (set == NULL)
    {
      size_t   space = End-Start+1;
      try {
	set = new RSET(space);
      } catch (...) {
	set = NULL;
	if (errno == 0) errno = ENOMEM;
	logf (LOG_ERRNO, "IRSET: Could not allocate space for RSET(%ld RESULTs)!", space);
      }
    }
  if (set == NULL)
    {
      if (Parent) Parent->SetErrorCode(2); // Temp Error
      return set;
    }

  logf (LOG_DEBUG, "Filling RSET %d-%d", Start, End);

  const DOUBLE pFactor = (MaxScore && (MaxScore != MinScore)) ?
	(Parent->GetPriorityFactor()*(MaxScore - MinScore))/MaxScore :
	Parent->GetPriorityFactor();
  DOUBLE newMinScore = MinScore;
  DOUBLE newMaxScore = MaxScore;
  enum SortBy newSort = Sort;

  for (size_t x=Start-1; x<End; x++) {
   MDTREC mdtrec;
   if (!GetMdtrec(x+1, &mdtrec))
      {
	logf (LOG_ERROR, "atomicIRSET Can't fill %d", x);
      }
    else if (mdtrec.GetDeleted() == GDT_FALSE)
      {
	RESULT result (mdtrec);
	DOUBLE myScore = Table[x].GetScore();
	// Handle Priority
	if (pFactor)
	  {
	    const _ib_priority_t priority = mdtrec.GetPriority();
	    const DOUBLE factor = priority ? pFactor/priority : 0;
	    if (factor)
	      {
		myScore += factor;
		if (myScore > newMaxScore)
		  newMaxScore = myScore;
		if (myScore < newMinScore)
		  newMinScore = myScore;
		if (newSort == ByScore)
		  newSort = Unsorted;
	      }
	  }
	result.SetScore( myScore );

	// Resolve Paths
        if (Parent && Parent->getUseRelativePaths())
	  result.SetPath ( Parent->ResolvePathname (result.GetPath()) );

	// From the Table..
	result.SetIndex (Table[x].GetIndex() );
  	result.SetAuxCount(Table[x].GetAuxCount());
	// Get the Hit table
	FCT Fctable = Table[x].GetHitTable();
	//Fct.ConvertHits(mdtrec);
	result.SetHitTable (Fctable -= (mdtrec.GetGlobalFileStart() + 
                mdtrec.GetLocalRecordStart())); // See index.cxx
	// Add the result to the set...
	set->FastAddEntry(result); // Was AddEntry()
	TotalHits += Table[x].GetHitCount ();
      }
    else
      logf (LOG_DEBUG, "Deleted record (%d)...", x);
  }
  set->SetScoreRange (newMaxScore, newMinScore); // Added..
  set->SetHitTotal ( TotalHits );
  if (newSort != SortRequest && SortRequest != Unsorted)
    {
      set->Sort = newSort;
      set->SortBy(SortRequest);
    }
  else
    set->Sort = SortRequest;

  set->SetTotalFound (TotalEntries);

  return set;
}

void atomicIRSET::Expand ()
{
  size_t Suggest = (1+(TotalEntries/1024))*4096 + (int)(Increment*exp((double)(allocs+1)));
  if (Suggest > MaxEntriesAdvice && MaxEntriesAdvice > MaxEntries)
    Suggest = MaxEntriesAdvice;
  else if (MaxEntriesAdvice < MaxEntries && Parent)
    {
      const size_t mRecords = Parent->GetTotalRecords();
      const size_t mr = (mRecords*3)/4;
      if (Suggest > mRecords && mRecords > MaxEntries)
	Suggest = mRecords;
      if (Suggest > mr && mr > MaxEntries)
	Suggest = mr;
    }

  Resize ( Suggest ); 
}

void atomicIRSET::CleanUp ()
{
  Resize (TotalEntries);
}

void atomicIRSET::Resize (const size_t Entries)
{
  if (Entries > 0)
    {
      if (Entries == MaxEntries)
	return; // Already as we wanted!

      const size_t NewTotal = (Entries >= TotalEntries) ? TotalEntries : Entries;
      const size_t NewMaxEntries = Entries;
      IRESULT *NewTable = NULL;

//cerr << "RESIZE: Allocate new.. " << NewMaxEntries << " // " << __IB_IRESULT_allocated_count << endl; 

      try {
	NewTable = new IRESULT[NewMaxEntries];
	allocs++;
      } catch (...) {
	if (errno == 0) errno = ENOMEM;
	logf (LOG_ERRNO, "IRSET::Resize(%ld): Could not allocate space for %ld IRESULTs!",
	   (long)Entries, (long)NewMaxEntries);
	if (Parent) Parent->SetErrorCode(2); // Temp Error
	Resize(0); // Resize to nothing!
	return;
      }

//cerr << "RESIZE: Set " << NewTotal << " entries // " << __IB_IRESULT_allocated_count << endl;

      for (size_t i = 0; i < NewTotal; i++)
	{
	  NewTable[i] = Table[i];
	}
      if (Table) delete [] Table;
      Table = NewTable;
      MaxEntries = NewMaxEntries;
      TotalEntries = NewTotal;
    }
  else
    {
      // Shrunk to nothing
      if (Table)
	{
	  delete [] Table;
	  Table = NULL;
	}
      MaxEntries = 0;
      TotalEntries = 0;
      allocs = 0;
    }
}

/* INLINED
size_t atomicIRSET::GetTotalEntries () const
{
  return TotalEntries;
}
*/

size_t atomicIRSET::SetTotalEntries(size_t NewTotal)
{
#if 0
  if (NewTotal == 0)
    Resize(0);
  else
#endif
  if (NewTotal < TotalEntries)
    TotalEntries = NewTotal;
  return TotalEntries;
}

#if 0
size_t  atomicIRSET::GetMaxAuxCount() const
{

}

#endif

size_t atomicIRSET::GetHitTotal ()
{
  if (HitTotal && TotalEntries > 0)
    return HitTotal;

  size_t Total = 0;
  for (size_t x = 0; x < TotalEntries; x++)
    Total += Table[x].GetHitCount ();
  return HitTotal = Total;
}

// added edz: Complement (binary NOT)
OPOBJ *atomicIRSET::Not ( )
{
  const size_t MdtTotal =  Parent ? Parent->GetTotalRecords () : 0;

  HitTotal = 0;
  if (TotalEntries >= MdtTotal)
    {
      // Nothing
      TotalEntries = 0;
      return this;
    }

  MaxEntries = MdtTotal - TotalEntries + 1;

  if (Parent)
    {
      INDEX *indexp = Parent->GetMainIndex();
     
      if (indexp)
	{
	  const size_t TooManyRecords = indexp->GetMaxRecordsAdvice();
	  if ((TotalEntries == 0) && (MdtTotal > TooManyRecords) )
	    {
	      Parent->SetErrorCode(12); //  "Too many records retrieved";
	      MaxEntries = TooManyRecords ;
	    }
	}
    }

  SortByIndex();

  // Caculate the score for this set...
  const DOUBLE SqrtSum     = sqrt( (double)(MdtTotal*MdtTotal) / (double)(MdtTotal - TotalEntries) );
  const DOUBLE Score       = (SqrtSum == 0.0) ? 1 : 1 / SqrtSum;

  PIRESULT OldTable = Table;
  size_t   OldTotal = TotalEntries;

  if (Parent)
    {
      // Keep the size down...
      size_t Cutoff = Parent->GetDbSearchCutoff();
      if (Cutoff > 0 && Cutoff < MaxEntries)
	MaxEntries = Cutoff;
    }

   try {
    Table = new IRESULT[MaxEntries];
   } catch (...) {
    Table = NULL;
    if (errno == 0) errno = ENOMEM;
    logf (LOG_ERRNO, "IRSET: Could not allocate space for NOT (%ld IRESULTs)!", (long)MaxEntries);
    if (Parent) Parent->SetErrorCode(2); // Temp Error
    Clear();
    return NULL; // To signify an error
   }

  TotalEntries = 0;
  MinScore=MAXFLOAT;
  MaxScore=0.0;

  IRESULT *ptr = OldTable;
  size_t offset = 0;

  INDEX_ID Index;
  Index.SetVirtualIndex(Parent ? Parent->GetVolume() : 0);

  for (size_t i = 1; i <= MdtTotal; i++)
    {
     Index.SetMdtIndex(i);
     _index_id_t index = Index;
     if (OldTotal == 0 ||
	(ptr = (IRESULT *)bsearch(&index, (const void *)(OldTable + offset), OldTotal-offset,
		sizeof(IRESULT), iIndexCompare)) == NULL)
        {
	  Table[TotalEntries].SetIndex (Index);
	  Table[TotalEntries].SetHitCount (1);
	  Table[TotalEntries].SetAuxCount (1);
	  Table[TotalEntries].SetScore (Score); 
#if 1
	  Table[TotalEntries].ClearHitTable(); // No HITS!
#else	  /* Should the whole record be a hit? */
	  // NO: Instead we want to (and this needs to be handled by the calling
	  // code in search. To handle in numeric search etc. fielded the fields
	  // in the other records of the same name but without the content.
	  if (Mdt)
	    {
	     const  MDTREC *mp = Mdt->GetEntry(i);
	      if (mp)
		{
		  const GPTYPE start = mp->GetLocalRecordStart ();
		  const GPTYPE end   = mp->GetLocalRecordEnd ());
		  Table[TotalEntries].SetHitTable ( FC(start,end) );
		}
	    }
#endif
	  if (++TotalEntries >= MaxEntries)
	    break;
	}
     else if (OldTotal)
	{
	  if ((offset = ptr - OldTable) == OldTotal)
	    OldTotal = 0; // Grab the rest...
        }
    }
  if (TotalEntries)
    {
      MinScore = Score;
      MaxScore = Score;
    }
  // Swap the tables out
  if (OldTable) delete [] OldTable;
//Sort = Unsorted;
  return this;
}

// added edz: Complement 
// Not but also in a field
OPOBJ *atomicIRSET::Not (const STRING& FieldName)
{
  if (FieldName == "*") return Not();

#ifdef FIELD_WILD_MATCH
  STRING    pattern(FieldName);

  if (!FieldName.IsPlain() && Parent)
    {
      DFDT *dfdtp = Parent->GetDfdt();
      int   count = 0;
      if (dfdtp)
        {
          const size_t total = dfdtp->GetTotalEntries();
          STRING       field, fld;
	  atomicIRSET  input(Parent);

	  // pattern.ToUpper();
	  // pattern.Replace("\\", "/");
          for (size_t i=0; i<total; i++)
            {
	      fld = field = dfdtp->GetFieldName(i+1);
	      //fld.Replace("\\", "/");
              if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, fld))
                {
		  if (count++ == 0) {
		     input.Cat(*this);
		     Not(field);
		  } else {
		    atomicIRSET  Set (Parent);
		    Set.Cat (input);
		    Set.Not(field);
		    And(Set);
		  }
                }
            }
        }
      if (count == 0) //  No matching field
	Clear(); // All of nothing is nothing
      return this; 
    }
#endif

  if (!FieldExists(FieldName))
    {
      // Complement in Nothing of Nothing is Nothing
      Clear();
      return this;
    }
 
  const size_t MdtTotal =  Parent ? Parent->GetTotalRecords () : 0;

  HitTotal = 0;
  if (TotalEntries >= MdtTotal)
    {
      // Nothing
      TotalEntries = 0;
      return this;
    }

  MaxEntries = MdtTotal - TotalEntries + 1;

  if (Parent)
    {
      INDEX *indexp = Parent->GetMainIndex();
     
      if (indexp)
	{
	  const size_t TooManyRecords = indexp->GetMaxRecordsAdvice();
	  if ((TotalEntries == 0) && (MdtTotal > TooManyRecords) )
	    {
	      Parent->SetErrorCode(12); //  "Too many records retrieved";
	      MaxEntries = TooManyRecords ;
	    }
	}
    }

  SortByIndex();

  // Caculate the score for this set...
  const DOUBLE SqrtSum     = sqrt( (double)(MdtTotal*MdtTotal) / (double)(MdtTotal - TotalEntries) );
  const DOUBLE Score       = (SqrtSum == 0.0) ? 1 : 1 / SqrtSum;

  PIRESULT OldTable = Table;
  size_t   OldTotal = TotalEntries;

  if (Parent)
    {
      // Keep the size down...
      size_t Cutoff = Parent->GetDbSearchCutoff();
      if (Cutoff > 0 && Cutoff < MaxEntries)
	MaxEntries = Cutoff;
    }

  // Allocate
  try {
    Table = new IRESULT[MaxEntries];
   } catch (...) {
    Table = NULL;
    if (errno == 0) errno = ENOMEM;
    logf (LOG_ERRNO, "IRSET:: Could not allocate space for NOT:%s (%ld IRESULTs)!",
	FieldName.c_str(), (long)MaxEntries);
    if (Parent) Parent->SetErrorCode(2); // Temp Error
    Clear();
    return NULL; // To signify an error
   }

  TotalEntries = 0;
  MinScore=MAXFLOAT;
  MaxScore=0.0;

  IRESULT *ptr = OldTable;
  size_t offset = 0;

  INDEX_ID  Index;
  DATERANGE DateRange;
  MDTREC    mdtrec;

  if (Parent) Parent->GetDateRange(&DateRange);

  Index.SetVirtualIndex(Parent ? Parent->GetVolume() : 0);

  for (size_t i = 1; i <= MdtTotal; i++)
    {
      const MDT *Mdt = GetMdt(i);
      Index.SetMdtIndex(i);
      _index_id_t index = Index;
      if (OldTotal == 0 ||
	(ptr = (IRESULT *)bsearch(&index, (const void *)(OldTable + offset), OldTotal-offset,
		sizeof(IRESULT), iIndexCompare)) == NULL)
        {
	  if (Mdt->GetEntry(i, &mdtrec))
	    {
	      if ( DateRange.Defined () )
                {
                  if ( !DateRange.Contains ( mdtrec.GetDate () ) )
                    continue; // Not in Range
                  if (mdtrec.GetDeleted ())
                    continue; // Don't bother with deleted records
                }
             // Now look for the hits
              FCT fct ( Parent->GetFieldFCT (mdtrec, FieldName) );

              if (! fct.IsEmpty() )
                {
		  Table[TotalEntries].SetIndex (Index);
		  Table[TotalEntries].SetHitCount (1);
		  Table[TotalEntries].SetAuxCount (1);
		  Table[TotalEntries].SetScore (Score); 
		  Table[TotalEntries].SetHitTable ( fct );
		  if (++TotalEntries >= MaxEntries)
		    break;
		}
	    }
	}
     else if (OldTotal)
	{
	  if ((offset = ptr - OldTable) == OldTotal)
	    OldTotal = 0; // Grab the rest...
        }
    }
  if (TotalEntries)
    {
      MinScore = Score;
      MaxScore = Score;
    }
  // Swap the tables out
  if (OldTable) delete [] OldTable;
//Sort = Unsorted;
  return this;
}


OPOBJ *atomicIRSET::SortBy (const STRING& ByWhat)
{
cerr << "SortBy:" << ByWhat << endl;
  if (ByWhat.IsNumber())
    SortBy((enum SortBy) ByWhat.GetInt());
  else switch (ByWhat.GetLength()) {
   case 3:
     if (ByWhat ^= "Key") SortBy(ByKey);
     break;
   case 4:
     if      (ByWhat ^= "Hits") SortBy(ByHits);
     else if (ByWhat ^= "Date") SortBy(ByDate);
     break;
   case 5:
     if      (ByWhat ^= "Index") SortBy(ByIndex);
     else if (ByWhat ^= "Score") SortBy(ByScore);
     break;
   case 8:
     if      (ByWhat ^= "AuxCount") SortBy(ByAuxCount);
     else if (ByWhat ^= "Newsrank") SortBy(ByNewsrank);
     else if (ByWhat ^= "Function") SortBy(ByFunction);
     else if (ByWhat ^= "Category") SortBy(ByCategory);
     break;
   case 11:
     if      (ByWhat ^= "ReverseHits") SortBy(ByReverseHits);
     else if (ByWhat ^= "ReverseDate") SortBy(ByReverseDate);
     break;
   default:
    if (strncasecmp(ByWhat, "private",  7) == 0)
      {
	const char *value = ByWhat.c_str() + 7;
	const int   which = *value ? atoi(value) : 0;
	SortByPrivate( which );
	break;
      }
    Parent->SetErrorCode(133); // Unsupported sort unit code
  }
  return this;
}


OPOBJ *atomicIRSET::Trim (const float Level)
{
  int cut = Level;
  if (cut > 0 && TotalEntries > cut && (Level - cut == 0.0))
    TotalEntries = cut;
  else if (Level < 1.0 && Level > 0.0)
    {
      cut = (int)( Level * Parent->GetTotalRecords () + 0.5);
      if (cut > 0 && TotalEntries > cut)
	TotalEntries = cut;
    }
  else if (Level == 0.0)
    {
      Clear();
    }
  else if (Level - cut > 0.0)
    {
      cut += (int)( (Level-cut) * Parent->GetTotalRecords () + 0.5);
      if (cut > 0 && TotalEntries > cut)
	TotalEntries = cut;
    }
  return this;
}

// Level: Min. number of hits needed to remain in set
//
//     10 -> 10 or more (including 10)
//    -10 -> 10 or less (including 10)
// Negative number means HitCount < number
OPOBJ *atomicIRSET::HitCount (const float Level)
{
  size_t        i = 0;
  int           cut = Level > 0 ? (int)Level : - (int)Level;

  enum SortBy oldSort = Sort;

  if (Level >= 0) SortByHits ();
  else            SortByReverseHits ();
  // Reset the score
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  for (; i < TotalEntries; i++)
    {
      const int hits = Table[i].GetHitCount();
      if (Level >= 0)
	{
	  if (hits < cut) break;
	}
      else if (hits > cut) break;
      const DOUBLE score = Table[i].GetScore();
      if (MinScore > score) MinScore = score;
      if (MaxScore < score) MaxScore = score;
    }
  TotalEntries = i;

  SortBy( oldSort ); // Sort the way it was

  return this;
}

OPOBJ *atomicIRSET::Reduce (const float Level)
{
  size_t      TermCount = (size_t)Level;
  enum SortBy oldSort = Sort;

  // Need at least 1 but don't want to reduce a set of 1 element
  if (TotalEntries <= 1) return this;

  SortByAuxCount ();

  UINT Count0 = Table[0].GetAuxCount();

  if (TermCount == 0)
    TermCount = Count0;

  size_t cutoff = TermCount > 2 ? 1 : 0;
  size_t i = 0;
  if (Count0 == TermCount)
     cutoff = TermCount -1;   
  else if (TotalEntries > 1)
    {
      UINT Count1 = Table[1].GetAuxCount();
      if (Count1 == TermCount)
	{
	  i = 1;
	  cutoff = TermCount - 1;
        }
      else if (Count1 > Count0 || TotalEntries < 4)
	{
	  i = 1;
	}
      else
	{
	  Count0 = Count1;
	  for (i=2; i < TotalEntries && ((Count1 = Table[i].GetAuxCount()) > Count0); i++)
	    /* loop */;
	}
    }

  // Reset the score
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  for (; i < TotalEntries; i++)
    {
      const DOUBLE score = Table[i].GetScore();
      if (MinScore > score) MinScore = score;
      if (MaxScore < score) MaxScore = score;
      if (i && (Count0 = Table[i].GetAuxCount()) <= cutoff)
        TotalEntries = i;
    }

  // TODO: set MinScore and MaxScore (!)
  HitTotal = 0;

  SortBy( oldSort ); // Sort the way it was

  return this;
}


OPOBJ *atomicIRSET::Or (const OPOBJ& OtherIrset)
{
  // Can't OR on different Dbs 
  if (Parent == NULL || Parent != OtherIrset.GetParent())
    {
      logf (LOG_DEBUG, "Can't OR between different physical indexes.");
      Parent->SetErrorCode(3); // Unsupported search
      return this; 
    }

  const size_t OtherTotal = OtherIrset.GetTotalEntries();

  if (OtherTotal == 0)
    return this; // Nothing to add..
  if (TotalEntries == 0)
    {
      // Just grab the others..
      *this = OtherIrset;
      return this;
    }

  HitTotal = 0;
  if (OtherIrset.GetSort() != ByIndex)
    return _Or(OtherIrset);

  // Merge both result sets into one sorted...
  IRESULT  OtherIresult, Iresult;
  INDEX_ID idx1, idx2;
  size_t   pos1 = 1, opos1 = 0;
  size_t   pos2 = 1, opos2 = 0;
  size_t   newMaxEntries = TotalEntries+OtherTotal;
  if (Parent)
    {
      // Keep the size down...
      size_t Cutoff = Parent->GetDbSearchCutoff();
      if (Cutoff > 0 && Cutoff < newMaxEntries)
        newMaxEntries = Cutoff;
    }

  IRESULT *NewTable = NULL; 
  try {
    NewTable = new IRESULT[newMaxEntries];
   } catch (...) {
    if (errno == 0) errno = ENOMEM;
    logf (LOG_ERRNO, "IRSET: Could not allocate space for OR (%ld IRESULTs)!", (long)newMaxEntries);
    if (Parent) Parent->SetErrorCode(2); // Temp Error
    Clear();
    return NULL; // ERROR
  }

  size_t   current = 0;

  const DOUBLE   OtherMin = OtherIrset.GetMinScore();
  const DOUBLE   OtherMax = OtherIrset.GetMaxScore();
  if (MinScore > OtherMin)
    MinScore = OtherMin;
  if (MaxScore < OtherMax)
    MaxScore = OtherMax;

  SortByIndex();

  while (pos1 <= TotalEntries && pos2 <= OtherTotal && current < newMaxEntries)
    {
      if (pos1 != opos1)
	{
	  GetEntry (opos1 = pos1, &Iresult);
	  idx1 = Iresult.GetIndex();
	}
      if (pos2 != opos2)
	{
	  OtherIrset.GetEntry (opos2 = pos2, &OtherIresult);
	  idx2 = OtherIresult.GetIndex();
	}
      if (idx2 == idx1)
	{
	  NewTable[current] = Iresult;
	  NewTable[current].IncAuxCount( OtherIresult.GetAuxCount () );
	  const UINT hits = NewTable[current].IncHitCount (OtherIresult.GetHitCount ());
	  NewTable[current].AddToHitTable (OtherIresult);
          NewTable[current].SetVectorTermHits ( hits );
	  const DOUBLE score = NewTable[current++].IncScore (OtherIresult.GetScore ());
	  if (score < MinScore) MinScore = score;
	  if (score > MaxScore) MaxScore = score;
	  pos1++, pos2++;
	}
      else if (idx1 < idx2)
	{
	  NewTable[current++] = Iresult;
	  pos1++;
	}
      else // idx2 < idx1
	{
	  NewTable[current++] = OtherIresult;
	  pos2++;
	}
    }
  // Anything left?
  while (pos1 <= TotalEntries && current < newMaxEntries)
    {
      GetEntry(pos1++, &Iresult);
      NewTable[current++] = Iresult;
    }
  // Anything left?
  while (pos2 <= OtherTotal && current < newMaxEntries)
    {
      OtherIrset.GetEntry(pos2++, &OtherIresult);
      NewTable[current++] = OtherIresult;
    }
  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  return this;
}

OPOBJ *atomicIRSET::_Or (const OPOBJ& OtherIrset)
{
  const size_t Total = OtherIrset.GetTotalEntries ();
  if (Total)
    {
      // Set New Min and Max Scores...
      const DOUBLE OtherMax = OtherIrset.GetMaxScore();
      const DOUBLE OtherMin = OtherIrset.GetMinScore();
      if (OtherMax > MaxScore) MaxScore = OtherMax;
      if (OtherMin < MinScore) MinScore = OtherMin;

      // Save an allocation/copy (maybe)
      if ((Total + TotalEntries) >= MaxEntries)
        Resize(Total + TotalEntries + Increment);
      IRESULT iresult;
      for (size_t x = 1; x <= Total; x++)
        OtherIrset.GetEntry (x, Table + TotalEntries++);
      MergeEntries(GDT_TRUE);
    }
  return this;
}

/*
  IRESULT resul1 and result2 are join (like AND) if
  they have the same keys.

  AND: means if they have the same index in the same physical INDEX
  JOIN: includes AND but can span more than one physical INDEX.
*/

OPOBJ *atomicIRSET::Join (OPOBJ *Other)
{
  if (Other == NULL) return And(atomicIRSET());

  // Join of the same physical is an AND
  if (Parent == NULL || Parent == Other->GetParent())
    return Other = And(*Other);

  atomicIRSET *OtherIrset = ((atomicIRSET *)&OtherIrset);
  if (OtherIrset->Parent == NULL)
    return Join(NULL);

  const size_t OtherTotal = OtherIrset->GetTotalEntries();

  MDT         *MainMdt    = Parent->GetMainMdt();
  MDT         *OtherMdt   = OtherIrset->Parent->GetMainMdt();

  HitTotal = 0;

  if (OtherTotal == 0 || TotalEntries == 0 ||
	MainMdt == NULL || OtherMdt == NULL)
    {
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't Join with nothing..
    }

  // Sort both by key
  OtherIrset->SortByKey();
  SortByKey();

  size_t      pos1 = 1, opos1 = 0;
  size_t      pos2 = 1, opos2 = 0;
  size_t      current = 0;

  DOUBLE   newMinScore = MAXFLOAT;
  DOUBLE   newMaxScore = 0.0;

  DOUBLE   other_newMinScore = MAXFLOAT;
  DOUBLE   other_newMaxScore = 0.0;

  DOUBLE   score;

  IRESULT *OtherTable = OtherIrset->Table;
  IRESULT *IresultPtr = NULL;
  IRESULT *OtherIresultPtr = NULL;
  STRING   Key1, Key2;
  _index_id_t idx1=0, idx2=0;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal)
    {
      if (pos1 != opos1)
	{
	  opos1 = pos1;
	  IresultPtr = Table + pos1 - 1;
	  idx1 = IresultPtr->GetIndex();
	  Key1 = MainMdt->GetKey(idx1);
	}
      if (pos2 != opos2)
	{
	  opos2 = pos2;
	  OtherIresultPtr = OtherTable + pos2 - 1;
	  idx2 = OtherIresultPtr->GetIndex();
	  Key2 = OtherMdt->GetKey(idx2);
	}

      int diff = Compare (Key1, Key2);
      if (diff == 0)
	{
	  score = Table[current].IncScore (OtherIresultPtr->GetScore ());
	  if (score < newMinScore) newMinScore = score;
	  if (score > newMaxScore) newMaxScore = score;

	  score = OtherTable[current++].IncScore (IresultPtr->GetScore ());
	  if (score < other_newMinScore) other_newMinScore = score;
	  if (score > other_newMaxScore) other_newMaxScore = score;

	  pos1++, pos2++;
	}
      else if (diff > 0) pos1++;
      else               pos2++;
    }
  OtherIrset->TotalEntries = TotalEntries = current;
  OtherIrset->Sort         = Sort         = ByKey;

  MinScore                 = newMinScore;
  MaxScore                 = newMaxScore;

  OtherIrset->MinScore     = other_newMinScore;
  OtherIrset->MaxScore     = other_newMaxScore;

  return this;
}


OPOBJ *atomicIRSET::Join (const OPOBJ& OtherIrset)
{
  // Join of the same physical is an AND
  if (Parent == NULL || Parent == OtherIrset.GetParent())
    return And(OtherIrset);

  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  MDT         *MainMdt    = Parent->GetMainMdt();

  HitTotal = 0;

  if (OtherTotal == 0 || TotalEntries == 0 || MainMdt == NULL)
    {
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't Join with nothing..
    }

  if (OtherIrset.GetSort() != ByKey)
    {
#if 1
      // Force for now
      ((atomicIRSET *)&OtherIrset)->SortByKey();
#else
      // Insert code;
      logf (LOG_ERROR, "Operator Join not yet implemented");
      return this;
#endif
    }
  SortByKey();

  size_t      pos1 = 1, opos1 = 0;
  size_t      pos2 = 1, opos2 = 0;
  size_t      current = 0;

  DOUBLE   newMinScore = MAXFLOAT;
  DOUBLE   newMaxScore = 0.0;
  DOUBLE   score;

  IRESULT *OtherTable = ((const atomicIRSET *)&OtherIrset)->Table;
  IRESULT *IresultPtr = NULL;
  IRESULT *OtherIresultPtr = NULL;
  STRING   Key1, Key2;
  _index_id_t idx1=0, idx2=0;

#if 0
  size_t      newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
  PIRESULT    NewTable = new IRESULT[newMaxEntries];
#endif

  while (pos1 <= TotalEntries && pos2 <= OtherTotal)
    {
      if (pos1 != opos1)
	{
	  opos1 = pos1;
	  IresultPtr = Table + pos1 - 1;
	  idx1 = IresultPtr->GetIndex();
	  Key1 = MainMdt->GetKey(idx1);
	}
      if (pos2 != opos2)
	{
	  opos2 = pos2;
	  OtherIresultPtr = OtherTable + pos2 - 1;
	  idx2 = OtherIresultPtr->GetIndex();
	  Key2 = MainMdt->GetKey(idx2);
	}
      if (Key1 == Key2)
	{
#if 0
          score =  (NewTable[current++] = *IresultPtr).IncScore (OtherIresultPtr->GetScore ());
#else
	  if (current != pos1)
	    Table[current] = *IresultPtr;
	  score =  Table[current++].IncScore (OtherIresultPtr->GetScore ());
#endif
	  if (score < newMinScore) newMinScore = score;
	  if (score > newMaxScore) newMaxScore = score;
	  pos1++, pos2++;
	}
      else if (Key1 > Key2) pos1++;
      else                  pos2++;
    }
  TotalEntries = current;
#if 0
  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
#endif
  Sort         = ByKey;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;

  return this;
}

void atomicIRSET::GC()
{
//cerr << "IRSET::GC()" << endl;
  if (TotalEntries < 1000 && ( (2*MaxEntries) > (5*TotalEntries)) )
    Resize(TotalEntries) ;
  else for (size_t i=TotalEntries; i < MaxEntries; i++)
    Table[i].Clear();
}

// Limit is experimental!
  
OPOBJ *atomicIRSET::And (const OPOBJ& OtherIrset)
{
  return And(OtherIrset, 0); // No limit
}


#if 1

OPOBJ *atomicIRSET::And (const OPOBJ& OtherIrset, size_t Limit)
{
  HitTotal = 0;

  if (Parent && Parent != OtherIrset.GetParent())
    {
      Parent->SetErrorCode(0); // No error 
      logf (LOG_DEBUG, "And between different physical indexes. Should use join instead");
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }

  if (OtherIrset.GetSort() != ByIndex)
    return _And(OtherIrset, Limit);
  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0 || TotalEntries == 0)
    {
      // Nothing 
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }
  // Merge both result sets into one sorted...
  SortByIndex();
  _index_id_t idx1=0, idx2=0;
  size_t      pos1 = 1, opos1 = 0;
  size_t      pos2 = 1, opos2 = 0;
  size_t      current = 0;

  DOUBLE   newMinScore = MAXFLOAT;
  DOUBLE   newMaxScore = 0.0;

  IRESULT *OtherTable = ((const atomicIRSET *)&OtherIrset)->Table;
  IRESULT *IresultPtr = NULL;
  IRESULT *OtherIresultPtr = NULL;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal)
    {
      if (pos1 != opos1)
	{
	  opos1 = pos1;
	  IresultPtr = Table + pos1 - 1;
	  idx1 = IresultPtr->GetIndex();
	}
      if (pos2 != opos2)
	{
	  opos2 = pos2;
	  OtherIresultPtr = OtherTable + pos2 - 1;
	  idx2 = OtherIresultPtr->GetIndex();
	}
      if (idx2 == idx1)
	{
	  if (current != pos1) Table[current] = *IresultPtr;
	  Table[current].IncAuxCount( OtherIresultPtr->GetAuxCount () );
	  const UINT hits = Table[current].IncHitCount (OtherIresultPtr->GetHitCount ());
	  Table[current].AddToHitTable (OtherIresultPtr->HitTable);
          Table[current].SetVectorTermHits ( hits );
	  const DOUBLE score = Table[current++].IncScore (OtherIresultPtr->GetScore ());
	  if (score < newMinScore) newMinScore = score;
	  if (score > newMaxScore) newMaxScore = score;
	  if (Limit > 0 && current >= Limit)
	    break; // Enough
	}
      if (idx1 <= idx2)
	pos1++;
      if (idx2 <= idx1)
	pos2++;
    }
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;

  GC(); // Collect garbage

  return this;
}

#else


OPOBJ *atomicIRSET::And (const OPOBJ& OtherIrset, size_t Limit)
{
  HitTotal = 0;

#if 1
  if (Parent && Parent != OtherIrset.GetParent())
    {
      Parent->SetErrorCode(0); // No error 
      logf (LOG_DEBUG, "And between different physical indexes. Should use join instead");
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }
#endif

  if (OtherIrset.GetSort() != ByIndex)
    return _And(OtherIrset, Limit);
  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0 || TotalEntries == 0)
    {
      // Nothing 
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }
  // Merge both result sets into one sorted...
  SortByIndex();
  _index_id_t idx1=0, idx2=0;
  size_t      pos1 = 1, opos1 = 0;
  size_t      pos2 = 1, opos2 = 0;
  size_t      newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
  if (Limit > 0 && newMaxEntries > Limit)
    newMaxEntries = Limit;

  PIRESULT    NewTable = NULL;

//cerr << "AND::Allocate..." << endl;

  try {
     NewTable = new IRESULT[newMaxEntries];
  } catch (....) {
    if (errno == 0) errno = ENOMEM;
    logf (LOG_ERRNO, "IRSET: Could not allocate space for AND (%ld IRESULTs)!", (long)newMaxEntries);
    if (Parent) Parent->SetErrorCode(2); // Temp Error
    Clear();
    return NULL; // ERROR
  }

  size_t      current = 0;

  DOUBLE   newMinScore = MAXFLOAT;
  DOUBLE   newMaxScore = 0.0;

  IRESULT *OtherTable = ((const atomicIRSET *)&OtherIrset)->Table;
  IRESULT *IresultPtr = NULL;
  IRESULT *OtherIresultPtr = NULL;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal)
    {
      if (pos1 != opos1)
	{
	  opos1 = pos1;
	  IresultPtr = Table + pos1 - 1;
	  idx1 = IresultPtr->GetIndex();
	}
      if (pos2 != opos2)
	{
	  opos2 = pos2;
	  OtherIresultPtr = OtherTable + pos2 - 1;
	  idx2 = OtherIresultPtr->GetIndex();
	}
      if (idx2 == idx1)
	{
	  NewTable[current] = *IresultPtr;
	  NewTable[current].IncAuxCount( OtherIresultPtr->GetAuxCount () );
	  const UINT hits = NewTable[current].IncHitCount (OtherIresultPtr->GetHitCount ());
	  NewTable[current].AddToHitTable (OtherIresultPtr->HitTable);
	  NewTable[current].SetVectorTermHits ( hits );
	  const DOUBLE score = NewTable[current++].IncScore (OtherIresultPtr->GetScore ());
	  if (score < newMinScore) newMinScore = score;
	  if (score > newMaxScore) newMaxScore = score;
	  if (current >= Limit) break;
	}
      if (idx1 <= idx2)
	pos1++;
      if (idx2 <= idx1)
	pos2++;
    }
//cerr << "Delete old table";
  if (Table) delete [] Table;
//cerr << "OK" << endl;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;

  return this;
}
#endif

// Faster AND implementation added by Glenn MacStravic
OPOBJ *atomicIRSET::_And (const OPOBJ& OtherIrset, size_t Limit)
{
  const size_t OtherTotal = OtherIrset.GetTotalEntries();

  if (TotalEntries == 0 || OtherTotal == 0)
    {
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }

  IRESULT OtherIresult;
  atomicIRSET MyResult(Parent);

  SortByIndex();
  size_t count = 0;

  size_t offset = 0;
  const atomicIRSET  *OtherPtr = (atomicIRSET *)&OtherIrset;
  GDT_BOOLEAN   OtherSorted = OtherPtr->Sort == ByIndex;
  for (size_t x=1; x <= OtherTotal &&  offset < TotalEntries; x++)
    {
      OtherIrset.GetEntry(x, &OtherIresult);
      // NOTE: With the bsearch can increment the start point since Index is also sorted!
      IRESULT *match = (IRESULT*)bsearch((const void *)&OtherIresult,
                (const void *)(Table + offset), TotalEntries - offset, sizeof(IRESULT), IrsetIndexCompare);
      if (match != NULL)
        {
          if (OtherSorted)
	    offset = match - Table;
	  match->IncAuxCount(OtherIresult.GetAuxCount());
          match->AddToHitTable(OtherIresult);
          const UINT hits = match->IncHitCount(OtherIresult.GetHitCount());
	  match->SetVectorTermHits ( hits );
          const DOUBLE score = match->IncScore(OtherIresult.GetScore());
	  if (score < MinScore) MinScore = score;
	  if (score > MaxScore) MaxScore = score;
          MyResult.FastAddEntry(*match);
          count++;
	  if (Limit > 0 && count >= Limit)
	    break; // Enough
        }
    }
#if AWWW
/*
  // Don't think that this is needed..
  if (count)
    MyResult.MergeEntries(GDT_FALSE);
*/
#endif
//cerr << "_AND::DELETE Table... ";
  if (Table) delete [] Table;
//cerr << "OK" << endl;
  TotalEntries=count;
  MaxEntries=MyResult.MaxEntries;
//cerr << "Steal Table" << endl;
  Table = MyResult.StealTable();
//cerr << "OK" << endl;
  return this;
}


OPOBJ *atomicIRSET::Peer (const OPOBJ& Irset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return Peer(Irset, NULL);
  return And(Irset);
}
OPOBJ *atomicIRSET::BeforePeer (const OPOBJ& Irset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return Peer(Irset, _compGTE);
  return Before(Irset);
}
OPOBJ *atomicIRSET::AfterPeer (const OPOBJ& Irset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return Peer(Irset, _compLTE);
  return After(Irset);
}


OPOBJ *atomicIRSET::Peer (const OPOBJ& Irset, peer_t compFunc)
{
#if 1
  if (Parent && Parent != Irset.GetParent())
    {
      Parent->SetErrorCode(0); // No error
      logf (LOG_DEBUG, "Peer between different physical indexes is not defined.");
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this;
    }
#endif

  atomicIRSET OtherIrset (Irset);

  if (OtherIrset.GetSort() != ByIndex)
    OtherIrset.SortByIndex();
  const size_t    OtherTotal = OtherIrset.GetTotalEntries();

  if (OtherTotal == 0 || TotalEntries == 0) {
    // Nothing
    MinScore = MAXFLOAT;
    MaxScore = 0.0;
    TotalEntries = 0;
    HitTotal     = 0;
    return this;		// Can't do proximity with nothing..
  }
  // Merge both result sets into one sorted...
  SortByIndex();

  _index_id_t     idx1=0, idx2=0;
  size_t          pos1 = 1, opos1 = 0;
  size_t          pos2 = 1, opos2 = 0;
  size_t          newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
/// @@@@ 
  PIRESULT        NewTable = new IRESULT[newMaxEntries];
  size_t          current = 0;

  DOUBLE          newMinScore = MAXFLOAT;
  DOUBLE          newMaxScore = 0.0;

  IRESULT        *OtherTable = ((const atomicIRSET *) &OtherIrset)->Table;
  IRESULT        *IresultPtr = NULL;
  IRESULT        *OtherIresultPtr = NULL;
  FCT             newHitTable;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal) {
    if (pos1 != opos1) {
      opos1 = pos1;
      IresultPtr = Table + pos1 - 1;
      idx1 = IresultPtr->GetIndex();
    }
    if (pos2 != opos2) {
      opos2 = pos2;
      OtherIresultPtr = OtherTable + pos2 - 1;
      idx2 = OtherIresultPtr->GetIndex();
    }
    if (idx2 == idx1) {
      const FCLIST   *MyHitPtr = (const FCLIST *) IresultPtr->GetHitTable();
      const FCLIST   *OtherHitPtr = (const FCLIST *) OtherIresultPtr->GetHitTable();

//      const GDT_BOOLEAN IsSorted = IresultPtr->HitTableIsSorted() && OtherIresultPtr->HitTableIsSorted();

      size_t          matchCount = 0;

      newHitTable.Clear();
      FC               oldFc;
      FC               oldMyFc;

      for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next()) {
	const FC        MyFc(kp->Value());

	FC PeerFC   = Parent->GetPeerFc (MyFc);

	for (const FCLIST * p = OtherHitPtr->Next(); p != OtherHitPtr; p = p->Next()) {
	  const FC fc (p->Value());
	  if ( (compFunc == NULL || compFunc(MyFc, fc)) && PeerFC.Contains( fc ) ) {
	    matchCount++;
	    if (oldFc   != fc) {
	      if ((oldFc = fc) != oldMyFc) newHitTable.AddEntry( fc );
	    }
	    if (oldMyFc != MyFc) {
	      if ((oldMyFc = MyFc) != fc) newHitTable.AddEntry( MyFc );
	    }
	  }
	}			// for
      }				// for
      if (matchCount)
	{
	  NewTable[current] = *IresultPtr;

          // Note: the table might have be sorted...
	  newHitTable.MergeEntries(); // Need to do this

	  NewTable[current].SetHitTable (newHitTable);
	  NewTable[current].SetHitCount (matchCount);
          NewTable[current].SetAuxCount (2);
          const DOUBLE score = NewTable[current++].GetScore();
          if (score < newMinScore) newMinScore = score;
          if (score > newMaxScore) newMaxScore = score;
	}
    }
    if (idx1 <= idx2)
      pos1++;
    if (idx2 <= idx1)
      pos2++;
  }

  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;
  HitTotal     = 0;
  return this;
}

// In the intersection (AND) but not a peer
//
OPOBJ *atomicIRSET::XPeer (const OPOBJ& Irset)
{
  atomicIRSET OtherIrset (Irset);
  if (OtherIrset.GetSort() != ByIndex)
    OtherIrset.SortByIndex();
  const size_t    OtherTotal = OtherIrset.GetTotalEntries();

  if (OtherTotal == 0 || TotalEntries == 0) {
    // Nothing
    MinScore = MAXFLOAT;
    MaxScore = 0.0;
    TotalEntries = 0;
    HitTotal     = 0;
    return this;		// Can't do proximity with nothing..
  }
  // Merge both result sets into one sorted...
  SortByIndex();

  _index_id_t     idx1=0, idx2=0;
  size_t          pos1 = 1, opos1 = 0;
  size_t          pos2 = 1, opos2 = 0;
  size_t          newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
// @@@@
  PIRESULT        NewTable = new IRESULT[newMaxEntries];
  size_t          current = 0;

  DOUBLE          newMinScore = MAXFLOAT;
  DOUBLE          newMaxScore = 0.0;

  IRESULT        *OtherTable = ((const atomicIRSET *) &OtherIrset)->Table;
  IRESULT        *IresultPtr = NULL;
  IRESULT        *OtherIresultPtr = NULL;
  FCT             newHitTable;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal) {
    if (pos1 != opos1) {
      opos1 = pos1;
      IresultPtr = Table + pos1 - 1;
      idx1 = IresultPtr->GetIndex();
    }
    if (pos2 != opos2) {
      opos2 = pos2;
      OtherIresultPtr = OtherTable + pos2 - 1;
      idx2 = OtherIresultPtr->GetIndex();
    }
    if (idx2 == idx1) {
      const FCLIST   *MyHitPtr = (const FCLIST *) IresultPtr->GetHitTable();
      const FCLIST   *OtherHitPtr = (const FCLIST *) OtherIresultPtr->GetHitTable();
//      const GDT_BOOLEAN IsSorted = IresultPtr->HitTableIsSorted() && OtherIresultPtr->HitTableIsSorted();

      size_t          matchCount = 0;

      newHitTable.Clear();
      FC              oldFc;
      FC              oldMyFc;
      for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next()) {
	const FC        MyFc(kp->Value());

	FC PeerFC   = Parent->GetPeerFc (MyFc);

	for (const FCLIST * p = OtherHitPtr->Next(); p != OtherHitPtr; p = p->Next()) {
	  if ( ! PeerFC.Contains( p->Value() ) ) {
	    matchCount++;
	    if (oldFc != p->Value()) newHitTable.AddEntry(oldFc = p->Value() );
	    if (oldMyFc != MyFc) {
	      if ((oldMyFc = MyFc) != p->Value()) newHitTable.AddEntry( MyFc );
	    }
	  }
	}			// for
      }				// for
      if (matchCount)
	{
	  NewTable[current] = *IresultPtr;

	  // Note: the table might have be sorted...
	  newHitTable.MergeEntries(); // Need to do this

	  NewTable[current].SetHitTable (newHitTable);
	  NewTable[current].SetHitCount (matchCount);
          NewTable[current].SetAuxCount (2);
          const DOUBLE score = NewTable[current++].GetScore();
          if (score < newMinScore) newMinScore = score;
          if (score > newMaxScore) newMaxScore = score;
	}
    }
    if (idx1 <= idx2)
      pos1++;
    if (idx2 <= idx1)
      pos2++;
  }

  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;
  HitTotal     = 0;
  return this;
}


// Private
GDT_BOOLEAN atomicIRSET::FieldExists(const STRING& FieldName)
{
  if (Parent == NULL) logf (LOG_ERROR, "Orphaned IRSET ????");
  else if (Parent->DfdtGetFileName (FieldName, NULL) == GDT_FALSE)
    logf (LOG_DEBUG, "Within non-existant field '%s'", FieldName.c_str());
  else if (Parent->GetFieldCache()->SetFieldName(FieldName, GDT_FALSE) == GDT_FALSE)
    logf (LOG_DEBUG, "Can't load field data for %s", FieldName.c_str());
  else
    return GDT_TRUE; // Exists
  return GDT_FALSE;
}


//
// Means that some of what I have are within the field
//
OPOBJ *atomicIRSET::Within(const OPOBJ& Irset, const STRING& FieldName)
{
  return Within(Irset, FieldName, NULL);
}
OPOBJ *atomicIRSET::BeforeWithin (const OPOBJ& Irset, const STRING& FieldName)
{
  return Within(Irset, FieldName, _compGTE);
}
OPOBJ *atomicIRSET::AfterWithin (const OPOBJ& Irset, const STRING& FieldName)
{
  return Within(Irset, FieldName, _compLTE);
}

#if 0
// Special
OPOBJ *atomicIRSET::AndWithinFile(const STRING& FileSpec)
{
  MDTREC             mdtrec;
  const GDT_BOOLEAN  isWild = FileSpec.IsWild();
  const GDT_BOOLEAN  noPath = !ContainsPathSep(FileSpec);
  const PATHNAME     myPath (FileSpec);
  DOUBLE             score;
  size_t             current = 0;

  MinScore=MAXFLOAT;
  MaxScore=0.0;
  for (size_t i=1; i<= TotalEntries; i++)
    {
      if (GetMdtrec(i, &mdtrec))
        {
          PATHNAME path (mdtrec.GetPathname());

          if (path.Compare(myPath) == 0 ||
              (noPath && ::FileGlob(FileSpec, path.GetFileName())) ||
              (isWild && ::FileGlob(FileSpec, path.GetFullFileName())) )
            {
	      if ((score = (Table[current++] = Table[i]).GetScore() < MinScore)
		MinScore = score;
	      else if (score > MaxScore)
		MaxScore = score;
            }
        }
    }
  TotalEntries = current;
  return this;
}
#endif

// Special
OPOBJ *atomicIRSET::WithinFile(const STRING& FileSpec)
{
#if 1
  return  Parent ? this->And(*(OPOBJ *)Parent->FileSearch(FileSpec)) : NULL;
#else
  if (Parent)  return Parent->FileSearch(FileSpec);
  return this;
#endif
}

// Special
OPOBJ *atomicIRSET::WithKey(const STRING& KeySpec)
{
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#if 1
  return  Parent ? this->And(*(OPOBJ *)Parent->KeySearch(KeySpec)) : NULL;
#else
  return  Parent ? (OPOBJ *)Parent->KeySearch(KeySpec) : NULL;
#endif
}

// Special
OPOBJ *atomicIRSET::WithinDoctype(const STRING& KeySpec)
{
  return  Parent ? (OPOBJ *)Parent->DoctypeSearch(KeySpec) : NULL;
}

OPOBJ *atomicIRSET::Inside(const STRING& FieldName)
{
  if (Parent) Parent->SetErrorCode(110); // "Operator unsupported";
  return this;

}

OPOBJ *atomicIRSET::Sibling ( )
{
  if (Parent) Parent->SetErrorCode(110); // "Operator unsupported";
  return this;
/*
  Implementation strategy:

  Go through each IRESULT hit set. Reduce the number of hits in the set to
  only those that have more than one hit in the same container.

  Recalculate score and re-sort.
*/

}


//
// NOTE: Idea: Support FieldName globing?
//

OPOBJ *atomicIRSET::Within(const OPOBJ& Irset, const STRING& FieldName,  peer_t compFunc)
{
  if (Irset.IsEmpty() || IsEmpty())
    {
      MinScore = MAXFLOAT;
      MaxScore = 0.0;
      TotalEntries = 0;
      HitTotal     = 0;
      return this;                // Can't do proximity with nothing..
    }


  if (FieldName == "*") return Peer(Irset, compFunc);

#if 1
  if (Parent && Parent != Irset.GetParent())
    {
      Parent->SetErrorCode(0); // No error
      logf (LOG_DEBUG, "WITHIN:%s between different physical indexes is not defined.", FieldName.c_str());
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this;
    }
#endif

#ifdef FIELD_WILD_MATCH
  STRING    pattern(FieldName);

  if (!pattern.IsPlain() && Parent)
    {
      DFDT *dfdtp = Parent->GetDfdt();
      int   count = 0;
      if (dfdtp)
        {
          const size_t total = dfdtp->GetTotalEntries();
          STRING       field, fld;
	  atomicIRSET  input(Parent);

	  //pattern.ToUpper(); // Field names are upper case
	  //pattern.Replace("\\","/");
          for (size_t i=0; i<total; i++)
            {
	      fld = field = dfdtp->GetFieldName(i+1);
	      //fld.Replace("\\", "/");
              if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, fld))
                {
		  if (count++ == 0) {
		     input.Cat(*this);
		     Within(Irset, field, compFunc);
		  } else {
		    atomicIRSET  Set (Parent);
		    Set.Cat (input);
		    Set.Within(Irset, field, compFunc);
		    Or(Set);
		  }
                }
            }
	  if (count > 1) {
	    for (size_t i=0; i < TotalEntries; i++)
	      {
		FCLIST list;
		Table[i].GetHitTable(&list);
		list.MergeEntries();
		Table[i].SetHitTable(list);
	      }
	  }

        }
      if (count == 0)
	Clear();

      return this;
    }
#endif
  // Can save some effort
  if (FieldExists(FieldName) == GDT_FALSE)
    {
      Clear();
      return this;
    }

  atomicIRSET OtherIrset (Irset);

  if (OtherIrset.GetSort() != ByIndex)
    OtherIrset.SortByIndex();
  const size_t    OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0 || TotalEntries == 0) {
    // Nothing
    MinScore = MAXFLOAT;
    MaxScore = 0.0;
    TotalEntries = 0;
    HitTotal     = 0;
    return this;		// Can't do proximity with nothing..
  }
  // Merge both result sets into one sorted...
  SortByIndex();

  _index_id_t     idx1=0, idx2=0;
  size_t          pos1 = 1, opos1 = 0;
  size_t          pos2 = 1, opos2 = 0;
  size_t          newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
// @@@@
  PIRESULT        NewTable = new IRESULT[newMaxEntries];
  size_t          current = 0;

  DOUBLE          newMinScore = MAXFLOAT;
  DOUBLE          newMaxScore = 0.0;

  IRESULT        *OtherTable = ((const atomicIRSET *) &OtherIrset)->Table;
  IRESULT        *IresultPtr = NULL;
  IRESULT        *OtherIresultPtr = NULL;
  FCT             newHitTable;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal) {
    if (pos1 != opos1) {
      opos1 = pos1;
      IresultPtr = Table + pos1 - 1;
      idx1 = IresultPtr->GetIndex();
    }
    if (pos2 != opos2) {
      opos2 = pos2;
      OtherIresultPtr = OtherTable + pos2 - 1;
      idx2 = OtherIresultPtr->GetIndex();
    }
    if (idx2 == idx1) {
      const FCLIST   *MyHitPtr = (const FCLIST *) IresultPtr->GetHitTable();
      const FCLIST   *OtherHitPtr = (const FCLIST *) OtherIresultPtr->GetHitTable();
//      const GDT_BOOLEAN IsSorted = IresultPtr->HitTableIsSorted() && OtherIresultPtr->HitTableIsSorted();

      size_t          matchCount = 0;

      newHitTable.Clear();

      FC              oldMyFc;
      FC              oldFc;
      for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next()) {
	const FC        MyFc(kp->Value());
	for (const FCLIST * p = OtherHitPtr->Next(); p != OtherHitPtr; p = p->Next()) {
	  FC              Fc(p->Value());

	  // If we have a compFunc and don't fit then we don't need to look more
          if (compFunc && !compFunc(MyFc, Fc)) continue; // This is the comparison!

	  if (MyFc.GetFieldStart() > Fc.GetFieldStart())
	    Fc.SetFieldEnd(MyFc.GetFieldEnd());
	  else
	    Fc.SetFieldStart(MyFc.GetFieldStart());

	  // We only want those in the named field
	  if ( Parent->GetFieldCache()->ValidateInField(Fc, FieldName )) {
	    matchCount++;
	    if (oldFc != p->Value()) {
	      if ((oldFc = p->Value()) != oldMyFc) newHitTable.AddEntry( oldFc );
	    }
	    if (oldMyFc != MyFc) {
	      if ( (oldMyFc = MyFc) != p->Value()) newHitTable.AddEntry( MyFc);
	    }
	  }
	}			// for
      }				// for
      if (matchCount)
	{
	  NewTable[current] = *IresultPtr;

	  // Note: the table might have be sorted...
	  newHitTable.MergeEntries(); // Need to do this

	  NewTable[current].SetHitTable (newHitTable);
	  NewTable[current].SetHitCount (matchCount);
          NewTable[current].SetAuxCount (2);
          const DOUBLE score = NewTable[current++].GetScore();
          if (score < newMinScore) newMinScore = score;
          if (score > newMaxScore) newMaxScore = score;
	}
    }
    if (idx1 <= idx2)
      pos1++;
    if (idx2 <= idx1)
      pos2++;
  }

  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;
  HitTotal     = 0;
  return this;
}

OPOBJ *atomicIRSET::Within(const DATERANGE& Daterange)
{
  if (!Daterange.Ok())
   return this;

  HitTotal = 0;
#if 1
  size_t count = 0;

  for (size_t i=0; i < TotalEntries; i++)
    {
      if (Daterange.Contains( Table[i].GetDate() ))
	{
	  if (count != i)
	     Table[count] = Table[i];
	  count++;
	}
      else Table[i].ClearHitTable(); // Optional code to reduce footprint of IRSET in memory
    }
      
   if (count != TotalEntries)
    {
      const size_t shrunkBy = TotalEntries - count;
      TotalEntries=count;

      // Regain some memory?  OPTIONAL CODE!
      if (shrunkBy > (3*count/2) && count < 1000 && shrunkBy > 500)
	CleanUp();

      if (ComputedS != NoNormalization)
        {
          enum NormalizationMethods newComputedS = ComputedS;
          ComputedS = NoNormalization;
          ComputeScores (1, newComputedS);
        }
     }

#else
  atomicIRSET MyResult(Parent);

  size_t misses = 0;

  for (size_t x=0; x < TotalEntries; x++)
    {
      if (Daterange.Contains( Table[x].GetDate() ))
	MyResult.FastAddEntry( Table[x] ));
      else
	misses++;
    }

   if (misses)
    {
      TotalEntries=MyResult.TotalEntries;
      MaxEntries=MyResult.MaxEntries;
      Table = MyResult.StealTable();

      if (ComputedS != NoNormalization)
	{
	  enum NormalizationMethods newComputedS = ComputedS;
	  ComputedS = NoNormalization;
	  ComputeScores (1, newComputedS);
	}
     }
#endif
  return this;
}

OPOBJ *atomicIRSET::Within(const char *x)
{
  if (x == NULL || *x== '\0') return this;
  return Within ((STRING)x);
}


OPOBJ *atomicIRSET::Within(const STRING& FieldName)
{
  if (FieldName == "*") return this;
#ifdef FIELD_WILD_MATCH
  STRING pattern(FieldName);
                  
  if (!pattern.IsPlain() && Parent)
    {
      DFDT *dfdtp = Parent->GetDfdt();
      int   count = 0;
      if (dfdtp)
        {
          const size_t total = dfdtp->GetTotalEntries();
          STRING       field, fld;
	  atomicIRSET  input(Parent);

	  //pattern.ToUpper(); // Field names are uppercase
	  //pattern.Replace("\\","/");
          for (size_t i=0; i<total; i++)
            {
	      field = dfdtp->GetFieldName(i+1);
	      fld = field;
	      //fld.Replace("\\", "/");
              if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, fld))
                {
		  if (count++ == 0) {
		     input.Cat(*this);
		     Within(field);
		  } else {
		    atomicIRSET  Set (Parent);
		    Set.Cat (input);
		    Set.Within(field);
		    Or(Set);
		  }
                }
            }

	  if (count > 1) // Merge
	    for (size_t i=0; i < TotalEntries; i++)
              {
                FCLIST list;
                Table[i].GetHitTable(&list);
                list.MergeEntries();
                Table[i].SetHitTable(list);
              }
        }
      if (count == 0)
	Clear();
      return this;
    }
#endif
  // Can save some effort
  if (FieldExists(FieldName) == GDT_FALSE)
    {
      DATERANGE Range (FieldName);

      if (Range.Ok())
	return Within(Range);
      Clear();
      return this;
    }

  atomicIRSET MyResult(Parent);
  size_t count  = 0;
  GDT_BOOLEAN Changed = GDT_FALSE;

  if (Parent)
    {
      unsigned hits, misses;
      FC myFC;
      IRESULT iresult;
      for (size_t x=1; x <= TotalEntries; x++)
	{
	  GetEntry(x, &iresult);
	  const FCLIST *MyHitPtr    = (const FCLIST *)iresult.GetHitTable ();
	  iresult.ClearHitTable();
	  hits = 0;
	  misses = 0;
	  for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next())
	    {
	      if (Parent->GetFieldCache()->ValidateInField(  myFC = kp->Value(), FieldName ))
		{
		  iresult.AddToHitTable(myFC);
		  hits++;
		}
	       else
		misses++;
	    }
	  if (hits)
	    {
	      // Have something
	      MyResult.FastAddEntry(iresult);
	      count++;
	    }
	  if (misses) Changed = GDT_TRUE;
	}
    }
  else
   Changed = GDT_TRUE; // Changed to nothing;

  if (Changed)
    {
      if (Table) delete [] Table;
      TotalEntries=count;
      MaxEntries=MyResult.MaxEntries;
      Table = MyResult.StealTable();
      HitTotal = 0;

      if (ComputedS != NoNormalization)
	{
	  enum NormalizationMethods newComputedS = ComputedS;
	  ComputedS = NoNormalization;
	  ComputeScores (1, newComputedS);
	}
    }
  return this;
}


OPOBJ *atomicIRSET::XWithin(const STRING& FieldName)
{
  if (FieldName == "*") {
    if (Parent)
      {
	DFDT *dfdtp = Parent->GetDfdt();
	if (dfdtp && dfdtp->GetTotalEntries() > 0)
	  Clear();
      }
    return this;
  }
#ifdef FIELD_WILD_MATCH
  STRING pattern(FieldName);

  if (!pattern.IsPlain() && Parent)
    {
      DFDT *dfdtp = Parent->GetDfdt();
      int   count = 0;
      if (dfdtp)
        {
          const size_t total = dfdtp->GetTotalEntries();
          STRING       field, fld;
	  atomicIRSET  input(Parent);

	  //pattern.ToUpper();
	  //pattern.Replace("\\","/");
          for (size_t i=0; i<total; i++)
            {
	      fld = field = dfdtp->GetFieldName(i+1);
	      //fld.Replace("\\", "/");
              if (field.GetLength() && FIELD_WILD_MATCH(pattern, fld))
                {
		  if (count++ == 0) {
		     input.Cat(*this);
		     XWithin(field);
		  } else {
		    atomicIRSET  Set (Parent);
		    Set.Cat (input);
		    Set.XWithin(field);
		    Or(Set);
		  }
                }
            }
        }
      if (count == 0)
	Clear();
      return this;
    }
#endif
  // Can save some effort
  if (FieldExists(FieldName) == GDT_FALSE)
    return this; // XWithin a non-existent field should be everything..

  atomicIRSET MyResult(Parent);
  size_t count  = 0;
  GDT_BOOLEAN Changed = GDT_FALSE;

  if (Parent)
    {
      unsigned hits;
      FC myFC;
      IRESULT iresult;
      for (size_t x=1; x <= TotalEntries; x++)
	{
	  hits = 0;
	  GetEntry(x, &iresult);
	  const FCLIST *MyHitPtr    = (const FCLIST *)iresult.GetHitTable ();
	  if (MyHitPtr)
	    for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next()) {
	      if (Parent->GetFieldCache()->ValidateInField(  myFC = kp->Value(), FieldName ))
		{
		  hits++;
		  break;
		}
	    } // for
	  if (hits == 0)
	    {
	      // Have something
	      MyResult.FastAddEntry(iresult);
	      count++;
	    }
	  else Changed = GDT_TRUE;
	}
    }

  if (Changed)
    {
      if (Table) delete [] Table;
      TotalEntries=count;
      MaxEntries=MyResult.MaxEntries;
      Table = MyResult.StealTable();
      HitTotal = 0;

      if (ComputedS != NoNormalization)
	{
	  enum NormalizationMethods newComputedS = ComputedS;
	  ComputedS = NoNormalization;
	  ComputeScores (1, newComputedS);
	}
    }
  return this;
}

//
// Means that ALL my hits are inside a given field 
//
OPOBJ *atomicIRSET::Inclusive(const STRING& FieldName)
{
  if (FieldName == "*") {
    if (Parent)
      {
	DFDT *dfdtp = Parent->GetDfdt();
	if (dfdtp && dfdtp->GetTotalEntries() > 0)
	  Clear();
      }
    return this;
  }
#ifdef FIELD_WILD_MATCH
  STRING    pattern(FieldName);

  if (!FieldName.IsPlain() && Parent)
    {
      DFDT *dfdtp = Parent->GetDfdt();
      int   count = 0;
      if (dfdtp)
        {
          const size_t total = dfdtp->GetTotalEntries();
          STRING       field, fld;
	  atomicIRSET  input(Parent);

	  //pattern.ToUpper();
	  //pattern.Replace("\\","/");
          for (size_t i=0; i<total; i++)
            {
	      fld = field = dfdtp->GetFieldName(i+1);
	      //fld.Replace("\\", "/");
              if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, fld))
                {
		  if (count++ == 0) {
		     input.Cat(*this);
		     Inclusive(field);
		  } else {
		    atomicIRSET  Set (Parent);
		    Set.Cat (input);
		    Set.Inclusive(field);
		    Or(Set);
		  }
                }
            }
        }
      if (count == 0)
	Clear();
      return this;
    }
#endif
  // Can save some effort
  if (FieldExists(FieldName) == GDT_FALSE)
    {
      Clear();
      return this;
    }

  atomicIRSET MyResult(Parent);
  size_t count  = 0;
  GDT_BOOLEAN Changed = GDT_FALSE;

  if (Parent)
    {
      unsigned hits, misses;
      FC myFC;
      IRESULT iresult;
      for (size_t x=1; x <= TotalEntries; x++)
	{
	  GetEntry(x, &iresult);
	  const FCLIST *MyHitPtr    = (const FCLIST *)iresult.GetHitTable ();
	  iresult.ClearHitTable();
	  hits = 0;
	  misses = 0;
	  for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next())
	    {
	      if (Parent->GetFieldCache()->ValidateInField(  myFC = kp->Value(), FieldName ))
		{
		  iresult.AddToHitTable(myFC);
		  hits++;
		}
	       else
		misses++;
	    }
	  if (hits && misses == 0)
	    {
	      // Have something
	      MyResult.FastAddEntry(iresult);
	      count++;
	    }
	  if (misses) Changed = GDT_TRUE;
	}
    }
  else
   Changed = GDT_TRUE; // Changed to nothing;

  if (Changed)
    {
      if (Table) delete [] Table;
      TotalEntries=count;
      MaxEntries=MyResult.MaxEntries;
      Table = MyResult.StealTable();

      if (ComputedS != NoNormalization)
	{
	  enum NormalizationMethods newComputedS = ComputedS;
	  ComputedS = NoNormalization;
	  ComputeScores (1, newComputedS);
	}
    }
  return this;
}


OPOBJ *atomicIRSET::AndNot (const OPOBJ& OtherIrset)
{
  if (TotalEntries == 0)
    {
      *this = OtherIrset;
      return this; // Can't take away from nothing
    }
  if (OtherIrset.GetSort() != ByIndex)
    return _AndNot (OtherIrset);

  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0)
    {
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Nothing 
    }

  SortByIndex();
  IRESULT  OtherIresult, Iresult;
  _index_id_t    idx1=0, idx2=0;
  size_t   pos1 = 1, opos1 = 0;
  size_t   pos2 = 1, opos2 = 0;
  size_t   newMaxEntries = OtherTotal;
// @@@@
  PIRESULT NewTable = new IRESULT[newMaxEntries];
  size_t   current = 0;

  DOUBLE newMinScore=MAXFLOAT;
  DOUBLE newMaxScore=0.0;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal)
    {
      if (pos1 != opos1)
	{
	  GetEntry (opos1 = pos1, &Iresult);
	  idx1 = Iresult.GetIndex();
	}
      if (pos2 != opos2)
	{
	  OtherIrset.GetEntry (opos2 = pos2, &OtherIresult);
	  idx2 = OtherIresult.GetIndex();
	}
      if (idx2 == idx1)
	{
	  // Nope... Don't want it..
	  pos1++, pos2++;
	}
      else if (idx2 < idx1)
	{
	  const DOUBLE score = (NewTable[current++] = OtherIresult).GetScore();
          if (score < newMinScore) newMinScore = score;
          if (score > newMaxScore) newMaxScore = score;
	  pos2++;
	}
      else // idx1 < idx2
	{
	  pos1++;
	}
    }
  // Anything left?
  while (pos2 <= OtherTotal)
    {
      OtherIrset.GetEntry(pos2++, &OtherIresult);
      const DOUBLE score = (NewTable[current++] = OtherIresult).GetScore();
      if (score < newMinScore) newMinScore = score;
      if (score > newMaxScore) newMaxScore = score;
    }
  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;
  return this;
}

// AndNOT added by Glenn MacStravic
OPOBJ *atomicIRSET::_AndNot (const OPOBJ& OtherIrset)
{
  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0)
    {
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Nothing
    }
  else if (TotalEntries == 0)
    {
      *this = OtherIrset;
      return this;
    }
  IRESULT OtherIresult;
  atomicIRSET MyResult(Parent);
  const GDT_BOOLEAN OtherSorted = (((atomicIRSET *) &OtherIrset)->Sort == ByIndex);
  IRESULT *ptr = NULL;

  SortByIndex();

  size_t count = 0;
  size_t offset = 0;
  for (size_t x = 1; x <= OtherTotal; x++)
    {
      OtherIrset.GetEntry(x, &OtherIresult);
      if (TotalEntries == 0 || (ptr = (IRESULT *)bsearch(&OtherIresult, (const void *)(Table+offset),
		TotalEntries-offset, sizeof(IRESULT), IrsetIndexCompare)) == NULL)
	{
	  MyResult.FastAddEntry(OtherIresult /*, GDT_FALSE */);
	  count++;
	}
      else if (TotalEntries && OtherSorted)
	{
	  if ((offset = ptr - Table) == TotalEntries)
	    TotalEntries = 0; // Grab the rest...
	}
    }
  if (Table) delete [] Table;
  TotalEntries=count;
  MaxEntries=MyResult.MaxEntries;
  Table = MyResult.StealTable();
  return this;
}

// x y xor -> (x U y) - ( x ^ y)
// author: edz@nonmonotonic.com

OPOBJ *atomicIRSET::Xor (const OPOBJ& OtherIrset)
{
  const size_t OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0)
    return this; // Nothing to add..
  if (TotalEntries == 0)
    {
      // Just grab the others..
      *this = OtherIrset;
      return this;
    }
  if (OtherIrset.GetSort() != ByIndex)
    return _Xor(OtherIrset);

  // Merge both result sets into one sorted...
  SortByIndex();
  IRESULT  OtherIresult, Iresult;
  _index_id_t    idx1=0, idx2=0;
  size_t   pos1 = 1, opos1 = 0;
  size_t   pos2 = 1, opos2 = 0;
  size_t   newMaxEntries = TotalEntries+OtherTotal;
  if (Parent)
    {
      // Keep the size down...
      size_t Cutoff = Parent->GetDbSearchCutoff();
      if (Cutoff > 0 && Cutoff < newMaxEntries)
        newMaxEntries = Cutoff;
    }
// @@@@
  PIRESULT NewTable = new IRESULT[newMaxEntries];
  size_t   current = 0;

  const DOUBLE   OtherMin = OtherIrset.GetMinScore();
  const DOUBLE   OtherMax = OtherIrset.GetMaxScore();
  if (MinScore > OtherMin)
    MinScore = OtherMin;
  if (MaxScore < OtherMax)
    MaxScore = OtherMax;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal && current < newMaxEntries)
    {
      if (pos1 != opos1)
	{
	  GetEntry (opos1 = pos1, &Iresult);
	  idx1 = Iresult.GetIndex();
	}
      if (pos2 != opos2)
	{
	  OtherIrset.GetEntry (opos2 = pos2, &OtherIresult);
	  idx2 = OtherIresult.GetIndex();
	}
      if (idx2 == idx1)
	{
	  // In both so we don't want it!
	  pos1++, pos2++;
	}
      else
	{
	  DOUBLE score;
	  if (idx1 < idx2)
	    {
	      NewTable[current++] = Iresult;
	      score = Iresult.GetScore();
	      pos1++;
	    }
	  else // idx2 < idx1
	    {
	      NewTable[current++] = OtherIresult;
	      score = OtherIresult.GetScore();
	      pos2++;
	    }
	  if (score < MinScore) MinScore = score;
	  if (score > MaxScore) MaxScore = score;
	}
    }
  // Anything left?
  while (pos1 <= TotalEntries && current < newMaxEntries)
    {
      GetEntry(pos1++, &Iresult);
      NewTable[current++] = Iresult;
    }
  // Anything left?
  while (pos2 <= OtherTotal && current < newMaxEntries)
    {
      OtherIrset.GetEntry(pos2++, &OtherIresult);
      NewTable[current++] = OtherIresult;
    }
  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  return this;
}

OPOBJ *atomicIRSET::_Xor (const OPOBJ& OtherIrset)
{
  // not a very fast implementation (linear search)
  const size_t OtherTotal = OtherIrset.GetTotalEntries ();

  if (OtherTotal == 0)
    return this;
  // Special case...
  if (TotalEntries == 0)
    {
      *this = OtherIrset;
      return this; 
    }
// @@@@
  PIRESULT AddTable = new IRESULT[OtherTotal];
  size_t AddCount = 0;
  IRESULT OtherIresult;
  for (size_t y = 1; y <= OtherTotal; y++)
    {
      OtherIrset.GetEntry (y, &OtherIresult);
      GDT_BOOLEAN found = GDT_FALSE;
      const _index_id_t Index = OtherIresult.GetIndex ();
      for (size_t x = 0; x < TotalEntries; x++)
	{
	  found = GDT_FALSE;
	  if (Table[x].GetIndex () == Index)
	    {
	      // Found so remove set
	      found = GDT_TRUE;
	      for (size_t w = x; w < (TotalEntries - 1); w++)
		Table[w] = Table[w + 1];
	      TotalEntries--;
	      break;
	    }
	}			/* for */
      if (found == GDT_FALSE)
	AddTable[AddCount++] = OtherIresult;	// Add this
    }				/* for */

  // Now add what we need
  while (AddCount > 0)
    FastAddEntry (AddTable[--AddCount]);
  delete[]AddTable;
  return this;
}

OPOBJ *atomicIRSET::Nor (const OPOBJ& OtherIrset)
{
  return Or (OtherIrset)->Not();
}

OPOBJ *atomicIRSET::Nand (const OPOBJ& OtherIrset)
{
  return And (OtherIrset)->Not();
}


// If metric < 1 then its % of the size of the document.
OPOBJ *atomicIRSET::CharProx (const OPOBJ& OtherIrset, const float Metric, DIR_T direction)
{
#if 1
  if (Parent && Parent != OtherIrset.GetParent())
    {
      Parent->SetErrorCode(0); // No error
      logf (LOG_DEBUG, "Proximity between different physical indexes is not defined.");
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this;
    }
#endif

  if (Metric == 0)
    return Peer(OtherIrset); // Semantics is distance=0 -> same set


  if (OtherIrset.GetSort() != ByIndex)
    return _CharProx (OtherIrset, Metric, direction);
  const size_t    OtherTotal = OtherIrset.GetTotalEntries();
  if (OtherTotal == 0 || TotalEntries == 0) {
    // Nothing
    MinScore = MAXFLOAT;
    MaxScore = 0.0;
    TotalEntries = 0;
    return this;		// Can't do proximity with nothing..
  }
  // Merge both result sets into one sorted...
  SortByIndex();

  _index_id_t     idx1 =0, idx2=0;
  size_t          pos1 = 1, opos1 = 0;
  size_t          pos2 = 1, opos2 = 0;
  size_t          newMaxEntries = TotalEntries > OtherTotal ? OtherTotal : TotalEntries;
// @@@@
  PIRESULT        NewTable = new IRESULT[newMaxEntries];
  size_t          current = 0;

  DOUBLE          newMinScore = MAXFLOAT;
  DOUBLE          newMaxScore = 0.0;

  IRESULT        *OtherTable = ((const atomicIRSET *) &OtherIrset)->Table;
  IRESULT        *IresultPtr = NULL;
  IRESULT        *OtherIresultPtr = NULL;
  FCT             newHitTable;
  MDTREC          mdtrec;
  INT             Distance;

  while (pos1 <= TotalEntries && pos2 <= OtherTotal) {
    if (pos1 != opos1) {
      opos1 = pos1;
      IresultPtr = Table + pos1 - 1;
      idx1 = IresultPtr->GetIndex();
    }
    if (pos2 != opos2) {
      opos2 = pos2;
      OtherIresultPtr = OtherTable + pos2 - 1;
      idx2 = OtherIresultPtr->GetIndex();
    }
    if (idx2 == idx1) {
      if (Metric < 1 && Metric > -1 && ((float) ((int) Metric) != Metric)) {
	// Got a Factor to multiply
	GetMdt(pos1)->GetEntry(IresultPtr->GetMdtIndex(), &mdtrec);
	Distance = (INT) (0.5 + Metric * (mdtrec.GetLocalRecordEnd()
		- mdtrec.GetLocalRecordStart()));
	if (Distance == 0 || Distance == 1)
	  Distance = 2;
      } else
	Distance = (INT) Metric;

      const FCLIST   *MyHitPtr = (const FCLIST *) IresultPtr->GetHitTable();
      const FCLIST   *OtherHitPtr = (const FCLIST *) OtherIresultPtr->GetHitTable();
      const GDT_BOOLEAN IsSorted = IresultPtr->HitTableIsSorted() && OtherIresultPtr->HitTableIsSorted();

      size_t          matchCount = 0;

      newHitTable.Clear();
      for (const FCLIST * kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next()) {
	const FC        MyFc(kp->Value());
	for (const FCLIST * p = OtherHitPtr->Next(); p != OtherHitPtr; p = p->Next()) {
	  GDT_BOOLEAN     IsMatch = GDT_FALSE;
	  const FC        OtherFc(p->Value());
	  const INT       diff = MyFc.GetFieldStart() - OtherFc.GetFieldStart();
	  if (Distance >= 0) {
	    // Near
	    if (diff < 0) {
	      if (direction != BEFORE &&
		  ((size_t) ((OtherFc.GetFieldStart() -
			      MyFc.GetFieldEnd() - 1)) <= (size_t) Distance))
		IsMatch = GDT_TRUE;
	    } else if (direction != AFTER &&
		       ((size_t) ((MyFc.GetFieldStart()
		      - OtherFc.GetFieldEnd() - 1)) <= (size_t) Distance)) {
	      IsMatch = GDT_TRUE;
	    }
	  } else {
	    // Far
	    if (diff < 0) {
	      if (direction != BEFORE &&
		  ((size_t) ((OtherFc.GetFieldStart() -
			  MyFc.GetFieldEnd() - 1)) >= (size_t) (-Distance)))
		IsMatch = GDT_TRUE;
	    } else if (direction != AFTER &&
		       ((size_t) ((MyFc.GetFieldStart()
		   - OtherFc.GetFieldEnd() - 1)) >= (size_t) (-Distance))) {
	      IsMatch = GDT_TRUE;
	    }
	  }
	  if (IsMatch) {
	    matchCount++;
	    newHitTable.AddEntry(OtherFc);
	    newHitTable.AddEntry(MyFc);
	  } else if (IsSorted) {
	    // Not Matched. Can we stop?
	    if ((direction != AFTER && diff > 0) || (direction != BEFORE && diff < 0))
	      break;		// Assumes that its been sorted..

	  }
	}			// for
      }				// for
      if (matchCount)
	{
	  NewTable[current] = *IresultPtr;
	  NewTable[current].SetHitTable (newHitTable);
	  NewTable[current].SetHitCount (matchCount);
          NewTable[current].SetAuxCount (2);
          const DOUBLE score = NewTable[current++].GetScore();
          if (score < newMinScore) newMinScore = score;
          if (score > newMaxScore) newMaxScore = score;
	}
    }
    if (idx1 <= idx2)
      pos1++;
    if (idx2 <= idx1)
      pos2++;
  }

  if (Table) delete [] Table;
  Table        = NewTable;
  MaxEntries   = newMaxEntries;
  TotalEntries = current;
  Sort         = ByIndex;
  MinScore     = newMinScore;
  MaxScore     = newMaxScore;
  return this;
}

// CharProx added by Kevin Gamiel
//
// Example:  'dog' within 5 characters of 'cat'
//
// For(all entries in dog result-list)
//   For(all entries in cat result-list)
//     if(in same record)
//       for(each hit in dog hit table)
//         for(each hit in cat hit table)
//           if(hits within 5 characters of each other)
//             add current dog item to final result-list
//
// If metric < 1 then its % of the size of the document.
OPOBJ *atomicIRSET::_CharProx (const OPOBJ& OtherIrset, const float Metric, DIR_T direction)
{
  IRESULT OtherIresult, MyIresult;
  atomicIRSET MyResult (Parent);
  const size_t OtherSetTotalEntries = OtherIrset.GetTotalEntries ();
  MDTREC mdtrec;
  INT Distance;
#if STORE_PROX
  FCT newHitTable;
#endif
  if (TotalEntries == 0 || OtherSetTotalEntries == 0)
    {
      // Nothing
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      TotalEntries = 0;
      return this; // Can't AND with nothing..
    }

  const atomicIRSET  *OtherPtr = (atomicIRSET *)&OtherIrset;
  GDT_BOOLEAN   OtherSorted = OtherPtr->Sort == ByIndex;
  if (OtherSorted)
    SortByIndex();

  _index_id_t  idx, other_idx;

  for (size_t i = 1; i <= TotalEntries; i++)
    {
      GetEntry (i, &MyIresult);
      idx = MyIresult.GetIndex ();
      for (size_t j = 1; j <= OtherSetTotalEntries; j++)
	{
	  OtherIrset.GetEntry (j, &OtherIresult);
	  other_idx = OtherIresult.GetIndex ();

	  // Only bother with things in the same record.
	  if (idx == other_idx)
	    {
	      if (Metric < 1 && Metric > -1 && ((float)((int)Metric) != Metric))
		{
		  // Got a Factor to multiply
		  GetMdt(i)->GetEntry (MyIresult.GetMdtIndex (), &mdtrec);
		  Distance = (INT)(0.5 + 
			Metric*(mdtrec.GetLocalRecordStart()-mdtrec.GetLocalRecordEnd()));
		  if (Distance == 0 || Distance == 1) Distance = 2;
		} else Distance = (INT)Metric;
	      const FCLIST *MyHitPtr    = (const FCLIST *)MyIresult.GetHitTable ();
	      const FCLIST *OtherHitPtr = (const FCLIST *)OtherIresult.GetHitTable();
	      const GDT_BOOLEAN IsSorted = MyIresult.HitTableIsSorted() && OtherIresult.HitTableIsSorted();

	      size_t matchCount = 0;
#if STORE_PROX
	      newHitTable.Clear();
#endif
	      for (const FCLIST *kp = MyHitPtr->Next(); kp != MyHitPtr; kp = kp->Next())
		{
		  const FC MyFc ( kp->Value() );
		  for (const FCLIST *p = OtherHitPtr->Next(); p != OtherHitPtr; p = p->Next())
		    {
		      GDT_BOOLEAN IsMatch = GDT_FALSE;
		      const FC OtherFc ( p->Value() );
		      const INT diff = MyFc.GetFieldStart () - OtherFc.GetFieldStart ();
		      if (Distance >= 0)
			{
			  // Near 
			  if (diff < 0)
			    {
			      if (direction != BEFORE &&
				  ((size_t) ((OtherFc.GetFieldStart () -
				     MyFc.GetFieldEnd () - 1)) <= (size_t)Distance))
				IsMatch = GDT_TRUE;
			    }
			  else if (direction != AFTER &&
				   ((size_t) ((MyFc.GetFieldStart ()
				- OtherFc.GetFieldEnd () - 1)) <= (size_t)Distance))
			    {
			      IsMatch = GDT_TRUE;
			    }
			}
		      else
			{
			  // Far
			  if (diff < 0)
			    {
			      if (direction != BEFORE &&
				  ((size_t) ((OtherFc.GetFieldStart () -
				     MyFc.GetFieldEnd () - 1)) >= (size_t)(-Distance)))
				IsMatch = GDT_TRUE;
			    }
			  else if (direction != AFTER &&
				   ((size_t) ((MyFc.GetFieldStart ()
				- OtherFc.GetFieldEnd () - 1)) >= (size_t)(-Distance)))
			    {
			      IsMatch = GDT_TRUE;
			    }
			}
		      if (IsMatch)
			{
			  matchCount++;
#if STORE_PROX
			  newHitTable.AddEntry( OtherFc );
			  newHitTable.AddEntry(  MyFc );
#else
			  break;
#endif
			}
		      else if (IsSorted)
			{
			  // Not Matched. Can we stop?
			  if ((direction != AFTER && diff > 0) || ( direction != BEFORE && diff < 0))
			    break; // Assumes that its been sorted..
			}
		    } // for
#if STORE_PROX == 0
		  if (matchCount)
		    break;
#endif
		} // for
#if STORE_PROX
	      if (matchCount)
		OtherIresult.SetHitTable(newHitTable);
#endif
	      if (matchCount)
		{
#if AWWW
		  MyResult.FastAddEntry (OtherIresult);
#else
		  MyResult.AddEntry (OtherIresult, GDT_FALSE);
#endif
		}
	    }
	  else if (OtherSorted && other_idx > idx)
	    break;
	} // for()
    } // for()

#if AWWW
  if (OtherSetTotalEntries)
    MyResult.MergeEntries(GDT_FALSE);
#endif

  delete[]Table;
  TotalEntries = MyResult.GetTotalEntries ();
  MaxEntries = MyResult.MaxEntries;
  Table = MyResult.StealTable ();
  return this;
}

// Within 5% of total size
// Others to consider 1,10,25
OPOBJ *atomicIRSET::Neighbor (const OPOBJ& OtherIrset)
{
  return CharProx(OtherIrset, 0.05, BEFOREorAFTER);
}

OPOBJ *atomicIRSET::Near (const OPOBJ& OtherIrset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return Peer(OtherIrset);
  return CharProx(OtherIrset, 50, BEFOREorAFTER);
}
OPOBJ *atomicIRSET::Far (const OPOBJ& OtherIrset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    XPeer(OtherIrset);
  return CharProx(OtherIrset, -50, BEFOREorAFTER);
}

// there used to have 2 as distance. Changed to 3.
OPOBJ *atomicIRSET::Adj (const OPOBJ& OtherIrset)
{
  return CharProx(OtherIrset, 3, BEFOREorAFTER);
}
// 2007 Dec.. Changed to 4 for cat & dot
OPOBJ *atomicIRSET::Follows (const OPOBJ& OtherIrset)
{
  return CharProx(OtherIrset, 4, AFTER);
}
OPOBJ *atomicIRSET::Precedes (const OPOBJ& OtherIrset)
{
  return CharProx(OtherIrset, 4, BEFORE);
}


//
// Before and After are 50 characters ONLY when not fielded.
// when fielded:
// the semantics are like PEER but ordered
// when BEFORE:field, resp. AFTER:field is specified then its before or after in
// the same instance of field
//


OPOBJ *atomicIRSET::Before (const OPOBJ& OtherIrset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return BeforePeer(OtherIrset);
  return CharProx(OtherIrset, 50, BEFORE);
}

OPOBJ *atomicIRSET::After (const OPOBJ& OtherIrset)
{
  if (Parent && Parent->GetDfdt() ->GetTotalEntries() > 0)
    return AfterPeer(OtherIrset); 
  return CharProx(OtherIrset, 50, AFTER);
}


PIRESULT atomicIRSET::StealTable()
{
  IRESULT* TempTablePtr = Table;
 
  TotalEntries = 0;
  MaxEntries = 2;
  try {
    Table = new IRESULT[MaxEntries];
  } catch (...) {
    Table = NULL;
    MaxEntries = 0;
    if (errno == 0) errno = ENOMEM;
    logf (LOG_ERRNO, "IRSET::StealTable() Could not allocate %ld IRESULTs!", MaxEntries);
    if (Parent) Parent->SetErrorCode(2); // Temp Error
  }
 
  return TempTablePtr;
}

void atomicIRSET::Adjust_Scores()
{
  if (TotalEntries && ComputedS > NoNormalization)
    {
      MinScore = MaxScore = Table[0].GetScore ();
      for (size_t i=1; i < TotalEntries; i++)
	{
	  DOUBLE score = Table[i].GetScore();
	  if (score > MaxScore) MaxScore = score;
	  if (score < MinScore) MinScore = score;
	}
    }
}


OPOBJ *atomicIRSET::ComputeScores (const INT TermWeight, enum NormalizationMethods Method)
{
  if (Method != ComputedS)
    {
       if (IsEmpty()) // Trivial case
	{
	  MinScore=0.0;
	  MaxScore=0.0;
	  ComputedS = Method;
	  return this; 
	}
      switch (Method)
	{
	  default:
	    logf (LOG_WARN, "Unknown/Undefined NormalizationMethod #%d.", (int)Method);
	  case Unnormalized:
	    break;
	  case NoNormalization:
	    return ComputeScoresNoNormalization (TermWeight);
	  case CosineNormalization:
	    return ComputeScoresCosineNormalization (TermWeight);
	  case CosineMetricNormalization:
	    if (ComputedS != preCosineMetricNormalization)
	      {
		OPOBJ *res = ComputeScoresCosineNormalization (TermWeight);
		ComputedS =  preCosineMetricNormalization;
		return res;
	      }
	    break;
	  case MaxNormalization:
	    return ComputeScoresMaxNormalization (TermWeight);
	  case LogNormalization:
	    return ComputeScoresLogNormalization (TermWeight);
	  case BytesNormalization:
	    return ComputeScoresBytesNormalization (TermWeight);
	}
    }
  return this;
}

OPOBJ *atomicIRSET::ComputeScoresNoNormalization (const INT TermWeight)
{
  if (TotalEntries && ComputedS == Unnormalized)
    {
      MinScore=0.0;
      MaxScore=0.0;
      ComputedS = NoNormalization;
    }
  return this;
}


extern "C" { float _ib_Distrank_weight_factor(const int); };

OPOBJ *atomicIRSET::ComputeScoresCosineMetricNormalization (const INT TermWeight)
{
//cerr << "ComputeScoresCosineMetricNormalization is called" << endl;


  // We must first have been normalized byt CosineNormalization!
  if (ComputedS != preCosineMetricNormalization && ComputedS != CosineMetricNormalization &&
    ComputedS != CosineNormalization)
    {
//cerr << "ComputedS was not pre or Cosine so need to do the pre Normalization" << endl;
      ComputeScoresCosineNormalization (1);
      ComputedS = preCosineMetricNormalization;
    }

  if (TotalEntries == 0)
    {
      ComputedS = CosineMetricNormalization;
    }
  else if ((ComputedS == preCosineMetricNormalization || ComputedS == CosineNormalization) && Parent)
    {
      for (size_t i = 0; i < TotalEntries; i++)
	{
	  FC Fc;
	  FC oldFc;
	  int min_dist = 1000;

	  if (Table[i].GetAuxCount() <= 1)
	    continue; // Not interested

	  for (const FCLIST *ptr  =  Table[i].GetHitTable().GetPtrFCLIST(),
		*p = ptr->Next(); p != ptr; p=p->Next())
	    {
	      int distance;

	      if (Fc == FC(0,0))
		oldFc = ptr->Value();
	      else
		oldFc = Fc;
	      Fc = p->Value();
	      if ((distance = Fc.GetFieldStart() - oldFc.GetFieldEnd()) < 0)
		distance = oldFc.GetFieldStart() - Fc.GetFieldEnd();
	      if (distance < min_dist)
		min_dist = distance;
	    }

          DOUBLE Score = Table[i].GetScore();
//cerr << "MinDistance=" << min_dist << "   Score = " << Score;
	  Score *=_ib_Distrank_weight_factor(min_dist)*TermWeight;
//cerr << " --> " << Score << endl;
	  Table[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
       }
      ComputedS = CosineMetricNormalization;
    }
  return this;
}

/*
 Dave Hawking's AF1:

  f_qt := number of times the current term occurred in the query
  f_dt := number of times term occurs in current document

  N    := number of documents in the collection 
          =  Parent->GetTotalRecords ()
  f_t  := number of documents in collection term occurs in
          = Parent->GetTotalRecords () / (double)TotalEntries


  k3 := 7 - 1000 (infinite)

 const float w_t  = log((N - f_t + 0.5F) / (f_t + 0.5F)) * log(TermWeight);
 const float w_qt = ((k3 + 1) * f_qt) / (k3 + f_qt);

 accumulator += w_qt * alpha * log(f_dt + 1) * w_t;

  for (size_t i = 0; i < TotalEntries; i++)
    {
       DOUBLE Score = (Table[i].GetHitCount () * f_t)* w_t;
    }

versus Cosine:

  accumulator += (1 + (float) log(f_qt)) * (1 + (float) log(f_dt));


*/


OPOBJ *atomicIRSET::ComputeScoresCosineNormalization (const INT TermWeight)
{
  if (TotalEntries && ComputedS != CosineNormalization && Parent)
    {
      // Get Freq of <Total Docs in Db>/<Total Docs in Result Set> 
      const off_t   TotalDocs = Parent->GetTotalRecords ();
      const DOUBLE InvDocFreq = (double)(TotalDocs) / (double)TotalEntries;

      double SumSqScores = 0;
      // Get sum of squares
      for (size_t i = 0; i < TotalEntries; i++)
	{
	  DOUBLE Score = Table[i].GetHitCount () * InvDocFreq;

#if USE_GEOSCORE
	  if (Table[i].HaveGscore()) 
	    Score += Table[i].GetGscore().Potenz();
#endif

	  SumSqScores += (Score * Score); // Running sum of squares
	  Score *= TermWeight;
	  Table[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
	}
      // The Root of Squares...
      errno = 0;
      DOUBLE SqrtSum= sqrt ((double)SumSqScores);
      if (SqrtSum == 0.0)
	{
	  SqrtSum = 1.0;
	}
      else
	{
	  MaxScore /= SqrtSum;
	  MinScore /= SqrtSum;
	}
      // Now caculate the Factor
      if (SqrtSum != 1.0)
	{
	  for (size_t x = 0; x < TotalEntries; x++)
	    Table[x].SetScore ( Table[x].GetScore () / SqrtSum );
	}
      ComputedS = CosineNormalization;
    }
  return this;
}


OPOBJ *atomicIRSET::ComputeScoresMaxNormalization (const INT TermWeight)
{
  if (TotalEntries && ComputedS != MaxNormalization && Parent)
    {
      // Get Freq of <Total Docs in Db>/<Total Docs in Result Set> 
      const DOUBLE InvDocFreq = ( Parent->GetTotalRecords () / (double)TotalEntries);

      MinScore=MAXFLOAT;
      MaxScore=0.0;
      {for (size_t i = 0; i < TotalEntries; i++)
	{
	  DOUBLE Score = Table[i].GetHitCount () * InvDocFreq;
#if USE_GEOSCORE
          if (Table[i].HaveGscore())
            Score += (Table[i].GetGscore().Potenz())*TermWeight;
#endif

	  Table[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
	}
      }
      if (MaxScore == 0.0 && TotalEntries)
	{
	  // Should never happen when we have at least one hit!
	  logf (LOG_PANIC, "MaxNormalization: ZERO Max Score?");
	  MinScore = 0.4*TermWeight;
	}
      else
	{
	  for (size_t i = 0; i < TotalEntries; i++)
	    Table[i].SetScore ((0.4 + (0.6*Table[i].GetScore()/MaxScore))*TermWeight);
	  MinScore = (0.4 + 0.6*MinScore/MaxScore)*TermWeight;
	}
      MaxScore = TermWeight;
      ComputedS = MaxNormalization;
    }
  return this;
}

OPOBJ *atomicIRSET::BoostScore (const float Weight)
{
  if (Weight != 1.0)
    {
      MinScore= Weight * MinScore;
      MaxScore= Weight * MaxScore;

      for (size_t i=0; i<TotalEntries; i++)
	Table[i].SetScore ( Table[i].GetScore() * Weight );
    }
  return this;
}

OPOBJ *atomicIRSET::ComputeScoresLogNormalization (const INT TermWeight)
{
  if (TotalEntries && ComputedS != LogNormalization && Parent)
    {
      const DOUBLE InvDocFreq = Parent->GetTotalRecords () / (double)TotalEntries;
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      for (size_t i = 0; i < TotalEntries; i++)
	{
	  DOUBLE Score = (1 + log (Table[i].GetHitCount () * InvDocFreq)) * TermWeight;
	  Table[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
	}
      ComputedS = LogNormalization;
    }
  return this;
}

OPOBJ *atomicIRSET::ComputeScoresBytesNormalization (const INT TermWeight)
{
  if (TotalEntries && ComputedS != BytesNormalization && Parent)
    {
      // Get Freq of <Total Docs in Db>/<Total Docs in Result Set> 
      const DOUBLE wInvDocFreq = TermWeight * ( Parent->GetTotalRecords () / (double)TotalEntries);

      MinScore=MAXFLOAT;
      MaxScore=0.0;

      for (size_t i = 0; i < TotalEntries; i++)
	{
	  // log of the length of the record..
	  MDTREC mdtrec;
	  DOUBLE logLen = 1 + log ( (double) ( GetMdtrec(i+1, &mdtrec) ? mdtrec.GetLength() : 1));
	  // Normalize by length..
	  DOUBLE Score = Table[i].GetHitCount () * wInvDocFreq / logLen;
	  Table[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
	}
      ComputedS = BytesNormalization;
    }
  return this;
}

static int IrsetScoreCompare (const void *x, const void *y)
{
  const DOUBLE Difference = ((const IRESULT *)y)->GetScore () - ((const IRESULT *)x)->GetScore ();
  if (Difference < 0)
    return (-1);
  else if (Difference > 0)
    return (1);
  return IrsetIndexCompare(y, x); // Reverse order in index
}

static int IrsetHitsCompare (const void *x, const void *y)
{
  const int Difference = ((const IRESULT *)y)->GetHitCount () - ((const IRESULT *)x)->GetHitCount ();
  if (Difference < 0)
    return (-1);
  else if (Difference > 0)
    return (1);
  return IrsetScoreCompare(x, y); // Same hits, order by score
}

static int IrsetHitsCompareReverse (const void *x, const void *y)
{
  const int Difference = ((const IRESULT *)x)->GetHitCount () - ((const IRESULT *)y)->GetHitCount ();
  if (Difference < 0)
    return (-1);
  else if (Difference > 0)
    return (1);
  return IrsetScoreCompare(y, x);
}


static int IrsetAuxCountCompare (const void *x, const void *y)
{
  int result = (((const IRESULT *) y)->GetAuxCount () - ((const IRESULT *) x)->GetAuxCount ());
  if (result == 0)
    {
      const DOUBLE Difference = ((const IRESULT *) y)->GetScore () - ((const IRESULT *) x)->GetScore ();
      if (Difference < 0) return (-1);
      else if (Difference > 0) return (1);
      return IrsetIndexCompare(y, x); // Reverse order in index
    }
  return result;
}

void atomicIRSET::SortByHits ()
{
  if (Sort != ByHits)
    {
      if (TotalEntries > 1 && ComputedS > NoNormalization)
        {
          logf (LOG_DEBUG, "Sorting IRSET by hits.");
          SORT (Table, TotalEntries, sizeof (IRESULT), IrsetHitsCompare);
        }
      Sort = ByHits;
    }
}

void atomicIRSET::SortByReverseHits ()
{
  if (Sort != ByReverseHits)
    {
      if (TotalEntries > 1 && ComputedS > NoNormalization)
        {
          logf (LOG_DEBUG, "Sorting IRSET by hits.");
          SORT (Table, TotalEntries, sizeof (IRESULT), IrsetHitsCompareReverse);
        }
      Sort = ByReverseHits;
    }
}


void atomicIRSET::SortByScore ()
{
  if (Sort != ByScore)
    {
      if (TotalEntries > 1 && ComputedS > NoNormalization)
	{
	  logf (LOG_DEBUG, "Sorting IRSET by Score.");
	  SORT (Table, TotalEntries, sizeof (IRESULT), IrsetScoreCompare);
	}
      Sort = ByScore;
    }
}

void atomicIRSET::SortByAuxCount ()
{
  if (Sort != ByAuxCount)
    {
      if (TotalEntries > 1)
	{
	  logf (LOG_DEBUG, "Sorting IRSET by AuxCount.");
          SORT (Table, TotalEntries, sizeof (IRESULT), IrsetAuxCountCompare);
	}
      Sort = ByAuxCount;
    }
}

void atomicIRSET::SortByFunction(int (*func)(const void *, const void *))
{
  if (Sort != ByFunction)
    {
      if (TotalEntries > 1)
	{
	  if (func)
	    {
	      logf (LOG_DEBUG, "Sorting IRSET by Function"); 
	      QSORT(Table, TotalEntries, sizeof(IRESULT), func);
	    }
	  else logf (LOG_WARN, "Application Error: Request to sort IRSET by undefined Function");
	}
       Sort = ByFunction;
    }
}

void atomicIRSET::SortByIndex ()
{
  if (Sort != ByIndex)
    {
      if (TotalEntries > 1)
	{
	  logf (LOG_DEBUG, "Sorting IRSET by Index.");
	  SORT (Table, TotalEntries, sizeof(IRESULT), IrsetIndexCompare);
	}
      Sort = ByIndex;
    }
}


static int IrsetSortIndexCompare(const void* x, const void* y)
{
  return ((IRESULT *)x)->GetSortIndex().Compare(  ((IRESULT *)y)->GetSortIndex() );
}


void atomicIRSET::SortByPrivate(int n)
{
  if (Sort != (ByPrivate+n))
    {
      if (__Private_IRSET_Sort)
	__Private_IRSET_Sort(Table, (int)TotalEntries, Parent, n, userData);
      else
	SortByScore();
      Sort = (enum SortBy)(ByPrivate+n);
    }
}


void atomicIRSET::SortByExtIndex (enum SortBy SortByWhich)
{
  if (SortByWhich >= ByExtIndex && Sort!= SortByWhich)
    {
      if (TotalEntries > 1 && Parent)
        {
	  const int SortNr = SortByWhich-ByExtIndex;
          logf (LOG_DEBUG, "Sorting IRSET by External Index %d.", SortNr);
	  // Load the Sort Indexes
	  if (Parent->SetSortIndexes(SortNr, this))
	    { 
	      // Now Sort
	      SORT (Table, TotalEntries, sizeof(IRESULT), IrsetSortIndexCompare);
	    }
	  else logf (LOG_WARN, "IRSET: Could not set external sort #%d", SortNr);
        }
      Sort = SortByWhich;
    }
}

// Sort by date (reverse chronological order)
int IrsetDateCompare(const void* x, const void* y)
{
  IRESULT *yp = (IRESULT *)y;
  IRESULT *xp = (IRESULT *)x;
  int    diff = Compare(yp->GetDate(), xp->GetDate());

  if (diff < 0)
    return (-1);
  else if (diff > 0)
    return 1;

  // else if diff = 0.0
  // If dates match then score first
  const DOUBLE Difference = yp->GetScore () - xp->GetScore ();
  if (Difference < 0) return (-1);
  if (Difference > 0) return (1);
  // Scores also match so..
  return IrsetIndexCompare(y, x); // Reverse order in index
}

void atomicIRSET::SortByDate ()
{
  if (Sort != ByDate)
    {
      if (TotalEntries > 1)
	{
	  logf (LOG_DEBUG, "Sorting IRSET by Date.");
	  SORT (Table, TotalEntries, sizeof(IRESULT), IrsetDateCompare);
	}
      Sort = ByDate;
    }
}


int IrsetReverseDateCompare(const void* y, const void* x)
{
  // Same as IrsetDateCompare but other direction
  return IrsetDateCompare(x, y);
}

void atomicIRSET::SortByReverseDate ()
{
  if (Sort != ByReverseDate)
    {
      if (TotalEntries > 1)
	{
	  logf (LOG_DEBUG, "Sorting IRSET by Reverse Date.");
          SORT (Table, TotalEntries, sizeof(IRESULT), IrsetReverseDateCompare);
	}
      Sort = ByReverseDate;
    }
}


// Sort by date (reverse chronological order)
static int IrsetKeyCompare(const void* x, const void* y)
{
  IRESULT *yp = (IRESULT *)y;
  IRESULT *xp = (IRESULT *)x;
  int    diff = Compare(yp->GetSortIndex(), xp->GetSortIndex());
 
  if (diff == 0)
    {
      if ((diff == IrsetScoreCompare(x, y)) == 0)
	return IrsetIndexCompare(y, x); // Reverse order in index
    }
  return (diff > 0 ? 1 : -1);
}
 
void atomicIRSET::SortByKey ()
{
  if (Sort != ByKey)
    {
      if (TotalEntries > 1 && Parent)
        {
	  MDT *MainMdt = Parent->GetMainMdt();
	  if (MainMdt)
	    {
	      for (size_t i=0; i<TotalEntries; i++)
		{
		   Table[i].SortIndex = MainMdt->KeySortPosition( Table[i].Index );
		}
	      logf (LOG_DEBUG, "Sorting IRSET by Key.");
	      SORT (Table, TotalEntries, sizeof(IRESULT), IrsetKeyCompare);
	    }
        }
      Sort = ByKey;
    }
} 


extern "C" {
  float _ib_Newsrank_weight_factor(const int);
  float _ib_Newsrank_weight_factor_hours(const int);
}; 

static int IrsetCompareNewsrank(const void* p1, const void* p2)
{
  register IRESULT *irp1 = (IRESULT *)p1;
  register IRESULT *irp2 = (IRESULT *)p2;

  const    double   score1 = irp1->GetScore();
  const    double   score2 = irp2->GetScore();

  SRCH_DATE         date1 = irp1->GetDate();
  SRCH_DATE         date2 = irp2->GetDate();
  DOUBLE            diff;

  if (date1.Ok() && date2.Ok())
    {
      if ((score1 == score2) ||
		(diff =  _ib_Newsrank_weight_factor_hours(date2.HoursAgo())*(score2) -
        	_ib_Newsrank_weight_factor_hours(date1.HoursAgo())*(score1)) == 0) {
	diff = Compare(date2, date1);
      }

      if (diff < 0)  return -1;
      if (diff > 0 ) return  1;
    }

   // Then sort by scores
   diff = score2 - score1;

   if (diff < 0) return -1;
   if (diff > 0) return  1;

   return IrsetIndexCompare(p2, p1);
}

void IRSET::SortByNewsrank()
{
  if (Sort != ByNewsrank)
    {
      SRCH_DATE Today("Today"); // Just to sync internal date
      if (Today.Ok())
        {
          SORT(Table, TotalEntries, sizeof(IRESULT), IrsetCompareNewsrank);
          Sort = ByNewsrank;
        }
    }
}


atomicIRSET::~atomicIRSET ()
{
//cerr << "Delete atomicIRSET : " << TotalEntries << endl;
//cerr << "Count = " << __IB_IRESULT_allocated_count;

#if 0
  if (allocs) logf (LOG_DEBUG, "*** IRSET::Total %d / Used %d / %d expands (%d)", MaxEntries, TotalEntries, allocs, Increment);
#endif
  if (Table) delete[]Table;
  // else cerr << "No table?" << endl;
//cerr << " -> " <<   __IB_IRESULT_allocated_count << endl;
}


void atomicIRSET::Clear()
{
  if (Table)
    {
      delete[]Table;
      Table = NULL;
    }
  TotalEntries = 0;
  MaxEntries   = 0;
  HitTotal     = 0;

  ComputedS = Unnormalized;

  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
  SortRequest = Unsorted;
}


