/*-@@@
File:		xml.hxx
Version:	1.00
Description:	Class XML - Basic XML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef XML_HXX
#define XML_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmlnorm.hxx"

class XML:public SGMLNORM
{
public:
  XML (PIDBOBJ DbParent, const STRING& Name);
  ~XML ();
  virtual const char *Description(PSTRLIST List) const;

  virtual void SourceMIMEContent(PSTRING StringBuffer) const;
  virtual void SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const;

  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const {
    return SGMLNORM::Present(ResultRecord, ElementSet, StringBuffer);
  }

  virtual void Present (const RESULT &ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const;

  virtual void ParseRecords(const RECORD& FileRecord);

  // Parser hook
  virtual bool StoreTagComplexAttributes(const char *tag_ptr) const;


  /* XML is now case dependent */
  virtual const PCHR find_end_tag (const char *const *t, const char *tag, size_t *offset=NULL) const;
};

typedef XML *PXML;

#endif
