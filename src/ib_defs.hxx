/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _IB_DEFS_HXX
# define _IB_DEFS_HXX 1

#ifdef PURE_STANDALONE
# ifndef STANDALONE
#   define STANDALONE 1
# endif
#endif
#ifndef STANDALONE

//#include <iostream>
#include "elements.hxx"

#if 1
typedef enum SortBy {
  Unsorted, ByDate, ByReverseDate, ByScore, ByAdjScore, ByAuxCount,
  ByHits, ByReverseHits, ByKey, ByIndex, ByCategory, ByNewsrank, ByFunction,
  ByPrivate, ByPrivateLocal1, ByPrivateLocal2, ByPrivateLocal3, ByExtIndex=64,
  ByExtIndex1,  ByExtIndex2, ByExtIndex3, ByExtIndex4, ByExtIndex5, ByExtIndex6, ByExtIndex7,
  ByExtIndex8, ByExtIndex9, ByExtIndex10, ByExtIndex11, ByExtIndex12,
  ByExtIndexLast = 127 /* External sorts are 0 - 63 */
} SortBy;

#else
// ByTime     := Time, ignoring date
// ByLocation := File Path
// ByName     := File name (not path)
// BySize     := By Record size

// Max 127
enum SortBy_t {
  Unsorted,   ByDate,     ByTime,    ByScore,         ByAdjScore,      ByAuxCount,
  ByHits,     ByKey,      ByIndex,   ByCategory,      ByNewsrank,      ByFunction,
  ByLocation, BySize,     ByPrivate, ByPrivateLocal1, ByPrivateLocal2, ByPrivateLocal3,
  ByExtIndex=64
};

enum SortDirection_t {
  SortDescending=-1, Unspecified=0, SortAscending=1
};

// NOTE: Unspecified means Descending sort too!

class SortBy {
public:
  SortBy(enum SortBy_t SortBy = Unsorted, enum SortDirection_t SortDirection = Unspecified) {
    Sort      = SortBy;
    Direction = SortDirection;
  }
  SortBy& operator =(const SortBy& Other) {
    Sort      = Other.Sort;
    Direction = Other.Direction;
    return *this;
  }
  bool IsAscending() const  { return Direction == SortAscending; }
  bool IsDescending() const { return Direction != SortAscending; }

  SortBy& operator =(int Value) {
    BYTE  Val = (BYTE)(((unsigned)Value) & 0xFF);
    if (Val > 0x7F)
      {
	Direction = SortAscending;
	Sort      = (enum SortBy_t)(Val & 0x7F);
      }
    else
      {
 	Direction = SortDescending;
	Sort      = (enum SortBy_t)Val;
      }
    return *this;
  }

  operator enum SortBy_t() const {
    return Sort;
  }
  operator enum SortDirection_t() const {
    return Direction;
  }

  operator int () const {
    BYTE val = (BYTE)Sort;
    if (Direction == SortAscending) val += 0x7F;
    return val;
  }

  void Write(FILE *fp) const {
    ::Write((BYTE)*this, fp);
  }
  bool Read(FILE *fp) {
    BYTE          x;
    if (::Read(&x, fp))
      {
	*this = x;
	return true;
      }
    return false;
     
  }
  enum SortBy_t        Sort;
  enum SortDirection_t Direction;
}

#endif

//
// This class stores the magic for handling the names for the dispatch
// and mangement of document handlers.
//
// The Id is a number that maps to the handler (which is an enumeration)
// The Name is what it should be called. This can be something completely
// different from what the name that corresponds to the Id
//
// See: dtregs.cxx
//
class          DOCTYPE_ID {
 public:
  DOCTYPE_ID();
  DOCTYPE_ID(const char *Doctype);
  DOCTYPE_ID(const STRING& DocType);

  DOCTYPE_ID&   operator = (const DOCTYPE_ID& NewId);

  STRING        DocumentType() const;

  void          Set(const STRING& Value);
  STRING        Get() const;

  STRING        ClassName(bool Base = false) const;

  const char   *c_str() const    { return DocumentType().c_str(); }

