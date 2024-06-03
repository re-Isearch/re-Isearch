/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		result.hxx
Description:	Class RESULT - Search Result
@@@*/

#ifndef RESULT_HXX
#define RESULT_HXX

#include "date.hxx"
#include "lang-codes.hxx"
#include "fct.hxx"
#include "pathname.hxx"

class DOCTYPE;
class MDTREC;

extern long __IB_RESULT_allocated_count; // Used to track stray RESULTs


class RESULT {
public:
  RESULT();
  RESULT(const MDTREC& Mdtrec);
  RESULT(const RESULT& OtherResult);

  RESULT& operator=(const RESULT& OtherResult);

  void     SetIndex(const INDEX_ID& newIndex) { Index = newIndex; }
  INDEX_ID GetIndex() const { return Index; }
  void     SetMdtIndex(const INT NewMdtIndex) { Index.SetMdtIndex(NewMdtIndex); }
  INT      GetMdtIndex() const { return Index.GetMdtIndex(); };
  void     SetVirtualIndex(const UCHR newIndex) { Index.SetVirtualIndex(newIndex); }
  INT      GetVirtualIndex() const { return Index.GetVirtualIndex(); }

  void           SetCategory(const _ib_category_t newCategory) { Category = newCategory; }
  _ib_category_t GetCategory() const                           { return Category;        }

  void    SetKey(const STRING& NewKey) { Key = NewKey;           }
  STRING  GetKey() const               { return Key;             }
  STRING& GetKey(STRING *ptr) const    { return *ptr = GetKey(); }
  STRING  GetGlobalKey(char Ch = '@') const;
  void    GetVKey(STRING* StringBuffer) const { *StringBuffer = GetGlobalKey(':'); }

  void        SetDocumentType(const DOCTYPE_ID& NewType) { DocumentType = NewType; }
  const STRING& GetDocumentType (STRING *Ptr) const { return *Ptr = DocumentType.DocumentType(); }
  DOCTYPE_ID  GetDocumentType() const               { return DocumentType; }
  STRING      GetDoctype() const                    { return DocumentType.DocumentType(); }

  LOCALE   GetLocale () const                      { return Locale;         }

  void     SetLocale (const LOCALE& NewLocale)     { Locale = NewLocale;    }
  void     SetLocale (const LANGUAGE& NewLanguage) { Locale = NewLanguage;  }
  void     SetLocale (const CHARSET& NewCharset)   { Locale = NewCharset;   }

  void     SetLanguage (const LANGUAGE& Language)  { Locale.SetLanguage(Language); }
  void     SetCharset  (const CHARSET& Charset)    { Locale.SetCharset(Charset);   }

  const char *GetLanguageCode () const             { return Locale.GetLanguageCode();  }
  const char *GetCharsetCode () const              { return Locale.GetCharsetCode();   }
  const char *GetLanguageName () const             { return Locale.GetLanguageName();  }
  const char *GetCharsetName () const              { return Locale.GetCharsetName();   }
  BYTE        GetCharsetId() const                 { return Locale.GetCharsetId();     }

#if 1
  void     SetPath(const STRING& newPath)     { Pathname.SetPath(newPath);         }
  STRING   GetPath() const                    { return Pathname.GetPath();         }
  STRING&  GetPathName(STRING *ptr) const     { return *ptr = GetPath();           }
  void     SetFileName(const STRING& newName) { Pathname.SetFileName(newName);     }
  STRING   GetFileName() const                { return Pathname.GetFileName();     }
  STRING&  GetFileName(STRING *ptr) const     { return *ptr = GetFileName();       }
#endif

  STRING   GetFullFileName() const            { return Pathname.GetFullFileName(); }
  STRING&  GetFullFileName(STRING *ptr) const { return *ptr = GetFullFileName();   }

  PATHNAME GetPathname() const               { return Pathname;     }
  PATHNAME GetOrigPathname() const           { return origPathname; }

  void     SetPathname(const PATHNAME& newPathname)     { Pathname = newPathname; }
  void     SetOrigPathname(const PATHNAME& newPathname) { origPathname = newPathname; }

  void   SetRecordStart(const UINT4 NewRecordStart) { RecordStart = NewRecordStart; }
  UINT4  GetRecordStart() const                     { return RecordStart;           }
  void   SetRecordEnd(const UINT4 NewRecordEnd )    { RecordEnd = NewRecordEnd;     }
  UINT4  GetRecordEnd() const                       { return RecordEnd;             }
  off_t  GetLength () const                         { return RecordEnd-RecordStart; }
  off_t  GetRecordSize() const                      { return GetLength() + 1;       }

  void        SetExtIndex(const _index_id_t newVal) { ExtIndex = newVal;            }
  _index_id_t GetExtIndex() const                   { return ExtIndex;              }

  void   SetScore(const DOUBLE NewScore) { Score = NewScore; }
  DOUBLE GetScore() const                { return Score;     }

