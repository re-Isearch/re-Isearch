/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/

namespace Syntax {
typedef enum {
  UndefinedRecordSyntax,
  DefaultRecordSyntax, // Unknown
  UnimarcRecordSyntax,
  IntermarcRecordSyntax,
  CCFRecordSyntax,
  USmarcRecordSyntax,
  UKmarcRecordSyntax,
  NormarcRecordSyntax,
  LibrismarcRecordSyntax,
  DanmarcRecordSyntax,
  FinmarcRecordSyntax,
  MABRecordSyntax,
  CanmarcRecordSyntax,
  SBNRecordSyntax,
  PicamarcRecordSyntax,
  AusmarcRecordSyntax,
  IbermarcRecordSyntax,
  CatmarcRecordSyntax,
  MalmarcRecordSyntax,
  ExplainRecordSyntax,
  SUTRSRecordSyntax,
  OPACRecordSyntax,
  SummaryRecordSyntax,
  GRS0RecordSyntax,
  GRS1RecordSyntax,
  ESTaskPackageRecordSyntax,
  fragmentRecordSyntax,
  PDFRecordSyntax,
  postscriptRecordSyntax,
  HtmlRecordSyntax,
  TiffRecordSyntax,
  jpegTDRecordSyntax,
  pngRecordSyntax,
  mpegRecordSyntax,
  sgmlRecordSyntax,
  XmlRecordSyntax,
  applicationXMLRecordSyntax,
  tiffBRecordSyntax,
  wavRecordSyntax,
  SQLRSRecordSyntax,
  Latin1RecordSyntax,
  PostscriptRecordSyntax,
  CXFRecordSyntax,
  SgmlRecordSyntax,
  ADFRecordSyntax,
  // Private
  DVBHtmlRecordSyntax, // DVB Private
  TextHighlightRecordSyntax,
  HTMLHighlightRecordSyntax,
  XMLHighlightRecordSyntax
} RecordSyntax_t;

#if 0
class RecordSyntax
{
public:
  RecordSyntax() { id = DefaultRecordSyntax;} 
  RecordSyntax(const STRING& Name) { id = RecordSyntaxID(Name); } 
  RecordSyntax(const RecordSyntax& Other) { id = Other.id; }

  bool  Equals(const RecordSyntax& Other) const { id == Other.id; }
private:
  RecordSyntax_t id;
}
#endif

} // Namespace

// Convert String xxx.xxxx.xxx -> Internal OID
Syntax::RecordSyntax_t RecordSyntaxID(const STRING& String);
// Convert Internal OID to xxx.xxx.xxx string
const char    *ID2RecordSyntax(Syntax::RecordSyntax_t);

// Convert String to cannonical xxx.xxxx.xxx 
const char *RecordSyntaxCannonical(const STRING& String,  Syntax::RecordSyntax_t Default);

// Is the Id valid?
bool   RegisteredRecordSyntaxID(int Id);


class RecordSyntax
{
public:
  RecordSyntax() { id = Syntax::DefaultRecordSyntax;}
  RecordSyntax(Syntax::RecordSyntax_t other) { id = other; }
  RecordSyntax(const char *Name)    { id = RecordSyntaxID(Name); }
  RecordSyntax(const STRING& Name) { id = RecordSyntaxID(Name); }
  RecordSyntax(const RecordSyntax& Other) { id = Other.id; }


  bool        Ok() const { return RegisteredRecordSyntaxID(id); }
  bool        Equals(const RecordSyntax& Other) const { return id == Other.id; }
  const char *OID() const { return  ID2RecordSyntax( id ); }

  Syntax::RecordSyntax_t id;
} ;

  inline bool operator==(const RecordSyntax& s1, const RecordSyntax& s2) { return s1.Equals(s2); }
  inline bool operator==(const RecordSyntax& s1, const STRING& s2)       { return RecordSyntaxID(s2) == s1.id; }
  inline bool operator==(const STRING& s1, const RecordSyntax& s2)       { return RecordSyntaxID(s1) == s2.id; }

  inline bool operator!=(const RecordSyntax& s1, const RecordSyntax& s2) { return !s1.Equals(s2); }
  inline bool operator!=(const RecordSyntax& s1, const STRING& s2)       { return RecordSyntaxID(s2) != s1.id; }
  inline bool operator!=(const STRING& s1, const RecordSyntax& s2)       { return RecordSyntaxID(s1) != s2.id; }



