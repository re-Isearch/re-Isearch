/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		idb.hxx
Version:	1.00
Description:	Class IDB
@@@*/

#ifndef IDB_HXX
#define IDB_HXX

#include "error.hxx"
#include "idbobj.hxx"
#include "date.hxx"
#include "record.hxx"
#include "index.hxx"
#include "mdt.hxx"
#include "squery.hxx"
#include "dfdt.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "lang-codes.hxx"
#include "recordsyntax.hxx"


class IDBVOL {
public:
  IDBVOL();
  IDBVOL(const STRING& dbname, INT volume);
  IDBVOL&     operator =(const IDBVOL& Other);
  bool operator==(const IDBVOL& Other) const {
	return Volume == Other.Volume && DbName == Other.DbName;}
  bool operator!=(const IDBVOL& Other) const {
	return Volume != Other.Volume || DbName != Other.DbName;}
  IDBVOL&     operator++()                  { Volume++; return *this; }
  IDBVOL&     operator--()                  { Volume--; return *this; }
  void        SetVolume(INT volume)         { Volume = volume; }
  INT         GetVolume() const             { return Volume;   }
  void        SetName(const STRING& dbname) { DbName = dbname; }
  STRING      GetName() const               { return DbName;   }
private:
  STRING      DbName;
  INT         Volume;
};

enum DbState {
  DbStateReady = 0,  // DB ready for searching
  DbStateBusy  = 0x2,  // DB files are being modified
  DbStateInvalid = -1  // DB files have been corrupted because  some operation was interrupted
};


class IDB : public IDBOBJ {
friend class INDEX;
friend class IRSET;
friend class DOCTYPE;
public:
  IDB();
  IDB(bool SearchOnly);
  IDB(const STRING& DBName, bool SearchOnly = false);
  IDB(IDBOBJ *myParent, const STRING& DBName, const STRLIST& NewDocTypeOptions,
	bool SearchOnly = false);
  IDB(const STRING& DBName, const STRLIST& NewDocTypeOptions,  bool SearchOnly = false);
  IDB(const STRING& NewPathName, const STRING& NewFileName,  bool SearchOnly = false);
  IDB(const STRING& NewPathName, const STRING& NewFileName, const STRLIST& NewDocTypeOptions,
	 bool SearchOnly = false);

  // Open a DB
  bool Open (const STRING& DBName, bool SearchOnly = false);
  bool Open (const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly = false);
  bool Open (const STRING& NewPathName, const STRING& NewFileName,
        const STRLIST& NewDocTypeOptions, bool SearchOnly = false);
  // Close a DB
  bool Close();

  void SetVolume(const STRING& Name, INT Vol);
  INT  GetVolume(STRING *StrBufferPtr = NULL) const;

  void SetFindConcatWords(bool Set=true) {
    if (MainIndex) MainIndex->SetFindConcatWords(Set);
  }
  bool GetFindConcatWords() const {
    if (MainIndex) return MainIndex->GetFindConcatWords();
    return false;
  }

  long TermFreq(const STRING& Word, bool Truncate=false) {
    if (MainIndex) return MainIndex->TermFreq(Word, Truncate);
    return 0;
  }

  // Stuff for virtual databases
  // Segment is the slot number in an array of IDB instances
  void   SetParent(IDBOBJ *Caller, int Segment) { Parent = Caller;  SegmentNumber = Segment;}
  void   SetSegment(const STRING& newName, int newNumber = -1);
  void   SetSegment(int newNumber) {
    if (newNumber >= 0) SegmentNumber = newNumber;
  }
  STRING      GetSegmentName() const { return SegmentName; }
  IDBOBJ     *GetParent()            { return Parent;      }

  int         Segment(const STRING& Name) {
    return Parent ? Parent->Segment(Name) : 0;
  }

  //
  void        SetWorkingDirectory(const STRING& Dir);
  void        ClearWorkingDirectoryEntry();

  int         SetErrorCode(int Error);
  int         GetErrorCode() const;
  const char *ErrorMessage() const;
  const char *ErrorMessage(int ErrorCode) const;

  void SetCommonWordsThreshold(off_t x) { if (MainIndex) MainIndex->SetCommonWordsThreshold(x); } 

