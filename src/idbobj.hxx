/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		idbobj.hxx
Description:	Class IDBOBJ: Database object virtual class
@@@*/

#ifndef IDBOBJ_HXX
#define IDBOBJ_HXX

#include "mdt.hxx"
#include "dfdt.hxx"
#include "dfd.hxx"
#include "result.hxx"
#include "record.hxx"
#include "hash.hxx"
#include "metadata.hxx"
#include "nodetree.hxx"
#include "words.hxx"
#include "boost.hxx"

class  DOCTYPE;
class  INDEX;
class  FCACHE;
class  FPT;
class  SCANLIST;
class  QUERY;


typedef enum MergeStatus { iNothing = 0, iOptimize, iMerge, iCollapse, iIncremental } t_merge_status;

class atomicIRSET;
class METADATA;

class IDBOBJ {
friend class IDB;
friend class INDEX;
friend class IRSET;
friend class atomicIRSET;
friend class DOCTYPE;
friend class METADATA;
public:
  IDBOBJ() { };
  virtual ~IDBOBJ() { }
  virtual size_t GetIndexingMemory() const { return 0; }
  virtual void DfdtAddEntry(const DFD&) { }
  virtual bool DfdtGetEntry(const size_t, PDFD) const { return false; }
  virtual size_t DfdtGetTotalEntries() const { return 0; }
  virtual size_t GetTotalRecords() const { return 0; }
  virtual size_t GetTotalDocumentsDeleted() const { return 0; }

  virtual  void SetMirrorBaseDirectory(const STRING& Mirror) {;}

  virtual off_t   GetTotalWords() const   { return 0; }
  virtual off_t   GetTotalUniqueWords() const { return 0; }
  virtual bool GetRecordDfdt(const STRING&, PDFDT)  { return false;}
  bool GetRecordDfdt(const RESULT& Result, DFDT *DfdtBuffer) {
    return GetRecordDfdt(Result.GetKey(), DfdtBuffer);
  }
  virtual bool Ok () const { return false; }
  virtual int BitVersion() const { return sizeof(GPTYPE)*8; }

  bool     ValidateDocType(const DOCTYPE_ID& Id) const { return Id.IsDefined(); }

  virtual bool GetDateRange(DATERANGE *DateRange = NULL) const {
     if (DateRange) DateRange->Clear();
     return false;
  }


  virtual bool ValidNodeName(const STRING& nodeName) const { return true; }

  virtual bool DfdtGetFileName(const DFD&, STRING *)   { return false; }
  virtual bool DfdtGetFileName(const STRING&, const FIELDTYPE&, STRING *) {
    return false; }
  virtual bool DfdtGetFileName(const STRING&, PSTRING) { return false; }

  virtual DOUBLE GetPriorityFactor() const       { return 0; }
  virtual DOUBLE GetIndexBoostFactor() const     { return 0; }
  virtual DOUBLE GetFreshnessBoostFactor() const { return 0; }
  virtual DOUBLE GetLongevityBoostFactor() const { return 0; }

  virtual size_t GetDbSearchCutoff() const   { return 0; } 

  virtual bool UsePersistantCache() const { return false; }
  virtual STRING PersistantCacheName() const     { return NulString; }

  virtual STRING GetFieldData(const RESULT&, const STRING&, const DOCTYPE* = NULL)  { 
    return NulString;
  }
  virtual bool GetFieldData(const RESULT& Result, const STRING& RecSyntax,
	PSTRING StringPtr, const DOCTYPE *DocPtr = NULL)  { 
    return !(*StringPtr = GetFieldData(Result, RecSyntax, DocPtr)).IsEmpty();

  }
  virtual bool GetFieldData(const RESULT&, const STRING&, PSTRLIST, const DOCTYPE * = NULL) { return false; }

  virtual bool GetFieldData(GPTYPE, DOUBLE*) {return false; }
  virtual bool GetFieldData(GPTYPE, SRCH_DATE* ) {return false; }
  virtual bool GetFieldData(GPTYPE, STRING*) {return false; }

