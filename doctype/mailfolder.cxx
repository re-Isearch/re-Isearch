#pragma ident  "@(#)mailfolder.cxx"
/************************************************************************
************************************************************************/

/*-@@@
File:		mailfolder.cxx
Version:	$Revision: 1.1 $
Description:	Class MAILFOLDER - Unix mail folder Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/
/*
// Revision History
// ===============
// $Log: mailfolder.cxx,v $
// Revision 1.1  2007/05/15 15:47:29  edz
// Initial revision
//
// Revision 1.7  1995/12/06  10:32:01  edz
// Sync
//
// Revision 1.6  1995/11/29  15:42:59  edz
// Sync before reorganization.
//
// Revision 1.5  1995/11/29  09:36:24  edz
// Sync
//
// Revision 1.4  1995/11/25  22:43:14  edz
// Minor mods to HtmlRecordSyntax and HTML header.
//
// Revision 1.3  1995/11/21  21:34:10  edz
// Minor improvements
//
//
*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "common.hxx"
#include "mailfolder.hxx"
#include "maildigest.hxx"
#include "newsfolder.hxx"
#include "irlist.hxx"
#include "mailman.hxx"
#include "yahoolist.hxx"
#include "doc_conf.hxx"

static const STRING AuthorDefault ("<Anonymous>");
static const STRING SubjectDefault ("<No subject given>");

static const char URL_SLASH[] = "%252F"; // was "/"


// find out what we have..
#if defined(MAILDIGEST_HXX) || defined(LISTDIGEST_HXX) || defined(IRLIST_HXX)
# define DIGEST
#endif

static const char HEX[] = "0123456789ABCDEF%";

// Some Preferences
static const STRING MailHeadTag      ("Head"); // For the Header
static const STRING MailBodyTag      ("Body"); // For the Body
static const STRING MailThreadTag    ("Thread"); // Message-Id, References, In-Reply-to
static const STRING MailRecipientTag ("Recipient"); // To, Cc, Bcc
static const STRING MailFrom         ("From");
static const STRING MailSender       ("Sender");
static const STRING MailResentFrom   ("Resent-From");
static const STRING MailReplyAddress ("Reply-To");
static const STRING MailOrigin       ("Original-Sender");
static const STRING MailOriginator   ("Originator");
static const STRING MailDate         ("Date");
static const STRING MailExpires      ("Expires");
static const STRING MailExpiresObsolete ("Expiry-Date");
static const STRING MailArrivalTime  ("X-OriginalArrivalTime");
/*
static const STRING MailXPriority     ("X-Priority");
static const STRING MailPriority     ("Priority");
*/
static const STRING MailContentLength("Content-Length");
static const STRING MailSubject      ("Subject");
static const STRING MailComments     ("Comments");
static const STRING MailOrganization ("Organisation");
static const STRING MailKeywords     ("Keywords");
static const STRING MailInReplyTo    ("In-Reply-to");
static const STRING MailReferences   ("References");
static const STRING MailMessageId    ("Message-Id");
static const STRING MailAuthor       ("Author");
static const STRING MailAuthorName   ("Author_name");
static const STRING MailAuthorAddress("Author_Address");
static const STRING MailTelefax      ("Telefax");

static const STRING MailListId       ("List-Id");

#if 1
static const char colOpen[] = "<TR><TH ALIGN=\"Right\">";
#else
static const char colOpen[] = "<TR><TH><P ALIGN=Right>";
#endif

static const char * const Keywords[] = {
    /* Must be sorted! */
    "ACategory", // Wed May 19 03:35:23 MET DST 1999 
//  "Autoforwarded",
    "Bcc",
    "Cc",
    "Comments", 	// added Thu Jul 27 02:23:22 MET DST 1995
//  "Content-Transfer-Encoding", // added Wed Oct  4 23:07:32 MET 1995
    "Content-Length", // added Fri Sep 12 2003
    "Content-Type",
    "Date",
    "Encrypted",	// added Thu Jul 27 02:23:22 MET DST 1995
    "Expires",
    "Expiry-Date",      // Obsolete (added 2021 to support old mail archives)
    "Followup-To",
    "Fax", 		// added 7 Oct 2003
    "From",
    "In-Reply-To",
    "Keywords",
//  "List-Archive",
//  "List-Help",
    "List-Id", // The name of the list
    "List-Post", // Highest level Reply-to
    "List-Subscribe",
    "List-Unsubscribe", // multiple links
    "Message-Id",
    "Newsgroup",	// Sometimes used in Mail
    "Newsgroups",	// This is used in Usenet News
    "Organisation",
    "Organization",
    "Original-Sender", // Used by some maillist servers, Added Fri Oct 25 13:31:22 MET DST 1996
    "Originator", // Added to support some mailing lists
    "Phone", // Phone number added 7 Oct 2003
//   "Priority",
    "Received", // added 7 Oct 2003
    "References",
    "Reply-To",
    "Resent-From", // Used by some maillist servers, Added Fri Oct 25 13:31:22 MET DST 1996
    "Return-Path", // Added 2021 for re-Isearch
    "Sender",
    "Speech-Act",	// added 7 Oct 2003
    "Subject",
    "Summary",
    "Telefax",		// added 7 Oct 2003
    "To",
    "X-No-Archive",	// added 7 Oct 2003
    "X-OriginalArrivalTime", // 7 Oct 2003
    "X-Original-Date",	// 7 Oct 2003
//    "X-Priority",	// added 7 Oct 2003
    "X-Supersedes", // Wed May 19 03:35:23 MET DST 1999
//  "X-Sun-Charset", // added Wed Oct  4 23:07:32 MET 1995
    "X-To",
    "X-UID",
    "X-Url"
};

void MAILFOLDER::LoadFieldTable()
{
  if (loadFieldTable && Db)
    {
      Db->AddFieldType(MailDate, FIELDTYPE::date);
      Db->AddFieldType(MailExpires, FIELDTYPE::date);
      Db->AddFieldType(MailArrivalTime, FIELDTYPE::date);
      Db->AddFieldType(MailContentLength, FIELDTYPE::numerical);
/*
      Db->AddFieldType(MailXPriority, FIELDTYPE::computed);
      Db->AddFieldType(MailPriority, FIELDTYPE::computed);
*/
      Db->AddFieldType("X-UID", FIELDTYPE::numerical);
      loadFieldTable = false;
    }
  DOCTYPE::LoadFieldTable();
}


NUMERICOBJ MAILFOLDER::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  if (FieldName ^= "Priority")
    {
      int value = 3; // Normal

      if (Buffer.IsEmpty())
	return NUMERICOBJ();

      // Normal, Urgent, Non-urgent
      if (Buffer.SearchAny("Normal"))
	value = 3;
      else if (Buffer.SearchAny("Non-urgent"))
	value = 4;
      else if (Buffer.SearchAny("Urgent"))
	value = 2;
      else if (Buffer.SearchAny("Bulk"))
	value = 5;
      return value;
    }
   return Buffer; 
}

const char *MAILFOLDER::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("MAILFOLDER");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  PTEXT::Description(List);
  return "Folders of mail (RFC822) messages\n\
Options:\n\
  RESTRICT_FIELDS         specified if only a list of 'known' message fields should\n\
                          be handled and not every tag (Deault YES)\n\
  PARSE_MESSAGE_STRUCTURE specifies if the message line/sentence/paragraph\n\
                          structure should be parsed (Default YES)\n";
}


void MAILFOLDER::SourceMIMEContent(PSTRING StringPtr) const
{ 
  // MIME/HTTP Content type for Mail folder records
  StringPtr->AssignCopy(14, "Message/rfc822");
} 

/*-
 MAILFOLDER expects the folder formats:

 From blah ... or Article NN of XXX.XXX.XXX 
 XXX: Blah, Blah, Blah
 .
 .
 .
 <blank line>
 <message body>
 <blank line>
 From blah ...  or Article NNN of XXX.XXX.XXX
 XXX: Blah, Blah, Blah
 .
 .
 .
 <blank link>
 <message body>
 .
 .
 .

 Applicable XXX: fields are stored under XXX and the
 message body is stored under the "message-body" field
 name.

Please Note:
-----------

Right now only RFC822 is (I hope) correctly handled.
The following are not yet correctly handled:
  1) Base64, BinHex, quoted-printable,.. encodings
  2) Character sets (assumed same as locale)
  3) Multiple record messages (Sun Attachments, MIME etc)
  4) UUencoded, btoa'd etc binary messages
  5) Face signatures.
In other words, right now the body of messages is assumed
to contain text in the character set of the host platform.

Should also handle Sunmail body tags:
X-Sun-Data-Type:
X-Sun-Data-Description:
X-Sun-Data-Name:
X-Sun-Content-Lines: 


-*/


// Loads of possible tags are in mail headers
// but we will ONLY use the following for
// tags.
bool MAILFOLDER::accept_tag(const PCHR tag) const
{
  if (!RestrictFields)
    return true; // Anything goes

  int n = 0;
  if (tag && *tag) {
    size_t i = 0;
    do {
      n = StrCaseCmp(tag, Keywords[i]);
    } while (++i < sizeof(Keywords)/sizeof(Keywords[0]) && n > 0);
  }
  return n == 0 ? true : false;
}

const CHR * MAILFOLDER::Seperator() const
{
  return NULL;
}


