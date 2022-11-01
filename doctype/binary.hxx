/*-@@@
File:		binary.hxx
Version:	0.01
Description:	Class BINARY - Binary files
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef BINARY_HXX
#define BINARY_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

#if USE_LIBMAGIC
# undef _MAGIC_H
# include <magic.h>
#endif

class BINARY:public DOCTYPE
{
 friend class XBINARY;
 public:
  BINARY (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void SetPostFix(const STRING& Ext = NulString);
  const STRING& GetPostFix() const;

  void BeforeIndexing();
  void AfterIndexing();

  virtual bool IsIgnoreMetaField(const STRING&) const { return false; };

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD FileRecord);

  bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBuffer) const;
  void SourceMIMEContent(const RESULT& ResultRecord, 
	PSTRING StringPtr) const;
   ~BINARY ();
 private:
  STRING PostFix;
#if USE_LIBMAGIC
  magic_t magic_cookie;
#endif
};

typedef BINARY *PBINARY;

class XBINARY:public BINARY
{
 public:
  XBINARY (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void SetBaseClass(const STRING& Class = NulString);
  const DOCTYPE_ID GetBaseClass() const { return InfoDoctype; }

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD FileRecord);

  virtual bool IsIgnoreMetaField(const STRING &Fieldname) const;

  bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
                const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void Present (const RESULT& ResultRecord, const STRING& ElementSet,
                const STRING& RecordSyntax, PSTRING StringBuffer) const;

   ~XBINARY () {};
private:
   DOCTYPE_ID InfoDoctype;
};

typedef XBINARY *PXBINARY;


class TBINARY:public XBINARY
{
 public:
  TBINARY (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual bool IsIgnoreMetaField(const STRING &Fieldname) const;

   ~TBINARY () {};
};



#endif
