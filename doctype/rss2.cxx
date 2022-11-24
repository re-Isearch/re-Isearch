
/*

language := Language field

Date fields:


pubDate        -> made this to date created
lastBuildDate  --> Map this to date modified

Let date be the date of the file (good for TTL)


Special Field:

ttl := Minutes to live.. Date of record + ttl = Expires

Numbers:

skipHours
skipDays
*/

#include <stdlib.h>
#include <ctype.h>
#include "common.hxx"
#include "gilsxml.hxx"
#include "doc_conf.hxx"

static const STRING  ttl      ("ttl"); // TTL Field Name
static const STRING  cdate_p  ("RSS\\CHANNEL\\PUBDATE") ; 
static const STRING  cdate    ("pubDate");
static const STRING  mdate    ("lastBuildDate");
static const STRING  eTag     ("eTag");
static const STRING  lang     ("language");


STRING RSS2::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  static struct {
    const char *tag;
    size_t      len;
  } bad_tags[] = {
    {"b", 1},
    {"big", 3},
    {"blockquote", 10},
    {"center", 6},
    {"div", 3}, // 2022
    {"em", 2},
    {"font", 4},
    {"h", 1}, // If it starts with h we won't want it..
    {"it", 2},
    {"java", 4},
    {"p", 1}, // 2022
    {"small", 5},
    {"strong", 6},
    {"tt", 2},
    {"u", 1}
  } ;
  bool have_namespace = false;
  size_t len = Tag.GetLength();
  const char *tag = Tag.c_str();
  for (const char *tp = tag; *tp; tp++)
    {
      if (*tp == levelCh)
	break;
      if (*tp == ':')
	{
	  have_namespace = true;
	  // Namespace
	  int nlen = tp-tag;
	  switch (nlen) {
	    case 4:
	      if (strncasecmp(tag, "html", 4) == 0)
		{
		  tag = ++tp;
		  len -= nlen+1;
		}
	      break;
	    case 5:
	      if (strncasecmp(tag, "xhtml", 5) == 0)
		{
		  tag = ++tp;
		  len -= nlen+1;
		}
	      break;
	  }
	}
    }
  
  for (size_t i=0; i < sizeof(bad_tags)/sizeof(bad_tags[0]); i++)
    {
      if (len == bad_tags[i].len && memcmp(bad_tags[i].tag, tag, len) == 0)
	{
//cerr << "Don't want " << tag << endl;
	  if (Value) Value->Clear();
	  return NulString;
	}
    }

  return DOCTYPE::UnifiedName(Tag, Value);

}


RSS2::RSS2 (PIDBOBJ DbParent, const STRING& Name) : XMLBASE (DbParent, Name)
{
  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "False").GetBool());

  if (TTLfieldName.IsEmpty())      TTLfieldName      = ttl;
  if (DateCreatedField.IsEmpty())  DateCreatedField  = cdate_p;
  if (DateModifiedField.IsEmpty()) DateModifiedField = mdate;
  if (LanguageField.IsEmpty())     LanguageField     = lang;
  if (KeyField.IsEmpty())          KeyField          = eTag;
  if (DateField.IsEmpty())         DateField         = cdate_p;

}

void RSS2::LoadFieldTable()
{
  if (Db)
    {
      Db->AddFieldType(DateField, FIELDTYPE::date);
      if (DateField != cdate) Db->AddFieldType(cdate, FIELDTYPE::date);
      if (DateField != mdate) Db->AddFieldType(mdate, FIELDTYPE::date);
      Db->AddFieldType("slash:comments", FIELDTYPE::numerical); // 2022
      Db->AddFieldType("skipHours", FIELDTYPE::numerical);
      Db->AddFieldType("skipHours", FIELDTYPE::numerical);
      Db->AddFieldType(TTLfieldName, FIELDTYPE::ttl);
      if (TTLfieldName != ttl) Db->AddFieldType(ttl, FIELDTYPE::ttl);
      Db->AddFieldType(UnifiedName(UnifiedName("enclosure", NULL)+"@length", NULL), FIELDTYPE::numerical);
      Db->AddFieldType("enclosure@length", FIELDTYPE::numerical);
    }
  DOCTYPE::LoadFieldTable();
}

