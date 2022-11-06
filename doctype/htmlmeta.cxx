/*-@@@
File:           htmlmeta.cxx
Version:        2.0
Description:    Class HTMLMETA - HTML documents, <HEAD> only
Author:         Edward C. Zimmermann <edz@nonmonotonic.net>, Nassib Nassar <nassar@etymon.com>
@@@*/

#pragma ident  "@(#)htmlmeta.cxx"

static const int HTMLMETA_MAX_TOKEN_LENGTH=16383;


//#include <stdio.h>
//#include <ctype.h>
//#include <string.h>
//#include <iostream.h>
#include "isearch.hxx"
#include "htmlmeta.hxx"
#include "metadata.hxx"
#include "doc_conf.hxx"

#define true 1
#define false 0


#if 0
class ZTokens {
public:
  ZTokens();
  ~ZTokens();

  const char * operator[](INT Oid);
  int  operator[](const STRING& Name);
};

struct _ZTokens {
  int token,
  const char *name
} _ZTokens[] = {
  {1,  "Control-Identifier"},
  {2,  "Street-Address"},
  {3,  "City "},
  {4,  "State-or-Province"},
  {5,  "Zip-or-Postal-Code"},
  {6,  "Hours-of-Service"},
  {7,  "Resource-Description"},
  {8,  "Technical-Prerequisites"},
  {9,  "West-Bounding-Coordinate"},
  {10, "East-Bounding-Coordinate"},
  {11, "North-Bounding-Coordinate"},
  {12, "South-Bounding-Coordinate"},
  {13, "Place-Keyword"},
  {14, "Place-Keyword-Thesaurus"},
  {15, "Time-Period-Structured"},
  {16, "Time-Period-Textual"},
  {17, "Linkage "},
  {18, "Linkage-Type"},
  {19, "Record-Source "},
  {20, "Controlled-Term"},
  {21, "Subject-Thesaurus"},
  {22, "Subject-Terms-Uncontrolled"},
  {23, "Original-Control-Identifier"},
  {24, "Record-Review-Date"},
  {25, "General-Access-Constraints"},
  {26, "Originator-Dissemination-Control"},
  {27, "Security-Classification-Control"},
  {28, "Order-Information"},
  {29, "Cost"},
  {30, "Cost-Information"},
  {31, "Schedule-Number"},
  {32, "Language-of-Resource"},
  {33, "Medium"},
  {34, "Language-of-Record"},
  {35, "Relationship"},
  {51, "Purpose"},
  {52, "Originator"},
  {53, "Access-Constraints"},
  {54, "Use-Constraints"},
  {55, "Order-Process"},
  {56, "Agency-Program"},
  {57, "Sources-of-Data"},
  {58, "Methodology"},
  {59, "Supplemental-Information"},
  {70, "Availability"},
  {71, "Spatial-Domain"},
  {90, "Distributor"},
  {91, "Bounding-Coordinates"},
  {92, "Place"},
  {93, "Time-Period"},
  {94, "Point-of-Contact"},
  {95, "Controlled-Subject-Index"},
  {96, "Subject-Terms-Controlled"},
  {97, "Subject-Terms-Uncontrolled"},
  {99, "Available-Linkage"}
};
#endif

static inline _ib_priority_t PagePriority(const STRING& Filename)
{
  int count = 0;
  for (const char *tp = Filename.c_str(); *tp; tp++)
    {
      if (*tp == '/') count++;
    }
  return (_ib_priority_t)count;
}

HTMLMETA::HTMLMETA(PIDBOBJ DbParent, const STRING& Name):COLONDOC(DbParent, Name)
{
  fieldTableLoaded = 0;
  defaultDirMetadata = Getoption("DefaultMetadata", ".default.metadata.xml") ;
  message_log (LOG_DEBUG, "Default directory metadata filename = '%s'", defaultDirMetadata.c_str());
}

void HTMLMETA::SourceMIMEContent(STRING *stringPtr) const
{
  stringPtr->AssignCopy(9, "text/html");
}

void HTMLMETA::SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const
{
  static const STRING Encoding ("$Content-Encoding");
  DOCTYPE::Present (Record, Encoding, StringPtr);
  if (StringPtr->IsEmpty())
    SourceMIMEContent(StringPtr);
}


const char *HTMLMETA::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("HTMLMETA");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  COLONDOC::Description(List);
  return "\
HyperText Markup Language (W3C HTML)\n\
  Indexes fields in the <HEAD>..</HEAD> , resp. <METAHEAD>..</METAHEAD> data secion\n\
  using types as defined (among otherplaces) in the Dublin Core. Both <META .. />\n\
  and <HTTP-EQUIV .. /> constructs are supported. The special \"dc.indentifier\" content\n\
  is handled as hash and date fields as of type date.\n\n\
  Supports .meta and .xml files to include supplemental ex-vitro metadata:\n\
    <filename>.meta   Lines with value pairs field:value (colondoc) are handled as\n\
                      <META name=\"field\"> value=\"value\" />. Lines starting with\n\
                      < are copied literally. Comment lines start with # (or :)\n\
    <filename>.xml    XML/SGML-like marked-up fields (Registry format).\n\
    If a file named \".default.metadata.xml\" or set via the index time option DefaultMetadata)\n\
    exists in the directory its values are used as defaults which can be over-written by <filename>.xml\n\n\
  Search time Options:\n\
    MIRROR_LAYOUT // Specifies the base of the HTML tree (root)\n\
    This can also be specified in the environment as MIRROR_LAYOUT or in the\n\
    doctype ini as MIRROR_LAYOUT=fmt in the [General] section.";
}

STRING& HTMLMETA::DescriptiveName(const STRING& FieldName, PSTRING Value) const
{
  bool http_equiv = (FieldName.GetChr(1) == '$');

  if (http_equiv)
    *Value = FieldName.c_str() + 1;
  else
    *Value = FieldName;
  if (!Value->IsEmpty())
    {
      // Lowercase
      Value->ToLower();
      // Make first Character UpperCase
      Value->SetChr(1, toupper(Value->GetChr(1)));
      // Replace '-' or '_' to ' '
      Value->Replace("-", " ");
      Value->Replace("_", " ");
    }
  if (http_equiv)
    Value->Cat( " (HTTP-EQUIV)" );
  return *Value;
}


