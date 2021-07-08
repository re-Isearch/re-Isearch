/*

File:        filename.cxx
Version:     1
Description: class FILENAME - index files based on their filename
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "filename.hxx"

FILENAME::FILENAME(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
}

const char *FILENAME::Description(PSTRLIST List) const
{
  List->AddEntry ("FILENAME");
  DOCTYPE::Description(List);
  return "Index files based on their filename";
}



void FILENAME::ParseRecords(const RECORD& FileRecord) {


  GPTYPE Start = 0;
  GPTYPE Position = 0;
  GPTYPE Pos = 0;

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  static int gdb_tester;
  

// Now we know the filename.  Make a "filename.fn" filename, and write
// the file name into that file?  Make sense?  Of course not.  Here's
// the box score:
//
// Given a file called "bubba" that contains the string "howdy, slim."
// create a file named bubba.fn that contains the string "bubba".
// On a search, we'll look for "bubba" in bubba.fn.
//
// We have to do this the silly way because Iindex can only index
// text that it can get a file pointer to.  Text in RAM-only cannot
// be indexed.  This also means that derived text cannot be indexed.

STRING newfilename = Fn ;
newfilename.Cat(".fn");


// Now that we have the new filename, let's open it for writing, stick the old
// file name in there, and close up that file.

FILE *fnfp = fopen(newfilename,"wb");
if (fnfp == NULL) {
   cout << "Cannot write file " << newfilename << ", bailing out.\n";
   return; // leaking all the way...
   }

Fn.Print(fnfp);
fclose(fnfp);

// Now, we need to use the SetFileName member function, to, eh, *adjust*
// (yeah, yeah, that's what we'll call it) the file name.
// Note that we'll use GetFileName to get the non-path part of the name,
// append to it, and then save the appended name.  It's just mildly
// easier that way.

RECORD Record;
STRING s;
GPTYPE i;
FileRecord.GetPath(&s);
Record.SetPath( s );

FileRecord.GetFileName(&s);
s.Cat(".fn");
Record.SetFileName( s );

FileRecord.GetDocumentType(&s);
Record.SetDocumentType ( s );

Record.GetFullFileName(&s);

Record.SetRecordStart(FileRecord.GetRecordStart());
Record.SetRecordEnd(FileRecord.GetRecordEnd());


// Now we can finally do the whatever to tell this thing to use the
// modified record.

Db->DocTypeAddRecord(Record);

// That *should* be all we need.
// Now we need a mutant Present().


}


void FILENAME::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) {
	*StringBufferPtr = "";

// First we see if the element set is "B", meaning they just want a file
// name.  We're going to return the full file name for now.

if (ElementSet.Equals("B")) {
	ResultRecord.GetRecordData(StringBufferPtr);
	return;
}


// Now we add code to see if the element set was "F", and if it was
// then we return the contents of the original file instead of the
// filename.  This is of course never called if an above condition
// was satisfied.

// First, let's get the (eh, "amended") filename...

STRING hackedFN;
ResultRecord.GetFullFileName(&hackedFN);
STRINGINDEX dotFN = hackedFN.SearchReverse(".fn");
hackedFN.EraseAfter(dotFN - 1);

// Brace yourself for the most overkill "read-a-file-and-malloc-a-buffer"
// in history.  I really, really should use mmap(), but alas, there are still
// HPUX 9.X machines without the unsupported kernal hack...

  PFILE fp = fopen (hackedFN, "rb");
  if (!fp)
    {
      cout << "Could not access '" << hackedFN << "'\n";
      return;			// File not accessed

    }

  

if(fseek(fp, 0, 2) == -1) {
	cout << "FILENAME::Present(): Seek failed (I) - ";
	cout << hackedFN << "\n";
	fclose(fp);
	return;	
	}
	
GPTYPE RecStart = 0;
GPTYPE RecEnd = ftell(fp);
if(RecEnd == 0) {
	cout << "FILENAME::Present(): Skipping ";
	cout << " zero-length record -" << hackedFN << "...\n";
	fclose(fp);
	return;
	}


if(fseek(fp, RecStart, 0) == -1) {
	cout << "FILENAME::Present(): Seek failed (II) - " << hackedFN << "\n";
	fclose(fp);
	return;	
	}
	
GPTYPE RecLength = RecEnd - RecStart;
	
PCHR RecBuffer = new CHR[RecLength + 2];
if(!RecBuffer) {
	cout << "FILENAME::Present(): Failed to allocate ";
	cout << RecLength + 1 << " bytes - " << hackedFN << "\n";
	fclose(fp);
	return;
	}

GPTYPE ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
if(ActualLength == 0) {
	cout << "FILENAME::Present(): Failed to fread\n";
	delete [] RecBuffer;
	fclose(fp);
	return;
	}
fclose(fp);
if(ActualLength != RecLength) {
	cout << "FILENAME::Present(): Failed to fread ";
	cout << RecLength << " bytes.  Actually read " << ActualLength;
	cout << " bytes - " << hackedFN << "\n";
	delete [] RecBuffer;
	return;
	}

RecBuffer[ActualLength]='\0';  // NULL-terminate the buffer for strfns

*StringBufferPtr = RecBuffer;

}


FILENAME::~FILENAME() {
}
