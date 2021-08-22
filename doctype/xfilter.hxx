/*@@@
File:           xfilter.dxx
Version:        1.00
Description:    Class XFILTER // was XPandoc 
Author:         Edward Zimmermann
@@@*/

#ifndef XFILTER_HXX
#define XFILTER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "xml.hxx"

#if USE_LIBMAGIC
# undef _MAGIC_H
# include <magic.h>
#endif



class XFILTER : public XML {
public:
   XFILTER(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;
   virtual const char *GetDefaultOptions() const;


   virtual void BeforeIndexing();
   virtual void AfterIndexing();
   // virtual void BeforeRset(const STRING& RecordSyntax);

   void ParseRecords(const RECORD& FileRecord);

   void ParseFields(RECORD *RecordPtr);

   GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~XFILTER() {; }

protected:
   STRING      SetFilter(const STRING& Filter);
   STRING      GetFilter() const { return Filter; }

   GDT_BOOLEAN SetOptions(const STRING& nOptions) {
      return (Options = nOptions).GetLength() != 0;
   } 
   STRING      GetOptions() const { return Options; }

   void        SetMIME_Type(const STRING& m) { MIME_Type = m; }
   STRING      GetMIME_Type() const          { return MIME_Type; }

private:
   GDT_BOOLEAN GenRecord(const RECORD& FileRecord);

   STRING Filter;
   STRING Options;
   STRING MIME_Type;
#if USE_LIBMAGIC
  magic_t     magic_cookie;
#endif
};


#endif
