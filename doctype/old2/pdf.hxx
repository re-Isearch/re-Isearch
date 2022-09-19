/**********************************************************************/
/*@@@
File:           pdf.hxx
Version:        2.00
Description:    Class ADOBE_PDFDOC - PDF Document Type
@@@*/

#ifndef PDF_HXX
#define PDF_HXX

#include "memodoc.hxx"

class ADOBE_PDFDOC : public _VMEMODOC {
public:
   ADOBE_PDFDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   void BeforeIndexing() { MEMODOC::BeforeIndexing(); }

   INT UnifiedNames (const STRING& tag, PSTRLIST Value) const;

   const char *GetDefaultFilter() const;

   off_t RunPipe(FILE *fp, const STRING& Source);

   void ParseFields(RECORD *RecordPtr);

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~ADOBE_PDFDOC() { }

private:
   STRING desc;
   off_t   HostID;
};

#endif
