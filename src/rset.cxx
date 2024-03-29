/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)$Id: rset.cxx,v 1.2 2021/01/17 13:15:45 edz Exp $ Project re:iSearch"


/************************************************************************
************************************************************************/

/*-@@@
File:		$RCSfile: rset.cxx,v $
Version:        $Revision: 1.2 $ 	
Description:	Class RSET - Search Result Set
@@@*/

#include <stdio.h>
#include "common.hxx"
#include "rset.hxx"
#include "magic.hxx"


#define INCREMENT 300 

static const float MAXSCORE=3.402823466E+18;
static const float MINSCORE=0.0;

RSET::RSET (size_t Reserve)
{
  MaxEntries = 0;
  Table = NULL;
  TotalEntries = 0;
  HighScore= MINSCORE;
  LowScore = MAXSCORE;
  HitTotal = 0;
  Increment = Reserve ? Reserve : INCREMENT;
  Sort = Unsorted; 
  Timestamp.SetNow();
  TotalFound = 0;
}

RSET::RSET (const RSET& OtherSet)
{
  Table = NULL;
  MaxEntries = 0;
  *this = OtherSet;
}

RSET& RSET::operator =(const RSET& OtherSet)
{
  const size_t OtherTotal = OtherSet.TotalEntries;
  TotalEntries = 0;
  if (MaxEntries < OtherTotal)
    Resize(OtherTotal);
  for (size_t x = 0; x < OtherTotal; x++)
    Table[x] = OtherSet.Table[x];
  TotalEntries = OtherTotal;
  HighScore    = OtherSet.HighScore;
  LowScore     = OtherSet.LowScore;
  Sort         = OtherSet.Sort;
  Increment    = OtherSet.Increment;
  Timestamp    = OtherSet.Timestamp;
  TotalFound   = OtherSet.TotalFound;

  return *this;
}

RSET& RSET::operator +=(const RSET& OtherSet)
{
  return Cat(OtherSet);
}

void RSET::SetVirtualIndex(const UCHR NewvIndex)
{
  for (size_t i=0; i<TotalEntries; i++)
    Table[i].SetVirtualIndex( NewvIndex );
}

UCHR RSET::GetVirtualIndex(size_t i) const
{
  if (i > 0 && i < TotalEntries)
    return Table[i-1].GetVirtualIndex();
  return 0;
}

////////////////////////////////////////////////////////////
//// TO DO //////////////////////////////////////////////////

RSET& RSET::Join(const RSET& OtherSet)
{
  message_log (LOG_PANIC, "RSET::Join not yet implemented");
 enum SortBy mySort = Sort;
 if (OtherSet.Sort == ByKey)
   {
     // Other set is sorted by key
   }
  return *this;
}

