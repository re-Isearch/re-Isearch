/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class VIDB extends IDBOBJ {
  private long swigCPtr;

  protected VIDB(long cPtr, boolean cMemoryOwn) {
    super(IBJNI.SWIGVIDBUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(VIDB obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      IBJNI.delete_VIDB(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  public VIDB() {
    this(IBJNI.new_VIDB__SWIG_0(), true);
  }

  public VIDB(String DbFullPrefix, boolean Searching) {
    this(IBJNI.new_VIDB__SWIG_1(DbFullPrefix, Searching), true);
  }

  public VIDB getself() {
    return new VIDB(IBJNI.VIDB_getself(swigCPtr, this), false);
  }

  public IDB GetIDB(long idx) {
    long cPtr = IBJNI.VIDB_GetIDB__SWIG_0(swigCPtr, this, idx);
    return (cPtr == 0) ? null : new IDB(cPtr, false);
  }

  public IDB GetIDB() {
    long cPtr = IBJNI.VIDB_GetIDB__SWIG_1(swigCPtr, this);
    return (cPtr == 0) ? null : new IDB(cPtr, false);
  }

  public long GetIDBCount() {
    return IBJNI.VIDB_GetIDBCount(swigCPtr, this);
  }

  public boolean IsDbVirtual() {
    return IBJNI.VIDB_IsDbVirtual(swigCPtr, this);
  }

  public MDT GetMainMdt() {
    long cPtr = IBJNI.VIDB_GetMainMdt__SWIG_0(swigCPtr, this);
    return (cPtr == 0) ? null : new MDT(cPtr, false);
  }

  public MDT GetMainMdt(long Idx) {
    long cPtr = IBJNI.VIDB_GetMainMdt__SWIG_1(swigCPtr, this, Idx);
    return (cPtr == 0) ? null : new MDT(cPtr, false);
  }

  public FCACHE GetFieldCache() {
    long cPtr = IBJNI.VIDB_GetFieldCache__SWIG_0(swigCPtr, this);
    return (cPtr == 0) ? null : new FCACHE(cPtr, false);
  }

  public FCACHE GetFieldCache(long Idx) {
    long cPtr = IBJNI.VIDB_GetFieldCache__SWIG_1(swigCPtr, this, Idx);
    return (cPtr == 0) ? null : new FCACHE(cPtr, false);
  }

  public ArraySTRING GetDocTypeOptions() {
    return new ArraySTRING(IBJNI.VIDB_GetDocTypeOptions(swigCPtr, this), true);
  }

  public String GetDbFileStem(int Idx) {
    return IBJNI.VIDB_GetDbFileStem__SWIG_0(swigCPtr, this, Idx);
  }

  public String GetDbFileStem() {
    return IBJNI.VIDB_GetDbFileStem__SWIG_1(swigCPtr, this);
  }

  public String XMLHitTable(RESULT Result) {
    return IBJNI.VIDB_XMLHitTable(swigCPtr, this, RESULT.getCPtr(Result), Result);
  }

  public String XMLNodeTree(RESULT ResultRecord, FC Fc) {
    return IBJNI.VIDB_XMLNodeTree(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, FC.getCPtr(Fc), Fc);
  }

  public void SetPriorityFactor(double x, long idx) {
    IBJNI.VIDB_SetPriorityFactor__SWIG_0(swigCPtr, this, x, idx);
  }

  public void SetPriorityFactor(double x) {
    IBJNI.VIDB_SetPriorityFactor__SWIG_1(swigCPtr, this, x);
  }

  public void SetDbSearchCutoff(long m, long idx) {
    IBJNI.VIDB_SetDbSearchCutoff__SWIG_0(swigCPtr, this, m, idx);
  }

  public void SetDbSearchCutoff(long m) {
    IBJNI.VIDB_SetDbSearchCutoff__SWIG_1(swigCPtr, this, m);
  }

  public long GetDbSearchCutoff() {
    return IBJNI.VIDB_GetDbSearchCutoff(swigCPtr, this);
  }

  public void SetDbSearchFuel(long Percent, long idx) {
    IBJNI.VIDB_SetDbSearchFuel__SWIG_0(swigCPtr, this, Percent, idx);
  }

  public void SetDbSearchFuel(long Percent) {
    IBJNI.VIDB_SetDbSearchFuel__SWIG_1(swigCPtr, this, Percent);
  }

  public void SetDbSearchCacheSize(long NewCacheSize, long idx) {
    IBJNI.VIDB_SetDbSearchCacheSize__SWIG_0(swigCPtr, this, NewCacheSize, idx);
  }

  public void SetDbSearchCacheSize(long NewCacheSize) {
    IBJNI.VIDB_SetDbSearchCacheSize__SWIG_1(swigCPtr, this, NewCacheSize);
  }

  public void BeforeSearching(QUERY SearchQuery) {
    IBJNI.VIDB_BeforeSearching(swigCPtr, this, QUERY.getCPtr(SearchQuery), SearchQuery);
  }

  public void SetDebugMode(boolean OnOff) {
    IBJNI.VIDB_SetDebugMode(swigCPtr, this, OnOff);
  }

  public int GetErrorCode(int Idx) {
    return IBJNI.VIDB_GetErrorCode__SWIG_0(swigCPtr, this, Idx);
  }

  public int GetErrorCode() {
    return IBJNI.VIDB_GetErrorCode__SWIG_1(swigCPtr, this);
  }

  public String ErrorMessage(int Idx) {
    return IBJNI.VIDB_ErrorMessage__SWIG_0(swigCPtr, this, Idx);
  }

  public String ErrorMessage() {
    return IBJNI.VIDB_ErrorMessage__SWIG_1(swigCPtr, this);
  }

  public int GetTotalWords(int Idx) {
    return IBJNI.VIDB_GetTotalWords__SWIG_0(swigCPtr, this, Idx);
  }

  public int GetTotalWords() {
    return IBJNI.VIDB_GetTotalWords__SWIG_1(swigCPtr, this);
  }

  public int GetTotalUniqueWords(int Idx) {
    return IBJNI.VIDB_GetTotalUniqueWords__SWIG_0(swigCPtr, this, Idx);
  }

  public int GetTotalUniqueWords() {
    return IBJNI.VIDB_GetTotalUniqueWords__SWIG_1(swigCPtr, this);
  }

  public long GetTotalRecords(int Idx) {
    return IBJNI.VIDB_GetTotalRecords__SWIG_0(swigCPtr, this, Idx);
  }

  public long GetTotalRecords() {
    return IBJNI.VIDB_GetTotalRecords__SWIG_1(swigCPtr, this);
  }

  public long GetTotalDocumentsDeleted(int Idx) {
    return IBJNI.VIDB_GetTotalDocumentsDeleted__SWIG_0(swigCPtr, this, Idx);
  }

  public long GetTotalDocumentsDeleted() {
    return IBJNI.VIDB_GetTotalDocumentsDeleted__SWIG_1(swigCPtr, this);
  }

  public long GetTotalDatabases() {
    return IBJNI.VIDB_GetTotalDatabases(swigCPtr, this);
  }

  public boolean IsDbCompatible() {
    return IBJNI.VIDB_IsDbCompatible(swigCPtr, this);
  }

  public boolean IsEmpty() {
    return IBJNI.VIDB_IsEmpty(swigCPtr, this);
  }

  public boolean Ok() {
    return IBJNI.VIDB_Ok(swigCPtr, this);
  }

  public void SetCommonWordsThreshold(int x) {
    IBJNI.VIDB_SetCommonWordsThreshold(swigCPtr, this, x);
  }

  public void SetStoplist(String Filename) {
    IBJNI.VIDB_SetStoplist(swigCPtr, this, Filename);
  }

  public boolean IsStopWord(String Word) {
    return IBJNI.VIDB_IsStopWord(swigCPtr, this, Word);
  }

  public String GetTitle(int Idx) {
    return IBJNI.VIDB_GetTitle__SWIG_0(swigCPtr, this, Idx);
  }

  public String GetTitle() {
    return IBJNI.VIDB_GetTitle__SWIG_1(swigCPtr, this);
  }

  public String GetComments(int Idx) {
    return IBJNI.VIDB_GetComments__SWIG_0(swigCPtr, this, Idx);
  }

  public String GetComments() {
    return IBJNI.VIDB_GetComments__SWIG_1(swigCPtr, this);
  }

  public String GetMaintainer(int Idx) {
    return IBJNI.VIDB_GetMaintainer__SWIG_0(swigCPtr, this, Idx);
  }

  public String GetMaintainer() {
    return IBJNI.VIDB_GetMaintainer__SWIG_1(swigCPtr, this);
  }

  public IRSET SearchSmart(QUERY INOUT, String DefaultField) {
    long cPtr = IBJNI.VIDB_SearchSmart__SWIG_0(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT, DefaultField);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET SearchSmart(QUERY INOUT) {
    long cPtr = IBJNI.VIDB_SearchSmart__SWIG_1(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET Search(QUERY Query) {
    long cPtr = IBJNI.VIDB_Search__SWIG_0(swigCPtr, this, QUERY.getCPtr(Query), Query);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public IRSET Search(QUERY Query, VIDB_STATS INOUT) {
    long cPtr = IBJNI.VIDB_Search__SWIG_1(swigCPtr, this, QUERY.getCPtr(Query), Query, VIDB_STATS.getCPtr(INOUT), INOUT);
    return (cPtr == 0) ? null : new IRSET(cPtr, true);
  }

  public RSET VSearch(QUERY Query) {
    long cPtr = IBJNI.VIDB_VSearch(swigCPtr, this, QUERY.getCPtr(Query), Query);
    return (cPtr == 0) ? null : new RSET(cPtr, true);
  }

  public RSET VSearchSmart(QUERY INOUT, String DefaultFIeld) {
    long cPtr = IBJNI.VIDB_VSearchSmart__SWIG_0(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT, DefaultFIeld);
    return (cPtr == 0) ? null : new RSET(cPtr, true);
  }

  public RSET VSearchSmart(QUERY INOUT) {
    long cPtr = IBJNI.VIDB_VSearchSmart__SWIG_1(swigCPtr, this, QUERY.getCPtr(INOUT), INOUT);
    return (cPtr == 0) ? null : new RSET(cPtr, true);
  }

  public SCANLIST Scan(String Term) {
    return new SCANLIST(IBJNI.VIDB_Scan__SWIG_0(swigCPtr, this, Term), true);
  }

  public SCANLIST Scan(String Term, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.VIDB_Scan__SWIG_1(swigCPtr, this, Term, TotalTermsRequested), true);
  }

  public SCANLIST Scan(String Term, String Field) {
    return new SCANLIST(IBJNI.VIDB_Scan__SWIG_2(swigCPtr, this, Term, Field), true);
  }

  public SCANLIST Scan(String Term, String Field, int TotalTermsRequested) {
    return new SCANLIST(IBJNI.VIDB_Scan__SWIG_3(swigCPtr, this, Term, Field, TotalTermsRequested), true);
  }

  public SCANLIST ScanGlob(String Pattern) {
    return new SCANLIST(IBJNI.VIDB_ScanGlob__SWIG_0(swigCPtr, this, Pattern), true);
  }

  public SCANLIST ScanGlob(String Pattern, SWIGTYPE_p_INT TotalTermsRequested) {
    return new SCANLIST(IBJNI.VIDB_ScanGlob__SWIG_1(swigCPtr, this, Pattern, SWIGTYPE_p_INT.getCPtr(TotalTermsRequested)), true);
  }

  public SCANLIST ScanGlob(String Pattern, String Field) {
    return new SCANLIST(IBJNI.VIDB_ScanGlob__SWIG_2(swigCPtr, this, Pattern, Field), true);
  }

  public SCANLIST ScanGlob(String Pattern, String Field, SWIGTYPE_p_INT TotalTermsRequested) {
    return new SCANLIST(IBJNI.VIDB_ScanGlob__SWIG_3(swigCPtr, this, Pattern, Field, SWIGTYPE_p_INT.getCPtr(TotalTermsRequested)), true);
  }

  public SCANLIST ScanSearch(QUERY Query, String Fieldname) {
    return new SCANLIST(IBJNI.VIDB_ScanSearch__SWIG_0(swigCPtr, this, QUERY.getCPtr(Query), Query, Fieldname), true);
  }

  public SCANLIST ScanSearch(QUERY Query, String Fieldname, long MaxRecordsThreshold) {
    return new SCANLIST(IBJNI.VIDB_ScanSearch__SWIG_1(swigCPtr, this, QUERY.getCPtr(Query), Query, Fieldname, MaxRecordsThreshold), true);
  }

  public void BeginRsetPresent(String RecordSyntax) {
    IBJNI.VIDB_BeginRsetPresent(swigCPtr, this, RecordSyntax);
  }

  public long GetAncestorContent(RESULT Result, String NodeName, STRLIST StrlistPtr) {
    return IBJNI.VIDB_GetAncestorContent(swigCPtr, this, RESULT.getCPtr(Result), Result, NodeName, STRLIST.getCPtr(StrlistPtr), StrlistPtr);
  }

  public String Headline(RESULT ResultRecord) {
    return IBJNI.VIDB_Headline__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String Headline(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.VIDB_Headline__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public String Summary(RESULT ResultRecord) {
    return IBJNI.VIDB_Summary__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String Summary(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.VIDB_Summary__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public String Context(RESULT ResultRecord, String Before, String After) {
    return IBJNI.VIDB_Context__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, Before, After);
  }

  public String Context(RESULT ResultRecord) {
    return IBJNI.VIDB_Context__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String NthContext(long N, RESULT ResultRecord) {
    return IBJNI.VIDB_NthContext__SWIG_0(swigCPtr, this, N, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String NthContext(long N, RESULT ResultRecord, String Before, String After) {
    return IBJNI.VIDB_NthContext__SWIG_1(swigCPtr, this, N, RESULT.getCPtr(ResultRecord), ResultRecord, Before, After);
  }

  public String URL(RESULT ResultRecord, boolean OnlyRemote) {
    return IBJNI.VIDB_URL__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, OnlyRemote);
  }

  public String URL(RESULT ResultRecord) {
    return IBJNI.VIDB_URL__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String HighlightedRecord(RESULT ResultRecord, String BeforeTerm, String AfterTerm) {
    return IBJNI.VIDB_HighlightedRecord(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, BeforeTerm, AfterTerm);
  }

  public String DocHighlight(RESULT ResultRecord) {
    return IBJNI.VIDB_DocHighlight__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord);
  }

  public String DocHighlight(RESULT ResultRecord, String RecordSyntax) {
    return IBJNI.VIDB_DocHighlight__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, RecordSyntax);
  }

  public STRLIST GetFieldData(RESULT ResultPtr, String ESet, DOCTYPE_ID Doctype) {
    long cPtr = IBJNI.VIDB_GetFieldData__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet, DOCTYPE_ID.getCPtr(Doctype), Doctype);
    return (cPtr == 0) ? null : new STRLIST(cPtr, false);
  }

  public STRLIST GetFieldData(RESULT ResultPtr, String ESet) {
    long cPtr = IBJNI.VIDB_GetFieldData__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet);
    return (cPtr == 0) ? null : new STRLIST(cPtr, false);
  }

  public ArraySTRING GetFieldContents(RESULT ResultPtr, String ESet) {
    return new ArraySTRING(IBJNI.VIDB_GetFieldContents(swigCPtr, this, RESULT.getCPtr(ResultPtr), ResultPtr, ESet), true);
  }

  public String Present(RESULT ResultRecord, String ElementSet) {
    return IBJNI.VIDB_Present__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet);
  }

  public String Present(RESULT ResultRecord, String ElementSet, String RecordSyntax) {
    return IBJNI.VIDB_Present__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet, RecordSyntax);
  }

  public String DocPresent(RESULT ResultRecord, String ElementSet) {
    return IBJNI.VIDB_DocPresent__SWIG_0(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet);
  }

  public String DocPresent(RESULT ResultRecord, String ElementSet, String RecordSyntax) {
    return IBJNI.VIDB_DocPresent__SWIG_1(swigCPtr, this, RESULT.getCPtr(ResultRecord), ResultRecord, ElementSet, RecordSyntax);
  }

  public void EndRsetPresent(String RecordSyntax) {
    IBJNI.VIDB_EndRsetPresent(swigCPtr, this, RecordSyntax);
  }

  public String GetGlobalDocType() {
    return IBJNI.VIDB_GetGlobalDocType(swigCPtr, this);
  }

  public RESULT KeyLookup(String Key) {
    long cPtr = IBJNI.VIDB_KeyLookup(swigCPtr, this, Key);
    return (cPtr == 0) ? null : new RESULT(cPtr, false);
  }

  public boolean KeyExists(String Key) {
    return IBJNI.VIDB_KeyExists(swigCPtr, this, Key);
  }

  public boolean SetDateRange(DATERANGE DateRange) {
    return IBJNI.VIDB_SetDateRange(swigCPtr, this, DATERANGE.getCPtr(DateRange), DateRange);
  }

  public String ProfileGetString(String Section, String Entry, String Default) {
    return IBJNI.VIDB_ProfileGetString__SWIG_0(swigCPtr, this, Section, Entry, Default);
  }

  public String ProfileGetString(String Section, String Entry) {
    return IBJNI.VIDB_ProfileGetString__SWIG_1(swigCPtr, this, Section, Entry);
  }

  public String FirstKey() {
    return IBJNI.VIDB_FirstKey(swigCPtr, this);
  }

  public String LastKey() {
    return IBJNI.VIDB_LastKey(swigCPtr, this);
  }

  public String NextKey(String Key) {
    return IBJNI.VIDB_NextKey(swigCPtr, this, Key);
  }

  public String PrevKey(String Key) {
    return IBJNI.VIDB_PrevKey(swigCPtr, this, Key);
  }

  public boolean GetDocumentInfo(int Idx, int Index, RECORD RecordBuffer) {
    return IBJNI.VIDB_GetDocumentInfo(swigCPtr, this, Idx, Index, RECORD.getCPtr(RecordBuffer), RecordBuffer);
  }

  public SRCH_DATE DateCreated() {
    return new SRCH_DATE(IBJNI.VIDB_DateCreated(swigCPtr, this), true);
  }

  public SRCH_DATE DateLastModified() {
    return new SRCH_DATE(IBJNI.VIDB_DateLastModified(swigCPtr, this), true);
  }

  public STRLIST GetAllDocTypes() {
    return new STRLIST(IBJNI.VIDB_GetAllDocTypes(swigCPtr, this), true);
  }

  public boolean ValidateDocType(String DocType) {
    return IBJNI.VIDB_ValidateDocType(swigCPtr, this, DocType);
  }

  public String GetVersionID() {
    return IBJNI.VIDB_GetVersionID(swigCPtr, this);
  }

  public int GetLocks() {
    return IBJNI.VIDB_GetLocks(swigCPtr, this);
  }

  public ArraySTRING GetFields(RESULT result) {
    return new ArraySTRING(IBJNI.VIDB_GetFields__SWIG_0(swigCPtr, this, RESULT.getCPtr(result), result), true);
  }

  public ArraySTRING GetFields() {
    return new ArraySTRING(IBJNI.VIDB_GetFields__SWIG_1(swigCPtr, this), true);
  }

}