/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*-@@@
File:		mdt.hxx
Version:	1.00
Description:	Class MDT - Multiple Document Table
@@@*/

#ifndef MDT_HXX
#define MDT_HXX


#include "string.hxx"
#include "mmap.hxx"
#include "fc.hxx"
#include "mdtrec.hxx"


extern int _IB_MDT_SEED; // Global integer!

class GPREC;
class KEYREC;
class KEYSORT;

class INDEX;
class FPT;

class MDT {
friend class MDTREC;
 public:
  MDT (INDEX *Index);
  MDT (const STRING& DbFileStem, const bool WrongEndian);

  STRING GetFileStem() const { return FileStem; }

  size_t AddEntry (const MDTREC& MdtRecord);
  void SetEntry (const size_t Index, const MDTREC& MdtRecord);

  MDTREC * operator[](const size_t Idx) { return GetEntry(Idx-1); }

  bool GetEntry (const size_t Index, MDTREC *MdtrecPtr) const;
  MDTREC     *GetEntry (const size_t Index);

  // Special to get Key
  STRING     GetKey (const size_t Index, int *Hash = NULL) const;

  // Special to just read date
  SRCH_DATE   GetDate (const size_t Index) const {
    return Index == 0 ? GetTimestamp() : tmpMdtrec.GetDate(MdtFp, Index); }

  // Mark the record by index as deleted
  bool Delete(const size_t Index)   { return SetDeleted(Index, true);  }
  // Mark the record by index as not deleted
  bool UnDelete(const size_t Index) { return SetDeleted(Index, false); }
  // Is the record Marked deleted?
  bool IsDeleted(const size_t Index) const;

  void Resize (const size_t Entries);

  size_t LookupByKey (const STRING& Key);
  size_t GetMdtRecord (const STRING& Key, MDTREC *MdtrecPtr);
  size_t GetMdtRecord (const GPTYPE gp, MDTREC *MdtrecPtr);
  size_t LookupByGp (const GPTYPE Gp, PFC FcPtr = NULL);
//INT GetMdtIndex(const GPTYPE gp);       // Use LookupByGp() instead

  GPTYPE GetNameByGlobal(GPTYPE gp, STRING *Path, GPTYPE *Size, GPTYPE *LS, DOCTYPE_ID *Doctype);

  GPTYPE GetNextGlobal ();
  bool IsEmpty() const  { return TotalEntries == 0; }
  size_t GetTotalEntries () const { return TotalEntries; }
  size_t GetTotalDeleted() const;

  STRING& GetUniqueKey (PSTRING StringPtr, bool Override=false);
  void Dump (INT Skip = 0, ostream& os = cout) const;
  bool GetChanged () const { return Changed; }

  void IndexSortByIndex ();
  size_t RemoveDeleted ();
  void SortKeyIndex ();
  void SortGpIndex ();

  bool Ok() const;
  bool IsSystemFile (const STRING& FileName);
  int         Version() const;

#if USE_MDTHASHTABLE && 0
  // Get Filename stuff
  void    SetPathName(MDTREC *Mdtrec, const STRING& NewPathName) {
    Mdtrec->SetPathName(MDTHashTable->AddPathName(NewPathName));
  }
  STRING  GetPath(MDTREC *Mdtrec) const {
    return MDTHashTable->GetPath(Mdtrec->GetPath());
  }
  void    SetFileName(MDREC Mdtrec, const STRING& NewFileName) {
    Mdtrec->SetFileName(MDTHashTable->AddFileName(NewFileName));
  }
  STRING  GetFileName(MDTREC Mdtrec) const {
    return MDTHashTable->GetFileName(Mdtrec->GetFileName());
  }
#endif

  // We store this here for convienience
  bool SetIndexNum(INT Num) const;
  INT         GetIndexNum() const;

  size_t      KeySortPosition(_index_id_t Index) const;

  void FlushMDTIndexes();

  bool KillAll();

  SRCH_DATE   GetTimestamp() const { return Timestamp; }

  ~MDT ();

  const char *c_str() const { return MdtName.c_str(); }

  // Needed by Import routine
  MDTHASHTABLE *GetMDTHashTable() { return MDTHashTable; }

 private:
  void        Init();
  bool SetDeleted(const size_t Index, bool Delete);
  void        WriteHeader() const;
  bool RebuildIndex();
  bool BuildKeySortTable ();

  void        WriteTimestamp();
  void        ReadTimestamp();

#if USE_MDTHASHTABLE
  MDTHASHTABLE *MDTHashTable;
  bool   fastAdd;

#endif
  STRING      FileStem;
  PFILE       MdtFp;
  KEYREC     *KeyIndex;
  GPREC      *GpIndex;
  MDTREC     *MdtIndex;
  KEYSORT    *KeySortTable;

  size_t      TotalEntries;
  size_t      MaxEntries;
  GPTYPE      NextGlobalGp;
  size_t      lastIndex;
  size_t      lastKeyIndex;

  MMAP        IndexMap;
  bool useIndexMap;
  MMAP        MdtMap;
  bool useMdtMap;
  MDTREC      tmpMdtrec;

  bool KeyIndexSorted;
  bool GpIndexSorted;
  bool Changed;
  bool MdtWrongEndian;
  bool ReadOnly;

  STRING      MdtIndexName;
  STRING      MdtName;
  const char *Magic;
  FPT        *Fpt;

  SRCH_DATE   Timestamp;
};

typedef MDT *PMDT;

#endif
