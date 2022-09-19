/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		iresult.hxx
Description:	Class IRESULT - Internal Search Result
@@@*/

#ifndef IRESULT_HXX
#define IRESULT_HXX

#include "defs.hxx"
#include "fct.hxx"
#include "date.hxx"
#include "mdt.hxx"

#define USE_GEOSCORE 1

//extern "C" { double  pow(double, double); };

class GEOSCORE {
#define K_TARGET 0.5
#define K_QUERY 0.5
public:
  GEOSCORE()
   {
     t = q = 0.0;
   }
  GEOSCORE(DOUBLE TScore, DOUBLE QScore)
   {
     t = TScore;
     q = QScore;
   }
  void          Write(FILE *fp) const;
  GDT_BOOLEAN   Read(FILE *fp);

  GDT_BOOLEAN Defined() const { return q>0 && t> 0;}
  GDT_BOOLEAN Equals(const GEOSCORE& Val) const  {
	return Val.t == t && Val.q == q;
  }
  INT         Compare(const GEOSCORE& Val) const {
    if (Val.t > t && Val.q > q) return 1;
    if (Val.t < t && Val.q < q) return -1;
    DOUBLE myP = Potenz();
    DOUBLE oP  = Val.Potenz();
    if (oP > myP) return 1; 
    if (oP < myP) return -1;
    return 0;
  }
  DOUBLE TScore(DOUBLE s)    { return t = s; }
  DOUBLE QScore(DOUBLE s)    { return q = s; }
  DOUBLE TScore() const      { return t;     }
  DOUBLE QScore() const      { return q;     }
  DOUBLE Potenz() const      { return (pow(t,K_TARGET)*pow(q,K_QUERY)); }
private:
  DOUBLE t;
  DOUBLE q;
#undef K_TARGET
#undef K_QUERY
};
inline void Write(const GEOSCORE& Score, FILE *fp) { Score.Write(fp); }
inline GDT_BOOLEAN Read(GEOSCORE *Score, FILE *fp) { return Score->Read(fp); }

// Operators
inline GDT_BOOLEAN operator==(const GEOSCORE& s1, const GEOSCORE& s2) { return s1.Equals(s2);  }
inline GDT_BOOLEAN operator!=(const GEOSCORE& s1, const GEOSCORE& s2) { return !s1.Equals(s2); }
inline GDT_BOOLEAN operator< (const GEOSCORE& s1, const GEOSCORE& s2) { return s1.Compare(s2) < 0; }
inline GDT_BOOLEAN operator<=(const GEOSCORE& s1, const GEOSCORE& s2) { return s1.Compare(s2) <= 0;}
inline GDT_BOOLEAN operator> (const GEOSCORE& s1, const GEOSCORE& s2) { return s1.Compare(s2) > 0; }
inline GDT_BOOLEAN operator>=(const GEOSCORE& s1, const GEOSCORE& s2) { return s1.Compare(s2) >= 0;}


#ifdef DEBUG_MEMORY
extern long __IB_IRESULT_allocated_count; // Used to track stray IRESULTs
#endif

class IRESULT {
  friend class atomicIRSET;
public:
  IRESULT();
  IRESULT(MDT *);
  IRESULT(const IRESULT& OtherIresult);
  IRESULT& operator=(const IRESULT& OtherIresult);

  INDEX_ID GetIndex() const { return Index; }
  void     SetIndex(const INDEX_ID& newIndex) { Index = newIndex; }

  SORT_INDEX_ID GetSortIndex() const { return SortIndex; }
  void     SetSortIndex(const SORT_INDEX_ID& newIndex) { SortIndex = newIndex; }

  void  SetMdtIndex(const INT NewMdtIndex) { Index.SetMdtIndex(NewMdtIndex); }
  INT   GetMdtIndex() const { return Index.GetMdtIndex(); };
  void  SetVirtualIndex(const UCHR newIndex) { Index.SetVirtualIndex(newIndex); }
  INT   GetVirtualIndex() const { return Index.GetVirtualIndex(); }

  void SetDate(const SRCH_DATE& newDate) { Date = newDate;};
  SRCH_DATE GetDate();

  void SetHitCount(const UINT NewHitCount) { HitCount = NewHitCount;     }
  UINT IncHitCount()                       { return ++HitCount;          }
  UINT IncHitCount(const UINT AddCount)    { return HitCount += AddCount;}
  UINT GetHitCount() const                 { return HitCount;            }

  void   SetScore(const DOUBLE NewScore)   { Score = NewScore;           }
  DOUBLE IncScore(const DOUBLE AddScore)   { return Score += AddScore;   }
  DOUBLE GetScore() const                  { return Score;               }

#if USE_GEOSCORE
  void     SetGscore(const GEOSCORE& gNew) { gScore = gNew;              }
  void     SetMaxGscore(const GEOSCORE& gOther) {
     if ( gOther > gScore) gScore = gOther;
  }
  GEOSCORE GetGscore() const               { return gScore;              }
  GDT_BOOLEAN HaveGscore() const           { return gScore.Defined();    }
# if 0
  // Following two for compatibility
  DOUBLE   GetTscore() const               { return gScore.TScore();     }
  DOUBLE   GetQscore() const               { return gScore.QScore();     }
# endif
#endif