MAILFOLDER::MAILFOLDER (PIDBOBJ DbParent, const STRING& Name) :
	PTEXT (DbParent, Name)
{
  const char PARSE_MESSAGE_STRUCTURE[] = "PARSE_MESSAGE_STRUCTURE";
  const char RESTRICT_FIELDS[]         = "RESTRICT_FIELDS";
  const char YES[]                     = "Y";
  const char msg[] = "%s: %s=%d";

  ParseMessageStructure = Getoption(PARSE_MESSAGE_STRUCTURE, YES).GetBool();
  message_log (LOG_DEBUG, msg, Doctype.c_str(), PARSE_MESSAGE_STRUCTURE, (int)ParseMessageStructure);


  RestrictFields = Getoption(RESTRICT_FIELDS, YES).GetBool();
  message_log (LOG_DEBUG, msg, Doctype.c_str(), RESTRICT_FIELDS, (int)RestrictFields);

  loadFieldTable = true;
  LoadFieldTable();
}

void MAILFOLDER::AddFieldDefs ()
{
  PTEXT::AddFieldDefs ();
}

void MAILFOLDER::ParseRecords (const RECORD& FileRecord) 
{
  // Break up the document into Mail message records
  off_t Start = FileRecord.GetRecordStart ();
  off_t End = FileRecord.GetRecordEnd();
  const STRING Fn ( FileRecord.GetFullFileName () );


// cerr << "XXXXX Parse Records " << Fn << endl;
  PFILE Fp = ffopen (Fn, "rb");
  if (!Fp)
    {
      message_log (LOG_ERRNO, "Couldn't access '%s'", (const char *)Fn);
      return;			// File not accessed
    }

  if (End == 0)
    {
      End = GetFileSize(Fp);
    }
  if (End <= Start)
    {
      message_log (LOG_WARN, "Truncated record, skipping..");
      ffclose(Fp);
      return;
    }

  char buf[512]; // was 256
  if (Start)
    {
      if (-1 == fseek (Fp, Start, SEEK_SET))
	{
	  message_log (LOG_WARN, "Can't seek '%s' to record start", (const char *)Fn);
	  ffclose(Fp);
	  return; // Bad start
	}
    }


  // Read first line of folder
  if (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) == NULL)
    {
      ffclose(Fp);
      return; // EMPTY Folder
    }
  // Try to guess folder type
  enum {MAIL, NEWS, MAIL_OR_NEWS} folder;
  if (IsMailFromLine(buf))
    folder = MAIL; // Expect mail folder
  else if (IsNewsLine(buf))
    folder = NEWS; // Expect News folder
  else
    folder = MAIL_OR_NEWS;

  RECORD Record (FileRecord);
  off_t Position = Start;
  off_t SavePosition = 0;
  off_t RecordEnd;

  // Read lines from file and search for record seperation
  enum {Header, ContentLength, DigestMagic} LookFor = Header;
#ifdef DIGEST
  bool IsDigest = false;
  bool DigestKnown = false;
#endif
  do
    {
      /*
	Search for "magic" lines: a blank line followed by
	a legal Email "from ", resp. News "Article ", line
       */
      if (((LookFor == Header || LookFor == DigestMagic) &&
	((folder != NEWS && IsMailFromLine(buf)) || (folder != MAIL && IsNewsLine(buf)))))
	{
	  LookFor = ContentLength;
	  SavePosition = Position;
	  Record.SetRecordStart (Start);
	  RecordEnd = (SavePosition == Start) ? Start : SavePosition - 1;

	  if (RecordEnd > Start)
	    {
#ifdef NEWSFOLDER_HXX
	      // Part of Auto-detection...
	      if (folder == NEWS || IsNewsLine(buf))
		{
		  Record.SetDocumentType ("NEWSFOLDER");
		}
#endif
	      Record.SetRecordEnd (RecordEnd);
#ifdef DIGEST
	      if (IsDigest && DigestKnown)
		{
		  // Parse into sub-records by Maildigest parser..
		  const off_t oPos = ftell( Fp ); // BUGFIX: store old position
		  Db->ParseRecords (Record);
		  Record.SetDocumentType (Doctype);
		  fseek(Fp, oPos, SEEK_SET); // BUGFIX: go back
		}
	     else
#endif
		{
		  Db->DocTypeAddRecord(Record);
		  Record.SetDocumentType(Doctype); // Reset Document Type
		}
	      Start = SavePosition;
#ifdef DIGEST
	      IsDigest = DigestKnown = false;
#endif
	    }
	}
      else if (LookFor == ContentLength &&
	!IsDigest && /* BUGFIX for Maildigests */
	(buf[0]=='C'||buf[0] == 'c') && StrNCmp(buf+1, "ontent-Length: ", 15) == 0)
	{
	  // To Handle some folders that don't ecape "From "-- eg. Solaris 2.x /bin/mail
	  Position += strlen(buf);
	  long off = atoi(&buf[16]);
	  // Search for start of message body...
	  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
	    {
	      if (buf[0] == '\n' || buf[0] == '\r')
		break;
	      Position += strlen(buf);
	    }
	  if (-1 != fseek(Fp, off, SEEK_CUR))
	    Position += off;
	  LookFor = Header;
	  // Note: we don't bother breaking up digest messages..
	}
#ifdef DIGEST 
     else if (LookFor == ContentLength && !IsDigest &&
		(buf[0]=='F'||buf[0]=='f') && StrNCmp(buf+1, "rom: ", 5) == 0)
	{
	  if (strstr(buf+6,"digest processor") ||
	     // AOL Listserver software
	     ((buf[6] == '\"' || buf[6] == '\t') && strncmp(buf+7, "L-Soft list server at", 21)) == 0)
	    {
	      IsDigest = true;
	    }
	}
     else if (LookFor == ContentLength && !IsDigest &&
		(buf[0]=='S'||buf[0]=='s') && StrNCmp(buf+1, "ubject: ", 8) == 0)
	{
	  const char *tcp = buf + 9;

	  if (*tcp == '[')
	    {
	      while (*tcp && *tcp != ']')
		tcp++;
	      if (*tcp == ']') if (*++tcp == ' ') tcp++;
	    }
	  if (StrNCaseCmp(tcp, "Digest Number ", 14) == 0)
	    {
	      IsDigest = true;
	    }
	  else
	    {
	      if ((tcp = strstr(buf+8, " Digest ")) == NULL)
		{
		  tcp = strstr(buf+8, " digest");
		  if (tcp && tcp[7] != ',' && tcp[7] != ':' && tcp[8] != ' ')
		    tcp = NULL;
		}
	      if (tcp)
		{
		  // Make sure not a responce to a digest...
		  char *tp = buf + 8;
		  while (isspace(*tp)) tp++;
		  if (strncasecmp(tp, "Re: ", 4) != 0)
		    {
		      // Looks like a maildigest
		      IsDigest = true;
		    }
		}
	    }
	}
      if (LookFor == DigestMagic)
	{
	  // code to determine if "MAILDIGEST", "LISTDIGEST" or "IRLIST"
	  // Look for magic or start of next message (Bogus " Digest " in Subject)
	  const struct {
	    const char *doctype;
	    const char *magic;
	    int         magic_len;
	    int         need_cr;
	  } Digests [] = {
#ifdef YAHOOLIST_HXX
           {"YAHOOLIST", "\
------------------------ Yahoo! Groups Sponsor ---------------------~-->", 72, 1},
/*
	   {"YAHOOLIST", "\
------------------------------------------------------------------------", 72, 1},
*/
	   {"YAHOOLIST", "\
________________________________________________________________________", 72, 0},

#endif

#ifdef MAILDIGEST_HXX
	    // RFC1173: The Preamble must be separated from the remainder
	    // of the message by a line of 70 hyphens followed by a blank line.
	    {"MAILDIGEST", "\
----------------------------------------------------------------------", 70, 1},

#endif

#ifdef MAILMAN_HXX
	    {"MAILMAN", "--__--__--", 10, 1},
#endif
#ifdef LISTDIGEST_HXX
	    {"LISTDIGEST", "\
========================================", 40, 0},
#endif
#ifdef IRLIST_HXX
	    {"IRLIST",     "*********", 9, 1}
#endif
	  };
	  for (size_t i = 0; i < sizeof(Digests)/sizeof(Digests[0]); i++)
	    {
	      if (strncmp(buf, Digests[i].magic, Digests[i].magic_len) == 0 &&
		(!Digests[i].need_cr || isspace(buf[Digests[i].magic_len])) )
		{
		  LookFor = Header;
		  IsDigest = false;
		  Record.SetDocumentType (Digests[i].doctype);
		  DigestKnown = true;
		  message_log (LOG_INFO, "Identified %s (Digest)",
			 Digests[i].doctype);
		  LookFor = Header;
		  break;
		}
	    }
	}
#endif
      else if (buf[0] == '\n' || buf[0] == '\r')
	{
#ifdef DIGEST
	  if (LookFor == ContentLength && IsDigest)
	    LookFor = DigestMagic;
	  else
#endif
	    LookFor = Header;
	}
      Position += strlen(buf);
    }
  while ((Start < End) && fgets(buf,sizeof(buf)/sizeof(char)-1,Fp) != NULL);

  ffclose (Fp);

  Record.SetRecordStart (Start);
  RecordEnd = Position - 1;

  if (RecordEnd > Start)
    {
#ifdef NEWSFOLDER_HXX
      if (folder == NEWS)
	Record.SetDocumentType ("NEWSFOLDER");
#endif
      Record.SetRecordEnd (RecordEnd);
#ifdef DIGEST
      if (IsDigest && DigestKnown)
	{
	  // Parse into sub-records by Maildigest parser..
	  Db->ParseRecords (Record);
	}
      else
#endif
	{
	  Db->DocTypeAddRecord(Record);
	}
    }
}

