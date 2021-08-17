#pragma ident  "@(#)html.cxx       1.69 04/20/01 14:23:52 BSN"
/* ########################################################################

               HTML Document Handler (DOCTYPE)

   File: html.cxx
   Version: 1.69 04/20/01 14:23:52
   Description: Class HTMLTAG - WWW HTML Document Type
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@nonmonotonic.net
   Modified: Tue Jun  3 11:10:23 MET 1997
   Last maintained by: Edward C. Zimmermann

   ########################################################################

   Note: Requires that SGMLNORM is configured

   ########################################################################

   Copyright (c) 1995-1997 : Edward C. Zimmermann. All Rights Reserved.
   Copyright (c) 1995-1997 : Basis Systeme netzwerk. All Rights Reserved.

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.
  tel: +49 (89) 692 8120
  fax: +49 (89) 692 8150

   ######################################################################## */
//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <string.h>
//#include <errno.h>
#include "common.hxx"
#include "html.hxx"
#include "doc_conf.hxx"

#define ATTRIB_SEP '@' /* Must match value in SGMLNORM */

#if BSN_EXTENSIONS
# include <sys/stat.h>
#endif

// <BASE HREF="xxx">
static const char baseHref[] = {'B','A','S','E', ATTRIB_SEP, 'H','R','E','F', 0};
// <BASE TARGET="xxx">
static const char baseTarget[] = {'B','A','S','E', ATTRIB_SEP, 'T','A','R','G', 'E', 'T', 0};


/* ------- HTML Support --------------------------------------------- */
// Look for WWW_ROOT (BSn standard), HTTP_PATH (CNIDR standard) or HTDOCS
HTML::HTML (PIDBOBJ DbParent, const STRING& Name) :
	SGMLNORM (DbParent, Name)
{
  // Read doctype options
  const char *env = getenv("HTML_LEVEL");
  const STRING level (env ? (STRING)env :  Getoption("HTML_LEVEL"));
  if (level.GetLength ())
    {
      // Just look at first letter
      switch (level.GetChr(1))
	{
	  case 'B': case 'b': // Basic
	    HTML_Level = 5;
	    break;
	  case 'V': case 'v': // Valid
	    HTML_Level = 4;
	    break;
	  case 'S': case 's': // STRICT
	    HTML_Level = 3;
	    break;
	  case 'N': case 'n': // NORMAL
	    HTML_Level = 2;
	    break;
	  case 'M': case 'm': // MINIMAL
	    HTML_Level = 1;
	    break;
	  case 'F': case 'f': // FULL
	    HTML_Level = 0;
	  case 'A': case 'a': // ANITIHTML
	    HTML_Level = -1;
	    break; 
	  default:
	    HTML_Level = level.GetInt();
	    break;
	}
    }
  else
    HTML_Level = DEFAULT_HTML_LEVEL; // Set the defined level
  if (DateField.IsEmpty())
    DateField = "Last-Modified";
  if (KeyField.IsEmpty())
    KeyField = "ETag";
}

const char *HTML::Description(PSTRLIST List) const
{
  List->AddEntry ("HTML");
  SGMLNORM::Description(List);
  return "\
HyperText Markup Language (W3C HTML)\n\
  Index time Options:\n\
    HTML_LEVEL arg // Takes Full (0), Minimal (1), Normal (2), Strict (3), Valid (4), Basic (5),\n\
                   // Antihtml (-1) and head fields only (-2) which determines the behaviour of the parser\n\
    This can also be specified in the evironment as HTML_LEVEL or in the database ini\n\
    as HTML_LEVEL=arg in the [HTML] section or HTML_LEVEL=arg in the doctype.ini [General] section.\n\
    An arg of 'antihtml' or -1 passes the HTML to the HTML-- document class.\n\
    DateField is also processed at index time and if not defined it defaults to: Last-Modified\n\
    (e.g. <META HTTP-EQUIV=\"Last-Modified\" CONTENT=\"date string\">)\n\
  Search time Options:\n\
    HTTP_PATH path // Specified the base of the HTML tree (root)\n\
    HTDOCS path    // same as above (obsolete)\n\
    This can also be specified in the environment as HTTP_PATH or HTDOCS or in the\n\
    database ini as Pages=path in the [HTTP] section.";
}

