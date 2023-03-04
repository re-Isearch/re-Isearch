/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		vidb.hxx
Description:	Class VIDB
@@@*/

#ifndef VIDB_HXX
#define VIDB_HXX

#ifndef IDB_HXX
# include "idb.hxx"
# include "gdt.h"
#endif
#if USE_STD_MAP
# include <map>
#endif

class IDB_STATS {
public:
  IDB_STATS() {
    Clear();
  }
  ~IDB_STATS() {;}
  void SetHits(size_t nHits)   { Hits = nHits;   }
  void SetTotal(size_t nTotal) { Total = nTotal; }
  size_t GetTotal() const      { return Total;   }
  size_t GetHits() const       { return Hits;    }
  void   Clear() { Hits = Total = MaxAuxCount = 0; Name.Clear();}

  void   SetName(const STRING& newName) { Name = newName; }
  STRING GetName() const { return Name; } 

  IDB_STATS& operator=(const IDB_STATS& Other) {
    Total       = Other.Total;
    Hits        = Other.Hits;
    MaxAuxCount = Other.MaxAuxCount;
    Name        = Other.Name;
    return *this;
  }

private:
  size_t  Total;
  size_t  Hits;
  size_t  MaxAuxCount;
  STRING  Name;
#if 0
  clock_t CPU; // Can track CPU utilization?
#endif
} ;

class VIDB_STATS {
public:
  VIDB_STATS() {
    Clear();
  }
  ~VIDB_STATS() {;}
  void Clear() {
    for (size_t i=0; i<= VolIndexCapacity; i++)
      Table[i].Clear();
  }
  IDB_STATS operator[](size_t n) const {
   return Table [(n <=0 || n> VolIndexCapacity) ? 0 : (size_t)n];
  }
  void SetTotal(size_t i, size_t total) {
     Table[i].SetTotal(total);
  }
  void SetHits(size_t i, size_t total) {
     Table[i].SetHits(total);
  }
  void SetName(size_t i, const STRING& Name) {
     Table[i].SetName(Name);
  }

private:
  // Since  VolIndexCapacity is 255 we don't need to bother with dynamics
  IDB_STATS Table[ VolIndexCapacity + 1];
} ;


class VIDB : public IDBOBJ {
public:
  VIDB();
  VIDB(const STRING& DBName);
  VIDB(const STRING& DBName, REGISTRY *Registry);
  VIDB(const STRING& DBName, bool Searching);
  VIDB(const STRING& DBName, const STRLIST& NewDocTypeOptions);
  VIDB(const STRING& DBName, const STRLIST& NewDocTypeOptions, const bool Searching);
  VIDB(const STRING& NewPathName, const STRING& NewFileName);
  VIDB(const STRING& NewPathName, const STRING& NewFileName, bool Searching);
  VIDB(const STRING& NewPathName, const STRING& NewFileName, const STRLIST& NewDocTypeOptions);
  VIDB(const STRING& NewPathName, const STRING& NewFileName,
	const STRLIST& NewDocTypeOptions, const bool Searching);

  bool  Open(const STRING& DBName, const STRLIST& NewDocTypeOptions, const bool Searching);
  bool  Open(const STRING& NewPathName, const STRING& NewFileName, 
		const STRLIST& NewDocTypeOptions, const bool Searching);
  bool  Open(const STRING& NewPathName, const STRING& NewFileName,
                const STRLIST& NewDocTypeOptions, const bool Searching, const STRING& XMLBuffer);
  bool  Close(); // Close a VIDB

  const STRLIST&  GetDocTypeOptions() const;
  const STRLIST  *GetDocTypeOptionsPtr() const;
  ~VIDB();

  bool  AddRecord(const RECORD& NewRecord); // 1st Queue
  void         DocTypeAddRecord(const RECORD& NewRecord); // 2nd Queue

#if USE_STD_MAP
  int         Segment(const STRING& Name)  { return Segments[Name]; }
#endif

  bool  Index(bool newIndex = false);

  SRCH_DATE   GetTimestamp() const;

  size_t      GetDbSearchCutoff(size_t idx=0) const;

  IDBOBJ     *GetIDB(size_t idx = 0) const;
  IDBOBJ     *GetIDB(const IRESULT& Result) const;
  IDBOBJ     *GetIDB(const RESULT& Result) const;

  size_t      GetIDBCount() const { return c_dbcount; }
  bool IsDbVirtual() const { return c_dbcount > 1;}

  virtual bool SetSortIndexes(int Which, atomicIRSET *Irset);

  void SetPriorityFactor(DOUBLE x, size_t Idx=0);

