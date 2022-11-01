/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:           nodetree.hxx
Version:        1.30
Description:    Class NODELIST - Node Lists 
Author:         Edward C. Zimmermann
@@@*/

#ifndef NODETREE_HXX
#define NODETREE_HXX

#include "defs.hxx"
#include "fc.hxx"
#include "vlist.hxx"


class TREENODE {
public:
 TREENODE() { }
 TREENODE(const FC& fc, const STRING& name) { myFc = fc; myName = name; }

 ~TREENODE() { }

 TREENODE& operator  =(const TREENODE& OtherNode) {
  myFc   = OtherNode.myFc;
  myName = OtherNode.myName;
  return *this;
 }

 STRING  Name() const { return myName; }
 FC      Fc()   const { return myFc;   }

 void    Print (ostream& Os) const;

 friend ostream& operator<<(ostream& os, const TREENODE& Node);

private:
 FC     myFc;
 STRING myName;
};


class TREENODELIST : public VLIST {
public:
  TREENODELIST() :VLIST () { }
  TREENODELIST(const TREENODELIST& List) :VLIST () {
    *this = List;
  }
  TREENODELIST& operator =(const TREENODELIST& List);

  TREENODELIST       *AddEntry(const TREENODE& Record);
  TREENODELIST       *AddEntry(const TREENODELIST& List);
  TREENODELIST       *AddEntry(const TREENODELIST* ListPtr);

  const TREENODE&   GetEntry(const size_t Index) const;

  void                Print (ostream& Os) const;

  STRING              XMLNodeTree(const STRING& Content = NulString) const;

  // Iteration methods
  TREENODELIST       *Next()        { return (TREENODELIST *)GetNextNodePtr();       }
  const TREENODELIST *Next() const  { return (const TREENODELIST *)GetNextNodePtr(); }
  TREENODELIST       *Prev()        { return (TREENODELIST *)GetPrevNodePtr();       }
  const TREENODELIST *Prev() const  { return (const TREENODELIST *)GetPrevNodePtr(); }
  TREENODE            Value() const { return Node;                               }

  int             Sort ();

  ~TREENODELIST() { }

  friend ostream& operator<<(ostream& os, const TREENODELIST& List);
private:
  TREENODE   Node;
};


/////////////////////////////////////////////////////////////////
// Freestore management for NODETREES
/////////////////////////////////////////////////////////////////

class TREENODELISTptr {
  public:
    TREENODELISTptr() {
      count_ = 1;
      unsorted_ = 0;
      table_ = new TREENODELIST();
    }
    TREENODELISTptr(const TREENODELIST& List) {
      count_ = 1;
      unsorted_ = 1;
      table_ = new TREENODELIST(List);
    }
    bool IsEmpty() const {
      return table_->IsEmpty();
    }
    size_t GetTotalEntries() const {
      return table_->GetTotalEntries(); ;
    }
   ~TREENODELISTptr() { delete table_; count_ = 0; }
  private:
    friend class   NODETREE;
    TREENODELIST  *table_;
    int            unsorted_;
    signed   int   count_;        // reference count
};


///////////////////////////////////////////////////////////////
/// TREENODELIST using "smart" pointers
///////////////////////////////////////////////////////////////

class NODETREE {
public:
  TREENODELISTptr* operator-> () { return p_; }
  TREENODELISTptr& operator* ()  { return *p_; }

  NODETREE() {
    p_ = new TREENODELISTptr();
  }
  NODETREE(const NODETREE& table) {
     ++table.p_->count_;
     p_ = table.p_;
  }
  NODETREE(const TREENODELIST& List) {
    p_ = new TREENODELISTptr(List);
  }

  NODETREE& operator= (const NODETREE& table) {
    if (table.p_ == NULL)
      {
	message_log (LOG_PANIC, "NODETREE pointer backstore nulled!?");
	Clear();
	message_log (LOG_DEBUG, "Cleared");
	return *this;
      }
    ++table.p_->count_;
    unlock();
    p_ = table.p_;
    return *this;
  }

  TREENODELIST   *AddEntry(const TREENODE& Node) {
    return node()->AddEntry(Node);
  }
  TREENODELIST   *AddEntry(const TREENODELIST& List) {
    return node()->AddEntry(List);
  }
  TREENODELIST   *AddEntry(const TREENODELIST *ListPtr) {
    return node()->AddEntry(ListPtr);
  }
  TREENODELIST   *AddEntry(const NODETREE& table) {
    if (p_->count_ == 1 && p_->IsEmpty())
      {
	*this = table;
	return p_->table_;
      }
    return node()->AddEntry( table.GetTREENODELIST() );
  }
  const TREENODELIST&  GetTREENODELIST() const { return *(p_->table_); }
  const TREENODELIST  *GetPtrTREENODELIST() const { return p_->table_; }
  operator const TREENODELIST*() const { return GetPtrTREENODELIST(); } 

  void Clear() {
    if (p_->count_ <= 1) {
      p_->table_->Clear(); // Just me so don't need to create
      p_->unsorted_ = 0;
    } else {
      // Don't need to copy to then just to clear 
      unlock(); // decrement count
      p_ = new TREENODELISTptr( );
    }
  }
  void Reverse() { node()->Reverse(); }
  void Sort() {
    if (p_->unsorted_)
      {
	node()->Sort();
	p_->unsorted_ = 0;
      }
  }
  bool IsSorted() const {
    return p_ && p_->unsorted_ == 0; }
  bool IsEmpty() const  {
    return p_ == NULL || p_->IsEmpty();    }
  size_t GetTotalEntries() const {
    return IsEmpty() ? 0 : p_->GetTotalEntries();
  }
  const TREENODE& GetEntry(const size_t Index) const {
    return p_->table_->GetEntry(Index);
  }

  void    Print (ostream& Os) const {
    p_->table_->Print(Os);
  }

  STRING  XMLNodeTree(const STRING& Content = NulString) const {
   return  p_->table_->XMLNodeTree(Content);
  }


  int Refcount_() const { return p_ ?  p_->count_  : -1; }
  ~NODETREE() { unlock(); }

 friend ostream& operator<<(ostream& os, const NODETREE& Tree);

 private:
  void    lock() { p_->count_++; }
  void    unlock() {
    if (--p_->count_ == 0)
      delete p_;
  }
  TREENODELIST * node() {
    if (p_->count_ > 1)
      {
	// detach ...
	unlock(); // decrement count
	p_ = new TREENODELISTptr( *(p_->table_) );
      }
    p_->unsorted_ = 1;
    return p_->table_;
  }
  TREENODELISTptr *p_;
};


#endif