  operator      STRING () const  { return DocumentType();         }
//operator        INT () const   { return Id;                     }
//operator   const char *() const{ return c_str();                }
  bool operator !() const { return Id <= 1; }
  bool  IsDefined() const { return Id > 1;  }
  bool  IsEmpty() const   { return Id <= 0; }

  void         Clear() {   Name.Clear(); Id = 0;   }

  friend inline ostream& operator<<(ostream& os, const DOCTYPE_ID& Id) {
    return os << Id.ClassName() ;
  }

  bool   Equals(const DOCTYPE_ID& Other) const {
    return (Id == Other.Id) &&
           (Name ^= Other.Name); 
  }
  bool   Equals(const STRING& name) const { return Name ^= name; }
  bool   Equals(const INT id) const       { return Id == id;     }

  ~DOCTYPE_ID();

  void          Write(FILE *fp) const;
  bool   Read(FILE *fp);

  UINT2         Id;   // Which Class (see dispatch)
  STRING        Name; // What we shall call it
};

extern const DOCTYPE_ID& NulDoctype;
extern const DOCTYPE_ID& ZeroDoctype;
extern const DOCTYPE_ID& NilDoctype;

// Operators
inline bool operator==(const DOCTYPE_ID& s1, const DOCTYPE_ID& s2) { return s1.Equals(s2);  }
inline bool operator==(const DOCTYPE_ID& s1, const STRING& s2)     { return s1.Equals(s2);  }
inline bool operator==(const STRING& s1, const DOCTYPE_ID& s2)     { return s2.Equals(s1);  }
inline bool operator==(const DOCTYPE_ID& s1, INT s2)               { return s1.Equals(s2);  }
inline bool operator!=(const DOCTYPE_ID& s1, const DOCTYPE_ID& s2) { return !s1.Equals(s2); }
inline bool operator!=(const DOCTYPE_ID& s1, const STRING& s2)     { return !s1.Equals(s2); }
inline bool operator!=(const STRING& s1, const DOCTYPE_ID& s2)     { return !s2.Equals(s1); }
inline bool operator!=(const DOCTYPE_ID& s1, INT s2)               { return !s1.Equals(s2); }

// Read/Write
inline void Write(const DOCTYPE_ID& Buffer, PFILE Fp) { Buffer.Write(Fp);     }
inline bool Read(DOCTYPE_ID *Ptr, PFILE Fp)    { return Ptr->Read(Fp); }

typedef UINT4   _ib_category_t; // Category Type
typedef INT2    _ib_priority_t; // Priority

const unsigned int MdtIndexCapacity = (1L << 25) - 1; // See below
const unsigned int VolIndexCapacity = 0xFF; // See below

typedef UINT4  _index_id_t;

#define MAX_VIRTUAL_INDEXES 255

class INDEX_ID {
public:
  INDEX_ID()                       { Index = 0;           }
  INDEX_ID(const INDEX_ID& Other)  { Index = Other.Index; }
  INDEX_ID(const _index_id_t index){ Index = index;       }

  INDEX_ID& operator=(const INDEX_ID& Val) { Index = Val.Index; return *this; }

  operator _index_id_t () const { return Index; }

/*
  operator ++() { Index = Index + 1; return *this; }
  operator --() { Index = Index - 1; return *this; }
*/

  bool Equals(const INDEX_ID& Val) const  { return Val.Index == Index; }
  INT         Compare(const INDEX_ID& Val) const { return Index - Val.Index;  }

  _index_id_t GetIndex() const { return Index; }
  void        SetMdtIndex(const INT NewMdtIndex) {
    Index = (NewMdtIndex | (Index & 0xFF000000));
  }
  INT         GetMdtIndex() const { return (Index & 0x00FFFFFF); };
  void        SetVirtualIndex(const UCHR NewvIndex) {
    Index = (Index & 0x00FFFFFF) | (((long)NewvIndex) << 24);
  }
  INT        GetVirtualIndex() const { return ((Index & 0xFF000000) >> 24);}

  void       Write(FILE *fp) const { ::Write(Index, fp); }
  void       Read(FILE *fp)        { ::Read(&Index, fp); }
private:
  _index_id_t Index;
};


