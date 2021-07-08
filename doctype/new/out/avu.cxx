/*
File:        usmarc.cxx
Version:     1
Description: class USMARC - MARC records for library use
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include <string.h>  /* For strstr() in ParseRecords */
#include "isearch.hxx"
#include "marc.hxx"
#include "avu.hxx"
#include "defs.hxx"  // to get record syntaxes

#define EOL "\n";


AVU::AVU(PIDBOBJ DbParent) 
  : USMARC(DbParent) 
{
}

int 
usefulAVUMarcField(char *fieldStr) 
{
  return 1;
}



AVUFieldNumToName(char *fieldStr, char *fieldName) 
{
  int fieldNum;

  sscanf(fieldStr,"%d",&fieldNum);  // I *like* sscanf. It makes me feel good.

  fieldName[0]='\0';  // This means that if nothing else happens, return null str.

  switch (fieldNum) {
  case 10: 
    strcpy(fieldName,"lccn"); break;
  case 20: 
    strcpy(fieldName,"isbn"); break;
  case 22: 
    strcpy(fieldName,"issn"); break;
  case 40: 
    strcpy(fieldName,"source"); break;
  case 82: 
    strcpy(fieldName,"dewey"); break;
  case 100: 
    strcpy(fieldName,"author"); break;
  case 245: 
    strcpy(fieldName,"title"); break;
  case 246: 
    strcpy(fieldName,"additional title"); break;
  case 250: 
    strcpy(fieldName,"edition"); break;
  case 260: 
    strcpy(fieldName,"publisher"); break;
  case 440: 
    strcpy(fieldName,"series"); break;
  case 490: 
    strcpy(fieldName,"title"); break;  // really should be "series statement"
  case 500: 
    strcpy(fieldName,"note"); break;
  case 520: 
    strcpy(fieldName,"description"); break;
  case 524: 
    strcpy(fieldName,"citation"); break;
  case 540: 
    strcpy(fieldName,"terms"); break;
  case 600: 
    strcpy(fieldName,"subject"); break;
  case 611: 
    strcpy(fieldName,"subject"); break;
  case 630: 
    strcpy(fieldName,"subject"); break;
  case 650: 
    strcpy(fieldName,"subject"); break;
  case 651: 
    strcpy(fieldName,"subject"); break;
  case 655: 
    strcpy(fieldName,"genre"); break;
  case 740: 
    strcpy(fieldName,"title"); break;
  case 830: 
    strcpy(fieldName,"title"); break; // "series added entry - uniform title"
  case 856: 
    strcpy(fieldName,"link"); break;
  default: 
    fieldName[0]='\0'; break;
  }
}


void 
AVU::ParseFields(PRECORD NewRecord) 
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

  int i;
  STRING FieldName;
  FC fc;
  PFCT pfct;
  DF df;
  PDFT pdft;
  DFD dfd;
  int fieldLength, fieldOffset;
  char newFieldName[256];

  pdft = new DFT();
  if(!pdft) {
    cout << "EMACSINFO::ParseRecords(): Failed to allocate DFT \n";
    delete [] RecBuffer;
    delete [] marcDir;
    return;
  }
  for (i=0; i< marcNumDirEntries; i++) {
    if (usefulAVUMarcField(marcDir[i].field)) { // is it a field we're interested in?
      sscanf(marcDir[i].length,"%d",&fieldLength);
      sscanf(marcDir[i].offset,"%d",&fieldOffset);
      
      FieldName = marcDir[i].field;
      dfd.SetFieldName(FieldName);
      Db->DfdtAddEntry(dfd);
      fc.SetFieldStart(fieldOffset+marcBaseAddr);
      fc.SetFieldEnd(fieldOffset+marcBaseAddr+fieldLength-1);
      pfct = new FCT();
      pfct->AddEntry(fc);
      df.SetFct(*pfct);
      df.SetFieldName(FieldName);
      pdft->AddEntry(df);
      delete pfct;
      
      AVUFieldNumToName(marcDir[i].field, newFieldName);
      if (newFieldName[0]!='\0') {  // if we want the same field to have two names
	FieldName = newFieldName;
	dfd.SetFieldName(FieldName);
	Db->DfdtAddEntry(dfd);
	fc.SetFieldStart(fieldOffset+marcBaseAddr);
	fc.SetFieldEnd(fieldOffset+marcBaseAddr+fieldLength-1);
	pfct = new FCT();
	pfct->AddEntry(fc);
	df.SetFct(*pfct);
	df.SetFieldName(FieldName);
	pdft->AddEntry(df);
	delete pfct;
      } // end of if we have a second name
    }    // end of if usefulMarcField
  }       // end of for loop


  NewRecord->SetDft(*pdft);
  delete pdft;

}          // end of function


void 
AVU::Present(const RESULT& ResultRecord, const STRING& ElementSet,
		STRING* StringBufferPtr) 
{
  STRING RecSyntax;
  RecSyntax = SutrsRecordSyntax;
  Present(ResultRecord, ElementSet, RecSyntax, StringBufferPtr);
}


void 
AVU::Present(const RESULT& ResultRecord, const STRING& ElementSet,
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
      MARC_REC *rec;
      MARC_FIELD *fld;
      MARC_SUBFIELD *sub;

      CHR fieldbuffer[10000];
      CHR *cBuff;
      STRING Hold;
      STRING Title;

      Hold = "<HTML>\n<HEAD>\n";

      cBuff = myBuff.NewCString();

      rec = GetMARC(cBuff,strlen(cBuff),0);
      fld = GetField(rec, (MARC_FIELD *)NULL, fieldbuffer, "245");
      sub = GetSubf(fld, fieldbuffer, 'a');
      if (sub) {
	codeconvert(fieldbuffer);
	Title.Cat(fieldbuffer);
      } else {
	Title.Cat("No title");
      }

      sub = GetSubf(fld, fieldbuffer, 'c');
      if (sub) {
	codeconvert(fieldbuffer);
	Hold.Cat(fieldbuffer);
      }

      *StringBufferPtr = "<h1>\n";
      StringBufferPtr->Cat(Hold);
      StringBufferPtr->Cat("\n</h1>");
      
      delete [] cBuff;
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

AVU::~AVU() {
}
