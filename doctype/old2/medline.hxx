/*-@@@
File:		medline.hxx
Version:	1.00
Description:	Class MEDLINE - MEDLINE-like Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// Medline was
//  ELHILL Unit Record Format (EURF) 
//
// future is XML
//
//


#ifndef MEDLINE_HXX
#define MEDLINE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "buffer.hxx"

// Generic Medline format
class MEDLINE :  public DOCTYPE {
  friend class ISI_CIW;
  friend class MEDLINE_RIS;
public:
  typedef struct { const char *key; const char *name;} TagTable_t;

  MEDLINE(PIDBOBJ, const STRING&);
  const char *Description(PSTRLIST List) const;

  virtual void    BeforeIndexing();
  virtual void    AfterIndexing();


  void AddFieldDefs();
  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  ~MEDLINE();
  void SourceMIMEContent(PSTRING StringBuffer) const;
// hooks into the guts of the field parser
  virtual INT UnifiedNames (const STRING& Tag, PSTRLIST Value) const;
  virtual STRING& DescriptiveName(const STRING& Language,
        const STRING& FieldName, PSTRING Value) const;
  virtual STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;
  virtual PCHR *parse_subtags (PCHR b, off_t len); 
protected:
  INT UnifiedNames (const TagTable_t *Table, const size_t Elements,
        const STRING& Tag, PSTRLIST Value) const;
  STRING& DescriptiveName (const TagTable_t *Table, const size_t Elements,
	const STRING& FieldName, PSTRING Value) const;
private:
  virtual PCHR *parse_tags(PCHR b, off_t len);
  BUFFER recBuffer, tmpBuffer, tagsBuffer, subtagsBuffer;

};
typedef MEDLINE* PMEDLINE;

class MEDLINE_RIS : public MEDLINE {
public:
  MEDLINE_RIS(IDBOBJ *Db, const STRING& Name) : MEDLINE(Db, Name) {}
  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~MEDLINE_RIS() {}
};

/*
class ISI_CIW : public MEDLINE {
public:
  ISI_CIW(IDBOBJ *Db, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~ISI_CIW();
private:
  PCHR *parse_tags(PCHR b, off_t len) const;
};
*/

#endif