// Read/Write
inline void Write(const INDEX_ID& Buffer, PFILE Fp) { Buffer.Write(Fp);     }
inline void Read(INDEX_ID *Ptr, PFILE Fp)    { Ptr->Read(Fp); }

// Operators
inline bool operator==(const INDEX_ID& s1, const INDEX_ID& s2)  { return s1.Equals(s2);      }
inline bool operator==(const _index_id_t s1, const INDEX_ID& s2){ return s2.Equals(s1);      }
inline bool operator==(const INDEX_ID& s1, _index_id_t s2)      { return s1.Equals(s2);      }
inline bool operator!=(const INDEX_ID& s1, const INDEX_ID& s2)  { return !s1.Equals(s2);     }
inline bool operator!=(const _index_id_t s1, const INDEX_ID& s2){ return !s2.Equals(s1);     }
inline bool operator!=(const INDEX_ID& s1, _index_id_t s2)      { return !s1.Equals(s2);     }
inline bool operator<(const INDEX_ID& s1, const INDEX_ID& s2)   { return s1.Compare(s2) < 0; }
inline bool operator<(const _index_id_t s1, const INDEX_ID& s2) { return s2.Compare(s1) < 0; }
inline bool operator<(const INDEX_ID& s1, _index_id_t s2)       { return s1.Compare(s2) < 0; }
inline bool operator<=(const INDEX_ID& s1, const INDEX_ID& s2)  { return s1.Compare(s2) <= 0;}
inline bool operator<=(const _index_id_t s1, const INDEX_ID& s2){ return s2.Compare(s1) <= 0;}
inline bool operator<=(const INDEX_ID& s1, _index_id_t s2)      { return s1.Compare(s2) <= 0;}
inline bool operator>(const INDEX_ID& s1, const INDEX_ID& s2)   { return s1.Compare(s2) > 0; }
inline bool operator>(const _index_id_t s1, const INDEX_ID& s2) { return s2.Compare(s1) > 0; }
inline bool operator>(const INDEX_ID& s1, _index_id_t s2)       { return s1.Compare(s2) > 0; }
inline bool operator>=(const INDEX_ID& s1, const INDEX_ID& s2)  { return s1.Compare(s2) >= 0;}
inline bool operator>=(const _index_id_t s1, const INDEX_ID& s2){ return s2.Compare(s1) >= 0;}
inline bool operator>=(const INDEX_ID& s1, _index_id_t s2)      { return s1.Compare(s2) >= 0;}

inline INT operator-(const INDEX_ID& s1, const INDEX_ID& s2) { return s1.GetIndex()-s2.GetIndex();  }
inline INT operator+(const INDEX_ID& s1, const INDEX_ID& s2) { return s1.GetIndex()+s2.GetIndex();  }



class SORT_INDEX_ID {
public:
  SORT_INDEX_ID()                                   { Index = 0;                   }
  SORT_INDEX_ID(const INDEX_ID& Other)              { Index = Other.GetMdtIndex(); }
  SORT_INDEX_ID(const SORT_INDEX_ID& Other)         { Index = Other.Index;         }
  SORT_INDEX_ID(const _index_id_t index)            { Index = index;               }
  SORT_INDEX_ID(FILE *fp, const INDEX_ID& index_id) { Set(fp, index_id);           }

  SORT_INDEX_ID& operator=(const SORT_INDEX_ID& Val) { Index = Val.Index; return *this; }

  bool    Ok() const { return Index != 0; }

  operator _index_id_t () const { return Index; }

  bool Equals(const SORT_INDEX_ID& Val) const  { return Val.Index == Index; }
  INT         Compare(const SORT_INDEX_ID& Val) const { return Index - Val.Index;  }

  _index_id_t GetIndex() const { return Index; }

  void       Write(FILE *fp) const { ::Write(Index, fp); }
  void       Read(FILE *fp)        { ::Read(&Index, fp); }