bool HTMLMETA::Summary(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  const char *fields[] = {
    "META.DC.DESCRIPTION",
    "META.DESCRIPTION",
    "DC.DESCRIPTION",
    "DESCRIPTION",
    NULL};
  STRING summary;
  for (size_t i=0; fields[i]; i++)
    {
      Present (ResultRecord, fields[i], RecordSyntax, &summary);
      if (summary.GetLength())
        break;
    }
  if (StringBuffer) *StringBuffer = summary;
  return summary.GetLength() != 0;
}

bool HTMLMETA::URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote) const
{
  // <BASE HREF="xxx"> -> $LOCATION like <META HTTP-EQUIV="LOCATION" VALUE="xxx">
  STRING url;
  DOCTYPE::Present(ResultRecord, "BASE", &url);
  if (url.GetLength()) // Have base
    {
      url.Cat ( ResultRecord.GetFileName() );
    }
  else
    {
      DOCTYPE::Present(ResultRecord, "$LOCATION", &url);
      if (url.IsEmpty())
	return DOCTYPE::URL(ResultRecord, StringBuffer, OnlyRemote);
    }
  if (StringBuffer) *StringBuffer = url;
  return url.GetLength() != 0;
}



// Like strstr but case independent..
static const char *cstrstr(const char *token, const char *what)
{
  const size_t tlen = strlen(token);
  const size_t len  = strlen(what);
  const char   ch   = toupper(what[0]);
  for (size_t i=0; token[i] && (tlen - i >= len); i++)
    {
      if (toupper(token[i]) == ch)
	{
	  if (strncasecmp(token+i, what, len) == 0)
	    return &token[i];
	}
    }
  return NULL;
}

//  $EXPIRES is date
//  $LAST-MODIFED is date
//  $REVIST is X days or X years 

void HTMLMETA::LoadFieldTable()
{
  if (fieldTableLoaded) return;
  if (Db)
    {
      Db->AddFieldType("$EXPIRES", FIELDTYPE::date);
      Db->AddFieldType("$LAST-MODIFIED", FIELDTYPE::date);
      Db->AddFieldType("$REVIST", FIELDTYPE::computed);
//   Db->AddFieldType("dc.date", FIELDTYPE::date); // Other dates come via "date" in name
      Db->AddFieldType("dc.identifier", FIELDTYPE::hash);
      fieldTableLoaded++;
    }
//else cerr << "Delayed table load" << endl;
  DOCTYPE::LoadFieldTable();
}


SRCH_DATE HTMLMETA::ParseDate(const STRING& Buffer, const STRING& FieldName) const
{
  STRING val (Buffer);
  val.Strip(STRING::both);
  const char Today[] = "Today";

  // if the string is long then its a real date
  if (val.GetLength() > 20)
    return DOCTYPE::ParseDate(val);

  int    value;
  char   tmp[20];
  if (val.IsNumber())
    {
      // Value of 11 is Now+11 days
      if ((value = val.GetInt()) < 999)
        {
	  SRCH_DATE date (Today);
	  date. PlusNdays (value);
	  return date;
        }
    }
  else if (sscanf(val.c_str(), "%d  %[a-zA-Z]", &value, tmp) == 2)
    {
      // Should be NN Days we we'll accept NN day etc.
      if (strncasecmp(tmp, "Day", 3) == 0)
	{
	  SRCH_DATE date (Today);
	  date.PlusNdays (value);
	  return date;
	}
      else if (strncasecmp(tmp, "Month", 5) == 0)
	{
	  SRCH_DATE date (Today);
	  date.PlusNmonths (value);
	  return date;
	}
      else if (strncasecmp(tmp, "Year", 4) == 0)
	{
	  SRCH_DATE date (Today);
	  date.PlusNyears (value);
	  return date;
	}
    }
  return DOCTYPE::ParseDate(val);
}

NUMERICOBJ HTMLMETA::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  return DOCTYPE::ParseComputed(FieldName, Buffer);

}

void HTMLMETA::BeforeIndexing()
{
  COLONDOC::BeforeIndexing();
}


void HTMLMETA::AfterIndexing()
{
  tagBuffer.Free();
  COLONDOC::AfterIndexing();
}


