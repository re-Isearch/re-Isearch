#pragma ident  "%Z%%Y%%M%       %I% %G% %U% BSN"
/* ########################################################################

               HTML Document Handler (DOCTYPE)

   File: html.cxx
   Version: %I% %G% %U%
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
#ident "This software is the proprietary and confidential property of \
Edward C. Zimmermann and Basis Systeme netzwerk, Munich."

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <string.h>
//#include <errno.h>
#include "common.hxx"
#include "htmlcore.hxx"
#include "doc_conf.hxx"

HTMLCORE::HTMLCORE (PIDBOBJ DbParent, const STRING& Name) :
	HTML (DbParent, Name)
{
}


// Kludge to identity Attribute tags
int HTMLCORE::IsHTMLAttributeTag (const char *tag)
{
  // HTML Attributes where we are also interested in values
  static struct {
    const char *tag;
    unsigned char len;
  } Tags[] = {
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
    { NULL, 0 }
  };

  if (*tag == '/')
    tag++;

  char ch = tolower(*tag);
  for (int i = 0; Tags[i].len; i++)
    {
      if ((ch == Tags[i].tag[0]
	  && StrNCaseCmp (tag, Tags[i].tag, Tags[i].len) == 0)
	  && isspace (tag[Tags[i].len]))
	return Tags[i].len;
    }
  return 0;			// Not found

}

// We are really only interested in content (container) tags
int HTMLCORE::IsHTMLFieldTag (const char *tag)
{
  // TODO: Better Support Tables
  static struct
  {
    const char *tag;
    unsigned char len;
  } HtmlTagList[] = {
/*-  UNSORTED LIST (lowercase names) -*/
    {"html", 4},
    /* Top level */
    {"head", 4},
    {"body", 4},
    /* header tags */
    {"title", 5},
    {"style", 5},
    {"script", 6}, // 3.2 way to have scripts
    /* body tags */
    // HTML Special Attribute
    {"a", 1}, {"area", 4},
    // Headers-- chapter, section, subsection,...
    {"h1", 2}, {"h2", 2}, {"h3", 2}, {"h4", 2}, {"h5", 2}, {"h6",2},
    // Document divisions
    {"div", 3},
    {"address", 7},
    {"xmp", 3}, // Obsolete
    // Misc format (HTML 3.0) elements
    {"lang", 4},
    {"person", 6}, {"acronym", 7}, {"abbrev", 6},
    {"abstract", 8},
    { 0, 0 }
  };

  // Is this part of a "meaningful" tag?
  if (*tag == '/')
    tag++;

  char ch = tolower(*tag);

  for (int i = 0; HtmlTagList[i].len; i++)
    {
      if (ch == HtmlTagList[i].tag[0]
	  && StrNCaseCmp (tag, HtmlTagList[i].tag, (int) HtmlTagList[i].len) == 0
	  && !isalnum (tag[HtmlTagList[i].len]))
	{
	  return HtmlTagList[i].len;	// YES

	}
    }

  // Is it a tag where we just want the attribute value?
  return IsHTMLAttributeTag (tag);
}

HTMLCORE::~HTMLCORE ()
{
}