  void SetDbSearchCutoff(size_t m);
  void SetDbSearchCutoff(size_t m, size_t idx);
  void SetDbSearchFuel(size_t Percent);
  void SetDbSearchFuel(size_t idx, size_t Percent);

  void SetDbSearchCacheSize(size_t NewCacheSize);
  void SetDbSearchCacheSize(size_t NewCacheSize, size_t idx);

  void SetFindConcatWords(bool Set=true);
  bool GetFindConcatWords() const;

  void  BeforeSearching (QUERY*);
  QUERY BeforeSearching(const QUERY& Query) {
    QUERY newQuery(Query);
    BeforeSearching(&newQuery);
    return newQuery;
  }
  IRSET *AfterSearching (IRSET*);

  void BeforeIndexing ();
  void AfterIndexing ();

  bool KillAll();
  bool KillAll(size_t idx);

  void SetDebugMode(bool OnOff);
  void DebugModeOn()      { SetDebugMode(true);}; // Obsolete
  void DebugModeOff()     { SetDebugMode(false);}; // Obsolete

  enum DbState GetDbState() const;

  int         GetErrorCode() const;
  int         GetErrorCode(const INT Idx) const;

  const char *ErrorMessage() const;
  const char *ErrorMessage(const INT Idx) const;

  off_t GetTotalWords() const;
  off_t GetTotalWords(const INT Idx) const;
  off_t GetTotalUniqueWords() const;
  off_t GetTotalUniqueWords(const INT Idx) const;

  size_t GetTotalRecords() const;
  size_t GetTotalRecords(const INT Idx) const;
  size_t GetTotalDocumentsDeleted() const;
  size_t GetTotalDocumentsDeleted(const INT Idx) const;
  size_t GetTotalDatabases() const;
  bool SetLocale(const CHR *LocaleName = NULL) const;


  bool IsDbCompatible() const;
  bool IsEmpty() const;
  bool Ok () const { return Opened && IsDbCompatible() && !IsEmpty(); }

  int BitVersion() const;

  void SetCommonWordsThreshold(long x);

  void SetStoplist(const STRING& Filename);
  bool IsStopWord (const STRING& Word) const;

// Virtual Database Information
  STRING GetTitle(const INT Idx=0) const;
  STRING GetComments(const INT Idx=0) const;

  // Maintainer
  void   GetMaintainer(STRING *Name, STRING *Address) const {
    GetMaintainer(Name, Address, 0);
  }
  void   GetMaintainer(STRING *Name, STRING *Address, const INT) const;
  STRING GetMaintainer() const { return GetMaintainer(0); }
  STRING GetMaintainer(const INT Idx) const;

  MDT   *GetMainMdt(INT Idx = 1) const;
  INDEX *GetMainIndex(INT Idx = 1) const;
  FCACHE *GetFieldCache(INT Idx = 1);


  size_t Scan(PSTRLIST ListPtr, const STRING& Field,
	const size_t Position = 0, const INT TotalTermsRequested = -1) const;
  size_t Scan(SCANLIST *ListPtr, const STRING& Field,
	const size_t Position = 0, const INT TotalTermsRequested = -1) const;
  SCANLIST Scan(const STRING& Field, const size_t Position, const INT TotalTermsRequested) const {
    SCANLIST List;
    Scan(&List, Field, Position, TotalTermsRequested);
    return List;
  }

  size_t Scan(PSTRLIST ListPtr, const STRING& Field,
	const STRING& Term, const INT TotalTermsRequested = -1) const;
  size_t Scan(SCANLIST *ListPtr, const STRING& Field,
	const STRING& Term, const INT TotalTermsRequested = -1) const;

  SCANLIST Scan(const STRING& Term) const {
    SCANLIST List;
    Scan(&List, NulString, Term, -1);
    return List;
  }
  SCANLIST Scan(const STRING& Term, const INT TotalTermsRequested) const {
    SCANLIST List;
    Scan(&List, NulString, Term, TotalTermsRequested);
    return List;
  }
  SCANLIST Scan(const STRING& Term, const STRING& Field) const {
    SCANLIST List;
    Scan(&List, Field, Term);
    return List;
  }
  SCANLIST Scan(const STRING& Term, const STRING& Field, const INT TotalTermsRequested) const  {
    SCANLIST List;
    Scan(&List, Field, Term, TotalTermsRequested);
    return List;
  }

