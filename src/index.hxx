/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		index.hxx
Description:	Class INDEX
@@@*/

#ifndef INDEX_HXX
#define INDEX_HXX

#include "buffer.hxx"
#include "idbobj.hxx"
#include "date.hxx"
#include "mdt.hxx"
#include "squery.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"
#include "fcache.hxx"
#include "scan.hxx"
#include "numbers.hxx"
#include "intlist.hxx"
#include "gpolylist.hxx"
#include "bboxlist.hxx"

class FPT;
class Dictionary;

class INDEX /* : public IDBOBJ */ {
friend class IDB;
public:
  enum MATCH {Unspecified, LeftMatch, Exact, ExactTerm, ExactTermCase,
	Phonetic, PhoneticCase, AlwaysMatches, LeftAlwaysMatches,
	Numerical, FreeForm, Phrase};
  const char *MatchType(enum MATCH Typ) const;

  INDEX(const PIDBOBJ DbParent, const STRING& NewFileName, size_t CacheSize = 0);


  STRING Description() const;

  STRING GetDbFileStem() const { return Parent ? Parent->GetDbFileStem() : NulString; }

  void SetSisLimit(size_t NewLimit);
  size_t GetSisLimit() const { return SisLimit; }
  void SetCacheSize(size_t NewCacheSize);
  void SetCommonWordsThreshold(long newThreshold) { CommonWordsThreshold = newThreshold; }

  void   SetMaxRecordsAdvice(size_t Limit);
  size_t GetMaxRecordsAdvice() const;

  void SetClippingThreshold(size_t NewLimit) { ClippingThreshold = NewLimit; }
  size_t GetClippingThreshold() const        { return ClippingThreshold;     }

  // Max in Seconds
  void   SetMaximumTermCPU_sec(size_t newMax);
  size_t GetMaximumTermCPU_sec() const;
  void   SetMaximumQueryCPU_sec(size_t newMax);
  size_t GetMaximumQueryCPU_sec() const;



  bool WriteFieldData(const RECORD& Record, const GPTYPE GpOffset);

  bool AddRecordList(PFILE RecordListFp);

  bool ValidateInField(const GPTYPE HitGp, const STRING& FieldName) const;
  bool ValidateInField (const GPTYPE HitGp, FILE *Fp,  const size_t Total) const;
  bool ValidateInField (const GPTYPE HitGp, const GPTYPE *Buffer, const size_t Total) const;

  bool ValidateInField(const FC& HitFc, const STRING& FieldName) const;
  bool ValidateInField (const FC& HitFc, FILE *Fp,  const size_t Total) const;

  FCACHE      *GetFieldCache() { return FieldCache; }

  bool Ok() const { return OK && Version() != -1; }

  bool IsEmpty() const;
  off_t GetTotalWords() const;
  off_t GetTotalUniqueWords() const;

  bool SetStoplist(const LOCALE& Locale);
  bool SetStoplist(const STRING& LanguageOrFilename);

  bool IsStopWord(const char *FieldName, const UCHR *WordStart, STRINGINDEX WordMaximum, INT Limit = 0) const;
  bool IsStopWord(const UCHR *WordStart, STRINGINDEX WordMaximum, INT Limit = 0) const;
  bool IsStopWord(const STRING& FieldName, const STRING& Word) const;
  bool IsStopWord(const STRING& Word) const;

  size_t Scan(PSTRLIST List, const STRING& Field,
	const size_t Offset = 0, const INT TotalTermsRequested = -1, const size_t Start = 0) const;
  size_t Scan(PSTRLIST ListPtr, const STRING& Fieldname,
	const STRING& Term, const INT TotalTermsRequested = -1) const;
  size_t ScanLR(PSTRLIST ListPtr, const STRING& Fieldname,
	const STRING& Term, const size_t Position = 0, const INT TotalTermsRequested = -1) const;
  size_t ScanGlob(PSTRLIST ListPtr, const STRING& Fieldname,
	const STRING& Pattern, const size_t Position = 0, const INT TotalTermsRequested = -1) const;

