/************************************************************************
************************************************************************/

/*@@@
File:		pdfdoc.hxx
Version:	1.00
Description:	Class PDFDOC - Portable Document Format
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

#ifndef PDFDOC_HXX
#define PDFDOC_HXX

#include "defs.hxx"
#include "doctype.hxx"
#include "colondoc.hxx"

class PDFDOC : public METADOC {
public:
  PDFDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void LoadFieldTable();

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);

  NUMERICOBJ ParseComputed(const STRING& FieldName, const STRING& Buffer) const;

  void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  bool GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const;

  ~PDFDOC();

private:
  void parsePDFinfo(const STRING& FileName, LOCATOR *Locator);

  STRING pdfinfo_Command;
  STRING pdf2text_Command;
  STRING pdf2jpeg_Command;
  STRING pdf2pdf_Command;

};

typedef PDFDOC* PPDFDOC;

#endif