// like Cat but adds the hits
RSET& RSET::Union(const RSET& OtherSet)
{
  message_log (LOG_PANIC, "RSET::Union not yet implemented");
  return *this;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

RSET& RSET::Cat(const RSET& OtherSet)
{
  if (TotalEntries == 0)
    {
      *this = OtherSet;
    }
  else
    {
      const size_t OtherTotal = OtherSet.TotalEntries;
      const size_t NewTotal = TotalEntries + OtherTotal;
      if (NewTotal > MaxEntries)
	Resize(NewTotal);
      for (size_t x = 0; x < OtherTotal; x++)
	SetEntry(++TotalEntries, Table[x]);
    }
  return *this;
}

void RSET::LoadTable (const STRING& FileName)
{
  TotalEntries = 0;
  LowScore  = MAXSCORE;
  HighScore = MINSCORE;
  PFILE fp = FileName.Fopen ("rb");
  if (fp != NULL)
    {
      Read(fp);
      fclose(fp);
    }
}

void RSET::SaveTable (const STRING& FileName) const
{
  if (TotalEntries == 0)
    {
      FileName.Unlink();
      return;
    }
      
  PFILE fp = FileName.Fopen ("wb");
  if (fp != NULL)
    {
      Write(fp);
      fclose(fp);
    }
}

bool RSET::Read (PFILE fp)
{
  TotalEntries = 0; 
  LowScore  = MAXSCORE;
  HighScore = MINSCORE;

  obj_t obj = getObjID (fp);
  if (obj != objRSET)
    {
      // Not a RSET!
      PushBackObjID (obj, fp);
    }
  else
    {
      // Is a RSET
      UINT4 NewTotal;
      ::Read(&NewTotal, fp);
      Resize (NewTotal);
      for (UINT4 i=0; i < NewTotal; i++)
	{
	  Table[i].Read(fp);
	}
      TotalEntries = NewTotal;
#ifdef PORTABLE
      INT4 x;
      ::Read(&x, fp);
      HighScore = x / 10000.0;
      ::Read(&x, fp);
      LowScore = x /  10000.0;
#else
      ::Read(&HighScore, fp);
      ::Read(&LowScore, fp);
#endif
      UINT4 i4; ::Read(&i4, fp); HitTotal = i4;
      UCHR x; ::Read(&x, fp); Sort = (enum SortBy)x;
      ::Read(&Timestamp, fp);
      ::Read(&i4, fp); TotalFound = i4;
    }
  return obj == objRSET;
}

void RSET::Write (PFILE fp) const
{
  size_t i;
//INT4 *Offsets = new INT4[TotalEntries];

  ::Write((CHR)objRSET, fp);
  ::Write((UINT4)TotalEntries, fp);
  for (i=0; i < TotalEntries; i++)
    {
//    Offsets[i] = (INT4)ftell(fp);
      Table[i].Write(fp);
    }
#ifdef PORTABLE
  ::Write ((INT4)(HighScore * 10000), fp);
  ::Write ((INT4)(HighScore * 10000), fp); 
#else
  ::Write(HighScore, fp);
  ::Write(LowScore, fp); 
#endif
  ::Write((UINT4)HitTotal, fp);
  ::Write((UCHR)Sort, fp);
  ::Write(Timestamp, fp);
  ::Write((UINT4)TotalFound, fp);
/*
  // Now Write the addresses
  for (i=0; i < TotalEntries; i++)
    {
      ::Write(Offsets[i], fp);
    }
  delete [] Offsets;
*/
}


void RSET::SetScoreRange(DOUBLE High, DOUBLE Low)
{
  HighScore=High;
  LowScore=Low;
}

void RSET::AddEntry (const RESULT& ResultRecord)
{
  if (TotalEntries >= MaxEntries)
    Expand ();
  if (MaxEntries) SetEntry(++TotalEntries, ResultRecord);
}

void RSET::FastAddEntry (const RESULT& ResultRecord)
{
  if (TotalEntries >= MaxEntries)
    Expand ();
  if (MaxEntries) Table[TotalEntries++] =  ResultRecord;
}

bool RSET::GetEntry (const size_t Index, PRESULT ResultRecord) const
{
//cerr << "@@@@@@@@@@@@@@ GET ENTRY " << Index << endl;
  if ((Index > 0) && ((size_t)Index <= TotalEntries))
    {
      message_log (LOG_DEBUG, "Fetching RSET element %d", Index);
      *ResultRecord = Table[Index - 1];
//cerr << "Score = " << ResultRecord->GetScore() << endl;
      return true;
    }
//message_log (LOG_DEBUG, "RSET Element %d does not exist", Index);
  return false;
}


const RESULT& RSET::GetEntry(const size_t Index) const
{
//cerr << "@@@@@@@@@@@@@@ WANT " << Index << endl;
//message_log (LOG_DEBUG, "Want RSET Element %d of %d", Index, TotalEntries);
#if 1
  if (Index > 0 && Index <= TotalEntries)
     return Table[Index-1];

  return NulResult;
#else
  if (TotalEntries)
    {
     if (Index <= 0) return Table[0];
     if (Index > TotalEntries) return Table[TotalEntries-1];
     return Table[Index - 1];
    }
  return NulResult;
#endif
}


void RSET::SetEntry(const size_t x, const RESULT& ResultRecord )
{
  if (x > 0)
    {
      const DOUBLE S = ResultRecord.GetScore();

      if (S > HighScore)HighScore = S;
      if (S < LowScore) LowScore = S;
      Table[x-1] = ResultRecord;
      if (x == 2 && x == TotalEntries)
	{
	  if (Sort == Unsorted)
	    {
	      if (S > Table[0].GetScore())
		Sort = ByScore;
	      // else if ();
	    }
	}
      else if (Sort == ByScore)
	{
	  if (x < TotalEntries && Table[x].GetScore() > S)
	    Sort = Unsorted;
          else if (x > 1 && Table[x-2].GetScore() < S)
	    Sort = Unsorted;
	}
      else
	Sort = Unsorted; // Broke the sort
    }
}


int RSET::GetScaledScore(const double UnscaledScore, const int scale)
{
//cerr << "@@@@@@@@ GetScaledSCORE  ( " << UnscaledScore << " with scale " << scale << endl;
  // @@@ edz: use double and add 0.5 to correctly normalize results
  // HighScore := ScaleFactor
  // LowScore  := 0
  const int    ScaleFactor = scale ? scale : 100; 
  const double Delta = HighScore - LowScore;
  int   result = 0;

  if (UnscaledScore >= HighScore)
    result = ScaleFactor;
  else if (UnscaledScore < LowScore)
    result = 0;
  else if (Delta)
    result = (int)(((UnscaledScore - LowScore)/Delta)*(ScaleFactor-1) + 0.5)+1;
  else if (UnscaledScore == LowScore)
    result = ScaleFactor; 
  else
    result = (int)((UnscaledScore - LowScore)*ScaleFactor + 0.5);

//cerr << "@@SCORE = " << result << endl;

  return result;
}

void RSET::Expand ()
{
  Resize ((TotalEntries<<1) + Increment);
}

void RSET::CleanUp ()
{
  if (MaxEntries - TotalEntries > Increment)
    Resize (TotalEntries);
}

void RSET::Resize (const size_t Entries)
{
  if ((Entries == MaxEntries) || (TotalEntries && (Entries == TotalEntries)))
    return; // Don't bother..
  PRESULT OldTable = Table;
  if ((MaxEntries = Entries) > 0)
    {
      try {
	Table = new RESULT[MaxEntries];
      } catch (...) {
	Table = NULL;
      }
      if (Table)
	{
	  TotalEntries = (Entries >= TotalEntries) ? TotalEntries : Entries;
	  for (size_t x = 0; x < TotalEntries; x++)
	    Table[x] = OldTable[x];
	}
      else
	{
	  TotalEntries = 0;
	  MaxEntries = 0;
	  message_log (LOG_PANIC|LOG_ERRNO, "RSET::Resize: Could not allocate %ld", (long)MaxEntries);
	}
    }
  else
    {
      Table = NULL;
      TotalEntries = 0;
    }
  if (OldTable) delete[]OldTable;
}

static int RsetCompareKeys(const void* ResultPtr1, const void* ResultPtr2)
{
  return Compare( ((RESULT*)ResultPtr1)->GetKey(), ((RESULT*)ResultPtr2)->GetKey());
}

static int RsetCompareIndex(const void* p1, const void* p2)
{
  const INDEX_ID c1 = ((RESULT*)p1)->GetIndex();
  const INDEX_ID c2 = ((RESULT*)p2)->GetIndex();
  if      (c1 > c2) return -1;
  else if (c1 < c2) return 1;
  return 0;
}

static int RsetCompareScores(const void* p1, const void* p2)
{
  FLOAT diff = (FLOAT)((RESULT*)p2)->GetScore() - (FLOAT)((RESULT*)p1)->GetScore();
  if (diff < 0)  return -1;
  if (diff > 0 ) return  1;
  return RsetCompareIndex(p2, p1); // Reverse order in index
}

static int RsetCompareHits(const void* p1, const void* p2)
{
  int diff = (FLOAT)((RESULT*)p2)->GetHitTotal() - (FLOAT)((RESULT*)p1)->GetHitTotal();
  if (diff < 0)  return -1;
  if (diff > 0 ) return  1;
  return RsetCompareScores(p1, p2);
  return 0;
}

static int RsetCompareHitsReverse(const void* p2, const void* p1)
{
  int diff = (FLOAT)((RESULT*)p2)->GetHitTotal() - (FLOAT)((RESULT*)p1)->GetHitTotal();
  if (diff < 0)  return -1;
  if (diff > 0 ) return  1;
  return RsetCompareScores(p1, p2);
  return 0;
}


void RSET::SortByFunction(int (*func)(const void *, const void *))
{
  if (Sort != ByFunction)
    {
      if (TotalEntries > 1)
	{
	  if (func)
	    {
	      message_log (LOG_DEBUG, "Sorting RSET by Function");
	      QSORT(Table, TotalEntries, sizeof(RESULT), func);
	    }
	  else message_log (LOG_WARN, "Application Error: Request to sort RSET by undefined Function");
	}
      Sort = ByFunction;
    }
}

void RSET::SortByIndex()
{
  if (Sort != ByIndex)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareIndex);
      Sort = ByIndex;
    }
}


