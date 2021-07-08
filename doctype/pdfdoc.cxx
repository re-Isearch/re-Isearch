#pragma ident  "@(#)pdfdoc.cxx  1.39 02/24/01 17:45:08 BSN"

/* ########################################################################

   File: pdfdoc.cxx
   Version: 1.13
   Description: Class PDFDOC - PDF documents
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@nonmonotonic.net
   Modified: Fri Dec 29 11:57:19 MET 1995
   Last maintained by: Edward C. Zimmermann


   ########################################################################

   Note: None 

   ########################################################################

   Copyright (c) 1995 : Basis Systeme netzerk. All Rights Reserved.

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.

   ######################################################################## */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.hxx"
#include "pdfdoc.hxx"
#include "process.hxx"
#include "doc_conf.hxx"
#include "mmap.hxx"

static const STRING SOURCE_PATH_ELEMENT ("Source");


// Highlight file format
//
// 
// <XML>
// <Body units=characters|words color=#RRGGBB mode=active|passive version=verNum>
// <Highlight>
// <loc pg=P pos=O len=L>
// </Highlight>
// </Body>
// </XML>
//
// The value of mode specifies whether the Acrobat viewer should 
// automatically go to the first page containing a highlight (active) or display 
// the document's first page until the user clicks the button to manually go to 
// the first highlight (passive).
//
//
// Example
//<Body units=character color=#FF00FF mode=active version=2>
//<Highlight>
//<loc pg=0 pos=0 len=10>
//<loc pg=1 pos=4 len=2>
//</Highlight>
//</Body>
//</XML>
///
//
// The URL that specifies the PDF file must also specify the highlight file, as 
// illustrated in the following example:
//
// http://www.adobe.com/a.pdf#xml=http://www.adobe.com/a.txt
//
//
// Goto page kludge:
//
// <XML>
//<Body units=words color=#FF00FF mode=active version=2>
//<Highlight>
//<loc pg=XXX pos=0 len=0>
//</Highlight>
//</Body>
//</XML>
// Where XXX is the pagenumber
//




//
// Convert PDF file to DB.cat/KEY.txt
//
// Each page is a record KEY:n where n is pagenumber
//
// 10 pages --> KEY:1 KEY:2 KEY:3 KEY:4
// KEY := inode.hostidPDF
//
//

/*
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
*/

/*
Scan from end for /INFO %d %d R
                    ^
                    |__ case independent
Example
/Info 33 0 R

Then Scan from the front of the file for
33 0 obj
<<

From there untill
>>

Is the Info section..

*/