  /* This is a bit of a bent scan.. really a search 
     it finds the words in a field of those records that match a query
  */
  // Scan for field contents according to a search
  size_t ScanSearch(SCANLIST *ListPtr, const QUERY& SearchQuery, const STRING& Fieldname,
         size_t MaxRecordsThreshold = 0, bool Cat = false);
  SCANLIST ScanSearch(const QUERY& Query, const STRING& Fieldname) {
    return ScanSearch(Query, Fieldname, 0);
  }
  SCANLIST ScanSearch(const QUERY& Query, const STRING& Fieldname,  size_t MaxRecordsThreshold);

  size_t ScanGlob(PSTRLIST ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const INT TotalTermsRequested) const;


  SCANLIST ScanGlob(const STRING& Pattern, const INT TotalTermsRequested = -1) {
    SCANLIST List;
    Scan(&List, NulString, Pattern, TotalTermsRequested);
    return List;
  }
  SCANLIST ScanGlob(const STRING& Pattern, const STRING& Field, const INT TotalTermsRequested = -1) {
    SCANLIST List;
    Scan(&List, Field, Pattern, TotalTermsRequested);
    return List;
  }


//  size_t ScanGlob(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Term,
//	const INT TotalTermsRequested) const;
/*
  SCANLIST ScanGlob(const STRING& Field, const STRING& Pattern,
        const INT TotalTermsRequested = -1) const {
    SCANLIST List;
    ScanGlob(&List, Field, Pattern, TotalTermsRequested);
    return List;
  }
*/

  PIRSET Search(const QUERY& SearchQuery, VIDB_STATS *Stats = NULL);

  // Obsolete
  PIRSET Search(const SQUERY& SearchQuery, SortBy Sort = Unsorted,
        enum NormalizationMethods Method = defaultNormalization, VIDB_STATS *Stats = NULL);
  // Convienience methods
  PIRSET SearchWords(const STRING& Words, SortBy Sort = Unsorted,
	enum NormalizationMethods Method = defaultNormalization);
  PIRSET SearchRpn(const STRING& RpnQuery, SortBy Sort = Unsorted,
	enum NormalizationMethods Method = defaultNormalization);
  PIRSET SearchInfix(const STRING& InfixQuery, SortBy Sort = Unsorted,
	enum NormalizationMethods Method = defaultNormalization);

  PIRSET SearchSmart(QUERY *Query, const STRING& DefaultField=NulString);
  PIRSET SearchSmart(const QUERY& Query, SQUERY *SqueryPtr = NULL);
  PIRSET SearchSmart(const QUERY& Query, const STRING& DefaultField, SQUERY *SqueryPtr = NULL);
  // Obsolete
  PIRSET SearchSmart(const SQUERY& SearchQuery, SortBy Sort = Unsorted,
        enum NormalizationMethods Method = defaultNormalization, SQUERY *SqueryPtr = NULL) {
    return SearchSmart(SearchQuery, NulString, Sort, Method, SqueryPtr);
  }
  PIRSET SearchSmart(const SQUERY& Query, const STRING& DefaultField,
        SortBy Sort = Unsorted, enum NormalizationMethods Method =defaultNormalization,
        SQUERY *SqueryPtr = NULL) ;
  // Convienience
  PIRSET SearchSmart(const STRING& QueryString, const STRING& DefaultField,
        SortBy Sort = Unsorted, enum NormalizationMethods Method =defaultNormalization,
	SQUERY *SqueryPtr = NULL) ;
  PIRSET SearchSmart(const STRING& QueryString, SortBy Sort = Unsorted,
        enum NormalizationMethods Method = defaultNormalization, SQUERY *SqueryPtr = NULL) {
    return SearchSmart(QueryString, NulString, Sort, Method, SqueryPtr);
  } 

  PIRSET  FileSearch(const STRING& FileSpec);

  // Combined
  PRSET VSearch(const QUERY& Query);
  PRSET VSearchSmart(QUERY *Query, const STRING& DefaultField=NulString);
  PRSET VSearchSmart(const QUERY& Query, const STRING& DefaultField=NulString, SQUERY *Ptr = NULL);
  PRSET VSearchSmart(const QUERY& Query, SQUERY *Ptr) {
    return VSearchSmart(Query, NulString, Ptr);
  }

