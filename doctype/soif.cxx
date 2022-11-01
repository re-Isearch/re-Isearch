/*

File:        soif.hxx
Version:     1
Description: Harvest SOIF records (derived from bibtex.cxx by Erik Scott)
Author:      Peter Valkenburg
*/
/*
@FILE { http://www.ukoln.ac.uk/metadata/dcdot/
Description{42}:	A CGI based Dublin Core META tag generator
Time-to-Live{7}:	2419200
Refresh-Rate{6}:	604800
Gatherer-Name{6}:	DC-dot
Type{9}:	text/html
MD5{32}:	24766f075e3f96a7b5494fea88659fa8
Keywords{74}:	Dublin Core
DC
generator
editor
Warwick Framework
SOIF
TEI
USMARC
XML
GILS
Title{6}:	DC-dot
Creator{11}:	Andy Powell
Creator-Email{20}:	a.powell@ukoln.ac.uk
Publisher{5}:	UKOLN
Rights{45}:	http://www.ukoln.ac.uk/metadata/dcdot/COPYING
}
*/

//#include <iostream.h>
//#include <ctype.h>
#include "soif.hxx"
#include "doc_conf.hxx"
#include "common.hxx"

#define ATTRIB_SEP '@' 
// <BASE HREF="xxx">
static const char baseHref[] = {'B','A','S','E', ATTRIB_SEP, 'H','R','E','F', 0};



SOIF::SOIF(PIDBOBJ DbParent, const STRING& Name) : BIBTEX(DbParent, Name)
{
}

const char *SOIF::Description(PSTRLIST List) const
{
  List->AddEntry ("SOIF");
  DOCTYPE::Description(List);
  return "SOIF format files.";
}

void SOIF::SourceMIMEContent(STRING *StringPtr) const
{ 
  // MIME/HTTP Content type for BibTeX records
  *StringPtr = "Application/X-SOIF";  
} 



void SOIF::BeforeIndexing()
{
  BIBTEX::AfterIndexing();
}

void SOIF::AfterIndexing()
{
  tmpBuffer.Free();
  BIBTEX::AfterIndexing();
}



void SOIF::ParseRecords(const RECORD& FileRecord)
{
  BIBTEX::ParseRecords(FileRecord);
}

//
//
// The new goal:  scan the record looking for (title = ") and (") pairs
// and mark them as a field named "title".
//
//
/*
The SOIF Grammar is as follows:

    SOIF            ::=  OBJECT SOIF |
                         OBJECT
    OBJECT          ::=  @ TEMPLATE-TYPE { URL ATTRIBUTE-LIST }
    ATTRIBUTE-LIST  ::=  ATTRIBUTE ATTRIBUTE-LIST |
                         ATTRIBUTE
    ATTRIBUTE       ::=  IDENTIFIER {VALUE-SIZE} DELIMITER VALUE
    TEMPLATE-TYPE   ::=  Alpha-Numeric-String
    IDENTIFIER      ::=  Alpha-Numeric-String
    VALUE           ::=  Arbitrary-Data
    VALUE-SIZE      ::=  Number
    DELIMITER       ::=  ":<tab>"
*/

