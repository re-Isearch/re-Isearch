/*-@@@
File:		htmlhead.hxx
Version:	1.00
Description:	Class HTMLHEAD - Basic HTML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef HTMLHEAD_HXX
#define HTMLHEAD_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "htmlmeta.hxx"

class HTMLHEAD:public HTMLMETA 
{
public:
  HTMLHEAD (PIDBOBJ DbParent, const STRING& Name);
  ~HTMLHEAD ();
  const char *Description(PSTRLIST List) const;

  GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
        GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength) {
    return HTMLMETA::ParseWords(DataBuffer,
	DataLength, DataOffset, GpBuffer, GpLength);
  }

  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const;
  void DocPresent (const RESULT &ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present (const RESULT &ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const;
};

typedef HTMLHEAD *PHTMLHEAD;

class HTMLZERO : public HTMLHEAD {
public:
  HTMLZERO(PIDBOBJ DbParent, const STRING& Name);

  const char *Description(PSTRLIST List) const;

  GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
        GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength);
  ~HTMLZERO() { }
};


#endif
