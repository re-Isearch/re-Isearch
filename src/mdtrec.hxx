/************************************************************************
************************************************************************/

/*@@@
File:		mdtrec.hxx
Version:	1.00
Description:	Class MDTREC - Multiple Document Table Record
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#ifndef MDTREC_HXX
#define MDTREC_HXX

#include "date.hxx"
#include "pathname.hxx"
#include "lang-codes.hxx"
#include "mdthashtable.hxx"

class MDT;

class MDTREC {
friend class MDT;
friend class KEYREC; 
public:
  MDTREC(MDT *Mdt= NULL);
  MDTREC(const MDTREC& OtherMdtRec);
  MDTREC& operator=(const MDTREC& OtherMdtRec);

  void           SetCategory(const _ib_category_t newCategory) { Category = newCategory; }
  _ib_category_t GetCategory() const                           { return Category;        }

  void           SetPriority(const _ib_priority_t newPriority) { Priority = newPriority; }
  _ib_priority_t GetPriority() const                           { return Priority;        }

  GDT_BOOLEAN    SetKey(const STRING& NewKey);
  STRING  GetKey() const { return STRING().Assign (Key, DocumentKeySize);}
  STRING& GetKey(PSTRING Buffer) const { return Buffer->Assign (Key, DocumentKeySize);}

  void    SetDocumentType(const DOCTYPE_ID &NewDoctypeId);
  void    SetDocumentType(const STRING& NewDocumentType);
  DOCTYPE_ID GetDocumentType() const {
    DOCTYPE_ID Doctype;
    Doctype.Name.Assign (DocumentType, DocumentTypeSize);
    Doctype.Id = Property & 0x7F;
    return Doctype;
  }
  INT GetDocumentType(STRING *Buffer) const {
    if (Buffer) Buffer->Assign (DocumentType, DocumentTypeSize);
    return Property & 0x7F;
  }

  void    SetPath(const STRING& NewPath);
  STRING  GetPath() const;
  STRING& GetPath(STRING *Buffer) const {
    return *Buffer = GetPath();
  }

  void    SetFileName(const STRING& NewFileName);
  STRING  GetFileName() const;

  STRING& GetFileName(STRING *Buffer) const {
    return *Buffer = GetFileName();
  }

  void    SetFullFileName(const STRING& NewFullPath);
  STRING  GetFullFileName() const {
    return GetPath() + GetFileName();
  }

  void    SetOrigPath(const STRING& NewPathName);
  STRING  GetOrigPath() const;
  void    SetOrigFileName(const STRING& NewFileName);
  STRING  GetOrigFileName() const;

  void    SetOrigFullFileName(const STRING& NewFullPath);
  STRING  GetOrigFullFileName() const {
    return GetOrigPath() + GetOrigFileName();
  }

  STRING& GetFullFileName(STRING *Buffer) const {
    return *Buffer = GetFullFileName();
  }

  PATHNAME GetPathname() const;
  PATHNAME GetOrigPathname() const;

  void SetPathname (const PATHNAME& Pathname);
  void SetOrigPathname (const PATHNAME& Pathname);

  void   SetGlobalFileStart(const GPTYPE NewStart) { GlobalFileStart = NewStart; }
  GPTYPE GetGlobalFileStart() const { return GlobalFileStart; };

  // For index import
  MDTREC& operator+=(const GPTYPE Offset) { IncrementStart(Offset); return *this;}
  void    IncrementStart(GPTYPE x)  { GlobalFileStart += x;}

#if 0
  void   SetGlobalFileEnd(const GPTYPE NewEnd) { GlobalFileEnd = NewEnd; }
  GPTYPE GetGlobalFileEnd() const { return GlobalFileEnd; };
#endif
  off_t GetLength() const { return LocalRecordEnd - LocalRecordStart; }

  void   SetLocalRecordStart(const UINT4 NewStart) { LocalRecordStart = NewStart; }
  UINT4  GetLocalRecordStart() const { return LocalRecordStart; }

  void   SetLocalRecordEnd(const UINT4 NewEnd) { LocalRecordEnd = NewEnd; }
  UINT4  GetLocalRecordEnd() const { return LocalRecordEnd; }

  void    SetLocale(const LOCALE& NewLocale)   { Locale = NewLocale; }
  LOCALE  GetLocale() const                    { return Locale;      }

  // We store the dates as a pair of 4-byte ints
  // Generic Date
  void SetDate(const SRCH_DATE& NewDate)         { Date = NewDate;         }
  SRCH_DATE GetDate() const                      { return Date;            }
  // Modified Date
  void SetDateModified(const SRCH_DATE& NewDate) {
    if ((DateModified = NewDate).Ok() && Date.IsNotSet())
      Date = DateModified;
  }
  SRCH_DATE GetDateModified() const              { return DateModified;    }
  // Date Created
  void SetDateCreated(const SRCH_DATE& NewDate)  { 
    if ((DateCreated = NewDate).Ok() && Date.IsNotSet())
      Date = DateCreated;
  }
  SRCH_DATE GetDateCreated() const               { return DateCreated;     }

  // Date Expires
  void SetDateExpires(const SRCH_DATE& NewDate)  { DateExpires = NewDate;  }
  SRCH_DATE GetDateExpires() const               { return DateExpires;     }

  // Time To Live
  int  TTL() const;
  int  TTL(const SRCH_DATE& Now) const;

  void SetDeleted(const GDT_BOOLEAN Flag);
  GDT_BOOLEAN GetDeleted() const;

  void FlipBytes();

  // In the structure read/write..
  GDT_BOOLEAN Write(FILE *fp, INT Index = 0) const;
  GDT_BOOLEAN Read(FILE *fp, INT Index = 0);
  SRCH_DATE   GetDate(FILE *fp, INT Index=0) const;


  // Is the MDT Index deleted?
  GDT_BOOLEAN IsDeleted(FILE *fp, INT Index);


  STRING        Dump() const;

  friend ostream& operator<<(ostream& os, const MDTREC& mdtrec);

  void         SetHashTable (MDTHASHTABLE  *newHashTable) {
    HashTable = newHashTable;
  }

  ~MDTREC();
private:
  SRCH_DATE      Date; // Date of Record (Must be first element!)
/* This is an object we write so the types need to be well defined */
  CHR            Key[DocumentKeySize];
  CHR            DocumentType[DocumentTypeSize];

  GPTYPE         pathNameID;
  GPTYPE         fileNameID;

  GPTYPE         origpathNameID;
  GPTYPE         origfileNameID;

  MDTHASHTABLE  *HashTable;
  GPTYPE         GlobalFileStart;
  UINT4          LocalRecordStart;
  UINT4          LocalRecordEnd;
  LOCALE         Locale;
  _ib_category_t Category;
  _ib_priority_t Priority;

  SRCH_DATE      DateModified;
  SRCH_DATE      DateCreated;
  SRCH_DATE      DateExpires;

  BYTE           Property; // This MUST be the last element!
};


typedef MDTREC* PMDTREC;

// Common Functions
inline void Write(const MDTREC& Mdtrec, PFILE Fp)
{
  Mdtrec.Write(Fp);
}


inline GDT_BOOLEAN Read(PMDTREC MdtrecPtr, PFILE Fp)
{
  return MdtrecPtr->Read(Fp);
}


#endif