void HTML::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for HTML Documents
  StringPtr->AssignCopy(9, "text/html");
}

void HTML::SourceMIMEContent(const RESULT&, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void HTML::ParseRecords (const RECORD& FileRecord)
{
  message_log (LOG_DEBUG, "%s(HTML):ParseRecord Level %d of \"%s\"",
	Doctype.c_str(), HTML_Level, FileRecord.GetFullFileName().c_str() );
  if (HTML_Level < 0)
    {
      // Make sure we are not getting called from HTML--
      if (! Doctype.CaseEquals("HTML--"))
	{
	  // Level == -1 means use ANTIHTML
	  RECORD Record;
	  Record.SetPath( FileRecord.GetPath() );
	  Record.SetFileName( FileRecord.GetFileName() );
	  Record.SetRecordStart ( FileRecord.GetRecordStart() );
	  Record.SetRecordEnd ( FileRecord.GetRecordEnd() );
	  Record.SetDocumentType ( HTML_Level == -1 ? "HTML--" : "HTMLHEAD");
	  Db->ParseRecords (Record);
	  return;
	}
    }
  SGMLNORM::ParseRecords (FileRecord);
}

void HTML::BeforeIndexing()
{
  SGMLNORM::BeforeIndexing();
}

void HTML::AfterIndexing()
{
  tmpBuffer.Free();
  tmpBuffer2.Free();
  SGMLNORM::AfterIndexing();
}


void HTML::AddFieldDefs()
{
  SGMLNORM::AddFieldDefs();
}


STRING HTML::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  if (HTML_Level == 5 && Tag.GetLength())
    {
      const char *tag = (const char *)Tag;
      // Alternative (Robot) view
      if ((*tag == 'H' || *tag == 'h') && isdigit(tag[1]))
	{
	  return SGMLNORM::UnifiedName ("Heading", Value);
	}
      if (StrNCaseCmp(tag, "ul", 2) == 0 ||
	  StrNCaseCmp(tag, "ol", 2) == 0 ||
	  StrNCaseCmp(tag, "menu", 4) == 0 ||
	  StrNCaseCmp(tag, "dir", 3) == 0) 
	{
	  return SGMLNORM::UnifiedName ("List", Value);
	}
      if (StrNCaseCmp(tag, "META", 4) == 0)
	{
	  if (tag[4] == '.')
	    {
	      return SGMLNORM::UnifiedName (tag + 5, Value);
	    }
	  else
	    {
	      return *Value = NulString;
	    }
	}
      if (*tag == 'A' || *tag == 'a')
	{
	  if (*tag == '\0')
	    return SGMLNORM::UnifiedName ("Link", Value);
	  else if (StrCaseCmp(tag, "A@HREF") == 0)
	    return SGMLNORM::UnifiedName ("Target", Value);
	  else
	    return *Value = NulString; // Not interested in NAME etc..
	}
    }
  return SGMLNORM::UnifiedName(Tag, Value);
}

typedef struct {
    const char *tag;
    unsigned char len;
} TagList_t;

// Lowercase sort (since the tags are listed in lowercase
static int _taglistcomp (const void *v1, const void *v2)
{
  return strcmp (((TagList_t *) v1)->tag, ((TagList_t *) v2)->tag);
}

// Search for the tag 'str' in the list 'tab'
// returns the length of the tag in 'tab', 0 if no match
static int tag_search (const char *str, TagList_t *tab, int n)
{
  int i, h, l, r;

  while (isspace(*str)) str++;
  if (*str == '/') str++;
  while (isspace(*str)) str++;

  l = 0;
  h = n - 1;
  for (;;)
    {
      i = (h + l) / 2;
      r = StrNCaseCmp (str, tab[i].tag, tab[i].len);
      if (r < 0)
	h = i - 1;
      else if (r == 0 && (isspace(str[tab[i].len]) || str[tab[i].len] == '\0'))
	{
	  return tab[i].len;
	}
      else
	l = i + 1;
      if (h < l) return 0;
    }
  // NOT REACHED
}


