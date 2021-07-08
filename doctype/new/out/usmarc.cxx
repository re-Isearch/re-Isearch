/*
File:        usmarc.cxx
Version:     1
Description: class USMARC - MARC records for library use
Author:      Erik Scott, Scott Technologies, Inc.

Modified by Bjorn L. Thordarson
  Added more fine grained indexing support. We can now index agains $t fields and specific subfields

*/

#include <ctype.h>
#include "isearch.hxx"
#include "marc.hxx"
#include "usmarc.hxx"
#include "defs.hxx"  // to get record syntaxes

#define EOL "\n";

#define END_OF_RECORD 0x1d
#define END_OF_FIELD 0x1e
#define START_OF_SUBFIELD 0x1f

CHR *RecBuffer;
GPTYPE marcNumDirEntries;
GPTYPE marcRecordLength;
GPTYPE marcBaseAddr;

marc_dir_entry *marcDir;

USMARC::USMARC(PIDBOBJ DbParent) 
  : DOCTYPE(DbParent) 
{
}

struct ParseEntry
{
  char *field;
  char *subfield;
  char *tag;
  char *name;
};

// Maps a character string representation of a marc field
// number to a field name that would be sensible to ordinary,
// unsophisticated users.  These users have a bibliographic vocabulary that
// contains "author", "subject", "title", and little else.  They couldn't
// care less if the author was a 110 corporate author or a 111 meeting name
// author, especially since something like proceedings of the world wide web
// consortium, for example, could very easily go either way.
// Real Librarians(TM) will want to do some hacking in here. :-)
ParseEntry ParseData[] = 
{
  // Ordaleit
  { "11*", "**", "*", "Ordaleit" },
  { "3**", "**", "*", "Ordaleit" },
  { "5**", "**", "*", "Ordaleit" },
  { "6**", "**", "*", "Ordaleit" },
  { "71*", "**", "*", "Ordaleit" },
  { "8**", "**", "*", "Ordaleit" },

  // Title
  { "24*", "**", "a", "title" },
  { "24*", "**", "b", "title" },
  { "24*", "**", "h", "title" },
  { "24*", "**", "i", "title" },
  { "24*", "**", "j", "title" },
  { "24*", "**", "k", "title" },
  { "4**", "**", "a", "title" },
  { "4**", "**", "b", "title" },
  { "4**", "**", "l", "title" },
  { "505", "**", "t", "title" },
  { "74*", "**", "a", "title" },

  // Adfong/ferill/thydingar
  { "700", "*1", "a", "Adfong" },
  { "700", "*1", "b", "Adfong" },
  { "700", "*1", "h", "Adfong" },
  { "700", "*1", "y", "Adfong" },

  // Author
  { "100", "*0", "a", "author" },
  { "100", "*0", "b", "author" },
  { "100", "*0", "h", "author" },
  { "700", "*0", "a", "author" },
  { "700", "*0", "b", "author" },
  { "700", "*0", "h", "author" },
  
  // Efnisord
  { "655", "00", "*", "subject" },
  { "655", "09", "*", "subject" },
  { "659", "00", "*", "subject" },

  // Bokmenntaform
  { "655", "09", "*", "Bokmenntaform" },

  { "001", "**", "*", "Control-number-local" },
  { "010", "**", "*", "lccn" },
  { "020", "**", "*", "isbn" },
  { "022", "**", "*", "issn" },
  { "082", "**", "*", "dewey" },
  { "260", "**", "*", "publisher" },
  { "500", "**", "*", "note" },
  { "535", "**", "*", "Identifier-authority/format" },
  { "593", "**", "*", "Author-name" },
  { "972", "**", "*", "Classification-local" }
};

int ParseEntries = sizeof(ParseData) / sizeof(ParseEntry);

void 
USMARC::ParseRecords(const RECORD& FileRecord) 
{
  // Finding the range of a MARC record is easy:  The first five bytes of any
  // record MARC record contains a zero-filled representation of the record
  // length.  We assume that your files will consist of a bunch of MARC
  // records concatenated into one big file, so we:
  //   while not end of file do
  //       read five bytes
  //       make an int out of them
  //       indicate that range as a record
  //       increment the file pointer to the start of the next record.
  //   done

  STRING fn;
  FileRecord.GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp) {
      cout << "Could not access '" << fn << "'" << EOL;
      return;			// File not accessed   
  }
  
  RECORD Record;
  STRING s;

  FileRecord.GetPath(&s);
  Record.SetPath( s );

  FileRecord.GetFileName(&s);
  Record.SetFileName( s );

  FileRecord.GetDocumentType(&s);
  Record.SetDocumentType ( s );

  int RS = 0;          // we also know the first record will begin @ 0.
  
  if(fseek(fp, 0, SEEK_SET) == -1) {
    cout << "USMARC::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;
  }

  int marcLength;
  char LenBuff[6];
  LenBuff[5] = '\0';

  while(fread(LenBuff, sizeof(char), 5, fp) == 5) {
    marcLength = atoi(LenBuff);

    if (marcLength <= 0) {
      cout << "Something went awry trying to read MARC record Length in "
	   << fn << " \n";
      return;
    }

    // else we must have a valid marcLength now, so lets burn some characters
    if (fseek(fp, marcLength - 5, SEEK_CUR) < 0) {
      cout << "Something went awry trying to read MARC record"
	   << fn << " \n";
      return;
    }

    Record.SetRecordStart(RS);
    Record.SetRecordEnd ((RS+marcLength)-1);

    Db->DocTypeAddRecord(Record);
    RS = RS+marcLength;
  }
}

