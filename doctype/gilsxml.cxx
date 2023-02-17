#include <stdlib.h>
#include <ctype.h>
#include "common.hxx"
#include "gilsxml.hxx"
#include "doc_conf.hxx"

#pragma ident  "@(#)gilsxml.cxx  1.6 04/20/01 20:09:48 BSN"

static const char defaultLevelCh = '\\';
static const char DefaultTypeKeyword[] = "Type";


/*
STRING XMLBASE::Getoption(const STRING& Option, const STRING& defaultVal)
{
  STRING string;
  STRING value;

  if (tagRegistry)
    tagRegistry->ProfileGetString("XML", Option, defaultVal, &string);
  else
    string = defaultVal;

  if (Db)
    Db->ProfileGetString(Doctype, Option, string, &value);
  else
    value = string;

  string.Clear();
  if (Db) Db->GetDocTypeOptionsPtr ()->GetValue(Option, &string);

  if (string.GetLength())
    {
      string.unEscape ();
      message_log (LOG_INFO, "%s: -o %s=%s overrides %s", Doctype.c_str(), Option, string.c_str(), value.c_str());
      value = string;
    }
  return value;
}
*/


GILSXML::GILSXML (PIDBOBJ DbParent, const STRING& Name) : XMLBASE (DbParent, Name)
{
  STRING string, value;
  const char toption[] = "DataTypeKeyword";

  message_log (LOG_DEBUG, "GILSXML Init");

  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "False").GetBool());

 if (tagRegistry)
    tagRegistry->ProfileGetString("XML", toption, DefaultTypeKeyword, &string);
  else
    string = DefaultTypeKeyword;

  if (DbParent)
    DbParent->ProfileGetString(Doctype, toption, string, &value);
  else
    value = string;

  string.Clear();
  if (DbParent)
    DbParent->GetDocTypeOptionsPtr ()->GetValue(toption, &string);

  if (string.GetLength())
    {
      string.unEscape ();
      message_log (LOG_INFO, "%s: -o %s=%s overrides %s", Doctype.c_str(),
        toption, string.c_str(), value.c_str());
      value = string;
    }
  if ((value.GetBool() == false) || (value.GetChr(1) == '<' ))
    value.Empty();
  typeKeyword = value;
  if (typeKeyword.GetLength() && (typeKeyword != DefaultTypeKeyword))
    message_log (LOG_INFO, "%s: Handling embeded data type definitions (%s)", typeKeyword.c_str());

  typeKeyword.Cat('=');
}

GILSXML::~GILSXML()
{
}

const char *GILSXML::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("GILSXML");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XMLBASE::Description(List);
  return "XML-like record format for GILS with special handling for\n\
heirarchical fields (example: for <a><b><c> defines a field a\\b\\c) and\n\
embeded TYPE selection as <tag Type=\"num\">...</tag> for numerical etc..\n\n\
Index time options:\n\
  [General]\n\
Complex=<0 or 1>                # Store complex attributes, default NO\n\n\
  [XML]\n\
  TagLevelSeperator=<character> # default '\\' (Alternative is '|')\n\
  DataTypeKeyword=<keyword>     # default Type, set to <empty> to disable\n\
alternatively in the [<Doctype>] section of <db>.ini.\n\n\
NOTE: The Type system intentionally only sets the type for the instances of\n\
the node and not for all instances of the tag. For example:\n\
  <file><length Type=\"num\">123</length>...</file> would define\n\
file\\length as numeric but not length.\n\n";
}


NEWSML::NEWSML (PIDBOBJ DbParent, const STRING& Name) : GILSXML (DbParent, Name)
{
  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "True").GetBool());
  if (KeyField.IsEmpty())          KeyField          = "NewsML@Duid";
  if (DateField.IsEmpty())         DateField         = "NewsML\\NewsEnvelope\\DateAndTime";
  if (DateCreatedField.IsEmpty())  DateCreatedField  = "FirstCreated";
  if (DateModifiedField.IsEmpty()) DateModifiedField = "ThisRevisionCreated";
  if (DateExpiresField.IsEmpty())  DateExpiresField  = "EndDate";
}

void NEWSML::LoadFieldTable()
{
  if (Db)
    {
      const char * const date_fields[] = {
	"DateAndTime",
	"DateId",
	"DateLabel",
	"DateLineDate",
	"FirstCreated",
	"ThisRevisionCreated",
	NULL};
      bool DateField_notseen = ! DateField.IsEmpty();
      for (size_t i=0; date_fields[i]; i++)
	{
	  if (DateField_notseen)
	    if (date_fields[i] ^= DateField) DateField_notseen = false;
	  Db->AddFieldType(date_fields[i], FIELDTYPE::date);
	}
      if (DateField_notseen && DateField.GetLength())
	Db->AddFieldType(DateField, FIELDTYPE::date);
    }
  DOCTYPE::LoadFieldTable();
}


void NEWSML::ParseRecords(const RECORD& FileRecord)
{
//  message_log (LOG_DEBUG, "%s: ParseRecords Start", Doctype.c_str());
  GILSXML::ParseRecords(FileRecord);
//  message_log (LOG_DEBUG, "%s: ParseRecords Done", Doctype.c_str());
} 

void NEWSML::ParseFields(PRECORD pRecord)
{
//  message_log (LOG_DEBUG, "%s: ParseFields Start", Doctype.c_str());
  GILSXML::ParseFields(pRecord);
//  message_log (LOG_DEBUG, "%s: ParseFields Done", Doctype.c_str());
}




const char *NEWSML::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("NewsML");
  if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  GILSXML::Description(List);
  return "XML-like record format \"to represent and manage news throughout its\n\
lifecycle, including production, interchange, and consumer use\"\n\
See: http://www.NewsML.org/\n";
}

NEWSML::~NEWSML() { }