  size_t Scan(SCANLIST *List, const STRING& Field,
	const size_t Offset = 0, const INT TotalTermsRequested = -1,
	const size_t Start = 0, bool Cat = false, size_t Sis_id = 0) const;
  size_t Scan(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Term,
	const INT TotalTermsRequested = -1, bool Cat = false, size_t Sis_id=0) const;
  size_t ScanLR(SCANLIST *ListPtr, const STRING& Fieldname,
	const STRING& Term, const size_t Position = 0,
	const INT TotalTermsRequested = -1, bool Cat = false, size_t Sis_id=0) const;
  size_t ScanGlob(SCANLIST *ListPtr, const STRING& Fieldname,
	const STRING& Pattern, const size_t Position = 0,
	const INT TotalTermsRequested = -1, bool Cat = false, size_t Sis_id=0) const;

  size_t ScanSoundex(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const size_t HashLen = 6,
        const size_t Position = 0, const INT TotalTermsRequested = -1, bool Cat = false,
	size_t Sis_id = 0) const;

  size_t ScanMetaphone(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const size_t Position = 0, const INT TotalTermsRequested = -1, bool Cat = false,
	size_t Sis_id = 0) const;

  // Scan for field contents according to a search
  size_t ScanSearch(SCANLIST *ListPtr, const QUERY& Query, const STRING& Fieldname, 
	 size_t MaxRecordsThreshold = 0, bool Cat = false);
  SCANLIST ScanSearch(const QUERY& Query, const STRING& Fieldname, size_t MaxRecordsThreshold = 0);

  // Type Specific Scans

  size_t DateScan(SCANLIST *List, const STRING& FieldName, size_t Start = 0,
	INT TotalRequested = -1);
  size_t DateScan(STRLIST *Strlist, const STRING& FieldName, size_t Start = 0,
	INT TotalRequested = -1);
  size_t NumericalScan(SCANLIST *List, const STRING& FieldName, size_t Start =0,
	INT TotalRequested = -1);
  size_t NumericalScan(STRLIST *Strlist, const STRING& FieldName, size_t Start = 0,
	INT TotalRequested = -1);
  size_t GPolyScan(SCANLIST *List, const STRING& FieldName, size_t Start = 0,
	INT TotalRequested = -1);
  size_t GPolyScan(STRLIST *Strlist, const STRING& FieldName, size_t Start = 0,
	INT TotalRequested = -1);

  bool SetDateRange(const DATERANGE& Range);
  bool GetDateRange(DATERANGE *Range = NULL) const;

  //size_t TermFrequency(const STRING& Term, const STRING& Fieldname = NulString) const;

  PIRSET Search(const QUERY& SearchQuery);
  PIRSET NumericSearch(const NUMBER fKey, const STRING& FieldName, INT4 Relation = ZRelEQ);
  PIRSET MonetarySearch(const MONETARYOBJ& Price, const STRING& FieldName, INT4 Relation = ZRelEQ);
  PIRSET HashSearch(const STRING& Contents, const STRING& FieldName, INT4 Relation = ZRelEQ, bool useCase = true);
  PIRSET LexiHashSearch(const STRING& Term, const STRING& FieldName, INT4 Relation = ZRelEQ);

  PIRSET FileSearch(const STRING& FileSpecification);
  PIRSET KeySearch (const STRING& KeySpecification);
  PIRSET DoctypeSearch (const STRING& DoctypeSpecification);

  // Bounding Box search

  PIRSET BoundingRectangle(const char *Term,
	NUMBER* NorthBC= NULL, NUMBER *SouthBC=NULL,
	NUMBER *WestBC=NULL, NUMBER *EastBC=NULL);
  PIRSET BoundingRectangle(NUMBER NorthBC, NUMBER SouthBC,
	NUMBER WestBC, NUMBER EastBC);
  PIRSET Interval(NUMBER WestLongitude, NUMBER EastLongitude,
                NUMBER SouthLatitude, NUMBER NorthLatitude);
  void   SetSpatialScores(IRSET* pirset, STRING FieldName,
	NUMBER NorthBC, NUMBER SouthBC, NUMBER WestBC, NUMBER EastBC);