static int RsetCompareExtIndex(const void* p1, const void* p2)
{
  const long c1 = ((RESULT*)p1)->GetExtIndex();
  const long c2 = ((RESULT*)p2)->GetExtIndex();
  if      (c1 > c2) return -1;   
  else if (c1 < c2) return 1;
  return RsetCompareIndex(p1, p2);
}
  
 
void RSET::SortByExtIndex()
{
  if (Sort < ByExtIndex)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareExtIndex);
      Sort = ByExtIndex;
    }
}




void RSET::SortByKey()
{
  if (Sort != ByKey)
    {
      message_log (LOG_DEBUG, "RSET::SortByKey()");
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareKeys);
      Sort = ByKey;
    }
}

size_t RSET::Find(const STRING& Key) const
{
  STRING s;
  // linear search (don't want to change sort)
  for (size_t j=0; j< TotalEntries; j++)
    {
      Table[j].GetKey(&s);
      if (s == Key)
	return j + 1;
    }
  return 0;
}

#if 0
// Fast Add each result..
// then sort.. And walk through each element
// collecting only the unique records..
void RSET::And (const PRSET& OtherSet)
{
  size_t newTotal = 0;
  RESULT ResultRecord;
  SortByKey();
  size_t OtherTotal = OtherSet->GetTotalEntries ();
  PRESULT newTable = new RESULT[(OtherTotal > TotalEntries ?
	TotalEntries : OtherTotal)];
  for (size_t j=0; j< TotalEntries; j++)
    {
      if (OtherSet->Contains(Table[j])
	{
	  newTable[newTotal++] = Table[j];
	}
   }     /* for () */
}

#endif

void RSET::SortByHits()
{
  if (Sort != ByHits)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareHits);
      Sort = ByHits;
    }
}