size_t HTMLMETA::CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn, off_t Start, off_t End) const
{
  size_t lines = 0;
  STRING metaInfoFile (Fn + ".meta");
  STRING metaInfoFileXml (Fn + ".xml");
  // If absolute filepath use it and ONLY it!
  STRING dirMeta ( IsAbsoluteFilePath(defaultDirMetadata) ?
	defaultDirMetadata : AddTrailingSlash(RemoveFileName(Fn)) + defaultDirMetadata );

  if (outFp && (FileExists(metaInfoFile) || FileExists(metaInfoFileXml) || FileExists(dirMeta)))
    {
      int sawHead = 0;
      int Ch;
      FILE *sfp = fopen(Fn, "r");

      if (sfp == NULL) return 0;

      if (fseek(sfp, Start, SEEK_SET) == -1)
	{
	  fclose(sfp); return 0;
	}
      // -1 means unlimited since it never goes down to Zero.
      off_t    BytesToRead = (End == 0 ? -1 : End - Start + 1);

      METADATA mbuf ("meta");

      while ((Ch = getc(sfp)) != EOF)
	{
	  if (Ch == '<' && !sawHead)
	    {
	      do {
		Ch = getc(sfp);
	      } while (isspace(Ch));
	      if (Ch == 'M' || Ch == 'm')
		{
		  sawHead++; // <metahead 
		}
	      else if (Ch == 'H' || Ch == 'h')
		{
		  int Ch2 = getc(sfp);
		  // Can be <HTML or <HEAD .. >
		  // We check only HE or he..
		  if (Ch2 == 'E' || Ch2 == 'e')
		    sawHead++;
		  ungetc(Ch2, sfp); // Push the character back)
		}
	      fputc('<', outFp);
	      fputc(Ch,  outFp);
	    }
	  else
	    fputc(Ch, outFp); // Write the character
	  if (sawHead && Ch == '>')
	    break;
	}
      if (!sawHead) fprintf(outFp, "<METAHEAD>\n");
	
      FILE *inp = fopen(metaInfoFile, "r");
      if (inp)
         {
            STRING S;
	    char  *tp;
            while (S.FGet (inp))
              {
                S.Pack (); // Remove Duplicate and trailing spaces
                char *fld = (char *)(S.c_str());

		if (*fld == '#' || *fld == ':')
		  {
		    continue; // Comment line
		  }
		else if (*fld == '<')
                  {
                    fprintf(outFp, "%s\n", fld);
                    lines++;
                  }
		else if ((tp = strchr(fld, ':')) != NULL)
                  {
                   *tp++ = '\0';
                    // Should have at most one space but..
                    while (*tp && isspace(*tp)) tp++;
                    if (*tp)
                      {
		        // Have a line so spit it out
			char quote1 = strchr(fld, '"') ? '\'' : '\"';
		        char quote2 = strchr(tp, '"') ? '\'' : '\"';
			int  mixed_fld = (strchr(fld, quote1) != NULL);
			int  mixed_tp  = (strchr(tp, quote2) != NULL);

			// Strip trailing spaces in field names
			for (char *end_fld = fld + strlen(fld) - 1; end_fld > fld; )
			  {
			    if (isspace(*end_fld)) *end_fld-- = '\0';
			    else break;
			  }

			// Make sure we don't have ' and " in text...
			if (mixed_fld || mixed_tp)
			  {
			    STRING name (fld);
			    STRING value (tp);
			    if (mixed_fld)
			      {
				name.Replace("\"", "&quot;");
				quote1 = '\"';
			      }
			    if (mixed_tp)
			      {
				value.Replace("\"", "&quot;");
				quote2 = '\"';
			      }
			    fprintf(outFp, "\n<META NAME=%c%s%c VALUE=%c%s%c />", quote1, name.c_str(), quote1,
				quote2, value.c_str(), quote2); 
			  }
			else // Normal line
			  fprintf(outFp, "\n<META NAME=%c%s%c VALUE=%c%s%c />", quote1, fld, quote1,
				 quote2, tp, quote2);
                        lines++;
                      }
                  }
		else if (!ispunct(*fld)) // We'll be quiet on punctuation
		  {
		    message_log (LOG_DEBUG, "%s: Encountered non meta line in '%s': %s",
			Doctype.c_str(), metaInfoFile.c_str(), fld);
		  }
              }
            fclose(inp);
          }
      if ((inp = ffopen(dirMeta, "r")) != NULL)
	{
	  mbuf.Read(inp);
	  ffclose(inp);
	}
      if ((inp = fopen(metaInfoFileXml, "r")) != NULL)
	{
	  mbuf.Add(inp);
	  fclose(inp);
	}

//cerr << "XML: " << mbuf.Metadata()->Sgml() << endl;
//cerr << "HtmlMeta: " << mbuf.Metadata()->HtmlMeta() << endl;

      mbuf.HtmlMeta().Print(outFp);

      if (!sawHead) fprintf(outFp, "\n</METAHEAD>\n");
      int oldCh = -1;
      while ((BytesToRead-- != 0) && (Ch = getc(sfp)) != EOF)
	{
	  if (!(isspace(Ch) && isspace(oldCh)))
	    fputc(Ch, outFp);
	  oldCh = Ch;
	}
      fclose(sfp); // close the input stream
    }
  return lines;
}


void HTMLMETA::ParseRecords (const RECORD& FileRecord)
{
  const STRING filename (  FileRecord.GetFullFileName() );
  if (Db && Exists(filename) && Exists(filename+".meta"))
    {
      const char out_ext[] = ".html";
      // Need to cat in .meta
      STRING key, s, outfile;
      unsigned long version = 0;

      INODE Inode(filename);

      if (!Inode.Ok())
	{
	  if (Inode.isDangling())  
	    message_log(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), filename.c_str());
	  else
	    message_log(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), filename.c_str());
	  return;
	}
      if (Inode.st_size == 0)
	{
	  message_log(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", filename.c_str());
	  return;
	}
      off_t start = FileRecord.GetRecordStart();
      off_t end   = FileRecord.GetRecordEnd();

      if ((end == 0) || (end > Inode.st_size) ) end = Inode.st_size;

      if ((key = FileRecord.GetKey()).IsEmpty())
	key = Inode.Key(start, end); // Get Key
      while (Db->KeyLookup (key)) key.form("%s.%ld", s.c_str(), ++version); 
      // Now we have a good key

      message_log (LOG_DEBUG, "%s: Key set to '%s'", Doctype.c_str(), key.c_str());

      Db->ComposeDbFn (&s, DbExtCat);
      if (MkDir(s, 0, true) == -1)
	{
	  message_log (LOG_ERRNO, "%s: Can't create filter directory '%s'", Doctype.c_str(), s.c_str() );
	  return;
	}
      // <db_ext>.cat/<Hash>/<Key>.html
      outfile =  AddTrailingSlash(s);
      outfile.Cat (((long)key.CRC16()) % 1000);
      if (MkDir(outfile, 0, true) == -1)
	outfile = s; // Can't make it
      AddTrailingSlash(&outfile);
      outfile.Cat (key);

      if (out_ext[0])
	outfile.Cat (out_ext);

      FILE *fp;

      if ((fp = fopen(outfile, "w")) == NULL)
	{
	  message_log (LOG_ERRNO, "%s: Could not create '%s'", Doctype.c_str(), outfile.c_str());
	  return;
	}
      CatMetaInfoIntoFile(fp, filename, start, end);
      off_t  recordLength = ftell(fp);
      fclose(fp);

      if (recordLength < 5)
	{
	  unlink(outfile);
	  message_log (LOG_WARN, "%s: Skipping '%s': Contained %d chars text?", Doctype.c_str(),
		FileRecord.GetFileName().c_str(), (int)recordLength);
	  return;
	}

      RECORD NewRecord(FileRecord);
      // We now have a record in outfile from 0 to len
      NewRecord.SetRecordStart (0);
      NewRecord.SetRecordEnd ( recordLength - 1 ); // To end
      NewRecord.SetFullFileName ( outfile );
      NewRecord.SetKey( key ); // Set the key since we did the effort

      NewRecord.SetOrigPathname ( filename ); //

      // Set some default dates
      SRCH_DATE mod_input;
      mod_input.SetTimeOfFile(filename);
      NewRecord.SetDateModified ( mod_input );
      NewRecord.SetDate ( mod_input );

      STRING urifile;
      if (Db->_write_resource_path(outfile, FileRecord, &urifile) == false)
	message_log (LOG_ERRNO, "%s: Could not create '%s'", Doctype.c_str(), urifile.c_str());

      Db->DocTypeAddRecord(NewRecord);
      return;
    }
  if (Db)
    {
#if 0
       FILE *fp;
       if ((fp = ffopen(filename, "rb")) != NULL)
	{
	  char buf[BUFSIZ];
	  fgets (buf, BUFSIZ+1, fp);
	  ffclose(fp);
	  if (memcmp(buf, "%PDF-", 5) == 0)
	    {
	      message_log (LOG_WARN, "File %s is NOT HTML but PDF!", filename.c_str());
	      RECORD Record(FileRecord);
	      DOCTYPE_ID Id("PDF");
	      if (Id.IsDefined())
		{ 
		  Record.SetDocumentType (Id);
		  Db->ParseRecords (Record);
		  return;
		}
	    }
	  int closed_brackets = 0;
	  int err = 1;
	  char *tcp = buf;
	  for (; *tcp; tcp++)
	    {
	       if (*tcp == '<')
		 {
		   if (closed_brackets < 4)
		    err = 0; // Its HTML
		   break;
		 }
	       if (*tcp == '>')
		{
		  closed_brackets++;
		}
	    }
	  if (err && (tcp - buf > 512))
	    {
              message_log (LOG_WARN, "File %s is NOT HTML! Trying autodetection.", filename.c_str());
              RECORD Record(FileRecord);
              DOCTYPE_ID Id("AUTODETECT");
              if (Id.IsDefined())
                {
		  err
                  Record.SetDocumentType (Id);
                  Db->ParseRecords (Record);
		  return;
                }
	    }
	}
#endif
       Db->DocTypeAddRecord (FileRecord);
    }
}

