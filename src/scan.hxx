/************************************************************************
************************************************************************/
#ifndef SCAN_HXX
#define SCAN_HXX 1

/*-@@@
File:		scan.hxx
Version:	1.00
Description:	Scan
Author:		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include <stdio.h>
#include "common.hxx"
#include "magic.hxx"
#include "strlist.hxx"

class SCANOBJ {
  friend class atomicSCANLIST;
public:
  SCANOBJ() {
    Count = 0;
  }
  SCANOBJ(const SCANOBJ& Other) {
    String = Other.String;
    Count  = Other.Count;
  }
  SCANOBJ(const STRING& Term, size_t Freq) {
    String = Term;
    Count  = Freq;
  }
  SCANOBJ& operator =(const SCANOBJ& Other) {
    String = Other.String;
    Count  = Other.Count;
    return *this;
  }
  size_t Frequency() const { return Count;  }
  STRING Term () const     { return String; }

  void   Dump(ostream& os = cout) const;

  void Write(FILE *fp) const;
  GDT_BOOLEAN Read(FILE *fp);

  ~SCANOBJ() { };
private:
  STRING String;
  UINT4  Count; 
};

class atomicSCANLIST : public VLIST {
public:
  atomicSCANLIST();

  void Add (const atomicSCANLIST *OtherList);
  atomicSCANLIST *AddEntry (const atomicSCANLIST& Entry);
  atomicSCANLIST *AddEntry (const STRING& StringEntry, size_t Frequency);

  atomicSCANLIST *AddEntry(const STRING& Words, const LISTOBJ *StopWords=NULL);
  atomicSCANLIST *AddEntry(const STRLIST& StrList);

  // Like AddEntry but don't sum frequency, instead use MAX count
  atomicSCANLIST *MergeEntry(const STRING& StringEntry, size_t Frequency);

  atomicSCANLIST   *Entry (const size_t Index) const;
  SCANOBJ     GetEntry (const size_t Index) const;
  GDT_BOOLEAN GetEntry (const size_t Index, STRING *StringEntry, size_t *Freq) const;

  // Iteration methods
  atomicSCANLIST       *Next()       { return (atomicSCANLIST *)VLIST::GetNextNodePtr();       }
  const atomicSCANLIST *Next() const { return (const atomicSCANLIST *)VLIST::GetNextNodePtr(); }
  atomicSCANLIST       *Prev()       { return (atomicSCANLIST *)VLIST::GetPrevNodePtr();       }
  const atomicSCANLIST *Prev() const { return (const atomicSCANLIST *)VLIST::GetPrevNodePtr(); }
  SCANOBJ        Value() const { return Scan;                                      }

  size_t Frequency() const { return Scan.Count;  }
  STRING Term () const     { return Scan.String; }

  size_t Search (const STRING& SearchTerm, size_t *Freq) const;
  size_t GetFrequency (const STRING& SearchTerm) const;
  size_t GetFrequency (const size_t Index) const;
  atomicSCANLIST *Find (const STRING &Term) const;
  void FastAddEntry (const atomicSCANLIST& Entry);
  void FastAddEntry (const SCANOBJ& Entry);
  void FastAddEntry (const STRING& StringEntry, size_t Frequency);

  size_t UniqueSort ();
  size_t UniqueSort (const size_t from, const size_t max, GDT_BOOLEAN add = GDT_TRUE);

  void   Dump(ostream& os = cout) const;

  void Write(FILE *fp) const;
  GDT_BOOLEAN Read(FILE *fp);

  void Save(const STRING& Filename) const;
  void Load(const STRING& Filename);

  ~atomicSCANLIST ();
private:
  SCANOBJ Scan;
};


/////////////////////////////////////////////////////////////////
// Freestore management for SCAN
/////////////////////////////////////////////////////////////////

class SCANLISTptr {
  public:
    SCANLISTptr() {
      count_ = 1;
      table_ = new atomicSCANLIST();
    }
    SCANLISTptr(const atomicSCANLIST *List) {
      count_ = 1;
      table_ = new atomicSCANLIST();
      table_-> Add(List);
    }
   ~SCANLISTptr() { delete table_; }
  private:
    friend class    SCANLIST;
    atomicSCANLIST *table_;
    signed   int    count_;        // reference count
};

///////////////////////////////////////////////////////////////
/// SCANLIST using "smart" pointers
///////////////////////////////////////////////////////////////

class SCANLIST {
public:
  SCANLISTptr* operator-> () { return p_; }
  SCANLISTptr& operator* ()  { return *p_; }

  SCANLIST() { p_ = new SCANLISTptr(); }
  SCANLIST(const SCANLIST& Table) {
     ++Table.p_->count_;
     p_ = Table.p_;
  }

  SCANLIST& operator= (const SCANLIST& Table) {
    ++Table.p_->count_;
    unlock();
    p_ = Table.p_;
    return *this;
  }

  void Add (const atomicSCANLIST *OtherList) {
    if (OtherList) node()->Add( OtherList );
  }
  void Add (const SCANLIST *OtherList) {
    if (OtherList) node()->Add( OtherList->p_->table_ );
  }
  void Add (const SCANLIST& OtherList) {
    if (OtherList) node()->Add( OtherList.p_->table_ );
  }

  SCANLIST& operator +=(const SCANLIST& OtherList) {
    Add(OtherList);
    return *this;
  }

  atomicSCANLIST   *AddEntry(const atomicSCANLIST& Entry) {
    return node()->AddEntry(Entry);
  }
  atomicSCANLIST   *AddEntry(const STRING& StringEntry, size_t Frequency) {
    return node()->AddEntry(StringEntry, Frequency);
  }

  atomicSCANLIST *AddEntry(const STRING& Words, const LISTOBJ *StopWords=NULL) {
    return node()->AddEntry(Words, StopWords);
  }

  atomicSCANLIST *AddEntry(const STRLIST& StrList) {
    return node()->AddEntry(StrList);
  }

  atomicSCANLIST   *AddEntry(const SCANLIST& Table) {
    if (p_->count_ == 1 && p_->table_->IsEmpty())
      {
	*this = Table;
	return p_->table_;
      }
    return node()->AddEntry( Table.GetatomicSCANLIST() );
  }

  atomicSCANLIST   *MergeEntry(const STRING& StringEntry, size_t Frequency) {
    return node()->MergeEntry(StringEntry, Frequency);
  }

  const atomicSCANLIST&  GetatomicSCANLIST() const { return *(p_->table_); }
  const atomicSCANLIST  *GetPtratomicSCANLIST() const { return p_->table_; }
  operator const atomicSCANLIST*() const { return GetPtratomicSCANLIST(); } 

  void Clear() {
    if (p_->count_ <= 1) {
      p_->table_->Clear(); // Just me so don't need to create
    } else {
      // Don't need to copy to then just to clear 
      unlock(); // decrement count
      p_ = new SCANLISTptr( );
    }
  }
  void Reverse()       { node()->Reverse(); }
  size_t UniqueSort () { return node()->UniqueSort();  }
  size_t UniqueSort (const size_t from, const size_t max, GDT_BOOLEAN add = GDT_TRUE) {
    return node()->UniqueSort(from, max, add);
  }

  GDT_BOOLEAN IsEmpty() const { return p_->table_->IsEmpty(); }
  size_t GetTotalEntries() const { return p_->table_->GetTotalEntries(); }

  GDT_BOOLEAN GetEntry(const size_t Index, STRING *StringEntry, size_t *Freq) const {
    return p_->table_->GetEntry(Index, StringEntry, Freq);
  }
  SCANOBJ     GetEntry (const size_t Index) const {
     return p_->table_->GetEntry(Index);
  }

  void   Dump(ostream& os = cout) const { p_->table_->Dump(os); }

  void        Write(FILE *fp) const { p_->table_->Write(fp);   }
  GDT_BOOLEAN Read(FILE *fp)        { return node()->Read(fp); }


  void   Save(const STRING& Filename) const { p_->table_->Save(Filename); }
  void   Load(const STRING& Filename)       { node()->Load(Filename); }

  ~SCANLIST() { unlock(); }
 private:
  void    lock() { p_->count_++; }
  void    unlock() {
    if (--p_->count_ == 0)
      delete p_;
  }
  atomicSCANLIST * node() {
    if (p_->count_ > 1)
      {
	// detach ...
	unlock(); // decrement count
	p_ = new SCANLISTptr( p_->table_ );
      }
    return p_->table_;
  }
  SCANLISTptr *p_;
};

#endif
