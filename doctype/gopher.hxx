/*

File:        gopher.hxx
Version:     1
Description: class GOPHER - present with with gopher-style .cap name files
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef GOPHER_HXX
#define GOPHER_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class GOPHER : public DOCTYPE {
public:
   GOPHER(PIDBOBJ DbParent);
   //void ParseRecords(const RECORD& FileRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer) const;
   ~GOPHER();
};

typedef GOPHER* PGOPHER;

#endif
