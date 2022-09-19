#ifndef PSDOC_HXX
#define PSDOC_HXX

#include "filter2.hxx"

class PSDOC : public FILTER2TEXTDOC {
public:
   PSDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void SourceMIMEContent(STRING *stringPtr) const;

   virtual const char *GetDefaultFilter() const;

   ~PSDOC();
};

#endif


