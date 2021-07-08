/*

File:        oneline.hxx
Version:     1
Description: class ONELINE - documents one per line
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef ONELINE_HXX
#define ONELINE_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class ONELINE : public DOCTYPE {
public:
   ONELINE(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void ParseRecords(const RECORD& FileRecord);
   ~ONELINE();
};

typedef ONELINE* PONELINE;

#endif
