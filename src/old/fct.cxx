/*-@@@
File:		fct.cxx
Version:	1.21
Description:	Class FCT - Field Coordinate Table
Author:		Edward C. Zimmermann, edz@bsn.com
@@@*/

#include <stdlib.h>
#include "fct.hxx"
#include "string.hxx"
#include "magic.hxx"

#pragma ident  "@(#)fct.cxx  1.21 05/08/01 21:36:39 BSN"

#ifdef DEBUG_MEMORY
long __IB_FCT_allocated_count = 0;
long __IB_FCLIST_allocated_count = 0;
#endif

FCLIST::FCLIST ():VLIST ()
{
#ifdef DEBUG_MEMORY
  __IB_FCLIST_allocated_count++;
#endif
}

FCLIST::FCLIST (const FCLIST& OtherFct):VLIST ()
{
#ifdef DEBUG_MEMORY
  __IB_FCLIST_allocated_count++;
#endif

  for (const FCLIST *p = OtherFct.Next(); p != &OtherFct; p = p->Next())
    AddEntry (p->Fc);
}

FCLIST::FCLIST(const FC& fc):VLIST()
{
#ifdef DEBUG_MEMORY
  __IB_FCLIST_allocated_count++;
#endif

  AddEntry(fc);
}

FCLIST *FCLIST::AddEntry(const FC& FcRecord)
{
  if (Prev() && Prev()->Fc != FcRecord)
    {
      FCLIST *newNode;

      try {
	newNode  = new FCLIST();
      } catch (...) {
	newNode = NULL;
      }
      if (newNode)
	{
	  newNode->Fc = FcRecord;
	  VLIST::AddNode (newNode);
	}
      else logf (LOG_PANIC|LOG_ERRNO, "FCLIST Alloc failed"); 
    }
  return this;
}

FCLIST *FCLIST::AddEntry (const FCLIST *Ptr)
{
  if (Ptr)
    {
      for (const FCLIST *p = Ptr->Next(); p != Ptr; p = p->Next())
	AddEntry (p->Fc);
    }
  return this;
}

FCLIST& FCLIST::operator = (const FC& fc)
{
  Clear ();
  AddEntry(fc);
  return *this;
}

FCLIST& FCLIST::operator =(const FCLIST& OtherFct)
{
  Clear ();
  AddEntry(&OtherFct);
  return *this;
}


FCLIST *FCLIST::AddEntry (const FCLIST& OtherFct)
{
  AddEntry (&OtherFct);
  return this;
}


