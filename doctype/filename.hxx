/*

File:        filename.hxx
Version:     1
Description: class FILENAME - indexes a file using  filename
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef FILENAME_HXX
#define FILENAME_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class FILENAME : public DOCTYPE {
public:
   FILENAME(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   void ParseRecords(const RECORD& FileRecord);
   void Present(const RESULT& ResultRecord, const STRING& ElementSet,
                PSTRING StringBuffer);
   ~FILENAME();
};

typedef FILENAME* PFILENAME;

#endif
