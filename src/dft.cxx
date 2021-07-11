/*-@@@
File:		dft.cxx
Version:	1.00
Description:	Class DFT - Data Field Table
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include "common.hxx"
#include "dft.hxx"

#pragma ident  "@(#)dft.cxx  1.11 06/27/00 20:29:11 BSN"
#define INCREMENT 50


DFT::DFT ()
{
  MaxEntries = 0;
  Table = NULL;
  TotalEntries = 0;
}

DFT::DFT (const DFT& OtherDft)
{
  MaxEntries = 0;
  *this = OtherDft;
}

DFT& DFT::operator =(const DFT& OtherDft)
{
  const size_t OtherTotal = OtherDft.GetTotalEntries ();
  if (OtherTotal > MaxEntries)
    {
      MaxEntries = OtherTotal;
      if (Table)
	delete[]Table;
      TotalEntries = 0;
      Table = new DF[MaxEntries];
    }
  for (TotalEntries = 0; TotalEntries < OtherTotal; TotalEntries++)
    {
      Table[TotalEntries] = OtherDft.Table[TotalEntries];
    }
  return *this;
}

void DFT::AddEntry (const DF& DfRecord)
{
#if 1
  FastAddEntry(DfRecord);
#else
  const STRING Field (DfRecord.GetFieldName());
  for (size_t i = 0; i < TotalEntries; i++)
    {
      if (Field == Table[i].GetFieldName())
	{
	  Table[i].AddFct(DfRecord.GetFcListPtr());
	  return;
	}
    }
  if (TotalEntries == MaxEntries)
    {
      Expand ();
    }
  Table[TotalEntries++] = DfRecord;
#endif
}

void DFT::FastAddEntry (const DF& DfRecord)
{
  if (!DfRecord.Ok())
    return; // Don't Add Empty fields // edz 16 Feb 2003

  if (TotalEntries == MaxEntries)
    {
      Expand ();
    }
  Table[TotalEntries++] = DfRecord;
}


GDT_BOOLEAN DFT::GetEntry (const size_t Index, DF *DfRecord) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *DfRecord = Table[Index - 1];
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


const DF *DFT::GetEntryPtr(const size_t Index) const
{
 if ((Index > 0) && (Index <= TotalEntries))
    return &Table[Index - 1];
  return NULL;
}

STRING DFT::GetFieldName (const size_t Index) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    return Table[Index - 1].GetFieldName();
  return NulString;
}


const FCLIST *DFT::GetFcListPtr(const size_t Index) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      return Table[Index - 1].GetFcListPtr();
    }
  return NULL;
}

void DFT::Expand ()
{
  Resize ((TotalEntries<<1) + INCREMENT);
}

void DFT::CleanUp ()
{
  // Shrink
  Resize (TotalEntries);
}

void DFT::Resize (const size_t Entries)
{
  DF *OldTable = Table;
  MaxEntries = Entries;
  Table = new DF[MaxEntries];

  TotalEntries = (Entries >= TotalEntries) ? TotalEntries : Entries;
  for (size_t i = 0; i < TotalEntries; i++)
    {
      Table[i] = OldTable[i];
    }
  if (OldTable)
    delete[]OldTable;
}

size_t DFT::GetTotalEntries () const
{
  return TotalEntries;
}

void DFT::Write (PFILE fp) const
{
  putObjID (objDFT, fp);
  ::Write ((UINT2)TotalEntries, fp);
  for (size_t i = 0; i < TotalEntries; i++)
    {
      Table[i].Write (fp);
    }
}

GDT_BOOLEAN DFT::Read (PFILE fp)
{
  DFT Dft;

  obj_t obj = getObjID (fp);
  if (obj != objDFT)
    {
      PushBackObjID (obj, fp);
    }
  else
    {
      UINT2 NewTotal;
      DF Df;

      ::Read(&NewTotal, fp);
      for (size_t i = 0; i < NewTotal; i++)
	{
	  Df.Read (fp);
	  Dft.AddEntry (Df);
	}
    }
  *this = Dft;
  return obj == objDFT;
}

DFT::~DFT ()
{
  if (Table)
    delete[]Table;
}