void SOIF::ParseFields(RECORD *NewRecord)
{
  PFILE 	fp;
  GPTYPE 	RecStart,
  RecEnd,
  RecLength,
  ActualLength;
  PCHR 	RecBuffer;

  // Open the file
  const STRING fn ( NewRecord->GetFullFileName() );
  if ((fp = ffopen(fn, "rb")) == NULL)
    {
      message_log (LOG_ERRNO, "SOIF::ParseRecords(): Failed to open '%s'", fn.c_str());
      return;
    }

  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();

  if (RecEnd == 0) {
    if(fseek(fp, 0L, SEEK_END) == -1) {
      message_log (LOG_ERRNO, "SOIF::ParseRecords(): Seek failed - %s", fn.c_str());
      ffclose(fp);
      return;	
    }
    RecStart = 0;
    RecEnd = ftell(fp);
    if(RecEnd == 0) {
      message_log(LOG_ERROR, "SOIF::ParseRecords(): Skipping zero-length record -%s", fn.c_str());
      ffclose(fp);
      return;
    }
    //RecEnd -= 1;
  }

  // Make two copies of the record in memory
  RecLength = RecEnd - RecStart + 1;
  RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1);
  if(RecBuffer == NULL) {
    message_log(LOG_ERRNO, "SOIF::ParseRecords(): Failed to allocate %ld bytes - %s", RecLength + 1, fn.c_str());
    ffclose(fp);
    return;
  }

  if ((ActualLength = ::pfread(fp, RecBuffer, RecLength, (long)RecStart)) != RecLength)
    {
      message_log (LOG_ERRNO, "SOIF::ParseRecords(): Failed to fread %ld bytes, got %ld instead", RecLength, ActualLength);
      if (ActualLength == 0)
	{
	  ffclose(fp);
	  return;
	}
    }
  ffclose(fp);
  RecBuffer[RecLength]='\0';


  // Parse the record and add fields to record structure
  STRING FieldName;
  STRLIST FieldNames;
  FC fc;
  DF df;
  INT val_start;
  INT val_end=0;
  INT val_len;
  DFD dfd;

  PDFT pdft = new DFT();
  if(!pdft) {
    message_log (LOG_ERRNO, "SOIF::ParseRecords(): Failed to allocate DFT - %s", fn.c_str());
    return;
  }

  for (PCHR p = RecBuffer; *p != '\0'; p = RecBuffer + val_end + 1) {
    CHR name[128], c;

    FieldName.Clear();
    // Skip whilespace
    while (isspace(*p)) p++;

    if (*p == '\0')
      break;
    else if (*p == '@' && strncmp(p, "@FILE { ", 8) /* } */ == 0) {
      PCHR q = strchr(p, '\n');
      if (q  == NULL) {
	message_log (LOG_ERROR, "SOIF::ParseRecords(): Badly started record - %s (%ld-%ld)",
                fn.c_str(), RecStart, RecEnd);
        return;
      }
      val_start = (p - RecBuffer) + 8;
      val_end = q - RecBuffer;
      val_len = val_end - val_start;
      strcpy (name, "url");
    } else if (*p == '@' && (strncmp(p, "@DELETE ", 8) == 0 ||
             strncmp(p, "@REFRESH ", 9) == 0 ||
	     strncmp(p, "@UPDATE", 8) == 0)) {
	message_log (LOG_DEBUG, "SOIF::ParseRecords(): Skipping %s", p); 
	return;
    } else if (sscanf(p, "%127[^{:\n \t]{%u}:%c", name, &val_len, &c) == 3 && /* } */
	     c == '\t' && strchr(p, '\t') + val_len < RecBuffer + RecLength) {
      name[127] = '\0';
      val_start = strchr(p, '\t') - RecBuffer + 1;
      val_end = val_start + val_len;
    } else if (c == ' ') {
	message_log (LOG_ERROR, "SOIF::ParseRecords(): Bad Record. Expecting tab @%ld but got space. Bad record-  %s (%ld-%ld).",
		RecStart+p-RecBuffer, fn.c_str(), RecStart, RecEnd);
        return;
    } else if /* { */ (*p != '}') {
        message_log (LOG_ERROR, "SOIF::ParseRecords(): Bad token (offset=%d) in record - %s (%ld-%ld)",
		p-RecBuffer, fn.c_str(), RecStart, RecEnd);
        return;
      }
    else
      break;

    // We have a attr/val pair
    if (RecBuffer[val_end] != '\n') {
      message_log (LOG_ERROR, "SOIF::ParseRecords(): Badly formatted record (missing nl) - %s", fn.c_str());
      return;
    }
   const INT Total = UnifiedNames(name, &FieldNames);
   if (Total)
     {
	fc.SetFieldStart (val_start);
	fc.SetFieldEnd (val_end - 1);
	df.SetFct(fc);
	// Now Walk through list (backwards)..
        for (const STRLIST *p = FieldNames.Prev(); p != &FieldNames; p=p->Prev())
	  {
	    dfd.SetFieldType( Db->GetFieldType( p->Value() ) ); // Get the type added 30 Sep 2003
	    dfd.SetFieldName (p->Value());
	    Db->DfdtAddEntry (dfd);
	    df.SetFieldName (p->Value());
	    pdft->AddEntry (df);
	  }
      }
  }

  NewRecord->SetDft(*pdft);
  delete pdft;
}

bool SOIF::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  StringBuffer->Clear();
  Present(ResultRecord, UnifiedName("URL"), "", StringBuffer);
  return !StringBuffer->IsEmpty();
}

bool SOIF::URL(const RESULT& ResultRecord, STRING *StringBuffer,
        bool OnlyRemote) const
{
  if (GetResourcePath(ResultRecord, StringBuffer))
    return true;
  if (OnlyRemote)
    return false;
  return DOCTYPE::URL(ResultRecord, StringBuffer, OnlyRemote);
}



void SOIF::Present(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, STRING *StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      bool useHTML = (RecordSyntax == HtmlRecordSyntax);
      STRING Value, Author, Object;

      StringBuffer->Clear();
      // Get a Title
      Present (ResultRecord, UnifiedName ("Title"), RecordSyntax, &Value);
      if (Value.GetLength() == 0)
	DOCTYPE::Present (ResultRecord, BRIEF_MAGIC, RecordSyntax, &Value);
      Present (ResultRecord, UnifiedName("Type"), RecordSyntax, &Object);
      Present (ResultRecord, UnifiedName("Author"), SutrsRecordSyntax, &Author);
      if (Author.GetLength() == 0)
	Present (ResultRecord, UnifiedName("Creator"), SutrsRecordSyntax, &Author);
      if (Author.GetLength())
	{
	  if (useHTML)
	    HtmlCat(Author, StringBuffer);
	  else
	    StringBuffer->Cat(Author);
	  StringBuffer->Cat(", ");
	}
      StringBuffer->Cat(Value);
      StringBuffer->Pack();
      if (Object.GetLength())
	*StringBuffer << " (" << Object << ")";
	
    }
  else if (ElementSet.Equals(baseHref))
    {
      URL(ResultRecord, StringBuffer, true);
    } 
  else
    {
      DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}


SOIF::~SOIF()
{
}