  bool SetLocale(const LOCALE& Locale);
  bool SetLocale(const CHR *LocaleName = NULL);
  LOCALE      GetLocale() const { return GlobalLocale; }

  bool IsDbCompatible() const;
  bool IsEmpty() const;
  bool Ok () const { return IsDbCompatible() && !IsEmpty(); }

  int BitVersion() const;

  bool CreateCentroid() { return MainIndex ? MainIndex->CreateCentroid() : false; }

  void SetMergeStatus(t_merge_status x) { if (MainIndex) MainIndex->SetMergeStatus(x);};
  t_merge_status GetMergeStatus() const {
    return MainIndex ? MainIndex->GetMergeStatus() : iNothing; }

  void SetDbState(enum DbState DbState) const;
  enum DbState GetDbState() const;

  void   SetDefaultPriorityFactor(DOUBLE x);
  void   SetPriorityFactor(DOUBLE x) { PriorityFactor = x;    }
  DOUBLE GetPriorityFactor() const   { return PriorityFactor; }

  void   SetIndexBoostFactor(DOUBLE x)     { IndexBoostFactor =x;        }
  DOUBLE GetIndexBoostFactor() const       { return IndexBoostFactor;    }
  void   SetFreshnessBoostFactor(DOUBLE x) { FreshnessBoostFactor = x;   }
  DOUBLE GetFreshnessBoostFactor() const   { return FreshnessBoostFactor;}
  void   SetLongevityBoostFattor(DOUBLE x) { LongevityBoostFactor = x;   }
  DOUBLE GetLongevityBoostFactor() const   { return LongevityBoostFactor;}

  void      SetFreshnessBaseDateLine(const SRCH_DATE& d) {FreshnessBaseDateLine = d; }
  SRCH_DATE GetFreshnessBaseDateLine() const { return FreshnessBaseDateLine; }

  void   SetDbSisLimit(size_t m) { if (MainIndex) MainIndex->SetSisLimit(m); }
  size_t GetDbSisLimit() const { return MainIndex ? MainIndex->GetSisLimit() : 0; }

  void   SetDefaultDbSearchCutoff(size_t x);
  void   SetDbSearchCutoff(size_t m) { if (MainIndex) MainIndex->SetClippingThreshold(m);        }
  size_t GetDbSearchCutoff() const   { return MainIndex ? MainIndex->GetClippingThreshold() : 0;}
  void   SetDbSearchFuel(size_t Percent) {
    if      (Percent >= 100) SetDbSearchCutoff( 0);
    else if (Percent <= 0)   SetDbSearchCutoff(1);
    else SetDbSearchCutoff( Percent * GetTotalRecords() / 100 );
  }
  void SetDbSearchCacheSize(size_t NewCacheSize) { if (MainIndex) MainIndex->SetCacheSize(NewCacheSize); }


  // DOCTYPE STUFF
  bool     RegisterDoctype (const STRING& name, DOCTYPE* Ptr) {
    if (DocTypeReg && DocTypeReg->RegisterDocType(name, Ptr) != NULL)
      return true;
    return false;
  }
  bool     UnregisterDoctype (const STRING& name) {
    return DocTypeReg ? DocTypeReg->UnregisterDocType(name) : false;
  };
  bool     DoctypeAddPluginPath(const STRING& Path) {
    if (DocTypeReg == NULL || Path.IsEmpty()) return false;
    DocTypeReg->AddPluginPath(Path);
    return true;
  }
  bool     DoctypePluginExists(const STRING& Name) const {
     return DocTypeReg ? DocTypeReg->PluginExists(Name) : false; }

  const STRLIST&  GetAllDocTypes()  { return DocTypeReg->GetDocTypeList (); };
  void GetDocTypeOptions(PSTRLIST StringListBuffer) const {
	*StringListBuffer = DocTypeOptions;};
  const STRLIST  *GetDocTypeOptionsPtr() const { return &DocTypeOptions; }
  const STRLIST&  GetDocTypeOptions() const    { return  DocTypeOptions; }
  bool     ValidateDocType(const STRING& DocType) const;
  bool     ValidateDocType(const DOCTYPE_ID& Id) const;

  // MDT Metadata stuff
  SRCH_DATE DateCreated() const;
  SRCH_DATE DateLastModified() const;

  bool KeyLookup(const STRING& Key, PRESULT ResultBuffer = NULL) const;

