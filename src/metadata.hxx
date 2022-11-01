/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:		metadata.hxx
Version:	1.00
Description:	Class METADATA - Structured Metadata Registry
Author:		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#ifndef METADATA_HXX
#define METADATA_HXX

#include "registry.hxx"
#include "lang-codes.hxx"
#include "idbobj.hxx"
#include "strstack.hxx"

class METADATA {
public:
  METADATA();
  METADATA(const STRING& MdType);
  METADATA(const STRING& MdType, const STRING& DefaultsPath);

  METADATA& operator=(const METADATA& OtherMetadata);

  bool SetCharset(const CHARSET& NewCharset);

  REGISTRY     *Metadata() const { return mdRegistry; };
  const STRING& MdType() const { return mdType; };

  operator STRING() const;

  METADATA *clone() const;

  void SetData(const STRLIST& Position, const STRLIST& Value);
  void AddData(const STRLIST& Position, const STRLIST& Value);
  size_t GetData(const STRLIST& Position, PSTRLIST StrlistBuffer) const;
  void Clear();
  void Clear(const STRLIST& Position);

  void Print(FILE *fp) const;
  void Print(FILE *fp, const STRLIST& Position) const;
  void Print(const STRING& Filename) const;
  void Print(const STRING& Filename, const STRLIST& Position) const;
  void Append(FILE *fp) const;
  void Append(FILE *fp, const STRLIST& Position) const;
  void Append(const STRING& Filename) const;
  void Append(const STRING& Filename, const STRLIST& Position) const;

  // From base
  STRING Text() const;
  STRING Html() const;
  STRING HtmlMeta() const;
  STRING Xml()  const;

  // From Position
  STRING Text(const STRLIST& Position) const;
  STRING Html(const STRLIST& Position) const;
  STRING HtmlMeta(const STRLIST& Position) const;
  STRING Xml(const STRLIST& Position)  const;

  STRING HtmlMeta(const REGISTRY *r) const;
  void Write(PFILE Fp) const;
  bool Read(FILE *Fp);
  bool Add(FILE *Fp);

  ~METADATA();
private:
  bool Load (const STRING& MdType);
  bool Load (const STRING& MdType, const STRING& DefaultsPath);
  STRING Text(const REGISTRY* r, STRSTACK *Stack = NULL) const;
  STRING HtmlMeta(REGISTRY *r, size_t level, size_t depth, const STRING& Tag) const;

  CHARSET   Charset;
  STRING    mdType;
  REGISTRY *mdRegistry;
};

/*
// Fill these from LDAP?
class METAPERSON {
public:
  STRING Title;
  STRING FirstName, LastName;
  STRING Organization;
  STRING StreetAddress;
  STRING City;
  STRING Country;
  STRING PostalCode;
  STRING EmailAddress;
  STRING WebURL;
  STRING Telephone;
  STRING Fax;
  STRING HoursOfService;
  STRING NetworkAddress;
}
*/

class LOCATOR {
public:
  LOCATOR();
  LOCATOR(const STRING& DefaultsPath);
  LOCATOR(const LOCATOR& OtherLocator);
  LOCATOR(const METADATA& DefaultMetadata);
  ~LOCATOR();

  operator STRING() const { return (STRING)*Metadata;}

  STRING Text() const     { return Metadata->Text();     }
  STRING Xml() const      { return Metadata->Xml();      }
  STRING Html() const     { return Metadata->Html();     }
  STRING HtmlMeta() const { return Metadata->HtmlMeta(); }

  STRING Text(const STRLIST& Position) const     { return Metadata->Text(Position);     }
  STRING Xml(const STRLIST& Position) const      { return Metadata->Xml(Position);      }
  STRING Html(const STRLIST& Position) const     { return Metadata->Html(Position);     }
  STRING HtmlMeta(const STRLIST& Position) const { return Metadata->HtmlMeta(Position); }

  void Print(FILE *Fp) const {
	Metadata->Print(Fp); }
  void Print(FILE *Fp, const STRLIST& Position) const {
	Metadata->Print(Fp, Position); }
  void Print(const STRING& Filename) const {
	Metadata->Print(Filename); }
  void Print(const STRING& Filename, const STRLIST& Position) const {
	Metadata->Print(Filename, Position); }

  void Append(FILE *Fp) const {Metadata->Append(Fp); }
  void Append(const STRING& Filename) const {Metadata->Append(Filename); }

  bool IsEmpty(const STRING& Name) const;

  // Set the bits in the Attribute set
  void Set(const STRING& Name, const STRING& Value);
  void Set(const STRLIST& Position, const STRING& Value);
  void Set(const STRLIST& Position, const STRLIST& Value);
  void Add(const STRING& Name, const STRING& Value);
  void Add(const STRLIST& Position, const STRING& Value);
  void Add(const STRLIST& Position, const STRLIST& Value);

  STRING Get(const STRING& Name) const;
  STRING Get(const STRLIST& Position) const;
  size_t Get(const STRING& Name, STRLIST *Buffer) const;
  size_t Get(const STRLIST& Position, STRLIST *Buffer) const;
  
  void SetTitle(const STRING& Title)       { Set("title", Title);                          }
  void SetAbstract(const STRING& Abstract) { Set("abstract", Abstract);                    }
  void SetLanguage(const SHORT   Language) { Set("Language-of-Record", Id2Lang(Language)); }
  void SetLanguage(const STRING& Language) { Set("Language-of-Record", Language);          }
private:
  METADATA *Metadata;
};

typedef METADATA* PMETADATA;

void Write(const METADATA& Registry, PFILE Fp);
bool Read(PMETADATA RegistryPtr, PFILE Fp);

#endif
