#pragma ident  "@(#)reclist.cxx  1.7 10/09/99 13:15:41 BSN"

/************************************************************************
************************************************************************/

/*-@@@
File:		reclist.cxx
Version:	1.00
Description:	Class RECLIST - Database Record List
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdio.h>
#include "reclist.hxx"
#include "common.hxx"
#include "magic.hxx"

#define INCREMENT 1000

RECLIST::RECLIST ()
{
  MaxEntries = 0;
  Table = NULL;
  TotalEntries = 0;
}

void RECLIST::AddEntry (const RECORD& RecordEntry)
{
  if (TotalEntries == MaxEntries)
    Expand ();
  Table[TotalEntries++] = RecordEntry;
}

GDT_BOOLEAN RECLIST::GetEntry (const size_t Index, PRECORD RecordEntry) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *RecordEntry = Table[Index - 1];
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

void RECLIST::Expand ()
{
  Resize (TotalEntries + INCREMENT);
}

void RECLIST::CleanUp ()
{
  Resize (TotalEntries);
}

void RECLIST::Resize (const size_t Entries)
{
  PRECORD OldTable = Table;
  MaxEntries = Entries;
  Table = new RECORD[MaxEntries];

  size_t RecsToCopy;
  if (Entries >= TotalEntries)
    {
      RecsToCopy = TotalEntries;
    }
  else
    {
      RecsToCopy = Entries;
      TotalEntries = Entries;
    }
  for (size_t x = 0; x < RecsToCopy; x++)
    {
      Table[x] = OldTable[x];
    }
  if (OldTable)
    delete[]OldTable;
}

size_t RECLIST::GetTotalEntries () const
{
  return TotalEntries;
}

void RECLIST::Write(PFILE Fp) const
{
  putObjID (objRECLIST, Fp);
  ::Write((UINT4)TotalEntries, Fp); // Write count
  for (size_t i = 0; i < TotalEntries; i++)
    {
      Table[i].Write(Fp);
    }
}

GDT_BOOLEAN RECLIST::Read(PFILE Fp)
{
  UINT4 Entries = 0;
  obj_t obj = getObjID (Fp);
  if (obj != objRECLIST)
    {
      PushBackObjID (obj, Fp);
    }
  else
    {
      ::Read(&Entries, Fp); // Get count
      PRECORD OldTable = Table;
      MaxEntries = Entries;
      Table = new RECORD[MaxEntries];

      for (size_t i=0; i < Entries; i++)
	{
	  Table[i].Read(Fp);
	}
      if (OldTable) delete[] OldTable;
    }
  TotalEntries = Entries;
  return obj == objRECLIST;
}

RECLIST::~RECLIST ()
{
  if (Table)
    delete[]Table;
}