  void Clear()                             { ClearHitTable(); Score = 0; }
  void ClearHitTable()                     { HitTable.Clear();           }

  void SetHitTable(const FC& Fc)                { HitTable = Fc;                    }
  void SetHitTable(const FCLIST& newHitTable)   { HitTable = newHitTable;           }
  void SetHitTable(const IRESULT& ResultRecord) { HitTable = ResultRecord.HitTable; }
  void SetHitTable(const FCT& newHitTable)      { HitTable = newHitTable;           }
  GDT_BOOLEAN HitTableIsSorted() const          { return HitTable.IsSorted();       }

  const FCT GetHitTable(FCLIST *Table = NULL) const;

  void AddToHitTable(const IRESULT& ResultRecord) { HitTable.AddEntry( ResultRecord.HitTable ); }
  void AddToHitTable(const FCT& OtherHitTable)    { HitTable.AddEntry( OtherHitTable );         }
  void AddToHitTable(const FC& Fc)                { HitTable.AddEntry(Fc);                      }

  void SetAuxCount(const UINT newAuxCount) { AuxCount = newAuxCount;     }
  UINT IncAuxCount()                       { return ++AuxCount;          }
  UINT IncAuxCount(const UINT AddCount)    { return AuxCount += AddCount;}
  UINT GetAuxCount() const                 { return AuxCount;            }

  GDT_BOOLEAN  SetVectorTermHits (const UINT count);

  void       SetMdt(const MDT *NewMdt) { Mdt = NewMdt; }
  const MDT *GetMdt() const            { return Mdt;   }


  GDT_BOOLEAN Read(FILE *fp);
  void Write(FILE *fp) const;
  ~IRESULT();
private:
  INDEX_ID      Index;
  SORT_INDEX_ID SortIndex;
  UINT4         HitCount;
  UINT2         AuxCount; // Max 65536 different search term matches
  UINT4        *TermHitsVector; // AuxCount elements, each with the number of hits..
  UINT2         maxTermHitsVector;  // allocated size of vector 

  DOUBLE        Score;
#if USE_GEOSCORE
  GEOSCORE      gScore;
#endif
  SRCH_DATE     Date;
  FCT           HitTable;
  const MDT    *Mdt;
};

#if 0

/////////////////////////////////////////////////////////////////
// Freestore management for IRESULTS 
/////////////////////////////////////////////////////////////////

class IRESULTptr {
  public:
    IRESULTPtr() {
      count = 1;
      ptr_ = IRESULT();
    }
    IRESULTptr(const IRESULT& Iresult) {
      count_ = 1;
      ptr_ = new IRESULT(Iresult);
    }
   ~IRESULTptr() { delete ptr_; }
  private:
    IRESULT       *ptr_;
    signed   int   count_;      // reference count
};

class CIRESULT {
public:
  IRESULTptr* operator-> () { return p_; }
  IRESULTptr& operator* ()  { return *p_; }

  CIRESULT() { p_ = new IRESULTptr(); }
  CIRESULT(const CIRESULT& Iresult) {
     ++Iresult.p_->count_;
     p_ = Iresult.p_;
  }
  CIRESULT& operator= (const CIRESULT& Iresult) {
    ++Iresult.p_->count_;
    unlock();
    p_ = Iresult.p_;
    return *this;
  }
  const IRESULT  GetIRSET() const    { return *(p_->ptr_);     }
  const IRESULT *GetPtrIRSET() const { return p_->ptr_;        }
  operator const IRESULT*() const    { return GetPtrIRESULT(); }
  operator const IRESULT() const     { return GetIRESULT();    }
}

#endif


class IRESULT_TABLE {
public:
  IRESULT_TABLE();
  IRESULT_TABLE(const IRESULT_TABLE& OtherTable);
  IRESULT_TABLE(const IRESULT& Record);
  IRESULT_TABLE(const IRESULT *TablePtr, size_t Total);

  IRESULT_TABLE& operator=(const IRESULT_TABLE& OtherTable);

  size_t         FindByMdtIndex(size_t Index) const;

  size_t         GetTotalEntries() const;
  void           SetTotalEntries(size_t Size);

  void           Clear() { SetTotalEntries(0); }
  void           Reinit();

  const IRESULT& operator[](size_t n) const { return t_Data[n]; }
  const IRESULT* GetData() const { return t_Data; }

  GDT_BOOLEAN    SetEntry(size_t n, const IRESULT& Record);
  void           AddEntry(const IRESULT& Record);

  DOUBLE         GetMinScore() const { return MinScore; }
  DOUBLE         GetMaxScore() const { return MaxScore; }

  void           NormalizeScores (const off_t TotalDocs, const INT TermWeight=1);

  void           SortByScore ();
  void           SortByIndex ();

  ~IRESULT_TABLE();

private:
  void          Expand(size_t Add);
  void          Allocate(size_t Len);
  void          CopyBeforeWrite();
// Data
  IRESULT      *t_Data;
  DOUBLE        MinScore, MaxScore;
  enum SortBy   Sort;
};


typedef IRESULT* PIRESULT;

void Write(const IRESULT& Iresult, PFILE Fp);
GDT_BOOLEAN Read(PIRESULT IresultPtr, PFILE Fp);

#endif