  // First, Last, Next, Prev Record Keys
  STRING FirstKey() const;
  STRING LastKey() const;
  STRING NextKey(const STRING& Key) const;
  STRING PrevKey(const STRING& Key) const;

  bool ValidNodeName(const STRING& nodeName) const;

  STRING GetXMLHighlightRecordFormat(const RESULT& Result, const STRING& PageField = NulString,
	const STRING& TagElement = NulString);

  int    GetNodeOffsetCount (const GPTYPE HitGp, const STRING& NodeName = NulString,
	FC *ContentFC = NULL, FC *ParentFC = NULL, off_t *FirstInstancePos = NULL);

  FCT    GetDescendentsFCT (const FC& HitFc, const STRING& NodeName);
  FCT    GetDescendentsFCT (const FC& HitFc, FILE *Fp);
  size_t GetDescendentsContent (const FC HitFc, FILE *Fp, STRLIST *StrlistPtr);
  size_t GetDescendentsContent (const FC HitFc, const STRING& NodeName, STRLIST *StrlistPtr);

  size_t GetAncestorContent (RESULT& Result, const STRING& NodeName, STRLIST *StrlistPtr);

  FC     GetAncestorFc (const FC& HitFc, const STRING& NodeName);

  FC GetPeerFc (const GPTYPE& HitGp, STRING *NodeNamePtr = NULL);
  FC GetPeerFc (const FC& HitFc,     STRING *NodeNamePtr = NULL);

  TREENODE GetPeerNode (const GPTYPE& HitGp) {
     STRING name;
     FC     fc = GetPeerFc(HitGp, &name);
     return TREENODE(fc, name);
  }
  TREENODE GetPeerNode (const FC& HitFc) {
     STRING name;
     FC     fc = GetPeerFc(HitFc, &name);
     return TREENODE(fc, name);
  }

#if 0
  int GetNodeList (const FC& HitFc, TREENODELIST *NodeList);
#endif
  NODETREE GetNodeTree( const FC& HitFc);

  STRING GetPeerContent(const FC& HitFc);
  STRING GetPeerContentXMLFragement(const FC& HitFc);

  DFDT *GetDfdt() { return MainDfdt;};
  DFDT *GetDfdt(DFDT *DfdtBuffer, const RESULT *ResultPtr = NULL) {
    if (ResultPtr && GetRecordDfdt(*ResultPtr, DfdtBuffer))
      return DfdtBuffer;
    if (DfdtBuffer)
      *DfdtBuffer = *MainDfdt;
    return MainDfdt;
  }
  bool GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer);
  bool GetRecordDfdt(const RESULT& Result, DFDT *DfdtBuffer) {
    return GetRecordDfdt(Result.GetKey(), DfdtBuffer);
  }

//
  void SetMirrorBaseDirectory(const STRING& Mirror);
  void SetHTTPServer(const STRING& Server);
  void SetHTTPPages(const STRING& Pages);

  void   SetTitle(const STRING& NewTitle);
  STRING GetTitle() const     { return Title;     }
  void   SetComments(const STRING& NewComments);
  STRING GetComments() const  { return Comments;  }
  void   SetCopyright(const STRING& NewCopyright);
  STRING GetCopyright() const { return Copyright; }
  void   SetMaintainer(const STRING& NewName, const STRING& NewAddress);
  void   GetMaintainer(STRING *Name, STRING *Address) const;
  STRING GetMaintainer() const; // Html mailto:

//
  void DfdtAddEntry(const DFD& NewDfd) {  MainDfdt->AddEntry(NewDfd);};
  bool DfdtGetEntry(const size_t Index, PDFD DfdRecord) const {  return MainDfdt->GetEntry(Index, DfdRecord);};
  size_t DfdtGetTotalEntries() const { return MainDfdt->GetTotalEntries ();};
  PDOCTYPE GetDocTypePtr(const DOCTYPE_ID& DocType) const;
//void AddRecordList(const RECLIST& NewRecordList);

  off_t GetTotalWords() const;
  off_t GetTotalUniqueWords() const;
  size_t GetTotalRecords() const;
  size_t GetTotalDocumentsDeleted() const;

  size_t DeleteExpired();
  size_t DeleteExpired(const SRCH_DATE Now);

  void   SetIndexingMemory(const size_t MemorySize, bool Force=false);
  size_t GetIndexingMemory() const { return IndexingMemory;};