  virtual bool GetFieldData(const FC&, const STRING&, STRING* Buffer) {
    if (Buffer) Buffer->Clear();
    return false;
  }

 virtual  bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        const FIELDTYPE& FieldType, STRING *StringBuffer, const DOCTYPE *DoctypePtr = NULL)
    { return false; }

/*
  virtual bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        DOUBLE* Buffer) { return false; }
  virtual bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        DATERANGE* Buffer) { return false; }
  virtual bool GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
        SRCH_DATE* Buffer) { return false; }
*/

  virtual void GetDocTypeOptions(PSTRLIST) const { }
  virtual const STRLIST *GetDocTypeOptionsPtr() const { return NULL;       }
  virtual const STRLIST& GetDocTypeOptions() const    { return NulStrlist; }

  virtual STRING Description() const { return NulString; }

  virtual void ProfileGetString(const STRING&, const STRING&,
		const STRING&, PSTRING) const { }

#if 1
  virtual STRING ProfileGetString(const STRING& Section, const STRING& Entry,
        const STRING& Default=NulString) const {
    STRING value;
    ProfileGetString(Section, Entry, Default, &value);
    return value;
  }
  virtual GPTYPE ProfileGetGPTYPE(const STRING& Section, const STRING& Entry,
	const GPTYPE& DefaultVal=0) {
    STRING s;
    ProfileGetString(Section, Entry, STRING(DefaultVal), &s);
    return ((GPTYPE)s.GetLongLong());
  }

#endif
  virtual void ProfileWriteString(const STRING&, const STRING&, const STRING&) { };

  virtual void GetFieldDefinitionList(STRLIST *) const { };

  virtual SRCH_DATE DateCreated() const {return SRCH_DATE(); };
  virtual SRCH_DATE DateLastModified() const { return DateCreated(); }

  virtual void GetMaintainer(STRING *, STRING *) const {};

//virtual void GetRecordData(const RESULT&, PSTRING) const { };
  virtual STRING  ComposeDbFn(const CHR*) const { return STRING(); };
  virtual STRING  ComposeDbFn(INT ) const { return STRING(); }
  virtual STRING& ComposeDbFn(STRING *Ptr, const CHR*) const { return *Ptr; };

  // 1st Queue
  virtual bool  AddRecord(const RECORD&) { return false; }
  // 2nd Queue
  virtual void         DocTypeAddRecord(const RECORD&) { };

  virtual bool GetDebugMode() const { return true; }


  virtual SRCH_DATE   GetTimestamp() const { return SRCH_DATE(); }

  virtual void ffGC() {; }
  virtual PFILE ffopen(const STRING& Name, const CHR* Mode) { return fopen(Name, Mode); }
  virtual INT ffclose(PFILE Fp){ return fclose(Fp); }
  virtual INT ffdispose(PFILE) { return 0; }
  virtual INT ffdispose(const STRING&) { return 0; }

  virtual DOCTYPE *GetDocTypePtr(const DOCTYPE_ID&) const { return NULL; }

  virtual bool KeyLookup (const STRING&, PRESULT = NULL) const { return false;};

  virtual STRING FirstKey() const             { return NulString; }
  virtual STRING LastKey() const              { return NulString; }
  virtual STRING NextKey(const STRING&) const { return NulString; }
  virtual STRING PrevKey(const STRING&) const { return NulString; }

  virtual bool GetDocumentInfo(const INT, RECORD *) const { return false; }

  virtual FC GetPeerFc (const GPTYPE& HitGp, STRING *NodeNamePtr = NULL) {
    return GetPeerFc(FC(HitGp,HitGp), NodeNamePtr);
  }
  virtual FC GetPeerFc (const FC& HitFc,     STRING *NodeNamePtr = NULL) {
    if (NodeNamePtr) *NodeNamePtr = "???";
    return FC(0,0);
  }
#if 0
  virtual int GetNodeList (const GPTYPE& HitGp, TREENODELIST *NodeList) {
    return GetNodeList(FC(HitGp,HitGp), NodeList);
  }
  virtual int GetNodeList (const FC& HitFc, TREENODELIST *NodeList) {
    if (NodeList) NodeList->Clear();
    return 0;
  }
