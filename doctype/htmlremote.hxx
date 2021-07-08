/*@@@
File:		htmlremote.hxx
Version:	1.0
Description:	Class HTMLREMOTE - HTML documents
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

#ifndef HTMLREMOTE_HXX
#define HTMLREMOTE_HXX

#include "htmlhead.hxx"

class HTMLREMOTE : public HTMLHEAD {
public:
  HTMLREMOTE(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  GDT_BOOLEAN URL(const RESULT& ResultRecord, PSTRING StringBuffer,
   GDT_BOOLEAN OnlyRemote = GDT_TRUE) const;
  void ParseRecords(const RECORD& FileRecord);

  ~HTMLREMOTE();

private:
  STRING DocumentRoot;
  GDT_BOOLEAN KnownRoot;
};

typedef HTMLREMOTE* PHTMLREMOTE;

#endif
