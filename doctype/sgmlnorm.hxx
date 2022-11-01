/*-@@@
File:		sgmlnorm.hxx
Version:	1.00
Description:	Class SGMLNORM - Normalized SGML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef SGMLNORM_HXX
#define SGMLNORM_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "HTMLEntities.hxx"
#include "buffer.hxx"
#include "words.hxx"

class SGMLNORM:public DOCTYPE
{
public:
  SGMLNORM (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseRecords (const RECORD & FileRecord);

  void BeforeIndexing();
  void AfterIndexing();

  void AddFieldDefs();
  void ParseFields (PRECORD NewRecord);

  virtual size_t ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const;

  virtual STRING      ParseBuffer(const STRING& Buffer) const;
  virtual SRCH_DATE   ParseDate(const STRING& Buffer) const;
  virtual SRCH_DATE   ParseDate(const STRING& Buffer, const STRING& FieldName) const;
  virtual DATERANGE   ParseDateRange(const STRING& Buffer) const;
  virtual NUMERICOBJ  ParsePhonhash(const STRING& Buffer) const;
  virtual NUMERICOBJ  ParseNumeric(const STRING& Buffer) const;
  virtual bool ParseRange(const STRING& Buffer, const STRING& FieldName,
        DOUBLE* fStart, DOUBLE* fEnd) const;
  virtual int         ParseGPoly(const STRING& Buffer, GPOLYFLD* gpoly) const;
  virtual NUMERICOBJ  ParseComputed(const STRING& FieldName, const STRING& Buffer) const;
  virtual long        ParseCategory(const STRING& Buffer) const;
  virtual MONETARYOBJ ParseCurrency(const STRING& FieldName, const STRING& Buffer) const;


  GPTYPE ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
	GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength);

  INT GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length);
  INT ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;

  virtual void XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax,
	STRING *StringBuffer) const;
  virtual void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& REcordSyntax, PSTRING StringBuffer) const;

  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	PSTRING StringBuffer) const {
    return DOCTYPE::Present(ResultRecord, ElementSet, StringBuffer);
  }
  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& REcordSyntax, PSTRING StringBuffer) const;

  ~SGMLNORM ();
  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SourceMIMEContent(const RESULT& ResultRecord, 
	PSTRING StringPtr) const;
// hooks into the guts of the field parser
  virtual STRING UnifiedName (const STRING& Tag, PSTRING Value) const;
  virtual STRING UnifiedName (const STRING& DTD, const STRING& Tag, PSTRING Value) const;
/* SGML helper functions */
   virtual PCHR *parse_tags (register PCHR b, const off_t len);
   virtual const PCHR find_end_tag (const char *const *t, const char *tag, size_t *offset=NULL) const;

   // Parser hook
   virtual bool StoreTagComplexAttributes(const char *tag_ptr) const { return false; }

   void store_attributes (PDFT pdft, PCHR base_ptr, PCHR tag_ptr,
	bool UseHTML=false, STRING *Key=NULL, SRCH_DATE *Datum = NULL) const;

   void SetStoreComplexAttributes(bool x) { StoreComplexAttributes = x; }
protected:
   void ExtractDTD(const STRING& Decl, PSTRING Dtd) const;
private:
   STRING _cleanBuffer(const STRING& Buffer) const;
   const char *_cleanBuffer(char *DataBuffer, size_t DataLength) const;

   bool StoreComplexAttributes;
   bool IgnoreTagWords;
   STRING  Headline, DTD;
   STRLIST EmptyTagList; // List of EMPTY tags we have seen
   HTMLEntities Entities;
   BUFFER tmpBuffer, tmpBuffer2;
   BUFFER tagsBuffer;
};

typedef SGMLNORM *PSGMLNORM;

#endif