  void       Set(FILE *fp, const INDEX_ID& index_id) {
    Index = 0;
    if (fp && fseek(fp, index_id.GetMdtIndex()*sizeof(Index) , SEEK_SET) != -1)
      Read(fp);
  }

private:
  _index_id_t Index;
};

#if 0
class PRESORT_MOD {
public:
  PRESORT_MOD(MDT *Mdt, const STRING& Filename);

  ~PRESORT_MOD();
}

#endif

inline INT Compare(const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2)     { return s1.Compare(s2);         }
// Operators
inline bool operator==(const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return s1.Equals(s2);    }
inline bool operator!=(const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return !s1.Equals(s2);   }
inline bool operator< (const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return s1.Compare(s2)< 0;}
inline bool operator<=(const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return s1.Compare(s2)<=0;}
inline bool operator> (const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return s1.Compare(s2)> 0;}
inline bool operator>=(const SORT_INDEX_ID& s1, const SORT_INDEX_ID& s2){ return s1.Compare(s2)>=0;}

class DOC_ID {
 public:
  DOC_ID() { Index = 0; }
  DOC_ID(const INT index) { Index = index; }
  DOC_ID(const INDEX_ID Id) { Index = Id.GetMdtIndex(); }
  DOC_ID(const STRING& GlobalKey) {
    STRINGINDEX pos = (Key = GlobalKey).Search('@');
    if (pos) { 
      if ((Index = Key.GetInt()) <= 0)
	Index = 1; 
      Key.EraseBefore(pos+1);
    }
  }
  DOC_ID(const STRING& key, const INDEX_ID Id) {
    Key   = key;
    Index = Id.GetMdtIndex();
  }
  DOC_ID&   operator = (const DOC_ID& NewId) {
    Key   = NewId.Key;
    Index = NewId.Index;
    return *this;
  }
  bool Equals(const DOC_ID& OtherDoc) const {
    return ((Index == OtherDoc.Index) && (Key == OtherDoc.Key));
  }
  INT Compare(const DOC_ID& OtherDoc) const {
    INT diff = Key.Compare(OtherDoc.Key);
    if (diff == 0)
      return Index - OtherDoc.Index;
    return diff;
  }
  STRING GlobalKey() const {
    return STRING(Index) + "@" + Key;
  }
  operator STRING () const { return GlobalKey(); }
  ~DOC_ID() { }
 private:
  STRING Key;
  INT    Index;
};
// Inline comparison overloads...
inline bool operator==(const DOC_ID Id1, const DOC_ID Id2) {
  return Id1.Equals(Id2);
}
inline bool operator!=(const DOC_ID Id1, const DOC_ID Id2) {
  return !Id1.Equals(Id2);
}
inline bool operator>=(const DOC_ID Id1, const DOC_ID Id2) {
  return Id1.Compare(Id2) >= 0;
}
inline bool operator<=(const DOC_ID Id1, const DOC_ID Id2) {
  return Id1.Compare(Id2) <= 0;
}
inline bool operator>(const DOC_ID Id1, const DOC_ID Id2) {
  return Id1.Compare(Id2) > 0;
}
inline bool operator<(const DOC_ID Id1, const DOC_ID Id2) {
  return Id1.Compare(Id2) < 0;
}


#if 0

STRING _GetResourcePath(const RECORD& FileRecord);


#endif

extern const STRING __IB_DefaultDbName;
extern const char  *__IB_Version;

extern const STRING Bib1AttributeSet;
extern const STRING StasAttributeSet;
extern const STRING IsearchAttributeSet;

const INT IsearchFieldAttr	= 1;
const INT IsearchWeightAttr	= 7;
const INT IsearchTypeAttr	= 8;

// Record Syntaxes
#ifndef __RECORDSYNTAX
extern const STRING SutrsRecordSyntax;
extern const STRING UsmarcRecordSyntax;
extern const STRING HtmlRecordSyntax;
extern const STRING SgmlRecordSyntax;
extern const STRING XmlRecordSyntax;
extern const STRING RawRecordSyntax;
extern const STRING DVBHtmlRecordSyntax;
#endif

