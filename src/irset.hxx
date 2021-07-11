/************************************************************************
************************************************************************/

/*@@@
File:		irset.hxx
Version:	1.00
Description:	Class IRSET - Internal Search Result Set
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef IRSET_HXX
#define IRSET_HXX

#include "defs.hxx"
#include "idbobj.hxx"
#include "string.hxx"
#include "iresult.hxx"
#include "rset.hxx"
#include "operand.hxx"

// NOTE:
//
// What was CosineNormalization shall in the future be called EuclideanNormalization
//          CosineMetricNormalization shall be  EuclideanNormalization
//
// Cosine shall become traditional Cosine
// pCosine shall be added
///
////
extern enum NormalizationMethods {
  Unnormalized = 0, NoNormalization, CosineNormalization, MaxNormalization, LogNormalization, BytesNormalization,
  preCosineMetricNormalization, CosineMetricNormalization, UndefinedNormalization
} defaultNormalization;

class atomicIRSET : public OPERAND {
public:
  atomicIRSET(const PIDBOBJ DbParent = NULL, size_t Reserve = 0);
  atomicIRSET(const OPOBJ& OtherIrset);

  atomicIRSET& operator = (const atomicIRSET& OtherIrset);
  OPOBJ& operator =(const OPOBJ& OtherIrset);

  OPOBJ& operator +=(const OPOBJ& OtherIrset);
  atomicIRSET& Concat (const atomicIRSET& OtherIrset); 

  OPOBJ& Cat(const OPOBJ& OtherIrset, GDT_BOOLEAN AddHitCounts = GDT_FALSE);
  OPOBJ& Cat (const OPOBJ& OtherIrset, size_t Total, GDT_BOOLEAN AddHitCounts = GDT_FALSE);

  t_Operand GetOperandType () const { return TypeRset; };

  //operator const RSET *() const;
  void   GC(); // Garbage collect

  OPOBJ* Duplicate () const;
  atomicIRSET* Duplicate();

  void MergeEntries(const GDT_BOOLEAN AddHitCounts = GDT_TRUE);
  void Adjust_Scores();

  GDT_BOOLEAN GetMdtrec(size_t i, MDTREC *Mdtrec) const;
 
  void SetMdt(const IDBOBJ *Idb);
  void SetMdt(const MDT *NewMdt);
  const MDT *GetMdt(size_t i) const;

  void SetVirtualIndex(const UCHR NewvIndex);
  UCHR GetVirtualIndex(size_t i) const;

  void LoadTable (const STRING& FileName);
  void SaveTable (const STRING& FileName) const;

  void AddEntry (const IRESULT& ResultRecord, const GDT_BOOLEAN AddHitCounts);
  void FastAddEntry(const IRESULT& ResultRecord);

  GDT_BOOLEAN GetEntry (const size_t Index, PIRESULT ResultRecord) const;
  IRESULT     GetEntry (const size_t Index) const;

  const IRESULT operator[](size_t n) const { return Table[n]; }
  IRESULT& operator[](size_t n)            { return Table[n]; }

  INDEX_ID    GetIndex(size_t i) {
    if (i>0 && i<= TotalEntries)
      return Table[i-1].GetIndex();
    return 0;
  }
  GDT_BOOLEAN SetSortIndex(size_t i, const SORT_INDEX_ID& sort) {
    if (i>0 && i<= TotalEntries)
      Table[i-1].SetSortIndex(sort);
    else
      return GDT_FALSE;
    return GDT_TRUE;
  }

  PRSET GetRset () const;
  PRSET GetRset (size_t Total) const;
  // Getting RSet in two passes...
  PRSET GetRset(size_t Start, size_t End) const; // Start count with 0

  // 
  // Fill(0) returns empty RSET
  // Fill() or Fill(1) return the entire IRSET as RSET
  PRSET Fill(size_t Start=1, size_t End = 0, PRSET set = NULL) const; // Fill start count with 1

  GDT_BOOLEAN IsEmpty() const { return TotalEntries == 0; }

  void Expand ();
  void Reserve (const size_t Entries) { if (Entries > MaxEntries) Resize(Entries); };
  void CleanUp ();
  void Resize (const size_t Entries);
  size_t GetTotalEntries () const { return TotalEntries; }
  size_t SetTotalEntries(size_t NewTotal);
  size_t GetHitTotal ();

  // Score normalization (scoring functions)
  OPOBJ *ComputeScores (const INT TermWeight, enum NormalizationMethods Method = defaultNormalization);
  // Methods
  OPOBJ *ComputeScoresNoNormalization (const INT TermWeight);
  OPOBJ *ComputeScoresCosineNormalization (const INT TermWeight);
  OPOBJ *ComputeScoresMaxNormalization (const INT TermWeight);
  OPOBJ *ComputeScoresLogNormalization (const INT TermWeight);
  OPOBJ *ComputeScoresBytesNormalization (const INT TermWeight);
  OPOBJ *ComputeScoresCosineMetricNormalization (const INT TermWeight);

  // Binary Functions
  virtual OPOBJ *Or (const OPOBJ& OtherIrset);
  virtual OPOBJ *Nor (const OPOBJ& OtherIrset);
  virtual OPOBJ *And (const OPOBJ& OtherIrset);
  virtual OPOBJ *And (const OPOBJ& OtherIrset, size_t Limit);

  virtual OPOBJ *Nand (const OPOBJ& OtherIrset);
  virtual OPOBJ *AndNot (const OPOBJ& OtherIrset);
  virtual OPOBJ *Xor (const OPOBJ& OtherIrset);

  virtual OPOBJ *Join (const OPOBJ& OtherIrset);
  virtual OPOBJ *Join (OPOBJ *OtherIrset);

  virtual OPOBJ *Near (const OPOBJ& OtherIrset);
  virtual OPOBJ *Far (const OPOBJ& OtherIrset);
  virtual OPOBJ *After (const OPOBJ& OtherIrset);
  virtual OPOBJ *Before (const OPOBJ& OtherIrset);
  virtual OPOBJ *Adj (const OPOBJ& OtherIrset);
  virtual OPOBJ *Follows (const OPOBJ& OtherIrset);
  virtual OPOBJ *Precedes (const OPOBJ& OtherIrset);
  virtual OPOBJ *Neighbor (const OPOBJ& OtherIrset);
  virtual OPOBJ *XPeer (const OPOBJ& OtherIrset);
  virtual OPOBJ *Peer (const OPOBJ& OtherIrset);
  virtual OPOBJ *BeforePeer (const OPOBJ& Irset);
  virtual OPOBJ *AfterPeer (const OPOBJ& Irset);


  virtual OPOBJ *Within(const OPOBJ& OtherIrset, const STRING& Fieldname);
  virtual OPOBJ *BeforeWithin(const OPOBJ& OtherIrset, const STRING& Fieldname);
  virtual OPOBJ *AfterWithin(const OPOBJ& OtherIrset, const STRING& Fieldname);

  // Unary Functions
  virtual OPOBJ *Within(const char *field);
  virtual OPOBJ *Within(const DATERANGE& Daterange);
  virtual OPOBJ *Within(const STRING& FieldName);
  virtual OPOBJ *Inside(const STRING& FieldName);
  virtual OPOBJ *Sibling ( );
  virtual OPOBJ *Inclusive(const STRING& FieldName);
  virtual OPOBJ *XWithin(const STRING& FieldName);
  virtual OPOBJ *Not ( );

  // Special Unary Functions
  virtual OPOBJ *WithinFile(const STRING& FileSpec);
  virtual OPOBJ *WithinDoctype(const STRING& DoctypeNameSpec);
  virtual OPOBJ *WithKey(const STRING& KeySpec);

  virtual OPOBJ *Not (const STRING& FieldName);

  // Unary
  virtual OPOBJ *Reduce(const float Metric=0.0);
  virtual OPOBJ *Trim(const float Metric=0.0);
  virtual OPOBJ *HitCount(const float Metric=0.0);

  // Score = Score * Weight
  OPOBJ *BoostScore (const float Weight = 1);

 // Unary Sort
  virtual OPOBJ *SortBy (const STRING& ByWhat);

  // Distance metric operator (Distance >= 0 near, Distance < 0 far)
  OPOBJ *CharProx (const OPOBJ& OtherIrset, const float Metric,
	DIR_T dir = OPOBJ::BEFOREorAFTER);
  // Convienience
  OPOBJ *WithinXChars (const OPOBJ& OtherIrset, float Metric = 50) {
     return CharProx (OtherIrset, Metric, BEFOREorAFTER);
  }
  OPOBJ *WithinXChars_Before (const OPOBJ& OtherIrset, float Metric = 50) {
     return CharProx (OtherIrset, Metric, BEFORE);
  }
  OPOBJ *WithinXChars_After (const OPOBJ& OtherIrset, float Metric = 50) {
       return CharProx (OtherIrset, Metric, AFTER);
  }
  OPOBJ *WithinXPercent (const OPOBJ& OtherIrset, float Metric) {
     if (Metric >= 100.0)
	return And(OtherIrset);
     return CharProx (OtherIrset, Metric/100, BEFOREorAFTER);
  }
  OPOBJ *WithinXPercent_Before (const OPOBJ& OtherIrset, float Metric) {
     if (Metric >= 100.0)
	return Before(OtherIrset);
     return CharProx (OtherIrset, Metric/100, BEFORE);
  }
  OPOBJ *WithinXPercent_After (const OPOBJ& OtherIrset, float Metric) {
     if (Metric >= 100.0)
	return After(OtherIrset);
     return CharProx (OtherIrset, Metric/100, AFTER);
  }

  void  setPrivateSortUserData(void *ptr) { userData = ptr; }
  void *getPrivateSortUserData() const    { return userData; }

  void SortBy(int (*func)(const void *, const void *)) {
    installSortFunction (func);
    SortByFunction(func);
  }
  void installSortFunction(int (*func)(const void *, const void *)) {
    if (IrsetCompar != func)
      {
	if (Sort == ByFunction && func) Sort = Unsorted;
	IrsetCompar = func;
      }
  }

  void SortBy(enum SortBy SortBy) {
    if      (SortBy == ByScore)       SortByScore();
    else if (SortBy == ByHits)        SortByHits();
    else if (SortBy == ByReverseHits) SortByReverseHits();
    else if (SortBy == ByAuxCount)    SortByAuxCount();
    else if (SortBy == ByIndex)       SortByIndex();
    else if (SortBy == ByDate)        SortByDate();
    else if (SortBy == ByKey)         SortByKey();
    else if (SortBy == ByReverseDate) SortByReverseDate();
    else if (SortBy == ByNewsrank)    SortByNewsrank();
    else if (SortBy == ByFunction)    SortByFunction(IrsetCompar);
    else if (SortBy >= ByPrivate && SortBy < ByExtIndex)
	SortByPrivate((int)SortBy-(int)ByPrivate);
    else if (SortBy >= ByExtIndex && SortBy <= ByExtIndexLast)
	SortByExtIndex(SortBy);
    SortRequest = SortBy;
  }
  INT GetSort() const { return (INT)Sort; }

  void SetParent (PIDBOBJ const NewParent) { Parent = NewParent; }
  PIDBOBJ GetParent () const               { return Parent;      }

  DOUBLE GetMaxScore() const { return MaxScore; }
  DOUBLE GetMinScore() const { return MinScore; }

  void   SetMaxEntriesAdvice (size_t nMax) { MaxEntriesAdvice=nMax; }
  size_t GetMaxEntriesAdvice () const      { return MaxEntriesAdvice; }

  void Write (PFILE fp) const;
  GDT_BOOLEAN Read (PFILE fp);

  void Dump(INT Skip =0, ostream& os = cout) const {
   Fill (1, TotalEntries, NULL)->Dump(Skip, os);
  };

  ~atomicIRSET();
private:
  typedef  GDT_BOOLEAN (*peer_t) (const FC&, const FC&);
  OPOBJ   *Peer (const OPOBJ& OtherIrset, peer_t Func);
  OPOBJ   *Within(const OPOBJ& OtherIrset, const STRING& Fieldname,  peer_t Func);
  GDT_BOOLEAN FieldExists(const STRING& FieldName);
  void     Clear();
  PIRESULT StealTable();
  size_t   FindByMdtIndex(size_t Index) const;
  void     SortByScore ();
  void     SortByHits ();
  void     SortByReverseHits ();
  void     SortByAuxCount ();
  void     SortByIndex ();
  void     SortByDate ();
  void     SortByReverseDate ();
  void     SortByKey ();
  void     SortByNewsrank ();
  void     SortByFunction (int (*func)(const void *, const void *));
  void     SortByPrivate (int n);
  void     SortByExtIndex(enum SortBy SortByWhich);
  OPOBJ   *_Or (const OPOBJ& OtherIrset);
  OPOBJ   *_And (const OPOBJ& OtherIrset, size_t Limit=0);
  OPOBJ   *_Xor (const OPOBJ& OtherIrset);
  OPOBJ   *_AndNot (const OPOBJ& OtherIrset);
  OPOBJ   *_CharProx (const OPOBJ& OtherIrset, const float Metric, DIR_T dir = BEFOREorAFTER);
  void     Set(const atomicIRSET *OtherPtr);

  IRESULT *Table;
  size_t   TotalEntries;
  size_t   MaxEntries;
  size_t   MaxEntriesAdvice;
  size_t   HitTotal;
  size_t   Increment;
  long     allocs;
  enum NormalizationMethods ComputedS;

  DOUBLE   MaxScore, MinScore;
  enum SortBy Sort, SortRequest;
  PIDBOBJ  Parent;
  int     (*IrsetCompar)(const void *, const void *);
  void    *userData; // For private Sort
};

extern "C" int (* __Private_IRSET_Sort) (void *Table, int Total, void *IDBParent, int which, void *UserData);


typedef atomicIRSET* PatomicIRSET;

// Common functions

inline void Write(const atomicIRSET Irset, PFILE Fp)
{
  Irset.Write(Fp);
}


inline GDT_BOOLEAN Read(PatomicIRSET IrsetPtr, PFILE Fp)
{
  return IrsetPtr->Read(Fp);
}

/////////////////////////////////////////////////////////////////
// Freestore management for CACHED atomicIRSETs 
/////////////////////////////////////////////////////////////////

class atomicIRSETptr {
  public:
    atomicIRSETptr(const PIDBOBJ DbParent, size_t Reserve) {
      count_ = 1;
      ptr_ = new atomicIRSET(DbParent, Reserve);
    }
    atomicIRSETptr(const atomicIRSET& Irset) {
      count_ = 1;
      ptr_ = new atomicIRSET(Irset);
    }
   ~atomicIRSETptr() { delete ptr_; }
  private:
    friend class   _IRSET;
    atomicIRSET         *ptr_;
    signed   int   count_;      // reference count
};

///////////////////////////////////////////////////////////////
/// _IRSET: atomicIRSET using "smart" pointers
///////////////////////////////////////////////////////////////

class _IRSET :  public OPERAND {
public:
  atomicIRSETptr* operator-> () { return p_; }
  atomicIRSETptr& operator* ()  { return *p_; }

  _IRSET(const PIDBOBJ DbParent = NULL, size_t Reserve = 0) {
    p_ = new atomicIRSETptr( DbParent, Reserve);
  }
  _IRSET (const atomicIRSET& OtherIrset) {
    p_ = new atomicIRSETptr(OtherIrset);
  }
  _IRSET (const OPOBJ& OtherIrset) {
    p_ = new atomicIRSETptr(OtherIrset);
  }

  _IRSET(const _IRSET& Irset) {
     ++Irset.p_->count_;
     p_ = Irset.p_;
  }
/*
  operator const atomicIRSET*() const { return p_->ptr_;    }
  operator const atomicIRSET() const  { return *(p_->ptr_); }
*/
  operator const OPOBJ*() const       { return p_->ptr_;    }
