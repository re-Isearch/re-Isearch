/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class IDB extends IDBOBJ {
  private long swigCPtr;

  protected IDB(long cPtr, boolean cMemoryOwn) {
    super(IBJNI.SWIGIDBUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(IDB obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      IBJNI.delete_IDB(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  public IDB(String DBFullPath, boolean SearchOnly) {
    this(IBJNI.new_IDB__SWIG_0(DBFullPath, SearchOnly), true);
  }

  public IDB(String DBFullPath) {
    this(IBJNI.new_IDB__SWIG_1(DBFullPath), true);
  }

  public boolean Open(String DBName, boolean SearchOnly) {
    return IBJNI.IDB_Open__SWIG_0(swigCPtr, this, DBName, SearchOnly);
  }

  public boolean Open(String DBName) {
    return IBJNI.IDB_Open__SWIG_1(swigCPtr, this, DBName);
  }

  public boolean Close() {
    return IBJNI.IDB_Close(swigCPtr, this);
  }

  public void SetDebugMode(boolean OnOff) {
    IBJNI.IDB_SetDebugMode(swigCPtr, this, OnOff);
  }

  public String FirstKey() {
    return IBJNI.IDB_FirstKey(swigCPtr, this);
  }

  public String LastKey() {
    return IBJNI.IDB_LastKey(swigCPtr, this);
  }

  public String NextKey(String Key) {
    return IBJNI.IDB_NextKey(swigCPtr, this, Key);
  }

  public String PrevKey(String Key) {
    return IBJNI.IDB_PrevKey(swigCPtr, this, Key);
  }

  public void SetVolume(String Name, int Vol) {
    IBJNI.IDB_SetVolume(swigCPtr, this, Name, Vol);
  }

  public SWIGTYPE_p_INT GetVolume() {
    return new SWIGTYPE_p_INT(IBJNI.IDB_GetVolume(swigCPtr, this), true);
  }

  public void SetFindConcatWords(boolean Set) {
    IBJNI.IDB_SetFindConcatWords__SWIG_0(swigCPtr, this, Set);
  }

  public void SetFindConcatWords() {
    IBJNI.IDB_SetFindConcatWords__SWIG_1(swigCPtr, this);
  }

  public SWIGTYPE_p_GDT_BOOLEAN GetFindConcatWords() {
    return new SWIGTYPE_p_GDT_BOOLEAN(IBJNI.IDB_GetFindConcatWords(swigCPtr, this), true);
  }

  public void SetSegment(String newName, int newNumber) {
    IBJNI.IDB_SetSegment__SWIG_0(swigCPtr, this, newName, newNumber);
  }

  public void SetSegment(String newName) {
    IBJNI.IDB_SetSegment__SWIG_1(swigCPtr, this, newName);
  }

  public void SetSegment(int newNumber) {
    IBJNI.IDB_SetSegment__SWIG_2(swigCPtr, this, newNumber);
  }

  public String GetSegmentName() {
    return IBJNI.IDB_GetSegmentName(swigCPtr, this);
  }

  public int Segment(String Name) {
    return IBJNI.IDB_Segment(swigCPtr, this, Name);
  }

  public boolean setUseRelativePaths(boolean val) {
    return IBJNI.IDB_setUseRelativePaths(swigCPtr, this, val);
  }

  public void SetWorkingDirectory(String Dir) {
    IBJNI.IDB_SetWorkingDirectory(swigCPtr, this, Dir);
  }

  public void ClearWorkingDirectoryEntry() {
    IBJNI.IDB_ClearWorkingDirectoryEntry(swigCPtr, this);
  }

  public int SetErrorCode(int Error) {
    return IBJNI.IDB_SetErrorCode(swigCPtr, this, Error);
  }

  public int GetErrorCode() {
    return IBJNI.IDB_GetErrorCode(swigCPtr, this);
  }

  public String ErrorMessage() {
    return IBJNI.IDB_ErrorMessage__SWIG_0(swigCPtr, this);
  }

  public String ErrorMessage(int ErrorCode) {
    return IBJNI.IDB_ErrorMessage__SWIG_1(swigCPtr, this, ErrorCode);
  }

  public boolean FieldExists(String FieldName) {
    return IBJNI.IDB_FieldExists(swigCPtr, this, FieldName);
  }

  public STRLIST GetFieldDefinitionList() {
    long cPtr = IBJNI.IDB_GetFieldDefinitionList(swigCPtr, this);
    return (cPtr == 0) ? null : new STRLIST(cPtr, true);
  }

  public RECORD GetDocumentInfo(int Index) {
    long cPtr = IBJNI.IDB_GetDocumentInfo(swigCPtr, this, Index);
    return (cPtr == 0) ? null : new RECORD(cPtr, true);
  }

  public STRLIST GetAllDocTypes() {
    return new STRLIST(IBJNI.IDB_GetAllDocTypes(swigCPtr, this), true);
  }

  public MDT GetMainMdt() {
    long cPtr = IBJNI.IDB_GetMainMdt(swigCPtr, this);
    return (cPtr == 0) ? null : new MDT(cPtr, false);
  }

  public SRCH_DATE DateCreated() {
    return new SRCH_DATE(IBJNI.IDB_DateCreated(swigCPtr, this), true);
  }

  public SRCH_DATE DateLastModified() {
    return new SRCH_DATE(IBJNI.IDB_DateLastModified(swigCPtr, this), true);
  }

  public void SetCommonWordsThreshold(int x) {
    IBJNI.IDB_SetCommonWordsThreshold(swigCPtr, this, x);
  }

  public boolean CreateCentroid() {
    return IBJNI.IDB_CreateCentroid(swigCPtr, this);
  }

  public boolean SetLocale(String LocaleName) {
    return IBJNI.IDB_SetLocale__SWIG_0(swigCPtr, this, LocaleName);
  }

  public boolean SetLocale() {
    return IBJNI.IDB_SetLocale__SWIG_1(swigCPtr, this);
  }

  public boolean IsDbCompatible() {
    return IBJNI.IDB_IsDbCompatible(swigCPtr, this);
  }

  public boolean IsEmpty() {
    return IBJNI.IDB_IsEmpty(swigCPtr, this);
  }

  public boolean Ok() {
    return IBJNI.IDB_Ok(swigCPtr, this);
  }

  public void ffGC() {
    IBJNI.IDB_ffGC(swigCPtr, this);
  }

  public void SetMergeStatus(MergeStatus MergeStatus) {
    IBJNI.IDB_SetMergeStatus(swigCPtr, this, MergeStatus.swigValue());
  }

  public void SetDbState(int DbState) {
    IBJNI.IDB_SetDbState(swigCPtr, this, DbState);
  }

  public int GetDbState() {
    return IBJNI.IDB_GetDbState(swigCPtr, this);
  }

  public void SetIndexBoostFactor(double x) {
    IBJNI.IDB_SetIndexBoostFactor(swigCPtr, this, x);
  }

  public double GetIndexBoostFactor() {
    return IBJNI.IDB_GetIndexBoostFactor(swigCPtr, this);
  }

  public void SetFreshnessBoostFactor(double x) {
    IBJNI.IDB_SetFreshnessBoostFactor(swigCPtr, this, x);
  }

  public double GetFreshnessBoostFactor() {
    return IBJNI.IDB_GetFreshnessBoostFactor(swigCPtr, this);
  }

  public void SetLongevityBoostFattor(double x) {
    IBJNI.IDB_SetLongevityBoostFattor(swigCPtr, this, x);
  }

  public double GetLongevityBoostFactor() {
    return IBJNI.IDB_GetLongevityBoostFactor(swigCPtr, this);
  }

  public void SetFreshnessBaseDateLine(SRCH_DATE d) {
    IBJNI.IDB_SetFreshnessBaseDateLine(swigCPtr, this, SRCH_DATE.getCPtr(d), d);
  }

  public SRCH_DATE GetFreshnessBaseDateLine() {
    return new SRCH_DATE(IBJNI.IDB_GetFreshnessBaseDateLine(swigCPtr, this), true);
  }

  public void SetDefaultDbSearchCutoff(long x) {
    IBJNI.IDB_SetDefaultDbSearchCutoff(swigCPtr, this, x);
  }

  public void SetDbSearchCutoff(long m) {
    IBJNI.IDB_SetDbSearchCutoff(swigCPtr, this, m);
  }

  public long GetDbSearchCutoff() {
    return IBJNI.IDB_GetDbSearchCutoff(swigCPtr, this);
  }

  public void SetDbSearchFuel(long Percent) {
    IBJNI.IDB_SetDbSearchFuel(swigCPtr, this, Percent);
  }

  public void SetDbSearchCacheSize(long NewCacheSize) {
    IBJNI.IDB_SetDbSearchCacheSize(swigCPtr, this, NewCacheSize);
  }

  public void SetDefaultPriorityFactor(double x) {
    IBJNI.IDB_SetDefaultPriorityFactor(swigCPtr, this, x);
  }

  public void SetPriorityFactor(double x) {
    IBJNI.IDB_SetPriorityFactor(swigCPtr, this, x);
  }

  public double GetPriorityFactor() {
    return IBJNI.IDB_GetPriorityFactor(swigCPtr, this);
  }

  public void SetDbSisLimit(long m) {
    IBJNI.IDB_SetDbSisLimit(swigCPtr, this, m);
  }

  public void SetTitle(String NewTitle) {
    IBJNI.IDB_SetTitle(swigCPtr, this, NewTitle);
  }

  public String GetTitle() {
    return IBJNI.IDB_GetTitle(swigCPtr, this);
  }

  public void SetComments(String NewComments) {
    IBJNI.IDB_SetComments(swigCPtr, this, NewComments);
  }

  public String GetComments() {
    return IBJNI.IDB_GetComments(swigCPtr, this);
  }

  public void SetCopyright(String NewCopyright) {
    IBJNI.IDB_SetCopyright(swigCPtr, this, NewCopyright);
  }

  public String GetCopyright() {
    return IBJNI.IDB_GetCopyright(swigCPtr, this);
  }

  public void SetMaintainer(String NewName, String NewAddress) {
    IBJNI.IDB_SetMaintainer(swigCPtr, this, NewName, NewAddress);
  }

  public String GetMaintainer() {
    return IBJNI.IDB_GetMaintainer(swigCPtr, this);
  }

  public void SetGlobalDoctype(DOCTYPE_ID NewGlobalDoctype) {
    IBJNI.IDB_SetGlobalDoctype(swigCPtr, this, DOCTYPE_ID.getCPtr(NewGlobalDoctype), NewGlobalDoctype);
  }

  public void SetIndexingMemory(int MemorySize, boolean Force) {
    IBJNI.IDB_SetIndexingMemory__SWIG_0(swigCPtr, this, MemorySize, Force);
  }

  public void SetIndexingMemory(int MemorySize) {
    IBJNI.IDB_SetIndexingMemory__SWIG_1(swigCPtr, this, MemorySize);
  }

  public int GetIndexingMemory() {
    return IBJNI.IDB_GetIndexingMemory(swigCPtr, this);
  }

  public void SetStoplist(String Filename) {
    IBJNI.IDB_SetStoplist(swigCPtr, this, Filename);
  }

  public void SetGlobalStoplist(String NewStoplist) {
    IBJNI.IDB_SetGlobalStoplist(swigCPtr, this, NewStoplist);
  }

  public String GetGlobalStoplist() {
    return IBJNI.IDB_GetGlobalStoplist(swigCPtr, this);
  }

  public int GetTotalWords() {
    return IBJNI.IDB_GetTotalWords(swigCPtr, this);
  }

  public int GetTotalUniqueWords() {
    return IBJNI.IDB_GetTotalUniqueWords(swigCPtr, this);
  }

  public long GetTotalRecords() {
    return IBJNI.IDB_GetTotalRecords(swigCPtr, this);
  }

  public long GetTotalDocumentsDeleted() {
    return IBJNI.IDB_GetTotalDocumentsDeleted(swigCPtr, this);
  }

  public FCACHE GetFieldCache() {
    long cPtr = IBJNI.IDB_GetFieldCache(swigCPtr, this);
    return (cPtr == 0) ? null : new FCACHE(cPtr, false);
  }

  public FC GetPeerFc(java.math.BigInteger HitGp, SWIGTYPE_p_STRING INOUT) {
    return new FC(IBJNI.IDB_GetPeerFc__SWIG_0(swigCPtr, this, HitGp, SWIGTYPE_p_STRING.getCPtr(INOUT)), true);
  }

  public FC GetPeerFc(java.math.BigInteger HitGp) {
    return new FC(IBJNI.IDB_GetPeerFc__SWIG_1(swigCPtr, this, HitGp), true);
  }

  public FC GetPeerFc(FC HitFc, SWIGTYPE_p_STRING INOUT) {
    return new FC(IBJNI.IDB_GetPeerFc__SWIG_2(swigCPtr, this, FC.getCPtr(HitFc), HitFc, SWIGTYPE_p_STRING.getCPtr(INOUT)), true);
  }

  public FC GetPeerFc(FC HitFc) {
    return new FC(IBJNI.IDB_GetPeerFc__SWIG_3(swigCPtr, this, FC.getCPtr(HitFc), HitFc), true);
  }

  public String GetFieldName(java.math.BigInteger HitGp) {
    return IBJNI.IDB_GetFieldName__SWIG_0(swigCPtr, this, HitGp);
  }

  public String GetFieldName(FC HitFc) {
    return IBJNI.IDB_GetFieldName__SWIG_1(swigCPtr, this, FC.getCPtr(HitFc), HitFc);
  }

  public TREENODE GetPeerNode(java.math.BigInteger HitGp) {
    return new TREENODE(IBJNI.IDB_GetPeerNode__SWIG_0(swigCPtr, this, HitGp), true);
  }

  public TREENODE GetPeerNode(FC HitFc) {
    return new TREENODE(IBJNI.IDB_GetPeerNode__SWIG_1(swigCPtr, this, FC.getCPtr(HitFc), HitFc), true);
  }

  public String GetPeerContent(FC HitFc) {
    return IBJNI.IDB_GetPeerContent(swigCPtr, this, FC.getCPtr(HitFc), HitFc);
  }

  public String GetPeerContentXMLFragement(FC HitFc) {
    return IBJNI.IDB_GetPeerContentXMLFragement(swigCPtr, this, FC.getCPtr(HitFc), HitFc);
  }

  public NODETREE GetNodeTree(java.math.BigInteger HitGp) {
    return new NODETREE(IBJNI.IDB_GetNodeTree__SWIG_0(swigCPtr, this, HitGp), true);
  }

  public NODETREE GetNodeTree(FC HitFc) {
    return new NODETREE(IBJNI.IDB_GetNodeTree__SWIG_1(swigCPtr, this, FC.getCPtr(HitFc), HitFc), true);
  }

  public boolean KillCache() {
    return IBJNI.IDB_KillCache(swigCPtr, this);
  }

  public boolean FillHeadlineCache() {
    return IBJNI.IDB_FillHeadlineCache__SWIG_0(swigCPtr, this);
  }

  public boolean FillHeadlineCache(String RecordSyntax) {
    return IBJNI.IDB_FillHeadlineCache__SWIG_1(swigCPtr, this, RecordSyntax);
  }

  public boolean IsSystemFile(String FileName) {
    return IBJNI.IDB_IsSystemFile(swigCPtr, this, FileName);
  }

  public void SetServerName(String ServerName) {
    IBJNI.IDB_SetServerName(swigCPtr, this, ServerName);
  }

  public String GetServerName() {
    return IBJNI.IDB_GetServerName(swigCPtr, this);
  }

  public boolean MergeIndexFiles() {
    return IBJNI.IDB_MergeIndexFiles(swigCPtr, this);
  }

  public boolean CollapseIndexFiles() {
    return IBJNI.IDB_CollapseIndexFiles(swigCPtr, this);
  }

  public long DeleteExpired() {
    return IBJNI.IDB_DeleteExpired__SWIG_0(swigCPtr, this);
  }

  public long DeleteExpired(SRCH_DATE Now) {
    return IBJNI.IDB_DeleteExpired__SWIG_1(swigCPtr, this, SRCH_DATE.getCPtr(Now), Now);
  }

  public boolean KillAll() {
    return IBJNI.IDB_KillAll(swigCPtr, this);
  }

  public String GetVersionID() {
    return IBJNI.IDB_GetVersionID(swigCPtr, this);
  }

  public void ParseRecords(RECORD NewRecord) {
    IBJNI.IDB_ParseRecords(swigCPtr, this, RECORD.getCPtr(NewRecord), NewRecord);
  }

  public boolean AddRecord(RECORD NewRecord) {
    return IBJNI.IDB_AddRecord__SWIG_0(swigCPtr, this, RECORD.getCPtr(NewRecord), NewRecord);
  }

  public boolean Index(boolean newIndex) {
    return IBJNI.IDB_Index__SWIG_0(swigCPtr, this, newIndex);
  }

  public boolean Index() {
    return IBJNI.IDB_Index__SWIG_1(swigCPtr, this);
  }

  public boolean Index1() {
    return IBJNI.IDB_Index1(swigCPtr, this);
  }

  public boolean Index2() {
    return IBJNI.IDB_Index2(swigCPtr, this);
  }

  public boolean AddRecord(String Filename) {
    return IBJNI.IDB_AddRecord__SWIG_1(swigCPtr, this, Filename);
  }

  public boolean AppendToIndex(RECORD Record) {
    return IBJNI.IDB_AppendToIndex(swigCPtr, this, RECORD.getCPtr(Record), Record);
  }

  public boolean AppendFileToIndex(String Filename) {
    return IBJNI.IDB_AppendFileToIndex(swigCPtr, this, Filename);
  }

  public boolean IsStopWord(String Word) {
    return IBJNI.IDB_IsStopWord(swigCPtr, this, Word);
  }

  public long MdtLookupKey(String Key) {
    return IBJNI.IDB_MdtLookupKey(swigCPtr, this, Key);
  }

  public boolean GetDocumentDeleted(int Index) {
    return IBJNI.IDB_GetDocumentDeleted(swigCPtr, this, Index);
  }

  public boolean DeleteByIndex(int Index) {
    return IBJNI.IDB_DeleteByIndex(swigCPtr, this, Index);
  }

  public boolean DeleteByKey(String Key) {
    return IBJNI.IDB_DeleteByKey(swigCPtr, this, Key);
  }

  public boolean UndeleteByIndex(int Index) {
    return IBJNI.IDB_UndeleteByIndex(swigCPtr, this, Index);
  }

  public boolean UndeleteByKey(String Key) {
    return IBJNI.IDB_UndeleteByKey(swigCPtr, this, Key);
  }

  public void SetOverride(boolean Flag) {
    IBJNI.IDB_SetOverride(swigCPtr, this, Flag);
  }

  public boolean GetOverride() {
    return IBJNI.IDB_GetOverride(swigCPtr, this);
  }

  public int CleanupDb() {
    return IBJNI.IDB_CleanupDb(swigCPtr, this);
  }

  public int GetLocks() {
    return IBJNI.IDB_GetLocks(swigCPtr, this);
  }

  public SCANLIST Scan(String Term, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.IDB_Scan__SWIG_0(swigCPtr, this, Term, TotalTermsRequested), true);
  }

  public SCANLIST Scan(String Term, String Field, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.IDB_Scan__SWIG_1(swigCPtr, this, Term, Field, TotalTermsRequested), true);
  }

  public SCANLIST Scan(String Term, String Field) {
    return new SCANLIST(IBJNI.IDB_Scan__SWIG_2(swigCPtr, this, Term, Field), true);
  }

  public SCANLIST Scan(String Term) {
    return new SCANLIST(IBJNI.IDB_Scan__SWIG_3(swigCPtr, this, Term), true);
  }

  public SCANLIST ScanGlob(String Pattern) {
    return new SCANLIST(IBJNI.IDB_ScanGlob__SWIG_0(swigCPtr, this, Pattern), true);
  }

  public SCANLIST ScanGlob(String Pattern, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.IDB_ScanGlob__SWIG_1(swigCPtr, this, Pattern, TotalTermsRequested), true);
  }

  public SCANLIST ScanGlob(String Pattern, String Field) {
    return new SCANLIST(IBJNI.IDB_ScanGlob__SWIG_2(swigCPtr, this, Pattern, Field), true);
  }

  public SCANLIST ScanGlob(String Pattern, String Field, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.IDB_ScanGlob__SWIG_3(swigCPtr, this, Pattern, Field, TotalTermsRequested), true);
  }

  public SCANLIST ScanSearch(QUERY Query, String Fieldname, long MaxRecordsThreshold) {
    return new SCANLIST(IBJNI.IDB_ScanSearch__SWIG_0(swigCPtr, this, QUERY.getCPtr(Query), Query, Fieldname, MaxRecordsThreshold), true);
  }

  public SCANLIST ScanSearch(QUERY Query, String Fieldname) {
    return new SCANLIST(IBJNI.IDB_ScanSearch__SWIG_1(swigCPtr, this, QUERY.getCPtr(Query), Query, Fieldname), true);
  }

  public void BeginRsetPresent(String RecordSyntax) {
    IBJNI.IDB_BeginRsetPresent(swigCPtr, this, RecordSyntax);
  }

  public void EndRsetPresent(String RecordSyntax) {
    IBJNI.IDB_EndRsetPresent(swigCPtr, this, RecordSyntax);
  }

  public void BeforeSearching(QUERY arg0) {
    IBJNI.IDB_BeforeSearching(swigCPtr, this, QUERY.getCPtr(arg0), arg0);
  }

  public IRSET AfterSearching(IRSET ResultSetPtr) {
    long cPtr = IBJNI.IDB_AfterSearching(swigCPtr, this, IRSET.getCPtr(ResultSetPtr), ResultSetPtr);
    return (cPtr == 0) ? null : new IRSET(cPtr, false);
  }

  public void BeforeIndexing() {
    IBJNI.IDB_BeforeIndexing(swigCPtr, this);
  }

  public void AfterIndexing() {
    IBJNI.IDB_AfterIndexing(swigCPtr, this);
  }

  public IRSET Search(QUERY SearchQuery) {
    long cPtr = IBJNI.IDB_Search__SWIG_0(swigCPtr, this, QUERY.getCPtr(SearchQuery), SearchQuery);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET Search(SQUERY SearchQuery, SortBy SortBy, NormalizationMethods arg2) {
    long cPtr = IBJNI.IDB_Search__SWIG_1(swigCPtr, this, SQUERY.getCPtr(SearchQuery), SearchQuery, SortBy.swigValue(), arg2.swigValue());
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET Search(SQUERY SearchQuery, SortBy SortBy) {
    long cPtr = IBJNI.IDB_Search__SWIG_2(swigCPtr, this, SQUERY.getCPtr(SearchQuery), SearchQuery, SortBy.swigValue());
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET Search(SQUERY SearchQuery) {
    long cPtr = IBJNI.IDB_Search__SWIG_3(swigCPtr, this, SQUERY.getCPtr(SearchQuery), SearchQuery);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET SearchSmart(QUERY INOUT, String DefaultField) {
    long cPtr = IBJNI.IDB_SearchSmart__SWIG_0(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT, DefaultField);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET SearchSmart(QUERY INOUT) {
    long cPtr = IBJNI.IDB_SearchSmart__SWIG_1(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public RSET VSearch(QUERY SearchQuery) {
    long cPtr = IBJNI.IDB_VSearch(swigCPtr, this, QUERY.getCPtr(SearchQuery), SearchQuery);
    return (cPtr == 0) ? null : new RSET(cPtr, true);
  }

  public RSET VSearchSmart(QUERY INOUT, String DefaultFIeld) {
    long cPtr = IBJNI.IDB_VSearchSmart__SWIG_0(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT, DefaultFIeld);
    return (cPtr == 0) ? null : new RSET(cPtr, false);
  }

  public RSET VSearchSmart(QUERY INOUT) {
    long cPtr = IBJNI.IDB_VSearchSmart__SWIG_1(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT);
    return (cPtr == 0) ? null : new RSET(cPtr, false);
  }

  public String Headline(RESULT ResultRecord) {
    return IBJNI.IDB_Headline__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String Headline(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.IDB_Headline__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public String Summary(RESULT ResultRecord) {
    return IBJNI.IDB_Summary__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String Summary(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.IDB_Summary__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public String Context(RESULT ResultRecord, String Before, String After) {
    return IBJNI.IDB_Context__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, Before, After);
  }

  public String Context(RESULT ResultRecord, String Before) {
    return IBJNI.IDB_Context__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, Before);
  }

  public String Context(RESULT ResultRecord) {
    return IBJNI.IDB_Context__SWIG_2(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String NthContext(long N, RESULT ResultRecord, String Before, String After) {
    return IBJNI.IDB_NthContext__SWIG_0(swigCPtr, this, N, RESULT.getCPtr(ResultRecord), ResultRecord, Before, After);
  }

  public String NthContext(long N, RESULT ResultRecord, String Before) {
    return IBJNI.IDB_NthContext__SWIG_1(swigCPtr, this, N, RESULT.getCPtr(ResultRecord), ResultRecord, Before);
  }

  public String NthContext(long N, RESULT ResultRecord) {
    return IBJNI.IDB_NthContext__SWIG_2(swigCPtr, this, N, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String URL(RESULT ResultRecord, boolean OnlyRemote) {
    return IBJNI.IDB_URL__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, OnlyRemote);
  }

  public String URL(RESULT ResultRecord) {
    return IBJNI.IDB_URL__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String HighlightedRecord(RESULT ResultRecord, String BeforeTerm, String AfterTerm) {
    return IBJNI.IDB_HighlightedRecord(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, BeforeTerm, AfterTerm);
  }

  public String DocHighlight(RESULT ResultRecord) {
    return IBJNI.IDB_DocHighlight__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String DocHighlight(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.IDB_DocHighlight__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public STRLIST GetFieldData(RESULT ResultPtr, String ESet, DOCTYPE_ID Doctype) {
    long cPtr = IBJNI.IDB_GetFieldData__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet, DOCTYPE_ID.getCPtr(Doctype), Doctype);
    return (cPtr == 0) ? null : new STRLIST(cPtr, false);
  }

  public STRLIST GetFieldData(RESULT ResultPtr, String ESet) {
    long cPtr = IBJNI.IDB_GetFieldData__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet);
    return (cPtr == 0) ? null : new STRLIST(cPtr, false);
  }

  public ArraySTRING GetFieldContents(RESULT ResultPtr, String ESet) {
    return new ArraySTRING(IBJNI.IDB_GetFieldContents(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet), true);
  }

  public String Present(RESULT ResultRecord, String ElementSet) {
    return IBJNI.IDB_Present__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet);
  }

  public String Present(RESULT ResultRecord, String ElementSet, String RecordSyntax) {
    return IBJNI.IDB_Present__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet, RecordSyntax);
  }

  public String DocPresent(RESULT ResultRecord, String ElementSet) {
    return IBJNI.IDB_DocPresent__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet);
  }

  public String DocPresent(RESULT ResultRecord, String ElementSet, String RecordSyntax) {
    return IBJNI.IDB_DocPresent__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet, RecordSyntax);
  }

  public String GetXMLHighlightRecordFormat(RESULT Result, String PageField, String TagElement) {
    return IBJNI.IDB_GetXMLHighlightRecordFormat__SWIG_0(swigCPtr, this, RESULT.getCPtr(Result), Result, PageField, TagElement);
  }

  public String GetXMLHighlightRecordFormat(RESULT Result, String PageField) {
    return IBJNI.IDB_GetXMLHighlightRecordFormat__SWIG_1(swigCPtr, this, RESULT.getCPtr(Result), Result, PageField);
  }

  public String GetXMLHighlightRecordFormat(RESULT Result) {
    return IBJNI.IDB_GetXMLHighlightRecordFormat__SWIG_2(swigCPtr, this, RESULT.getCPtr(Result), Result);
  }

  public int GetNodeOffsetCount(java.math.BigInteger HitGp, String NodeName, FC ContentFC, FC ParentFC) {
    return IBJNI.IDB_GetNodeOffsetCount__SWIG_0(swigCPtr, this, HitGp, NodeName, FC.getCPtr(ContentFC), ContentFC, FC.getCPtr(ParentFC), ParentFC);
  }

  public int GetNodeOffsetCount(java.math.BigInteger HitGp, String NodeName, FC ContentFC) {
    return IBJNI.IDB_GetNodeOffsetCount__SWIG_1(swigCPtr, this, HitGp, NodeName, FC.getCPtr(ContentFC), ContentFC);
  }

  public int GetNodeOffsetCount(java.math.BigInteger HitGp, String NodeName) {
    return IBJNI.IDB_GetNodeOffsetCount__SWIG_2(swigCPtr, this, HitGp, NodeName);
  }

  public int GetNodeOffsetCount(java.math.BigInteger HitGp) {
    return IBJNI.IDB_GetNodeOffsetCount__SWIG_3(swigCPtr, this, HitGp);
  }

  public FCT GetDescendentsFCT(FC HitFc, String NodeName) {
    return new FCT(IBJNI.IDB_GetDescendentsFCT(swigCPtr, this, FC.getCPtr(HitFc), HitFc, NodeName), true);
  }

  public FC GetAncestorFc(FC HitFc, String NodeName) {
    return new FC(IBJNI.IDB_GetAncestorFc(swigCPtr, this, FC.getCPtr(HitFc), HitFc, NodeName), true);
  }

  public long GetDescendentsContent(FC HitFc, String NodeName, STRLIST StrlistPtr) {
    return IBJNI.IDB_GetDescendentsContent(swigCPtr, this, FC.getCPtr(HitFc), HitFc, NodeName, STRLIST.getCPtr(StrlistPtr), StrlistPtr);
  }

  public long GetAncestorContent(RESULT Result, String NodeName, STRLIST StrlistPtr) {
    return IBJNI.IDB_GetAncestorContent(swigCPtr, this, RESULT.getCPtr(Result), Result, NodeName, STRLIST.getCPtr(StrlistPtr), StrlistPtr);
  }

  public RESULT KeyLookup(String Key) {
    long cPtr = IBJNI.IDB_KeyLookup(swigCPtr, this, Key);
    return (cPtr == 0) ? null : new RESULT(cPtr, true);
  }

  public boolean KeyExists(String Key) {
    return IBJNI.IDB_KeyExists(swigCPtr, this, Key);
  }

  public STRLIST GetFields(RESULT result) {
    return new STRLIST(IBJNI.IDB_GetFields__SWIG_0(swigCPtr, this, RESULT.getCPtr(result), result), true);
  }

  public STRLIST GetFields() {
    return new STRLIST(IBJNI.IDB_GetFields__SWIG_1(swigCPtr, this), true);
  }

}