// Elements
extern const STRING BRIEF_MAGIC;
extern const STRING FULLTEXT_MAGIC;
extern const STRING SOURCE_MAGIC;
extern const STRING LOCATION_MAGIC;
extern const STRING METADATA_MAGIC;
extern const STRING HIGHLIGHT_MAGIC;


// Attributes
enum ZAttrTypes {
  ZUseAttribute = 1,
  ZRelationAttribute = 2,
  ZPositionAttribute = 3,
  ZStructureAttribute = 4,
  ZTruncationAttribute = 5,
  ZCompletenessAttr = 6
};


// Z39.50 AttributeValues (also other profiles)
typedef enum ZRelations {
  ZRelNoop = 0,
  ZRelLT,
  ZRelLE,
  ZRelEQ,
  ZRelGE,
  ZRelGT,
  ZRelNE,
  ZRelOverlaps,
  ZRelEnclosedWithin,
  ZRelEncloses,
  ZRelOutside,
  ZRelNear,
  ZRelMembersEQ,
  ZRelMembersNE,
  ZRelBefore,
  ZRelBeforeDuring,
  ZRelDuring,
  ZRelDuringAfter,
  ZRelAfter,

  ZRelPhonetic = 100,
  ZRelStem = 101,
  ZRelRelevance = 102,
  ZRelAlwaysMatches = 103,

// Strict date searching for GEO profile tests
// The default date search for GEO matches if any date in the target
// interval matches the query date.  These attributes change the default
// to match only if all dates in the target interval matches the query.
  ZRelBefore_Strict       = 3014,
  ZRelBeforeDuring_Strict = 3015,
  ZRelDuring_Strict       = 3016,
  ZRelDuringAfter_Strict  = 3017,
  ZRelAfter_Strict        = 3018
} ZRelation_t;

enum ZStruct {
  ZStructPhrase      = 1,
  ZStructWord        = 2,
  ZStructKey         = 3,
  ZStructYear        = 4,
  ZStructDate        = 5,
  ZStructWordList    = 6,
  ZStructDateTime    = 100,
  ZStructNameNorm    = 101,
  ZStructNameUnnorm  = 102,
  ZStructStructure   = 103,
  ZStructURx         = 104,
  ZStructText        = 105, // FreeFormText
  ZStructDocText     = 106, // DocumentText 
  ZStructLocalNumber = 107,
  ZStructCoord       = 108,
  ZStructCoordString = 109,
  ZStructCoordRange  = 110,
  ZStructBounding    = 111,
  ZStructComposite   = 112,
  ZStructRealMeas    = 113,
  ZStructIntMeas     = 114,
  ZStructDateRange   = 115,
  ZGEOStructCoord       = 200,
  ZGEOStructCoordString = 201,
  ZGEOStructBounding    = 202,
  ZGEOStructComposite   = 204,
  ZGEOStructRealMeas    = 205,
  ZGEOStructIntMeas     = 206,
  ZGEOStructDateRange   = 207,
  ZLOCALGlob            = 300,// Local Extension
  ZLOCALNumericalRange  = 301 // Local Extension
};

enum ZTrunc {
  ZTruncRight     = 1,
  ZTruncLeft      = 2,
  ZTruncLeftRight = 3,
  ZTruncNone      = 100,
  ZTruncProcess   = 101,
  ZTruncRE1       = 102,
  ZTruncRE2       = 103
};

enum ZComplete {
  ZCompleteIncSub    = 1,
  ZCompleteCompSub   = 2,
  ZCompleteCompField = 3
};



// Configuration
  // We need to be long enough to support 58-bit encoding of 32-byte hashes
  // 32 * log(256) / log(58) + 1  -> (1.38 * 32 = 44
  // -> 48
const size_t DocumentKeySize = 48; // 36; // Bumped up for base58 keys

const size_t DocumentTypeSize = 16-1;

#if  !USE_MDTHASHTABLE
const size_t MaxDocPathNameSize = 128 - 2; // 256 - 2;
#endif

const size_t StringCompLength = 64; // 32; // 64; // how many characters to compare
const size_t DefaultSisLength = 28; // was 32

