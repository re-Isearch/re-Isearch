/*

File:        soif.hxx
Version:     1
Description: Harvest SOIF records 
Author:      Peter Valkenburg
Modification: Edward C. Zimmermann
*/


#ifndef SOIF_HXX
#define SOIF_HXX

#include "bibtex.hxx"
#include "buffer.hxx"

class SOIF : public BIBTEX {
public:
  SOIF(PIDBOBJ DbParent, const STRING& Name);
  virtual const char *Description(STRLIST *List) const;
  virtual void SourceMIMEContent(STRING *StringBuffer) const;

  virtual void BeforeIndexing();
  virtual void AfterIndexing();

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);

  virtual void DocPresent (const RESULT& ResultRecord,
    const STRING& ElementSet, const STRING & RecordSyntax,
    STRING *StringBuffer) const {
	BIBTEX::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
   }


  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING & RecordSyntax, STRING *StringBuffer) const;

  virtual bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;
  virtual bool URL(const RESULT& ResultRecord, STRING *StringBuffer,
   bool OnlyRemote) const;

  ~SOIF();
private:
  BUFFER tmpBuffer;
};

typedef SOIF* PSOIF;

#endif