XMLBASE::XMLBASE (PIDBOBJ DbParent, const STRING& Name) :
	XML (DbParent, Name)
{
  STRING string, value;
  const char option[] = "TagLevelSeparator";

  levelCh = defaultLevelCh;

  if (tagRegistry)
    tagRegistry->ProfileGetString("XML", option, levelCh, &string);
  else
    string = STRING(levelCh);

  if (DbParent)
    DbParent->ProfileGetString(Doctype, option, string, &value);
  else
    value = string;

  string.Clear();
  if (DbParent)
    DbParent->GetDocTypeOptionsPtr ()->GetValue(option, &string);

  if (string.GetLength())
    {
      string.unEscape ();
      message_log (LOG_INFO, "%s: -o %s=%s overrides %s", Doctype.c_str(),
	option, string.c_str(), value.c_str()); 
      value = string;
    }

  if (value.Equals(__AncestorDescendantSeperator))
    {
      message_log (LOG_ERROR, "%s: Reserved '%s' specified for %s. Using %c instead.",
                Doctype.c_str(), value.c_str(), option, levelCh);
      value = levelCh;
    }

  char Ch = value.GetChr(1);
  if (isspace(Ch))
    {
      for (const char *tcp = value.c_str(); (Ch = *tcp)!= '\0' && isspace(Ch); tcp++)
	/* */;
    }
  if (Ch != '\0' && !iscntrl(Ch))
    {
      if (Ch == '/' || Ch == '=' || Ch == '>' || Ch == '<')
        message_log (LOG_WARN, "%s: Potentially problematic character '%c' specified for %s.", Doctype.c_str(), Ch, option);
      else if (isalnum(Ch))
	message_log (LOG_WARN, "%s: Malicious  character '%c' specified for %s. Why?", Doctype.c_str(), Ch, option);
      else if (Ch == ':')
	message_log (LOG_WARN, "%s: '%c' as %s can create conflicts with its use in namespaces.", Doctype.c_str(), Ch, option);
      else if (Ch != '\\' && Ch != '|')
	message_log (LOG_INFO, "%s: Using '%c' for %s is not recommended (best is '\\' or '|')", Doctype.c_str(), Ch, option);
      if (levelCh != Ch)
	{
          message_log (LOG_DEBUG, "%s: Setting %s from %c to %c.", Doctype.c_str(), option, levelCh, Ch);
          levelCh = Ch;
	}
    }
  else
    message_log (LOG_ERROR, "%s: Nul or control character specified for %s, ignored.",
	Doctype.c_str(), option);

  message_log (LOG_DEBUG, "XMLBASE class inited");
}


#if 0
DOUBLE XMLBASE::GetNumericValue(const STRING& Tag, const char *Buffer)
{
  if (Tag.IsEmpty()) return ParseNumeric(Buffer);
  PCHR *tags = parse_tags (Buffer, strlen(Buffer));
  if (tags == NULL)
    return 0; // Nothing

  for (PCHR *tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      if (Tag ^= tags_ptr)
	{
	}
    }

  return(0.0); // Nothing
}
#endif


XMLBASE::~XMLBASE ()
{
  readBuffer.Free(Doctype.c_str(), "readBuffer");
  tempBuffer.Free(Doctype.c_str(), "tempBuffer");
}


bool XMLBASE::StoreTagComplexAttributes(const char *tag_ptr) const
{
  return false;
}


const char *XMLBASE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("XMLBASE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XML::Description(List);
  return "XML-like record format for with special handling for heirarchical\n\
fields (example: for <a><b><c> defines a field a\\b\\c)\n\n\
Index options:\n\
  [XML]\n\
  TagLevelSeperator=<character>  # default '\\'\n\
alternatively in the [<Doctype>] section of <db>.ini.\n\
NOTE: Root level tags then get a trailed character. Example:\n\
LOCATOR\\\n\
LOCATOR\\AVAILABILITY\n\
AVAILABILITY\n\
would be produced from <LOCATOR>..<AVAILABILITY>...</AVAILABILITY></LOCATOR>\n";
}


void XMLBASE::ParseRecords(const RECORD& FileRecord)
{
//cerr << "XMLBASE::ParseRecords" << endl;
  XML::ParseRecords(FileRecord); 
}

char *XMLBASE::_get_value(char *Buffer, size_t val_len)
{
  STRING value;

  char *entry = (char *)tempBuffer.Want(val_len + 1);
  strncpy (entry, Buffer, val_len);
  entry[val_len] = '\0';

  if (strchr(entry, '<'))
    {
      value = STRING(entry).XMLCommentStrip();
      memcpy(entry, value.c_str(), value.GetLength()+1);
    }
  return entry;
}

#if 0

PDFT XMLBASE::ParseStructure(FILE *fp, off_t Position, off_t Length)
{
  PDFT        pdft = new DFT();

  if (!ParseBody)
    return pdft; // Nothing to parse

  off_t PageStart = Position < 0 ? ftell(fp) : Position;
  off_t ParaStart = PageStart;
  off_t LineStart = ParaStart;
  off_t SatzStart = LineStart;

  long  lineCount = 0;
  long  satzCount = 0;
  long  paraCount = 0;
  long  pageCount = 0;
  bool have_headline_field = !(headlineFieldName.IsEmpty());

  int Ch;
  off_t pos = 0;
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan = 0, Start, Newline, Punct, LineBreak, ParaBreak, PageBreak} State = Scan;
#if DEBUG
  char *StateNames[] = {"Scan", "Start", "Newline", "Punct", "LineBreak", "ParaBreak", "PageBreak" };
#endif


while (pos < Length && Length != 0) {
  FC fc;
  DF df;

#if DEBUG
# define STATE_ANNOUNCE(_x) {\
  cerr << "****** POS=" << pos << "->" <<  StateNames[(int)(_x)] << "  Previous state was: " << StateNames[(int)State] << endl;} 

  STATE_ANNOUNCE(Scan);
