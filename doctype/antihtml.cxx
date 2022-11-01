/* ########################################################################

               ANTIHTML Document Handler (DOCTYPE)

   File: antihtml.cxx
   Version: $Revision: 1.1 $
   Description: Class ANTIHTMLTAG - WWW ANTIHTML Document Type
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@nonmonotonic.net
   Modified: Thu Dec 28 21:47:08 MET 1995
   Last maintained by: Edward C. Zimmermann

   RCS $Revision: 1.1 $ $State: Exp $


   ########################################################################

   Note: Requires that HTML is configured

   ########################################################################

   Copyright (c) 1995 : Basis Systeme netzerk. All Rights Reserved.

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.

   ######################################################################## */

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "common.hxx"
#include "antihtml.hxx"
#include "doc_conf.hxx"

#define ATTRIB_SEP '@' /* Must match value in SGMLNORM */


/* ------- ANTIHTML Support --------------------------------------------- */
ANTIHTML::ANTIHTML (PIDBOBJ DbParent, const STRING& Name) :
	HTML (DbParent, Name)
{
/*
  TagRegistry = NULL;
*/
}

const char *ANTIHTML::Description(PSTRLIST List) const
{
  List->AddEntry ("ANTIHTML");
  HTML::Description(List);
  return "HTML but not indexing the standard tags but meta and title.";
}


void ANTIHTML::BeforeIndexing()
{
  // TODO: Load FieldName Mappings here..
/*
  TagRegistry = new REGISTRY ("ANTIHTML");

  // Read doctype options
  STRING S;
  STRLIST StrList;
  Db->GetDocTypeOptions (&StrList);
  if (StrList.GetValue ("CONFIG-FILE", &S) == false)
    S = "/opt/BSN/conf/antihtml.ini";
  useBuiltin = (TagRegistry->ProfileLoadFromFile (S) == 0);
*/
  HTML::BeforeIndexing();
}

void ANTIHTML::AfterIndexing()
{
  tmpBuffer.Free();
  tmpBuffer2.Free();
 // if (TagRegistry) delete TagRegistry;
  HTML::AfterIndexing();
}

void ANTIHTML::AddFieldDefs()
{
  HTML::BeforeIndexing();
}

// Map META.XXX --> XXX and pass to HTML unification..
STRING ANTIHTML::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  const char *tag = (const char *)Tag;
  if (tag && StrNCaseCmp(tag, "META", 4) == 0)
    {
      if (tag[4] == '.')
	{
	  // eg. Map META.DC.DATE to DC.DATE
	  return HTML::UnifiedName (tag + 5, Value);
	}
      else
	{
	  // '@' or ...
	  return *Value = ""; // Not interested in these!
	}
    }
  else if (Tag ^= "REFRESH")
    return *Value = ""; // uninteresting..
  return HTML::UnifiedName(Tag, Value);
}

typedef struct {
    const char *tag;
    unsigned char len;
} TagList_t;