// Kludge to identity Attribute tags
static inline int IsHTMLAttributeTag (const char *tag)
{
  // HTML Attributes where we are also interested in values
  static TagList_t Taglist[] = {
/*- UNSORTED LIST (lowercase names) -*/ 
    { "html", 4 },
    // Head elements
    { "base", 4 },
    { "meta", 4 },
    { "link", 4 },
    // General HTML Attribute
    { "a", 1 },
    // HTML 3.x elements
    { "area", 4 },
    { "style", 5 },
    { "div", 3 },
    { "fig", 3 },
    { "note", 4 },
    // Misc -- Not Really interesting except to browers
//  { "img", 3 },
//  { "nextid", 6 },
//  { "font", 4 },
    // Odd stuff
    { "embed", 5 }
  };
  static GDT_BOOLEAN sort = GDT_TRUE;
  if (sort)
    {
      qsort ((void *)Taglist, (sizeof (Taglist) / sizeof (TagList_t)),
           sizeof (TagList_t), _taglistcomp);
      sort = GDT_FALSE;
    }

  return tag_search(tag, Taglist, sizeof(Taglist)/sizeof(TagList_t));
}

// Real Basic stuff..
static int IsBasicHTMLFieldTag (const char *tag)
{
  // A real minimal list for a robot!
  static TagList_t HtmlTagList[] = {
    // lowercase and sorted
    {"a", 1},
    {"body", 4},
    {"dir", 3},
    {"h1", 2},
    {"h2", 2},
    {"h3", 2},
    {"h4", 2},
    {"h5", 2},
    {"h6", 2},
    {"head", 4},
    {"link", 4},
    {"menu", 4},
    {"meta", 4},
    {"ol", 2},
    {"ul", 2}
  };

  // Is this part of a "meaningful" tag?
  return tag_search(tag, HtmlTagList, sizeof(HtmlTagList)/sizeof(TagList_t));
}

// We are really only interested in content (container) tags
static int IsHTMLFieldTag (const char *tag)
{
  // TODO: Better Support Tables
  static TagList_t HtmlTagList[] = {
/*-  UNSORTED LIST (lowercase names) -*/
    {"html", 4},
    /* Top level */
    {"head", 4},
    {"body", 4},
    /* header tags */
    {"title", 5},
    {"style", 5},
    {"script", 6}, // 3.2 way to have scripts
    {"layer",  5},
    {"ilayer", 6},
    /* body tags */
    // HTML Special Attribute
    {"a", 1}, {"area", 4},
    // Headers-- chapter, section, subsection,...
    {"h1", 2}, {"h2", 2}, {"h3", 2}, {"h4", 2}, {"h5", 2}, {"h6",2},
    // Document divisions
    {"div", 3},
    // Flow
//  {"p", 1},  /* Ignore the paragraph tag (few use </p>) */
//  {"br", 2}, /* Ignore the line break tag (only <br></br> is allowed) */
//  {"hr", 2}, /* Ignore horizontal rules */
    // Semantic format elements
    {"em", 2}, {"strong", 6},
    {"code", 4}, {"samp", 4},
    {"kbd", 3}, {"var", 3}, {"cite", 4},
    {"address", 7},
    {"xmp", 3}, // Obsolete
    // Misc format (HTML 3.0) elements
    {"q", 1}, {"lang", 4}, {"au", 2}, {"dfn", 3},
    {"person", 6}, {"acronym", 7}, {"abbrev", 6},
    {"abstract", 8},
    // Tables and Arrays
    {"array", 5}, {"row", 3}, {"item", 4},
    {"caption", 7},
    {"table", 5}, {"th", 2}, {"td", 2}, {"tr", 2},
    // Cougar tables
    {"thead", 5},
    {"tfoot", 5},
    {"tbody", 5},
    {"colgroup", 8},
//  {"col", 3},
    // Misc
    {"ins", 3}, {"del", 3}, 
    // Obsolete format (HTML 0.9) elements
    {"listing", 7}, // Obsolete
    {"blockquote", 10}, // Obsolete
    {"bq", 2}, // Depreciated Blockquote
    /* Descriptive markup (should we ignore these?) 
       --- big violation of content model! */
    // fonts (Forget them since they are descriptive!)
//  {"big", 3}, {"small", 5},
//  {"b", 1}, {"tt", 2}, {"i", 1}, {"u", 1}, {"s", 1},
//  {"strike", 6},
//  {"font", 4},
    // Misc 
    {"pre", 3},
//  {"center", 6}, // Netscape Nonsense
    /* List environments */
    {"lh", 2}, {"dl", 2}, {"ul", 2}, {"ol",2}, {"tli", 3}, {"ruby", 4},
    {"menu", 4}, {"dir", 3},
    /* List items */
    {"li", 2}, {"tl", 2}, {"dt", 2}, {"dd", 2}, {"rb", 2}, {"rt", 2},
    /* Misc */
    {"form", 4}, {"message", 7}, {"note", 4},
//  {"select", 6}, {"option", 6}, {"input", 5}, {"textarea", 8},
    /* Bogus HTML "extension" by popular demand */
    {"comment", 7} // <comment><!-- searchable words --></comment>
  };
  static GDT_BOOLEAN sort = GDT_TRUE;
  if (sort)
    {
      qsort (
           (void *)HtmlTagList, (sizeof (HtmlTagList) / sizeof (TagList_t)),
           sizeof (TagList_t), _taglistcomp);
      sort = GDT_FALSE;
    }

  // Is this part of a "meaningful" tag?
  if (tag_search(tag, HtmlTagList, sizeof(HtmlTagList)/sizeof(TagList_t)))
    return GDT_TRUE;
  // Is it a tag where we just want the attribute value?
  return IsHTMLAttributeTag (tag);
}

