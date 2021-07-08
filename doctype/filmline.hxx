/*-@@@
File:		filmline.hxx
Version:	1.00
Description:	Class FILMLINE - FILMLINE v1.x Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef FILMLINE_HXX
#define FILMLINE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "medline.hxx"

// Filmline v 1.x Interchange format
class FILMLINE :  public MEDLINE {
public:
	FILMLINE(PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;

	void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		const STRING & RecordSyntax, PSTRING StringBuffer) const;
	~FILMLINE();
        void SourceMIMEContent(PSTRING StringBuffer) const;
// hooks into the guts of the Medline field parser
	INT UnifiedNames (const STRING& tag, PSTRLIST Value) const;
	STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;
};
typedef FILMLINE* PFILMLINE;


#endif