#else
# define STATE_ANNOUNCE(_x)
#endif

  State = Scan;
  for (;;) {
    if (++pos > Length && Length != 0) break;
    Ch = getc(fp);

#if DEBUG
    cerr << "POS [" << pos << "] = " << (char)Ch << endl;
#endif

    // Kludge to handle MSDOS's \r\n for line characters
    if (Ch == '\r')
      {
	// \r is the MAC end-of-line so we need to be carefull!
        if ((Ch = getc(fp)) != '\n')
	  {
	    ungetc(Ch, fp); // pushback character
	    Ch = '\r';
	  }
	else
	  if (++pos > Length && Length != 0) break;; // increment count
      }

    if (State == Scan)
      {
#if DEBUG
        cerr << "State = Scan, pos=" << pos << "  Ch=" << (int)Ch << endl;
#endif
	if (isalnum(Ch))
	  {
	    STATE_ANNOUNCE(Start);
	    State = Start;
	  }
	SatzStart = pos-1;

	if (Ch == CTL_PB && (pos - 4) > PageStart)
	  {
#if DEBUG
	    cerr << "Write PAGE: " << PageStart << "-" << pos-1 << endl;
#endif
	    fc.SetFieldStart(PageStart);
	    fc.SetFieldEnd (pos-1);
	    PageStart = pos+1;
	    df.SetFct (fc);
	    df.SetFieldName (pageFieldName);
	    pdft->AddEntry (df);
	    ++pageCount;
	  }
      }
    else if (Ch == '\r' || Ch == '\n' || Ch == EOF || Ch == CTL_PB)
      {
#if DEBUG
        cerr << "Parser found linebreak ch=" << (int)Ch <<  endl;
#endif
	// At least 2 characters wide
	if (pos-2 > LineStart)
	  {
	    if (State != Newline)
	      {
#if DEBUG
		cerr << "Write Line: " << LineStart << "-" << pos-1 << endl;
#endif
	        // We now have a line
	        fc.SetFieldStart(LineStart);
	        fc.SetFieldEnd (pos-1);
	        df.SetFct (fc);
	        df.SetFieldName (lineFieldName);
	        pdft->AddEntry (df);
	        if (++lineCount == 1)
		  {
		    df.SetFieldName(firstlineFieldName);
		    pdft->AddEntry(df);
#if USE_HEADINE_FIELD 
		    if (satzCount > 0 && have_headline_field)
		      {
			df.SetFieldName(headlineFieldName);
			pdft->AddEntry(df);
		      }
#endif
		  }
	     }
	    LineStart = pos;
	  }
	if (Ch == EOF || Ch == CTL_PB)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
	else if (State == Newline || State == Punct)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    LineStart = pos;
	    State = ParaBreak;
	    break; // Done
	  }
	else
	  {
	    STATE_ANNOUNCE(Newline);
	    State = Newline;
	  }
      }
    else if (State == LineBreak)
      {
#if DEBUG
	cerr << "Parse was in linebreak modus" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	cerr << "This is a space so its a pagrapraph break" << endl;
#endif
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
      } 
    else if (State == Newline)
      {
	if (isalnum(Ch))
	  {
	    // See a character after the newline
#if DEBUG
	     cerr << "We were in newline mode and we now see an alpha-num character: " << (char)Ch << endl;
#endif
	     STATE_ANNOUNCE(State);
	     State = Start;
	  }
	else if (Ch == '\t' || Ch == '\v' || Ch == EOF || Ch == CTL_PB)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
	else if (Ch == '\f' || Ch == ' ')
	  {
#if DEBUG
	    cerr << "Newline modus sees: " << (int)Ch << endl;
#endif
	    State = LineBreak; // We might be going into a paragraph
	  }
	else if (isspace(Ch))
	  break;
      }
    else if (State == Punct)
      {
#if DEBUG
	cerr << "State is punctuation" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	    cerr << "And we have a space so break out of loop" << endl;
#endif
	    break; // Done
	  }
      }
    else if (Ch == CTL_PB)
      {
	State = PageBreak;
	break;
      }
    else if (Ch == '.' || Ch == '!' || Ch == '?')
      {
	STATE_ANNOUNCE(Punct);
	State = Punct;
      }
  }		// for(;;)

  off_t p = pos;
  if (State == Newline) p--; // Leave off NL
  if (isspace(Ch)) p--; // Leave off trailing white space

  if (Length && SatzStart >= Length) break; // Nothing

 if (State == PageBreak && (p-2 > PageStart))
    {
      fc.SetFieldStart(PageStart);
      fc.SetFieldEnd (p-1);
      PageStart = pos;
      df.SetFct (fc);
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
      State = ParaBreak; // Its also a paragraph break
    }
  if ((State == ParaBreak || State == Newline) && (p-2 > ParaStart))
    {
#if DEBUG_PARAGRAPH
      STRING S;
      S.Fread(Debug_Fp, p-ParaStart, ParaStart);
      cerr << "Paragraph goes from: " << ParaStart << " - " << p -1 << endl;
      cerr << "<paragraph>" << S << "</paragraph>" << endl;
#endif
      fc.SetFieldStart(ParaStart);
      fc.SetFieldEnd (p-1);
      ParaStart = pos;
      df.SetFct (fc);
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);
      if (++paraCount == 1)
	{
	  df.SetFieldName (firstparaFieldName);
	  pdft->AddEntry (df);
	}
    }

  if (p-2 > SatzStart)
    {
#if DEBUG_SENTENCE
      STRING S;
      S.Fread(Debug_Fp, p-SatzStart, SatzStart);
      cerr << "Sentence goes from: " << SatzStart << " - " << p -1 << endl;
      cerr << "<sentence ID=\"" << satzCount+1 << "\">" << S << "</sentence>" << endl; 
#endif
      fc.SetFieldStart (SatzStart);
      fc.SetFieldEnd (p-1);
      df.SetFct (fc);
      df.SetFieldName (satzFieldName);
      pdft->AddEntry (df);
      if (++satzCount == 1)
        {
          df.SetFieldName (firstsatzFieldName);
          pdft->AddEntry (df);
#if USE_HEADINE_FIELD 
	  if (lineCount > 0 && have_headline_field)
	    {
	      df.SetFieldName(headlineFieldName);
	      pdft->AddEntry(df);
	    }
#endif
        }
    }

  if (Ch == EOF)
    break; // Done
}

  if ((pos-2) > ParaStart)
    {
#if DEBUG
      cerr << "FINAL PARAGRAPH" << endl;
#endif
#if DEBUG_PARAGRAPH
      STRING S;
      S.Fread(Debug_Fp, pos-ParaStart, ParaStart);
      cerr << "Paragraph goes from: " << ParaStart << " - " << pos -1 << endl;
      cerr << "<paragraph>" << S << "</paragraph>" << endl;
#endif
      DF df;
      df.SetFct ( FC(ParaStart, pos));
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);
      paraCount++;
    }

  if ((pos - 2) > PageStart)
    {
#if DEBUG
     cerr << "Write FINAL page: " << PageStart << "-" << pos << endl;
#endif
      // Final page
      DF df;
      df.SetFct (FC(PageStart, pos));
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
    }


  message_log (LOG_INFO, "%s: %ld lines/%ld sentences/%ld paragraphs/%ld pages.",
	Doctype.c_str(), lineCount, satzCount, paraCount, pageCount);

  return pdft;
}

