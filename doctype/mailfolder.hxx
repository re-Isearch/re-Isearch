/*-@@@
File:		mailfolder.hxx
Version:	1.00
Description:	Class MAILFOLDER - Mail Folder Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MAILFOLDER_HXX
#define MAILFOLDER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "ptext.hxx"
#endif
#include "buffer.hxx"

class MAILFOLDER :  public PTEXT {
public:
  MAILFOLDER(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void LoadFieldTable();
  void AddFieldDefs();

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);

  virtual NUMERICOBJ ParseComputed(const STRING& FieldName, const STRING& Buffer) const;

  virtual void BeforeIndexing();
  virtual void AfterIndexing();

  void Present(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

  ~MAILFOLDER();

  INT GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length);
  INT ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const;
  INT GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const;

  void SourceMIMEContent(PSTRING StringBuffer) const;
  virtual STRING& DescriptiveName(const STRING& Language,
	const STRING& FieldName, PSTRING Value) const;
  // To Manipulate the "acceptable" mail fields
  virtual bool accept_tag(const PCHR tag) const;
  // For children
  virtual const CHR *Seperator() const;

  virtual PCHR        *parse_tags(PCHR b, off_t len);


protected:
  // Utility functions
  bool IsMailFromLine(const char *line) const;
  bool IsNewsLine(const char *line) const;
  PCHR NameKey(PCHR buf, bool wantName) const;
  INT EncodeKey (STRING *Key, const CHR *line, size_t val_len) const;
  void Mailto (const STRING &Value, PSTRING StringBufferPtr) const;
  void Mailto (const STRING &Value, const STRING &Subject, PSTRING StringBufferPtr) const;
  void Mailto (const STRING &Value, const STRING &Subject,
	const STRING &Reference, PSTRING StringBufferPtr) const;
  void Thread (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

private:

  bool  loadFieldTable;
  bool  RestrictFields;
  bool  ParseMessageStructure;
  BUFFER       TagBuffer;
  BUFFER       tempBuffer;
};

typedef MAILFOLDER* PMAILFOLDER;

#endif