//  void SetDebugSkip(const INT Skip);

  bool FieldExists(const STRING& FieldName) const {
    return MainDfdt ? MainDfdt->FieldExists(FieldName) > 0 : false;
  }

  // Standard Search Interface function
  PIRSET Search(const QUERY& SearchQuery);

  // Obsolete
  PIRSET Search(const SQUERY& SearchQuery, enum SortBy Sort = Unsorted,
	enum NormalizationMethods Method = defaultNormalization) {
    return Search(QUERY(SearchQuery, Sort, Method));
  }

  // Smart Search
  PIRSET SearchSmart(QUERY *Query, const STRING& DefaultField=NulString);

  PIRSET SearchSmart(const QUERY& Query, SQUERY *SqueryPtr = NULL);
  PIRSET SearchSmart(const QUERY& Query, const STRING& DefaultField, SQUERY *SqueryPtr = NULL);

  // Obsolete
  PIRSET SearchSmart(const SQUERY& SearchQuery, enum SortBy Sort = Unsorted,
	enum NormalizationMethods Method = defaultNormalization, SQUERY *SqueryPtr = NULL) {
    return SearchSmart(QUERY(SearchQuery, Sort, Method), SqueryPtr);
  }
  PIRSET SearchSmart(const SQUERY& SearchQuery, const STRING& DefaultField,
	enum SortBy Sort = Unsorted, enum NormalizationMethods Method = defaultNormalization,
	SQUERY *SqueryPtr = NULL) {
    return SearchSmart(QUERY(SearchQuery, Sort, Method), DefaultField, SqueryPtr);
  };

  RSET *VSearch(const QUERY& SearchQuery); 
  RSET *VSearchSmart(QUERY *Query, const STRING& DefaultField=NulString);

  IRSET *TermSearch(const STRING& SearchTerm, const STRING& FieldName,
	INDEX::MATCH Typ=INDEX::Unspecified, INT Weight=1,
	enum NormalizationMethods Method= defaultNormalization) const {
    IRSET *irset = MainIndex->TermSearch(SearchTerm, FieldName, Typ);
    return Weight ? (IRSET *)irset->ComputeScores(Weight, Method) : irset;
  }

  PIRSET FileSearch(const STRING& FileSpecification) {
    return MainIndex->FileSearch(FileSpecification);
  }
  PIRSET KeySearch(const STRING& KeySpecification) {
    return MainIndex->KeySearch(KeySpecification);
  }
  PIRSET DoctypeSearch(const STRING& DoctypeSpecification) {
    return MainIndex->DoctypeSearch(DoctypeSpecification);
  }

  bool DfdtGetFileName(const STRING& FieldName, PSTRING StringBuffer);
  bool DfdtGetFileName(const STRING& FieldName, const FIELDTYPE& FieldType,
	PSTRING StringBuffer);
  bool DfdtGetFileName(const DFD& dfd, STRING *StringBuffer);