#endif

void XMLBASE::ParseFields (PRECORD NewRecord)
{
  PFILE fp;
  SRCH_DATE Now, Datum;

  Now.SetNow();

  if (NewRecord == NULL) return; // Error
  // Open the file
  const STRING fn = NewRecord->GetFullFileName ();
  if (!(fp = fopen (fn, "rb")))
    {
      message_log (LOG_ERRNO, "Couldn't access '%s'", fn.c_str());
      return;			// ERROR
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    {
      fseek (fp, 0L, SEEK_END);
      RecStart = 0;
      RecEnd = ftell (fp);
    }

  if (RecEnd <= RecStart)
    {
      message_log (LOG_ERROR, "'%s' (%s) Record Start = Record End (Zero length), skipping..", fn.c_str(), Doctype.c_str());
      ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  fseek (fp, (long)RecStart, SEEK_SET);

  // Read the whole record in a buffer
  GPTYPE RecLength = RecEnd - RecStart + 1;

  PCHR RecBuffer = (PCHR)readBuffer.Want(RecLength + 1);

//cerr << "Read Buffer" << endl;

  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

//cerr << "End of" << endl;

  fclose (fp);

  STRING FieldName, Key;
  DF df;
  DFD dfd;

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL)
    {
      message_log (LOG_WARN, "Unable to parse `%s' tags in file %s", Doctype.c_str(), fn.c_str());
      return;
    }
  PDFT pdft = new DFT ();

  int NestLevel=0;		// Nesting level of tag pairs
  STRING TagPath;		// Tag path from root of SGMLNORM tree, without attributes
  STRINGINDEX TagPathTCP;	// Index to end of tag name (where attributes should start)
  STRING TagPathWithAttrs;	// Whole tag path, including attributes of last tag in path
  STRINGINDEX i;

  for (PCHR *tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
//cerr << "Looking at '" << *(tags_ptr) << "'" <<  endl;
      if ((*tags_ptr)[0] == '?')
	{
//cerr << "XXXX  DEBUG: ignore <? .." << endl;
          continue; // Ignore <?...?> for now
	}

      if ((*tags_ptr)[0] == '/') {
	// Going one level up; if at level 0, this is an error in the input.
	// Otherwise, decrement nestlevel and update tagpath by
	// removing the rightmost tag in the tagpath.
	// XXX Should check that (*tags_ptr)[1] matches TagPath[i] ?
	//     (not for SGML... perhaps for SGMLNORM)


	if ((*tags_ptr)[1] == 'p' && (*tags_ptr)[2] == '\0')
	    continue; // See IP: Ignore paragraphs below

	if(NestLevel) {
	  NestLevel--;
          if(NestLevel) {
	    // Find & remove rightmost tag from the path
//cerr << "Going in with " << TagPath;
	    i = TagPath.SearchReverse(levelCh);
	    while (TagPath.GetChar(i) == levelCh) i--;
	    if (i) TagPath.EraseAfter(i-1);
//cerr << "   Outwith " << TagPath << endl;
          } else {
	    // At level 0, tagpath is empty
            TagPath.Clear();
          }
//cerr << "Left nestlevel " << NestLevel+1 << ". Tagpath='" << TagPath << "'.\n";
	} else {
	  message_log (LOG_WARN, "Extraneous end tag '%s' in file \"%s\".",  *tags_ptr, fn.c_str());
        }
	continue;
      }

      const size_t tag_len = strlen (*tags_ptr);
      if (isalnum((*tags_ptr)[0]) && (*tags_ptr)[tag_len - 1] == '/')
        {
          if (strchr (*tags_ptr, '='))
            {
	      (*tags_ptr)[tag_len - 1] = 0; // edz 2021 addition
              store_attributes (pdft, RecBuffer, *tags_ptr, false, &Key, &Datum);
            }
#if DEBUG
cerr << "NO CONTENT (empty tag)" << endl;
#endif
          continue; // No content
        }

      size_t offset = 0;
      const CHR *p = find_end_tag (tags_ptr, *tags_ptr, &offset);
      int have_attribute_val = (NULL != strchr (*tags_ptr, '='));

// cerr << "@@@@ Start tag =" << *tags_ptr << " end=" << p << " length=" << p-*tags_ptr << endl;

      if (p == NULL && have_attribute_val == 0)
	continue;

      // tags_ptr[offset] = ""; // Don't re-use same end-tag

#if 1
      if (p && have_attribute_val)
	{
	  STRING MetaField("."); 
	  dfd.SetFieldName (MetaField);
	  Db->DfdtAddEntry (dfd);
	  df.SetFct ( FC(*tags_ptr - RecBuffer,  p - RecBuffer + strlen(p)) );
	  df.SetFieldName (MetaField);
	  pdft->AddEntry (df);
	}
#endif

      // Opening tag found. Append it to TagPath, increment NestLevel

      // Do we want /root or root/
#ifndef ROOT_SLASH
      if(NestLevel)		// Add tagpath separator char if needed
#endif
        TagPath += levelCh;
      TagPath += *tags_ptr;	// Add tag to tagpath
      // Make a copy of tag path with all attributes
      TagPathWithAttrs = TagPath;
      // Remove attributes from tagpath
      if ((TagPathTCP = TagPath.FirstWhiteSpace())>0)
	TagPath.EraseAfter(TagPathTCP-1); // Break at first white space
      NestLevel++;
//cerr << "Entered nestlevel " << NestLevel << ". Tagpath = '" << TagPath << "'" << "\n";


      if (p != NULL)
	{
	  // We have a tag pair
	  size_t val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
	  int val_len = (p - *tags_ptr) - tag_len - 2;

	  // Skip leading white space
	  while (isspace (RecBuffer[val_start]))
	    val_start++, val_len--;
	  // Leave off trailing white space
	  while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	    val_len--;

	  // Don't bother storing empty fields
	  if (val_len > 0)
	    {
	      // Cut the complex values from field name
	      CHR orig_char = 0;
	      char* tcp;
	      for (tcp = *tags_ptr; *tcp; tcp++)
		if (isspace (*tcp))
		  {
		    orig_char = *tcp;
		    *tcp = '\0';
		    break;
		  }
#if ACCEPT_EMPTY_TAGS
	      if (*tags_ptr[0] == '\0')
		{
		  // Tag name is the name of the last element
		  if (FieldName.GetLength () == 0)
		    {
		      // Give some information
		      message_log (LOG_WARN, "\
%s: Warning: \"%s\" offset %ld: Bad use of empty tag feature, skipping field.\n",
			Doctype.c_Str(), fn.c_str(), (long)(*tags_ptr - RecBuffer)); 
		      continue;
		    }
		}
	      else
		{
#endif

		  if (UnifiedName(*tags_ptr, &FieldName).IsEmpty())
		    continue; // ignore these
#if ACCEPT_EMPTY_TAGS
		}
#endif
	      if (orig_char)
		*tcp = orig_char;

	      // Make sure its a good printable fieldname
	      if (!FieldName.IsPrint())
		{
		  FieldName.ToPrint();
		  message_log (LOG_WARN, "Non-ascii characters found in '%s'", FieldName.c_str());
		}

//--> Add Entry
	      // Atomic fieldname (also applies to type of tagPath)
	      FIELDTYPE ft ( Db->GetFieldType(FieldName) );
	      dfd.SetFieldType( ft ); // Get the type

	      dfd.SetFieldName (FieldName);
	      Db->DfdtAddEntry (dfd);


	      df.SetFct ( FC(val_start, val_start + val_len - 1) );
#if 0
{
   char *contents = (char *)tempBuffer.Want(val_len + 1);
   strncpy (contents, &RecBuffer[val_start], val_len); contents[val_len] = '\0';
   cerr << "** FIELD " << FieldName << " = " << STRING(contents).XMLCommentStrip() << endl;
}
#endif
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);

	      if ((FieldName ^= TTLfieldName) && (TTLfieldName != TTLField))
		{
		  SRCH_DATE     expiry (Now);
		  char         *entry_id = _get_value(&RecBuffer[val_start], val_len);

		  expiry.PlusNminutes(atol(entry_id));
		  NewRecord->SetDateExpires( expiry );
		}
              else if (IsSpecialField(FieldName))
                {
		  HandleSpecialFields(NewRecord, FieldName, _get_value(&RecBuffer[val_start], val_len));
                }
	      // Store data for the whole tagpath as well.


//cerr << "FieldNAme = " << FieldName << endl;
//cerr << "TagPath   = " << TagPath << endl;
//cerr << "DF " << df << endl;
//if (TagPath.Contains("\\p\\")) {cerr << "BAD: " << fn << endl;  abort(); }
//if (TagPath.Contains("\\item\\item") ) {cerr << "BAD: " << fn << endl;  abort(); }


	      // Here we look at the path....
	      if (UnifiedName(TagPath, &FieldName).IsEmpty())
		continue; // ignore these
	      // To distinguish between top-level tags (ie. tag paths with just one
	      // tag component) and tags with same name but residing deeper in the
	      // SGML structure, we add a trailing backslash (levelCh) to level 1
	      // tagpaths.
	      //
#ifndef ROOT_SLASH
	      if(NestLevel==1)
	        FieldName.Cat(levelCh);
#endif


	      // sanity check on fieldname 
	      if (!dfd.checkFieldName(FieldName))
		continue; // bad field name

  	      dfd.SetFieldName (FieldName);

	      if (!ft.IsText())
		  Db->AddFieldType(FieldName, ft);

	      Db->DfdtAddEntry (dfd);
	      // df.SetFct ( FC(val_start, val_start + val_len - 1) );
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);

              if (IsSpecialField(FieldName))
                {
		  char *entry_id =  _get_value(&RecBuffer[val_start], val_len);
                  if ((FieldName ^= KeyField) && !Key.IsEmpty())
		    message_log(LOG_WARN, "Duplicate Keys defined: overwriting %s with %s", Key.c_str(), entry_id);
		  HandleSpecialFields(NewRecord, FieldName, entry_id);
                }
	    }
//else cerr << "EMPTY FIELD" << endl;
	}
      if (have_attribute_val)
	{
	  store_attributes (pdft, RecBuffer, *tags_ptr, false, &Key, &Datum);
	  // Store attributes as attributes of the whole tag path as well
	  // if TagPath!=Tagname, ie. nestlevel!=0
	  if(NestLevel) {

	    // ---- added 9 Nov 2003 to handle type=
           if (typeKeyword.GetLength()) {

	    STRINGINDEX pos = TagPathWithAttrs.SearchAny(typeKeyword);
	    if (pos)
	      {
		STRING substring = TagPathWithAttrs.Right(
			TagPathWithAttrs.GetLength()-pos+1);
		const char *tp = substring.c_str() + typeKeyword.GetLength();
		while (isspace(*tp)) tp++; // skip spaces
		if (*tp == '\'' || *tp == '"') tp++;
		char tmp[24];
		for (size_t x=0; x<sizeof(tmp); x++)
		  {
		    tmp[x] = *tp++;
		    if (tmp[x] == '"' || tmp[x] == '\'' || isspace(tmp[x]))
		      {
			tmp[x] = '\0';
			break;
		      }
		  }
		FIELDTYPE ft (tmp);
		if (ft.Ok())
		  {
		    Db->AddFieldType(TagPath, ft);
		    dfd.SetFieldType( ft ); // Get the type
		    dfd.SetFieldName ( TagPath );
		    Db->DfdtAddEntry (dfd);
		    continue;
		  }
	      }
	    }
	    // ---- end added code of 9 Nov 2003
	    store_attributes(pdft, RecBuffer,
		(char *)(TagPathWithAttrs.c_str()), false, &Key, &Datum);
	  }
	}
      else if (p == NULL)
	{
#if 0
	  // Give some information
	  cout << doctype << " Warning: \""
	    << fn << "\" offset " << (*tags_ptr - RecBuffer) << ": "
	    << "No end tag for <" << *tags_ptr << "> found, skipping field.\n";
#endif
	}
      // If no endtag is found, TagPath must be trimmed to keep it in sync.
      if(p == NULL) {
	if(NestLevel) {
	  NestLevel--;
          if(NestLevel) {
	    i = TagPath.SearchReverse(levelCh);
	    while (TagPath.GetChar(i) == levelCh) /* loop */i--;
            TagPath.EraseAfter(i-1);
          } else {
            TagPath.Clear();
          }
        }
      }
    }

  NewRecord->SetDft (*pdft);

  if (Datum.Ok()) NewRecord->SetDate( Datum );
  if (!Key.IsEmpty())
    {
      if (Db->KeyLookup (Key))
        {
          message_log (LOG_WARN, "Record in \"%s\" used a non-unique %s '%s'",
                fn.c_str(), KeyField.c_str(), Key.c_str());
          Db->MdtSetUniqueKey(NewRecord, Key);
        }
      else
        NewRecord->SetKey (Key);
    }
  // Clean up;
