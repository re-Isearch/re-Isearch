/*@@@
File:           filter2.cxx
Version:        1.00
Description:    Class FILTER2
Author:         Edward Zimmermann
@@@*/

#ifndef FILTER2_HXX
#define FILTER2_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "htmlhead.hxx"
#include "ptext.hxx"
#include "xml.hxx"
#include "memodoc.hxx"


class FILTER2HTMLDOC : public HTMLHEAD {
public:
   FILTER2HTMLDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;

   virtual void BeforeIndexing();
   // virtual void BeforeRset(const STRING& RecordSyntax);

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);


   bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~FILTER2HTMLDOC() { }
protected:
   bool SetFilter(const STRING& Filter);
   STRING      GetFilter() const { return Filter; }

   bool SetArgs(const STRING& nArgs) {
      return (Args = nArgs).GetLength() != 0;
   }
   STRING      GetArgs() const { return Args; }


   void        SetMIME_Type(const STRING& m) { MIME_Type = m; }
   STRING      GetMIME_Type() const          { return MIME_Type; }

private:
   STRING Filter;
   STRING Args;
   STRING MIME_Type;
};


class FILTER2XMLDOC : public XML {
public:
   FILTER2XMLDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;

   virtual void BeforeIndexing();
   // virtual void BeforeRset(const STRING& RecordSyntax);

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);


   bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~FILTER2XMLDOC() { }

protected:
   bool SetFilter(const STRING& Filter);
   STRING      GetFilter() const { return Filter; }

   bool SetArgs(const STRING& nArgs) {
      return (Args = nArgs).GetLength() != 0;
   }
   STRING      GetArgs() const { return Args; }


   void        SetMIME_Type(const STRING& m) { MIME_Type = m; }
   STRING      GetMIME_Type() const          { return MIME_Type; }

private:
   STRING Filter;
   STRING Args;
   STRING MIME_Type;
};

class FILTER2TEXTDOC : public PTEXT {
public:
   FILTER2TEXTDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;

   virtual void BeforeIndexing();
   // virtual void BeforeRset(const STRING& RecordSyntax);

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);


   bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~FILTER2TEXTDOC() { }

protected:
   bool SetFilter(const STRING& Filter);
   STRING      GetFilter() const { return Filter; }

   bool SetArgs(const STRING& nArgs) {
      return (Args = nArgs).GetLength() != 0;
   }
   STRING      GetArgs() const { return Args; }


   void        SetMIME_Type(const STRING& m) { MIME_Type = m; }
   STRING      GetMIME_Type() const          { return MIME_Type; }

private:
   STRING Filter;
   STRING Args;
   STRING MIME_Type;
};


class FILTER2MEMODOC : public MEMODOC {
public:
   FILTER2MEMODOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;

   virtual void BeforeIndexing();
   // virtual void BeforeRset(const STRING& RecordSyntax);

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);


   bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~FILTER2MEMODOC() { }

protected:
   bool SetFilter(const STRING& Filter);
   STRING      GetFilter() const { return Filter; }


   bool SetArgs(const STRING& nArgs) {
      return (Args = nArgs).GetLength() != 0;
   }
   STRING      GetArgs() const { return Args; }


   void        SetMIME_Type(const STRING& m) { MIME_Type = m; }
   STRING      GetMIME_Type() const          { return MIME_Type; }

private:
   STRING Filter;
   STRING Args;
   STRING MIME_Type;
};



#endif