void 
USMARC::readFileContents(PRECORD NewRecord) 
{
  STRING fn;
  PCHR   file;
  PFILE  fp;

  NewRecord->GetFullFileName(&fn);
  file = fn.NewCString();
  fp = fopen(fn, "rb");
  if (!fp) {
    cout << "USMARC::ParseRecords(): Failed to open file\n\t";
    perror(file);
    return;
  }
  // Determine the start and size of the record
  GPTYPE RecStart = NewRecord->GetRecordStart();
  GPTYPE RecEnd = NewRecord->GetRecordEnd();
  GPTYPE RecLength;

  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      cout << "USMARC::ParseRecords(): Seek failed - ";
      cout << fn << "\n";
      fclose(fp);
      return;
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      cout << "USMARC::ParseRecords(): Skipping ";
      cout << " zero-length record -" << fn << "...\n";
      fclose(fp);
      return;
    }
    //RecEnd -= 1;
  }
  if(fseek(fp, (long)RecStart, SEEK_SET) == -1) {
    cout << "USMARC::ParseRecords(): Seek failed - " << fn << "\n";
    fclose(fp);
    return;
  }
  RecLength = RecEnd - RecStart;

  RecBuffer = new CHR[RecLength + 1];
  if(!RecBuffer) {
    cout << "USMARC::ParseRecords(): Failed to allocate ";
    cout << RecLength + 1 << " bytes - " << fn << "\n";
    fclose(fp);
    return;
  }

  GPTYPE ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
  if(ActualLength == 0) {
    cout << "USMARC::ParseRecords(): Failed to fread\n\t";
    cout << "RecLength is: " << RecLength << endl;
    perror(file);
    delete [] RecBuffer;
    fclose(fp);
    return;
  }
  fclose(fp);
  if(ActualLength != RecLength) {
    cout << "USMARC::ParseRecords(): Failed to fread ";
    cout << RecLength << " bytes.  Actually read " << ActualLength;
    cout << " bytes - " << fn << "\n";
    delete [] RecBuffer;
    return;
  }

  RecBuffer[RecLength]='\0';
}


int
USMARC::readRecordLength(void) 
{
  char lenstr[6];
  for (int i = 0; i < 5; i++) lenstr[i] = RecBuffer[i];
  lenstr[5] = '\0';
  return atoi(lenstr);
}

int
USMARC::readBaseAddr(void) 
{
  char lenstr[6];
  for (int i = 12; i < 17; i++) lenstr[i-12] = RecBuffer[i];
  lenstr[5] = '\0';
  return atoi(lenstr);
}

void 
USMARC::readMarcStructure(PRECORD NewRecord)
{
  readFileContents(NewRecord);
  marcRecordLength = readRecordLength(); // set global "marcRecordLength"
  marcBaseAddr = readBaseAddr();         // set global "marcBaseAddr"

  marcNumDirEntries = ( (marcBaseAddr-25) / 12);
  unsigned int i;
  marcDir = new marc_dir_entry[marcNumDirEntries];

  int bytenum,j;
  for (i = 0; i < marcNumDirEntries; i++) {
    // read that dir entry.
    // seek to 24 + (numdirentry*12), read the field (3 bytes), the length
    // (4 bytes), and the starting address (5 bytes).  Remember to 
    // null-terminate each component.  Remember the starting address is 
    // relative to marcBaseAddr.
    bytenum = 24+(i*12);

    for (j=0; j<3; j++)
      marcDir[i].field[j]=RecBuffer[bytenum+j];
    marcDir[i].field[3]='\0';

    for (j=0; j<4; j++)
      marcDir[i].length[j]=RecBuffer[bytenum+3+j];
    marcDir[i].length[4]='\0';

    for (j=0; j<5; j++)
      marcDir[i].offset[j]=RecBuffer[bytenum+7+j];
    marcDir[i].offset[5]='\0';

    // We now read the subfield... There is only a subfield if it has the following format digit, digit, 0x1f
    int TagPos = marcBaseAddr + atoi(marcDir[i].offset);
    marcDir[i].subfield[0] = '\0';
    marcDir[i].subfield[1] = '\0';
    marcDir[i].subfield[2] = '\0';

    if (RecBuffer[TagPos + 2] == START_OF_SUBFIELD) {
      marcDir[i].subfield[0] = RecBuffer[TagPos + 0];
      marcDir[i].subfield[1] = RecBuffer[TagPos + 1];
    }
  }
}