#endif
  virtual NODETREE GetNodeTree( const GPTYPE& HitGp) { return GetNodeTree(FC(HitGp,HitGp)); }
  virtual NODETREE GetNodeTree( const FC& HitFc)     { return NODETREE(); }


  virtual STRING GetPeerContent(const GPTYPE& HitGp) { return GetPeerContent(FC(HitGp,HitGp)); }
  virtual STRING GetPeerContentXMLFragement(const GPTYPE& HitGp) { 
    return GetPeerContentXMLFragement(FC(HitGp,HitGp));
  }
  virtual STRING GetPeerContent(const FC&)             { return NulString; }
  virtual STRING GetPeerContentXMLFragement(const FC&) { return NulString; }


  virtual DFDT *GetDfdt() { return NULL; }
  virtual DFDT *GetDfdt(DFDT *DfdtBuffer, const RESULT * = NULL) {
    if (DfdtBuffer) {
      DFDT *res = GetDfdt();
      if (res == NULL)
	DfdtBuffer->Empty();
      else
	*DfdtBuffer = *res;
    }
    return DfdtBuffer;
  }

  virtual t_merge_status GetMergeStatus() const { return iNothing; }

  virtual bool IsStopWord (const STRING&) const { return true; };
  virtual bool IsStopWord (const UCHR *, STRINGINDEX, INT=0) const { return true; };

  virtual void   SetServerName(const STRING&) {};
  virtual STRING GetServerName() const { return NulString; }

  virtual STRING GetDbFileStem() const { return STRING(); }
  virtual STRING GetDbFileStem(PSTRING ptr) const { return *ptr = GetDbFileStem(); }
  virtual STRING DbName() const {return STRING(); }
  virtual STRING DbName(PSTRING ptr) const {return *ptr = DbName(); }
  virtual STRING URLfrag() const {return STRING(); }
  virtual STRING URLfrag(PSTRING ptr) const {return *ptr = URLfrag(); }
  virtual STRING URLfrag(const RESULT&) const { return STRING();}
  virtual STRING URLfrag(const RESULT& res, PSTRING ptr) const { return *ptr = URLfrag(res);}

  virtual size_t MdtLookupKey (const STRING&) const { return 0; }
  virtual void   MdtSetUniqueKey(PRECORD, const STRING&) { }

  virtual void   SetOverride(bool) {  }
  virtual bool GetOverride() const { return false; }

  virtual size_t Scan(PSTRLIST, const STRING&, const size_t = 0, const INT =-1) const { return 0;};
  virtual size_t Scan(PSTRLIST, const STRING&, const STRING&, const INT) const { return 0;};

  virtual size_t Scan(SCANLIST *, const STRING&, const size_t = 0, const INT =-1) const { return 0;};
  virtual size_t Scan(SCANLIST *, const STRING&, const STRING&, const INT) const { return 0;};

  virtual size_t ScanSearch(SCANLIST *, const QUERY&, const STRING&, size_t Max = 0, bool Cat = false) { return 0; }

  virtual atomicIRSET *FileSearch(const STRING&) { return NULL; }
  virtual atomicIRSET *KeySearch(const STRING&) { return NULL; }
  virtual atomicIRSET *DoctypeSearch(const STRING&) { return NULL;}

  virtual void AddTemplate (const STRING&) {}

  virtual bool FieldExists(const STRING& Fieldname) const { return false; }

  virtual void ParseRecords(RECORD&) { }
  virtual void ParseFields(PRECORD) { };

  virtual PMDT GetMainMdt() const { return 0; }
  virtual PMDT  GetMainMdt(INT) const { return GetMainMdt(); }
  virtual PDFDT GetMainDfdt() { return 0; }

  virtual INDEX *GetMainIndex() const { return 0; }
  virtual INDEX *GetMainIndex(INT) const { return GetMainIndex(); }

  virtual size_t ImportIndex(IDBOBJ *IndexPtr) { return 0; }

  virtual FCACHE *GetFieldCache() { return NULL; }
  virtual FCACHE *GetFieldCache(INT) { return GetFieldCache(); }

  virtual void SetFindConcatWords(bool Set=true) { ; }
  virtual bool GetFindConcatWords() const { return false; } 

  virtual INT GetVolume(PSTRING = NULL) const { return 0; }

  virtual int   SetErrorCode(int) { return 0; }
  virtual int   GetErrorCode() const { return 0; }
  virtual const char *ErrorMessage() const { return NULL; }

  virtual LOCALE GetLocale() const { return LOCALE(0); }

  // Headline
  virtual bool Headline(const RESULT&, const STRING&, PSTRING) const {
    return false;
  }
  virtual bool Headline(const RESULT&, PSTRING) const {
    return false;
  }
  virtual STRING      Headline(const RESULT& ResultRecord, const STRING& RecordSyntax) const {
    STRING Buffer;
    Headline(ResultRecord, RecordSyntax, &Buffer);
    return Buffer;
  }

  // Record Summary (if available)
  virtual bool Summary(const RESULT&, const STRING&, STRING *StringBuffer) const {
    StringBuffer->Clear();
    return false;
  }
  virtual STRING      Summary(const RESULT& ResultRecord, const STRING& RecordSyntax) const {
    STRING Buffer;
    Summary(ResultRecord, RecordSyntax, &Buffer);
    return Buffer;
  }

  virtual STRING XMLHitTable(const RESULT& ResultRecord) { return ResultRecord.XMLHitTable(); }

  // Record Highlighting
  virtual void HighlightedRecord(const RESULT&, const STRING&, const STRING&, PSTRING) const {};
  virtual void   DocHighlight (const RESULT&, const STRING&, PSTRING) const {};
  virtual STRING DocHighlight(const RESULT& ResultRecord, const STRING& RecordSyntax) const {
    STRING Buffer;
    DocHighlight(ResultRecord, RecordSyntax, &Buffer);
    return Buffer;
  }

  virtual size_t GetAncestorContent (RESULT& Result, const STRING& NodeName, STRLIST *StrlistPtr) {
    if (StrlistPtr) StrlistPtr->Clear();
    return 0;
  }
  // Added 2023 for ExoDAO
  virtual FC GetAncestorFc (const FC& HitFc, const STRING& NodeName) {
    return HitFc;
  }

  virtual void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const { StringBuffer->Clear(); }
  virtual void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const { StringBuffer->Clear(); }


  // For Presentation (Copyrights for data)
  virtual void   SetTitle(const STRING&)         {                        }
  virtual STRING GetTitle() const                { return NulString;      }
  virtual void   GetTitle(STRING *Ptr) const     { *Ptr = GetTitle();     }
  virtual void   SetComments(const STRING&)      {                        }
  virtual STRING GetComments() const             { return NulString;      }
  virtual void   GetComments(STRING *Ptr) const  { *Ptr = GetComments();  }
  virtual void   SetCopyright(const STRING&)     {                        }
  virtual STRING GetCopyright() const            { return NulString;      }
  virtual void   GetCopyright(STRING *Ptr) const { *Ptr = GetCopyright(); }

  virtual bool GetHTTP_server(PSTRING) const { return false;};
  virtual bool GetHTTP_root(PSTRING= NULL, bool * = NULL) const   { return false;};

  virtual METADATA *GetMetadefaults(const STRING&) { return NULL; }

  virtual bool DoctypePluginExists(const STRING&) const { return false; } 


  virtual FCT       GetFieldFCT (const MDTREC&, const STRING&) { return FCT(); };

  void       AddFieldType(const STRING& Definition); 
  void       AddFieldType(const STRING& FieldName, FIELDTYPE FieldType);
  FIELDTYPE  GetFieldType(const STRING& FieldName);


  virtual   FPT *GetMainFpt() { return NULL; }

  virtual   bool SetSortIndexes(int, atomicIRSET *) { return false; }

  bool _write_resource_path (const STRING& file,
	const RECORD& Filerecord, STRING *PathFilePath = NULL) const;
  bool _write_resource_path (const STRING& file,
	const RECORD& Filerecord, const STRING& mime, STRING *PathFilePath = NULL) const;
  void        _get_resource_path(STRING *fullPathPtr) const;
  STRING      _get_resource_path (const RESULT& Result) const {
    return    _get_resource_path (Result.GetFullFileName()); }
  STRING      _get_resource_path(const STRING& FullPath) const;

  void       SetWorkingDirectory();
  void       SetWorkingDirectory(const STRING& Dir);
  STRING     GetWorkingDirectory() const { return WorkingDirectory; }

  virtual bool getUseRelativePaths() const { return useRelativePaths; }
  virtual bool setUseRelativePaths(bool val=true) {
	useRelativePaths = val; return (!WorkingDirectory.IsEmpty() || val == false); }

  STRING     RelativizePathname(const STRING& Path) const;

  virtual bool ResolvePathname(STRING *Path) const;
  virtual STRING      ResolvePathname(const STRING& Path) const;

  virtual int         Segment(const STRING&) { return 0; }

  virtual bool         checkFieldName(const STRING& fieldname) const {
          return true;
  }