INT MAILFOLDER::EncodeKey (STRING *Key, const CHR *line, size_t val_len) const
{
  // cerr << "DEBUG:     Encode Key: " << line << endl; // EDZ 2021
  char        tmp[64];
  size_t      length = 0;

  if (val_len)
    {
      int len = (int)val_len;
      while (!isalnum (*line) && len > 0)
	line++, len--;
      while (*line && len >= 0 && length < (sizeof(tmp)-1))
	{
	  if (*line == '@')
	    {
	      if (length >= 12)
		{
		  tmp[length++] = 'Z';
		  break;
		}
	      tmp[length++] = 'A';
	    }
	  else if (*line == '&')
	    tmp[length++] = 'B';
	  else if (*line == '%')
	    tmp[length++] = 'C';
	  else if (*line == '+')
	    tmp[length++] = 'P';
	  else if (*line == '*')
	    tmp[length++] = 'S';
	  else if (*line == '\\')
	    tmp[length++] = '/';
	  else if (isalnum(*line))
	    tmp[length++] = tolower((unsigned char)*line); // lowercase
	  else if (*line == '_' || *line == '-' || *line == '!' ||
	    *line == ':' || *line == ';' || *line == '~' || *line == ',' || *line == '.' || *line == '/')
	    {
	      tmp[length++] = *line;
	    }
	  line++, len--;
	}
      while (!isalnum(tmp[length-1]) && length > 0)
	tmp[length--] = '\0';
    }
  if (length > 0)
    {
      tmp[length] = '\0';

      STRING newKey(tmp);
      Key->form("%04X", (((newKey.Hash()) % 1021)*64 + length) & 0xFFFF);
      if (length > (DocumentKeySize-8))
	{
	  // Backwards...
	  for (unsigned i=1; i<= DocumentKeySize - 8; i++)
	    Key->Cat ( tmp[length-i] );
	}
      else
	Key->Cat (newKey);
    }
  else
    Key->Clear();
  return Key->GetLength();
}

void MAILFOLDER::ParseFields (RECORD *NewRecord)
{
  const STRING fn (NewRecord->GetFullFileName ());
  PFILE fp = ffopen(fn, "rb");

  if (!fp)
    {
      NewRecord->SetBadRecord();
      return;		// ERROR
    }

  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp);
  if (RecEnd <= RecStart)
    {
      ffclose(fp);
      NewRecord->SetBadRecord();
      return;
    }
  const off_t RecLength = RecEnd - RecStart + 1;
  char *RecBuffer = (char *)tempBuffer.Want(RecLength + 1);
  if (RecBuffer == NULL)
    {
      ffclose(fp);
      message_log (LOG_PANIC, "Can't Parse %s Fields in '%s'. Allocation failed",
	Doctype.c_str(), fn.c_str());
      return;
    }
  size_t ActualLength = pfread(fp, RecBuffer, RecLength, RecStart);
  ffclose (fp);
  RecBuffer[ActualLength] = '\0';
  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      message_log (LOG_ERROR,
#ifdef _WIN32
	"%s: %s '%s' record (%I64d - %I64d)"
#else
	"%s: %s '%s' record (%lld - %lld)"
#endif
	, Doctype.c_str(), tags ? "No fields/tags in" : "Unable to parse", fn.c_str(),
	(long long)RecStart, (long long)RecEnd);
      NewRecord->SetBadRecord();
      return;
    }

  DF df;
  PDFT pdft = new DFT ();
  DFD dfd;
  STRING FieldName;
  // Walk though tags

  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      if (!accept_tag(*tags_ptr))
	continue;
#if 0
      // Some warning...
      if (strncmp(*tags_ptr, "Content-Type", 12) == 0)
	{
	  if (strstr(*tags_ptr, "X-sun-attachment")) 
	    cout << "MAILFOLDER: record in \"" << fn << "\" is " << *tags_ptr << "\n";
	}
      else if (strncmp(*tags_ptr, "Content-Transfer-Encoding", 12) == 0)
	{
	  cout << "MAILFOLDER: record in \"" << fn << "\" is " << *tags_ptr << "\n";
	}
#endif

      PCHR p = tags_ptr[1];
      if (p == NULL)
	p = &RecBuffer[RecLength];
      // eg "From:"
      int off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space 
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;

      // Strip potential trailing while space
      while (val_len > 0 && isspace (RecBuffer[val_len + val_start]))
	val_len--;
      if ((*tags_ptr)[0] == '\0' || (*tags_ptr)[0] == '\n')
	FieldName = MailBodyTag;
      else if (StrCaseCmp("Fax", *tags_ptr) == 0)
	FieldName = MailTelefax;
      else if (StrCaseCmp("Organization", *tags_ptr) == 0)
	FieldName = MailOrganization; // Ocean kludge
      else if (StrCaseCmp("Newsgroup", *tags_ptr) == 0)
	FieldName = "Newsgroups"; // Compat. kludge
      else if (MailOrigin ^= *tags_ptr)
	FieldName = MailOriginator; // Map these two together
      else
	FieldName = *tags_ptr;

      if (FieldName ^= MailExpiresObsolete) FieldName = MailExpires; // Remap obsolete name 

      dfd.SetFieldType( Db->GetFieldType(FieldName) ); // Get the type added 30 Sep 2003

      if (FieldName.CaseCompare("List-", 5) == 0)
	{
	  if (!(FieldName ^= MailListId))
	    FieldName.Insert(1, "+");
	}
      else if (FieldName ^= MailDate)
        {
          NewRecord->SetDate( *tags_ptr + FieldName.GetLength() + 1);
	  dfd.SetFieldType (FIELDTYPE::date);
        }
      else if (FieldName ^= MailExpires)
      {
	  NewRecord->SetDateExpires( *tags_ptr + FieldName.GetLength() + 1);
          dfd.SetFieldType (FIELDTYPE::date);
	}
/*
      else if ((FieldName ^= MailXPriority) || (FieldName ^= MailPriority))
	{
	  NewRecord->SetPriority(10 -
		(int) MAILFOLDER::ParseComputed(FieldName, *tags_ptr+FieldName.GetLength() + 1) );
	}
*/
      else if (FieldName ^= MailContentLength)
        {
	  dfd.SetFieldType (FIELDTYPE::numerical);
        }
      else
        {
//	  dfd.SetFieldType (FIELDTYPE::text);
	  if (FieldName ^= MailMessageId)
	    {
	      STRING Key;
	      if (EncodeKey(&Key, &RecBuffer[val_start], val_len))
		{
		  message_log (LOG_DEBUG, "Message-Id encoded as %s", Key.c_str());
		  // Make Unique Key
		  // Message IDs need to be globally unique!
		  // if two messages have the same this is a NO-GO!
		  if (Db->KeyLookup(Key)) {
		    message_log (LOG_WARN, "Encounterd a non-unique MESSAGE-Id: %s", &RecBuffer[val_start] );
		    // if non-unique we process it.. and it'll get another key (and maybe marked deleted) 
		    Db->MdtSetUniqueKey(NewRecord, Key);
		  } else NewRecord->SetKey(Key); // Key was OK.. Set
		}
	    }
        }

      dfd.SetFieldName (FieldName);

      Db->DfdtAddEntry (dfd);
      df.SetFct ( FC(val_start, val_start + val_len)  );
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
      // In-Reply-To, Message-Id and References hold thread info
      if ((FieldName ^= MailInReplyTo) ||
	  (FieldName ^= MailMessageId) ||
	  (FieldName ^= MailReferences) )
	{
	  // Clone into Mail thread field infomation
	  FieldName = MailThreadTag;
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);
	  df.SetFieldName (FieldName);
	  pdft->AddEntry (df);
	}
      else if ((FieldName ^= "To") || (FieldName ^= "Cc") || (FieldName ^= "Bcc"))
	{
	  // Clone into Recipient Field
	  FieldName =  MailRecipientTag;
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);
	  df.SetFieldName (FieldName);
	  pdft->AddEntry (df);
	}
      else if (FieldName ^= MailBodyTag)
	{
#if 1
	  // Now we want to also parse the contents
          if (ParseMessageStructure)
	    ParseStructure(pdft, &RecBuffer[val_start], val_start, val_len);
#endif
	  // Handle Header, the counterpoint to Body
	  if (*(tags[0]))
	    {
	      FieldName = MailHeadTag; // Mailhead 
	      dfd.SetFieldName(FieldName);
	      SetMetaIgnore(FieldName); // Don't want Head in Meta
	      Db->DfdtAddEntry (dfd);
	      // 2003 March 25 Added edz GPTYPE casts
	      FC fc(*tags - RecBuffer, *tags_ptr - RecBuffer - 1);
	      df.SetFct ( fc ); 
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);
	    }
	  else
	    {
	      message_log (LOG_WARN, "No %s tags, only body [%s(%lu-%lu)]..",
		Doctype.c_str(), RemovePath(fn).c_str(), RecStart, RecEnd);
	    }
	}
    }

  NewRecord->SetDft (*pdft);
  delete pdft;
}

void MAILFOLDER::BeforeIndexing()
{
  PTEXT::BeforeIndexing();
}


void MAILFOLDER::AfterIndexing()
{
  TagBuffer.Free();
  tempBuffer.Free();
  PTEXT::AfterIndexing();
}

/*-
 * Find Author's name in mail address
 * In "XXX (YYY)" or YYY <XXX>" return "YYY"
 * Find Author's address in mail address
 * In "XXX (YYY)" or YYY <XXX>" return "XXX"
 */