// Locks
const int L_WRITE  = 0x01;
const int L_READ   = 0x02;
const int L_APPEND = 0x04;
const int L_CHECK  = 0xF0;


// Indexing Status
typedef enum {
  IndexingStatusInit,
  IndexingStatusReading,
  IndexingStatusParsingRecord,
  IndexingStatusIndexingDocument,
  IndexingStatusRecordAdded,
  IndexingStatusIndexing,
  IndexingStatusFlushing,
  IndexingStatusParsingFiles,
  IndexingStatusParsingRecords,
  IndexingStatusMerging,
  IndexingStatusClose
} t_IndexingStatus;

// Extensions
enum DbExtensions {
  ExtDbInfo = 0,
  ExtIndex,
  ExtMdt,
  ExtMdtIndex,
  ExtMdtKeyIndex,
  ExtMdtGpIndex,
  ExtMdtStrings,
  ExtDfd,
  ExtDft,
  ExtIndexQueue1,
  ExtIndexQueue2,
  ExtTemp,
  ExtTemplate,
  ExtDict,
  ExtDesc,
  ExtCat,
  ExtVdb,
  ExtSta,
  ExtCache,
  ExtSynonyms,
  ExtSynParents,
  ExtSynChildren,
  ExtCentroid,
  ExtMno, // Obsolete
  ExtDbi, // Obsolete

  oExtIndex, // Other bits
  oExtMdt, // Other bits
  oExtMdtIndex, // Other bits
  oExtDfd, // Other bits
  oExtDft, // Other bits
  oExtDict, // Other bits

  ExtPath, // Path Extensions
  ExtCentroidCompressed,

  ExtLAST
};

#define ExtFIRST ExtDbInfo

extern const CHR* _DbExt(enum DbExtensions);
#define DbExtDbInfo        _DbExt(ExtDbInfo)
#define DbExtIndex         _DbExt(ExtIndex)
#define DbExtMdt           _DbExt(ExtMdt)
#define DbExtMdtIndex      _DbExt(ExtMdtIndex)
#define DbExtMdtKeyIndex   _DbExt(ExtMdtKeyIndex)
#define DbExtMdtGpIndex    _DbExt(ExtMdtGpIndex)
#define DbExtMdtStrings    _DbExt(ExtMdtStrings)
#define DbExtDfd           _DbExt(ExtDfd)
#define DbExtDft           _DbExt(ExtDft)
#define DbExtIndexQueue1   _DbExt(ExtIndexQueue1)
#define DbExtIndexQueue2   _DbExt(ExtIndexQueue2)
#define DbExtTemp          _DbExt(ExtTemp)
#define DbExtTemplate      _DbExt(ExtTemplate)
#define DbExtDict          _DbExt(ExtDict)
#define DbExtDesc          _DbExt(ExtDesc)
#define DbExtCat           _DbExt(ExtCat)
#define DbExtVdb           _DbExt(ExtVdb)
#define DbExtDbState       _DbExt(ExtSta)
#define DbExtCache         _DbExt(ExtCache)
#define DbExtDbSynonyms    _DbExt(ExtSynonyms)
#define DbExtDbSynParents  _DbExt(ExtSynParents)
#define DbExtDbSynChildren _DbExt(ExtSynChildren)
#define DbExtCentroid      _DbExt(ExtCentroid)
#define DbExtCentroidCompressed _DbExt(ExtCentroidCompressed)
/*
#define DbExtMno           _DbExt(ExtMno)
#define DbExtDbi           _DbExt(ExtDbi)
*/

extern const char * const __AncestorDescendantSeperator;
extern const char * const __CompilerUsed;
extern const char * const __HostPlatform;

extern const char * _IB_SUPPORT_EMAIL_ADDRESS;
extern const char * _IB_HTDOCS_HOME; 
extern const char * _IB_REG_ADDR;

extern const char * const __CopyrightData;


extern long __Register_IB_Application(const char *Appname = NULL, FILE *output = NULL, int DebugFlag = 0);
extern int  __IB_CheckUserRegistration(const char *file = ".ibreg");

extern long _IB_SerialID();

#endif /* !STANDALONE */
#endif