void RSS2::Present (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      if (Db && Db->GetFieldData (ResultRecord, "[1]title", StringBuffer, this))
	return;
    }

  // Link is special.. Want RAW!
  int pos = ElementSet.SearchAny("link");
  char ch;
  if ((pos == 1 && ElementSet.GetLength() == 4)
	|| pos > 3 &&  (ch = ElementSet.GetChr(pos-1)) == '/' || ch == '\\')
    return DOCTYPE::Present(ResultRecord, ElementSet, StringBuffer);

  return XMLBASE::Present(ResultRecord, ElementSet, StringBuffer);
}


RSS2::~RSS2()
{
}

const char *RSS2::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("RSS2");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XMLBASE::Description(List);
  return "RSS, Really Simple Syndication 2.0.(XML) format handler.\n\
Note: default special extension is \"etag\" field for key.\n";
}


void RSS2::ParseRecords(const RECORD& FileRecord)
{
//  message_log (LOG_DEBUG, "%s: ParseRecords Start", Doctype.c_str());
  XMLBASE::ParseRecords(FileRecord);
//  message_log (LOG_DEBUG, "%s: ParseRecords Done", Doctype.c_str());
}

void RSS2::ParseFields(PRECORD pRecord)
{
//  message_log (LOG_DEBUG, "%s: ParseFields Start", Doctype.c_str());
  XMLBASE::ParseFields(pRecord);
//  message_log (LOG_DEBUG, "%s: ParseFields Done", Doctype.c_str());
}

bool RSS2::StoreTagComplexAttributes(const char *tag_ptr) const
{
  if (tag_ptr && *tag_ptr)
    {
      const char *tag = tag_ptr;
      struct {
	const char *tag;
	size_t      len;
      } rss_fields[] = {
//	{"cloud", 5},
	{"enclosure", 9}, 
	{"source", 6}
      };

      for (size_t i=0; i< sizeof(rss_fields)/sizeof(rss_fields[0]); i++)
	{
	  int diff = strncmp(rss_fields[i].tag, tag_ptr, rss_fields[i].len);
	  if (diff == 0)
	    return true;
	  if (diff > 0)
	    break;
	}
      if (strncasecmp(tag, "html:", 5) == 0)
	tag += 5; // HTML namespace
      // <IMG ..>
      if (strncasecmp(tag, "img", 3) == 0 && isspace(tag[3]))
	return true;
      // <A HREF= ..>
      if ((*tag == 'a' || *tag == 'A') && isspace(tag[1]))
	return true;
      return (XMLBASE::StoreTagComplexAttributes(tag_ptr)) ||
	(XMLBASE::StoreTagComplexAttributes(tag));
    }
  return false;
}



RSSCORE::RSSCORE (PIDBOBJ DbParent, const STRING& Name) : RSS2 (DbParent, Name)
{
#if 1
  baseLinkField  = Getoption("BASEPATH", "RSS\\CHANEL\\LINK");
  linkField      = Getoption("LINKPATH", "RSS\\CHANNEL\\ITEM\\LINK");
  titleField     = Getoption("TITLEPATH","RSS\\CHANNEL\\ITEM\\TITLE");
#else
  baseLinkField  = "RSS\\CHANEL\\LINK";
  linkField      = "RSS\\CHANNEL\\ITEM\\LINK";
  titleField     = "RSS\\CHANNEL\\ITEM\\TITLE";
#endif
}

RSSCOREARCHIVE::RSSCOREARCHIVE (PIDBOBJ DbParent, const STRING& Name) : RSSCORE (DbParent, Name)
{
  KeyField = "RSS\\CHANNEL\\ID";
}

bool RSSCORE::URL(const RESULT& ResultRecord, PSTRING StringBuffer,  
        bool OnlyRemote) const
{
  STRING url;
  DOCTYPE::Present(ResultRecord, linkField, RawRecordSyntax, &url);
  if (!url.IsEmpty())
    {
      url.XMLCommentStrip();
//cerr << "XXXXXX " << linkField << "  URL=\"" << url << "\"" << endl;
      // **start Added Fed 2010 edz to support link as base 
      if (url.Search(":/") == 0)
	{
	  STRING base;
	  DOCTYPE::Present(ResultRecord, baseLinkField, RawRecordSyntax, &base);
	  base.XMLCommentStrip();
	  if (base.Search(":/"))
	    {
	      if (!IsPathSep(url.GetChr(1)))
		AddTrailingSlash(base);
	      url.Prepend(base);
	    }
	}
     // **end Added 2010
      if (url.Search("://"))
	{
	  if (StringBuffer) *StringBuffer = url;
	  return true;
	}
    }
  // Let it look at the saved structure (spider?)
  return DOCTYPE::URL(ResultRecord, StringBuffer, OnlyRemote);
}