void PDFDOC::parsePDFinfo(const STRING& FileName, LOCATOR *Locator)
{
/*
  struct {
    const char *Tag;
    int length;
    PSTRING value;
  } KeyWords[] = {
    {"Title",         5, Title},
    {"Subject",       6, Subject},
    {"Author",        6, Author},
    {"Keywords",      8, Keywords},
    {"Creator",       7, Creator},
    {"Producer",      8, Producer},
    {"CreationDate", 12, CreationDate},
    {"ModDate",       7, ModDate}
  };
*/
  logf (LOG_DEBUG, "Parse PDF info");

  MMAP MemoryMap (FileName);
  if (!MemoryMap.Ok())
    {
      logf (LOG_ERRNO, "Can't map '%s'", FileName.c_str());
      return;
    }
  const UCHR     *Buffer = MemoryMap.Ptr();
  const size_t    Length = MemoryMap.Size();
  char            directory[126];
  size_t          directory_len = 0;

  // Look for /Info %d %d %c  (last character is typically 'R')
{
  for (size_t i= Length-10; i > 6; i--)
    {
      if (Buffer[i] == '/' && strncasecmp("/Info ", (char *)&Buffer[i], 6) == 0)
	{
	  if (Buffer[i-1] == '\015' || Buffer[i-1] == '\012')
	    {
	      int n1 = 0, n2 = 0;
	      // Found the infomation pointer
	      i += 6;
	      while (Buffer[i] == ' ')
		i++; // Skip white space
	      if (sscanf((const char *)&Buffer[i], "%d %d R", &n1, &n2) == 2)
		{
		  sprintf(directory, "%d %d obj", n1, n2);
		  directory_len = strlen(directory);
		  break;
		}
	    }
	  i -= 7; // Continue looking
	}
    }
}
  if (directory_len == 0)
    {
      logf (LOG_INFO, "Could not find a PDF Info Directory in '%s'!", FileName.c_str());
      return;
    }
  logf (LOG_DEBUG, "Looking for %s in file..", directory);
  // Now we can look/process
  enum { SCANING, LOCATING, EXTRACTING, FIELD, CONTENT } State = SCANING;
  char token[BUFSIZ];
  int  token_len = 0;
  STRING Name;
{
  for (size_t i = 10; i < Length - 50; i++) {
    if (State == FIELD || State == CONTENT)
      {
	if (Buffer[i] == ')' && (Buffer[i+1] == '\015' || Buffer[i+1] == '\012'))
	  {
	    if (token_len)
	      {
		if (State == CONTENT)
		  {
		    token[token_len] = '\0';
		    if (Name.SearchAny("Date")) // Date is special case
		      {
			SRCH_DATE date (token);
			if (date.Ok())
			  Locator->Set(Name, date.ISOdate());
			else
			  Locator->Set (Name, token);
		      }
		    else
		      {
			if ((Name ^= "Creator") && strstr(token, "Scan Plug-in"))
			  {
			    logf(LOG_WARN, "'%s' created with '%s'. \
PDF file probably contains only image data.", FileName.c_str(), token);
			  }
			Locator->Set(Name, token);
		      }
		    logf (LOG_DEBUG, "Set %s with %s", Name.c_str(), token);
		  }
		else
		  logf (LOG_DEBUG, "Still in Field but found end of Content");
		token_len = 0;
	      }
	    State = EXTRACTING;
	  }
	else
	  {
	    if (State == FIELD && Buffer[i] == '(')
	      {
		do {
		  token[token_len] = '\0';
		  if (token_len == 0)
		    break;
		} while (token[--token_len] == ' ');
		// Have the Field Name
		Name = token;
	        token_len = 0;
		State = CONTENT;
	      }
	    else
	      {
	       	token[token_len++] = Buffer[i];
		if (token_len >= BUFSIZ)
		  {
		    logf (LOG_PANIC, "Buffer overflow in PDF info");
		    token_len = 0;
		  }
	      }
	  }
      }
    else if (State == EXTRACTING)
      {
	if (Buffer[i] == '\015' || Buffer[i] == '\012')
	  {
	    if (Buffer[i+1] == '>' && Buffer[i+2] == '>')
	      break; // Done
	  }
	// Want the fields /KEYWORD (....) untill a line with just << on it
	else if (Buffer[i] == '/')
	  {
	    // Start of a field /Author (Edward C. Zimmermann)
	    State = FIELD;
	  }
      }
    else if (State == LOCATING)
      {
	if (Buffer[i] == '<' && Buffer[i+1] == '<')
	  {
	    i += 2;
	    if (Buffer[i] == '\015' || Buffer[i] == '\012')
	      {
		State = EXTRACTING;
	      }
	    else
	      {
		logf (LOG_DEBUG, "PDF Info dictionary is confusing. Probably new style.");
		State = SCANING; // This should NOT HAPPEN!
	      }
	  }
      }
    else if (State == SCANING && (Buffer[i] == '\015' || Buffer[i] == '\012'))
      {
	while (Buffer[i] == '\015' || Buffer[i] == '\012')
	  i++;
	if (memcmp(&Buffer[i], directory, directory_len) == 0)
	  {
	    i += directory_len;
	    State = LOCATING;
	  }
      }
  }
}
  logf (LOG_DEBUG, "Old style Metadata extracted.. Now look for new.." );

  if (pdfinfo_Command.IsEmpty())
    pdfinfo_Command = ResolveBinPath("pdfinfo");


  char *argv[3];
  argv[0] = (char *)pdfinfo_Command.c_str();
  argv[1] = (char *)FileName.c_str();
  argv[2] = NULL;

  fflush(NULL); // Flush all streams
  logf(LOG_DEBUG, "Pipe to '%s'", argv[0]);
  FILE *Fp = _IB_popen(argv, "r");
  if (Fp == NULL) {
    logf(LOG_ERRNO, "Can't open pipe to PDFinfo filter '%s'.", argv[0]);
    return;
  } else {
    char buf[4048];
    char *tcp;
    while (fgets (buf, sizeof(buf)/sizeof(char), Fp) != NULL) {
      if ((tcp = strchr(buf, ':')) != NULL)
	{
	  *tcp++ = '\0';
	  while (isspace(*tcp)) tcp++;
	  if (*tcp && strncmp(buf, "File size", 9) ) // Don't add File size
	    {
	      STRING token(tcp);
	      STRING Name(buf);

	      token.Trim();
	      Name.Replace(" ", "_");
	      if (Name.SearchAny("Date")) // Date is special case
		{
		  SRCH_DATE date (token);
		  if (date.Ok())
		    token = date.ISOdate();
                }
	      Locator->Set(Name, token);
	    }
	}
    }
    _IB_pclose(Fp);
    logf (LOG_DEBUG, "Metadata info extracted..");
  }
}