/* TODO:

Add body tag search for:

<abbr title="United Nations">UN</abbr>
<acronym title="World Wide Web">WWW</acronym>

and perhaps
<address>..</address>

*/

void HTMLMETA::ParseFields(RECORD *NewRecord)
{
  unsigned       tagCount = 0;
  // open the file
  FILE           *fp;
  const STRING    filename ( NewRecord->GetFullFileName() );

  if ((fp = ffopen(filename, "rb")) == NULL) {
    message_log (LOG_ERRNO, "%s::ParseFields(): Failed to open file '%s'",
	Doctype.c_str(), filename.c_str());
    NewRecord->SetBadRecord();
    return;
  }

//cerr << "HTMLMETA::ParseFields" << endl;;

  long            RecordStart = NewRecord->GetRecordStart();
  long            RecordEnd   = NewRecord->GetRecordEnd();

  if  (RecordEnd == 0) RecordEnd = GetFileSize(fp);

  STRING          Key;
  int             inHead = 0;	// 1 if we are in the <HEAD> ... </HEAD> section
  int             metaHead = 0; // 0 if <HEAD>, 1 if <METAHEAD>

  int             done = 0;	// 1 if it's time to stop parsing
  int             in_quote;     // Am I in a quote?

  char           *token = (char *)tagBuffer.Want(HTMLMETA_MAX_TOKEN_LENGTH + 1);
  int             tokenLength;	// maintained while we're still building the string

  int             foundTag = 0;	// 1 if we hit a tag and are building a token

  long            position = (RecordStart > 0) ? 
			fseek(fp,  RecordStart, SEEK_SET) : 0;	// offset position within input file

  long            tokenPosition;// offset position of the beginning of the token

  int             tokenReady;	// 1 if the token string is ready to be processed

  long            titlePosition;// offset position of start of title (after <TITLE>)

  DFD             dfd;
  DFT             dft;
  FC              fc;
  DF              df;


  LoadFieldTable();


  if (position == -1) // Error Condition
    {
      ffclose(fp);
      message_log (LOG_ERRNO, "%s::ParseFields(): Failed to seek on '%s' to RecordStart (%ld)",
        Doctype.c_str(), filename.c_str(), RecordStart);
      NewRecord->SetBadRecord();
      return;
    }

  // Handle Web Server response here
  /*
HTTP/1.1 200 OK
Date: Thu, 03 Nov 2022 09:08:26 GMT
Server: Apache
Last-Modified: Wed, 06 Feb 2019 13:33:00 GMT
ETag: "a3-58139c3bbbd2e"
Accept-Ranges: bytes
Content-Length: 163
Connection: close
Content-Type: text/html

*/

  // For now we skip to start of HTML but later we want to parse
  // metadata header
  // 2022 Edward Zimmermann
  //
  // Will be mainly interested in ETag, Last-Modified and Expires data
  //
 
  int ch;
  if ((ch = fgetc(fp)) == 'H') {
    bool first_line = true;
    long parse_start = position; // The original start of the record to parse
    // we can use the token buffer since it is not being used yet!
    char *line;
    char *tp;
    // NOTE: 1024 is much less than the size of token (HTMLMETA_MAX_TOKEN_LENGTH)
    while ((line = fgets(token,  1024, fp)) != NULL) {
cerr << "LINE: " << line << endl;
       if (*line == '\0' || *line == '\r' || *line == '\n') // Back line seperating META from HTML
	    break;
       if (first_line) {
	  // NOTE: We ate the H so looking at TTP/ for HTTP/
	  if (memcmp(line, "TTP/", 4) != 0) { // Not really a HTTP response head
	     fseek(fp, parse_start ,SEEK_SET);
	     break; // Just garbage at the top so ...
	  }
	  first_line = false;
       } else if ( (tp = strchr(line, ':')) != NULL &&  (tp != line) ) {
	 // search for : as in key: value
	 // :vvv is not key: value
         long field_start = (tp - line) + position;
	 long field_end   = strlen(line) + position - 1; //  -1
	 *tp = '\0';

	 bool   add_field = true;

	 const STRING  fieldname (line);

	 if (fieldname.CaseEquals("ETag")) {
	   Key = tp + 1;
	   Key.removeWhiteSpace(); // No spaces are ever to be expected !!!!
           dfd.SetFieldType( FIELDTYPE::text ); // Key is just text 
	 } else if (fieldname.CaseEquals("Expires")) {
	   // Expires: <http-date>
	   dfd.SetFieldType( FIELDTYPE::ttl_expires ); // TTL 
	 } else if (fieldname.CaseEquals("Last-Modified")) {
	   // What the server things the date of the resource is
	   STRING value = tp + 1;
	   NewRecord->SetDate( value );
	   dfd.SetFieldType( FIELDTYPE::date);
	 } else if (fieldname.SearchAny("Date")) { // Some date field?
	   dfd.SetFieldType( FIELDTYPE::date ); // No date ranges in meta
	 } else if (fieldname.CaseEquals("Content-Language")) {
	    // Set the language	
	    STRING value = tp + 1;
	    value.removeWhiteSpace(); // No spaces are ever to be expected !!!!
	    NewRecord->SetLanguage (value);
	    dfd.SetFieldType( FIELDTYPE::text );
	 } else if (fieldname.CaseEquals("Content-Type")) {
	    // Content-Type: text/html; charset=utf-8
	    // TODO: Read charset and set !
	    dfd.SetFieldType( FIELDTYPE::text );
	 } else dfd.SetFieldType( FIELDTYPE::text );


	 // Question: Do we index all the meta headers? 
	 // For now we do but we may want to restrict which ones we accept...
	 // Want, of course, things like Digest ...
	 if (add_field) {
	   dfd.SetFieldName( fieldname );
           Db->DfdtAddEntry(dfd);
           fc.SetFieldStart( field_start );
           fc.SetFieldEnd( field_end );
           df.SetFct(fc);
           df.SetFieldName( fieldname );
           dft.AddEntry(df);
	 }
	}
       else break; // No key:value ...
       position = ftell(fp); // keep track of where we are
    } // while
  } else ungetc(ch, fp);

  // main parsing loop

  while (!done && position < RecordEnd) {
    // int           ch;

    // get next token (i.e. the next HTML tag)
    tokenLength = 0;
    foundTag = 0;
    tokenReady = 0;
    in_quote = 0;
    do {
      if ((ch = fgetc(fp)) == EOF) {
	token[tokenLength] = '\0';
      } else {
	switch (ch) {
	case 0:
	  message_log (LOG_WARN, "<NUL> character (\\000) in '%s'(%ld) stream. Are you sure its %s (HTML)? Setting record as bad.",
		filename.c_str(), position, Doctype.c_str());
	  ffclose(fp);
	  NewRecord->SetBadRecord();
	  return;
	case 1: case 2: case 3: case 4: case 5: case 6:
	case 7: case 8: case 12: case 14: case 15: case 16:
	case 17: case 18: case 19: case 20: case 21: case 22:
	case 23: case 24: case 25: case 26: case 27: case 28:
	case 29: case 30: case 31: case 127:
	  message_log (LOG_DEBUG, "%s Shunchar Control 0x%x encountered in '%s'(%ld)!",
		Doctype.c_str(), (int)ch, filename.c_str(), position);
	  break;
	case '<':
	  if (!foundTag) {
	    foundTag = 1;
	    tokenPosition = position;
	  }
	  token[tokenLength++] = '<';
	  break;
	case '>':
	  if (foundTag && !in_quote) {
	    token[tokenLength++] = '>';
	    token[tokenLength] = '\0';
	    foundTag = 0;
	    tokenReady = 1;
	    tagCount++;
	  }
	  break;
	case '"':
	case '\'':
	  // Quote
	  if (foundTag) {
	    if (in_quote)
	      {
		if (ch == in_quote)
		  in_quote = 0;
	      }
	    else in_quote = ch ;
	    token[tokenLength++] = (char)ch;
	  }
	  break;
	case '!':
	  if (foundTag && tokenLength == 1) {
	    foundTag = 0;
	    tokenLength = 0;
	    // Skip <!DOCTYPE, <!-- --> etc
	    position++;
	    if ((ch = fgetc(fp)) == '-') {
	      position++;
	      if ((ch = fgetc(fp)) == '-') {
		  while ((ch = fgetc(fp)) != EOF) {
		    position++;
		    if (ch == '-') {
		      position++;
		      if ((ch = fgetc(fp)) == '-')
			break;
		    }
		  }
	       }
	    }
	  }
	// fall into...
	default:
	  if (foundTag)
	    token[tokenLength++] = (char)ch;
	  break;
	}
	position++;
      }
    } while ((!tokenReady) && (ch != EOF) && (tokenLength < HTMLMETA_MAX_TOKEN_LENGTH));

    if (ch == EOF) {
      done = true;
      break;
    }
    // process token

    if (inHead) {
      // we're in the <HEAD> section, so we do want to process this
      if (metaHead) {
	if (TagMatch(token, "/METAHEAD", 9)) {
	  done = true;
	  break;
	}
      } else if (TagMatch(token, "/HEAD", 5)) {
	done = true;
	break;
      }
      if (TagMatch(token, "TITLE", 5)) {
	titlePosition = tokenPosition + 7;
      } else if (TagMatch(token, "/TITLE", 6)) {
	STRING          fieldName ("TITLE");

	dfd.SetFieldName(fieldName);
	dfd.SetFieldType( FIELDTYPE::text ); // Get the type added 30 Sep 2003


	Db->DfdtAddEntry(dfd);
	fc.SetFieldStart(titlePosition);
	fc.SetFieldEnd(tokenPosition - 1);
	df.SetFct(fc);
	df.SetFieldName(fieldName);
	dft.AddEntry(df);
      } else if (TagMatch(token, "LINK", 4)) {
	// <LINK REL="ToC" --> LINK.REL:ToC
	// <LINK REV="Maker" --> LINK.REV:Maker
        // CONTENT is the HREF="...."
        size_t          nlen = 4; // strlen("REL=") and strlen("REV=")
	const char     *object = "LINK.REL:";
        const char     *name = (char *)cstrstr(token + nlen+1, "REL=");
        const size_t    clen = 5; // strlen("HREF=")
        const char     *content = cstrstr(token + nlen+1, "HREF=");

	if (name == NULL)
	  {
	    object = "LINK.REV:";
	    name = (const char *)cstrstr(token + nlen+1, "REV=");
	  }

        if (name && content) {
          char             nameq = name[nlen];  // quote?
          char             contq = content[clen];

          if (nameq != '\'' && nameq != '"')
            nameq = 0;
          if (contq != '\'' && contq != '"')
            contq = 0;

          const char      *contentEndQuote;
          for (contentEndQuote = content + clen + 1; *contentEndQuote; contentEndQuote++) {
            if (contq && *contentEndQuote == contq)
              break;
            if (contq == 0 && (isspace(*contentEndQuote) || *contentEndQuote == '>'))
              break;
          }
          name += nlen + (nameq ? 1 : 0);
          if (contentEndQuote && *contentEndQuote) {
            // extract NAME value
            int             x = 0;
            while ((name[x] != nameq) && (name[x] != '\0')) {
              if (nameq == 0 && (isspace(name[x]) || name[x] == '>'))
                break;
              x++;
            }
	    STRING          fieldName (object);
	    STRING          subField (name, x); // Fieldname with x characters from name

	    fieldName.Cat (subField);
            // now build the position data
            long            contentStart = tokenPosition + (content - token) + clen + (contq ? 1 : 0);
            long            contentEnd = tokenPosition + (contentEndQuote - token) - 1;

	    if (fieldName.Length() == 0)
	      {
		message_log(LOG_WARN, "NIL FieldName encountered in %s.", filename.c_str());
	      }
	    else
	      {
		if (fieldName.SearchAny("Date"))
		  dfd.SetFieldType( FIELDTYPE::date);
		else
		  dfd.SetFieldType( Db->GetFieldType(fieldName) ); // Get the type added 30 Sep 2003
		dfd.SetFieldName(fieldName);
		Db->DfdtAddEntry(dfd);
		fc.SetFieldStart(contentStart);
		fc.SetFieldEnd(contentEnd);
		df.SetFct(fc);
		df.SetFieldName(fieldName);
		dft.AddEntry(df);
	      }
           }
	}
      } else if (TagMatch(token, "META", 4)) {
	size_t          clen = 8; // strlen("CONTENT=")
	const char     *content = cstrstr(token + 6, "CONTENT=");
	const size_t    zlen  = 3; // strlen("Z=")
	const char     *ztoken  = cstrstr(token + 6, "Z=");
#if 0
	const char     *schema  = cstrstr(token + 6, "SCHEME=");
	const char     *type    = cstrstr(token + 6, "TYPE=");
#endif

	size_t          nlen;
	char           *name;
	if (ztoken == NULL)
	  {
	    if ((name = (char *)cstrstr(token + 6, "NAME=")) == NULL)
	      {
		name = (char *)cstrstr(token+6, "HTTP-EQUIV=");
		nlen = 11; // strlen("HTTP-EQUIV")
	      }
	    else
	      nlen = 5; // strlen("NAME=")
	  }
	else
	  {
	    name = (char *)ztoken;
	    nlen = zlen;
	  }
	if (content == NULL)
	  {
	    content = cstrstr(token + 6, "VALUE=");
	    clen = 6; 
	  }
	if (name && content) {
	  char             nameq = name[nlen];	// quote?
	  char             contq = content[clen];

	  if (nameq != '\'' && nameq != '"')
	    nameq = 0;
	  if (contq != '\'' && contq != '"')
	    contq = 0;

	  const char      *contentEndQuote;
	  for (contentEndQuote = content + clen + 1; *contentEndQuote; contentEndQuote++) {
	    if (contq && *contentEndQuote == contq)
	      break;
	    if (contq == 0 && (isspace(*contentEndQuote) || *contentEndQuote == '>'))
	      break;
	  }
	  name += nlen + (nameq ? 1 : 0);
	  if (contentEndQuote && *contentEndQuote) {
	    // extract NAME value
	    int             x = 0;
	    while ((name[x] != nameq) && (name[x] != '\0')) {
	      if (nameq == 0 && (isspace(name[x]) || name[x] == '>'))
		break;
	      x++;
	    }
	    if (nlen == 11) // Is HTTP-EQUIV
	      {
		// Which HTTP-EQUIV do we want to ignore?
		struct {
		  const char *tag;
		  size_t      len;
		} IgnoreEQUIV[] = {
		  {"REFRESH", 7},
		  {"CACHE-", 6},
		  {"EXT-", 4}, // EXT-CACHE
		  {"PRAGMA", 6},
		  {"SET-COOKIE",  10},
		  {"WINDOW-", 7},
		  {NULL, 0}
		};
		int skip = 0;
		if (strncasecmp(name, "ETag", 4) == 0)
		  {
		    size_t  len = contentEndQuote - content - clen - (contq ? 1 : 0);
		    STRING Value(content + clen + (contq ? 1 : 0), len);
		    Key = Value;
		  }
		else if (strncasecmp(name, "Content-Language", 16) == 0)
		  {
                   size_t  len = contentEndQuote - content - clen - (contq ? 1 : 0);
                    STRING Value(content + clen + (contq ? 1 : 0), len);
                    LOCALE  locale(Value);

                    if (locale.Ok())
		      NewRecord->SetLocale( locale );
                    else
                      message_log (LOG_WARN, "Could not parse HTTP-EQUIV Locale '%s'", Value.c_str());
		  }
		else if (strncasecmp(name, "Last-Modified", 4) == 0)
		  {
		    // Have a HTTP-EQUIV="LAST-MODIFIED"
		    size_t  len = contentEndQuote - content - clen - (contq ? 1 : 0);
		    STRING Value(content + clen + (contq ? 1 : 0), len);
		    SRCH_DATE Date (Value);
		    if (Date.Ok())
		      NewRecord->SetDate( Date );
		    else
		      message_log (LOG_WARN, "Could not parse HTTP-EQUIV Date '%s'", Value.c_str());
		  }
		else
		for (size_t i=0; skip == 0 && IgnoreEQUIV[i].len; i++)
		  {
		    if (strncasecmp(name, IgnoreEQUIV[i].tag, IgnoreEQUIV[i].len) == 0)
		      skip = 1;
		  }
		if (skip)
		  continue; // Ignore this one
		// Kludge (saves a prepend)
		*--name = '$'; // Mark all HTTP-EQUIV with '$' as the first character
		x++; // Prepend $ to the fieldname
	      }
	    STRING          fieldName (name, x); // Fieldname with x characters from name

	    // now build the position data
	    long            contentStart = tokenPosition + (content - token) + clen + (contq ? 1 : 0);
	    long            contentEnd = tokenPosition + (contentEndQuote - token) - 1;

	    // dc.date.created is the "official" element name for..
	    // creation_date, creationdate etc.
	    if (fieldName.CaseCompare ("CREAT", 5) == 0 && fieldName.SearchAny("DATE"))
	      {
		fieldName = "dc.date.created";
	      }
	    else if (
		(fieldName.CaseCompare ("LAST-MOD", 8) == 0) ||
		(fieldName.CaseCompare ("$LAST-MOD", 9) == 0) ||
		(fieldName.CaseCompare ("MODIFI", 6) == 0 && fieldName.SearchAny("DATE"))  )
	      {
		fieldName = "dc.date.modified";
	      }
	    else if (fieldName.CaseCompare ("$dc.") == 0)
	      {
		fieldName.EraseBefore(2);  // Zap the $
	      }

	    if (KeyField ^= fieldName)
	      {
		STRING Value(&content[contentStart], contentEnd-contentStart+1);
		Key = Value;
	      }
	    else if (LanguageField ^= fieldName)
              {
		STRING Value(&content[contentStart], contentEnd-contentStart+1);
		NewRecord->SetLanguage (Value);
	      }
	    else if (DateField ^= fieldName)
	      {
		STRING Value(&content[contentStart], contentEnd-contentStart+1);
		SRCH_DATE Date (Value);
		if (Date.Ok())
		  NewRecord->SetDate( Date );
		else
		  message_log (LOG_WARN, "Could not parse %s Date '%s'", fieldName.c_str(), Value.c_str());
	      }

	    if (fieldName.GetLength() != 0)
	      {
		if (fieldName.GetChar(0) == '<')
		  message_log(LOG_WARN, "Oddball Field '%s' encountered in %s",
			fieldName.c_str(), filename.c_str());

		// Check if "date", 26 Dec 2007
		FIELDTYPE ft (  Db->GetFieldType(fieldName) );
		if (!ft.Defined() && fieldName.SearchAny("Date"))
		  {
		    ft = FIELDTYPE::date;
		    Db->AddFieldType(fieldName, ft);
		  }
		dfd.SetFieldType( ft );

		dfd.SetFieldName(fieldName);
		Db->DfdtAddEntry(dfd);
		fc.SetFieldStart(contentStart);
		fc.SetFieldEnd(contentEnd);
		df.SetFct(fc);
		df.SetFieldName(fieldName);
		dft.AddEntry(df);
	      }
	  }
	}
      }
    } else if (TagMatch(token, "HEAD", 4)) {
      inHead = 1;
      metaHead = 0;
    } else if (TagMatch(token, "METAHEAD", 8)) {
      inHead = 1;
      metaHead = 1;
    }
  }
  ffclose(fp);

#if 0
  // Make a field from Position to end:
  if ((RecordEnd - position) > 10)
    {
      STRING fieldname("..TAIL");
      dfd.SetFieldName(fieldname);
      Db->DfdtAddEntry(dfd);
      fc.SetFieldStart(position);
      fc.SetFieldEnd(RecordEnd);
      df.SetFct(fc);
      df.SetFieldName(fieldname);
      dft.AddEntry(df);
    }
#endif

  if (tagCount == 0)
    message_log (LOG_WARN, "File %s was not of type %s (no tags)!",
	filename.c_str(), Doctype.c_str() );

  NewRecord->SetDft(dft);
  if (!Key.IsEmpty())
    {
      if (Db->KeyLookup (Key))
	{
	  message_log (LOG_WARN, "Record in \"%s\" used a non-unique %s '%s'",
		filename.c_str(), KeyField.c_str(), Key.c_str());
	  Db->MdtSetUniqueKey(NewRecord, Key);
	}
      else
        NewRecord->SetKey (Key);
    }
  NewRecord->SetPriority( PagePriority(filename) );
}