  // Combined (Obsolete)
  PRSET  VSearch(const SQUERY& SearchQuery, SortBy Sort = Unsorted,
	size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization) {
    QUERY Query(SearchQuery);
    Query.Sort = Sort;
    Query.Method = Method;
    Query.MaxResults = Total;
    PRSET prset = VSearch(Query);
    if (TotalFound) *TotalFound = prset ? prset->GetTotalFound() : 0;
    return prset;
  }
  PRSET  VSearchSmart(const SQUERY& Query, SortBy Sort,
	size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization, SQUERY* QueryPtr = NULL) {
    return VSearchSmart(Query, NulString, Sort, Total, TotalFound, Method, QueryPtr);
  }
  PRSET  VSearchSmart(const SQUERY& Query, const STRING& DefaultField = NulString,
        SortBy Sort = Unsorted, size_t Total = 300, size_t *TotalFound = NULL,
        enum NormalizationMethods Method = defaultNormalization, SQUERY* QueryPtr = NULL);

  // Convienience methods
  PRSET  VSearchWords(const STRING& Words, SortBy Sort = Unsorted,
	size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization);
  PRSET  VSearchRpn(const STRING& RpnQuery, SortBy Sort = Unsorted,
        size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization);
  PRSET  VSearchInfix(const STRING& InfixQuery, SortBy Sort = Unsorted,
        size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization);
  PRSET  VSearchSmart(const STRING& Query, const STRING& DefaultField = NulString,
	SortBy Sort = Unsorted, size_t Total = 300, size_t *TotalFound = NULL,
	enum NormalizationMethods Method = defaultNormalization, SQUERY* QueryPtr = NULL);

  void BeginRsetPresent(const STRING& RecordSyntax);

  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRING StringBuffer, const DOCTYPE *DoctypePtr = NULL);
  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRLIST StrlistBuffer, const DOCTYPE *DoctypePtr = NULL);

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
	const STRING& Before = NulString, const STRING& After = NulString) const;
  STRING      Context(const RESULT& ResultRecord,
	const STRING& Before = NulString, const STRING& After = NulString) const {
    STRING line;
    Context(ResultRecord, &line, NULL, Before, After);
    return line;
  }
  bool NthContext(size_t N, const RESULT& ResultRecord, PSTRING Line, STRING *Term = NULL,
        const STRING& Before = NulString, const STRING& After = NulString) const;
  STRING      NthContext(size_t N, const RESULT& ResultRecord,
        const STRING& Before = NulString, const STRING& After = NulString) const {
    STRING line;
    NthContext(N, ResultRecord, &line, NULL, Before, After);
    return line;
  }

  STRING XMLHitTable(const RESULT& Result);

  bool XMLContext(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
        const STRING& Tag) const;

  // Record Summary (if available)
  bool Summary(const RESULT& ResultRecord,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  STRING      Summary(const RESULT& ResultRecord, const STRING& RecordSyntax = NulString) const {
    STRING summary;
    Summary(ResultRecord, RecordSyntax, &summary);
    return summary;
  }

  // Return URL to Source Document
  bool URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote = false) const;

  STRING  URL (const RESULT& ResultRecord, bool OnlyRemote = false) const {
    STRING url;
    URL(ResultRecord, &url, OnlyRemote);
    return url;
  }

  STRING XMLNodeTree (const RESULT& ResultRecord, const FC& Fc) const;

  // Record Highlighting

  STRING GetXMLHighlightRecordFormat(const RESULT& Result, const STRING& PageField = NulString,
        const STRING& TagElement = NulString);

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
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const;
  STRING Present(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax = NulString) const {
    STRING Presentation;
    Present(ResultRecord, ElementSet, RecordSyntax, &Presentation);
    return Presentation;
  }


  // Present Document (containing the element, "F" for fulltext
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer,
	const QUERY& Query) const;
  STRING DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax = NulString) const {
    STRING Presentation;
    DocPresent(ResultRecord, ElementSet, RecordSyntax, &Presentation);
    return Presentation;
  }


  void EndRsetPresent(const STRING& RecordSyntax);

  void GetGlobalDocType(PSTRING StringBuffer) const;
  const STRING& GetGlobalDocType() const { return GlobalDocType; }

  DFDT *GetDfdt() { return GetDfdt(NULL, NULL); }
  DFDT *GetDfdt(DFDT *DfdtBuffer, const RESULT *ResultPtr = NULL);

  bool GetRecordDfdt(const RESULT& Result, DFDT *DfdtBuffer);
  bool GetRecordDfdt(const STRING& Key, DFDT *DfdtBuffer);
  bool KeyLookup(const STRING& Key, PRESULT ResultBuffer = NULL) const;

  bool SetDateRange(const DATERANGE& DateRange);
  bool SetDateRange(const SRCH_DATE& From, const SRCH_DATE& To);
  bool GetDateRange(DATERANGE *DateRange = NULL) const;

  STRING Description() const;

  void ProfileGetString(const STRING&, const STRING&,
	const STRING&, PSTRING) const;

  STRING ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default=NulString) const {
    STRING value;
    ProfileGetString(Section, Entry, Default, &value);
    return value;
  }


  PDOCTYPE GetDocTypePtr(const DOCTYPE_ID& DocType) const;

  STRING FirstKey() const;
  STRING LastKey() const;
  STRING NextKey(const STRING& Key) const;
  STRING PrevKey(const STRING& Key) const;

  size_t GetAncestorContent (RESULT& Result, const STRING& NodeName, STRLIST *StrlistPtr);

  // idx is the index into the list starting at 1
  FCT    GetDescendentsFCT (const FC& HitFc, const STRING& NodeName, size_t idx = 0) {
    if (idx < 1 && c_dbcount == 1) idx = 1;
    if (idx > 0 && idx <= c_dbcount)
      return  c_dblist[idx-1]->GetDescendentsFCT(HitFc, NodeName);
    return FCT();
  }
  FC     GetAncestorFc (const FC& HitFc, const STRING& NodeName, size_t idx=0) {
    if (idx < 1 && c_dbcount == 1) idx = 1;
    if (idx > 0 && idx <= c_dbcount)
      return  c_dblist[idx-1]->GetAncestorFc(HitFc, NodeName);
    return FC();
  }

  bool GetDocumentInfo (const INT Idx, const INT Index, PRECORD RecordBuffer) const;
  bool GetDocumentInfo (const INT Index, PRECORD RecordBuffer) const;

  SRCH_DATE DateCreated() const;
  SRCH_DATE DateLastModified() const;

