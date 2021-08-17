/* ########################################################################

               Binary Multiple Document Handler (DOCTYPE)

   File: binary.cxx
   Author: Edward C. Zimmermann, edz@nonmonotonic.net


   ########################################################################

   Note: None 

   ########################################################################


   ######################################################################## */

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "common.hxx"
#include "binary.hxx"
#include "doc_conf.hxx"

static const STRING Basename (".BASENAME");
static const STRING Fullpath (".FULLPATH");

/* ------- Binary Support --------------------------------------------- */

BINARY::BINARY (PIDBOBJ DbParent, const STRING& Name):
	DOCTYPE (DbParent, Name)
{
#if USE_LIBMAGIC
  magic_cookie = NULL;
#endif
  SetPostFix();
}

void BINARY::SetPostFix(const STRING& Ext)
{
  if (Ext.IsEmpty())
    PostFix = (char *)",info";
  else
    PostFix = Ext;
}


XBINARY::XBINARY (PIDBOBJ DbParent, const STRING& Name):
        BINARY (DbParent, Name)
{
  SetPostFix(",info.xml");
  if (Getoption("XMLPATHS", "Y").GetBool())
    SetBaseClass("GILSXML");
  else
    SetBaseClass( Getoption("USECLASS") );
}

void XBINARY::SetBaseClass(const STRING& Class)
{
  message_log (LOG_DEBUG, "%s: SetBaseClass(%s)", Doctype.c_str(), Class.c_str());
  if (Class.IsEmpty())
    SetBaseClass("XML");
  else
    InfoDoctype.Set(Doctype + ":" + Class);
  message_log (LOG_DEBUG, "%s: InfoDoctype=\"%s\"", Doctype.c_str(), InfoDoctype.ClassName().c_str());
}


TBINARY::TBINARY (PIDBOBJ DbParent, const STRING& Name):
        XBINARY (DbParent, Name)
{
  SetPostFix(",info.txt");
  SetBaseClass("MEMO");
}



const char *BINARY::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("BINARY");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);

#if USE_LIBMAGIC
 return "Binary files with their plaintext descriptions in ,info files.\n\
- Uses the first sentence for the default headline\n\
- First tries to use the extension of the record source to determine the MIME type\n\
- if that fails it then uses libmagic\n\
NOTE: The reason we look at the extension first is to allow for type spoofing as is\n\
commonly practiced by formats that use archivers such as ZIP.\n";
#else
  return "Binary files with their plaintext descriptions in ,info files.\n\
- Uses the first sentence for the default headline\n\
- Uses the extension to determine the MIME type\n";
#endif
}


const char *XBINARY::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("XBINARY");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  BINARY::Description(List);
  return "Binary files with their XML marked-up descriptions in ,info.xml files.\n\
- Uses the field TITLE for the default headline\n\
- Uses the extension to determine the MIME type\n\n\
Options:\n\
   XMLPATHS specifies if these should be stored (Default ON)\n\
   USECLASS specifies an alternative BASE class (default XML)\n\
Note: XMLPATHS off sets the BASE class to XML. On set it to GILSXML\n\
which also handles TYPE= defines (see the documentation).\n\n\
See also: XMLBASE\n\n";
}


const char *TBINARY::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("TBINARY");
  if ( List->IsEmpty() && Doctype != ThisDoctype && Doctype != "OCR")
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XBINARY::Description(List);
  return "Binary files with their metadata in MEMO-type ,info.txt files.\n\
- Uses the field TITLE for the default headline\n\
- Uses the extension to determine the MIME type\n\
See also the MEMO doctype info\n";
}


GDT_BOOLEAN XBINARY::IsIgnoreMetaField(const STRING &Fieldname) const
{
  if (Fieldname.CaseCompare(UnifiedName("title")))
    return GDT_TRUE;
  DOCTYPE *ptr = Db->GetDocTypePtr(InfoDoctype);
  if (ptr != NULL && ptr != this)
    {
      return ptr->IsIgnoreMetaField(Fieldname);
    }
  return GDT_FALSE;
}

