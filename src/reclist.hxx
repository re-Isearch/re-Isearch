/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		reclist.hxx
Description:	Class RECLIST - Database Record List
@@@*/

#ifndef RECLIST_HXX
#define RECLIST_HXX

#include "defs.hxx"
#include "string.hxx"
#include "record.hxx"

class RECLIST {
public:
  RECLIST();

  void AddEntry(const RECORD& RecordEntry);
  bool GetEntry(const size_t Index, PRECORD RecordEntry) const;

  const RECORD operator[](size_t n) const { return Table[n]; }
  RECORD& operator[](size_t n)            { return Table[n]; }

  void Expand();
  void CleanUp();
  void Resize(const size_t Entries);
  size_t GetTotalEntries() const;
  void Write(PFILE Fp) const;
  bool Read(PFILE Fp);
  ~RECLIST();
private:
  PRECORD Table;
  size_t TotalEntries;
  size_t MaxEntries;
};

typedef RECLIST* PRECLIST;

inline void Write(const RECLIST& Reclist, PFILE Fp)
{
  Reclist.Write(Fp);
}

inline bool Read(PRECLIST ReclistPtr, PFILE Fp)
{
  return ReclistPtr->Read(Fp);
}


#endif