// Some tags in HTML will don't want to clutter
// our index with.....
static int IgnoreHTMLTag (const char *tag)
{
  static TagList_t IgnoreList[] = {
    // UNSORTED.. NO NULL TERMINATION!
    {"plaintext", 9}, // More than just obsolete!
    {"p", 1},  /* Ignore the paragraph tag (few use </p>) */
    {"br", 2}, /* Ignore the line break tag (only <br></br> is allowed) */
    {"hr", 2}, /* Ignore horizontal rules */
    // Math
    {"sub", 3}, {"sup", 3},
    /* Descriptive markup--- big violation of content model! */
    // fonts (Forget them since they are descriptive!)
    {"big", 3}, {"small", 5},
    {"b", 1}, {"tt", 2}, {"i", 1}, {"u", 1}, {"s", 1},
    {"center", 6}, // Netscape Nonsense
    // Misc Attribute Tags -- Not Really interesting except to browers
    { "tab", 3},
    { "img", 3 },
    { "nextid", 6 },
    { "font", 4 },
    { "overlay", 7}
  };
  static GDT_BOOLEAN sort = GDT_TRUE;

  if (sort)
    {
      qsort (
           (void *)IgnoreList, (sizeof (IgnoreList) / sizeof (TagList_t)),
           sizeof (TagList_t), _taglistcomp);
      sort = GDT_FALSE;
    }

  // Is this part of a tag that we don't want?
  return tag_search(tag, IgnoreList, sizeof(IgnoreList)/sizeof(TagList_t));
}

// Search for the next occurance of an element of tags in tag_list
static const char *find_next_tag (const char *const *tag_list, const char *const *tags)
{
  if (tag_list[0])
    {
      // Start with the NEXT tag
      for (size_t i=1; tag_list[i]; i++)
	{
	  // Walk through the list..
	  for (size_t j = 0; tags[j]; j++)
	    {
	      if (0 == StrCaseCmp (tag_list[i], tags[j]))
		return tag_list[i]; // FOUND IT
	    }
	}
    }
  return NULL; // NOTFOUND
}


