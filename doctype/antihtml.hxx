/*-@@@
File:		antihtml.hxx
Version:	1.03
Description:	Class ANTIHTML - WWW ANTIHTML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef ANTIHTML_HXX
#define ANTIHTML_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "registry.hxx"
#include "html.hxx"

class ANTIHTML:public HTML
{
  public:
  ANTIHTML (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void BeforeIndexing();
  void AfterIndexing();

  void AddFieldDefs();
  void ParseFields (PRECORD NewRecord);
  ~ANTIHTML ();

  STRING UnifiedName (const STRING& Tag, PSTRING Value) const;
private:
//  REGISTRY *TagRegistry;
//  bool useBuiltin;
  BUFFER tmpBuffer, tmpBuffer2;
};

typedef ANTIHTML *PANTIHTML;

#endif
