/************************************************************************
************************************************************************/
/*@@@
File:		doctype.hxx
Version:	2.00
Description:	Class DOCTYPE - Document Type
@@@*/

#ifndef DOCTYPE_HXX
#define DOCTYPE_HXX

#include "defs.hxx"
#include "idbobj.hxx"
#include "result.hxx"
#include "squery.hxx"
#include "rset.hxx"
#include "registry.hxx"
#include "lang-codes.hxx"
#include "numbers.hxx"

const int __IB_Doctype_Major_DefVerson = 6;

class GPOLYFLD;
class BBOXFLD;

class DOCTYPE {
friend class RESULT;
public:
  DOCTYPE();
  DOCTYPE(IDBOBJ* DbParent);
  DOCTYPE(IDBOBJ* DbParent, const STRING& NewDoctype);

  virtual void LoadFieldTable();

  void Help(PSTRING) const;
  const STRING& Name() const { return Doctype; }
  virtual const char *Description(PSTRLIST List) const;

  virtual void BeforeIndexing();
  virtual void AddFieldDefs();
  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void SelectRegions(const RECORD& Record, FCT* FctPtr);

  virtual bool IsStopWord(const UCHR* Word, STRINGINDEX MaxLen) const;

  virtual GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
	GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength);

  virtual size_t ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const;

  virtual STRING     ParseBuffer(const STRING& Buffer) const;
  virtual SRCH_DATE  ParseDate(const STRING& Buffer) const;
  virtual SRCH_DATE  ParseDate(const STRING& Buffer, const STRING& FieldName) const {
	return ParseDate(Buffer); }
  virtual DATERANGE  ParseDateRange(const STRING& Buffer) const;
  virtual DATERANGE  ParseDateRange(const STRING& Buffer, const STRING& FieldName) const {
	return ParseDateRange(Buffer); }

  virtual NUMERICOBJ ParsePhonhash(const STRING& Buffer) const;
  virtual NUMERICOBJ ParseNumeric(const STRING& Buffer) const;
  virtual bool ParseRange(const STRING& Buffer, const STRING& FieldName,
	DOUBLE* fStart, DOUBLE* fEnd) const;

  virtual int        ParseBBox(const STRING& Buffer,const STRING& FieldName,  BBOXFLD* gpoly) const;
  virtual int        ParseBBox(const STRING& Buffer, BBOXFLD* gpoly) const;
  virtual int        ParseGPoly(const STRING& Buffer,const STRING& FieldName,  GPOLYFLD* gpoly) const;
  virtual int        ParseGPoly(const STRING& Buffer, GPOLYFLD* gpoly) const;
  virtual NUMERICOBJ ParseComputed(const STRING& FieldName, const STRING& Buffer) const;
  virtual NUMERICOBJ ParseTTL(const STRING& FieldName, const STRING& Buffer) const;
  virtual long       ParseCategory(const STRING& Buffer) const;
  virtual MONETARYOBJ ParseCurrency(const STRING& FieldName, const STRING& Buffer) const;


  virtual INT ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;
  virtual INT ReadFile(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length) const;
  virtual INT ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  virtual INT ReadFile(const STRING& Filename, STRING *StringPtr, off_t Offset, size_t Length) const;
  virtual INT GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const {
    return ReadFile(Fp, StringPtr, Offset, Length); }
  virtual INT GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const {
    return ReadFile(Fp, Buffer, Offset, Length);    }
  virtual INT GetRecordData(const STRING& Filename, STRING *StringPtr, off_t Offset, size_t Length) const;
  virtual INT GetRecordData(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length) const;
  virtual INT GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length);

  virtual void ParseFields(RECORD* NewRecordPtr);
  virtual void AfterIndexing();
  virtual void BeforeSearching(QUERY* QueryPtr);
  virtual PIRSET AfterSearching(IRSET* ResultSetPtr);
  virtual void BeforeRset(const STRING& RecordSyntax);
  virtual void AfterRset(const STRING& RecordSyntax);

  size_t CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn) const { return 0; }

  virtual REGISTRY *GetMetadata(const RECORD& record,
	const STRING& mdType, const REGISTRY* defaults);
  virtual bool Headline(const STRING& HeadlineFormat,
	const RESULT& ResultRecord, const STRING& RecordSyntax, PSTRING StringBuffer) const;
  virtual bool Headline(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;
  virtual bool Headline(const RESULT& ResultRecord, PSTRING StringBuffer) const;

  virtual bool Summary(const RESULT& ResultRecord,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  virtual bool Summary(const RESULT& ResultRecord, PSTRING StringBuffer) const;

  // These are about getting the URL to access the record
  virtual bool GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  virtual bool URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote = true) const;

  virtual bool Full(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;

  // Highlight using the "right" 
  virtual void DocHighlight (const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const;

  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	STRING* StringBufferPtr) const;
  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING* StringBufferPtr) const;

  virtual void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING* StringBufferPtr) const;
  virtual void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING* StringBufferPtr,
	const QUERY& Query) const;

  virtual void SetMetaIgnore(const STRING& Tag);
  virtual bool IsMetaIgnoreField(const STRING& Tag, STRING *Val = NULL) const;
  virtual bool IsIgnoreMetaField(const STRING&) const { return false; };

  virtual void XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax,
	STRING *StringBuffer) const;

  virtual void Sort(RSET *Set);

  virtual ~DOCTYPE();

  virtual void SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringBufferPtr) const;
  virtual void SourceMIMEContent(PSTRING StringBufferPtr) const;

  // Start and End of Document (Head and Tail)
  void DocHead (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBufferPtr) const;
  void DocTail (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

  // hooks into the guts of the field parsers
  // used during index and presentation.
  virtual INT UnifiedNames (const STRING& Tag, PSTRLIST Value, bool Use) const;
  virtual INT     UnifiedNames (const STRING& Tag, PSTRLIST Value) const;
  virtual STRING  UnifiedName (const STRING& tag) const;
  virtual STRING  UnifiedName (const STRING& tag, PSTRING Value) const;

  virtual  STRING UnifiedNamePath (const STRING& Tag) const;

  virtual STRING& DescriptiveName(const STRING& Art,
	const STRING& FieldName, PSTRING Value) const;
  virtual STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;

  STRING LanguageField, KeyField, CategoryField, PriorityField;
  STRING DateField, DateModifiedField, DateCreatedField;
  STRING DateExpiresField, TTLField;

// protected:
  const STRING& URLencode(const STRING& Str, PSTRING Code) const;

  // Convert to HTML
  void HtmlCat (const RESULT& Result, const STRING& Input, PSTRING StringBufferPtr,
	bool Anchor) const;
  void HtmlCat (const RESULT& Result, const STRING& Input, PSTRING StringBufferPtr) const;
  void HtmlCat (const RESULT& Result, const CHR Ch, PSTRING StringBufferPtr) const;

  void HtmlCat (const STRING& Input, PSTRING StringBufferPtr,
	bool Anchor) const;
  void HtmlCat (const STRING& Input, PSTRING StringBufferPtr) const;
  void HtmlCat (const CHR Ch, PSTRING StringBufferPtr) const;

  // The Head and Tail of a HTML document
  void HtmlHead (const RESULT& ResultRecord, const STRING& ElementSet,
	STRING *StringBufferPtr) const;
  void HtmlTail (const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const;

  void XmlHead (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const;
  void XmlTail (const RESULT& ResultRecord, const STRING& ElementSet, STRING *StringBufferPtr) const;

  // SGML document
  void SgmlHead (const RESULT& ResultRecord, const STRING& ElementSet,
	STRING *StringBufferPtr) const;
  void SgmlTail (const RESULT& ResultRecord, const STRING& ElementSet,
	STRING *StringBufferPtr) const;

  // Set field type only if not already defined to something special
  bool SetFieldType(const STRING& FieldName, const FIELDTYPE FieldType);

  virtual bool IsSpecialField(const STRING &FieldName) const;
  virtual void        HandleSpecialFields(RECORD* NewRecord, const STRING& Field, const char *Buffer);
  const char *c_str() const { return Doctype.c_str(); }

protected:
   bool PluginExists(const STRING& doctype);

   bool _write_resource_path (const STRING& file,
        const RECORD& Filerecord, STRING *PathFilePath = NULL) const;

  STRING      Httpd_Content_type (const RESULT& Record, const STRING& MimeType = NulString) const;

  STRING      Getoption(const STRING& Entry, const STRING& Default = NulString) const;
  STRING      Getoption(const STRLIST * StrlistPtr, const STRING& Entry,
		const STRING& Default = NulString) const;

  FILE       *ffopen(const STRING& Filename, const char *mode) const {
	return Db ? Db->ffopen(Filename, mode) : fopen(Filename, mode);
  }
  INT         ffclose(FILE *Fp) const { return Db ? Db->ffclose(Fp) : fclose(Fp); }


  IDBOBJ     *Db;
  STRING      Doctype;
  STRING      HeadlineFmt;
  STRING      SummaryFmt;
  REGISTRY   *tagRegistry;
  STRINGINDEX WordMaximum;
private:
  CHARSET     Charset;
  HASH        Defaults;
  HASH        Unified;
  const STRLIST *DoctypeOptions;
  bool        trustKey;
};

typedef DOCTYPE* PDOCTYPE;

#ifdef O_BUILD_IB64
const int DoctypeDefVersion = (__IB_Doctype_Major_DefVerson + 0x80000); // 8 bytes
# else
const int DoctypeDefVersion = (__IB_Doctype_Major_DefVerson + 0x40000); // 4 bytes
#endif


#endif
