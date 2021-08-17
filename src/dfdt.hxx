/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		dfdt.hxx
Description:	Class DFDT - Data Field Definitions Table
@@@*/

#ifndef DFDT_HXX
#define DFDT_HXX

#include "dfd.hxx"

class IDBOBJ;

class DFDT {
public:
  DFDT();
  DFDT(size_t InitialSize);
  DFDT (const DFDT& OtherDfdt);
  DFDT (const STRING& FileName);
  DFDT (PFILE Fp);

  // Field Exists? Return number matching (if wild)
  size_t FieldExists(const STRING& FieldName) const;
  size_t NumericFieldExists(const STRING& FieldName, STRING *Ptr=NULL) const;
  size_t DateFieldExists(const STRING& FieldName, STRING *Ptr=NULL) const;
  size_t TypeFieldExists(const FIELDTYPE& Ft, const STRING& FieldName, STRING *Ptr=NULL) const;

  DFDT& operator  =(const DFDT& OtherDfdt);
  DFDT& operator +=(const DFDT& OtherDfdt);

  void Flush (const STRING& FileName);

  void LoadTable(const STRING& FileName);
  void SaveTable(const STRING& FileName);

  void Clear();
  void Empty(); // Release data and reset everthing
  GDT_BOOLEAN  Resize(const size_t Entries);

  DFDT& Cat (const DFDT& OtherDfdt);
  void AddEntry(const DFD& DfdRecord);
  void FastAddEntry(const DFD& DfdRecord);

  GDT_BOOLEAN GetEntry(const size_t Index, PDFD DfdRecord) const;
  GDT_BOOLEAN GetDfdRecord(const STRING& FieldName, PDFD DfdRecord) const;
  INT GetFileNumber(const STRING& FieldName) const;
  GDT_BOOLEAN IsNumericalField (const STRING& FieldName) const;

  int         Roots(STRLIST *StrlistPtr = NULL) const; // Returns number of root paths 
  STRING      FirstRoot() const;  // Returns first root

  STRING      GetFieldName(size_t Index) const;
  GDT_BOOLEAN GetAttributes (const size_t Index, PATTRLIST AttributesBuffer) const;
  GDT_BOOLEAN GetAttributes (const STRING& FieldName, PATTRLIST AttributesBuffer) const;
  size_t      GetTotalEntries() const { return TotalEntries; }
  GDT_BOOLEAN GetChanged() const { return Changed; }

  void Sort();

  GDT_BOOLEAN IsSystemFile(const STRING& FileName) const;

  GDT_BOOLEAN KillAll(IDBOBJ* DbParent);
  STRING GetFileName(IDBOBJ* DbParent, const STRING& FieldName);

  void Write (PFILE fp) const;
  GDT_BOOLEAN Read (PFILE fp);
  ~DFDT();
private:
//void Initialize();
  size_t      FieldExists(const STRING& FieldName, STRING *Val) const;
  INT         GetNewFileNumber() const;
  GDT_BOOLEAN Expand();
  void        CleanUp();
  size_t      Lookup (const STRING& FieldName) const;

  PDFD Table;
  size_t TotalEntries;
  size_t MaxEntries;

  GDT_BOOLEAN Changed;
  GDT_BOOLEAN Sorted;
};

typedef DFDT* PDFDT;

// Common

inline void Write(const DFDT& Dfdt, FILE *Fp)
{
  Dfdt.Write(Fp);
}

inline GDT_BOOLEAN Read(PDFDT DfdtPtr, FILE *Fp)
{
  return DfdtPtr->Read(Fp);
}


#endif
