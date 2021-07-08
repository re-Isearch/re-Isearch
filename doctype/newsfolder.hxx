/*-@@@
File:		newsfolder.hxx
Version:	1.00
Description:	Class NEWSFOLDER - RN News folder Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef NEWSFOLDER_HXX
#define NEWSFOLDER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

class NEWSFOLDER :  public MAILFOLDER {
public:
	NEWSFOLDER(PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;

	void SourceMIMEContent(PSTRING StringBuffer) const;
	~NEWSFOLDER();
};

typedef NEWSFOLDER* PNEWSFOLDER;

#endif