//  delete tags; // 9 Oct 2003 Don't do this, created with BUFFER(!)
  delete pdft;
}

///////////////// Code to handle the Heirarchical Structure

struct DTAB {
    DFD Dfd;
    FC Fc;
};

static int DfdtCompare (const void *p1, const void *p2)
{
  int dif = ((struct DTAB *) p1)->Fc.GetFieldStart() -
	((struct DTAB *) p2)->Fc.GetFieldStart();
  if (dif == 0)
    dif = ((struct DTAB *) p2)->Fc.GetFieldEnd() -
	((struct DTAB *) p1)->Fc.GetFieldEnd();
  return dif;
}


bool XMLBASE::GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const
{
  PMDT MainMdt = Db->GetMainMdt ();
  PDFDT MainDfdt = Db->GetMainDfdt ();

  DfdtBuffer->Clear();

  MDTREC Mdtrec;
  if (!MainMdt->GetMdtRecord (Key, &Mdtrec))
    return false;

  const GPTYPE MdtS = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordStart ();
  const GPTYPE MdtE = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordEnd ();

  FC MdtFc (MdtS, MdtE);

  DFD dfd;
  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  size_t count = 0;
  struct DTAB *Table = new struct DTAB[TotalEntries*20];
  STRING Fn;
  for (size_t x = 1; x <= TotalEntries; x++)
    {
      MainDfdt->GetEntry (x, &dfd);
      const STRING FieldName = dfd.GetFieldName ();
      if (Db->DfdtGetFileName (FieldName, &Fn) == false)
	continue;
      PFILE Fp = Fn.Fopen ("rb");
      if (Fp)
	{
	  // Get Total coordinates in file
	  const size_t Total = GetFileSize (Fp) / sizeof (FC);	// used fseek(Fp)

	  if (Total)
	    {
	      size_t Low = 0;
	      size_t High = Total - 1;
	      size_t X = (High / 2);
	      INT OldX;
	      FC Fc;	// Field Coordinates

	      do
		{
		  OldX = X;
		  if (-1 == fseek (Fp, X * sizeof (FC), SEEK_SET) ||
			Fc.Read (Fp) == false)
		    {
		      // Read Error
		      if (++X >= Total) X = Total - 1;
		      continue;
		    }
		  if (MdtFc.Compare(Fc) == 0)
		    {
		      // Find first... 
		      if (X != 0)
			{
			  do {
			    if (-1 == fseek(Fp, (--X)*sizeof(FC), SEEK_SET) || Fc.Read(Fp) == false)
			      break;
			  } while (X > 0 && MdtFc.Compare(Fc) == 0);
			  if (MdtFc.Compare(Fc) != 0)
			    Fc.Read(Fp);
			}
		      // Now Add
		      do {
			Table[count].Dfd = dfd;
			Table[count++].Fc = Fc;
		      } while (Fc.Read(Fp) && MdtFc.Compare(Fc) == 0);
		      break;	// Done this loop
		    }
		  if (MdtE < Fc.GetFieldEnd())
		    High = X;
		  else
		    Low = X + 1;
		  // Check range
		  if ((X = (INT)(Low + High) / 2) >= Total)
		    X = Total - 1;
		}
	      while (X != OldX);
	    }
	  fclose (Fp);
	}
    }				// for()


  // qsort Table and put into DfdtBuffer so
  // that it represents the "order" in the record
  QSORT (Table, count, sizeof (struct DTAB), DfdtCompare);
  // Now add things NOT *outside* groups
  DfdtBuffer->Resize(count+1);
  for (int i=0; i < count; i++)
    {
      if (Table[i].Dfd.GetFieldName().Search(levelCh) == 0)
	continue;
      const GPTYPE start = Table[i].Fc.GetFieldStart();
      const GPTYPE end   = Table[i].Fc.GetFieldEnd();
      bool found = false; 
      for (int j=1; j < count && !found; j++)
	{
	  if (i == j || Table[j].Dfd.GetFieldName().Search(levelCh) == 0)
	    continue;
	  if (start <= Table[j].Fc.GetFieldStart() && end >= Table[j].Fc.GetFieldEnd())
	    {
	      found = true;
	    }
	}
      if (!found)
	{
	  DfdtBuffer->FastAddEntry (Table[i].Dfd);
	}
    }
  delete[] Table;

  return true;
}