protected:
  virtual void IndexingStatus(const t_IndexingStatus, const STRING&, const long = 0)  { };
  virtual void IndexingStatus(const t_IndexingStatus Status, const long Count = 0)  {
    IndexingStatus (Status, NulString, Count); };

  STRING StoplistFileName; // @@@ edz

private:
  HASH    FieldTypes;
  STRING  WorkingDirectory;
  bool useRelativePaths;
//virtual PMDT GetMainMdt() { return 0; };
//virtual PDFDT GetMainDfdt() { return 0; };
  virtual void SelectRegions(const RECORD&, FCT *) { };
  virtual GPTYPE ParseWords(const DOCTYPE_ID&, UCHR*, GPTYPE, GPTYPE,
		GPTYPE*, GPTYPE) { return 0; }; 
  virtual size_t  ParseWords2(const STRING&, WORDSLIST *) const { return 0; }
};

typedef IDBOBJ* PIDBOBJ;

class DBINFO {
public:
  DBINFO(const PIDBOBJ DbParent);
  DBINFO(const PIDBOBJ DbParent, PSTRLIST DocTypeOptions);

  void   SetValue(const STRING& Entry, const STRING& Value);
  void   SetValue(const STRING& Entry, const STRLIST& Value);

  void   GetOptions(PSTRLIST DocTypeOptions) const;
  STRING GetValue(const STRING& Entry) const;
  STRING GetValue(const STRING& Entry, const STRING& Default) const;
  void   SetTitle(const STRING& Title);
  STRING GetTitle() const;
  void   SetGlobalDoctype(const DOCTYPE_ID& Doctype);
  DOCTYPE_ID GetGlobalDoctype() const;
  void   SetGlobalCharset(const STRING& Charset);
  STRING GetGlobalCharset() const;
  void   SetGlobalStoplist(const STRING& Value);
  STRING GetGlobalStoplist() const;
  void   SetDateCreated(const STRING& Value);
  STRING GetDateCreated() const;
  void   SetDateLastModified(const STRING& Value);
  STRING GetDateLastModified() const;
  void   SetComments(const STRING& Value);
  STRING GetComments() const;
  void   SetRights(const STRING& Value);
  STRING GetRights() const;
  void   SetMaintainerName(const STRING& Name);
  STRING GetMaintainerName() const;
  void   SetMaintainerEmail(const STRING& Name);
  STRING GetMaintainerEmail() const;
  bool GetDatabaseList(PSTRLIST FilenameList) const;
  ~DBINFO();
private:
  void init(const PIDBOBJ DbParent, PSTRLIST DocTypeOptions);
  PIDBOBJ     Parent;
  DOCTYPE_ID  GlobalDoctype;
  bool DbInfoChanged;
  METADATA   *MainRegistry;
};


#endif
