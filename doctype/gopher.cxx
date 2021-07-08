#pragma ident  "@(#)gopher.cxx	1.3 02/24/01 17:45:27 BSN"
/*

File:        gopher.cxx
Version:     1
Description: class GOPHER - present with with gopher-style .cap name files
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "gopher.hxx"

GOPHER::GOPHER(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE::DOCTYPE(DbParent, Name)
{
}


// Note:  To present a fully-qualified path name as a headline when there
// is no .cap file, #define FULLFILENAME either in here or in the Makefile.
// Otherwise, you get just the basename, sort of gopher-like.  I don't
// know which one you want, so have it your way.

void GOPHER::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) {
	
*StringBufferPtr = "Confused...";
// Basic strategy:  Given /local/fname
// If an "F" present, dump the entire file out.
// If a "B" present then
//    if can read /local/.cap/fname then
//        emit name based on contents of /local/.cap/fname
//    else
//        emit the fname of the record.

if (ElementSet.Equals("F")) {
   ResultRecord.GetRecordData(StringBufferPtr);
   }
else {
   FILE *nameFile;
   STRING pathName;
   STRING fname;
   ResultRecord.GetPath(&pathName);
   ResultRecord.GetFileName(&fname);
   pathName.Cat(".cap/");
   pathName.Cat(fname);
   nameFile = fopen(pathName,"rb");
   if (nameFile == (FILE *)0) {
      // must not be a capfile, so we just emit the filename
#ifdef FULLFILENAME
      *StringBufferPtr = pathName;
#else
      *StringBufferPtr = fname;
#endif
      }
   else {
      // we're going to read the capfile, look for a name= line, and emit
      // the part after "Name=".
      STRING linebuff;
      STRING testbuff;
      *StringBufferPtr = "Badly formed .cap file?";
      while (linebuff.FGet(nameFile,1024)) {
         testbuff = linebuff;
         testbuff.EraseAfter(5);
         if (testbuff.CaseEquals("Name=")) {
            // The line must be a name= line, so do something.
            linebuff.EraseBefore(6);
            *StringBufferPtr = linebuff;
            }
         } // end of while loop
      } // end of else we're going to read the capfile
   } // end of else it was a "B" present
         

}


GOPHER::~GOPHER() {
}
