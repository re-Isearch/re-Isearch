/*@@@
File:		sgmltag.hxx
Version:	1.03
Description:	Class SGMLTAG - SGML-like Document Type
Author:		Kevin Gamiel, Kevin.Gamiel@cnidr.org
Changes:	See sgmltag.cxx
@@@*/

#ifndef SGMLTAG_HXX
#define SGMLTAG_HXX

#include "defs.hxx"
#include "doctype.hxx"
#include "doc_conf.hxx"
#include "buffer.hxx"

class SGMLTAG : public DOCTYPE {
public:
  SGMLTAG(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void AfterIndexing();

  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	STRING *StringBuffer) const { DOCTYPE::Present(ResultRecord, ElementSet, StringBuffer); }
  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const;

  ~SGMLTAG();
  virtual void ParseFields(PRECORD NewRecord);
  virtual void SourceMIMEContent(PSTRING StringBuffer) const;
  virtual void SourceMIMEContent(const RESULT& Record, PSTRING StringBuffer) const;

  virtual void BeforeRset (const STRING& RecordSyntax);
  virtual char **parse_tags(char *b, off_t len);
private:
  BUFFER recBuffer, tagsBuffer;
};

typedef SGMLTAG* PSGMLTAG;

#endif