void XMLBASE::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const
{
#if 0
  if (ElementSet == FULLTEXT_MAGIC)
    {
      DFDT Dfdt;
      if (GetRecordDfdt ( ResultRecord.GetKey(), &Dfdt))
	{
	  const size_t Total = Dfdt.GetTotalEntries();
	  DFD Dfd;
	  STRLIST Position, Value;
	  METADATA Meta(Doctype);
	  for (size_t i=1; i<=Total; i++)
	    {
	      Dfdt.GetEntry (i, &Dfd);
	      STRING FieldName = Dfd.GetFieldName ();
	      Db->GetFieldData(ResultRecord, FieldName, &Value);
	      if (!Value.IsEmpty())
		{
		  // Have Something
		  Position.Split(levelCh, FieldName); // Break up the field
		  Meta.SetData(Position, Value);
		}
	    }
	  if (RecordSyntax == HtmlRecordSyntax)
	    {
	      HtmlHead (ResultRecord, ElementSet, StringBuffer);
	      StringBuffer->Cat (Meta.Html());
	      HtmlTail (ResultRecord, ElementSet, StringBuffer);
	    }
	  else
	    *StringBuffer = Meta.Text();
	  return; // Done;;
	}
    }
#endif
  SGMLNORM::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


XMLREC::XMLREC (PIDBOBJ DbParent, const STRING& Name) : XMLBASE (DbParent, Name)
{
  RecordSeperator  = Getoption("RecordSeperator");
  if (RecordSeperator.IsEmpty())
    RecordSeperator  = Getoption("Seperator");
  XMLPreface       = Getoption("Preface");
  XMLTail          = Getoption("Tail");
  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "True").GetBool());
}



