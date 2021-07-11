/*@@@
File:		fct.hxx
Version:	1.30
Description:	Class FCLIST - Field Coordinate Table
Author:		Edward C. Zimmermann
@@@*/

#ifndef FCLIST_HXX
#define FCLIST_HXX

#include "defs.hxx"
#include "fc.hxx"
#include "vlist.hxx"

class FCLIST : public VLIST {
public:
  FCLIST();
  FCLIST(const FCLIST& OtherFct);
  FCLIST(const FC& Fc);

  FCLIST& operator  =(const FCLIST& OtherFct);
  FCLIST& operator  =(const FC& Fc);

  FCLIST& operator +=(const GPTYPE GpOffset);
  FCLIST& operator +=(const FC& Fc);
  FCLIST& operator -=(const GPTYPE GpOffset);
  FCLIST& operator -=(const FC& Fc);

  FCLIST   *AddEntry(const FC& FcRecord);
  FCLIST   *AddEntry(const FCLIST& Fct);
  FCLIST   *AddEntry(const FCLIST *FctPtr);

  void      MergeEntries();

  GDT_BOOLEAN GetEntry(const size_t Index, FC* FcRecord) const;
  const FC&   GetEntry(const size_t Index) const; 

  // Iteration methods
  FCLIST       *Next()        { return (FCLIST *)GetNextNodePtr();       }
  const FCLIST *Next() const  { return (const FCLIST *)GetNextNodePtr(); }
  FCLIST       *Prev()        { return (FCLIST *)GetPrevNodePtr();       }
  const FCLIST *Prev() const  { return (const FCLIST *)GetPrevNodePtr(); }
  FC           Value() const  { return Fc;                               }

  void SortByFc();
  void Write(PFILE fp) const;
  GDT_BOOLEAN Read(PFILE fp);
  void Print(ostream& Os) const;

  void SubtractOffset(const GPTYPE GpOffset); // Isearch Compat. function

  friend ostream& operator<<(ostream& os, const FCLIST& Fct);

  size_t WriteFct(FILE *fp, GPTYPE GpOffset) const;

  ~FCLIST();
private:
  FC Fc;
};


// Common Functions
inline void Write(const FCLIST& Fct, PFILE Fp)
{
  Fct.Write(Fp);
}

inline GDT_BOOLEAN Read(FCLIST *FctPtr, PFILE Fp)
{
  return FctPtr->Read(Fp);
}

/////////////////////////////////////////////////////////////////
// Freestore management for FCT
/////////////////////////////////////////////////////////////////

class FCLISTptr {
  public:
    FCLISTptr() {
      count_ = 1;
      unsorted_ = 0;
      table_ = new FCLIST();
    }
    FCLISTptr(const FCLIST& Fct) {
      count_ = 1;
      unsorted_ = 1;
      table_ = new FCLIST(Fct);
    }
    FCLISTptr(const FC& Fc) {
      count_ = 1;
      unsorted_ = 0;
      table_ = new FCLIST(Fc);
    }
    GDT_BOOLEAN IsEmpty() const {
      return table_->IsEmpty();
    }
    size_t GetTotalEntries() const {
      return table_->GetTotalEntries(); ;
    }
   ~FCLISTptr() { delete table_; count_ = 0; }
  private:
    friend class   FCT;
    FCLIST        *table_;
    int            unsorted_;
    signed   int   count_;        // reference count
};

///////////////////////////////////////////////////////////////
/// FCLIST using "smart" pointers
///////////////////////////////////////////////////////////////


#ifdef DEBUG_MEMORY
extern long __IB_FCT_allocated_count; // Used to track stray FCLISTs
extern long __IB_FCLIST_allocated_count;
#endif

class FCT {
public:
  FCLISTptr* operator-> () { return p_; }
  FCLISTptr& operator* ()  { return *p_; }

  FCT() {
    p_ = new FCLISTptr();
#ifdef DEBUG_MEMORY
    __IB_FCT_allocated_count++;
#endif
  }
  FCT(const FCLIST& Fct) {
    p_ = new FCLISTptr(Fct);
#ifdef DEBUG_MEMORY
    __IB_FCT_allocated_count++;
#endif
  }
  FCT(const FC& Fc)      {
    p_ = new FCLISTptr(Fc);
#ifdef DEBUG_MEMORY
    __IB_FCT_allocated_count++;
#endif
  }
  FCT(const FCT& Fctable) {
     ++Fctable.p_->count_;
     p_ = Fctable.p_;
  }
  FCT& operator= (const FCT& Fctable) {
    if (Fctable.p_ == NULL)
      {
	logf (LOG_PANIC, "FCT pointer backstore nulled!?");
	Clear();
	logf (LOG_DEBUG, "Cleared");
	return *this;
      }
    ++Fctable.p_->count_;
    unlock();
    p_ = Fctable.p_;
    return *this;
  }
  FCT& operator +=(const GPTYPE GpOffset) {
     *node() += GpOffset;
     return *this;
  }
  FCT& operator +=(const FC& Fc) {
     *node() += Fc;
     return *this;
  }
  FCT& operator -=(const GPTYPE GpOffset) {
     *node() -= GpOffset;
     return *this;
  }
  FCT& operator -=(const FC& Fc) {
     *node() -= Fc;
     return *this;
  }

