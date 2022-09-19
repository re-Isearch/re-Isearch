/************************************************************************
************************************************************************/

/*-@@@
File:		bibcolon.hxx
Version:	1.0
Description:	Class BIBCOLON - Colon Tagged Document Type for use with bibs
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

#ifndef BIBCOLON_HXX
#define BIBCOLON_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "colondoc.hxx"
#include "buffer.hxx"

class BIBCOLON :  public COLONDOC {
public:
  BIBCOLON(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void BeforeIndexing();
  void AfterIndexing();

  void ParseRecords (const RECORD& FileRecord);
  void ParseFields(RECORD *NewRecord);
  STRING& DescriptiveName(const STRING& FieldName, STRING *Value) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const;
  void SourceMIMEContent(STRING *StringBufferPtr) const;
  ~BIBCOLON();
private:
  BUFFER tmpBuffer, tmpBuffer2;
};

typedef BIBCOLON* PBIBCOLON;

#endif
