/*-@@@
File:		solarmail.hxx
Version:	1.00
Description:	Class SOLARMAIL - Solaris 2.x Mailfolder Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef SOLARMAIL_HXX
#define SOLARMAIL_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

class SOLARMAIL :  public MAILFOLDER {
public:
	SOLARMAIL(PIDBOBJ DbParent);
	void ParseRecords (const RECORD& FileRecord);
	~SOLARMAIL();
};

typedef SOLARMAIL* PSOLARMAIL;

#endif