// Parse more-or-less valid HTML (as well as a few typical
// invalid but common constructs).
// Kludgy but common uses of HTML are kludgy as well
// --- and the job here is to parse and not to validate!
//
//
// To do:
// <!-- <META>                Generic Metainformation         -->
// <!-- <META HTTP-EQUIV=...> HTTP response header name       -->
// <!-- <META NAME=...>       Metainformation name            -->
// <!-- <META CONTENT="...">  Associated information          -->
//
// Map <META NAME="FOO" CONTENT="BAR"> to
//      @NAME with content "BAR"
// -- we current map META@NAME="FOO", META@CONTENT="BAR"
//
void HTML::ParseFields (PRECORD NewRecord)
{
  // Open the file
  const STRING fn = NewRecord->GetFullFileName ();
  PFILE fp = ffopen (fn, "rb");

  if (fp == NULL)
    {
      message_log (LOG_ERRNO, "Couldn't access '%s'", fn.c_str());
      NewRecord->SetBadRecord();
      return;                   // ERROR
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp);

  SRCH_DATE Datum;
  STRING    Key;

  Datum.SetTimeOfFile(fp);
  if (RecEnd <= RecStart)
    {
      message_log (LOG_WARN, "%s: zero-length record - '%s' [%u-%u]...",
	Doctype.c_str(), fn.c_str(), (unsigned)RecStart, (unsigned)RecEnd );
      message_log (LOG_NOTICE, "%s skipping record", Doctype.c_str());
      ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  if (RecStart && -1 == fseek (fp, RecStart, SEEK_SET))
    {
      message_log (LOG_ERRNO, "%s::ParseRecords(): Seek failed - '%s', skipping.", Doctype.c_str(), fn.c_str());
      ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  // Read the whole record in a buffer
  GPTYPE RecLength = RecEnd - RecStart + 1 ;
  PCHR RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1, sizeof(CHR));
  size_t ActualLength = (size_t) fread (RecBuffer, sizeof(CHR), RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  ffclose (fp);

  STRING FieldName;
  FC fc;
  DF df;
  DFD dfd;

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL)
    {
      message_log (LOG_WARN, "Unable to parse `%s' tags in file '%s'", Doctype.c_str(), fn.c_str() );
      NewRecord->SetBadRecord();
      return; // ERROR
    }

  PDFT pdft = new DFT ();
  STRING DTD;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      if ((*tags_ptr)[0] == '/')
	continue;
      if ((*tags_ptr)[0] == '!')
	{
	  if (strncmp(*tags_ptr, "!DOCTYPE", 8) == 0)
	    {
	      PCHR p = *tags_ptr;
	      while (*p && !isspace(*p)) p++;
	      ExtractDTD(p+1, &DTD);
	    }
	}

      switch (HTML_Level)
	{
	  // Accept only those tags we "know about"
	  case 5:
	    if (IsBasicHTMLFieldTag(*tags_ptr) == 0) continue;
	    break;
	  default:
	  case 3:
	  case 2:
	    if (IsHTMLFieldTag(*tags_ptr) == 0) continue;
	    break;
	  case -1: // This case should not happen!
	  case 1:
	    // Accept almost anything not in our list to ignore
	    if (IgnoreHTMLTag(*tags_ptr)) continue;
	    break;
	  case 0:
	    // Ugh..
	    if ((*tags_ptr)[0] == '!') continue;
	    break;
	}

      const char *p = find_end_tag ((const char *const *)tags_ptr, (const char *)*tags_ptr);

      // Kludge to support some missing close tags,
      // some min tags and most "common"
      // incorrect uses of DD, DT, LI, TL and TR
      if (p == NULL)
	{
	  GDT_BOOLEAN complain = GDT_TRUE;
	  if (StrCaseCmp  (*tags_ptr, "html") == 0)
	    {
eof:
	      complain = GDT_FALSE;
	      // For <html>, <body> and <pre> its end-of-file
	      p = RecBuffer + ActualLength; 
#if BSN_EXTENSIONS
	      message_log (LOG_INFO, "\
\"%s\"(%u): Bogus use of <%s> using EOF as implied end.",
		(const char *)fn,
		(unsigned) (*tags_ptr - RecBuffer),
		*tags_ptr);
#endif
	    }
	  else if (StrNCaseCmp (*tags_ptr, "body", 4) == 0 ||
		   StrNCaseCmp (*tags_ptr, "pre", 3) == 0  ||
		   StrNCaseCmp (*tags_ptr, "listing", 7) == 0 ||
		   StrNCaseCmp (*tags_ptr, "table", 5) == 0 )
	    {
	      const char *tags[] = {"/body", "/html", NULL};
              if ((p = find_next_tag (tags_ptr, tags)) == NULL)
		goto eof;
	    }
	  else if (StrCaseCmp (*tags_ptr, "title") == 0)
	    {
	      // By title use next tag...
	      p = tags_ptr[1];
	    }
	  else if (StrCaseCmp (*tags_ptr, "head") == 0)
	    {
	      for (PCHR *tp = &tags_ptr[1]; *tp; tp++)
		{
		  // "SCRIPT|STYLE|META|LINK" 
		  // Look for something..
		  if ((*tp)[0] == '/')
		    continue; // Look for start tags
		  if (!(StrNCaseCmp(*tp, "meta", 4) == 0 ||
		        StrNCaseCmp(*tp, "styl", 4) == 0 ||
		        StrNCaseCmp(*tp, "scri", 4) == 0 ||
		        StrNCaseCmp(*tp, "link", 4) == 0 ||
		        StrNCaseCmp(*tp, "titl", 4) == 0 ||
		        StrNCaseCmp(*tp, "base", 4) == 0))
		    {
		      p = *tp;
		      break;
		    }
		}
	    }
	  else if (StrCaseCmp (*tags_ptr, "dd") == 0)
	    {
	      // Look for nearest <DT> or </DL>
	      const char *tags[] = {"dt", "/dl", NULL};
	      p = find_next_tag (tags_ptr, tags);
	      if (p == NULL)
		{
		  // Some bogus uses
		  const char *tags[] = {"dd", "/ul", "/ol", NULL};
		  p = find_next_tag (tags_ptr, tags);
		}
	      else
		complain = GDT_FALSE;
	    }
	  else if (StrCaseCmp (*tags_ptr, "dt") == 0)
	    {
	      // look for next <DD> or </DL>
	      const char *tags[] = {"dd", "/dl", NULL};
	      p = find_next_tag (tags_ptr, tags);
	      if (p == NULL)
		{
		  // Some bogus uses
		  const char *tags[] = {"dt", "/ul", "/ol", NULL};
		  p = find_next_tag (tags_ptr, tags);
		}
	      else
		complain = GDT_FALSE;
	    }
	  else if (StrCaseCmp (*tags_ptr, "li") == 0)
	    {
	      // look for next <LI>, </OL> or </UL>
	      const char *tags[] = {"li", "/ol", "/ul", NULL};
	      p = find_next_tag (tags_ptr, tags);
	      complain = GDT_FALSE;
	    }
	  else if (StrCaseCmp (*tags_ptr, "rb") == 0 || StrCaseCmp (*tags_ptr, "rt") == 0)
	    {
	      const char *tags[] = {"rb", "rt", "/ruby", NULL};
	      p = find_next_tag (tags_ptr, tags);
	      complain = GDT_FALSE;
	    }
	  else if (StrCaseCmp (*tags_ptr, "tl") == 0)
	    {
	      // look for nearest <TL> or </TLI>
	      const char *tags[] = {"tl", "/tli", NULL};
	      p = find_next_tag (tags_ptr, tags);
	      complain = GDT_FALSE;
	    }
	  else if (StrNCaseCmp (*tags_ptr, "td", 2) == 0 ||
		StrNCaseCmp (*tags_ptr, "th", 2) == 0 )
	    {
	      // look for nearest <td>, <th>, </tr>, </table>"
	      const char *tags[] = {"td", "th", "/tr", "tr", "/table", NULL};
	      p = find_next_tag (tags_ptr, tags);
	    }
	  else if (StrNCaseCmp (*tags_ptr, "tr", 2) == 0 ||
		   StrNCaseCmp (*tags_ptr, "caption", 7) == 0)
	    {
	      // look for nearest <tr>, </table>"
	      const char *tags[] = {"tr", "/table", NULL};
	      p = find_next_tag (tags_ptr, tags);
	    }
	  // THEAD, TFOOT and TBODY should be normalized but...
	  else if (StrNCaseCmp(*tags_ptr, "thead", 5) == 0 ||
		   StrNCaseCmp(*tags_ptr, "tfoot", 5) == 0 ||
		   StrNCaseCmp(*tags_ptr, "tbody", 5) == 0 )
	    {
	      // This is an incorrect use of these tags!
	      // look for nearest <thead>, <tfoot>, <tbody> or </table>
	      const char *tags[] = {"thead", "tfoot", "tbody", "/table", NULL};
	      p = find_next_tag (tags_ptr, tags);
	    }
	  else if (StrCaseCmp(*tags_ptr, "colgroup") == 0)
	   {
	     PCHR *end_tag = tags_ptr;
	     // COLGROUP contains COL and COL is EMPTY
	     // <!ELEMENT COL - O EMPTY>
	     do {
	      end_tag++;
	     } while (
		// Short hand <> </> style...
		(*end_tag)[0] == '\0' ||
		((*end_tag)[0] == '/' && (*end_tag)[1] == '\0') ||
	        // <col>
		StrNCaseCmp(*end_tag, "col", 3) == 0 ||
		// </col>
		StrNCaseCmp(*end_tag, "/col", 4) == 0 );
		// We loop till we find something else...
	     p = *end_tag;
	   }
#if BSN_EXTENSIONS
	if (p != NULL && complain)
	  {
	    // Give some information
	    message_log (LOG_INFO, "\
\"%s\"(%u): Bogus use of <%s> using <%s> as end tag.",
                (const char *)fn, (unsigned) (*tags_ptr - RecBuffer),
                *tags_ptr, p);
	  }
#endif
	}			// end code to handle some HTML minimized tags

      if (p != NULL && (p != *tags_ptr))
	{
	  // We have a tag pair
	  size_t tag_len = strlen (*tags_ptr);
	  size_t val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
	  int val_len = (p - *tags_ptr) - tag_len - 2;

	  // Skip leading white space
	  while (isspace (RecBuffer[val_start]))
	    val_start++, val_len--;
	  // Leave off trailing white space
	  while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	    val_len--;
	  // Don't bother storing empty fields
	  if (val_len > 0) {
	    // Now cut out the complex values to get tag name
	    CHR orig_char = 0;
	    char* tcp;
	    for (tcp = *tags_ptr; *tcp; tcp++)
	      if (isspace (*tcp))
		{
		  orig_char = *tcp;
		  *tcp = '\0';
		  break;
		}

	    if (SGMLNORM::UnifiedName (DTD, *tags_ptr, &FieldName).GetLength() == 0)
	      continue; // Don't bother with this field..
	    FieldName.ToUpper(); // store tags uppercase (not needed yet)
	    if (orig_char) *tcp = orig_char;

	    dfd.SetFieldName (FieldName);
	    Db->DfdtAddEntry (dfd);

//	    fc.SetFieldId ( Db->GetMainDfdt()->GetFileNumber(FieldName) );
	    fc.SetFieldStart (val_start);
	    fc.SetFieldEnd (val_start + val_len - 1);
	    if (FieldName ^= KeyField)
	      {
	        char *entry_id = (char *)tmpBuffer2.Want(val_len + 2);
		strncpy (entry_id, &RecBuffer[val_start], val_len + 1);
		entry_id[val_len + 1] = '\0';
		if (!Key.IsEmpty())
		  message_log(LOG_WARN, "Duplicate Keys defined: overwriting %s with %s", Key.c_str(), entry_id);
		Key = entry_id;
	      }
	    df.SetFct (fc);
	    df.SetFieldName (FieldName);
	    pdft->AddEntry (df);
	  }
	}

      // Store the Attribute value if in our list 
      if ((IsHTMLAttributeTag (*tags_ptr)) > 0)
	{
	  store_attributes (/* Db, */ pdft, RecBuffer, *tags_ptr, GDT_TRUE, &Key, &Datum);
	}
      else if (p == NULL)
	{
#if BSN_EXTENSIONS
	  if (HTML_Level > 2)
	    {
	      // Give some information
	      message_log (LOG_INFO, "%s Warning: '%s'(%u): No end tag for <%s> found, skipping field.",
		Doctype.c_str(), fn.c_str(), (unsigned)(*tags_ptr - RecBuffer), *tags_ptr);
	    }
#endif
	}
    }				/* for() */

#if BSN_EXTENSIONS
  if (Datum.Ok()) NewRecord->SetDate( Datum );
#endif
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
  NewRecord->SetDft (*pdft);
  delete pdft;
}

