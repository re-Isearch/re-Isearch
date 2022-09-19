/************************************************************************
************************************************************************/

/*@@@
File:		record.hxx
Version:	1.01
Description:	Class RECORD - Database Record
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@bsn.com
@@@*/

#ifndef RECORD_HXX
#define RECORD_HXX

//#include "defs.hxx"
#include "common.hxx"
#include "pathname.hxx"
#include "date.hxx"
#include "lang-codes.hxx"
#include "dft.hxx"


class RECORD {
friend class VRECORD;
public:
  RECORD ();
  RECORD (const STRING& Fullpath);
  RECORD (const STRING& NewPathName, const STRING& NewFileName);
  RECORD (const RECORD& OtherRecord);

  RECORD& Clear();

  RECORD& operator=(const RECORD& OtherRecord);


  GDT_BOOLEAN Exists() const { return Pathname.Exists(); }

  void    SetKey(const STRING& NewKey)  { Key = NewKey; }
  STRING  GetKey() const                { return Key;           }
  STRING  GetKey(PSTRING Buffer) const  { return *Buffer = Key; }

  void    SetPath(const STRING& NewPathName);
  STRING  GetPath() const;
  STRING  GetPath(PSTRING Buffer) const { return *Buffer = GetPath(); }

  void    SetFileName(const STRING& NewName);
  STRING  GetFileName() const;
  STRING  GetFileName(PSTRING Buffer) const   { return *Buffer = GetFileName(); }

  void    SetFullFileName (const STRING& FullName);
  STRING  GetFullFileName() const               { return Pathname;                    }
  STRING  GetFullFileName(STRING *Buffer) const { return *Buffer = GetFullFileName(); }

  PATHNAME GetPathname() const { return Pathname; }
  void     SetPathname(const PATHNAME& nPathname) { Pathname = nPathname; }

  PATHNAME GetOrigPathname() const { return origPathname; }
  void     SetOrigPathname(const PATHNAME& nPathname) { origPathname = nPathname; }

  void      RelativizePathnames(const STRING& RootDir);
  PATHNAME  RelativizePathname(const STRING& RootDir);
  PATHNAME  RelativizeOrigPathname(const STRING& RootDir);

  void    SetRecordStart(const UINT4 NewStart) { RecordStart = NewStart; }
  UINT4   GetRecordStart() const               { return RecordStart;     }

  void    SetRecordEnd(const UINT4 NewEnd) { RecordEnd = NewEnd; }
  UINT4   GetRecordEnd() const             { return RecordEnd;   }

  GDT_BOOLEAN IsEmpty() const { return GetLength() <= 0; }
  off_t       GetLength () const;

  void    SetDocumentType(const DOCTYPE_ID& NewType){ DocumentType = NewType; }
  const STRING&  GetDocumentType(STRING *Ptr) const { return *Ptr = DocumentType.DocumentType(); }
  DOCTYPE_ID  GetDocumentType() const               { return DocumentType;                }

  LOCALE   GetLocale () const                      { return Locale;         }

  void     SetLocale (const LOCALE& NewLocale)     { Locale = NewLocale;    }
  void     SetLocale (const LANGUAGE& NewLanguage) { Locale = NewLanguage;  }
  void     SetLocale (const CHARSET& NewCharset)   { Locale = NewCharset;   }

  void        SetLanguage (const LANGUAGE& Language) { Locale.SetLanguage(Language); }
  void        SetCharset  (const CHARSET& Charset)   { Locale.SetCharset(Charset);   }
  const char *GetLanguageCode () const               { return Locale.GetLanguageCode();  }
  const char *GetCharsetCode () const                { return Locale.GetCharsetCode();   }
  const char *GetLanguageName () const               { return Locale.GetLanguageName();  }
  const char *GetCharsetName () const                { return Locale.GetCharsetName();   }

  SRCH_DATE GetDate() const                          { return Date;            }
  void      SetDate (const SRCH_DATE& NewDate)       { Date = NewDate;         }
  void      SetDateModified(const SRCH_DATE& NewDate){ DateModified = NewDate; }
  SRCH_DATE GetDateModified() const                  { return DateModified;    }
  void      SetDateCreated(const SRCH_DATE& NewDate) { DateCreated = NewDate;  }
  SRCH_DATE GetDateCreated() const                   { return DateCreated;     }

  // Date Expires
  void           SetDateExpires(const SRCH_DATE& NewDate) { DateExpires = NewDate;  }
  SRCH_DATE      GetDateExpires() const                   { return DateExpires;     }

  int            TTL() const;
  int            TTL(const SRCH_DATE& Now) const; 

  _ib_priority_t GetPriority() const                     { return Priority;        }
  void           SetPriority(_ib_priority_t newPriority) { Priority = newPriority; }
  void           SetPriority(const STRING& newVal)       { Priority = newVal.GetLong();      }

  _ib_category_t GetCategory() const                     { return Category;        }
  void           SetCategory(_ib_category_t newCategory) { Category = newCategory; }
  void           SetCategory(const STRING& newVal)       { Category = newVal.GetLong();      }

  void           SetDft(const DFT& NewDft)    { Dft = NewDft;    }
  void           GetDft(PDFT DftBuffer) const { *DftBuffer = Dft;}
  const DFT     *GetDftPtr() const            { return &Dft;     }

  void           Write(PFILE fp) const;
  GDT_BOOLEAN    Read(PFILE fp);

  // Top bit for Bad Record and the rest for Segment number
  // NOTE: max 32768 segments
  // This must be more than  VolIndexCapacity (currently "only" 255)
  void           SetBadRecord(GDT_BOOLEAN bad = GDT_TRUE) {
    if (bad) Segment = (Segment | 0x8000);
    else     Segment = (Segment & 0x7FFF);
  }
  GDT_BOOLEAN    IsBadRecord() const  { return (Segment & 0x8000) != 0; }
  GDT_BOOLEAN    SetSegment(int nSegment) {
    if (nSegment < 0 || nSegment > 0x7FFF)
      return GDT_FALSE;
    Segment = (nSegment | (Segment & 0x8000));
    return GDT_TRUE;
  }
  int             GetSegment() const       { return Segment & 0x7FFF; }

  ~RECORD();

private:
  STRING         Key;
  STRING         baseDir;
  PATHNAME       Pathname;
  PATHNAME       origPathname;
  UINT4          RecordStart;
  UINT4          RecordEnd;
  DOCTYPE_ID     DocumentType;
  DFT            Dft;
  LOCALE         Locale;

  SRCH_DATE      Date;
  SRCH_DATE      DateModified;
  SRCH_DATE      DateCreated;
  SRCH_DATE      DateExpires;

  _ib_category_t Category;
  _ib_priority_t Priority;

  UINT2          Segment; // 
};


typedef RECORD* PRECORD;

// Common Functions
inline void Write (const RECORD& Record, PFILE Fp)
{
  Record.Write (Fp);
}

inline GDT_BOOLEAN Read (PRECORD RecordPtr, PFILE Fp)
{
  return RecordPtr->Read (Fp);
}

#endif