  void       SetDate(const SRCH_DATE& NewDate) { Date = NewDate; }
  SRCH_DATE  GetDate() const                   { return Date;    }

  void       SetDateModified(const SRCH_DATE& NewDate) { DateModified = NewDate; }
  SRCH_DATE  GetDateModified() const                   { return DateModified;    }

  void       SetDateCreated(const SRCH_DATE& NewDate) { DateCreated = NewDate; }
  SRCH_DATE  GetDateCreated() const                   { return DateCreated;    }

  void       SetDateExpires(const SRCH_DATE& NewDate) { DateExpires = NewDate; }
  SRCH_DATE  GetDateExpires() const                   { return DateExpires;  }

  void   SetAuxCount(UINT newCount) { AuxCount = newCount; }
  UINT   GetAuxCount() const        { return AuxCount;     }

  size_t GetHitTotal() const { return HitTable.GetTotalEntries(); }
  int    GetRefcount_() const { return HitTable.Refcount_(); }

  void   SetHitTable(const FCT& NewHitTable)    { (HitTable = NewHitTable).SortByFc(); }
//void   SetHitTable(const FCLIST& NewHitTable) { HitTable = NewHitTable; }

  const FCT GetHitTable(FCLIST *HitTableBuffer = NULL) const;

  // Dump a XML hit table
  STRING XMLHitTable() const;
  
  // Dump a Json hit table;
  STRING JsonHitTable() const;

  // Get the data
  void   GetRecordData(STRING *StringBuffer, DOCTYPE *DoctypePtr = NULL) const;


  bool PresentHit(const FC& Fc, STRING *StringBuffer, STRING *Term,
        const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr = NULL,
	STRING *Tag = NULL) const;

  // Context..
  FC GetBestContextHit() const;
  bool PresentBestContextHit(STRING *StringBuffer, STRING *Term,
	const STRING& BeforeTerm = NulString, const STRING& AfterTerm = NulString,
	DOCTYPE *DoctypePtr = NULL, STRING *FieldNamePtr=NULL) const;

  // Get the Context of the Nth hit
  bool PresentNthHit(size_t N, STRING *StringBuffer, STRING *Term,
        const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr = NULL, STRING *TagPtr = NULL) const;
  // Get the Context of the first hit
  bool PresentFirstHit(STRING *StringBuffer, STRING *Term = NULL, DOCTYPE *DoctypePtr = NULL,
	STRING *TagPtr = NULL) const {
    return PresentNthHit(1, StringBuffer, Term,  NulString,  NulString, DoctypePtr, TagPtr);
  }
  bool PresentFirstHit(STRING *StringBuffer, STRING *Term,
	const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr = NULL, STRING *TagPtr = NULL) const {
    return PresentNthHit(1, StringBuffer, Term, BeforeTerm, AfterTerm, DoctypePtr, TagPtr);
  }
  // XML versions of above
  bool XMLPresentNthHit(size_t N, STRING *StringBuffer, const STRING& Tag,
        STRING *Term = NULL, DOCTYPE *DoctypePtr = NULL) const;
  bool XMLPresentFirstHit(STRING *StringBuffer, const STRING& Tag,
	STRING *Term = NULL, DOCTYPE *DoctypePtr = NULL) const {
    return XMLPresentNthHit(1, StringBuffer, Tag, Term, DoctypePtr);
  }

  STRING GetXMLHighlightRecordFormat(int pageno = 0, off_t offset=0) const;

  // Record with highlighted "hits"
  void GetHighlightedRecord(const STRING& BeforeTerm, const STRING& AfterTerm,
	STRING *StringBuffer, DOCTYPE *DoctypePtr = NULL) const;
  void GetHighlighted(const STRING& BeforeTerm,
	const STRING& AfterTerm, FC Range, STRING *StringBuffer, DOCTYPE *DoctypePtr = NULL) const;

  void Write(FILE *fp) const;
  bool Read(FILE *fp);

  ~RESULT();
private:
  INDEX_ID       Index;
  _index_id_t    ExtIndex; // Other order
  STRING         Key;
  DOCTYPE_ID     DocumentType;
  PATHNAME       Pathname;
  PATHNAME       origPathname;
  UINT4          RecordStart;
  UINT4          RecordEnd;
  SRCH_DATE      Date;
  SRCH_DATE      DateModified;
  SRCH_DATE      DateCreated;
  SRCH_DATE      DateExpires;
  LOCALE         Locale;
  DOUBLE         Score;
  UINT           AuxCount;
  _ib_category_t Category;
  FCT            HitTable;
};

extern const RESULT& NulResult;

typedef RESULT* PRESULT;

// Common Functions
inline void Write(const RESULT& Result, PFILE Fp)
{
  Result.Write(Fp);
}

inline bool Read(PRESULT ResultPtr, PFILE Fp)
{
  return ResultPtr->Read(Fp);
}


#endif
