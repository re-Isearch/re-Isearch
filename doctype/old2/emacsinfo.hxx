/*

File:        emacsinfo.hxx
Version:     1
Description: class EMACSINFO - first line is headline, rest is real body
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef EMACSINFO_HXX
#define EMACSINFO_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class EMACSINFO : public DOCTYPE {
public:
   EMACSINFO(PIDBOBJ DbParent);
   void ParseRecords(const RECORD& FileRecord);   
   void ParseFields(PRECORD NewRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer) const;
   ~EMACSINFO();
};

typedef EMACSINFO* PEMACSINFO;

#endif
