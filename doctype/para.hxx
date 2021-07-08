/*-
File:        para.hxx
Version:     1
Description: class PARA - index paragraphs
Author:      Erik Scott, Scott Technologies, Inc.
Modifications: Edward C. Zimmermann, edz@nonmonotonic.net
*/


#ifndef PARA_HXX
#define PARA_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif
#include "buffer.hxx"

class PARA : public DOCTYPE {
public:
   PARA(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   virtual void AfterIndexing();

   virtual void ParseRecords(const RECORD& FileRecord);
   virtual void ParseFields (PRECORD NewRecord);
   virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
         STRING *StringBufferPtr) const;
   ~PARA();
private:
   BUFFER recBuffer;
};

typedef PARA* PPARA;

#endif