  // Simple date searches
  PIRSET DateSearch(const STRING& Term, const STRING& FieldName, INT4 Relation);
  PIRSET DateSearch(const SRCH_DATE& Key, const STRING& FieldName, INT4 Relation);


  // More complicate Date fields

  PIRSET DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
                      INT4 Relation, INT4 Structure);
  PIRSET DoDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
                      INT4 Relation, INT4 Structure, bool Strict);
  PIRSET DateRangeSearch(const STRING& QueryTerm, const STRING& FieldName, 
                         INT4 Relation, bool Strict);
  PIRSET SingleDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
                          INT4 Relation, bool Strict);
  PIRSET SingleDateSearchBefore(const SRCH_DATE& QueryDate, 
                                const STRING& FieldName, 
                                IntBlock FindBlock,
                                bool EndpointFlag);
  PIRSET YSearchBefore(const SRCH_DATE& DateY, const STRING& FieldName,
                       IntBlock FindBlock, bool EndpointFlag);
  PIRSET YMSearchBefore(const SRCH_DATE& DateYM, const STRING& FieldName,
                        IntBlock FindBlock, bool EndpointFlag);
  PIRSET YMDSearchBefore(const SRCH_DATE& QueryDate, const STRING& FieldName,
                         IntBlock FindBlock, bool EndpointFlag);
  PIRSET SingleDateSearchAfter(const SRCH_DATE& QueryDate, 
                               const STRING& FieldName, 
                               IntBlock FindBlock,
                               bool EndpointFlag);
  PIRSET YMDSearchAfter(const SRCH_DATE& DateYMD, const STRING& FieldName,
                        IntBlock FindBlock, bool EndpointFlag);
  PIRSET YMSearchAfter(const SRCH_DATE& DateYM, const STRING& FieldName,
                       IntBlock FindBlock, bool EndpointFlag);
  PIRSET YSearchAfter(const SRCH_DATE& DateY, const STRING& FieldName,
                      IntBlock FindBlock, bool EndpointFlag);
  PIRSET DateRangeSearchContains(const DATERANGE& QueryDate, 
                                 const STRING& FieldName,
                                 IntBlock FindBlock,
                                 bool EndpointFlag);
  // This searches indexes of intervals
  IRSET* DateSearch(const DOUBLE fKey, const STRING& FieldName, 
                        INT4 Relation, IntBlock FindBlock);


 PIRSET MetaphoneSearch(const STRING& SearchTerm, const STRING& FieldName,
        bool UseCase = false);
  PIRSET SoundexSearch(const STRING& SearchTerm, const STRING& FieldName,
	bool UseCase = false);
  PIRSET GlobSearch(const STRING& SearchTerm, const STRING& FieldName,
	bool UseCase = false);
  PIRSET TermSearch(const STRING& SearchTerm, const STRING& FieldName,
	enum MATCH Typ=Unspecified);

  bool IsSystemFile(const STRING& FileName) const;
  bool IsWrongEndian() const; // Swap of not?
  INT Version() const; // Index Version

  INT Cleanup();

  void SetMergeStatus(t_merge_status x=iOptimize);
  t_merge_status GetMergeStatus() const { return MergeStatus; }

  void SortNumericFieldData();

  bool KillAll ();

  long Dump(INT Skip, ostream& os = cout, bool OnlyErrors=false);
  void Dump(ostream& os = cout);
  long CheckIndex(ostream& os = cout) {
    return Dump(0, os, true);
  }
  long CheckIndex(INT Skip, ostream& os = cout) {
    return Dump(Skip, os, true);
  }

  bool CreateCentroid();
  bool WriteCentroid(FILE *fp);

  PFILE ffopen(const STRING& FileName, const CHR* Type) const;
  INT  ffclose (PFILE FilePointer) const;

  void SetDocTypePtr(const PDOCTYPE NewDocTypePtr);
  PDOCTYPE GetDocTypePtr() const;

  void SetDebugMode(bool OnOff) { DebugMode = OnOff; }
  bool GetDebugMode() const { return DebugMode; };

  // Profile
  STRING ProfileGetString(const STRING& Section, const STRING& Entry,
        const STRING& Default=NulString) const {
    STRING Result;
    Parent->ProfileGetString(Section, Entry, Default, &Result);
    return Result;
  }

  ~INDEX();

  size_t ImportIndex(INDEX *IndexPtr); // Returns number of records addded

  INT GpFwrite(const GPTYPE Gp, FILE *Stream) const;
  INT GpFwrite(const GPTYPE * const Ptr, size_t NumElements, FILE* Stream) const;

  GPTYPE GpFread(FILE *Stream) const;
  GPTYPE GpFread(off_t Position, FILE *Stream) const;

  INT GpFread(GPTYPE *Ptr, FILE *Stream) const;
  INT GpFread(GPTYPE *Ptr, size_t NumElements, FILE* Stream) const;
  INT GpFread(GPTYPE *Ptr, size_t NumElements, off_t Position, FILE *Stream) const;

  bool MergeIndexFiles();
  bool CollapseIndexFiles();

  INT SisWrite(const STRING Str, FILE *dp);

  void     setCharset(const CHARSET &Set) { Charset = Set;  }
  CHARSET& getCharset()                   { return Charset; }

  // Service function
  size_t ReadIndirect(const STRING& Filename, PUCHR Buffer,
	off_t Start, size_t Length, const DOCTYPE_ID& Doctype) const {
	return ::ReadIndirect(Filename, (char *)Buffer, Start,
	         Length, Parent->GetDocTypePtr ( Doctype )); }

  FPT* GetMainFpt() { return (Parent ? Parent->GetMainFpt()  : NULL); }

  GPTYPE getIndexingTotalBytesCount () const { return IndexingTotalBytesCount;}
  GPTYPE getIndexingTotalWordsCount () const { return IndexingTotalWordsCount;}

  void SetFindConcatWords(bool Set=true) { findConcatWords = Set; }
  bool GetFindConcatWords() const { return findConcatWords; }

  bool CheckIntegrity() const;