void HTML::BeforeSearching(QUERY* SearchQueryPtr)
{
  // Rewrite &auml;xxxx -> äxxxx etc.
}

// Summary, e.g. Meta Description field
GDT_BOOLEAN HTML::Summary(const RESULT& ResultRecord,
  const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  const char *fields[] = {
    "META.DC.DESCRIPTION",
    "META.DESCRIPTION",
    "DC.DESCRIPTION",
    "DESCRIPTION",
    NULL};
  StringBuffer->Clear();
  for (size_t i=0; fields[i]; i++)
    {
      Present (ResultRecord, fields[i], RecordSyntax, StringBuffer);
      if (StringBuffer->GetLength())
	break;
    }
  return StringBuffer->GetLength() != 0;
}

GDT_BOOLEAN HTML::URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	GDT_BOOLEAN OnlyRemote) const
{
  // <BASE HREF="xxx">
  STRING path;
  DOCTYPE::Present(ResultRecord, baseHref, &path);
  if (path.GetLength())
    {
      RemoveFileName(&path);
      if (path.GetChr(1) == '"')
	path.EraseBefore(2);
    }
  if (path.IsEmpty())
    return DOCTYPE::URL(ResultRecord, StringBuffer, OnlyRemote);

  StringBuffer->Cat ( ResultRecord.GetFileName() );
  if (StringBuffer) *StringBuffer = path;
  return GDT_TRUE;
}

