#ifndef GILS_HXX
#define GILS_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmltag.hxx"

class GILS :  public SGMLTAG {
public:
	GILS(PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;

	void SourceMIMEContent(PSTRING StringBuffer) const;
	~GILS();
};

typedef GILS* PGILS;

#endif