//void GetRecordData(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRING StringBuffer, const DOCTYPE *DoctypePtr = NULL);
  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRLIST StrlistBuffer, const DOCTYPE *DoctypePtr = NULL);

  bool GetFieldData (const RESULT &ResultRecord, const FC& Fc,
        STRING * StingBuffer, const DOCTYPE *DoctypePtr = NULL) ;

  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	const FIELDTYPE& FieldType, STRING *StringBuffer, const DOCTYPE *DoctypePtr = NULL);

  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	DOUBLE* Buffer);
  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        DATERANGE* Buffer);
  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        SRCH_DATE* Buffer); 

  bool GetFieldData(GPTYPE, DOUBLE*); 
  bool GetFieldData(GPTYPE, SRCH_DATE* ); 
  bool GetFieldData(GPTYPE, STRING*); 

  bool GetFieldData(const FC& FieldFC, const STRING& FieldName, STRING* Buffer);

  // Show Headline ("B")..
  bool Headline(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;
  bool Headline(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  STRING      Headline(const RESULT& ResultRecord, const STRING& RecordSyntax = NulString) const {
    STRING headline;
    Headline(ResultRecord, RecordSyntax, &headline);
    return headline;
  }

  // Context Match
  bool Context(const RESULT& ResultRecord, PSTRING Line, STRING *Term = NULL,
	const STRING& Before = NulString, const STRING& After = NulString,
	STRING *TagName = NULL) const;
  STRING      Context(const RESULT& ResultRecord,
	const STRING& Before = NulString, const STRING& After = NulString, STRING *TagName = NULL) const {
    STRING line;
    Context(ResultRecord, &line, NULL, Before, After, TagName);
    return line;
  }
  bool NthContext(size_t N, const RESULT& ResultRecord, PSTRING Line, STRING *Term = NULL,
        const STRING& Before = NulString, const STRING& After = NulString, STRING *TagName = NULL) const {
    return (&ResultRecord)->PresentNthHit(N, Line, Term, Before, After, 
	GetDocTypePtr( ResultRecord.GetDocumentType() ), TagName);
  }
  STRING      NthContext(size_t N, const RESULT& ResultRecord,
        const STRING& Before = NulString, const STRING& After = NulString,
	STRING *TagName = NULL) const {
    STRING line;
    NthContext(N, ResultRecord, &line, NULL, Before, After, NULL);
    return line;
  }

  STRING XMLHitTable(const RESULT& Result);

  bool XMLContext(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
	const STRING& Tag) const;

  // Record Summary (if available)
  bool Summary(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;
  STRING      Summary(const RESULT& ResultRecord, const STRING& RecordSyntax = NulString) const {
    STRING summary;
    Summary(ResultRecord, RecordSyntax, &summary);
    return summary;
  }

  // Return URL to Source Document
  bool URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote=false) const;
  STRING  URL (const RESULT& ResultRecord, bool OnlyRemote = false) const {
    STRING url;
    URL(ResultRecord, &url, OnlyRemote);
    return url;
  }

  // Record Highlighting
  void HighlightedRecord(const RESULT& ResultRecord,
	const STRING& BeforeTerm, const STRING& AfterTerm, PSTRING StringBuffer) const;
  STRING HighlightedRecord(const RESULT& ResultRecord, const STRING& BeforeTerm,
        const STRING& AfterTerm) const {
    STRING Tmp;
    HighlightedRecord(ResultRecord, BeforeTerm, AfterTerm, &Tmp);
    return Tmp;
  }
  void DocHighlight (const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;
  STRING DocHighlight (const RESULT& ResultRecord, const STRING& RecordSyntax = NulString) const {
    STRING Tmp;
    DocHighlight(ResultRecord, RecordSyntax, &Tmp);
    return Tmp;
  }

  // Present Element Data
  STRING Present(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax = NulString) const;
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const {
    *StringBuffer = Present(ResultRecord, ElementSet, RecordSyntax);
  }
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const {
    *StringBuffer = Present(ResultRecord, ElementSet);
  }

  STRING XmlMetaPresent(const RESULT &ResultRecord) const;

  // Present Document (containing the element, "F" for fulltext
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer);
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer, const QUERY& Query);
  STRING DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax = NulString) {
    STRING Presentation;
    DocPresent(ResultRecord, ElementSet, RecordSyntax, &Presentation);
    return Presentation;
  }

  size_t Scan(PSTRLIST ListPtr, const STRING& Field, const size_t Position =0, const INT TotalTermsRequested = -1) const;
  size_t Scan(SCANLIST *ListPtr, const STRING& Field, const size_t Position =0, const INT TotalTermsRequested = -1) const {
    return  MainIndex->Scan(ListPtr, Field, Position, TotalTermsRequested);
  }

  SCANLIST Scan(const size_t Position = 0, const STRING& Field = NulString, const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    Scan(&List, Field, Position, TotalTermsRequested);
    return List;
  }

  size_t Scan(PSTRLIST ListPtr, const STRING& Field, const STRING& Term, const INT TotalTermsRequested = -1) const;
  size_t Scan(SCANLIST *ListPtr, const STRING& Field, const STRING& Term, const INT TotalTermsRequested = -1) const{
    return  MainIndex->Scan(ListPtr, Field, Term, TotalTermsRequested);
  }
  SCANLIST Scan(const STRING& Term, const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    Scan(&List, NulString, Term, TotalTermsRequested);
    return List;
  }
  SCANLIST Scan(const STRING& Term, const STRING& Field, const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    Scan(&List, Field, Term, TotalTermsRequested);
    return List;
  }

  size_t ScanGlob(PSTRLIST ListPtr, const STRING& Field,
	const STRING& Pattern, const INT TotalTermsRequested = -1) const;
  size_t ScanGlob(SCANLIST *ListPtr, const STRING& Field,
        const STRING& Pattern, const INT TotalTermsRequested = -1) const {
    return  MainIndex->ScanGlob(ListPtr, Field, Pattern, TotalTermsRequested);
  }

  SCANLIST ScanGlob(const STRING& Pattern, const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    ScanGlob(&List, NulString, Pattern, TotalTermsRequested);
    return List;
  }
  SCANLIST ScanGlob(const STRING& Pattern, const STRING& Field, 
	const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    ScanGlob(&List, Field, Pattern, TotalTermsRequested);
    return List;
  }


 size_t ScanSoundex(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
        const size_t HashLen = 6,
        const size_t Position = 0, const INT TotalTermsRequested = -1, bool Cat = false) const {
     return MainIndex->ScanSoundex(ListPtr, Fieldname, Pattern, HashLen, Position, TotalTermsRequested, Cat);
  }

  // Scan for field contents according to a search
  size_t ScanSearch(SCANLIST *ListPtr, const QUERY& SearchQuery, const STRING& Fieldname,
         size_t MaxRecordsThreshold = 0, bool Cat = false) {
     return MainIndex->ScanSearch(ListPtr, SearchQuery, Fieldname, MaxRecordsThreshold, Cat);
  }
  SCANLIST ScanSearch(const QUERY& Query, const STRING& Fieldname, size_t MaxRecordsThreshold = 0) {
    return MainIndex->ScanSearch(Query, Fieldname, MaxRecordsThreshold);
  }

  void AddTemplate (const STRING& foo);