//operator const OPOBJ() const        { return *(p_->ptr_); }

  _IRSET& operator= (const _IRSET& Irset) {
    ++Irset.p_->count_;
    unlock();
    p_ = Irset.p_;
    return *this;
  }

  _IRSET& Concat (const _IRSET& OtherIrset) {
    node()->Concat(OtherIrset);
    return *this;
  }

  OPOBJ* Duplicate () const { return (OPOBJ *)this; }
  _IRSET* Duplicate()       { lock(); return this;  }

  INT    GetSort() const         { return p_->ptr_->GetSort();         }
  DOUBLE GetMaxScore() const     { return p_->ptr_->GetMaxScore();     }
  DOUBLE GetMinScore() const     { return p_->ptr_->GetMinScore();     }
  size_t GetHitTotal () const    { return p_->ptr_->GetHitTotal();     }
  size_t GetTotalEntries() const { return p_->ptr_->GetTotalEntries(); }
  size_t SetTotalEntries(size_t NewTotal) {
    return (p_->ptr_ = node())->SetTotalEntries(NewTotal); }
  t_Operand GetOperandType () const{ return p_->ptr_->GetOperandType();}

  PRSET GetRset () const               { return p_->ptr_->GetRset();      }
  PRSET GetRset (size_t Total) const   { return p_->ptr_->GetRset(Total); }
  // Getting RSet in two passes...
  PRSET GetRset(size_t Start, size_t End) const {
    return p_->ptr_->GetRset(Start, End); }
  PRSET Fill(size_t Start, size_t End = 0, PRSET set = NULL) const {
    return p_->ptr_->Fill(Start, End, set); }

  void LoadTable (const STRING& FileName)          { nnode()->LoadTable(FileName); }
  void SaveTable (const STRING& FileName) const    { p_->ptr_->SaveTable(FileName); }

  void Resize (const size_t Entries)  { if (Entries >= GetTotalEntries()) p_->ptr_->Resize(Entries);
    else node()->Resize(Entries); }
  void AddEntry (const IRESULT& ResultRecord, const GDT_BOOLEAN Add) {
    node()->AddEntry(ResultRecord, Add); }
  void FastAddEntry(const IRESULT& ResultRecord) { node()->FastAddEntry(ResultRecord); }

  OPOBJ *ComputeScores (const INT TermWeight) {
        return node()->ComputeScoresCosineNormalization(TermWeight); }
  OPOBJ *ComputeScores (const INT TermWeight, enum NormalizationMethods Method) {
	return node()->ComputeScores(TermWeight, Method); }

  void MergeEntries(GDT_BOOLEAN Add = GDT_TRUE)       { p_->ptr_->MergeEntries(Add);      }
  void Adjust_Scores()                                { p_->ptr_->Adjust_Scores();        }
  GDT_BOOLEAN GetMdtrec(size_t i, MDTREC *m) const    { return p_->ptr_->GetMdtrec(i, m); }
  void SetMdt(const IDBOBJ *Idb)                      { p_->ptr_->SetMdt(Idb);            }
  void SetMdt(const MDT *NewMdt)                      { p_->ptr_->SetMdt(NewMdt);         }
  const MDT *GetMdt(size_t i) const                   { return p_->ptr_->GetMdt(i);       }

  // Binary Functions
   virtual OPOBJ *Or (const _IRSET& OtherIrset)       { return node()->Or (OtherIrset);       }
   virtual OPOBJ *Nor (const _IRSET& OtherIrset)      { return node()->Nor (OtherIrset);      }
   virtual OPOBJ *And (const _IRSET& OtherIrset)      { return node()->And (OtherIrset);      }
   virtual OPOBJ *Nand (const _IRSET& OtherIrset)     { return node()->Nand (OtherIrset);     }
   virtual OPOBJ *AndNot (const _IRSET& OtherIrset)   { return node()->AndNot (OtherIrset);   }
   virtual OPOBJ *Xor (const _IRSET& OtherIrset)      { return node()->Xor (OtherIrset);      }
   virtual OPOBJ *Near (const _IRSET& OtherIrset)     { return node()->Near (OtherIrset);     }
   virtual OPOBJ *Far (const _IRSET& OtherIrset)      { return node()->Far (OtherIrset);      }
   virtual OPOBJ *After (const _IRSET& OtherIrset)    { return node()->After (OtherIrset);    }
   virtual OPOBJ *Before (const _IRSET& OtherIrset)   { return node()->Before (OtherIrset);   }
   virtual OPOBJ *Adj (const _IRSET& OtherIrset)      { return node()->Adj (OtherIrset);      }
   virtual OPOBJ *Follows (const _IRSET& OtherIrset)  { return node()->Follows (OtherIrset);  }
   virtual OPOBJ *Precedes (const _IRSET& OtherIrset) { return node()->Precedes (OtherIrset); }
   virtual OPOBJ *Neighbor (const _IRSET& OtherIrset) { return node()->Neighbor (OtherIrset); }
  // Distance metric operator (Distance >= 0 near, Distance < 0 far)
   virtual OPOBJ *CharProx (const _IRSET& OtherIrset, const float Metric, DIR_T dir = BEFOREorAFTER) {
     return node()->CharProx(OtherIrset, Metric, dir);
   }

  virtual GDT_BOOLEAN IsEmpty() const { return p_->ptr_->IsEmpty(); }

  // Unary Functions
   virtual OPOBJ *Not ( )                             { return node()->Not ();                }

   virtual OPOBJ *Reduce(const float Metric)          { return node()->Reduce(Metric);        }
   virtual OPOBJ *Reduce(const int Metric=0)          { return node()->Reduce((float)Metric); }

  void SortBy(enum SortBy SortBy) { node()->SortBy(SortBy);     }

  GDT_BOOLEAN GetEntry(const size_t Index, IRESULT* ResultRecord) const {
    return p_->ptr_->GetEntry(Index, ResultRecord);
  }
  const IRESULT operator[](size_t n) const { return (*(p_->ptr_))[n]; }
  IRESULT& operator[](size_t n)            { return (*(p_->ptr_))[n]; }

  void Write(PFILE fp) const { p_->ptr_->Write(fp);     }
  GDT_BOOLEAN Read(PFILE fp) { return nnode()->Read(fp); }
  ~_IRSET() { unlock(); }
 private:
  void    lock() { p_->count_++; }
  void    unlock() {
    if (--p_->count_ == 0)
      delete p_;
  }
  atomicIRSET * node() {
    if (p_->count_ > 1)
      {
	if (p_->ptr_)
	  {
	    // detach ...
	    unlock(); // decrement count
	    p_ = new atomicIRSETptr( *(p_->ptr_) );
	  }
	else
	  {
	    // This should never happen!
	    logf (LOG_PANIC, "Stray IRSET in atomicIRSET *node() [count=%d]", p_->count_);
	    p_ = new atomicIRSETptr ( p_->ptr_->GetParent () );
	  }
      }
    return p_->ptr_;
  }
  // Create a new node if needed but don't bother copying
  atomicIRSET *nnode() {
    if (p_->count_ > 1)
      {
        // detach ...
        unlock(); // decrement count
        p_ = new atomicIRSETptr(  p_->ptr_->GetParent ());
      }
    return p_->ptr_;
  }
  atomicIRSETptr *p_;
};

typedef _IRSET* P_IRSET;

#if 0
# define IRSET  _IRSET
# define PIRSET P_IRSET
#else
# define IRSET atomicIRSET
# define PIRSET PatomicIRSET
#endif

#endif
