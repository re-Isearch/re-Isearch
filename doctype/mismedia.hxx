/*-@@@
File:		mismedia.hxx
Version:	1.00
Description:	Class MISMEDIA - Colon Tagged Document Type for LMZ
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MISMEDIA_HXX
#define MISMEDIA_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "registry.hxx"
#include "colongrp.hxx"

class MISMEDIA :  public COLONGRP {
public:
  MISMEDIA(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  void SourceMIMEContent(const RESULT& ResultRecord, PSTRING StringPtr) const;
  ~MISMEDIA();

private:
  STRING& Standort(const char *Code, PSTRING Value) const;
  void DefineField(const STRING& Name, const STRING& Default, PSTRING Value);
  STRING StandortsURL, Title, DateOfIssue, Keywords;
  STRLIST EmbedFields;
};

typedef MISMEDIA* PMISMEDIA;

#endif