void RSET::SortByReverseHits()
{
  if (Sort != ByReverseHits)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareHitsReverse);
      Sort = ByReverseHits;
    }
}

void RSET::SortByScore()
{
  if (Sort != ByScore)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareScores);
      Sort = ByScore;
    }
}

static int RsetCompareAdjScores(const void* p1, const void* p2)
{
  FLOAT diff = (FLOAT)((RESULT*)p2)->GetScore() - (FLOAT)((RESULT*)p1)->GetScore();
  if (diff < 0)  return -1;
  if (diff > 0 ) return  1;
  // Now sort by Category....
  const _ib_category_t c1 = ((RESULT*)p1)->GetCategory();
  const _ib_category_t c2 = ((RESULT*)p2)->GetCategory();
  if      (c1 > c2) return -1;
  else if (c1 < c2) return 1;
  return 0;
}


void RSET::SortByAdjScore()
{
  if (Sort != ByAdjScore)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareAdjScores);
      Sort = ByAdjScore;
    }
}

void RSET::SortByPrivate(int n)
{
  if (Sort != (ByPrivate+n))
    {
      if (__Private_RSET_Sort)
	__Private_RSET_Sort(Table, (int)TotalEntries, n);
      else
	SortByScore();
      Sort = (enum SortBy)(ByPrivate+n);
    }
}

void RSET::SortByCategoryMagnetism(DOUBLE Factor)
{
  if (Sort != ByAdjScore)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareAdjScores);
      Sort = ByAdjScore;
    }
  if (TotalEntries > 4 && HighScore != 0 && HighScore != LowScore)
    {
      bool changed = false;
      const DOUBLE mFactor = Factor*(HighScore - LowScore)/HighScore;
      for (size_t i=0; i < TotalEntries-3; i++)
	{
	  _ib_category_t myCategory = Table[i].GetCategory();
	  if (myCategory)
	    {
	      for (size_t j = i+1; j < TotalEntries-2 ; j++)
		{
		  if (Table[j].GetScore() > (Table[j+1].GetScore()+mFactor))
		    break;
		  if (myCategory != Table[j].GetCategory() && myCategory == Table[j+1].GetCategory())
		    {
		      RESULT tmp = Table[j];
		      Table[i+1] = Table[j+1];
		      Table[i+2] = tmp;
		      changed = true;
		    }
		}
	    }
	}
      if (changed)
	Sort = Unsorted;
    }
}