  FCLIST   *AddEntry(const FC& FcRecord) {
    return node()->AddEntry(FcRecord);
  }
  FCLIST   *AddEntry(const FCLIST& Fct) {
    return node()->AddEntry(Fct);
  }
  FCLIST   *AddEntry(const FCLIST *FctPtr) {
    return node()->AddEntry(FctPtr);
  }
  FCLIST   *AddEntry(const FCT& Fctable) {
    if (p_->count_ == 1 && p_->IsEmpty())
      {
	*this = Fctable;
	return p_->table_;
      }
    return node()->AddEntry( Fctable.GetFCLIST() );
  }

  void      MergeEntries() { node()->MergeEntries(); }

  void Print(ostream& Os) const { p_->table_->Print(Os); }

  const FCLIST&  GetFCLIST() const { return *(p_->table_); }
  const FCLIST  *GetPtrFCLIST() const { return p_->table_; }
  operator const FCLIST*() const { return GetPtrFCLIST(); } 

  void Clear() {
    if (p_->count_ <= 1) {
      p_->table_->Clear(); // Just me so don't need to create
      p_->unsorted_ = 0;
    } else {
      // Don't need to copy to then just to clear 
      unlock(); // decrement count
      p_ = new FCLISTptr( );
#ifdef DEBUG_MEMORY
      __IB_FCT_allocated_count++;
#endif
    }
  }
  void Reverse() { node()->Reverse(); }
  void SortByFc() {
    if (p_->unsorted_)
      {
	node()->SortByFc();
	p_->unsorted_ = 0;
      }
  }
  GDT_BOOLEAN IsSorted() const {
    return p_ && p_->unsorted_ == 0; }
  GDT_BOOLEAN IsEmpty() const  {
    return p_ == NULL || p_->IsEmpty();    }
  size_t GetTotalEntries() const {
    return IsEmpty() ? 0 : p_->GetTotalEntries();
  }
  GDT_BOOLEAN GetEntry(const size_t Index, FC* FcRecord) const {
    return p_->table_->GetEntry(Index, FcRecord);
  }
  const FC& GetEntry(const size_t Index) const {
    return p_->table_->GetEntry(Index);
  }

#if 0
  const FC *FirstElementPtr() {
    cursor = p_->table_->Next();
    return &(cursor->Value());
  }
  const FC *NextElementPtr() {
    return &((cursor = cursor->Next())->Value());
  }
  const FC *PrevElementPtr() {
    return &((cursor = cursor->Prev())->Value());
  }
  GDT_BOOLEAN atEndOfList() {
    return cursor == p_->table_;
  }
#endif

  friend inline ostream& operator<<(ostream& os, const FCT& Fct) {
    return os << Fct.GetFCLIST();
  }

  void Write(PFILE fp) const {
    p_->table_->Write(fp);
  }
  size_t WriteFct(FILE *fp, GPTYPE GpOffset) const {
    return p_->table_->WriteFct(fp, GpOffset);
  }
  GDT_BOOLEAN Read(PFILE fp) {
    return node()->Read(fp);
  }
  int Refcount_() const { return p_ ?  p_->count_  : -1; }
  ~FCT() { unlock(); }
 private:
  void    lock() { p_->count_++; }
  void    unlock() {
    if (--p_->count_ == 0)
      {
        delete p_;
#ifdef DEBUG_MEMORY
	__IB_FCT_allocated_count--;
#endif
      }
  }
  FCLIST * node() {
    if (p_->count_ > 1)
      {
	// detach ...
	unlock(); // decrement count
	p_ = new FCLISTptr( *(p_->table_) );
#ifdef DEBUG_MEMORY
	__IB_FCT_allocated_count++;
#endif
      }
    p_->unsorted_ = 1;
    return p_->table_;
  }
  FCLISTptr *p_;
#if 0
  FCLIST    *cursor;
#endif
};

// Common Functions
inline void Write(const FCT& Fctable, PFILE Fp)
{
  Fctable.Write(Fp);
}

inline GDT_BOOLEAN Read(FCT *FctablePtr, PFILE Fp)
{
  return FctablePtr->Read(Fp);
}

#endif
