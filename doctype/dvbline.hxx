/*-@@@
File:		filmline.hxx
Version:	1.00
Description:	Class DVBLINE - DVB FILMLINE v1.x Variant Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef DVBLINE_HXX
#define DVBLINE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "medline.hxx"

// Filmline v 1.x Interchange format
class DVBLINE :  public MEDLINE {
public:
  DVBLINE(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  ~DVBLINE();
  void SourceMIMEContent(PSTRING StringBuffer) const;
// hooks into the guts of the Medline field parser
  INT UnifiedNames (const STRING& Tag, PSTRLIST Value) const;
  STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;

  PCHR *parse_subtags (PCHR b, off_t len) const;
// Presentation hooks
  bool GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const;
};
typedef DVBLINE* PDVBLINE;


#endif