// The function usefulMarcField is used to decide if you want to index a given
// field. It should be used by both ParseFields and ParseWords, since it's
// a bad idea to have a field with no words or words without a field.
// The idea is that we will pass in a field as a null-terminated string, 
// like "110", and the function will return 0 is we should ignore it and non-0
// if we should index it.  For now, I'm going to say "index everything" even
// though that isn't the best policy.

int 
USMARC::usefulMarcField(char *fieldStr)
{
  return 1;
}

int
USMARC::compareReg(char *s1 , char *s2) {
  if (s1 == NULL || s2 == NULL) { // FIXME: Think out behavior if this happens
  }

  if (*s2 == '\0')
    return 1;      // An empty match string matches everything

  if (*s1 == '\0') {
    for (unsigned int i = 0; i < strlen(s2); i++) {
      if (s2[i] != '*')
	return 0;  // Return false, empty string does not match a specified tag
    }
    return 1;      // Empty string matches all * strings though
  }

  for (unsigned int i = 0; i < strlen(s2); i++) {
    if (s2[i] != '*' && s2[i] != s1[i])
      return 0;    // No match
  }

  return 1;         // We have a match!
}

char 
USMARC::findNextTag(char *RecBuffer, int &pos, int &tagPos, int &tagLength)
{
  char tag;

  if (RecBuffer[pos] == END_OF_FIELD) // End of field marker?
    return 0x00;

  while (RecBuffer[pos] != END_OF_FIELD && RecBuffer[pos] != START_OF_SUBFIELD)
    pos++;

  if (RecBuffer[pos] == END_OF_FIELD) {
    pos++;
    return 0x00;
  }
  else {            // We have found a tag
    pos++;
    tag = RecBuffer[pos];
    pos++;
    tagPos = pos;
                    // Calculate length of field
    tagLength = 0;
    while (RecBuffer[pos] != START_OF_SUBFIELD && RecBuffer[pos] != END_OF_FIELD) {
      tagLength++;
      pos++;
    }

    return tag;
  }
}

void
USMARC::addSearchEntry(PDFT pdft, STRING fieldName, int fieldStart, int fieldEnd)
{
  // The following three data items can be static at the cost of a little
  // more memory giving increased speed because we will only call the
  // constructor once in that case.
  static FC fc;
  static DF df;
  static DFD dfd;

  PFCT pfct;
  dfd.SetFieldName(fieldName);
  Db->DfdtAddEntry(dfd);
  fc.SetFieldStart(fieldStart);
  fc.SetFieldEnd(fieldEnd);
  pfct = new FCT();
  pfct->AddEntry(fc);
  df.SetFct(*pfct);
  df.SetFieldName(fieldName);
  pdft->AddEntry(df);
  delete pfct;
}

void 
USMARC::ParseFields(PRECORD NewRecord) 
{
  // Right now, we're just going to call readMarcStructure()
  // and use this to debug that.  Joy joy.
  readMarcStructure(NewRecord);

  // Now the strategy is:
  // for each dir entry,
  //    check to see if we should add this field
  //    if so, add it as its MARC field number (like 110 for corp author)
  //    check to see if that field has been given a name (like "author").
  //      if so, then add it again using the name we give it.
  PDFT pdft = new DFT();
  if(!pdft) {
    cout << "USMARC::ParseRecords(): Failed to allocate DFT \n";
    delete [] RecBuffer;
    delete [] marcDir;
    return;
  }

  for (unsigned int i = 0; i < marcNumDirEntries; i++) {
    if (usefulMarcField(marcDir[i].field)) { // is it a field we're interested in?
      int fieldLength = atoi(marcDir[i].length);
      int fieldOffset = atoi(marcDir[i].offset);
      int fieldPos = fieldOffset+marcBaseAddr;
      int tagPos, tagLength;
      char tag;
 
      // We always index the first by the field name (ie. 100, 245 and soforth).
      addSearchEntry(pdft, marcDir[i].field, fieldPos, fieldPos + fieldLength-1);

      for (int j = 0; j < ParseEntries; j++) {
	if ( compareReg(marcDir[i].field, ParseData[j].field) &&
	     compareReg(marcDir[i].subfield, ParseData[j].subfield)) {
	  // Ok we have matched the field and subfield... We are now searching for a specific tag
	  // First we need to see if that tag exists and if so we find the span of it.

	  if (*ParseData[j].tag == '*')
	    addSearchEntry(pdft, ParseData[j].name, fieldPos, fieldPos + fieldLength-1);
	  else
	    while((tag = findNextTag(RecBuffer, fieldPos, tagPos, tagLength)) != '\0' && (tag == *ParseData[j].tag))
	      addSearchEntry(pdft, ParseData[j].name, tagPos, tagPos + tagLength);
	}
      }
    }    // end of if usefulMarcField
  }       // end of for loop


  NewRecord->SetDft(*pdft);
  delete pdft;
}          // end of function


