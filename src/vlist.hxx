/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		vlist.hxx
Description:	Class VLIST - Doubly Linked Circular List Base Class
@@@*/

#ifndef VLIST_HXX
#define VLIST_HXX

#include "gdt.h"

class VLIST {
public:
  VLIST();
  virtual bool IsEmpty() const;
  virtual void   Clear();
  virtual void   Empty() { Clear(); };
  virtual void   EraseAfter(const size_t Index);
  virtual void   Reverse();
  virtual size_t GetTotalEntries() const;

  virtual ~VLIST();
protected:
  virtual void   AddNode(VLIST* NewEntryPtr);
  virtual VLIST* GetNodePtr(const size_t Index) const;
  virtual VLIST* GetNextNodePtr() const { return Next; }
  virtual VLIST* GetPrevNodePtr() const { return Prev; }
  virtual VLIST* Disattach();
private:
  VLIST* Next; // forward link
  VLIST* Prev; // backward link
};

#endif
