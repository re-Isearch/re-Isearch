/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		registry.hxx
Version:	2.00
Description:	Class REGISTRY - Structured Profile Registry
@@@*/

#ifndef REGISTRY_HXX
#define REGISTRY_HXX

#include "gdt.h"
#include "string.hxx"
#include "strlist.hxx"

class REGISTRY {
  friend class METADATA;
  friend class LOCATOR;
public:
  REGISTRY(const STRING& Title);
  REGISTRY(const CHR *Title);

  GDT_BOOLEAN Empty() const;

  REGISTRY* clone() const;
  REGISTRY* Add (REGISTRY *Reg1);
  REGISTRY* Add (REGISTRY *Reg1, REGISTRY *Reg2) const;

  REGISTRY& operator=(const REGISTRY& OtherRegistry);
  void SetData(const STRLIST& Position, const STRLIST& Value);
  void AddData(const STRLIST& Position, const STRLIST& Value);
  size_t GetData(const STRLIST& Position, PSTRLIST StrlistBuffer) const;
  void SaveToFile(const STRING& FileName, const STRLIST& Position);
  size_t LoadFromFile(const STRING& FileName, const STRLIST& Position);
  size_t AddFromFile(const STRING& FileName, const STRLIST& Position);
  // Note: ProfileGetString() and ProfileWriteString() consider
  // a comma-delimited string in Entry to be a list of values, and
  // will store each as a separate node in the registry.  That is
  // the deepest level of nesting that these two functions allow.
  // For access to full nesting, I recommend using SetData()/AddData()
  // and GetData().

  void GetEntryList(const STRING& Section, STRLIST *Ptr,
	GDT_BOOLEAN Cat=GDT_FALSE) const;


  GDT_BOOLEAN ProfileGetBoolean(const STRING& Section, const STRING& Entry);
  int         ProfileGetInteger(const STRING& Section, const STRING& Entry);
  NUMBER      ProfileGetNumber(const STRING& Section, const STRING& Entry);
  STRING      ProfileGetString(const STRING& Section, const STRING& Entry);

  void ProfileGetString (const STRING& Section, const STRING& Entry,
	PSTRLIST StrlistBuffer);
  void ProfileGetString (const STRING& Section, const STRING& Entry,
	const STRING& Default, PSTRLIST StrlistBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default, PSTRING StringBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default, PPCHR CStringBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const INT Default, INT *IntBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
        const UINT Default, UINT *IntBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
        const FLOAT Default, FLOAT *FloatBuffer);
  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const DOUBLE Default, DOUBLE *DoubleBuffer);

#ifdef G_BOOL
  void ProfileGetString(const STRING& Section, const STRING& Entry,
	const GDT_BOOLEAN Default, GDT_BOOLEAN *BoolBuffer);
#endif
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const STRLIST& StringlistData);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const STRING& StringData);
#ifdef G_BOOL
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const GDT_BOOLEAN BooleanData);
#endif
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const CHR *Data);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
	const INT Data);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
        const UINT Data);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
        const long Data);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
        const unsigned long Data);
  void ProfileWriteString(const STRING& Section, const STRING& Entry,
        const DOUBLE Data);
  // Note: ProfileLoadFromFile() and ProfileAddFromFile() also parse
  // a comma-delimited list (e.g., tag=val1,val2,val3) into multiple nodes.
  size_t ProfileLoadFromFile(const STRING& FileName);
  size_t ProfileLoadFromFile(const STRING& FileName, const STRLIST& Position);

  size_t ProfileAddFromFile(const STRING& FileName, int depth=0);
  size_t ProfileAddFromFile(const STRING& FileName, const STRLIST& Position, int depth=0);

  void ProfileWrite(ostream& os, const STRING& FileName, const STRLIST& Position);

  friend ostream & operator<<(ostream& os, const REGISTRY& Registry);

  // Actually its XML
  STRING Sgml() const;
  STRING Sgml(const STRLIST& Position) const;

  // Plain Jane HTML
  STRING Html() const;
  STRING Html(const STRLIST& Position) const;

  // HTML 3.x METAs
  STRING HtmlMeta() const;
  STRING HtmlMeta(const STRLIST& Position) const;

  GDT_BOOLEAN PrintSgml(const STRING& Filename) const;
  void        PrintSgml(FILE* fp) const;
  void        PrintSgml(FILE* fp, const STRLIST& Position) const;

  GDT_BOOLEAN ReadFromSgml(const STRING& Filename);
  GDT_BOOLEAN ReadFromSgml(FILE *fp);

  size_t      AddFromSgml(FILE *fp);
  size_t      AddFromSgml(const STRING& Root, FILE *fp);
  size_t      AddFromSgml(const STRLIST& Position, FILE *fp);

  GDT_BOOLEAN ReadFromSgmlBuffer(const STRING& Buffer);
  size_t      AddFromSgmlBuffer(const STRING& Buffer);
  size_t      AddFromSgmlBuffer(const STRING& Root, const STRING& Buffer);
  size_t      AddFromSgmlBuffer(const STRLIST& Position, const STRING& Buffer);

//STRING      Value() const { return Child ? Child->Data : Data; }

  void Write(FILE *Fp) const;
  GDT_BOOLEAN Read(FILE *Fp);
  ~REGISTRY();

private:
//	enum RegFormats { UnknownFormat, IniFormat, RegFormat, SgmlFormat } Format;
  size_t ProfileAddFromFile (FILE *Fp, int depth = 0);
  size_t LoadFromFile(const STRING& FileName);
  size_t AddFromFile (FILE *Fp);
  size_t AddFromFile(const STRING& FileName);
  STRING HtmlMeta(size_t Level, size_t depth, const STRING& Tag) const;
  void Print(ostream& os, int Level) const;
  void ProfilePrint(FILE *Fp, int Level) const;
  void ProfilePrint(ostream& os, int Level) const;

  void ProfilePrintBinary(FILE *Fp, int Level) const;
  void DumpBinary (const STRING& FileName) const;

  size_t GetData(PSTRLIST StrlistBuffer) const;
  void DeleteChildren();
  const REGISTRY* FindNode(const STRING& Position) const;
  REGISTRY* GetNode(const STRING& Position);
  const REGISTRY* FindNode(const STRLIST& Position) const;
  REGISTRY* GetNode(const STRLIST& Position);
  void SetData(const STRLIST& Value);
  void AddData(const STRLIST& Value);

// Data Bits..
  STRING Data;
  REGISTRY* Next;
  REGISTRY* Child;
};

typedef REGISTRY* PREGISTRY;

void Write(const REGISTRY& Registry, FILE *Fp);
GDT_BOOLEAN Read(PREGISTRY RegistryPtr, FILE *Fp);

#endif