private:
  void  CPU_ResourcesExhausted (); // Exception

  long TermFreq(const STRING& Word, bool Truncate=false);
  bool IsSpecialTerm(const UCHR *Term) const;
  // These two are *very* private 
  INT find(INT Index, const STRING& Word, bool Truncate, off_t *start,
	bool *overflow);
  INT find(const STRING& SisFn, const INT Slot, const STRING& Word,
	bool Truncate, off_t *start, bool *overflow);

  int findIt(MMAP *MemoryMap, const UCHR *Term, size_t TermLength, bool Truncate,
	off_t *start, bool *overflow);
  int findIt(bool Truncate, const STRING& Index,
	const UCHR *Term, size_t TermLength, off_t *start, bool *overflow);

  GPTYPE * const GpPtr(size_t SubIndex, GPTYPE Position) const;
  GPTYPE GpOf(GPTYPE *Ptr, size_t Index) const;

  PFILE GetFilePointer(const GPTYPE gp, const INT Off = 0) const;
  INT GetIndirectBuffer(const GPTYPE Gp, PUCHR Buffer, INT Off=0,
	INT Len=StringCompLength) const;
  INT    GetIndirectTerm(const GPTYPE Gp, PUCHR Buffer, INT MaxLength = StringCompLength) const;
  STRING GetIndirectTerm(GPTYPE Gp) const;

  void TermSort(const void *base, void *MemoryIndex, size_t MemoryIndexLength) const;
  size_t BuildGpList(const DOCTYPE_ID& Doctype,
	GPTYPE StartingPosition, PUCHR MemoryData,
	GPTYPE MemoryDataLength, PGPTYPE MemoryIndex,
	GPTYPE MemoryIndexLength);

  bool FlushIndexFiles(PUCHR MemoryData, 
	PGPTYPE NewMemoryIndex, GPTYPE MemoryIndexLength,
	GPTYPE GlobalStart);

  bool SetIndexNum(INT Num);
  INT         GetIndexNum() const;

  size_t      _scanAddEntry(SCANLIST *, BUFFER *, size_t, bool, FILE *, const char *,
	const char *, const size_t = 0, bool = false) const;

  void        DumpPersistantCache();

  void        LoadBoundingBoxFieldNames();

  NUMERICOBJ  encodeHash(const STRING& String) const;
  NUMERICOBJ  encodeCaseHash(const STRING& String) const;
  NUMERICOBJ  encodeLexiHash(const STRING& String) const;
  FILE       *OpenForAppend(const STRING& FieldName, FIELDTYPE FieldType);