// TODO: Rework using a state machine. The current implementation
// is getting to be a collection of kludges.
PCHR MAILFOLDER::NameKey (PCHR buf, bool wantName) const
{
  char p1, p2, b1, b2;
  char email[1024];
  // Skip leading white space
  while (isspace(*buf))
    buf++;


  size_t len = strlen(buf);
  // Handle XXX ()
  if (len > 3 && buf[len-2] == '(' && buf[len-1] == ')')
    len -= 2;
  if (len >= sizeof(email)/sizeof(char))
   len = sizeof(email)/sizeof(char)-1;

#if 1
  { char *tp = buf;
    char *s  = email;
    for (size_t i=0; i < len; i++)
      {
	if (isspace(*tp))
	  {
	    // we know that the 1st char is NEVER space
	    if (s[-1] != ' ') *s++ = ' ';
	    tp++;
	  }
	else
	   *s++ = *tp++;
      }
    *s = '\0';
  }
#else
  memcpy (email, buf, len);
  email[len] = '\0';
#endif

  // Kludge to handle <XXX>(YYY) and (YYY)<XXX>
  if (*email == '<' || *email == '(')
    {
      char *a = strrchr(email, ')');
      char *b = strchr(email, '>');
      if (a && b)
	{
	  if      (*email == '<') *b = ' ';
	  else if (*email == '(') *a = ' ';
	  *email = ' ';
	}
      // Something like <Blah>
      else if (b && *email == '<' && *(b+1) == '\0')
        {
	  *b = '\0';
	  strcpy(buf, email+1);
	  return buf; // Address and name are same
        }
    }

  if (wantName)
    {
      p1 = '('; p2 = ')';
      b1 = '<'; b2 = '>';
    }
  else
    {
      p1 = '<'; p2 = '>';
      b1 = '('; b2 = ')';
    }

  // BUGFIX: Wed Apr 17 01:02:20 MET DST 1996
  // Some mail headers have \( so we can't use strchr
  register PCHR s = NULL;
  register PCHR e = NULL;

  // BUGFIX: Wed Aug 28 10:41:25 MET DST 1996
  // Strip multiple as in: Joe Blow <joe@blow@foo.bar> (By way of Elmer Fudd <Elmer@Fudd.com>)

  // BUGFIX: Sun Jan 12 00:38:58 MET 1997
  // problem with "Lee Newman (814) 865-1818" <EWN%PSULIAS.BITNET@vm.gmd.de>
  // Need to look for ". Used Quoted logic

  bool Quoted = false;
  for (s = email; *s ; s++)
    {
      if (*s == '"') Quoted = !Quoted;
      else if (*s == '\\') s++;
      else if (!Quoted && *s == p2) break;
    }
  Quoted = false;
  for (e = email; *e ; e++)
    {
      if (*e == '"') Quoted = !Quoted;
      else if (*e == '\\') e++;
      else if (!Quoted && *e == b2) break;
    }
  if (*e && *s)
    {
      // Cut after first > or )
      if (e > s) *++s = '\0';
      else if (s>e) *++e = '\0';
    }

  // Look for bits..
  Quoted = false;
  for (s = email; *s ; s++)
    {
      if (*s == '"') Quoted = !Quoted;
      else if (*s == '\\') s++;
      else if (!Quoted && *s == p1) break;
    }
  Quoted = false;
  for (e = email; *e ; e++) 
    {
      if (*e == '"') Quoted = !Quoted;
      else if (*e == '\\') e++; 
      else if (!Quoted && *e == p2) break; 
    }


  if (e > s && *e)
    {
      *e = '\0';		/* Chop off everything after p2 (')' or '>') */
      strcpy (email, ++s);
    }
  else
    {
      Quoted = false;
      for (s = email; *s ; s++) 
	{
	  if (*s == '"') Quoted = !Quoted;
	  else if (*s == '\\') s++; 
	  else if (!Quoted && *s == b1) break; 
	}
      Quoted = false;
      for (e = email; *e ; e++)  
	{
	  if (*e == '"') Quoted = !Quoted;
	  else if (*e == '\\') e++;  
	  else if (!Quoted && *e == b2) break;  
	}
      if (e > s && *e)
	strcpy (s, ++e);	/* Remove <...> or (...) */
    }
  s = email;
  while(isspace(*s) || *s == '"') s++; // Skip leading space
  strcpy (buf, s);
  s = buf + strlen(buf) - 1;
  while (s > buf && (isspace(*s) || *s == '"'))
    *s-- = '\0'; // Trim trailing space

  return buf;
}

void MAILFOLDER::Mailto (const STRING &Value, PSTRING StringBufferPtr) const
{
  Mailto(Value, NulString, NulString, StringBufferPtr);
}

void MAILFOLDER::Mailto (const STRING &Value, const STRING &Subject, PSTRING StringBufferPtr) const
{
  Mailto(Value, Subject, NulString, StringBufferPtr);
}

void MAILFOLDER::Mailto (const STRING &Value, const STRING &Subject, const STRING& References,
  PSTRING StringBufferPtr) const
{

  if (Value.GetLength ())
    {
      char buf[1024];
      Value.GetCString (buf, sizeof (buf) / sizeof (char) - 1);
      CHR *addr = NameKey (buf, false); // was false 31 Oct 2003

      STRING address (addr);
      *StringBufferPtr << "<A HREF=\"mailto:";
      // Encode Anchor
      for (UCHR ch; (ch = *addr++) != '\0';)
	{
	  if (isalnum (ch) ||
	  // Non Alphanumeric but allowed characters
	      ch == '-' || ch == '_' || ch == '.' || ch == '@' ||
	      ch == '+' || ch == '?' || ch == '/' || ch == ':' || ch == '~')
	    {
	      *StringBufferPtr << ch;
	    }
	  else
	    {
	      // Encode Anchor per IETF URL Specification
	      *StringBufferPtr <<
		HEX[16] <<
		HEX[(ch & '\377') >> 4] <<
		HEX[(ch & '\377') % 16];
	    }
	}			// for()
      if (strchr (buf, '@') == NULL)
	{
	  // Since this is an exported href we need to resolve
	  // the maildomain.. Since we don't really know the
	  // mailhost we just use the host name.
	  if (getOfficialHostName(buf, sizeof (buf) / sizeof (char) - 1) != -1)
	    {
	      *StringBufferPtr << "@" << buf;
	    }
	}
      /* Include NAME for mailto: */
      // Get name of target
      Value.GetCString (buf, sizeof (buf) / sizeof (char) - 1);
      STRING name = NameKey (buf, true);
      name.Replace ("\"", "");
      char sep = '?';
      if (Subject.GetLength())
	{
	  *StringBufferPtr << "?subject=" << "Re:%20" << Subject.w3Encode();
	  sep = '&';
	}
      if (References.GetLength())
	{
	  *StringBufferPtr << sep << "References=" << References.w3Encode();
	  sep = '&';
	}
      StringBufferPtr->Cat ("\" onMouseOver=\"self.status='Reply to ");
      StringBufferPtr->Cat (address);
      StringBufferPtr-> Cat ("'; return true");
      if (Subject.GetLength())
	*StringBufferPtr << "\" TITLE=\"Re: " << Subject;
      StringBufferPtr->Cat("\">");
      HtmlCat (name, StringBufferPtr, false);
      StringBufferPtr->Cat ("</A>");
    }
}

STRING& MAILFOLDER::DescriptiveName(const STRING& Language,
        const STRING& FieldName, PSTRING Value) const
{
  return DOCTYPE::DescriptiveName(Language, FieldName, Value);
}

INT MAILFOLDER::GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length)
{
  return DOCTYPE::GetTerm(Filename, Buffer, Offset, Length);
}

INT MAILFOLDER::ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  return DOCTYPE::ReadFile(Fp, StringPtr, Offset, Length);
}

INT MAILFOLDER::ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  return DOCTYPE::ReadFile(Fp, Buffer, Offset, Length);
}

INT MAILFOLDER::GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  return DOCTYPE::GetRecordData(Fp, StringPtr, Offset, Length);
}

INT MAILFOLDER::GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  return DOCTYPE::GetRecordData(Fp, Buffer, Offset, Length);
}


// Experimental Code (Threads)
void MAILFOLDER::Thread (const RESULT& ResultRecord,
	const STRING& Element, const STRING& RecordSyntax,
	PSTRING StringBufferPtr) const
{
  STRING Value;
  if (RecordSyntax == HtmlRecordSyntax)
    {
      DOCTYPE::Present (ResultRecord, Element, &Value);
      if (Value.GetLength ())
	{
	  PCHR scratch = Value.NewCString ();
	  PCHR tcp, ptr, tp = scratch;
	  STRING tmp, Language ( ResultRecord.GetLanguageCode() );

	  *StringBufferPtr << colOpen << DescriptiveName(Language, Element, &tmp) << ":</TH><TD>";
	  while ((ptr = strchr (tp, '>')) != NULL)
	    {
	      *ptr++ = '\0';
	      // TODO: References can contain more than one message-id
	      if ((tcp = strchr (tp, '<')) != NULL)
		{
		  *tcp++ = '\0';
		  // Have a thread?
		  STRING key;
		  EncodeKey (&key, tcp, strlen (tcp));
		  // Look it up
		  if (Db->KeyLookup (key))
		    {
#define CGI_FETCH "ifetch"
		      STRING furl;
		      Db->URLfrag(&furl);
		      *StringBufferPtr << tp <<
			"&lt;<A HREF=\"" << CGI_FETCH << furl << key << "\">" <<
			tcp << "</A>&gt;";
		    }
		  else
		    {
		      *StringBufferPtr << "&lt;" << tcp << "&gt;";
		    }
		}		// have a '<'
	      tp = ptr;
	    }			// have a '>'

	  if (tp && *tp)
	    HtmlCat (tp, StringBufferPtr);
	  StringBufferPtr->Cat ("</TD></TR>\n");
	  delete scratch;
	}			// Have a In-Reply-To field
    }				// Use HTML Record Syntax
  else
    {
      Present (ResultRecord, Element, RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  *StringBufferPtr << Element << ": " << Value << "\n";
	}
    }
}