GDT_BOOLEAN TBINARY::IsIgnoreMetaField(const STRING &Fieldname) const
{
  return XBINARY::IsIgnoreMetaField(Fieldname);
}

void BINARY::SourceMIMEContent(PSTRING StringPtr) const
{
  // Default
  *StringPtr = "Application/Octet-Stream";
}

static struct {
  const char *extension;
  const char *content;
  const char *description;
 } Builtin[] = {
  // Add to list..
//  {".txt",	"text/plain", "Plain Text"},
  {".gif",	"image/gif",  "GIF Image"},
  {".iff",      "image/iff", "IFF ILBM Image"},
  {".jpg",	"image/jpeg", "JPEG Image"},
  {".mpg",      "image/mpeg", "MPEG Video"},
  {".tif",	"image/x-tiff", "TIFF Image"},
  {".tiff",	"image/x-riff", "TIFF Image"},
  {".fax",	"image/x-tiff", "TIFF Image"},
  {".bmp",	"image/x-bmp",  "Windows BMP Image"},
  {".pcx",	"image/x-pcx",  "PC Paintbrush Image"},
  {".htm",	"text/html",  "HTML"},
  {".html",	"text/html",  "HTML"},
  {".sgm",	"text/sgml",  "SGML"},
  {".sgml",	"text/sgml",  "SGML"},
  {".gz",	"application/x-gzip", "Gnu Zipped"},
  {".tgz",	"application/x-gzip", "Gnu Zipped"},
  {".Z",	"application/x-compress", "Unix Compressed"},
  {".tar",	"application/x-tar",  "Tar Archive"},
  {".cpio",	"application/x-cpio", "CPIO Archive"},
  {".dvi",	"application/x-dvi",  "TeX DVI"},
  {".tex",	"application/x-tex",  "TeX Source"},
  {".ltx",	"application/x-latex","LaTeX Source"},
  {".latex",	"application/x-latex","LaTeX Source"},
  {".zip",	"application/zip",    "Zip Archive"},
  {".zoo",	"application/x-zoo",  "Zoo Archive"},
  {".lha",	"application/x-lharc","LHarc Archive"},
  {".arc",	"application/x-arc",  "ARC Archive"},
  {".pdf",	"application/pdf", "PDF"},
  {".ps",	"application/postscript", "Postcript"},
  {".eps",      "application/postscript", "Postscript"},
  {".mif",	"application/x-mif",  "Framemaker MIF"},
  {".wp5",	"application/x-wp5",  "WordPerfect 5.x"},
  {".wp4",      "application/x-wp4",  "WordPerfect 4.x"},
  {".doc",      "application/x-msword", "MS Word"},
  {".xls",      "application/x-excel", "Excel spreadsheet"},
  {NULL, NULL, NULL}
};

void BINARY::SourceMIMEContent(const RESULT& ResultRecord, PSTRING StringPtr) const
{
  STRING pathname;
  BINARY::Present(ResultRecord, Basename, NulString, &pathname);
  STRING extension = pathname;
  STRINGINDEX pos = extension.SearchReverse ('.');
  if (pos)
    { 
      extension.EraseBefore(pos);
      for (size_t i = 0; Builtin[i].extension; i++)
	{
	  if (extension.Equals(Builtin[i].extension))
	    {
	      *StringPtr = Builtin[i].content;
	      return; // Got a match
	    }
	}
    }
#if USE_LIBMAGIC
   if (magic_cookie)
     {
        // Have a handle to the magic database
        const char *mime_typ = magic_file(magic_cookie, pathname.c_str());
        if (mime_typ) 
	  {
	     *StringPtr = mime_typ;
             return;
	  }
        
     }
#endif
  // Default
  SourceMIMEContent(StringPtr);
}

