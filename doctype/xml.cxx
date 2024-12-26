#include <ctype.h>
#include "common.hxx"
#include "xml.hxx"
#include "doc_conf.hxx"

#pragma ident  "@(#)xml.cxx  1.11 04/20/01 20:10:17 BSN"

XML::XML (PIDBOBJ DbParent, const STRING& Name) :
	SGMLNORM (DbParent, Name)
{
  message_log (LOG_DEBUG, "XML: Init");
}

XML::~XML ()
{
}

const char *XML::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("XML");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  SGMLNORM::Description(List);
  return "eXtended Mark-up Language (W3C XML standard)";
}


void XML::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for XML documents
  *StringPtr = "text/xml";
}

void XML::SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void XML::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const
{
//cerr << "XXXXXX   XML::Present " << ElementSet << "   docytpe=" << Doctype << endl;
  SGMLNORM::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


void XML::ParseRecords(const RECORD& FileRecord)
{
  STRING fn (FileRecord.GetFullFileName ());
  RECORD Record (FileRecord); // Easy way

  const off_t GlobalRecordStart = FileRecord.GetRecordStart();
  const off_t GlobalRecordEnd   = FileRecord.GetRecordEnd();

//cerr << "XML::ParseRecords" << endl;

  MMAP mapping (fn,
	GlobalRecordStart,
	GlobalRecordEnd ? GlobalRecordEnd+1 : 0,
	MapSequential);
  if (!mapping.Ok())
    {
      message_log (LOG_ERRNO, "%s::ParseRecords: Could not map '%s' into memory", Doctype.c_str(), fn.c_str());
      SGMLNORM::ParseRecords(FileRecord);
      return;
    }
  PCHR RecBuffer = (PCHR)mapping.Ptr();
  off_t ActualLength = mapping.Size();

  // Min. XML is: <x>xx</x>
  if (ActualLength < 10) {
    if (ActualLength == 0)
      message_log (LOG_WARN, "Record '%s' is empty.", fn.c_str());
    else
      message_log (LOG_WARN, "Record '%s' is too short [%lu byte(s)], skipping", fn.c_str(), ActualLength);
    return;
  }

  off_t Start = 0, End = 0, tagStart=0;
  STRING tag, eTag;
  int     recordCount = 0;

#if 1
  //
  // Check if I'm being not being fed garbage
  //
  int badXML = 0; // 1 true, -1 no, 0 is don't know yet 
  // Scan to first character
  for (off_t fStart=Start;fStart < ActualLength && (badXML>=0);fStart++) {
    int Ch = RecBuffer[fStart];
    switch (Ch) {
       case '<':
	badXML--; 
	if (tagStart == 0) tagStart = fStart;
	break;
       case 1: case 2: case 3: case 4: case 5: case 6:
       case 7: case 8: case 12: case 14: case 15: case 16:
       case 17: case 18: case 19: case 20: case 21: case 22:
       case 23: case 24: case 25: case 26: case 27: case 28:
       case 29: case 30: case 31: case 127:
          message_log (LOG_WARN, "%s Shunchar Control 0x%x encountered '%s'(%ld)!",
          Doctype.c_str(),Ch , fn.c_str(), fStart);
	  // fall into..
       case '>':
	  badXML++;
     }
    if (badXML > 2) break;
  }
  if (badXML > 1 || ((ActualLength-tagStart) < 5 || (tagStart - Start > 100))) {
    const char msg[] = " is not clean XML, skipping!";
    if (GlobalRecordStart > 0)
      message_log(LOG_ERROR, "%s: Record '%s'(%u-%u)%s", Doctype.c_str(),
	fn.c_str(), (unsigned)GlobalRecordStart, (unsigned)FileRecord.GetRecordEnd(), msg);
    else
      message_log(LOG_ERROR, "%s: Document '%s'%s", Doctype.c_str(), fn.c_str(), msg);
    return;
  } 
#endif

  // off_t level2_pos = 0;

 do {
  off_t   i;
  enum {ScanToStart, ScanToEnd, inComment, inTag, findEnd} State;

  // Search for 1st tag
  State = ScanToStart;
  tag.Clear();

//cerr << "ActualLength = " << ActualLength << endl;
  for (i = Start; i < ActualLength; i++)
    {
      CHR Ch = RecBuffer[i];

      if (State == inTag)
	{
	  if (isalnum(Ch) || strchr("-_:.", Ch))
	    {
	      // Have something OK
	      tag << Ch;
	    }
	  else
	    break;
	}
      else if (State == ScanToEnd && Ch == '>')
	{
	   State = ScanToStart; 
	}
      else if (State == inComment && Ch == '-' && i> 5)
	{
	  if (RecBuffer[i-1] == '-')
	    State = ScanToEnd;
	}
      else if (Ch == '<' && (i+5 < ActualLength))
	{
	  if ((Ch=RecBuffer[++i]) == '!')
	    {
	      if (RecBuffer[++i] == '-')
		State = inComment;
	    }
	  else if (isalnum(Ch))
	    {
	      State = inTag;
	      tag << Ch;
	    }
	}
    }

  // level2_pos = i;

  if (tag.IsEmpty() || i > (ActualLength-3))
    break; // Can't continue
  // We now have the first tag
  eTag.Clear();

// cerr << "XXXXXXXXXXXXX First Tag: <" << tag << "> ..... </" << tag << ">" << endl; 
  eTag << "</" << tag; // Need to look for this!

  message_log (LOG_DEBUG, "%s:ParseRecords Looking for %s", Doctype.c_str(), eTag.c_str());

  for (; i < ActualLength; i++)
    {
      size_t len;

      if (State == findEnd)
	{
	  if (RecBuffer[i] == '>')
	    {
	       i++; 
	       break; // Done
	    }
	}
      else if (RecBuffer[i] == '<' && ((off_t)(len = i+eTag.GetLength()) < ActualLength))
	{
	  if (strncasecmp(RecBuffer+i, eTag.c_str(), eTag.GetLength()) == 0 
		&& ( !isalnum(RecBuffer[len])  ) // Added 3 Oct 2006
		) 
	    {
//cerr << "Found " << eTag << endl;
	       State = findEnd;
	       i += eTag.GetLength()-1;
	    }
	  else if (RecBuffer[i+1] == '!' && RecBuffer[i+2] == '-')
	   {
	     // in Comment
	     for (i = i+2; i < ActualLength; i++)
	      {
		if (RecBuffer[i] == '-' && RecBuffer[i+1] == '-')
		   break; // Found end of comment
	      }
	   }
	}
    }

  while (i < ActualLength && isspace(RecBuffer[i]))
    {
      i++; // Skip white space
    }

  End = i-1;

  Record.SetRecordStart (Start+GlobalRecordStart);
  Record.SetRecordEnd (End+GlobalRecordStart);

  Db->DocTypeAddRecord (Record);
  Start = End+1;

  recordCount++;


 } while (Start < ActualLength);


#if 0
  // 2nd level parse
  if (recordCount == 1) {
     Record.SetRecordStart( level2_pos );
     Record.SetRecordEnd (Record.GetRecordEnd() - level2_pos - 1);
cerr << "Recurse.." << endl;
cerr << Record.GetRecordStart() << " -- " << Record.GetRecordEnd() << endl;
    ParseRecords(Record);
  }
#endif

 if (recordCount > 1) message_log (LOG_INFO, "%s: Added %d records.", Doctype.c_str(), recordCount);
}

/*
   Searches through string list t look for "/" followed by tag, e.g. if
   tag = "TITLE REL=XXX", looks for "/TITLE" or a empty end tag (</>).

   Pre: t is is list of string pointers each NULL-terminated.  The list
   should be terminated with a NULL character pointer.

   Post: Returns a pointer to found string or NULL.

   Note: Case dependent!
 */
const PCHR XML::find_end_tag (const char *const *t, const char *tag, size_t *offset) const
{

  if (tag == NULL || tag[0] == '/') return NULL; // End of an End???

  if (t == NULL || *t == NULL) return NULL; // Error


  if (*t[0] == '/') return NULL; // I'am confused!

  size_t len = 0;
  // Look for "real" tag name.. stop at white space
  while (tag[len] && !isspace(tag[len])) len++;
  // Have length
  //

  // Now search for nearest matching close
  size_t i = 0;
#define RETRO 1 /* Old XML draft */
#if RETRO
  const char *caseTag = NULL;
  const char firstCh = toupper(tag[0]);
#endif

  const char *tt = *t;
  do
    {
      if (tt[0] == '/')
	{
#if ACCEPT_SGML_EMPTY_TAGS 
	  // for empty end tags: see SGML Handbook Annex C.1.1.1, p.68
	  if (tt[1] == '\0' || tag[0] == '\0')
	    break;
#endif

	  // XML tags are mapped case DEPENDENT
	  if (tag[0] == tt[1] && strncmp (tt+1, tag, len) == 0)
	    {
	      // Bugfix Feb '97: Moved check here
	      if (tt[1 + len] == '\0' || isspace (tt[1 + len]))
		{
		  break;	// Found it
		}
	    }
#if RETRO
	  else if (caseTag == NULL && firstCh == toupper(tt[1]))
	    {
	      if (StrNCaseCmp (tt+1, tag, len) == 0 &&
		  (tt[1 + len] == '\0' || isspace (tt[1 + len])))
		{
		  caseTag = tt;
		}
	    }
#endif
	}
    } while ((tt = t[++i]) != NULL);


#if RETRO
  if (tt == NULL && caseTag != NULL)
    {
      message_log (LOG_NOTICE, "%s: Case independent match for <%s> used (violates XML 1.0 Spec).", Doctype.c_str(), tag);
      tt = caseTag;
    }
#endif

  if (offset) *offset = i;
  return (const PCHR) tt;
}


bool XML::StoreTagComplexAttributes(const char *tag_ptr) const
{
  return SGMLNORM::StoreTagComplexAttributes(tag_ptr);
}


