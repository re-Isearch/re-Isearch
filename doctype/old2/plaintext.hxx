#ifndef PLAINTEXT_HXX
#define PLAINTEXT_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "plaintext.hxx"

class PLAINTEXT :  public DOCTYPE {
public:
	PLAINTEXT (PIDBOBJ DbParent, const STRING& Name);
	const char *Description(PSTRLIST List) const;
	~PLAINTEXT();
};

typedef PLAINTEXT* PPLAINTEXT;

#endif