static long HostID = 0;


PDFDOC::PDFDOC (PIDBOBJ DbParent, const STRING& Name): METADOC (DbParent, Name)
{
  if (HostID == 0)
    {
      HostID = _IB_Hostid(); 
    }
  METADOC::SetPresentStyle(GDT_TRUE);
  METADOC::SetSepChar('=');

  pdf2jpeg_Command = ResolveBinPath("pdf2jpeg.bin");
  pdf2pdf_Command = ResolveBinPath("pdf2pdf.bin");
}


// CreationDate is Date

void PDFDOC::LoadFieldTable()
{
  if (Db)
    {
      Db->AddFieldType("CreationDate", FIELDTYPE::date);
      Db->AddFieldType("ModDate", FIELDTYPE::date);
      Db->AddFieldType("File-Date", FIELDTYPE::date);
      Db->AddFieldType("Record-Date", FIELDTYPE::date);
      Db->AddFieldType("PDF_version", FIELDTYPE::numerical);
      Db->AddFieldType("Size", FIELDTYPE::computed);
    }
  DOCTYPE::LoadFieldTable();
}

NUMERICOBJ PDFDOC::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  if (FieldName ^= "Size")
    {
#ifdef _WIN32
      unsigned long x;
      if (sscanf(Buffer.c_str(), "%lu bytes", &x) == 1)
        return (DOUBLE)x;
#else
      long long x;
      if (sscanf(Buffer.c_str(), "%lld bytes", &x) == 1)
	return (DOUBLE)x;
#endif
    }
  return NUMERICOBJ();
}


static const char myDescription[] = "OLD Adobe PDF Plugin";

const char *PDFDOC::Description(PSTRLIST List) const
{
  List->AddEntry ("PDFDOC");
  DOCTYPE::Description(List);
  return "Adobe Portable Document Format (PDF)\n\n\
Special presentation elements are \"PDF\" and  \"JPEG\"";
}


void PDFDOC::SourceMIMEContent(PSTRING StringPtr) const
{
  // Default
  *StringPtr = "Application/pdf";
}


static int Str2Hash(const STRING& Str, int modulo)
{
  int Value=0;
  const unsigned char *idxstr = (const unsigned char *)Str.c_str();
  const size_t   len = Str.GetLength();

  if (len > 0)
    {
      register size_t t = 0;
      register int    st = *idxstr;

      Value += st;
      do {
        Value+=((st)*(st))-t++;
        st = *++idxstr;
      } while (t < len);
    }
  return Value % modulo;
}