// Present the specified ElementSet as a document with the
// specified RecordSyntax..
//
void MAILFOLDER::
DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    const STRING& RecordSyntax, PSTRING StringBufferPtr) const
{
  StringBufferPtr->Clear();
  if (ElementSet == SOURCE_MAGIC)
    {
      DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
      return;
    }
  else if (ElementSet == METADATA_MAGIC)
    {
      DOCTYPE::XmlMetaPresent(ResultRecord, RecordSyntax, StringBufferPtr);
      return;
    }
  bool UseHtml = (RecordSyntax == HtmlRecordSyntax);

  bool Shrink = false;
  if (UseHtml)
    {
      // NaviPress needs to be shrunk
      char *tcp = getenv("HTTP_USER_AGENT");
      if (tcp && (strncmp(tcp,"Navi", 4) == 0 || strncmp(tcp, "GNN", 3) == 0))
	{
	  // Don't shrink "NaviPress/1.1 AOLpress/1.2"
	  if (strstr(tcp, "AOLpress") == NULL)
	    Shrink = true;
	}
    }

  // Get Content
  STRING Content;
  if (ElementSet == FULLTEXT_MAGIC)
    DOCTYPE::Present (ResultRecord, MailBodyTag, &Content);
  else
    DOCTYPE::Present (ResultRecord, ElementSet, &Content);

  if (UseHtml)
    {
      // Convert to HTML
      HtmlHead (ResultRecord, ElementSet, StringBufferPtr);
      StringBufferPtr->Cat( "<!-- MAILFOLDER $Revision: 1.1 $ -->");
    }

  if (ElementSet == FULLTEXT_MAGIC)
    {
      STRING DBname;
      Db->DbName(&DBname);
      RemovePath(&DBname);

      STRING Language ( ResultRecord.GetLanguageCode()), tmp;

      // Build Document
      if (UseHtml) StringBufferPtr->Cat ("<TABLE CELLPADDING=\"2\">");

      // Fetch Subject
      STRING Subject;
      STRING rSubject; // Raw Subject
      DOCTYPE::Present (ResultRecord, MailSubject, &rSubject); // Fetch NON-HTML Subject
      if (!rSubject.IsEmpty())
	{
	  Subject = rSubject;
	  Subject.Pack(); // Zap extra spaces in Multi-line subject
	}

      STRING References;
      DOCTYPE::Present (ResultRecord, MailMessageId, &References);
      if (!References.IsEmpty())
	References.Pack();

      STRING      Value;
      bool mailingList = false;


      DOCTYPE::Present (ResultRecord, MailListId, &Value);
      if (Value.GetLength())
	{
          mailingList = true;
	  if (UseHtml)
	    StringBufferPtr->Cat (colOpen);
	  *StringBufferPtr << DescriptiveName(Language, MailListId, &tmp) << ": ";
	  if (UseHtml)
	    {
	      StringBufferPtr->Cat ( "</TH><TD>" );
	      HtmlCat(ResultRecord, Value.Trim(false), StringBufferPtr);
	      StringBufferPtr->Cat ( "</TD></TR>" );
	    }
	  else
	    StringBufferPtr->Cat ( Value.Pack() );
	  StringBufferPtr->Cat( "\n" );
	}


      // From
      MAILFOLDER::Present (ResultRecord, MailAuthor, SutrsRecordSyntax, &Value);

      if (Value.GetLength ())
	{
	  if (UseHtml)
	    {
	      char buf[1024];
	      Value.GetCString (buf, sizeof (buf) / sizeof (char) - 1);
              STRING name = NameKey (buf, true); // @@@@ 

              // Encode Anchor per IETF URL Specification
              *StringBufferPtr << colOpen << "<A HREF=\"" <<
                "i.search?DATABASE%3D" << DBname << "/TERM%3D%22"
		<< MailFrom << URL_SLASH; 

              for (const UCHR *tp = (const UCHR *)name.c_str(); *tp; tp++)
                {
                  if (isalnum(*tp))
                    StringBufferPtr->Cat (*tp);
                  else if (isspace(*tp))
                    StringBufferPtr->Cat (',');
                  else if (*tp == '%')
                    StringBufferPtr->Cat ("%25");
                  else // Other characters
                    *StringBufferPtr << "%25" <<
                        HEX[(*tp & '\377') >> 4] <<
                        HEX[(*tp & '\377') % 16];
                }
              *StringBufferPtr << "%22\" \
onMouseOver=\"self.status='Search for records from the same author'; return true\" \
TARGET=\"From\">" << DescriptiveName(Language, MailFrom, &tmp) << ":</TH><TD>";
	      Mailto(Value, Subject, References, StringBufferPtr);
	      StringBufferPtr->Cat ("</TD></TR>");
	    }
	  else
	    {
	      *StringBufferPtr << DescriptiveName(Language, MailFrom, &tmp) << ": " << Value.Pack ();
	    }
	  StringBufferPtr->Cat ("\n");
	} else {
	  // No from? Use Originator of message 
	  DOCTYPE::Present (ResultRecord, MailOriginator, &Value);
	  if (Value.GetLength())
	    {
	      if (UseHtml)
		*StringBufferPtr << colOpen;
	      *StringBufferPtr << DescriptiveName(Language, MailOriginator, &tmp) << ": ";
	      if (UseHtml)
		{
		  *StringBufferPtr << "</TH><TD>";
		  Mailto(Value, Subject, References, StringBufferPtr);
		  StringBufferPtr->Cat ("</TD></TR>");
		}
	      else
		*StringBufferPtr << Value.Pack () << "\n";
	    }
        }

      // Organisation
      DOCTYPE::Present (ResultRecord, MailOrganization, RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen;
	  *StringBufferPtr << DescriptiveName(Language, MailOrganization, &tmp) <<  ": ";
	  if (UseHtml) *StringBufferPtr << "</TH><TD>";

	  // Zap extra spaces in Multi-line Organization
	  *StringBufferPtr << Value.Pack () << "\n";
	  if (UseHtml) *StringBufferPtr << "</TD></TR>";  
	}

      // Date
      Present (ResultRecord, MailDate, RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  bool date_link = false;
	  SRCH_DATE date (ResultRecord.GetDate());
	  if (UseHtml)
	    {
	      *StringBufferPtr << colOpen;

	      if (date.Ok())
		{
		  STRING d;
		  if (date.Strftime("%d,%b,%Y", &d))
		    {
		      *StringBufferPtr
			<< "<A HREF=\"i.search?DATABASE%3D"
			<< DBname << "/TERM%3D%22" << MailDate
			<< URL_SLASH
			<< d
			<< "%22/RANK%3DC\" onMouseOver=\"\
self.status='Search for date'; return true\" TARGET=\"" << MailDate << "\">";
		      date_link = true;
		    }
		}
	    }
	  *StringBufferPtr << DescriptiveName(Language, MailDate, &tmp) <<  ": ";
	  if (UseHtml)
	    {
	      if (date_link) StringBufferPtr->Cat("</A>");
	      StringBufferPtr->Cat ("</TH><TD>");
	      if (date.Ok())
		*StringBufferPtr << "<!-- " << date.ISOdate() << " -->";
	    }
	  *StringBufferPtr << Value << "\n";
	  if (UseHtml) *StringBufferPtr << "</TD></TR>";
	}

      // Subject
      if (Subject.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen;
	  if (UseHtml)
	    {
	      STRING query; // Convert to Query;
	      query = rSubject; // the raw one for phrases!
	      const char *ptr = query.c_str();
	      // Want to Zap [Bah] Re:  and Re: [Bah] and Re: Re: etc.
	      do {
		while (isspace(*ptr) || *ptr == ':')
		  ptr++;
		while (*ptr == '[')
		  {
		    // Remove Id:
	 	    while (*ptr && *ptr != ']') ptr++;
		    if (*ptr == ']')
		      ptr++;
		    while (isspace(*ptr)) ptr++;
		  }
		// Remove the Re: Re: ... stuff
		while (strncasecmp(ptr, "Re:", 3) == 0)
		  {
		    ptr += 3;
		    while (isspace(*ptr) || *ptr == ':')
		      ptr++;
		  }
	      }  while (*ptr == '[');
		
	      if (*ptr == '\0')
		{
		  query = Subject;
		}
	      else if (!isalnum(*ptr))
		{
		  const char *tp = ptr;
		  // Stop leading *s...
		  while (*tp && !isalnum(*tp))
		    tp++;
		  if (*tp)
		    query = tp;
		  else
		    query = ptr;
		}
	      else query = ptr;

	      // Encode Anchor per IETF URL Specification
	      *StringBufferPtr << "<A HREF=\"" <<
		"i.search?DATABASE%3D" <<
		DBname << "/TERM%3D%22" << MailSubject << URL_SLASH;
	      for (const UCHR *tp = (const UCHR *)query.c_str(); *tp; tp++)
		{
		  if (isalnum(*tp))
		    StringBufferPtr->Cat (*tp);
		  else if (*tp == '\\')
		    StringBufferPtr->Cat("%5C%5C%5C%5C");
		  else if (isspace(*tp))
		    StringBufferPtr->Cat (',');
		  else if (*tp == '%')
		    StringBufferPtr->Cat ("%25");
		  else // Other characters
		    {
		      if (*tp == '"' || *tp == '*')
			StringBufferPtr->Cat("%5C%5C");
		      *StringBufferPtr << "%25" <<
			HEX[(*tp & '\377') >> 4] <<
			HEX[(*tp & '\377') % 16];
		    }
		}
	      *StringBufferPtr << "%22\" \
onMouseOver=\"self.status='Search for subject thread'; return true\" \
TARGET=\"Thread\">";

	    }
	  *StringBufferPtr << DescriptiveName(Language, MailSubject, &tmp) << ": ";
	  if (UseHtml) *StringBufferPtr << "</A>";
	  if (UseHtml) *StringBufferPtr << "</TH><TD><FONT SIZE=+1><STRONG>";

	  if (UseHtml)
	    HtmlCat(Subject, StringBufferPtr);
	  else
	    *StringBufferPtr << Subject;
	  if (UseHtml) *StringBufferPtr << "</STRONG></FONT></TD></TR>";
	  *StringBufferPtr << "\n";
	}

      // Keywords
      Present (ResultRecord, MailKeywords, RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen;
	  *StringBufferPtr << DescriptiveName(Language, MailKeywords, &tmp) << ": ";
	  if (UseHtml) *StringBufferPtr << "</TH><TD>";

	  // Zap extra spaces in Multi-line subject
	  *StringBufferPtr << Value.Pack () << "\n";
	  if (UseHtml) *StringBufferPtr << "</TD></TR>";
	}

      // Summary
      Present (ResultRecord, "Summary", RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen; 
	  *StringBufferPtr << DescriptiveName(Language, "Summary", &tmp) << ": ";
	  if (UseHtml) *StringBufferPtr << "</TH><TD>";

	  *StringBufferPtr << Value << "\n";
	  if (UseHtml) *StringBufferPtr << "</TD></TR>";
	}

      // Newsgroups
      DOCTYPE::Present (ResultRecord, "Newsgroups", &Value);
      if (Value.GetLength ())
	{
	  Value.Pack ();
	  if (UseHtml)
	    *StringBufferPtr << colOpen;
	  *StringBufferPtr << DescriptiveName(Language, "Newsgroups", &tmp) << ": ";

	  if (UseHtml)
	    {
	      STRLIST NewsList;

	      NewsList.Split (",", Value);
	      *StringBufferPtr << "</TH><TD><!-- " << Value << " -->";
	      int x = 0;
	      for (const STRLIST *p = NewsList.Next(); p != &NewsList; p = p->Next() )
		{
		  if (x++)
		    StringBufferPtr->Cat (", ");
		  *StringBufferPtr << "<A HREF=\"news:" << p->Value() << "\">"
			<< p->Value() << "</A>";
		}
	      *StringBufferPtr << "</TD></TR>";
	    }
	  else
	    *StringBufferPtr << Value << "\n";
	}

      // Other Recipients
      STRING Recipient, ReplyAddress;
      DOCTYPE::Present(ResultRecord, MailRecipientTag, &Recipient);

      // Handle now the Reply-To stuff
      DOCTYPE::Present (ResultRecord, MailReplyAddress, &ReplyAddress);

      // Sender 
      DOCTYPE::Present (ResultRecord, MailResentFrom, &Value);
      if (Value.GetLength () == 0)
	{
	  DOCTYPE::Present (ResultRecord, MailSender, &Value);
	}

      bool skip_item = false;
      if (Recipient.GetLength() != 0 && (Value != Recipient))
        {
	  // May still be same...
	  char buf[512];
	  Recipient.GetCString(buf, sizeof(buf)/sizeof(char) - 1);
	  STRING to (NameKey(buf, false));
          Value.GetCString(buf, sizeof(buf)/sizeof(char) - 1);
          STRING from (NameKey(buf, false));

	  bool skip = (to ^= from);
	  skip_item = skip;
	  
	  if (!skip)
	    {
	      // Check with reply address
	      ReplyAddress.GetCString(buf, sizeof(buf)/sizeof(char) - 1);
	      STRING reply (NameKey(buf, false));
	      skip = ((reply ^= from) || (reply ^= to));
	    }

	  if (!skip)
	    {
	      // Are different
	      if (UseHtml)
		StringBufferPtr->Cat (colOpen);
	      *StringBufferPtr << DescriptiveName(Language, MailRecipientTag, &tmp) << ": ";
	      if (UseHtml)
		{
		  *StringBufferPtr << "</TH><TD>";
		  Recipient.Replace(",", ", "); // Cosmetic
		  HtmlCat(ResultRecord, Recipient.Pack(), StringBufferPtr);
		  *StringBufferPtr << "</TD></TR>";
		}
	      else
		*StringBufferPtr << Recipient.Pack() << "\n";
	    }
        }

      if (Value.GetLength ())
	{
          if (!skip_item)
	    {
	      if (UseHtml)
		StringBufferPtr->Cat (colOpen);
	      *StringBufferPtr << DescriptiveName(Language, MailSender, &tmp) << ": ";
	      if (UseHtml)
		{
		  StringBufferPtr->Cat("</TH><TD>");
		  Mailto(Value, Subject, References, StringBufferPtr);
		  StringBufferPtr->Cat ("</TD></TR>");
		}
	      else
		StringBufferPtr->Cat (Value);
	      StringBufferPtr->Cat("\n");
	    }
	}

      if (ReplyAddress.GetLength ())
        {
          if (UseHtml)
            StringBufferPtr->Cat (colOpen);
          *StringBufferPtr << DescriptiveName(Language,  MailReplyAddress, &tmp) << ": ";
          if (UseHtml)
            {
              StringBufferPtr->Cat("</TH><TD>");
              Mailto(ReplyAddress, Subject, References, StringBufferPtr);
              StringBufferPtr->Cat ("</TD></TR>");
            }
          else
            StringBufferPtr->Cat (ReplyAddress);
          StringBufferPtr->Cat("\n");
        }

#if 0
     // The rest of the LIST stuff??
    if (mailingList)
      {
	DFDT   Dfdt;
 	DFD    Dfd;
	size_t total_fields;
        STRING field;

	Db->GetDfdt(&Dfdt);
	total_fields = Dfdt.GetTotalEntries();

	for (size_t i=1; i<= total_fields; i++)
	  {
	    Dfdt.GetEntry(i, &Dfd);
	    // Am only interested in List- 
	    if ((field = Dfd.GetFieldName()).CaseCompare("+list-", 6) != 0)
	      continue;
	    DOCTYPE::Present (ResultRecord, field, &Value);
	    if (Value.IsEmpty())
	      continue; // Nothing
	    Field.EraseBefore(2); // zap the '+'
	    if (UseHtml) StringBufferPtr->Cat (colOpen);
	    *StringBufferPtr << DescriptiveName(Language, field, &tmp) << ": ";
	    if (UseHtml)
	      {
		StringBufferPtr->Cat("</TH><TD>");
		HtmlCat (Value, StringBufferPtr);
		StringBufferPtr->Cat ("</TD></TR>");
	      }
	    else
	      StringBufferPtr->Cat (ReplyAddress);
	    StringBufferPtr->Cat("\n");
	 }
      }
#endif

#if 1 /* EXPERIMENTAL CODE */
      // Threads
      Thread (ResultRecord, MailInReplyTo, RecordSyntax, StringBufferPtr);
      Thread (ResultRecord, MailReferences, RecordSyntax, StringBufferPtr);
#endif

      // X-URL is tricky, can be a URL or can be HTML (seen both)
      // -- so we first fetch the raw field data
      DOCTYPE::Present (ResultRecord, "X-URL", &Value);
      if (Value.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen;
	  *StringBufferPtr << "See Also: ";
	  if (UseHtml) *StringBufferPtr << "</TH><TD>";

	  // Try to guess for HTMLRecordSyntax if HTML
	  STRINGINDEX pos;
	  // If Value contains <A or <a then assume HTML
	  if (!UseHtml ||
		((pos = Value.Search('<')) != 0 && toupper(Value.GetChr(pos+1)) == 'A' ))
	    {
	      // Insert RAW data
	      *StringBufferPtr << Value;
	    }
	  else
	    {
	      // Convert
	      HtmlCat(ResultRecord, Value, StringBufferPtr); // Convert
	    }
	  if (UseHtml) StringBufferPtr->Cat ("</TD></TR>");
	  *StringBufferPtr << "\n";
	}

      // Comments
      Present (ResultRecord, MailComments, RecordSyntax, &Value);
      if (Value.GetLength ())
	{
	  if (UseHtml) *StringBufferPtr << colOpen;
	  *StringBufferPtr << DescriptiveName(Language, MailComments, &tmp) << ": ";
	  if (UseHtml) *StringBufferPtr << "</TH><TD>";
	  *StringBufferPtr << Value.Pack () << "\n";
	  if (UseHtml) StringBufferPtr->Cat ("</TD></TR>");
	}


      if (UseHtml)
	{
#ifdef MAILDIGEST_HXX
	  if (Doctype ^= "MAILDIGEST")
	    {
	      RESULT pResult;
	      STRING pKey;
	      ResultRecord.GetKey(&pKey);
	      STRINGINDEX ix = pKey.SearchReverse(':');
	      if (ix)
		{
		  pKey.EraseAfter(ix-1);
		  if (Db->KeyLookup (pKey, &pResult))
		    {
		      STRING Title;
		      Present(pResult, BRIEF_MAGIC, RecordSyntax, &Title);
		      if (Title.GetLength())
			{
			  STRING furl;
			  Db->URLfrag(&furl);
			  *StringBufferPtr << colOpen << "Digest Issue:</TH><TD><A HREF=\"/cgi-bin/ifetch"
				<< furl << pKey << "\">" << Title << "</A></TD></TR>\n";
			}
		    }
		}
	    }
#endif
          *StringBufferPtr << "</TABLE><HR><PRE>";
	  if (Shrink)
	    {
	      *StringBufferPtr << "<!-- Shrink for this browser --><FONT SIZE=\"-1\">";
	    }
	}
      else
	{
	  static const char rule[] = "\n\
_________________________________________________________________\n";
	  *StringBufferPtr << rule;
	}
    }
  else
    {
      if (UseHtml)
	*StringBufferPtr << "<DL><DT>";
      *StringBufferPtr << ElementSet << ": ";
      if (UseHtml)
	*StringBufferPtr << "<DD>";
      if (UseHtml && ((ElementSet ^= MailBodyTag) || (ElementSet ^= MailHeadTag)))
	*StringBufferPtr << "<PRE>\n";
    }

  if (UseHtml)
    {
#if 1
      const CHR *sep;
      if (( sep = Seperator()) != NULL)
	{
	  INT pos = Content.SearchReverse( sep );
	  if (pos) Content.EraseAfter(pos);
	}
#endif
      HtmlCat (ResultRecord, Content, StringBufferPtr);
      if (ElementSet == FULLTEXT_MAGIC && Shrink)
	*StringBufferPtr << "</FONT>";
      if ((ElementSet == FULLTEXT_MAGIC) || (ElementSet ^= MailBodyTag) || (ElementSet ^= MailHeadTag))
	*StringBufferPtr << "</PRE>";
      if (ElementSet != FULLTEXT_MAGIC)
	*StringBufferPtr << "</DL>";
      HtmlTail (ResultRecord, ElementSet, StringBufferPtr); // Tail bits
    }
  else
    *StringBufferPtr << Content << "\n";
}


