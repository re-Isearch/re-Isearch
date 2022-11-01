/*-@@@
File:		colondoc.hxx
Version:	1.00
Description:	Class METADOC - Colon Tagged Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef METADOC_HXX
#define METADOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "buffer.hxx"

class METADOC :  public DOCTYPE {
  friend class COLONDOC;
  friend class DIALOGB;
public:
  METADOC(PIDBOBJ DbParent, const STRING& Name);
  virtual const char  *Description(PSTRLIST List) const;

  virtual void    LoadFieldTable();

  virtual void    BeforeIndexing();
  virtual void    AfterIndexing();

  virtual FIELDTYPE GuessFieldType(const STRING& FieldName, const STRING& Contents);
  virtual FIELDTYPE CheckFieldType(const STRING& FieldName);

  virtual void    AddFieldDefs();
  virtual void    ParseRecords(const RECORD& FileRecord);
  virtual void    ParseFields(PRECORD NewRecord);

  virtual SRCH_DATE  ParseDate(const STRING& Buffer, const STRING& FieldName) const;
  virtual NUMERICOBJ ParseComputed(const STRING& FieldName, const STRING& Buffer) const;

  virtual GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
        GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength);

  virtual STRING& DescriptiveName(const STRING& FieldName, PSTRING Value) const;
  virtual void    DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const;
  virtual void    Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const;
  virtual void    SourceMIMEContent(PSTRING StringBufferPtr) const;
  virtual void    SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const;

  ~METADOC();

protected:
  void            SetPresentStyle(bool useTable);
  PCHR           *parse_tags(PCHR b, off_t len);
  bool     GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const;
  void            SetSepChar(const STRING& Default);

private:
  void            SetPresentType(const STRING& Default);
  BUFFER          tmpBuffer, tagBuffer;
  bool     allowWhiteBeforeField;
  bool     UseTable;
  bool     IgnoreTagWords;
  CHR             sepChar;
  bool     autoFieldTypes;
  unsigned        recordsAdded;
};

typedef METADOC* PMETADOC;

#define COLONDOC_HXX 1

class COLONDOC : public METADOC {
public:
  COLONDOC(PIDBOBJ DbParent, const STRING& Name);
  virtual const char *Description(PSTRLIST List) const;
  virtual void        SourceMIMEContent(PSTRING StringBufferPtr) const;

  virtual void    BeforeIndexing() { METADOC::BeforeIndexing(); }
  virtual void    AfterIndexing()  { METADOC::AfterIndexing();  }

  virtual void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, STRING *StringBuffer) const {
    METADOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  }
  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, STRING *StringBuffer) const {
    METADOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  }

  ~COLONDOC();
};

typedef COLONDOC* PCOLONDOC;


class DIALOGB : public METADOC {
public:
  DIALOGB(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  void        SourceMIMEContent(PSTRING StringBufferPtr) const;
  void        ParseRecords(const RECORD& FileRecord);
  ~DIALOGB();
};

typedef DIALOGB* PDIALOGB;

#endif