INT HTMLMETA::ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, StringPtr, Offset, Length);
  if (count)
    Entities.normalize((char *)(StringPtr->c_str()), count); // WARNING: WARNING: WARNING (Hack!)
  return count;
}

INT HTMLMETA::ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, Buffer, Offset, Length);
  if (count)
    Entities.normalize(Buffer, count);
  return count;
}


INT HTMLMETA::GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, StringPtr, Offset, Length);
  if (count)
    Entities.normalize2((char *)(StringPtr->c_str()), count); // WARNING: WARNING: WARNING (Hack!)
  return count;
}

INT HTMLMETA::GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, Buffer, Offset, Length);
  if (count)
    Entities.normalize2(Buffer, count);
  return count;
}


// Used by GetIndirect Buffer.. Need to read more!!
INT HTMLMETA::GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length)
{
  int segment = 10;
  if (Offset > segment)
    Offset -= segment;
  else
    segment = 0;
  size_t        want = Length*4 + 12 + segment; // Should be enough
  char         *tmp = (char *)tagBuffer.Want(want+1); 
  if (tmp == NULL)
    {
      message_log (LOG_PANIC|LOG_ERRNO, "%s GetTerm buffer exhausted, wanted %d bytes", Doctype.c_str(), want);
      Buffer[0] = '\0';
      return 0;
    }
  int           count = 0;
  PFILE         fp = ffopen(Filename, "rb");
  if (fp)
    {
      count = DOCTYPE::ReadFile(fp, tmp, Offset, want);
      ffclose(fp);
    }
  if (count)
    {
      if (segment)
	{
	  if (tmp[segment] == ';')
	    {
	      while (--segment > 0)
		{
		  if (tmp[segment] == '&')
		    {
		      if (segment) segment--;
		      break;
		    }
		}
	    }
	  count -= segment;
	  tmp += segment;
	}
      Entities.normalize2(tmp, count);

      int i = 0, j = 0;
      for (;j < count && tmp[j] == ' '; j++)
	/* loop */;
      while ((size_t)i < Length && j < count)
	{
	  if ((Buffer[i++] = tmp[j]) == ' ')
	    while (tmp[++j] == ' ');
	  else
	    j++;
	}
      Buffer[i] = '\0';
      count = i; // FIX edz May 2006
    }
  else
    Buffer[0] = '\0';

  if ((size_t)count > Length)
    message_log (LOG_PANIC, "%s::GetTerm: read too much", Doctype.c_str());

  return count;
}