//void GenerateKeys();

  void SetDebugMode(bool OnOff);
  void DebugModeOn()	{ SetDebugMode(true);}; // Obsolete
  void DebugModeOff()	{ SetDebugMode(false);}; // Obsolete
  bool GetDebugMode() const { return DebugMode; };


  bool SetCacheDir(const STRING& Dir);
  STRING      GetCacheDir() const { return CacheDir; }
  bool KillCache(bool Complete=false) const;
  bool FillHeadlineCache(const STRING& RecordSyntax = NulString);

  void        SetStoplist(const STRING& Filename);
  bool IsSystemFile(const STRING& FileName);

  void   SetServerName(const STRING& ServerName);
  STRING GetServerName() const;

  bool MergeIndexFiles();
  bool CollapseIndexFiles();

  bool KillAll();
  STRING  ComposeDbFn(const CHR* Suffix) const;
  STRING& ComposeDbFn(PSTRING StringBuffer, const CHR* Suffix) const {
	return *StringBuffer = ComposeDbFn(Suffix); }
  STRING  ComposeDbFn(INT fileNumber) const;

  STRING& ComposeDbFn(PSTRING StringBuffer, INT fileNumber) const {
	return *StringBuffer = ComposeDbFn(fileNumber); }

  STRING GetDbFileStem() const            { return DbFileStem;             }
  STRING GetDbFileStem(STRING *ptr) const { return *ptr = GetDbFileStem(); }
  STRING DbName() const;
  STRING DbName(STRING *ptr) const        { return *ptr = DbName();        }
  STRING URLfrag() const;
  STRING URLfrag(STRING *ptr) const       { return *ptr = URLfrag();       }
  STRING URLfrag(const RESULT& ResultRecord) const;
  STRING URLfrag(const RESULT& ResultRecord, STRING *ptr) const {
	return *ptr = URLfrag(ResultRecord); }

  void GetDbVersionNumber(PSTRING StringBuffer) const;
  STRING GetVersionID() const;
  void GetIsearchVersionNumber(STRING *StringBuffer) const {
    if (StringBuffer) *StringBuffer = GetVersionID();
  }

  virtual bool SetSortIndexes(int Which, atomicIRSET *Irset);
  bool         BeforeSortIndex(int Which);
  bool         AfterSortIndex(int Which=0);
  SORT_INDEX_ID       GetSortIndex(int Which, INDEX_ID index_id);

  STRING Description() const;

  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default, PSTRING StringBuffer) const;

  STRING ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default=NulString) const {
    STRING value;
    ProfileGetString(Section, Entry, Default, &value);
    return value;
  }

  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const STRING& Value);

  void SetMaximumRecordSize(INT value);

  bool  AddRecord(const RECORD& NewRecord); // 1st Queue
  void         DocTypeAddRecord(const RECORD& NewRecord); // 2te Queue
  bool Index(bool newIndex = false);

  bool Index1();
  bool Index2();
 

  size_t ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const;

  void ParseRecords(RECORD& FileRecord);
  void ParseFields(RECORD *Record);
  void SelectRegions(const RECORD& Record, FCT* RegionsPtr);
  GPTYPE ParseWords(const DOCTYPE_ID& Doctype, UCHR* DataBuffer,
	GPTYPE DataLength, GPTYPE DataOffset,
	GPTYPE* GpBuffer, GPTYPE GpLength);

  void  ffGC() { MainFpt.Sync(); }
  PFILE ffopen(const STRING& FileName, const CHR* Type) {
	return MainFpt.ffopen (FileName, Type); }
  INT ffclose(PFILE FilePointer) {
	return MainFpt.ffclose (FilePointer); }
  INT ffclose(const STRING& Filename) {
	return MainFpt.ffclose(Filename);
  }
  INT ffdispose(PFILE FilePointer) {
	return MainFpt.ffdispose(FilePointer); }
  INT ffdispose(const STRING& Filename) {
        return MainFpt.ffdispose(Filename); }
  INT fflush(PFILE FilePointer) {
	return MainFpt.fflush(FilePointer); }

  bool IsStopWord (const STRING& Word) const {
	return MainIndex->IsStopWord(Word);} ;
  bool IsStopWord (const UCHR* Word, STRINGINDEX MaxLen, INT Limit=0) const {
	return MainIndex->IsStopWord(Word, MaxLen, Limit);};

  void SetDocumentInfo(const INT Index, const RECORD& Record);
  bool GetDocumentInfo(const INT Index, RECORD *RecordBuffer) const;

  size_t MdtLookupKey (const STRING& Key) const {
	return MainMdt->LookupByKey (Key);
  };
  void MdtSetUniqueKey(RECORD *NewRecord, const STRING& Key);

  bool GetDocumentDeleted(const INT Index) const;
  bool DeleteByIndex (const INT Index);
  bool DeleteByKey(const STRING& Key);
  bool UndeleteByIndex (const INT Index);
  bool UndeleteByKey(const STRING& Key);

  // Daterange code will change..
  bool SetDateRange(const DATERANGE& DateRange);
  bool SetDateRange(const SRCH_DATE& From, const SRCH_DATE& To);
  bool GetDateRange(DATERANGE *DateRange = NULL) const;

  void        SetOverride(bool Flag) { Override = Flag; }
  bool GetOverride() const           { return Override; }

  INT CleanupDb();

  void              SetGlobalDoctype(const DOCTYPE_ID& NewGlobalDoctype);
  const DOCTYPE_ID& GetGlobalDoctype();
  void              GetGlobalDocType (PSTRING StringBuffer) const;

  void SetGlobalStoplist (const STRING& NewStoplist);
  STRING GetGlobalStoplist () const;

  void   BeforeSearching (QUERY*);
  QUERY  BeforeSearching(const QUERY& Query) {
    QUERY newQuery(Query);
    BeforeSearching(&newQuery);
    return newQuery;
  }

  IRSET *AfterSearching (IRSET* ResultSetPtr);
  void BeforeIndexing ();
  void AfterIndexing ();

  void BeginRsetPresent(const STRING& RecordSyntax);
  void EndRsetPresent(const STRING& RecordSyntax);

  INT GetLocks() const;

  bool GetHTTP_server(PSTRING path) const;
  bool GetHTTP_root(PSTRING path = NULL, bool *Mirror = NULL) const;

  PDTREG  GetDocTypeReg()      { return DocTypeReg; }
  PMDT    GetMainMdt() const   { return MainMdt;    }
  PDFDT   GetMainDfdt()        { return MainDfdt;   }
  INDEX  *GetMainIndex() const { return MainIndex;  }

  size_t ImportIndex(IDBOBJ *IndexPtr) { // Returns number of records addded
    return MainIndex ? MainIndex->ImportIndex( IndexPtr ? IndexPtr->GetMainIndex() : NULL) : 0;
  }

  FCACHE     *GetFieldCache()      { return MainIndex->GetFieldCache(); }

  METADATA   *GetMetadefaults(const STRING& MdType);

  FCT         GetFieldFCT (const MDTREC& mdtrec, const STRING& FieldName);

  void        GetFieldDefinitionList(STRLIST *StrlistPtr) const;

  bool UsePersistantCache() const  { return !PersistantIrsetCache.IsEmpty(); }
  STRING PersistantCacheName() const { return PersistantIrsetCache; }

  FPT       *GetMainFpt() { return &MainFpt ; }

  bool setUseRelativePaths(bool val);
  bool setAutoDeleteExpired(bool val);

  ~IDB();

