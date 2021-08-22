/*@@@
File:           pandoc.cxx
Version:        1.00
Description:    Class Pandoc 
Author:         Edward Zimmermann
@@@*/

#ifndef PANDOC_HXX
#define PANDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "xml.hxx"

#if USE_LIBMAGIC
# undef _MAGIC_H
# include <magic.h>
#endif

class PANDOC : public FILTER2HTMLDOC {
public:
   PANDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   const char *GetDefaultFilter() const;
   ~PANDOC() {; }
private:
   STRING format;
};


#endif