static int RsetCompareCategories(const void* p1, const void* p2)
{
  const _ib_category_t c1 = ((RESULT*)p1)->GetCategory();
  const _ib_category_t c2 = ((RESULT*)p2)->GetCategory();
  if      (c1 > c2) return -1;
  else if (c1 < c2) return 1;
  return RsetCompareScores(p1, p2);
}


void RSET::SortByCategory()
{
  if (Sort != ByCategory)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareCategories);
      Sort = ByCategory;
    }
}


static int RsetCompareAuxCount(const void* p1, const void* p2)
{
  int result = ((const RESULT*)p2)->GetAuxCount() - ((const RESULT*)p1)->GetAuxCount();
  if (result == 0)
    {
      FLOAT diff = (FLOAT)((const RESULT*)p2)->GetScore() - (FLOAT)((const RESULT*)p1)->GetScore();
      if (diff < 0)  return -1;
      if (diff > 0 ) return  1;
    }
  return result;
}

void RSET::SortByAuxCount()
{
  if (Sort != ByAuxCount)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareAuxCount);
      Sort = ByAuxCount;
    }
}

// 3 May 2007: Now same logic as IRSET::Reduce(..)

size_t RSET::Reduce(INT Level)
{
  if (TotalEntries && Level >= 0)
    {
      // Do we have AuxCounts in our table?
      if (Table[0].GetAuxCount() == 0)
	{
	  // Check that something has a count to even bother!
	  int count = 0;
	  for (size_t i=1; i<TotalEntries && !count; i++)
	    count = Table[i].GetAuxCount();
	  if (count == 0)
	    return TotalEntries; // Nothing to do
	}

      enum SortBy oldSort = Sort;

      UINT TermCount = (UINT)Level;

      SortByAuxCount();

      UINT Count0 = Table[0].GetAuxCount();
      if (TermCount == 0)
	TermCount = Count0;

      size_t i = 0;
      UINT  cutoff = TermCount > 2 ? 1 : 0;
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
	    i = 1;
	  else
	    {
	      Count0 = Count1;
	      for (i=2; i < TotalEntries && ((Count1 = Table[i].GetAuxCount()) > Count0); i++)
		/* loop */;
	    }
	}

#if 1
      LowScore = MAXSCORE;
      HighScore= MINSCORE; 
#endif
      for (; i < TotalEntries; i++)
	{
#if 1
	  const DOUBLE score = Table[i].GetScore();
	  if (LowScore > score) LowScore = score;
	  if (HighScore< score) HighScore= score;
#endif
	  if (i && (Count0 = Table[i].GetAuxCount()) <= cutoff)
	    TotalEntries = i;
	}
      SortBy(oldSort);
    }
  return TotalEntries;
}

size_t RSET::DropByTerms(size_t TermCount)
{
  if (TotalEntries)
    {
      const enum SortBy oldSort = Sort;
      SortByAuxCount();
      for (;TotalEntries>0; TotalEntries--)
        {
          if (Table[TotalEntries-1].GetAuxCount() >= TermCount)
            break;
        }
      SortBy(oldSort);
    }
  return TotalEntries;
}



size_t RSET::DropByScore(INT ScaledScore, INT ScaleFactor)
{
  // LowScore  := 0
  const DOUBLE Delta = HighScore != LowScore ? HighScore - LowScore : 1;

  if (TotalEntries)
    {
      const enum SortBy oldSort = Sort;
      SortByScore();
      for (;TotalEntries>0; TotalEntries--)
	{
	  if ((INT)(((Table[TotalEntries-1].GetScore() - LowScore) * ScaleFactor)/Delta + 0.5)
		>= ScaledScore)
	    break;
	}
      SortBy(oldSort);
    }
  return TotalEntries;
}