void HTML::
DocPresent(const RESULT& ResultRecord, const STRING& ElementSet, 
   const STRING& RecordSyntax, STRING* StringBufferPtr) const
{
  StringBufferPtr->Clear();
  if ((ElementSet == FULLTEXT_MAGIC) || (ElementSet == SOURCE_MAGIC))
    Present(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
  else
    DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}


void HTML::Present (const RESULT& ResultRecord, const STRING& ElementSet,
 const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  STRING Value;
  StringBuffer->Clear(); // Zap

  if ((ElementSet == FULLTEXT_MAGIC) || (ElementSet == SOURCE_MAGIC))
    {
       DOCTYPE::DocPresent(ResultRecord, RecordSyntax, ElementSet, &Value);
    }
  else if (ElementSet.Equals(BRIEF_MAGIC))
    {
      // Brief headline is "title"
      STRING Title;
      DOCTYPE::Present (ResultRecord, "title", &Title);

      // If we don't have a name for the headline use its name
      if (Title.IsEmpty())
	{
	  Present(ResultRecord, baseHref, RecordSyntax, &Title);
	  if (Title.GetLength())	  
	    {
	      // Fixup
	      if (Title.GetChr(1) == '"')
		{
		  STRING file = Title.SubString(2, Title.GetLength()-1);
		  Title = RemoveFileName(file) + ResultRecord.GetFileName();
		}
	    }
	  else
	    {
	      // Use file name
	      Title = ResultRecord.GetFileName();
	    }
	}

      if (RecordSyntax == HtmlRecordSyntax)
	{
	  HtmlCat(ResultRecord, Title, &Value, GDT_FALSE);
	}
      else
	Value = Title;
    }
  else if (ElementSet.Search(ATTRIB_SEP) > 1)
    {
      // If <BASE HREF=...> is requested and the document did
      // not have the field, try to generate a synthetic one

      // Is the <BASE HREF="xxx"> value requested?
      if (ElementSet ^= baseHref)
	{
	  // Get the value even on local file system
	  URL(ResultRecord, &Value, GDT_FALSE);
	}
    }
  if (Value.IsEmpty())
     SGMLNORM::Present (ResultRecord, ElementSet, RecordSyntax, &Value);

  // Note: for HTML the world is HtmlRecordSyntax
  if (ElementSet == FULLTEXT_MAGIC)
    {
      STRING temp;
      DOCTYPE::Present(ResultRecord, baseHref, &temp);
      if (temp.GetLength() == 0)
	{
	  // No Base to jam one in
	  Present(ResultRecord, baseHref, HtmlRecordSyntax, &temp);
	  if (temp.GetLength())
	    {
	      STRING target;
	      // Some have:  <base target=_top>
	      DOCTYPE::Present(ResultRecord, baseTarget, &target);
	      // Jam into Value
	      STRINGINDEX pos = Value.SearchAny("</HEAD");
	      if (pos == 0)
		if ((pos = Value.SearchAny("<TITLE")) == 0)
		  if ((pos = Value.SearchAny("<BODY")) == 0)
		    pos = Value.Search('<'); // Ugh? Just do it!
	      if (pos)
		{
		  STRING base;
		  if (pos <=1 || Value.GetChr(pos-1) != '\n')
		    base << "\n";
		  base << "<!-- Inserted by interBasis-WWW " << ISOdate(0) << "-->\n<BASE HREF="
			<< temp;
		  if (target.GetLength())
		    base << " target=" << target;
		  base << ">\n<!-- End Document Modification -->\n";
		  Value.Insert(pos, base);
		}
	    }
	}
    }
  *StringBuffer = Value;
}

HTML::~HTML ()
{
}
