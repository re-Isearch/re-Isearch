/*-@@@
File:		gilsxml.hxx
Version:	1.00
Description:	Class GILS XML - Basic GILS XML Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef GILSXML_HXX
#define GILSXML_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "xml.hxx"
#include "buffer.hxx"

class XMLBASE:public XML 
{
friend class GILSXML;
friend class RSS2;
friend class NEWSML;
friend class XMLREC;
friend class RDFREC;
public:
  XMLBASE (PIDBOBJ DbParent, const STRING& Name);

  const char *Description(PSTRLIST List) const;

  void ParseFields (PRECORD NewRecord);

  // Parser hook
  bool StoreTagComplexAttributes(const char *tag_ptr) const;

  void Present (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const {
    return XML::Present(ResultRecord, ElementSet, StringBuffer);
  }
  void Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& Syntax,
	PSTRING StringBuffer) const {
    return XML::Present(ResultRecord, ElementSet, StringBuffer);
  }

  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;

  virtual void ParseRecords(const RECORD& FileRecord);

  // Lowest nodes
  bool GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const;

  // STRING GetOption(const STRING& Option, const STRING& defaultValue = NulString);

  ~XMLBASE ();

private:
  char   *_get_value(char *Buffer, size_t val_len);
  BUFFER tempBuffer, readBuffer;
  STRING typeKeyword;
  STRING TTLfieldName;
  char   levelCh;
};

class XML_Element
{
public:
  XML_Element()                         { start = 0; end = 0; }
  XML_Element(const STRING& Tag)        { tag = Tag; } 
  void    set_tag(const STRING NewTag)  { tag = NewTag; }
  STRING  get_tag() const               { return tag; }
  void    set_start(const INT NewStart) { start = NewStart; }
  size_t  get_start() const             { return start; }
  void    set_end(const size_t NewEnd)  { end = NewEnd; }   
  size_t  get_end() const               { return end; }
  operator FC() const                   { return FC(start,end); }
private:
  STRING  tag;
  size_t  start;
  size_t  end;
};



class GILSXML:public XMLBASE
{
public:
  GILSXML (PIDBOBJ DbParent, const STRING& Name);
  ~GILSXML ();

  const char *Description(PSTRLIST List) const;
};

class NEWSML:public GILSXML 
{
public:
  NEWSML (PIDBOBJ DbParent, const STRING& Name);
  ~NEWSML ();

  const char *Description(PSTRLIST List) const;

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);

  void LoadFieldTable();
};

//-----------------------------------------------------------------------

class RSS2:public XMLBASE
{
public:
  RSS2 (PIDBOBJ DbParent, const STRING& Name);
  ~RSS2();

  void LoadFieldTable();
  const char *Description(PSTRLIST List) const;

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);

  virtual STRING UnifiedName (const STRING& Tag, PSTRING Value) const;

  virtual bool StoreTagComplexAttributes(const char *tag_ptr) const;

  void Present (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const;

  void Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& Syntax,
        PSTRING StringBuffer) const {
    return XML::Present(ResultRecord, ElementSet, StringBuffer);
  }

};


class IETF_ATOM:public XMLBASE 
{
public:
  IETF_ATOM (PIDBOBJ DbParent, const STRING& Name);
  ~IETF_ATOM();

  STRING UnifiedName (const STRING& Tag, PSTRING Value) const;

  void LoadFieldTable();

  const char *Description(PSTRLIST List) const;
};


class CAP_RSS:public RSS2
{
public:
  CAP_RSS (PIDBOBJ DbParent, const STRING& Name);
  ~CAP_RSS();

  STRING UnifiedName (const STRING& Tag, PSTRING Value) const;

  void LoadFieldTable();

  const char *Description(PSTRLIST List) const;
};


class RSSCORE:public RSS2 
{
public:
  RSSCORE (PIDBOBJ DbParent, const STRING& Name);
  ~RSSCORE();

  bool URL(const RESULT& ResultRecord, PSTRING StringBuffer, bool OnlyRemote) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const;


  void LoadFieldTable();
  const char *Description(PSTRLIST List) const;

private:
  STRING     linkField;
  STRING     titleField;
  STRING     baseLinkField; // 
};


class RSSCOREARCHIVE:public RSSCORE 
{
public:
   RSSCOREARCHIVE (PIDBOBJ DbParent, const STRING& Name);
  ~RSSCOREARCHIVE() {;};

  const char *Description(PSTRLIST List) const;
};

class XMLREC:public XMLBASE
{
friend class RDFREC;
public:
  XMLREC (PIDBOBJ DbParent, const STRING& Name);

  void ParseRecords(const RECORD& FileRecord);
  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const;

/*
  void setRecordSeperator(const STRING& sep) { RecordSeperator = sep; }
  void setXMLPreface(const STRING& preface)  { XMLPreface = preface;  }
  void setXMLTail(const STRING& tail)        { XMLTail = tail;        }
*/

  const char *Description(PSTRLIST List) const;

  ~XMLREC ();

private:

  STRING RecordSeperator;
  STRING XMLPreface;
  STRING XMLTail;
} ;


class RDFREC:public XMLREC 
{
public:
  RDFREC (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  ~RDFREC ();
} ;


#endif