void BINARY::ParseRecords(const RECORD& FileRecord)
{
  // Add postfix to point to info "text" file
  const STRING Filename (FileRecord.GetFullFileName( ) );

  if (Filename.Right(',') == PostFix)
    return; // Nothing to do

  STRING Fn (Filename + PostFix);

  if (!FileExists(Fn))
    {
      STRING Ext ( PostFix.Right('.') );
      if (Ext.IsEmpty() || (Ext == PostFix)) Ext = "txt";
      STRING Fn2 (Filename.Before('.') + "." +  Ext );
      if (Fn2 == Filename)
	{
	  message_log (LOG_ERROR, "%s: Could not access '%s'", Doctype.c_str(), Fn.c_str());
          return;
	}
      else if (!FileExists(Fn2))
	{
	  message_log (LOG_ERROR, "%s: Could not access neither of '%s' or '%s'", Doctype.c_str(),
		Fn.c_str(), Fn2.c_str());
	  return;
	}
      Fn = Fn2;

    }

  RECORD Record (FileRecord);
  Record.SetFullFileName(Fn);

  message_log (LOG_INFO, "Using '%s' for '%s' (%s) content.", Fn.c_str(), Filename.c_str(),
	Doctype.c_str());

  Record.SetRecordStart(0);
  Record.SetRecordEnd (GetFileSize(Fn) - 1 );


  Record.SetDate( SRCH_DATE().GetTimeOfFile(Filename));
  Record.SetDateModified( SRCH_DATE().GetTimeOfFile(Fn));
  Record.SetDateCreated( SRCH_DATE().GetTimeOfFileCreation(Filename));


  Db->DocTypeAddRecord(Record);
}


void XBINARY::ParseRecords(const RECORD& FileRecord)
{
  BINARY::ParseRecords(FileRecord);
}


void XBINARY::ParseFields (PRECORD NewRecord)
{
  DOCTYPE_ID oldDoctype ( NewRecord->GetDocumentType() );

  if (oldDoctype == InfoDoctype)
    return; // Done (loop)

  NewRecord->SetDocumentType(  InfoDoctype );
  Db->ParseFields (NewRecord);
  NewRecord->SetDocumentType(oldDoctype);
}


//
//	Method name : ParseFields
//
//	Description : Parse to get the coordinates of the first sentence.
//	Input : Record Pointer
//	Output : None
//
//  A sentence is recognized as ending in ". ", "? ", "! ", ".\n" or "\n\n".
//
//
void BINARY::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      return;		// ERROR
    }

  // Set key to be basename (this helps
  // when downloading binary files)
  RemovePath(&fn); // Get basename
  // remove ,info
  STRINGINDEX x = fn.SearchReverse(PostFix);
  if (x) fn.EraseAfter(x-1);
  if (!Db->MdtLookupKey ( fn ))
    {
      NewRecord->SetKey (fn);
    }

  int Ch;
  INT start = -1, end = 0, next, length;
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan, Start, Newline, Punct} State = Scan;
  while ((Ch = getc(fp)) != EOF) {
    end++;
    if (State == Scan)
      {
	if (!isspace(Ch))
	  State = Start;
	start = end-1;
      }
    else if (Ch == '\r' || Ch == '\n')
      {
	if (State == Newline || State == Punct)
	  break; // Done
	else
	  State = Newline;
      }
    else if (State == Punct || State == Newline)
      {
	if (isspace(Ch))
	  break; // Done
      }
    else if (Ch == '.' || Ch == '!' || Ch == '?')
	State = Punct;
  }		// while
  // Mark end
  next  = end;
  if (State == Newline) end--; // Leave off NL
  if (isspace(Ch)) end--; // Leave off trailing white space
  // Find start of non-white space
  while ((Ch = getc(fp)) != EOF && isspace(Ch))
    next++;

  // Get Length
  length =  GetFileSize(fp);
  Db->ffclose (fp);

  if (start == -1) return; // Nothing

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;

  // Headline (Title)
  STRING FieldName (UnifiedName("Title"));
  dfd.SetFieldName (FieldName);
  Db->DfdtAddEntry (dfd);
  fc.SetFieldStart (start);
  fc.SetFieldEnd (end-1);
  df.SetFct (fc);
  df.SetFieldName (FieldName);
  pdft->AddEntry (df);

  // Whole Document (Description)
  if (next != length)
    {
      FieldName = UnifiedName("Description");
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (next);
      fc.SetFieldEnd (length-1);
      df.SetFct (fc);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
    }
  // Add
  NewRecord->SetDft (*pdft);
  delete pdft;
}


