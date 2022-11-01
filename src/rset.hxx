/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		$RCSfile: rset.hxx,v $
Version:	$Revision: 1.1 $
Description:	Class RSET - Search Result Set
@@@*/

#ifndef RSET_HXX
#define RSET_HXX

#include "result.hxx"

class RSET {
public:
  RSET(size_t Reserve = 0);
  RSET(const RSET& OtherSet);

  RSET& operator =(const RSET& OtherSet);
  RSET& operator +=(const RSET& OtherSet);

  void SetVirtualIndex(const UCHR NewvIndex);
  UCHR GetVirtualIndex(size_t i) const;

  RSET& Join(const RSET& OtherSet);
  RSET& Union(const RSET& OtherSet);

  RSET& Cat(const RSET& OtherSet);

  void LoadTable(const STRING& FileName);
  void SaveTable(const STRING& FileName) const;

  void AddEntry(const RESULT& ResultRecord);
  void FastAddEntry (const RESULT& ResultRecord);

  void SetScoreRange(DOUBLE High, DOUBLE Low);
  DOUBLE GetMaxScore () const { return HighScore; }
  DOUBLE GetMinScore () const { return LowScore;  }

  bool GetEntry(const size_t Index, PRESULT ResultRecord) const;
  const RESULT& GetEntry(const size_t Index) const;

  void SetEntry(const size_t x, const RESULT& ResultRecord);

  const RESULT operator[](size_t n) const { return GetEntry(n+1); }
  RESULT& operator[](size_t n)            { return Table[n]; }

  int  GetScaledScore(const double UnscaledScore, const int ScaleFactor = 100);

  size_t GetHitTotal () const          { return HitTotal; }
  void   SetHitTotal (size_t NewTotal) { HitTotal = NewTotal; }

  size_t Find(const STRING& Key) const;

  void Expand();
  void CleanUp();
  void Resize(const size_t Entries);

  size_t GetTotalEntries() { return  TotalEntries; }
  size_t SetTotalEntries(size_t NewTotal);

  void   SetTotalFound(size_t Total) { TotalFound = Total; }
  size_t GetTotalFound() const       { return TotalFound;  }

  bool FilterDateRange(const DATERANGE& Range);


  void SortBy(int (*func)(const void *, const void *)) {
    installSortFunction (func);
    SortByFunction(func);
  }
  void installSortFunction(int (*func)(const void *, const void *)) {
    if (RsetCompar != func)
      {
        if (Sort == ByFunction && func) Sort = Unsorted;
        RsetCompar = func;
      }
  }

  void SortBy(enum SortBy SortBy) {
    if      (SortBy == ByKey)         SortByKey();
    else if (SortBy == ByIndex)       SortByIndex();
    else if (SortBy == ByScore)       SortByScore();
    else if (SortBy == ByHits)        SortByHits();
    else if (SortBy == ByReverseHits) SortByReverseHits ();
    else if (SortBy == ByAdjScore)    SortByAdjScore();
    else if (SortBy == ByAuxCount)    SortByAuxCount();
    else if (SortBy == ByDate)        SortByDate();
    else if (SortBy == ByReverseDate) SortByReverseDate();
    else if (SortBy == ByNewsrank)    SortByNewsrank();
    else if (SortBy == ByCategory)    SortByCategory();
    else if (SortBy == ByFunction)    SortByFunction(RsetCompar);
    else if (SortBy >= ByExtIndex)    SortByExtIndex();
    else if (SortBy >= ByPrivate && SortBy < ByExtIndex)
      SortByPrivate(SortBy-ByPrivate);
  }
  void SortByCategoryMagnetism(DOUBLE Factor);

  size_t Reduce(INT TermCount);
  size_t DropByTerms(size_t TermCount);
  size_t DropByScore(DOUBLE Score);
  size_t DropByScore(INT ScaledScore, INT ScaleFactor=100);

  void Dump(INT Skip =0, ostream& os = cout) const;

  bool SetTimestamp(SRCH_DATE newTimestamp) {
    if (newTimestamp.Ok())
      {
	Timestamp = newTimestamp;
	return true;
      }
    return false;
  }
  SRCH_DATE GetTimestamp() const { return Timestamp; }

  void Write (PFILE fp) const;
  bool Read (PFILE fp);

  ~RSET();
private:
  friend class atomicIRSET;
  void SortByIndex();
  void SortByExtIndex();
  void SortByDate ();
  void SortByReverseDate ();
  void SortByScore ();
  void SortByHits ();
  void SortByReverseHits ();
  void SortByAuxCount ();
  void SortByKey ();
  void SortByAdjScore();
  void SortByCategory();
  void SortByNewsrank();
  void SortByFunction (int (*func)(const void *, const void *));
  void SortByPrivate(int n);

  PRESULT Table;
  size_t Increment;
  size_t TotalEntries;
  size_t MaxEntries;
  size_t HitTotal;
  size_t TotalFound; // Guess
  DOUBLE HighScore, LowScore;
  enum SortBy Sort;
  SRCH_DATE Timestamp;
  int (*RsetCompar)(const void *, const void *);
};

extern "C" int (* __Private_RSET_Sort) (void *Table, int Total, int which);

typedef RSET* PRSET;

// Common functions

inline void Write(const RSET Rset, PFILE Fp)
{
  Rset.Write(Fp);
}

inline bool Read(PRSET RsetPtr, PFILE Fp)
{
  return RsetPtr->Read(Fp);
}


#endif