void PDFDOC::ParseRecords(const RECORD& FileRecord)
{
//cerr << "PDFDOC::ParseRecords" << endl;
  STRING Fn;
  char filename[1024];
  int pageno;
  long start, end;
  struct stat stbuf;
  STRING s;

  Db->ComposeDbFn (&s, DbExtCat);
  if (stat(s, &stbuf) == -1 || !S_ISDIR(stbuf.st_mode))
    unlink(s); // ignore result

  FileRecord.GetFullFileName (&Fn);
  if (_IB_lstat(Fn, &stbuf) == -1)
    {
      logf(LOG_ERRNO, "Can't stat '%s'.", Fn.c_str());
      return;
    }

  s.Cat (STRING().form("/%x_%s/%lx", Str2Hash(Fn, 311), Doctype.c_str(), (stbuf.st_ino) % 263) );

  // Create directory
#define mask (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
  if (MkDirs(s , mask) == -1)
    {
      logf (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return;
    }

  STRING key, outfile;
  key.form("%lx%lx", stbuf.st_ino, HostID);
  outfile.form ("%s/%s", s.c_str(), key.c_str());

  if (pdf2text_Command.IsEmpty())
    pdf2text_Command = ResolveBinPath("pdf2text");

  STRING catfile = outfile +".cat";

  char *argv[4];
  argv[0] = (char *)pdf2text_Command.c_str();
  argv[1] = (char *)Fn.c_str();
  argv[2] = (char *)catfile.c_str();
  argv[3] = NULL;

  fflush(NULL); // Flush all streams
  logf(LOG_DEBUG, "Pipe to \"%s\"",  argv[0]);
  FILE *Fp = _IB_popen(argv, "r");
  if (Fp == NULL)
    {
      logf(LOG_ERRNO, "%s can't open pipe to PDF filter '%s'.",
	Doctype.c_str(), argv[0]);
      return;
    }
  else
     logf(LOG_DEBUG, "Running '%s'", argv[0]);
  logf (LOG_INFO, "Processing PDF in '%s'", Fn.c_str());

  unsigned page_count = 0;
  RECORD Record (FileRecord);

  // pump filename with the input to bootstrap error messages
  strncpy(filename, Fn.c_str(), sizeof(filename)/sizeof(char));
  filename[sizeof(filename)/sizeof(char)-1] = '\0';
  while (!feof(Fp))
    {
      int c = 0;
      if ((c = fscanf(Fp, "< %d %ld %ld %s >\n",
	&pageno, &start, &end, filename)) == 4)
	{
	  if (start == -1 && end == -1)
	    {
	      logf(LOG_WARN, "PDF Input is %s.", filename);
	    }
	  else
	    {
	      page_count++;
	      s.form("%s:%d", key.c_str(), pageno);
	      Record.SetKey (s);
	      Record.SetFullFileName (filename);
	      Record.SetRecordStart (start);
	      Record.SetRecordEnd (end);
	      Db->DocTypeAddRecord(Record);
	    }
	}
      else
	{
	  logf(LOG_ERROR, "Problems parsing PDF filter output from %s", filename);
	  if (c == 0)
	    break;
        }
    }
  _IB_pclose(Fp);

  if (page_count == 0)
    {
      logf(LOG_ERROR, "Input did not contain any (parseable) PDF pages!");
      unlink (catfile);
      return;
    }
  else
    {
      struct stat stbuf2;
      if (stat(catfile, &stbuf2) == -1)
	logf(LOG_ERROR, "Can't access PDF filter output!");
      else if (stbuf2.st_size <= (page_count*3 + 1))
	logf(LOG_WARN, "PDF filter could not extract text from '%s'. Graphics?", Fn.c_str());
      else
	logf(LOG_INFO, "Processed %d PDF pages", page_count);
    }

  // Add whole PDF to PDFINFO for info processing
  if ((Fp = fopen(outfile, "w")) != NULL) {
    STRING          basename (Fn);
    STRING          path (Fn);
    STRING          capfile;

    RemovePath(&basename);
    RemoveFileName(&path);

    logf (LOG_DEBUG, "Create Locator for %s", (const char *)path);

    LOCATOR        Locator(path);

    STRING         Owner (ResourceOwner(path));

    if (Owner.GetLength())
      Locator.Set("Owner", Owner);
    
    STRING         Publisher (ResourcePublisher(path));

    if (Publisher.GetLength())
      Locator.Set("Publisher", Publisher);

    parsePDFinfo(Fn, &Locator);
    if (FileExists ( capfile = (path + ".cap/" + basename)))
      {
	// Contents of .capfile over-ride what we have!!
	PFILE fp = fopen(capfile, "rb");
	if (fp)
	  {
	    logf (LOG_DEBUG, "Parsing .cap file '%s'", capfile.c_str());
	    // we're going to read the capfile, look for a name= line, and emit
	    // the part after "Name=".
	    STRING linebuff;
	    while (linebuff.FGet(fp,1024))
	      {
		if (linebuff.CaseCompare("Name=", 5) == 0)
		  {
		    // The line must be a name= line, so do something.
		    linebuff.EraseBefore(6);
		    Locator.Set("Title", linebuff);
		  }
		else if (linebuff.CaseCompare("Publisher=", 10) == 0)
		  {
		    linebuff.EraseBefore(11);
		    Locator.Set ("Publisher", linebuff);
		  }
		else if (linebuff.CaseCompare("Volume=", 7) == 0)
		  {
		    linebuff.EraseBefore(8);
		    Locator.Set ("Collection", linebuff);
		  }
		else if (linebuff.CaseCompare("Original=", 9) == 0)
		  {
		    linebuff.EraseBefore(10);
		    Locator.Set ("Original-URL", linebuff);
		  }
	      } // while
	    fclose(fp);
	  }
      }
    else
      logf (LOG_DEBUG, "Building locator");
    if (Locator.IsEmpty("Title"))
      Locator.Set("Title", basename);
    Locator.Set("Handle", key);
    Locator.Set("File-Date", ISOdate(stbuf.st_mtime));
    Locator.Set("Record-Date", ISOdate(0));
    Locator.Set(SOURCE_PATH_ELEMENT, (Db && Db->getUseRelativePaths()) ? Db->RelativizePathname(Fn) : Fn);

    {
      char tmp[125];
      sprintf(tmp,
#ifdef _WIN32
	"%I64d bytes (%d pages)"
#else
	"%lld bytes (%d pages)"
#endif
	, (long long)stbuf.st_size, page_count);
      Locator.Set("Size", tmp);
    }
    Record.SetRecordStart(ftell(Fp));

    logf(LOG_DEBUG, "Dumping Metadata to '%s'", outfile.c_str());
    STRLIST x;
    x.AddEntry("locator"); 
    Locator.Text(x).Print(Fp);
    Record.SetRecordEnd(ftell(Fp) - 1);
    fclose (Fp);

    if (!Locator.IsEmpty("CreationDate"))
      {
	Record.SetDate(SRCH_DATE( Locator.Get("CreationDate") ));
      }
    else
      Record.SetDate(SRCH_DATE(& stbuf.st_mtime));
    Record.SetKey(key);
    Record.SetFullFileName(outfile);
    logf (LOG_DEBUG, "Passing to indexer");
    Db->DocTypeAddRecord(Record);
  } else {
    logf(LOG_ERRNO, "Can't create PDF Info record '%s'.", outfile.c_str());
  }
}

void PDFDOC::ParseFields (PRECORD NewRecord)
{
  STRING Key ( NewRecord->GetKey() );
  if (Key.Search(':') == 0)
    {
//cerr << "Pass off to METADOC" << endl;
      METADOC::ParseFields(NewRecord);
    }
  else
    {
//cerr << "Parse my own fields" << endl;
      off_t start = NewRecord->GetRecordStart ();
      off_t end   = NewRecord->GetRecordEnd ();
      // Jam everything into CONTENT
      if (end - start > 3)
	{
	  // A page *must* have more than 3 characters
          // since the filter ends a page with \n\f\n
	  STRING FieldName;
	  PDFT pdft;
	  DFD dfd;

	  pdft = new DFT ();
	  FieldName = "CONTENT";
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);

	  FC fc;
	  fc.SetFieldStart (0);
	  fc.SetFieldEnd (end - start - 3); // Forget the last 3 characters

	  DF df;
	  df.SetFct (fc);
	  df.SetFieldName (FieldName);
 	  pdft->AddEntry (df);
	  NewRecord->SetDft (*pdft);
	  delete pdft;
	}
      else
	logf (LOG_INFO, "No content!");
    }
}

