/************************************************************************
************************************************************************/

/*-@@@
File:		iknowdoc.hxx
Version:	1.0
Description:	Class IKNOWDOC - Colon Tagged Document Type for use with Iknow
Author:		Tim Gemma, stone@cnidr.org
@@@-*/

#ifndef IKNOWDOC_HXX
#define IKNOWDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "colondoc.hxx"
#include "buffer.hxx"

class IKNOWDOC :  public COLONDOC {
public:
  IKNOWDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void ParseRecords (const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);
  STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;
  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  ~IKNOWDOC();
private:
  BUFFER tmpBuffer, tagBuffer;
};

typedef IKNOWDOC* PIKNOWDOC;

#endif
