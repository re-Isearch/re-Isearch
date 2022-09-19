/*@@@
File:		htmlmeta.hxx
Version:	1.0
Description:	Class HTMLMETA - HTML documents, <HEAD> only
Author:         Edward C. Zimmermann <edz@nonmonotonic.net>
@@@*/

#ifndef HTMLMETA_HXX
#define HTMLMETA_HXX

#include "defs.hxx"
#include "colondoc.hxx"
#include "HTMLEntities.hxx"
#include "buffer.hxx"

class HTMLMETA : public COLONDOC {
public:
  HTMLMETA(PIDBOBJ DbParent, const STRING& Name);

  void SourceMIMEContent(PSTRING stringPtr) const;
  void SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const;
  const char *Description(PSTRLIST List) const;

  GDT_BOOLEAN Summary(const RESULT& ResultRecord,
        const STRING& RecordSyntax, PSTRING StringBuffer) const;

  GDT_BOOLEAN URL(const RESULT& ResultRecord, PSTRING StringBuffer,
        GDT_BOOLEAN OnlyRemote) const;

  void LoadFieldTable();

  void BeforeIndexing();
  void AfterIndexing();

  SRCH_DATE  ParseDate(const STRING& Buffer, const STRING& FieldName) const;
  NUMERICOBJ ParseComputed(const STRING& FieldName, const STRING& Buffer) const;

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);
  GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
	GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength);

  INT GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length);
  INT ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;

  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING* StringBufferPtr) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

  void XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax,
	STRING *StringBuffer) const;

  STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;

  ~HTMLMETA();

private:
  size_t CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn, off_t Start =0, off_t End = 0) const;
  int    TagMatch(char* tag, const char* tagType, size_t len) const;
  int    TagMatch(char* tag, const char* tagType) const;
  int    fieldTableLoaded;  

  BUFFER       tagBuffer;
  STRING       defaultDirMetadata;
  HTMLEntities Entities;
};

typedef HTMLMETA* PHTMLMETA;

#endif