GPTYPE 
USMARC::ParseWords(CHR* DataBuffer, INT DataLength, INT DataOffset,
			  GPTYPE* GpBuffer, INT GpLength) 
{
  INT GpListSize = 0;
  INT Position = 0;
  INT fieldLength, fieldOffset;
  INT endingPosition;

  for (unsigned int i = 0; i < marcNumDirEntries; i++) {
    fieldLength = atoi(marcDir[i].length);
    fieldOffset = atoi(marcDir[i].offset);

    Position = marcBaseAddr + fieldOffset; // beginning of marc field

    if (RecBuffer[Position + 2] == START_OF_SUBFIELD)
      Position += 2;                // the +2 is to skip the two-character "indicator" at the front of some USMARC fields.

    endingPosition = fieldOffset + marcBaseAddr + fieldLength -2;  // we don't need the trailer field delimiter, either.

    while (Position < endingPosition) {
      if (RecBuffer[Position] == START_OF_SUBFIELD) {
	Position += 2; // skip over the subfield indicator
      }

      while ( (Position < endingPosition) && 
	      (!IsAlnum(DataBuffer[Position])) ) {
	Position++;
      }

      if ( (Position < endingPosition) &&
	   (!(Db->IsStopWord(DataBuffer + Position, DataLength))) ) {
	GpBuffer[GpListSize++] = DataOffset + Position;
      }

      /*      int s = Position;
	      int e;*/

      while ( (Position < endingPosition) &&
	      (IsAlnum(DataBuffer[Position])) ) {
	Position++;
      }

      /*      e = Position;

       char *tmp = new char[e - s + 1];
       strncpy(tmp, DataBuffer + s, e - s);
       tmp[e - s] = 0x00;
       cerr << "DataBuffer: " << tmp << endl;
       delete []tmp;

       tmp = new char[e - s + 1];
       strncpy(tmp, RecBuffer + s, e - s);
       tmp[e - s] = 0x00;
       cerr << "RecBuffer:  " << tmp << endl;
       delete []tmp;*/

    } // end of while loop;
  } // end of for i in dir entries
   
  // before we leave, we should free some leaks (er, I mean, "blocks of memory")
  delete [] RecBuffer;
  delete [] marcDir;
   
  return GpListSize;
}	// Return # of GP's added to GpBuffer

void 
USMARC::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) 
{
  STRING RecSyntax;
  RecSyntax = SutrsRecordSyntax;
  Present(ResultRecord, ElementSet, RecSyntax, StringBufferPtr);
}

void 
USMARC::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		     const STRING& RecordSyntax, STRING* StringBufferPtr) 
{
  STRING myBuff;
  ResultRecord.GetRecordData(&myBuff);

  MARC *m;
  STRING outputBuff;

  if (RecordSyntax == UsmarcRecordSyntax) {
    // hey, if they want MARC, just shove it at 'em.  They'll figure it out.
    *StringBufferPtr = myBuff;
    return;  // don't forget to bail out now.

  } else if (RecordSyntax == HtmlRecordSyntax) {
    m = new MARC(myBuff);
    if (ElementSet.Equals("B")) {
      m->SetDisplayFormat(MARC_FORMAT_TITLE);         // = 4
      m->Print(&outputBuff);
      delete m;
      *StringBufferPtr = outputBuff;

    } else {
	m->Print(&outputBuff);
	delete m;
	*StringBufferPtr = "<pre>\n";
	StringBufferPtr->Cat(outputBuff);
	StringBufferPtr->Cat("</pre>");
    }

  } else { // default to SUTRS, the Z39.50-compliant way to say "ASCII".
    m = new MARC(myBuff);
    if (ElementSet.Equals("B")) {
      m->SetDisplayFormat(MARC_FORMAT_TITLE);         // = 4
    }
    m->Print(&outputBuff);
    delete m;
    *StringBufferPtr = outputBuff;
  }

  return;
} 

USMARC::~USMARC() {
}