protected:
  virtual void IndexingStatus(const t_IndexingStatus StatusMessage, const STRING& Filename = NulString,
		const long WordCount = 0);
  STRLIST     DocTypeOptions;
  PMDT        MainMdt;

private:
  void Initialize (const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly = false);
  void Initialize (IDBOBJ *myParent, const STRING& DBName, const STRLIST& NewDocTypeOptions,
        bool SearchOnly = false);
  void Initialize(const STRING& NewPathName, const STRING& NewFileName,
        const STRLIST& NewDocTypeOptions, bool SearchOnly = false);
  void Initialize(IDBOBJ *myParent, const STRING& NewPathName, const STRING& NewFileName,
        const STRLIST& NewDocTypeOptions, bool SearchOnly = false);

  void        FlushMainRegistry();
  void        MergeTemplates ();
  bool CacheHeadline(GPTYPE, const STRING&, const STRING&) const;
  STRING      LookupHeadlineCache(GPTYPE, const STRING&) const;
  SRCH_DATE   DateOf(const STRING&) const;

  bool GetFieldData(const FC& FieldFC, const STRING& FieldName, DOUBLE* Buffer);
  bool GetFieldData(const FC& FieldFC, const STRING& FieldName, SRCH_DATE* Buffer);


  bool  checkFieldName(const STRING& fieldname) const {
	  return MainDfdt->checkFieldName(fieldname);
  }

  // Openend?
  bool Initialized;

  // Variables...
  DOCTYPE_ID GlobalDoctype;
  STRING      DbPathName, DbFileName;
  STRING      DbFileStem;
  STRLIST     DatabaseList;
  STRING      HTpath, HTDocumentRoot;
  bool        isMirror;
  STRLIST     TemplateTypes;
  PINDEX      MainIndex;
  PDFDT       MainDfdt;
  size_t      IndexingMemory;
  bool        DebugMode;
  bool        autoDeleteExpired; 
  PDTREG      DocTypeReg;
  FPT         MainFpt;
  size_t      TotalRecordsQueued;
  PREGISTRY   MainRegistry;
  bool        DbInfoChanged;
  bool        wrongEndian; // @@@ edz: Cache
  bool        Override;
  int         errorCode;
//Z3950_ERROR ZerrorCode;
  FILE       *SortIndexFp;
  int         ActiveSortIndex; 

  DOUBLE      PriorityFactor;

  DOUBLE      IndexBoostFactor;
  DOUBLE      FreshnessBoostFactor;
  DOUBLE      LongevityBoostFactor;
  SRCH_DATE   FreshnessBaseDateLine;


//INT         DebugSkip;
  IDBVOL      Volume; // Parent volume

  LOCALE      GlobalLocale; // Locale

  // All about the target
  STRING Title, Comments, Copyright, MaintainerName, MaintainerMail;

  STRING      SegmentName;
  int         SegmentNumber;

  // Metadata
  METADATA   *MetaDefaults;
  // Hash
  HASH        FileNames;
  // Presistant RESULT Cache
  STRING      CacheDir;
  // Persistant IRSET Cache
  STRING PersistantIrsetCache;

  bool compatible;
  long        Queue1Add;
  long        Queue2Add;

  // helpers
  size_t      lastPeerField;

  //

  INT         MaximumRecordSize;

  IDBOBJ     *Parent;  
};

typedef IDB* PIDB;

#endif