GPTYPE HTMLMETA::ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
  GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength)
{
  // Convert "&amp;xxx &lt;yyyy" to
  //         "&xxxx    <yyyy   "

  Entities.normalize((char *)DataBuffer, DataLength);
  return DOCTYPE::ParseWords(DataBuffer, DataLength, DataOffset, GpBuffer, GpLength);
}


HTMLMETA::~HTMLMETA()
{
}

// returns 1 if tag is of type tagType.
// e.g. if tag[] == "<META NAME=\"AUTHOR\" CONTENT=\"Nassar\">"
// and tagType[] == "META"
// then TagMatch will return 1
int HTMLMETA::TagMatch(char *tag, const char *tagType) const
{
  if (tag == NULL)
    return 0;
  return TagMatch(tag, tagType, strlen(tagType));
}


int HTMLMETA::TagMatch(char *tag, const char *tagType, size_t len) const
{
  // check first character
  if (tag == NULL || *tag != '<') {
    return 0;
  }
  while (isspace(*tag))
    tag++; // Skip while space
  // iterate tagType[] and compare (case-insensitive) with tag
  for (size_t i = 0; i < len; i++) {
    if (toupper(tag[i + 1]) != /* toupper */ (tagType[i])) {
      return 0;
    }
  }
#define isTAGC(_c) (isalnum(_c) || (_c) == '.' || (_c) == '-' || (_c) == '_')
  // now just make sure that was really the end of the tag
  return (!isTAGC(tag[len + 1]));
}

