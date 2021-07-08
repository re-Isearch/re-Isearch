/*-@@@
File:		yahoolist.hxx
Version:	1.00
Description:	Class YAHOOLIST - Yahoo's Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef YAHOOLIST_HXX
#define YAHOOLIST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "listdigest.hxx"

class YAHOOLIST :  public LISTDIGEST {
public:
  YAHOOLIST(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  const CHR *Seperator() const;

  virtual void ParseRecords(const RECORD& FileRecord);

  virtual PCHR        *parse_tags(PCHR b, off_t len);


  virtual void BeforeIndexing() { LISTDIGEST::BeforeIndexing(); };
  virtual void AfterIndexing()  { LISTDIGEST::AfterIndexing();  };


  ~YAHOOLIST();

private:
  BUFFER       TagBuffer;
};

#endif
