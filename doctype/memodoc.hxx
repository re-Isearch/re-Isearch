/*-@@@
File:		memodoc.hxx
Version:	1.00
Description:	Class MEMODOC - Colon Tagged Memo Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MEMODOC_HXX
#define MEMODOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "ptext.hxx"
#endif
#include "buffer.hxx"

class MEMODOC :  public PTEXT {
public:
  MEMODOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  bool IsIgnoreMetaField(const STRING& Fieldname);

  FIELDTYPE GuessFieldType(const STRING& FieldName, const STRING& Contents);

  INT UnifiedNames (const STRING& tag, PSTRLIST Value) const
    { return PTEXT::UnifiedNames(tag, Value); }

  virtual void    BeforeIndexing();
  virtual void    AfterIndexing();

  void AddFieldDefs();
  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SetParseMessageStructure(bool newVal=true) {
    ParseMessageStructure = newVal;
  }

  // To be used only by children
  size_t       CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn) const;

~MEMODOC();

private:
  size_t       CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn, int level) const;
  BUFFER       tmpBuffer, tagBuffer;
  STRING       help_text;
  PCHR        *parse_tags(PCHR b, off_t len);
  bool  ParseMessageStructure;
  bool  autoFieldTypes;
  unsigned     recordsAdded;
};

typedef MEMODOC* PMEMODOC;

// Private for services
class _VMEMODOC : public MEMODOC {
public:
  _VMEMODOC(PIDBOBJ DbParent, const STRING& Name);

  bool   GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;
  void          ParseRecords(const RECORD& FileRecord);

  virtual off_t RunPipe(FILE *fp, const STRING& Source);


  void          Present(const RESULT& ResultRecord, const STRING& ElementSet,
			const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void          DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
			const STRING& RecordSyntax, PSTRING StringBuffer) const;

  STRING        SOURCE_PATH_ELEMENT;
  const char   *out_ext;
  STRING        Filter;
};


#endif