// Kludge to identity Attribute tags
static inline int IsHTMLAttributeTag (const char *tag)
{
  // ANTIHTML Attributes where we are also interested in values
  static TagList_t Tags[] = {
/*- UNSORTED LIST (lowercase names) -*/ 
    // Head elements
    { "base", 4 },
    { "meta", 4 },
//  { "link", 4 }, // Not interested in links
    { NULL, 0 }
  };

  if (*tag == '/')
    tag++;

  char ch = tolower(*tag);
  // Linear search since its so a short list!
  for (int i = 0; Tags[i].len; i++)
    {
      if ((ch == Tags[i].tag[0]
	  && StrNCaseCmp (tag, Tags[i].tag, Tags[i].len) == 0)
	  && isspace (tag[Tags[i].len]))
	return Tags[i].len;
    }
  return 0;			// Not found
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
// Ignore HTML tags.
// Only TITLE, META and BASE are not ignored.
// these are usefull
static int IgnoreHTMLTag (const char *tag)
{
  // TODO: Better Support Tables
  static TagList_t List[] = {
/*- SORTED LIST (lowercase names) AND NO NULL TERMINATION -*/
   {"a", 1},
   {"address", 7},
   {"applet", 6},
   {"area", 4},
   {"array", 5},
   {"au", 2},
   {"b", 1},
   {"basefont", 8},
   {"bdo", 3},
   {"big", 3},
   {"blink", 5},
   {"blockquote", 10},
   {"body", 4},
   {"box", 3},
   {"bq", 2},
   {"br", 2},
   {"button", 6},
   {"caption", 7},
   {"center", 6},
   {"center", 6},
   {"cite", 4},
   {"code", 4},
   {"col", 3},
   {"colgroup", 8},
   {"color", 5},
   {"dd", 2},
   {"del", 3},
   {"dfn", 3},
   {"dir", 3},
   {"div", 3},
   {"dl", 2},
   {"dt", 2},
   {"em", 2},
   {"fieldset", 8},
   {"font", 4},
   {"form", 4},
   {"frame", 5},
   {"frameset", 8},
   {"h1", 2},
   {"h2", 2},
   {"h3", 2},
   {"h4", 2},
   {"h5", 2},
   {"h6", 2},
   {"head", 4},
   {"hr", 2},
   {"html", 4},
   {"i", 1},
   {"iframe", 6},
   {"img", 3},
   {"input", 5},
   {"ins", 3},
   {"item", 4},
   {"java", 4},
   {"javascript", 10},
   {"kbd", 3},
   {"label", 5},
   {"lang", 4},
   {"lh", 2},
   {"li", 2},
   {"link", 4},
   {"listing", 7},
   {"map", 3},
   {"math", 4},
   {"menu", 4},
   {"message", 7},
   {"nextid", 6},
   {"noframe", 7}, // Cougar is below.. 
   {"noframes", 8},
   {"noscript", 8},
   {"note", 4},
   {"object", 6},
   {"ol", 2},
   {"option", 6},
   {"overlay", 7},
   {"overstrike", 10},
   {"p", 1},
   {"param", 5},
   {"plaintext", 9},
   {"pre", 3},
   {"q", 1},
   {"quote", 5},
   {"row", 3},
   {"s", 1},
   {"samp", 4},
   {"script", 6},
   {"select", 6},
   {"size", 4},
   {"small", 5},
   {"span", 4},
   {"strike", 6},
   {"strong", 6},
   {"style", 5},
   {"sub", 3},
   {"sup", 3},
   {"tab", 3},
   {"table", 5},
   {"tbody", 5},
   {"td", 2},
   {"textarea", 8},
   {"textflow", 8},
   {"tfoot", 5},
   {"th", 2},
   {"thead", 5},
   {"tl", 2},
   {"tli", 3},
   {"tr", 2},
   {"tt", 2},
   {"u", 1},
   {"ul", 2},
   {"underline", 9},
   {"var", 3},
   {"xmp", 3}
  };

  // Is this part of a tag that we don't want?
  return tag_search(tag, List, sizeof(List)/sizeof(List[0]));
// return 0;
}

void ANTIHTML::ParseFields (RECORD *NewRecord)
{
  // Open the file
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = Db->ffopen (fn, "rb");
  if (fp == NULL)
    {
      message_log (LOG_ERRNO, "Couldn't access '%s'", (const char *)fn);
      NewRecord->SetBadRecord();
      return;                   // ERROR
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp);

  SRCH_DATE RecDate(fp);
  STRING Key;
  if (RecEnd <= RecStart)
    {
#if BSN_EXTENSIONS
      // 2147483647-2147483647
      char tmp[32];
      if (RecStart || RecEnd)
        sprintf(tmp, "[%ld-%ld]", (long)RecStart, (long)RecEnd);
      else
        tmp[0] = '\0';
      message_log (LOG_WARN, "%s: zero-length record - '%s'%s...",
       Doctype.c_str(), fn.c_str(), tmp);
      message_log (LOG_NOTICE, "%s skipping record", Doctype.c_str());
#else
      cerr << Doctype << ": zero-length record - " << fn << endl;
#endif
      Db->ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  if (RecStart && -1 == fseek (fp, RecStart, SEEK_SET))
    {
#if BSN_EXTENSIONS
      message_log (LOG_ERRNO, "%s::ParseRecords(): Seek to %ld failed - '%s', skipping.",
	Doctype.c_str(), (long)RecStart, fn.c_str());
#endif
      Db->ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  // Read the whole record in a buffer
  GPTYPE RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1);
  size_t ActualLength = (size_t) fread (RecBuffer, sizeof(CHR), RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  Db->ffclose (fp);

  STRING FieldName;
  DF df;
  DFD dfd;

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL)
    {
#if BSN_EXTENSIONS
      char tmp[32];
      if (RecStart || RecEnd)
	sprintf(tmp, "[%ld-%ld]", (long)RecStart, (long)RecEnd);
      else
	tmp[0] = '\0';
      message_log (LOG_WARN, "Unable to parse %s-tags in '%s'%s",
	Doctype.c_str(), fn.c_str(), tmp);
#endif
      NewRecord->SetBadRecord();
      return; // ERROR
    }

  PDFT pdft = new DFT ();
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      if ((*tags_ptr)[0] == '/')
	continue;
      // Accept almost anything not in our list to ignore
      if (IgnoreHTMLTag(*tags_ptr))
	continue;

      const char *p = find_end_tag ((const char *const *)tags_ptr, (const char *)*tags_ptr);
      if (p == NULL)
	{
	  if (StrCaseCmp (*tags_ptr, "title") == 0)
	    {
	      // By title use next tag...
	      if ((p = tags_ptr[1]) != NULL)
		{
#if BSN_EXTENSIONS
		  message_log (LOG_INFO, "%s: \"%s\"(%u): \
Bogus use of <%s> found, using <%s> as end tag.", Doctype.c_str(), fn.c_str(),
			(unsigned)(*tags_ptr - RecBuffer),
			tags_ptr[0],
			p );
#endif
		}
	    }
	}			// end code to handle some ANTIHTML minimized tags

      if (p != NULL)
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
	    if (UnifiedName (*tags_ptr, &FieldName).GetLength() == 0)
	      continue; // Ignore this one
	    FieldName.ToUpper(); // store tags uppercase (not needed yet)
	    if (orig_char) *tcp = orig_char;

	    dfd.SetFieldName (FieldName);
	    Db->DfdtAddEntry (dfd);

	    df.SetFct ( FC(val_start, val_start + val_len - 1) );
	    if ((FieldName ^= KeyField) || (FieldName ^= DateField) || (FieldName ^= LanguageField))
	      {
		char *entry_id = (char *)tmpBuffer2.Want(val_len + 2);
		strncpy (entry_id, &RecBuffer[val_start], val_len + 1);
		entry_id[val_len] = '\0';

		if (FieldName ^= KeyField)
		  {
		    if (!Key.IsEmpty())
		      message_log(LOG_WARN, "Duplicate Keys defined: overwriting %s with %s", Key.c_str(), entry_id);
		    Key = entry_id;
		  }
		else if (FieldName ^= DateField)
		  {
		    if (RecDate.Ok())
		      message_log (LOG_DEBUG, "Setting date (%s) to %s", FieldName.c_str(), entry_id);
		    RecDate = entry_id;
		    if (!RecDate.Ok())
		      message_log (LOG_WARN, "Unsupported/Unrecognized date format: '%s'", entry_id);
		  }
		else
		  {
		    // Only if valid do we over-ride
		    SHORT code = Lang2Id ( &RecBuffer[val_start] );
		    if (code != 0)
		      NewRecord->SetLanguage (code);
		  }
	      }
	    df.SetFieldName (FieldName);
	    pdft->AddEntry (df);
	  }
	}

      // Store the Attribute value if in our list 
      if ((IsHTMLAttributeTag (*tags_ptr)) > 0)
	{
	  store_attributes (/* Db, */ pdft, RecBuffer, *tags_ptr, true,
		&Key, &RecDate);
	}
      else if (p == NULL)
	{
	  /* Nothing */
	}
    }				/* for() */

  NewRecord->SetDft (*pdft);
  if (RecDate.Ok()) NewRecord->SetDate(RecDate);
  if (!Key.IsEmpty())
    {
      if (Db->KeyLookup (Key))
        message_log (LOG_ERROR, "Record in \"%s\" uses a non-unique %s '%s'",
          fn.c_str(), KeyField.c_str(), Key.c_str());
      else
	{
	  message_log (LOG_DEBUG, "Setting key of record in \"%s\" to '%s'", fn.c_str(), Key.c_str());
          NewRecord->SetKey (Key);
	}
    }
  delete pdft;
}

ANTIHTML::~ANTIHTML ()
{
/*
  if (TagRegistry) delete TagRegistry; 
*/
}
