/*-@@@
File:		firstline.hxx
Version:	1.00
Description:	Class FIRSTLINE - Text Document Type, first line is headline
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/
#ifndef FIRSTLINE_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class FIRSTLINE : public DOCTYPE {
public:
  FIRSTLINE(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  virtual void ParseFields(PRECORD NewRecord);
  virtual void Present (const RESULT& ResultRecord,
		const STRING& ElementSet, STRING *StringBufferPtr) const;
  virtual ~FIRSTLINE();
private:
  int StartLine;
};

typedef FIRSTLINE* PFIRSTLINE;

#endif
