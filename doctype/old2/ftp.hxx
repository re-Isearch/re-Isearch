/*-@@@
File:		ftp.hxx
Version:	0.01
Description:	Class FTP - Binary files
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef FTP_HXX
#define FTP_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "iafadoc.hxx"

class FTP:public IAFADOC
{
 public:
  FTP (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD FileRecord);
  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SourceMIMEContent(const RESULT& ResultRecord, 
	PSTRING StringPtr) const;
   ~FTP ();
 private:
  STRING PostFix;
};

typedef FTP *PFTP;

#endif
