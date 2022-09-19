#pragma ident  "@(#)vlist.cxx  1.20 06/27/00 20:32:03 BSN"

/************************************************************************
************************************************************************/

/*-@@@
File:		vlist.cxx
Version:	1.00
Description:	Class VLIST - Doubly Linked Circular List Base Class
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include "vlist.hxx"

//#include <iostream>
//#include <fstream>

using namespace std;

VLIST::VLIST ()
{
  Next = Prev = this; // a circle of one
}

GDT_BOOLEAN VLIST::IsEmpty() const
{
  return (Next == NULL || Next == this);
}

size_t VLIST::GetTotalEntries () const
{
  size_t count = 0;
  for (const VLIST *p = Next; p != this; p = p->Next)
   {
     count++;
   }
  return count;
}

void VLIST::AddNode (VLIST *NewEntryPtr)
{
  if (NewEntryPtr != NULL)
    {
      if (Prev != NULL)
	Prev->Next = NewEntryPtr;  // set last node to point to new node
      NewEntryPtr->Prev = Prev;  // set new node to point to last node
      Prev = NewEntryPtr;        // set top node to point to new node
      NewEntryPtr->Next = this;  // set new node to point to top node
    }
}

VLIST *VLIST::Disattach ()
{
  if (Prev != NULL)
    Prev->Next = Next;
  if (Next != NULL)
    Next->Prev = Prev;
  Prev = Next = this; // circle of one...
  return this;
}

VLIST *VLIST::GetNodePtr (const size_t Index) const
{
  size_t x = 0;
  for (VLIST *p = Next; p != this; p = p->Next)
    {
      if (++x == Index)
	return p;		// found it
    }
  return NULL;			// Index was never matched
}

void VLIST::Clear ()
{
  if (this->Next != this)
    {
      if (Prev)
	Prev = Prev->Next = NULL;	// disattach circle to isolate nodes
      delete this->Next;	// delete all subsequent nodes
      Next = Prev =  this;	// reattach circle of one
    }
}

void VLIST::EraseAfter (const size_t Index)
{
  VLIST *p = VLIST::GetNodePtr (Index);
  if (p && p->Next != this)
    {
      if (Prev)
	Prev = Prev->Next = NULL;	// disattach circle to isolate nodes
      delete p->Next;	// delete all subsequent nodes
      p->Next = this;	// reattach circle
      this->Prev = p;
    }
}

void VLIST::Reverse ()
{
  VLIST *p = this;
  VLIST *nextp;
  VLIST *temp;
  do {
    nextp = p->Next;
    temp = p->Prev;		// reverse pointers
    p->Prev = p->Next;
    p->Next = temp;
  } while ((p = nextp) != this);
}
#if 0
void VLIST::Sort( int (*compar) (const void *, const void *))
{
}
#endif

VLIST::~VLIST ()
{
  // Disattach from previous node
  if (Prev)
    Prev->Next = NULL;
  // Delete next node
  if (Next)
    delete Next;
  Next = Prev = this; // a circle of one
}
