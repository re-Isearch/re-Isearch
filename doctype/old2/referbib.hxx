/*-@@@
File:		referbib.hxx
Version:	1.00
Description:	Class REFERBIB - Refer Bibliographic records Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef REFERBIB_HXX
#define REFERBIB_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "buffer.hxx"

class REFERBIB :  public DOCTYPE {
public:
  REFERBIB(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void AddFieldDefs();
  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);
  void BeforeIndexing();
  void AfterIndexing();

  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~REFERBIB();
// hooks into the guts of the field parser
  virtual STRING UnifiedName (const STRING& tag, PSTRING Value) const;
private:
  PCHR *parse_tags(PCHR b, off_t len);
  BUFFER Buffer, tagBuffer;
};
typedef REFERBIB* PREFERBIB;


class REFERBIB_ENDNOTE :  public REFERBIB {
public:
  REFERBIB_ENDNOTE(PIDBOBJ DbParent, const STRING& Name) : REFERBIB(DbParent, Name) { }
  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~REFERBIB_ENDNOTE() {};
};

class REFERBIB_PAPYRUS :  public REFERBIB {
public:
  REFERBIB_PAPYRUS(PIDBOBJ DbParent, const STRING& Name) : REFERBIB(DbParent, Name) { }
  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~REFERBIB_PAPYRUS() {};
};



#endif