void HTMLMETA::DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
  const STRING& RecordSyntax, STRING* StringBufferPtr) const
{
  if (ElementSet == METADATA_MAGIC) 
    XmlMetaPresent(ResultRecord, RecordSyntax, StringBufferPtr);
  if (ElementSet == FULLTEXT_MAGIC)
    Present(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
  else
    COLONDOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}

void HTMLMETA::XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax, STRING *StringBuffer) const
{
  HTMLMETA::Present(ResultRecord, METADATA_MAGIC, RecordSyntax, StringBuffer);
}

void HTMLMETA::Present (const RESULT& ResultRecord, const STRING& ElementSet,
 const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet == BRIEF_MAGIC)
    {
      DOCTYPE::Present (ResultRecord, "title", StringBuffer);
      if (StringBuffer->IsEmpty())
	{
	  URL(ResultRecord, StringBuffer, false);
	}
      else
	StringBuffer->Pack();
      return;
    }
  else if ((ElementSet == METADATA_MAGIC) || (ElementSet == FULLTEXT_MAGIC))
    {
      STRING Value;
      STRING Section ("HTMLMETA");

      // HTMLMETA class documents are 1-deep so just collect the fields
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey (&Key);
      COLONDOC::GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();
      DFD Dfd;
      STRING FieldName;
      // Walth-though the DFD
      METADATA Meta(Section);
      STRLIST Position, Contents;

      Present(ResultRecord, BRIEF_MAGIC, NulString, &Value);
      if (!Value.IsEmpty())
        {
	  Position.AddEntry(Section);
	  Position.AddEntry("LOCATOR");
	  Position.AddEntry("BRIEF");
	  Contents.AddEntry( Value );
	  Meta.SetData(Position, Contents);
	  Position.Clear();
	  Contents.Clear();
        }
      // <LANGUAGE><CODE>...</CODE></LANGUAGE>
      Position.AddEntry(Section);
      Position.AddEntry("LOCATOR");
      Position.AddEntry("LANGUAGE");
      Position.AddEntry("CODE");
      Contents.AddEntry( ResultRecord.GetLanguageCode() );
      Meta.SetData(Position, Contents);
      // <LANGUAGE><NAME>...</NAME></LANGUAGE>
      Position.Clear();
      Contents.Clear();
      Position.AddEntry(Section);
      Position.AddEntry("LOCATOR");
      Position.AddEntry("LANGUAGE");
      Position.AddEntry("NAME");
      Contents.AddEntry( ResultRecord.GetLanguageName() );
      Meta.SetData(Position, Contents);
      if (URL(ResultRecord, &Value, true))
        {
	  Position.Clear();
	  Contents.Clear();
	  Position.AddEntry(Section);
	  Position.AddEntry("LOCATOR");
	  Position.AddEntry("URL");
	  Contents.AddEntry( Value );
	  Meta.SetData(Position, Contents);
	}
      for (size_t i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);

	  if (tagRegistry)
	    {
	      tagRegistry->ProfileGetString("Metadata-Maps", FieldName, FieldName, &Value);
	      if (Value.IsEmpty() || (Value ^= "<Ignore>"))
		continue; // Ignore this
	      FieldName = Value;
	    }
	  // Get Value of the field
	  Db->GetFieldData (ResultRecord, FieldName, &Value); 
	  if (!Value.IsEmpty())
	    {
	      Position.Clear();
	      Contents.Clear();
	      Position.AddEntry(Section);
	      if (FieldName.GetChr(1) == '$')
		{
		  Position.AddEntry("HTTP-EQUIV");
		  Position.AddEntry(1+(const char *)FieldName);
		}
	      else
		{
		  size_t pos;
		  STRING S;
		  while ((pos = FieldName.Search ('.')) != 0)
		    {
		      S = FieldName;
		      S.EraseAfter (pos - 1);
		      FieldName.EraseBefore (pos + 1);
		      Position.AddEntry (S);
		    }
		  while (Value.GetChr(1) == '(')
		    {
		      if ((pos = Value.Search(/* ( */ ')')) != 0)
			{
			  S = 1 + (const char *)Value;
			  Value.EraseBefore(pos + 1);
			  S.EraseAfter(pos - 2);
			  FieldName.Cat (' ');
			  if ((pos = S.Search('=')) != 0)
			    {
			      if (S.GetChr(pos+1) != '"' && S.GetChr(pos+1) != '\'')
				{
				  char qchar = S.Search('"') == 0 ? '"' : '\'';
				  S.Insert(pos+1, qchar);
				  S.Cat (qchar);
				}
			    }
			  FieldName.Cat (S);
			}
		      else
			break;
		    }
		  Position.AddEntry(FieldName);
		}
	      Contents.AddEntry(Value);
	      Meta.SetData(Position, Contents);
	    }
	}			/* for */
      if (ElementSet == FULLTEXT_MAGIC)
	{
	  if (RecordSyntax == HtmlRecordSyntax)
	    {
	      HtmlHead (ResultRecord, ElementSet, StringBuffer);
	      StringBuffer->Cat (Meta.Html());
	      HtmlTail (ResultRecord, ElementSet, StringBuffer);
	    }
	  else
	    *StringBuffer = Meta.Text();
	}
      else if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING value  = ResultRecord.GetCharsetCode();
	  // HTTP Header (for transport)
	  *StringBuffer = "Content-type: text/xml";
	  if (!value.IsEmpty() && (value != "us-ascii"))
	    *StringBuffer << "; charset=" << value;
	  SRCH_DATE date ( ResultRecord.GetDate());
	  if (date.Ok())
	    *StringBuffer << "\nLast-Modified: " <<  date.RFCdate();
	  *StringBuffer << "\nLanguage: " << ResultRecord.GetLanguageCode();
	  DOCTYPE::Present(ResultRecord, "$ETAG", &Value); 
	  if (!Value.IsEmpty())
	    *StringBuffer << "\nETag: " << Value;
	  StringBuffer->Cat("\n\n");
	  StringBuffer->Cat ( (STRING)Meta );
	}
      else
	*StringBuffer =  (STRING)Meta;
      return;
  } else
    COLONDOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