void MAILFOLDER::Present (const RESULT& ResultRecord,
  const STRING& ElementSet, const STRING& RecordSyntax,
  STRING *StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      bool Html = (RecordSyntax == HtmlRecordSyntax);
      STRING      rsyntax;

      if (Html)
	rsyntax = SutrsRecordSyntax;
      else
	rsyntax = RecordSyntax;

      // Brief Headline is "From: Subject"
      STRING SubjectValue;
      MAILFOLDER::Present (ResultRecord, MailSubject, rsyntax, &SubjectValue);

      SubjectValue.Pack (); // Zap extra spaces in Multi-line subject

      STRING Author;
      MAILFOLDER::Present (ResultRecord,  MailAuthorName, rsyntax, &Author);
      STRING Headline = Author +  ": " + SubjectValue;
      if (Html)
	{
	  StringBuffer->Clear();
	  HtmlCat(ResultRecord, Headline, StringBuffer, false);
	}
      else
	*StringBuffer = Headline;

    }
  else
    {
      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      if (StringBuffer->GetLength() == 0)
	{
	  STRING Val;
	  bool want_author;
	  if ((want_author = (ElementSet ^=  MailAuthorName)) || (ElementSet ^= MailAuthorAddress) ||
		(ElementSet ^= MailAuthor) )
	    {
	      STRING FromValue;

	      DOCTYPE::Present (ResultRecord, MailOriginator, SutrsRecordSyntax, &FromValue);
	      if (FromValue.IsEmpty())
		{
		  DOCTYPE::Present (ResultRecord, MailFrom, SutrsRecordSyntax, &FromValue);
		  if (FromValue.IsEmpty())
		    {
		      DOCTYPE::Present (ResultRecord, MailSender, SutrsRecordSyntax, &FromValue);
		    }
		}

	      if (!(ElementSet ^= MailAuthor))
		{
		  char buf[512];

		  FromValue.GetCString(buf, sizeof(buf)/sizeof(char) - 1);
		  Val = NameKey(buf, want_author);
		  if (want_author)
		    {
		      size_t pos = Val.Search('@');
		      if (pos) Val.EraseAfter(pos-1);
		    }
		  Val.Pack (); // Zap extra spaces
		  if (Val.IsEmpty())
		    Val = AuthorDefault;
		}
	      else
		Val = FromValue;
	    }
	  else if (ElementSet ^= MailSubject)
	    {
	      Val = SubjectDefault;
	    }
	  if (RecordSyntax == HtmlRecordSyntax)
	    HtmlCat (ResultRecord, Val, StringBuffer);
	  else
	    StringBuffer->Cat (Val);
	}
      else if ((ElementSet ^= MailBodyTag) || (ElementSet ^= MailHeadTag))
	{
	  if ((RecordSyntax == HtmlRecordSyntax))
	    {
	      if (StringBuffer->GetLength())
		{
		  StringBuffer->Insert(1, "<PRE>");
		  StringBuffer->Cat("</PRE>");
		}
	      else
		*StringBuffer = "\
<STRONG><FONT COLOR=\"Red\">&lt;NO OR UNREADABLE CONTENT ????&gt;</FONT></STRONG>";
	    }
	}
    }
}