size_t RSET::DropByScore(DOUBLE Score)
{
  if (TotalEntries)
    {
      const enum SortBy oldSort = Sort;
      SortByScore();
      for (;TotalEntries>0; TotalEntries--)
	{
	  if (Table[TotalEntries-1].GetScore() >= Score)
	    break;
	}
	SortBy(oldSort);
    }
  return TotalEntries;
}

extern "C" { float _ib_Newsrank_weight_factor_hours(const int); };

static int RsetCompareNewsrank(const void* p1, const void* p2)
{
  const SRCH_DATE Date1 (((RESULT*)p1)->GetDate());
  const SRCH_DATE Date2 (((RESULT*)p2)->GetDate());
  float diff = (float)(
	_ib_Newsrank_weight_factor_hours(Date2.HoursAgo())*((RESULT*)p2)->GetScore() -
	_ib_Newsrank_weight_factor_hours(Date1.HoursAgo())*((RESULT*)p1)->GetScore() );

   if (diff < 0)  return -1;
   if (diff > 0 ) return  1;

  // Scores and dates match then category
  const _ib_category_t c1 = ((RESULT*)p1)->GetCategory();
  const _ib_category_t c2 = ((RESULT*)p2)->GetCategory();
  if      (c1 > c2) return -1;
  else if (c1 < c2) return 1;

  return Compare(Date2, Date1);
}
  
void RSET::SortByNewsrank()
{
  if (Sort != ByNewsrank)
    {
      SRCH_DATE Today("Today"); // Just to sync internal date
      if (Today.Ok())
	{
	  QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareNewsrank);
	  Sort = ByNewsrank;
	}
    }
}


static int RsetCompareDate(const void* p1, const void* p2)
{
  int diff = Compare(((RESULT*)p2)->GetDate(), ((RESULT*)p1)->GetDate());
  if (diff == 0)
    diff = Compare(((RESULT*)p2)->GetDateModified(), ((RESULT*)p1)->GetDateModified());
  if (diff == 0)
    diff = Compare(((RESULT*)p2)->GetDateCreated(), ((RESULT*)p1)->GetDateCreated());

  if (diff == 0)
    return RsetCompareScores (p1, p2);
  else if (diff < 0)
     return -1;
  return 1;
}
 
void RSET::SortByDate()
{
  if (Sort != ByDate)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetCompareDate);
      Sort = ByDate;
    }
}

static int RsetReverseCompareDate(const void* p2, const void* p1)
{
#if 1
  return  RsetCompareDate(p1, p2) ;
#else
  int diff = Compare(((RESULT*)p2)->GetDate(), ((RESULT*)p1)->GetDate());
  if (diff == 0)
    return RsetCompareScores (p1, p2);
  else if (diff < 0)
     return -1;
  return 1;
#endif
}

void RSET::SortByReverseDate()
{
  if (Sort != ByReverseDate)
    {
      QSORT(Table, TotalEntries, sizeof(RESULT), RsetReverseCompareDate);
      Sort = ByReverseDate;
    }
}



size_t RSET::SetTotalEntries(size_t NewTotal)
{
  if (NewTotal < TotalEntries)
    TotalEntries = NewTotal;
  return TotalEntries;
}


bool RSET::FilterDateRange(const DATERANGE& Range)
{
  if (TotalEntries <= 1)
    return true; // Don't bother..
  if (!Range.Ok())
    return false;
  PRESULT oldTable = Table;
  MaxEntries = TotalEntries;
  Table = new RESULT[MaxEntries];
  TotalEntries = 0;
  HighScore= MINSCORE;
  LowScore = MAXSCORE;
  for (size_t i = 0; i < MaxEntries; i++)
    {
      if (Range.Contains(oldTable[i].GetDate()))
	{
	  const DOUBLE score = (Table[TotalEntries++] = oldTable[i]).GetScore();
	  if (score > HighScore) HighScore = score;
	  if (score < LowScore)  LowScore  = score;
	}
    }
  if (oldTable) delete[]oldTable;
  Timestamp.SetNow();
  return true;
}


void RSET::Dump (INT SKIP, ostream& os) const
{
  STRING key;
  for (size_t x = SKIP; x < TotalEntries; x++)
    {
      Table[x].GetKey(&key);
      os << key << ", ";
    }
  os << endl;
}

RSET::~RSET ()
{
  if (Table) delete[]Table;
}