// Variables (constants)
  size_t      SisLimit;
  size_t      ClippingThreshold;
  size_t      TooManyRecordsThreshold;
  clock_t     MaxCPU_ticks, MaxQueryCPU_ticks;
  bool DebugMode;
  bool findConcatWords;
  bool forceConcatWords;
  bool storeHitCoordinates; // Everything fulltext (no coordinates, no proximity?)

  int         CommonWordsThreshold;
  int         PhraseWaterlimit;

  STRING      NorthFieldName, EastFieldName, SouthFieldName, WestFieldName;

  DATERANGE   DateRange;


  GPTYPE      SIScount;
  STRING      lastSIString;
  t_merge_status MergeStatus;
  STRING      IndexFileName;
  STRING      SisFileName;
  PIDBOBJ     Parent;
  PMDT        MemoryMdt;
  UINT2       IndexMagic;
  bool wrongEndian;
  bool useSoundex;
  INT         IndexNum; // count of indexes to merge

  // Volatile and active stuff

  RCACHE              *SetCache;
  FCACHE              *FieldCache;

  volatile Dictionary  *NumFieldCache;

  MMAP_TABLE *MemorySISCache; // The SIS cache
  MMAP_TABLE *MemoryIndexCache; // The Index cache

  PDOCTYPE    DocTypePtr;
  LISTOBJ*    StopWords;
  CHARSET     Charset;
  HASH*       TermAliases;
  STRLIST    *CommonWords;

  volatile GPTYPE IndexingTotalBytesCount;
  volatile GPTYPE IndexingTotalWordsCount;
  volatile GPTYPE IndexingWordsTruncated;
  volatile GPTYPE IndexingWordsLongestLength;

  volatile bool ActiveIndexing;

  // Set at start to indicate things are OK 
  bool OK;
};

typedef INDEX* PINDEX;

#ifndef H_PERL

inline GPTYPE GP(const void *ptr, int y = 0)
{
#if IS_BIG_ENDIAN
  register GPTYPE Gpp;
  memcpy(&Gpp, (const BYTE *)ptr+y, sizeof(GPTYPE));
  return Gpp;
#else
# ifdef O_BUILD_IB64
  const BYTE *x = (const BYTE *)ptr;
  return (((GPTYPE)x[y])   << 56) + (((GPTYPE)x[y+1]) << 48) +
         (((GPTYPE)x[y+2]) << 40) + (((GPTYPE)x[y+3]) << 32) +
         (((GPTYPE)x[y+4]) << 24) + (((GPTYPE)x[y+5]) << 16) + 
	  (((GPTYPE)x[y+6]) << 8) + x[y+7];

# else
  const BYTE *x = (const BYTE *)ptr;
  return (((GPTYPE)x[y])   << 24) + (((GPTYPE)x[y+1]) << 16) +
         (((GPTYPE)x[y+2]) << 8)  + x[y+3];
# endif
#endif
}
//inline GPTYPE GP(const void *x) { return GP(x, 0); }
#endif

#endif
