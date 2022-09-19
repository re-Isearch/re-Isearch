/*-@@@
File:		euromedia.hxx
Version:	1.00
Description:	Class EUROMEDIA - Colon Tagged Document Type for EURO
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef EUROMEDIA_HXX
#define EUROMEDIA_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "registry.hxx"
#include "colongrp.hxx"

class EUROMEDIA :  public COLONGRP {
public:
  EUROMEDIA(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  void SourceMIMEContent(const RESULT& ResultRecord, PSTRING StringPtr) const;
  ~EUROMEDIA();

  virtual STRING& DescriptiveName(const STRING& Language,
	const STRING& FieldName, PSTRING Value) const;

  virtual STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const {
    return METADOC::DescriptiveName(FieldName, Value);
  }
};

typedef EUROMEDIA* PEUROMEDIA;

#endif
