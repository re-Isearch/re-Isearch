/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		slist.hxx
Version:	1.00
Description:	Class SLIST - Single Linked Circular List Base Class
Author:		Edward C. Zimmermann
@@@*/

#ifndef SLIST_HXX
#define SLIST_HXX

#include "gdt.h"

class SLIST {
public:
  SLIST() { Next = this; } // a circle of one
  virtual GDT_BOOLEAN IsEmpty() const { return (Next == this); }
  virtual void Clear();
  virtual void EraseAfter(const size_t Index);
  virtual void Reverse();
  virtual size_t GetTotalEntries() const;
  virtual ~SLIST();
protected:
  virtual void AddNode(SLIST* NewEntryPtr);
  virtual SLIST* GetNodePtr(const size_t Index) const;
  virtual SLIST* GetNextNodePtr() const { return Next; }
private:
  SLIST* Next; // forward link
};

#endif