void RSSCORE::Present (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  STRING buf;
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      RSS2::Present(ResultRecord, titleField, RecordSyntax, &buf);
      if (!buf.IsEmpty())
	{
	  if (StringBuffer) *StringBuffer = buf;
	  return;
	}
    }
  RSS2::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


RSSCORE::~RSSCORE() {};

void RSSCORE::LoadFieldTable()
{
  RSS2::LoadFieldTable();

}


const char *RSSCORE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("RSSCORE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  RSS2::Description(List);
  return "CORE RSS XML format handler for NONMONOTONIC labs' pre-digested feeds.\n\
  Search time Options:\n\
   BASEPATH = Path to the base URL    (default: RSS\\CHANNEL\\LINK)\n\
   LINKPATH = Path to item's link URL (default: RSS\\CHANNEL\\ITEM\\LINK)\n\
   TITLEPATH= Path to an item's title (default: RSS\\CHANNEL\\ITEM\\TITLE)";
}


const char *RSSCOREARCHIVE::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);

  RSSCORE::Description(List);
  return "CORE RSS XML format handler for NONMONOTONIC labs' pre-digested feeds (Archive).\n\
- KeyField is by default \"RSS\\CHANNEL\\ID\" (and not eTag).";
}



CAP_RSS::CAP_RSS (PIDBOBJ DbParent, const STRING& Name): RSS2(DbParent,Name)
{
}

STRING CAP_RSS::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  // CAP:X
  if ((Tag.GetLength() > 4) && Tag[3] == ':' && Tag[2] == 'P' && Tag[1] == 'A' && Tag[0] == 'C')
   return RSS2::UnifiedName (Tag.c_str()+4, Value);
  return RSS2::UnifiedName (Tag, Value); 
}


void CAP_RSS::LoadFieldTable()
{
}

CAP_RSS::~CAP_RSS()
{
}

const char *CAP_RSS::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);
  RSS2::Description(List);
  return "Common Alerting Protocol, v1.1\n\
Supports a variant of the OASIS Standard CAP-V1.1, October 2005\n\n";
}




IETF_ATOM::IETF_ATOM (PIDBOBJ DbParent, const STRING& Name): XMLBASE(DbParent,Name)
{
  SGMLNORM::SetStoreComplexAttributes (Getoption("Complex", "False").GetBool());

  if (DateCreatedField.IsEmpty())  DateCreatedField  = "created";
  if (DateModifiedField.IsEmpty()) DateModifiedField = "modified";
  if (LanguageField.IsEmpty())     LanguageField     = "xml:lang";

  // Id identifies the feed using a universally unique and permanent URI.
  // If you have a long-term, renewable lease on your Internet domain name,
  // then you can feel free to use your website's address
  if (KeyField.IsEmpty())          KeyField          = "feed\\id";
  if (DateField.IsEmpty())         DateField         = "issued";

}

STRING IETF_ATOM::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  // atom:X
  if ((Tag.GetLength() > 5) && Tag[4] == ':' &&
	tolower(Tag[3]) == 'm' && tolower(Tag[2]) == 'o' && tolower(Tag[1]) == 't' && tolower(Tag[0]) == 'a')
   return UnifiedName (Tag.c_str()+5, Value);

#if 0
   // skip tag:
   if ((Tag.GetLength() > 4) && 
        tolower(Tag[3]) == ':' && tolower(Tag[2]) == 'g' && tolower(Tag[1]) == 'a' && tolower(Tag[0]) == 't')
   return UnifiedName (Tag.c_str()+5, Value);
#endif


  if (Tag.GetLength() == 1 || (Tag ^= "div") || (Tag ^= "span") || (Tag ^= "em"))
    return NulString; // bad
  return XMLBASE::UnifiedName (Tag, Value);
}
 
  
void IETF_ATOM::LoadFieldTable()
{
}

IETF_ATOM::~IETF_ATOM()
{
}

const char *IETF_ATOM::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);
  XMLBASE::Description(List);
  return "IETF Atom\n\
Supports various flavours of the Atom 1.0 Syndication Format.\n\n";
}


