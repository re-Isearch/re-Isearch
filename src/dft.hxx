/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		dft.hxx
Description:	Class DFT - Data Field Table
@@@*/

#ifndef DFT_HXX
#define DFT_HXX

#include "df.hxx"
#include "magic.hxx"

class DFT {
public:
  DFT();
  DFT(const DFT& OtherDft);
  DFT& operator=(const DFT& OtherDft);

  void        AddEntry(const DF& DfRecord);
  void        FastAddEntry(const DF& DfRecord);
  GDT_BOOLEAN GetEntry(const size_t Index, DF *DfRecord) const;
  const DF   *GetEntryPtr(const size_t Index) const;

  STRING     GetFieldName (const size_t Index) const;
  const FCLIST *GetFcListPtr(const size_t Index) const;
  void WriteFct(const size_t Index, FILE *Fp, const GPTYPE Offset) const {
    const FCLIST *Pfclist = GetFcListPtr(Index);
    if (Pfclist) Pfclist->WriteFct(Fp, Offset);
  }

  size_t GetTotalEntries() const;

  void Write(PFILE fp) const;
  GDT_BOOLEAN Read(PFILE fp);

  ~DFT();
private:
  void Expand();
  void CleanUp();
  void Resize(const size_t Entries);

  DF    *Table;
  size_t TotalEntries;
  size_t MaxEntries;
};

typedef DFT* PDFT;

// Common Functions
inline void Write (const DFT& Dft, PFILE Fp)
{
  Dft.Write (Fp);
}

inline GDT_BOOLEAN Read (DFT *DftPtr, PFILE Fp)
{
  return DftPtr->Read (Fp);
}



#endif