GDT_BOOLEAN FCLIST::GetEntry (const size_t Index, FC* FcRecord) const
{
  FCLIST *NodePtr = (FCLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    {
      *FcRecord = NodePtr->Fc;
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

const FC& FCLIST::GetEntry (const size_t Index) const
{
  static FC NulFc((GPTYPE)-1,(GPTYPE)-1);
  FCLIST *NodePtr = (FCLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    return NodePtr->Fc;
  return NulFc;
}

static int FctFcCompare (const void *x, const void *y)
{
#if 1
  // @@@ edz: Sort last then first to allow highlighting to work
  // correctly-- see result.cxx
  int diff = (((FC *) x)->GetFieldEnd () - ((FC *) y)->GetFieldEnd ());
  if (diff)
    return diff;
  return ((FC *) y)->GetFieldStart () - ((FC *) x)->GetFieldStart ();
#else
  int diff = (((FC *) x)->GetFieldStart () - ((FC *) y)->GetFieldStart ());
  if (diff)
    return diff;
  return ((FC *) x)->GetFieldEnd () - ((FC *) y)->GetFieldEnd ();
#endif
}

void FCLIST::SortByFc ()
{
#if 1
 MergeEntries();
#else
  // Get total while we check if already sorted
  size_t TotalEntries = 0;
  GDT_BOOLEAN sorted = GDT_TRUE;
  for (register const FCLIST *p = Next(); p != this; p = p->Next())
   {
     if (sorted && (p->Fc < p->Prev()->Fc))
       sorted = GDT_FALSE;
     TotalEntries++;
   }

  // DO we have anything to sort?
  if (sorted == GDT_FALSE && TotalEntries > 1)
    {
      FC           *TablePtr = new FC[TotalEntries];
      register FC  *ptr = TablePtr;
      register FCLIST *p;

      // Put into TablePtr
      for (p = Next(); p != this; p = p->Next())
	*ptr++ = p->Fc;

      // Sort
      QSORT (TablePtr, TotalEntries, sizeof (FC), FctFcCompare);

      // Put back into list
      ptr = TablePtr;
      for (p = Next(); p != this; p = p->Next())
	p ->Fc = *ptr++;
      delete[] TablePtr;		// Cleanup
    }
#endif
}

size_t FCLIST::WriteFct(FILE *fp, GPTYPE GpOffset) const
{
  size_t count = 0;
  for (const FCLIST *p = Next(); p != this; p = p->Next())
    {
      p->Fc.Write(fp, GpOffset);
      count++;
    }
  return count;
}


void FCLIST::Write (PFILE fp) const
{
  const size_t TotalEntries = GetTotalEntries ();

  ::Write((UCHR)objFCT, fp);
  ::Write((UINT4)TotalEntries, fp);
  size_t i = 0;
  for (const FCLIST *p = Next(); p != this && i < TotalEntries; p = p->Next(), i++)
    p->Value().Write(fp);
}

GDT_BOOLEAN FCLIST::Read (PFILE fp)
{
  Clear ();

  obj_t obj = getObjID (fp);
  if (obj != objFCT)
    {
      // Not a FCLIST object!
      PushBackObjID (obj, fp);
    }
  else
    {
      // Load FCLIST
      UINT4 NewTotal;
      ::Read(&NewTotal, fp);

      FC fc;
      for (UINT4 i = 0; i < NewTotal; i++)
	{
	  fc.Read (fp);
	  AddEntry (fc);
	}
    }
  return obj == objFCT;
}

void FCLIST::Print (ostream& Os) const
{
  for (const FCLIST *p = Next (); p != this; p = p->Next ())
    Os << p->Fc;
}

// Obsolete
void FCLIST::SubtractOffset (const GPTYPE GpOffset)
{
  *this -= GpOffset;
}

FCLIST& FCLIST::operator -=(const GPTYPE GpOffset)
{
  for (register FCLIST *p = Next(); p != this; p = p->Next() )
    p->Fc -= GpOffset;
  return *this;
}

FCLIST& FCLIST::operator -=(const FC& fc)
{
  for (register FCLIST *p = Next (); p != this; p = p->Next ())
    p->Fc -= fc;
  return *this;
}


FCLIST& FCLIST::operator +=(const GPTYPE GpOffset)
{
  for (register FCLIST *p = Next (); p != this; p = p->Next () )
    p->Fc += GpOffset;
  return *this;
}

FCLIST& FCLIST::operator +=(const FC& fc)
{
  for (register FCLIST *p = Next (); p != this; p = p->Next () ) 
    p->Fc += fc; 
  return *this; 
}


ostream& operator <<(ostream& Os, const FCLIST& Fct)
{
  Fct.Print (Os);
  return Os;
}

// Destructor
FCLIST::~FCLIST()
{
#ifdef DEBUG_MEMORY
  if (--__IB_FCLIST_allocated_count < 0)
    logf (LOG_PANIC, "FCLIST global allocated count %ld < 0!", (long)__IB_FCLIST_allocated_count);
#endif
}


void FCLIST::MergeEntries()
{
  // Get total while we check if already sorted
  size_t TotalEntries = 0;
  GDT_BOOLEAN sorted = GDT_TRUE;

  for (register FCLIST *p = Next(); p != this; p = p->Next())
   {
     // Look at not just order but also for dups(!).
     if (p->Fc == p->Prev()->Fc)
	delete (FCLIST *)p->Prev()->Disattach(); // Added delete (Apr 2008)
     else if (sorted && (p->Fc <= p->Prev()->Fc))
       sorted = GDT_FALSE;
     TotalEntries++;
   }

  // DO we have anything to sort?
  if (sorted == GDT_FALSE && TotalEntries > 1)
    {
      FC           *TablePtr = new FC[TotalEntries];
      FC            fc (0,0);
      register FC  *ptr = TablePtr;
 
      // Put into TablePtr
      for (register FCLIST *p = Next(); p != this; p = p->Next())
        *ptr++ = p->Fc;
  
      // Sort  
      QSORT (TablePtr, TotalEntries, sizeof (FC), FctFcCompare);
 
      // Put back into list
      Clear();
      for (size_t i=0; i<TotalEntries; i++)
	{
	  if (TablePtr[i] != fc)
            AddEntry(fc = TablePtr[i]);
	}
      delete[] TablePtr;                // Cleanup
    }
}

