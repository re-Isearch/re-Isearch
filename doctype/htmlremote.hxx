/*@@@
File:		htmlremote.hxx
Version:	1.0
Description:	Class HTMLREMOTE - HTML documents
@@@*/

#ifndef HTMLREMOTE_HXX
#define HTMLREMOTE_HXX

#include "htmlhead.hxx"

class HTMLREMOTE : public HTMLHEAD {
public:
  HTMLREMOTE(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  bool        URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote = true) const;
  void       ParseRecords(const RECORD& FileRecord);

  ~HTMLREMOTE();

private:
  STRING DocumentRoot;
  bool   KnownRoot;
};

typedef HTMLREMOTE* PHTMLREMOTE;

#endif