MAILFOLDER::~MAILFOLDER ()
{
}

/*--------------------------------- Utility Functions -----------------------------*/

/*-
 * IsMailFromLine - Is this a legal unix mail "From " line?
 *
 * We check to make sure that the line has the format:
 * From <addr> <Day> <Month> <Day#> <hour>:<min.>[:<sec.>] [<TZ>] [<TZ mod>] <year>
 *
 * Examples of acceptable "from " lines:
 *    From foo@bar Fri Sep 30 21:15:05 1994
 *    From foo@bar Fri Sep 30 21:15:05 MET 1994
 *    From foo@bar Fri Sep 30 21:15:05 MET DST 1994
 *    From foo@bar Fri Sep 30 21:15 MET 1994
 *    From foo@bar Fri Sep 30 21:15 1994
 * as well as these odd-balls (never seen 'em):
 *    From foo@bar Fri Sep 30 21:15:05 +0200 1994
 *    From foo@bar Fri Sep 30 21:15:05 UTC +0200 1994
 *    From foo@bar Fri Sep 30 21:15:05 EET -0100 1994
 *
 */
bool MAILFOLDER::IsMailFromLine (const char *line) const
{
  const char magic[] = "From "; // Mail magic

// cerr << "XXXXXXXXXX first line: " << line << endl;

#define MAX_FIELDS 10
  char *fields[MAX_FIELDS];
  const char *sender_tail;
  register const char *lp;
  register char **fp;
  // Email (RFC822) has English language dates from 1 Jan 1970 on
  const char legal_day[] = "SunMonTueWedThuFriSat";
  const char legal_month[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  const int legal_numbers[] = {1, 31, 0, 23, 0, 59, 0, 60, 1969, 2199};
  //                           Month   Hour   Min    Sec     Year

  // Looking at a "magic" line?
  if (strncmp(line, magic, sizeof(magic)/sizeof(char) - 1) != 0)
    return false;

  // Move past magic
  lp = line + (sizeof(magic)/sizeof(char) - 1);

  /*
     Break up the line into fields
  */ 

  /* sender day mon dd hh:mm:ss year */
  int n;
  for (n = 0, fp = fields; n < MAX_FIELDS; n++)
    {
      while (*lp && *lp != '\n' && isspace (*lp))
	lp++;
      if (*lp == '\0' || *lp == '\n')
	break;
      *fp++ = (char *)lp;
      while (*lp && !isspace (*lp))
	if (*lp++ == ':' && (n == 4 || n == 5))
	  break;
      if (n == 0)
	sender_tail = lp;
    }

  // Minimum number of fields?
  if (n < 7) return false;

  fp = fields;

  /* 
     Expect:
	field 0: sender
	field 1: day_of_week
	field 2: month_name
	field 3: day_of_month
	field 4: dd
	field 5: hh
	field 6: ss
	field 7: year
     So we must mangle a bit...
  */ 

  if (n == 7)
    {
      if (!isdigit(fp[6][0])) return false;
      // From someone Tue Sep 19 15:33 1995
      fp[7] = fp[6]; // year
      fp[6] = (char *)"00"; // missing seconds
    }
  else if (n == 8 && !isdigit(fp[6][0]))
    fp[6] = (char *)"00";		/* ... hh:mm TZ year */
  if (n > 8 && !isdigit (fp[7][0]))
    fp[7] = fp[8];		/* ... hh:mm:ss TZ year */
  if (n > 9 && !isdigit (fp[7][0]))
    fp[7] = fp[9];		/* ... hh:mm:ss TZ DST year */

  // Check the day (Field 1)-- 7 days in a week
  fp++;
  const size_t dlen = (sizeof(legal_day)/sizeof(char) -1);
  const size_t dtoklen = dlen/7; /* Length of a day name */
  size_t i;
  for (i = 0; i < dlen; i += dtoklen)
    if (strncmp (*fp, &legal_day[i], dtoklen) == 0)
      break;
  if (i == dlen) return false; // Not a legal day

  // Check the month (Field 2)-- 12 months in a year
  fp++;
  const size_t mlen = (sizeof(legal_month)/sizeof(char) - 1);
  const size_t mtoklen = mlen/12; /* Length of a month name */
  for (i = 0; i < mlen; i += mtoklen)
    if (strncmp (*fp, &legal_month[i], mtoklen) == 0)
      break;
  if (i == mlen) return false; // Not a legal month

  // Check the numbers (bounds condition)-- field 3 to 7
  for (i = 0; i < (sizeof(legal_numbers)/sizeof(legal_numbers[0])); i += 2)
    {
      lp = *++fp;
      if (!isdigit (*lp)) {
	return false;
      }
      n = atoi (lp);
      if (n < legal_numbers[i] || legal_numbers[i + 1] < n) {
	 return false;
      }
    }

  // Looks good
  return true;
#undef MAX_FIELDS
}

/*-
 * Start of News:
 * "Article <Number> of <Newsgroup>:"
 */
bool MAILFOLDER::IsNewsLine (const char *line) const
{
  const char magic[] = "Article ";

  // News folder Magic?
  if (strncmp (line, magic, sizeof(magic)/sizeof(char) - 1) != 0)
    return false;

  // Move past magic
  line += (sizeof(magic)/sizeof(char) - 1);
  /* Skip white space */
  while (isspace (*line)) line++;

  if (!isdigit (*line)) return false; // No #
  if (atoi (line) < 1 ) return false; // Illegal Number
  /* skip number data */
  while (isdigit (*line)) line++;

  if (!isspace (*line)) return false;
  /* Skip white space */
  while (isspace (*line)) line++;

  if (line[0] != 'o' || line[1] != 'f') return false; // Missing "of"
  /* Skip the of */
  line += 2;

  if (!isspace (*line)) return false;
  /* Skip white space */
  while (isspace (*line)) line++;

  if (*line == '\0') return false; // Missing newsgroup name

  /* OK, if was "Article NNN of XXX.XXX.XXXXX:" */
  return true;
}


PCHR *MAILFOLDER::parse_tags (PCHR b, off_t len)
{
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING } State = HUNTING;

  // Skip leading bogus white space
  while (isspace(*b)) b++;
  // Is it a mail or news folder?
  if (IsMailFromLine(b) || IsNewsLine(b)) { 
    // Now skip the From/Article line
    while (*b != '\n') b++; // looking at end of line
    while (*b == '\n') b++; // Looking at first character
    // Should now be looking at fist tag line
  }

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  PCHR *t = (PCHR *)TagBuffer.Want(max_num_tags, sizeof(PCHR));

  off_t last_nl = 0;
  for (off_t i = 0; i < len; i++)
    {
      if (State == HUNTING)
	{
	  t[tc] = &b[i];
	  State = STARTED;
	  if (b[i] == '\n' || b[i] == '\r')
	   {
	      b[i] = '\0';
	      tc++;
	      break; // looking at body, done
	   }

	}
      // Handle continuation
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '\t'))
	{
	  State = CONTINUING;
	}
      else if ((State == STARTED) && (b[i] == ':'))
	{
	  b[i] = '\0';
	  // Expand memory if needed
	  t = (PCHR *)TagBuffer.Expand(++tc,  sizeof(PCHR));
	  State = CONTINUING;
	}
      else if ((State == CONTINUING) && (b[i] == '\n' || b[i] == '\r'))
	{
	  // Line below correct the problem with MS-DOS \r\n files
	  if (b[i] == '\r' && b[i+1] == '\n') i++; // 2007 Oct 31
	  State = HUNTING;
	}
    }
  t[tc] = (PCHR) NULL;
  return t;
}