const char *XMLREC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("XMLREC");
  if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XMLBASE::Description(List);
  return "\
XML-like record format. Uses element name to slice-and-dice records: <tag>..</tag>\n\
For XML retrieval it gets the specified preface and tail added to be XML compliant.\n\
  Options:\n\
  [XML]\n\
  RecordSeperator=<tag>\n\
  Preface=<XML preface fragment including declaration>\n\
  Tail=<XML tail fragment>\n";
}

XMLREC::~XMLREC() { }



void XMLREC::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet == FULLTEXT_MAGIC) {
    // MIME 
    STRING tmp;
    if (RecordSyntax == HtmlRecordSyntax) {
	SGMLNORM::SourceMIMEContent(ResultRecord, &tmp);
	*StringBuffer << "Content-type: " << tmp << "\n\n";
    }
    XMLBASE::DocPresent(ResultRecord, ElementSet, RawRecordSyntax, &tmp);

    StringBuffer->Cat(XMLPreface);
    StringBuffer->Cat(tmp);
    StringBuffer->Cat(XMLTail);
  } else
    XMLBASE::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



#if 1

void XMLREC::ParseRecords(const RECORD& FileRecord)
{
  // No seperator then let the parent parse...
  if (RecordSeperator.IsEmpty()) {
    XMLBASE::ParseRecords(FileRecord);
    return;
  }

  size_t count = 0;
  STRING fn (FileRecord.GetFullFileName ());
  RECORD Record (FileRecord); // Easy way

  const off_t GlobalRecordStart = FileRecord.GetRecordStart();
  const off_t GlobalRecordEnd   = FileRecord.GetRecordEnd();

  MMAP mapping (fn, GlobalRecordStart, GlobalRecordEnd ? GlobalRecordEnd+1 : 0, MapSequential);
  if (!mapping.Ok())
    {
      message_log (LOG_ERRNO, "%s::ParseRecords: Could not map '%s' into memory", Doctype.c_str(), fn.c_str());
      SGMLNORM::ParseRecords(FileRecord);
      return;
    }
  const size_t len   = RecordSeperator.GetLength();
  PCHR RecBuffer     = (PCHR)mapping.Ptr();
  off_t ActualLength = mapping.Size();
  off_t length       = ActualLength - len; 

  // Search for <RecordSeperator
  enum {Scan = 0, Start, SeekEnd} State = Scan;
  off_t start;

  for (off_t i = 0; i < length; i++) {
     if (RecBuffer[i] == '<') {
        start = i;
	// Skip space to handle <    tag ..>
	while (++i < length && isspace(RecBuffer[i]))
		/* */;
	if (State != Scan && RecBuffer[i] == '/') {
	  // End-tag
	  State = SeekEnd;
          // Skip space to handle  </   tag..>
          while (++i < length && isspace(RecBuffer[i]))
	    /* */;
	} else if (RecBuffer[i] == '/')
	  continue;

	CHR ch = RecBuffer[i+len];
        if (ch != '>' && !isspace(ch))
	  continue;
	// Know at least the length matches...
        const char *t = RecordSeperator.c_str();
	if ( RecordSeperator.CaseMatches (RecBuffer+i)) {
	  // Found tag ....
	  switch (State) {
	     case SeekEnd:
		// Scan end of tag
		while (RecBuffer[i] != '>' && i < length ) i++;

		// If followed by space this belongs to the record
		if (isspace(RecBuffer[i+1])) {
		  do { i++;} while ( isspace(RecBuffer[i]) );
		  i--;
		}

	        Record.SetRecordEnd ( GlobalRecordStart + i ) ;
	        Db->DocTypeAddRecord (Record);

#if 0
if (1) {char tmp[12001];
bzero(tmp, 12000);
size_t x = Record.GetRecordStart() - GlobalRecordStart;
size_t y = Record.GetRecordEnd() - GlobalRecordStart;
cerr << "RECORD has " << y-x << endl;
memcpy(tmp, &RecBuffer[x], y -x  + 1 );
cerr << "LOOKING AT: "<< endl << tmp << endl <<  "##### " << endl;
}
#endif

		count++;
	        State = Scan;
	        break;
	     case Start:
		// A nasty case <tag> ... </><tag> but we also accpet
		// <tag> ... <tag>  
		Record.SetRecordEnd ( GlobalRecordStart + start - 1 );
		Db->DocTypeAddRecord (Record);
		count++;
		State == Scan;
		message_log(LOG_WARN, "<%s> before </%s>, assume </><%s>:  (%ld)", t, t, t,  GlobalRecordStart + start); 
		break;
	     case Scan:
#if 0
	     	// edz: 2021 Kludge test
		if (count == 0 && start > 2) {
		  RECORD NewRecord (Record);
		  NewRecord.SetDocumentType ( "0" );
		  NewRecord.SetRecordStart( GlobalRecordStart );
		  NewRecord.SetRecordEnd( GlobalRecordStart + start - 1 );
		  Db->DocTypeAddRecord (NewRecord);
		  cerr << "Adding " << GlobalRecordStart << " -- " << (GlobalRecordStart + start - 1)  << endl;
		}
#endif
		//
		//
//cerr << "XMLREC start = " << start << endl;
		if (count == 0 && start > 0 ) {
		   // Create a header element
		   //
		   Record.SetRecordStart( GlobalRecordStart);
		   Record.SetRecordEnd( GlobalRecordStart + start - 1);
		   Db->DocTypeAddRecord (Record);
		}

		//

		Record.SetRecordStart( GlobalRecordStart + start );
		// Scan to end of tag >
		while (RecBuffer[++i] != '>' && i < length) /* loop */;
		if (RecBuffer[i-1] == '/') {
		  // <element /> construct
		  if (isspace(RecBuffer[i+1])) {
		    do { i++;} while ( isspace(RecBuffer[i]) );
                    i--;
		  }
		  Record.SetRecordEnd( GlobalRecordStart + i);
		  Db->DocTypeAddRecord (Record);
//cerr << "Construct <../>" << endl;
		  count++;
		}  else
		  State = Start;
		break;
	  } 
      } else if (State == SeekEnd)
	 State = Start; // False Alarm
      // Need to find the next instance.. or </RecordSeperator 
     // i += len - 1;
     }
  } // loop 


  // Add trailing bits
  if (count && start < ActualLength ) {
	Record.SetRecordEnd( GlobalRecordEnd );
	Db->DocTypeAddRecord (Record);
  }



  if (State == Start)
    message_log (LOG_WARN, "Missing </%s>. Bytes %ld-%ld skipped in \"%s\".",
	RecordSeperator.c_str(), Record.GetRecordStart(), GlobalRecordEnd,
	fn.c_str());
  if (count == 0) {
    message_log(LOG_WARN, "No <%s> elements found. Fall-back to parent doctype.",
	RecordSeperator.c_str());
    XMLBASE::ParseRecords(FileRecord);
  }
  message_log (LOG_INFO, "Added %ld sub-records", count);
}


