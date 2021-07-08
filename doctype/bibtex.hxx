/*

File:        bibtex.hxx
Version:     1
Description: class BIBTEX - index BibTEX files
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef BIBTEX_HXX
#define BIBTEX_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif
#include "buffer.hxx"

class BIBTEX : public DOCTYPE {
public:
   BIBTEX(PIDBOBJ DbParent, const STRING& Name);
   virtual const char *Description(PSTRLIST List) const;

  virtual void BeforeIndexing();
  virtual void AfterIndexing();

   virtual void ParseRecords(const RECORD& FileRecord);
   virtual void ParseFields(PRECORD NewRecord);
   virtual void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING & RecordSyntax, PSTRING StringBuffer) const;
   virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING & RecordSyntax, PSTRING StringBuffer) const;

   virtual void SourceMIMEContent(PSTRING StringBuffer) const;
   ~BIBTEX();
private:
   BUFFER recBuffer, tmpBuffer;
};

typedef BIBTEX* PBIBTEX;

#endif
