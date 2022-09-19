/*-@@@
File:		html.hxx
Version:	1.03
Description:	Class HTML - WWW HTML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef HTML_HXX
#define HTML_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "sgmlnorm.hxx"
#include "buffer.hxx"

#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0==> CNIDR's Isearch 1==> BSn's */
#endif

#ifndef DEFAULT_HTML_LEVEL 
# define DEFAULT_HTML_LEVEL	3
#endif

class HTML:public SGMLNORM
{
  public:
  HTML (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void BeforeIndexing();
  void AfterIndexing();

  void AddFieldDefs();
  void ParseRecords (const RECORD & FileRecord);
  void ParseFields (PRECORD NewRecord);

  void BeforeSearching(QUERY* QueryPtr);

  STRING UnifiedName (const STRING& Tag, PSTRING Value) const;

  GDT_BOOLEAN Summary(const RESULT& ResultRecord,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

  GDT_BOOLEAN URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	GDT_BOOLEAN OnlyRemote) const;

  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet, 
		const STRING& RecordSyntax, STRING* StringBufferPtr) const;
  void Present (const RESULT &ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const;
  ~HTML ();
private:
  INT HTML_Level;
  BUFFER tmpBuffer, tmpBuffer2;
//HTMLEntities Entities;
};
typedef HTML *PHTML;

#endif