#else

void XMLREC::ParseRecords(const RECORD& FileRecord)
{
  // No seperator then let the parent parse...
  if (RecordSeperator.IsEmpty()) {
    XMLBASE::ParseRecords(FileRecord);
    return;
  }

  size_t count = 0;
  STRING fn (FileRecord.GetFullFileName ());
  RECORD Record (FileRecord); // Easy way

  const off_t GlobalRecordStart = FileRecord.GetRecordStart();
  const off_t GlobalRecordEnd   = FileRecord.GetRecordEnd();

  MMAP mapping (fn, GlobalRecordStart, GlobalRecordEnd ? GlobalRecordEnd+1 : 0, MapSequential);
  if (!mapping.Ok())
    {
      message_log (LOG_ERRNO, "%s::ParseRecords: Could not map '%s' into memory", Doctype.c_str(), fn.c_str());
      SGMLNORM::ParseRecords(FileRecord);
      return;
    }
  const size_t len   = RecordSeperator.GetLength();
  PCHR RecBuffer     = (PCHR)mapping.Ptr();
  off_t ActualLength = mapping.Size();
  off_t length       = ActualLength - len; 

  // Search for <RecordSeperator
  off_t start = 0;
  off_t end   = ActualLength;
  const char *sep = RecordSeperator.c_str();

  for (off_t i = 0; i < length; i++) {
     if (RecBuffer[i] == '<') {
	off_t mark = i;
	// Skip potential space
	do i++; while ( isspace(RecBuffer[i])) ;
	CHR ch = RecBuffer[i+len];

	if ((ch == '>' || ch == '/' || isspace(ch)) && memcmp( &RecBuffer[i] , sep, len) == 0)
	{
          // Found first instance of tag
	  start = mark;
	  break;
	}
     }
  }
  if (start != 0) {
    // We found a start.. now search for the end
    for (off_t i= length; i > start; i--)
    {
      if (RecBuffer[i] == '<') {
	// skip potential space
	off_t j = i + 1;
	if (RecBuffer[j] == '/') j++;
	while ( isspace(RecBuffer[j])) j++;
	if (RecBuffer[j] == '/') j++;
	CHR ch =  RecBuffer[j+len];

	if ((ch == '>' || ch == '/' || isspace(ch)) && memcmp( &RecBuffer[j] , sep, len) == 0) {
	  // match.. So need to seek to > to finish off
	  for (;j < ActualLength; j++) {
	    if ( RecBuffer[j] == '>') { // Done
	       end = j;
	       i = start;
	       break;
	    }
	  }

        }
      }
    }
  } 
  Record.SetRecordStart(start);
  Record.SetRecordEnd(end);
  XMLBASE::ParseRecords(Record);
}

#endif



// `rdf:Description

RDFREC::RDFREC (PIDBOBJ DbParent, const STRING& Name) : XMLREC (DbParent, Name)
{
  RecordSeperator  = Getoption("RecordSeperator");
  if (RecordSeperator.IsEmpty())
    RecordSeperator  = Getoption("Seperator");
  if (RecordSeperator.IsEmpty())
	  RecordSeperator = "rdf:Description";
  XMLPreface       = Getoption("Preface");
  if (XMLPreface.IsEmpty())
    XMLPreface = "\
<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" \
xmlns:s=\"http://description.org/schema/\">\n";
  XMLTail          = Getoption("Tail");
  if (XMLTail.IsEmpty())
    XMLTail = "\n</rdf:RDF>\n";
  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "True").GetBool());
}


const char *RDFREC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("RDF");
  if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XMLREC::Description(List);
  return "\
RDF-like record format. Uses element name to slice-and-dice records by default\n\
<rdf:Description>.\n";
}

RDFREC::~RDFREC () {; }



