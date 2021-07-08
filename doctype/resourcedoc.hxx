/*-@@@
File:		roadsdoc.hxx
Version:	1.00
Description:	Class RESOURCEDOC - Colon Tagged (IAFA-like) Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef RESOURCEDOC_HXX
#define RESOURCEDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "roadsdoc.hxx"

class RESOURCEDOC :  public ROADSDOC {
public:
  RESOURCEDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void DocPresent (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  void SourceMIMEContent(const RESULT& ResultRecord, PSTRING StringPtr) const;
  STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;

  ~RESOURCEDOC();
};
typedef RESOURCEDOC* PRESOURCEDOC;

#endif