////////////////////////////////////////////////////////////////////////////

static const char base64idx[128] = {
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377','\377','\377','\377','\377','\377',
    '\377','\377','\377',    62,'\377','\377','\377',    63,
        52,    53,    54,    55,    56,    57,    58,    59,
        60,    61,'\377','\377','\377','\377','\377','\377',
    '\377',     0,     1,     2,     3,     4,     5,     6,
         7,     8,     9,    10,    11,    12,    13,    14,
        15,    16,    17,    18,    19,    20,    21,    22,
        23,    24,    25,'\377','\377','\377','\377','\377',
    '\377',    26,    27,    28,    29,    30,    31,    32,
        33,    34,    35,    36,    37,    38,    39,    40,
        41,    42,    43,    44,    45,    46,    47,    48,
        49,    50,    51,'\377','\377','\377','\377','\377'
};

#ifdef __cplusplus
inline int isbase64(int a) {
    return ('A' <= a && a <= 'Z')
        || ('a' <= a && a <= 'z')
        || ('0' <= a && a <= '9')
        || a == '+' || a == '/';
}
#else
#define isbase64(a) (  ('A' <= (a) && (a) <= 'Z') \
                    || ('a' <= (a) && (a) <= 'z') \
                    || ('0' <= (a) && (a) <= '9') \
                    ||  (a) == '+' || (a) == '/'  )
#endif


// Base 64
static int Base64(const char* aIn, size_t aInLen, char* aOut,
    size_t aOutSize, size_t* aOutLen)
{
    if (!aIn || !aOut || !aOutLen)
        return -1;
    size_t inLen = aInLen;
    char* out = aOut;
    size_t outSize = (inLen+3)/4*3;
    if (aOutSize < outSize)
        return -1;
    /* Get four input chars at a time and decode them. Ignore white space
     * chars (CR, LF, SP, HT). If '=' is encountered, terminate input. If
     * a char other than white space, base64 char, or '=' is encountered,
     * flag an input error, but otherwise ignore the char.
     */
    int isErr = 0;
    int isEndSeen = 0;
    int b1, b2, b3;
    int a1, a2, a3, a4;
    size_t inPos = 0;
    size_t outPos = 0;
    while (inPos < inLen) {
        a1 = a2 = a3 = a4 = 0;
        while (inPos < inLen) {
            a1 = aIn[inPos++] & 0xFF;
            if (isbase64(a1)) {
                break;
            }
            else if (a1 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a1 != '\r' && a1 != '\n' && a1 != ' ' && a1 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a2 = aIn[inPos++] & 0xFF;
            if (isbase64(a2)) {
                break;
            }
            else if (a2 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a2 != '\r' && a2 != '\n' && a2 != ' ' && a2 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a3 = aIn[inPos++] & 0xFF;
            if (isbase64(a3)) {
                break;
            }
            else if (a3 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a3 != '\r' && a3 != '\n' && a3 != ' ' && a3 != '\t') {
                isErr = 1;
            }
        }
        while (inPos < inLen) {
            a4 = aIn[inPos++] & 0xFF;
            if (isbase64(a4)) {
                break;
            }
            else if (a4 == '=') {
                isEndSeen = 1;
                break;
            }
            else if (a4 != '\r' && a4 != '\n' && a4 != ' ' && a4 != '\t') {
                isErr = 1;
            }
        }
        if (isbase64(a1) && isbase64(a2) && isbase64(a3) && isbase64(a4)) {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            a4 = base64idx[a4] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            b3 = ((a3 << 6) & 0xC0) | ( a4       & 0x3F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            out[outPos++] = char(b3);
        }
        else if (isbase64(a1) && isbase64(a2) && isbase64(a3) && a4 == '=') {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            a3 = base64idx[a3] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            b2 = ((a2 << 4) & 0xF0) | ((a3 >> 2) & 0x0F);
            out[outPos++] = char(b1);
            out[outPos++] = char(b2);
            break;
        }
        else if (isbase64(a1) && isbase64(a2) && a3 == '=' && a4 == '=') {
            a1 = base64idx[a1] & 0xFF;
            a2 = base64idx[a2] & 0xFF;
            b1 = ((a1 << 2) & 0xFC) | ((a2 >> 4) & 0x03);
            out[outPos++] = char(b1);
            break;
        }
        else {
            break;
        }
        if (isEndSeen) {
            break;
        }
    } /* end while loop */
    *aOutLen = outPos;
    return (isErr) ? -1 : 0;
}

// Quoted Printable
static int QuotedPrintable(const char* aIn, size_t aInLen, char* aOut,
    size_t /* aOutSize */, size_t* aOutLen)
{
    size_t i, inPos, outPos, lineLen, nextLineStart, numChars, charsEnd;
    int isEolFound, softLineBrk, isError;
    int ch, c1, c2;

    if (!aIn || !aOut || !aOutLen)
        return -1;
    isError = 0;
    inPos = 0;
    outPos = 0;
    for (i=0; i < aInLen; ++i) {
        if (aIn[i] == 0) {
            aInLen = i;
            break;
        }
    }
    if (aInLen == 0) {
        aOut[0] = 0;
        *aOutLen = 0;
        return 0;
    }
    while (inPos < aInLen) {
        /* Get line */
        lineLen = 0;
        isEolFound = 0;
        while (!isEolFound && lineLen < aInLen - inPos) {
            ch = aIn[inPos+lineLen];
            ++lineLen;
            if (ch == '\n') {
                isEolFound = 1;
            }
        }
        nextLineStart = inPos + lineLen;
        numChars = lineLen;
        /* Remove white space from end of line */
        while (numChars > 0) {
            ch = aIn[inPos+numChars-1] & 0x7F;
            if (ch != '\n' && ch != '\r' && ch != ' ' && ch != '\t') {
                break;
            }
            --numChars;
        }
        charsEnd = inPos + numChars;
        /* Decode line */
        softLineBrk = 0;
        while (inPos < charsEnd) {
            ch = aIn[inPos++] & 0x7F;
            if (ch != '=') {
                /* Normal printable char */
                aOut[outPos++] = (char) ch;
            }
            else /* if (ch == '=') */ {
                /* Soft line break */
                if (inPos >= charsEnd) {
                    softLineBrk = 1;
                    break;
                }
                /* Non-printable char */
                else if (inPos < charsEnd-1) {
                    c1 = aIn[inPos++] & 0x7F;
                    if ('0' <= c1 && c1 <= '9')
                        c1 -= '0';
                    else if ('A' <= c1 && c1 <= 'F')
                        c1 = c1 - 'A' + 10;
                    else if ('a' <= c1 && c1 <= 'f')
                        c1 = c1 - 'a' + 10;
                    else
                        isError = 1;
                    c2 = aIn[inPos++] & 0x7F;
                    if ('0' <= c2 && c2 <= '9')
                        c2 -= '0';
                    else if ('A' <= c2 && c2 <= 'F')
                        c2 = c2 - 'A' + 10;
                    else if ('a' <= c2 && c2 <= 'f')
                        c2 = c2 - 'a' + 10;
                    else
                        isError = 1;
                    aOut[outPos++] = (char) ((c1 << 4) + c2);
                }
                else /* if (inPos == charsEnd-1) */ {
                    isError = 1;
                }
            }
        }
        if (isEolFound && !softLineBrk) {
            const char* cp = "\n";
            aOut[outPos++] = *cp++;
            if (*cp) {
                aOut[outPos++] = *cp;
            }
        }
        inPos = nextLineStart;
    }
    aOut[outPos] = 0;
    *aOutLen = outPos;
    return (isError) ? -1 : 0;
}
