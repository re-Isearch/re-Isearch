/*-@@@
File:		html.hxx
Version:	1.03
Description:	Class HTMLCACHE - WWW Server Cached Documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef HTMLCACHE_HXX
#define HTMLCACHE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "html.hxx"

class HTMLCACHE:public HTML
{
 public:
  HTMLCACHE (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseFields (PRECORD Record);
  void Present (const RESULT & ResultRecord, const STRING & ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
   ~HTMLCACHE ();
 private:
   STRING CacheMnt; // Cache mount point
};

typedef HTMLCACHE *PHTMLCACHE;

#endif