GDT_BOOLEAN PDFDOC::GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  PDFDOC::Present(ResultRecord, SOURCE_PATH_ELEMENT, NulString, StringBuffer);
  return StringBuffer->GetLength() != 0;
}


void PDFDOC::DocPresent (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  STRING S;
  StringBuffer->Clear();
  if (ElementSet.Equals("HIGHLIGHT"))
    {
      const STRING Key (ResultRecord.GetKey());
      int pageno = Key.SearchReverse(':');
      if (pageno) pageno = atoi(Key.c_str() + pageno);
      if (pageno > 1) pageno--;
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  StringBuffer->Cat ("Content-type: application/xml\n\n");
  	}
      StringBuffer->Cat (ResultRecord.GetXMLHighlightRecordFormat(pageno));
    }
  else if (ElementSet.Equals("PDF"))
    {
      if (RecordSyntax == HtmlRecordSyntax)
        StringBuffer->Cat ("Content-type: Application/pdf\n\n");
      const STRING Key (ResultRecord.GetKey());
      int pageno = Key.SearchReverse(':');
      if (pageno) pageno = atoi(Key.c_str() + pageno);
      if (pageno > 1) pageno--;

      if (GetResourcePath(ResultRecord, &S))
	{
	  STRING Pageno (pageno);
	  char *argv[4];
	  argv[0] = (char *)pdf2pdf_Command.c_str();
	  argv[1] = (char *)Pageno.c_str();
	  argv[2] = (char *)S.c_str();
	  argv[3] = NULL;

	  logf (LOG_DEBUG, "Use I/O pipe '%s'", argv[0]);
	  FILE *Fp = _IB_popen(argv, "r");
	  if (Fp != NULL) {
	    int ch;
	    while ((ch = getc(Fp)) != EOF)
	      StringBuffer->Cat((char)ch);
	    _IB_pclose(Fp);
	  } else
	    logf (LOG_ERRNO, "Could not open Pipe '%s'", argv[0] );
	}
    }
  else if (ElementSet.Equals("JPEG"))
    {
      if (RecordSyntax == HtmlRecordSyntax)
        StringBuffer->Cat ("Content-type: image/jpeg\n\n");

      const STRING Key (ResultRecord.GetKey());
      int pageno = Key.SearchReverse(':');
      if (pageno) pageno = atoi(Key.c_str() + pageno);
      if (pageno > 1) pageno--;

      if (GetResourcePath(ResultRecord, &S) )
	{
	  STRING Pageno (pageno);
	  char *argv[4];
	  argv[0] = (char *)pdf2jpeg_Command.c_str();
	  argv[1] = (char *)Pageno.c_str();
	  argv[2] = (char *)S.c_str();
	  argv[3] = NULL;

	  logf (LOG_DEBUG, "Use I/O pipe '%s'", argv[0]);
	  FILE *Fp = _IB_popen(argv, "r");
	  if (Fp != NULL) {
	    for (int ch; (ch = getc(Fp)) != EOF;)
	    StringBuffer->Cat((char)ch);
	    _IB_pclose(Fp);
	  } else
	    logf (LOG_ERRNO, "Could not open Pipe '%s'", argv[0] );
	}
    }
  else if (ElementSet.Equals(SOURCE_MAGIC))
    {
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING content;
	  SourceMIMEContent(&content);
	  StringBuffer->Cat ("Content-type: ");
	  StringBuffer->Cat (content);
	  StringBuffer->Cat ("\n\n");
	}
      if (GetResourcePath(ResultRecord, &S))
	StringBuffer->CatFile(S);
    }
  else if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals("TEXT") ||
	   (ElementSet ^= UnifiedName("CONTENT", &S)))
    {
      ResultRecord.GetKey(&S);
      STRINGINDEX x = S.SearchReverse(':');
      if (x == 0)
	{
	  METADOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	  return;
	}

      if (RecordSyntax == HtmlRecordSyntax)
	{
	  RESULT RsRecord;
	  RsRecord = ResultRecord;
	  int pageno = 0;
	  if (x)
	    {
	      pageno = atoi(((const char *)S) + x);
	      S.EraseAfter(x - 1);
	    }
	  RsRecord.SetKey(S); // Look at parent document
	  STRING Href, Title, URL;
	  Db->URLfrag(RsRecord, &Href);

	  // Get the URL to the PDF source document..
          Present(ResultRecord, "URL", NulString, &URL);
	  if (URL.GetLength() == 0)
	    {
	      URL << CGI_FETCH << Href << "+" << SOURCE_MAGIC;
	    }
	  Present (RsRecord, BRIEF_MAGIC, RecordSyntax, &Title);
	  // Convert to HTML
	  HtmlHead (ResultRecord, ElementSet, StringBuffer); 
	  // TODO: Insert link to PDF file here
	  *StringBuffer << "<TABLE><TR>\
<TH VALIGN=\"Top\" ALIGN=\"Left\"><A HREF=\"" << URL << "\" \
onMouseOver=\"self.status='Click here to view the PDF document'; return true\">\
<IMG BORDER=0 ALIGN=Middle \
SRC=\"/IB/Asset/pdf.gif\" ALT=\"PDF Source\"></A></TH><TD><A HREF=\"" << URL;
	  if (pageno)
	    {
	      STRING HTTP_Server;
	      if (Db->GetHTTP_server(&HTTP_Server))
		{
		  STRING bin;
		  char *tp = getenv("SCRIPT_NAME");
		  if (tp) {
		     bin = tp;
		     RemoveFileName (&bin);
		  } else
		     bin = "/cgi-bin/";
		  *StringBuffer << "#xml=" << HTTP_Server << bin << "igoto?" << pageno
		  << "\" onMouseOver=\"self.status='Click here to view the PDF document p."
		  << pageno << "'; return true";
		}
	    }
	 *StringBuffer << "\">" << Title << "</A> &nbsp;&nbsp;&nbsp<A HREF=\"" << CGI_FETCH << Href << "+F\" \
onMouseOver=\"self.status='Click here to view the PDF document info'; return true\">\
<IMG BORDER=0 ALIGN=AbsMiddle SRC=\"/IB/Asset/info.gif\" ALT=\"PDF Info\"></A></TD></TR>\
<TR><TH VALIGN=\"TOP\" ALIGN=\"Left\">";
	if (pageno > 0 && ElementSet.Equals(FULLTEXT_MAGIC))
	  {
	     *StringBuffer << "p. " << pageno
		<< "</TH><TD><A HREF=\""
		<< CGI_FETCH << Href << ":" << pageno
		<< "+TEXT\" onMouseOver=\"self.status=\
'Click here to view the text'; return true\" \
TITLE=\"p. " << pageno << " text\"><IMG BORDER=0 SRC=\"pdf2jpeg"
		<< Href << ":" << pageno
		<< "\" ALT=\"Image of p." << pageno
		<< "\"></A></TD></TR>";
	  }
	else
	  {
	    STRING Content;
	    // Get Raw Record Content
	    ResultRecord.GetRecordData (&Content);
	    *StringBuffer << "p." << pageno << " Text:<BR><BR><A HREF=\""
		<< CGI_FETCH << Href << ":" << pageno << "\" \
onMouseOver=\"self.status='Click here to view the image'; return true\">\
<IMG BORDER=0 ALIGN=Middle VSPACE=20 SRC=\"/IB/Asset/image.gif\" ALT=\"Image\"></A></TH><TD>";


	    if (Content.GetLength() <= 3 && Content.GetChr(2) == '\f')
	      {
		*StringBuffer << "\
<font face=\"Arial,Helvetica\" color=\"#ff0000\" size=\"+2\"><EM>Image Data Only</EM></font>";
	      }
	    else
	      {
		*StringBuffer << "<PRE><FONT SIZE=2>";
		HtmlCat (Content, StringBuffer); // Convert to HTML
		StringBuffer->Cat ("</FONT></PRE>");
	      }
	    StringBuffer->Cat("</TD></TR>");
	  }
	  StringBuffer->Cat ("</TABLE>");
	  HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
	}
      else
	{
	  STRING Content;
	  // Get Raw Record Content
	  ResultRecord.GetRecordData (&Content);
	  *StringBuffer = Content;
	}
    }
  else
    {
      METADOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}

