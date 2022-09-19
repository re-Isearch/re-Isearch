/*-@@@
File:		iafadoc.hxx
Version:	1.00
Description:	Class IAFADOC - Colon Tagged (IAFA-like) Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef IAFADOC_HXX
#define IAFADOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "colondoc.hxx"

class IAFADOC :  public COLONDOC {
public:
	IAFADOC(PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;

	void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
	void SourceMIMEContent(PSTRING StringBufferPtr) const;
	void SourceMIMEContent(const RESULT& ResultRecord,
		PSTRING StringPtr) const;
	~IAFADOC();
};
typedef IAFADOC* PIAFADOC;

#endif
