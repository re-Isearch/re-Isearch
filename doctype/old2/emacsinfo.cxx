/*

File:        emacsinfo.cxx
Version:     1
Description: class EMACSINFO - index files with "File:" separators
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include <string.h>  /* For strstr() in ParseRecords */
#include "emacsinfo.hxx"

EMACSINFO::EMACSINFO(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE::DOCTYPE(DbParent, Name)
{
}



void EMACSINFO::ParseRecords(const RECORD& FileRecord) {


  GPTYPE Start = 0;
  GPTYPE End = 0;
  GPTYPE i = 0;
  PCHR   RecBuffer;
  GPTYPE RecStart, RecEnd, RecLength;
  GPTYPE ActualLength=0;
  
  STRING fn;
  FileRecord.GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp)
    {
      cout << "Could not access '" << fn << "'\n";
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
  
  

if(fseek(fp, 0, 2) == -1) {
	cout << "EMACSINFO::ParseRecords(): Seek failed - ";
	cout << fn << "\n";
	fclose(fp);
	return;	
	}
	
RecStart = 0;
RecEnd = ftell(fp);
if(RecEnd == 0) {
	cout << "EMACSINFO::ParseRecords(): Skipping ";
	cout << " zero-length record -" << fn << "...\n";
	fclose(fp);
	return;
	}


if(fseek(fp, RecStart, 0) == -1) {
	cout << "EMACSINFO::ParseRecords(): Seek failed - " << fn << "\n";
	fclose(fp);
	return;	
	}
	
RecLength = RecEnd - RecStart;
	
RecBuffer = new CHR[RecLength + 2];
if(!RecBuffer) {
	cout << "EMACSINFO::ParseRecords(): Failed to allocate ";
	cout << RecLength + 1 << " bytes - " << fn << "\n";
	fclose(fp);
	return;
	}

ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
if(ActualLength == 0) {
	cout << "EMACSINFO::ParseRecords(): Failed to fread\n";
	delete [] RecBuffer;
	fclose(fp);
	return;
	}
fclose(fp);
if(ActualLength != RecLength) {
	cout << "EMACSINFO::ParseRecords(): Failed to fread ";
	cout << RecLength << " bytes.  Actually read " << ActualLength;
	cout << " bytes - " << fn << "\n";
	delete [] RecBuffer;
	return;
	}

RecBuffer[ActualLength]='\0';  // NULL-terminate the buffer for strfns

// Now we walk through the buffer in RecBuffer and look for "FILE:" strings
// and mark the record pairs.

PCHR oldw = RecBuffer;
PCHR w;
PCHR bp = RecBuffer;

while ((w=strstr(bp+1,"File:")) != (char *) 0) { // while we can still find the next File: marker
   Start = oldw - RecBuffer;
   End   = (w - RecBuffer) - 1;
   Record.SetRecordStart(Start); Record.SetRecordEnd(End);
   Db->DocTypeAddRecord(Record);
   bp = w;
   oldw = w;
   }

Start = oldw-RecBuffer;   
End = ActualLength - 1;   

Record.SetRecordStart(Start); Record.SetRecordEnd(End);
Db->DocTypeAddRecord(Record);

	
}


// The goal of this is to find those pesky "File:" and "Node:" fields and mark
// them as searchable.

void EMACSINFO::ParseFields(PRECORD NewRecord) {
	PFILE 	fp;
	STRING 	fn;
	GPTYPE 	RecStart, 
		RecEnd, 
		RecLength, 
		ActualLength;
	PCHR 	RecBuffer;
	PCHR 	file;
	


	// Open the file
	NewRecord->GetFullFileName(&fn);
	file = fn.NewCString();
	fp = fopen(fn, "rb");
	if (!fp) {
		cout << "EMACSINFO::ParseRecords(): Failed to open file\n\t";
		perror(file);
		return;
	}

	// Determine the start and size of the record
	RecStart = NewRecord->GetRecordStart();
	RecEnd = NewRecord->GetRecordEnd();
	
	if (RecEnd == 0) {
		if(fseek(fp, 0, 2) == -1) {
			cout << "EMACSINFO::ParseRecords(): Seek failed - ";
			cout << fn << "\n";
			fclose(fp);
			return;	
		}
		RecStart = 0;
		RecEnd = ftell(fp);
		if(RecEnd == 0) {
			cout << "EMACSINFO::ParseRecords(): Skipping ";
			cout << " zero-length record -" << fn << "...\n";
			fclose(fp);
			return;
		}
		//RecEnd -= 1;
	}

	// Make two copies of the record in memory
	if(fseek(fp, RecStart, 0) == -1) {
		cout << "EMACSINFO::ParseRecords(): Seek failed - " << fn << "\n";
		fclose(fp);
		return;	
	}
	RecLength = RecEnd - RecStart;
	
	RecBuffer = new CHR[RecLength + 1];
	if(!RecBuffer) {
		cout << "EMACSINFO::ParseRecords(): Failed to allocate ";
		cout << RecLength + 1 << " bytes - " << fn << "\n";
		fclose(fp);
		return;
	}

	ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
	if(ActualLength == 0) {
		cout << "EMACSINFO::ParseRecords(): Failed to fread\n\t";
		perror(file);
		delete [] RecBuffer;
		fclose(fp);
		return;
	}
	fclose(fp);
	if(ActualLength != RecLength) {
		cout << "EMACSINFO::ParseRecords(): Failed to fread ";
		cout << RecLength << " bytes.  Actually read " << ActualLength;
		cout << " bytes - " << fn << "\n";
		delete [] RecBuffer;
		return;
	}
	RecBuffer[RecLength]='\0';
	

	// Parse the record and add fields to record structure
	STRING FieldName;
	FC fc;
	PFCT pfct;
	DF df;
	PDFT pdft;
	PCHR p;
	INT val_start;
	INT val_end;
	INT val_len;
	DFD dfd;
	PCHR fileStarter, nodeStarter;

	pdft = new DFT();
	if(!pdft) {
		cout << "EMACSINFO::ParseRecords(): Failed to allocate DFT - ";
		cout << fn << "\n";
		delete [] RecBuffer;
		return;
		}

	fileStarter = strstr(RecBuffer,"File:");
	if (fileStarter != (char *)0) {
		val_start = (fileStarter-RecBuffer)+5;
		for (val_end = val_start; (RecBuffer[val_end]!=',') 
			&& (val_end < ActualLength); val_end++);
		// We have a tag pair
		FieldName = "file";
		dfd.SetFieldName(FieldName);
		Db->DfdtAddEntry(dfd);
		fc.SetFieldStart(val_start);
		fc.SetFieldEnd(val_end);
		pfct = new FCT();
		pfct->AddEntry(fc);
		df.SetFct(*pfct);
		df.SetFieldName(FieldName);
		pdft->AddEntry(df);
		delete pfct;
		}
		

	nodeStarter = strstr(RecBuffer,"Node:");
	if (nodeStarter != (char *)0) {
		val_start = (nodeStarter-RecBuffer)+5;
		for (val_end = val_start; (RecBuffer[val_end]!=',') 
			&& (val_end < ActualLength); val_end++);
		// We have a tag pair
		FieldName = "node";
		dfd.SetFieldName(FieldName);
		Db->DfdtAddEntry(dfd);
		fc.SetFieldStart(val_start);
		fc.SetFieldEnd(val_end);
		pfct = new FCT();
		pfct->AddEntry(fc);
		df.SetFct(*pfct);
		df.SetFieldName(FieldName);
		pdft->AddEntry(df);
		delete pfct;
		}
	NewRecord->SetDft(*pdft);
	delete pdft;
	delete [] RecBuffer;

}




void EMACSINFO::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) {
	
*StringBufferPtr = "";
// Basic strategy:  on a "B" present, show the first line.  On an "F" present,
// show everything *but* the first line.  Simple enough, right?

STRING myBuff;
ResultRecord.GetRecordData(&myBuff);
STRINGINDEX firstNL = myBuff.Search('\n');
if (firstNL == 0) {
   cout << "FTP::Present() -- Can't find first Newline in file to present.\n";
   return;
   }

if (ElementSet.Equals("F")) {
   // do we want the File: and Node: line on a full present? I think so.
   }
else if (ElementSet.Equals("B")) {
   myBuff.EraseAfter(firstNL);
   }

*StringBufferPtr = myBuff;

} 


EMACSINFO::~EMACSINFO() {
}