void PDFDOC::
Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  STRING S;
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC) ||
	ElementSet.Equals("PDF") || ElementSet.Equals("JPEG"))
    {
      DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet.Equals (BRIEF_MAGIC))
    {
      GDT_BOOLEAN UseHtml = (RecordSyntax == HtmlRecordSyntax);
      RESULT RsRecord;
      RsRecord = ResultRecord;
      RsRecord.GetKey(&S);
      STRINGINDEX x = S.SearchReverse(':');
      INT pageno = 0;
      INT pagecount = 0;
      if (x)
	{
	  pageno = atoi(((const char *)S) + x);
	  S.EraseAfter(x - 1);
	}
      RsRecord.SetKey(S); // Look at parent document
      Present (RsRecord, "Title", RecordSyntax, StringBuffer);
      if (pageno)
	{
	  if (UseHtml)
	    *StringBuffer << " [<EM>p.</EM> <VAR>" << pageno << "</VAR>]";
	  else
	    *StringBuffer <<  " [p. " << pageno << "]";
	}
      else
	{
	  Present (RsRecord, "Size", RecordSyntax, &S);
	  if ((x = S.Search('(')) > 0)
	    {
	      if ((pagecount = atoi(((const char *)S) + x)) > 0)
		{
		  if (UseHtml)
		    {
		      *StringBuffer << " (<VAR>"
			<< pagecount << "</VAR> <EM>pages</EM>)";
		    }
		  else
		    {
		      *StringBuffer << " (" << pagecount << " pages)";
		    }
		}
	    }
	}
    }
  else if (ElementSet ^= UnifiedName("CONTENT", &S))
    {
      ResultRecord.GetKey(&S);
      STRINGINDEX x = S.SearchReverse(':');
      if (x == 0)
	{
	  MDTREC Mdtrec;
	  // Send the content, use key:1 for the path
	  S.Cat(":1");
	  if (Db->GetMainMdt()->GetMdtRecord (S, &Mdtrec))
	    {
	      // Now grab Fn
	      S = Db->ResolvePathname( Mdtrec.GetFullFileName() );
	      if (RecordSyntax == HtmlRecordSyntax)
		{
		  STRING Content;
		  Content.ReadFile(S);
		  *StringBuffer = "<PRE>";
		  HtmlCat(Content, StringBuffer);
		  StringBuffer->Cat ("</PRE>");
		}
	      else
		StringBuffer->ReadFile(S);
	    }
	}
      else
	DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet ^= SOURCE_PATH_ELEMENT)
    {
      ResultRecord.GetKey(&S);
      STRINGINDEX x = S.SearchReverse(':');
      if (x)
	{
	  S.EraseAfter(x - 1);
	}
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING URL;
	  Present(ResultRecord, "URL", NulString, &URL);
	  if (URL.GetLength() == 0)
	    {
	      STRING furl;
	      Db->URLfrag(&furl);
	      URL << CGI_FETCH << furl << S << "+" << SOURCE_MAGIC;
	    }
	 *StringBuffer << "<A HREF=\"" << URL << "\">PDF Source</A>";
	}
      else if (x == 0)
	{
	  Db->GetFieldData (ResultRecord, ElementSet, StringBuffer);
	}
      else
	{
	  RESULT RsRecord (ResultRecord);
	  RsRecord.SetKey(S);
	  Db->GetFieldData (RsRecord, ElementSet, StringBuffer);
	}
    }
  else if (ElementSet == "URL")
    {
      DOCTYPE::URL(ResultRecord, StringBuffer, GDT_TRUE); 
    }
  else
    DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

PDFDOC::~PDFDOC ()
{
}

// Stubs for dynamic loading
extern "C" {
  PDFDOC *  __plugin_pdfdoc_create (IDBOBJ * parent, const STRING& Name)
  {
    return new PDFDOC (parent, Name);
  }
  int          __plugin_pdfdoc_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_pdfdoc_query (void) { return myDescription; }
}