GDT_BOOLEAN BINARY::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING Filename (ResultRecord.GetFullFileName() );
  STRINGINDEX x = Filename.SearchReverse(PostFix);
  if (x)
    {
       Filename.EraseAfter(x-1);
    }
  else
    {
      STRING s, fn = Filename.Before('.');
      for (size_t i = 0; Builtin[i].extension; i++)
	{
	  s = fn + Builtin[i].extension;
	  if (s != Filename && FileExists(s))
	    {
	      Filename = s;
	      break;
	    }
	}
    }
  if (StringBuffer)
    *StringBuffer = Filename;
  return Filename.IsEmpty() == GDT_FALSE;
}


void BINARY::DocPresent (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  STRING Content;

  StringBuffer->Clear();
  if (ElementSet.Equals(SOURCE_MAGIC))
    {
      Content.ReadFile( ResultRecord.GetFullFileName() );
      if (RecordSyntax == HtmlRecordSyntax)
	*StringBuffer << "Content-type: " << "text/plain" << "\n\n";
    }
  else if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      STRING Filename;
      if (GetResourcePath(ResultRecord, &Filename))
	Content.ReadFile(Filename);
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING mime;
	  SourceMIMEContent(ResultRecord, &mime);
	  *StringBuffer << "Content-type: " << mime << "\n\n";
	}
    }
  else
    {
      Present(ResultRecord, ElementSet, RecordSyntax, &Content);
    }
  *StringBuffer << Content;
}

void BINARY::
Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    {
      DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet.Equals (Fullpath))
    {
      STRING s;
      if (GetResourcePath(ResultRecord, &s))
	{
	  if (RecordSyntax == HtmlRecordSyntax)
	    HtmlCat(s, StringBuffer);
	  else
	    *StringBuffer = s;
	}
    }
  else if (ElementSet.Equals(Basename))
    {
      STRING s;
      if (GetResourcePath(ResultRecord, &s))
	{
	  if (RecordSyntax == HtmlRecordSyntax)
	    HtmlCat(RemovePath(s), StringBuffer);
	  else
	    *StringBuffer = RemovePath(s);
	}
    }
  else if (ElementSet.Equals(BRIEF_MAGIC))
    {
      DOCTYPE::Present(ResultRecord, UnifiedName("Title"), RecordSyntax, StringBuffer);
      STRING Path;
      Present(ResultRecord, Basename, RecordSyntax, &Path);
      if (Path.GetLength() > 0)
	*StringBuffer << " (" << Path << ")";
    }
  else
    DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


void BINARY::BeforeIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie == NULL)
     magic_cookie = magic_open(MAGIC_MIME|MAGIC_SYMLINK|MAGIC_ERROR);
  if (magic_cookie) magic_load(magic_cookie, NULL); // Use default 
#endif
}



void BINARY::AfterIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie != NULL)
    {
      magic_close (magic_cookie);
      magic_cookie = NULL;
    }
#endif
}


BINARY::~BINARY ()
{
#if USE_LIBMAGIC
  if (magic_cookie) magic_close(magic_cookie);
#endif
}


// ----------------------------------

GDT_BOOLEAN XBINARY::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  return BINARY::GetResourcePath(ResultRecord, StringBuffer);
}

void XBINARY::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
                const STRING& RecordSyntax, PSTRING StringBuffer) const
{
 BINARY::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

void XBINARY::Present (const RESULT& ResultRecord, const STRING& ElementSet,
                const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  BINARY::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
