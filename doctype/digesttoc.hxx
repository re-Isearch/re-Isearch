/*-@@@
File:		digesttoc.hxx
Version:	1.00
Description:	Class DIGESTTOC - Internet Mail Digest Table of Contents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef DIGESTTOC_HXX
#define DIGESTTOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "maildigest.hxx"

class DIGESTTOC :  public MAILDIGEST {
public:
	DIGESTTOC(PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;

	void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
        void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
	void SourceMIMEContent(PSTRING StringBuffer) const;
	const CHR *Seperator() const;
	~DIGESTTOC();
};
typedef DIGESTTOC* PDIGESTTOC;

#endif