// Since Virtual Get stem
  STRING GetDbFileStem() const;
  STRING GetDbFileStem(const INT Idx) const;
  STRING GetDbFileStem(const INT Idx, PSTRING StringBuffer) const;
  STRING GetDbFileStem(const RESULT& ResultRecord) const;
  STRING GetDbFileStem(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  STRING GetDbFileStem(const STRING& Key) const;
  STRING GetDbFileStem(const STRING& Key, PSTRING StringBuffer) const;

  const STRLIST& GetAllDocTypes ();
  bool ValidateDocType(const STRING& DocType) const;

  STRING GetVersionID() const;
  void   GetIsearchVersionNumber(PSTRING StringBuffer) const {
    if (StringBuffer) *StringBuffer = GetVersionID();
  };

  INT GetLocks() const;
private:
  bool     Opened;
  size_t          VirtualSet(const STRING& Key, STRING *NewKey, char Ch='@') const;
  size_t          VirtualSet(const RESULT& ResultRecord, RESULT *Result) const;
  PDFDT           MainDfdt;
  PREGISTRY       MainRegistry;
  STRING          GlobalDocType;
  STRING          DbTitle, DbComments;
  STRING          DbMaintainerName, DbMaintainerMail;
  STRING          DbPathName, DbFileName;
  STRING          StoplistFileName;
  IDB           **c_dblist;
  IRSET         **c_irsetlist;
  RSET          **c_rsetlist;
  size_t          c_dbcount;
  bool     c_inconsistent_doctypes;
  BYTE            c_index[VolIndexCapacity]; // See above 

  struct strCmp {
    int operator()( const STRING& s1, const STRING& s2 ) const
      { return s1 <= s2; }
  };
#if USE_STD_MAP
  std::map<const STRING, int, strCmp> Segments;
#endif
};


class TEMP_VIDB {
public:
  TEMP_VIDB (const STRLIST DbList, const STRLIST& Options = NulStrlist);
  operator VIDB *()  const { return vidbPtr; }
  operator STRING () const { return Fn;      }
  ~TEMP_VIDB();
private:
  VIDB   *vidbPtr;
  STRING  Fn;
};
  

class SearchSession {
public:
  SearchSession(pid_t SaveProcessId);
  SearchSession(const STRING& SavedSearchSessionFn);
  SearchSession(const QUERY& Query);

  // Load Old Sessions
  IRSET      *Load(const STRING& SavedSearchSessionFn);
  IRSET      *Load(pid_t ProcessId);
  // Create a New Session
  IRSET      *Create(const QUERY& Query);
  // Save this Session
  bool Save(const STRING& SearchSessionFn) const;
  bool Save(pid_t ProcessId) const;

  // Session Information
  QUERY       GetQuery() const { return Query; }

  // Destroy this
  ~SearchSession();
private:
  STRING     DBName;
  IRSET     *Prset;
  VIDB      *Pdb;
  QUERY      Query;        // Full Search query
};


typedef VIDB* PVIDB;

#endif


