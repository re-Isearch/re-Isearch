#pragma ident  "@(#)htmlcache.cxx	1.6 02/24/01 17:45:30 BSN"
/* ########################################################################

	       HTTP Cache HTML Document Handler (DOCTYPE)

   File: htmlcache.cxx
   Version: $Revision: 1.1 $
   Path: /home/furball/edz/Dist/Networking/CNIDR/Dist/New/Isearch-1.09/doctype/htmlcache.cxx
   Description: Class HTMLCACHE - WWW Server Cached Documents
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@bsn.com
   Modified: Thu Dec 28 21:38:31 MET 1995
   Last maintained by: Edward C. Zimmermann

   RCS $Revision: 1.1 $ $State: Exp $
   

   ########################################################################

   Note: Requires that both HTML and SGMLNORM are configured 

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
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <string.h>
//#include <errno.h>
#include "common.hxx"
#include "htmlcache.hxx"
#include "doc_conf.hxx"

/* NOTE: The following might need to be changed to reflect
   the character and nature of "your" HTTP proxy server.
 */
#define CACHE_MAGIC "Date: "	/* First stuff in HTML Cache */
#define DEFAULT_MNT "/http/"	/* default  .../http/host/path */


#if BSN_EXTENSIONS < 3
#define  ATTRIB_SEP '@'		/* Must match '@' in sgmlnorm.cxx ! */
#endif


/* ------- HTML Support --------------------------------------------- */

HTMLCACHE::HTMLCACHE (PIDBOBJ DbParent, const STRING& Name):
	HTML (DbParent, Name)
{
  // Read doctype options
  const char *tp = getenv("HTTPD_CACHE_ROOT");
  STRING S;
  if (Db)
    {
      STRLIST StrList;
      Db->GetDocTypeOptions (&StrList);
      StrList.GetValue ("HTTPD_CACHE_ROOT", &S);
    }
  if (S.GetLength() == 0)
    S = tp ? tp : DEFAULT_MNT;
  if (S.GetLength ())
    {
      AddTrailingSlash(&S);
      CacheMnt = S;
    }
}

const char *HTMLCACHE::Description(PSTRLIST List) const
{
  List->AddEntry ("HTMLCACHE");
  HTML::Description(List);
  return "HTML Caches from proxy (e.g. CERN) servers";
}


#if 0
void HTMLCACHE::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Medline records
  GPTYPE Start = FileRecord.GetRecordStart();
  GPTYPE End = FileRecord.GetRecordEnd();

  STRING Fn;
  FileRecord.GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      return;                   // File not accessed
    }

  const char content[] = "Content-type: ";
  while (fgets(tmp, sizeof(tmp)/sizeof(char) -1 , Fp) != NULL)
    {
      // End of header?
      if (tmp[0] == '\r' || tmp[1] == '\n') break;
      if (strncmp(tmp, content, sizeof(content)/sizeof(char)-1) == 0) {
	// Have the Content magic
	if (strncmp(&tmp[sizeof(content)/sizeof(char)], "text/html") == 0) {
	  fclose(fp);
	  return;
	}
      }
    }
  // Not HTML so just store header information
  GPTYPE RecEnd =  GetFileSize(Fp);
  if (End > 10 && RecEnd > End) RecEnd = End;
  fclose(Fp);
 
  RECORD Record;
  STRING s;
  FileRecord.GetPath(&s);
  Record.SetPath( s );
  FileRecord.GetFileName(&s);
  Record.SetFileName( s );
  s = "WEBCACHE";
  Record.SetDocumentType ( s );
  Record.SetRecordStart(Start);
  Record.SetRecordEnd(RecEnd);
  Db->DocTypeAddRecord(Record);

}
#endif

void HTMLCACHE::ParseFields (PRECORD Record)
{
#if BSN_EXTENSIONS
  STRING Fn;
  Record->GetFullFileName (&Fn);
  PFILE Fp = fopen (Fn, "rb");
  if (!Fp)
    {
      return;                   // File not accessed
    }
  char buf[256];
  while (NULL != fgets(buf, sizeof(buf)/sizeof(char)-1, Fp))
    {
      if (strncmp(buf, "Date: ",  6) == 0)
	{
	  Record->SetDate ( buf + 6 );
	  break;
	}
      // Empty line means end of cache header
      if (buf[0] == '\0' || isspace(buf[0])) break;
    }
  fclose(Fp);
#endif

  HTML::ParseFields (Record);
}

// Kludgy code to handle proxy caches, it might need to
// be changed/modified for other proxy servers...
void HTMLCACHE::
Present (const RESULT &ResultRecord,
	 const STRING &ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  STRING FieldName;
  STRING Url;

  if (ElementSet.Equals (BRIEF_MAGIC))
    FieldName = "title";	// Brief headline is "title"
  else
    {
      if (CacheMnt != "")
	{
	  // Derive the URL for the document
	  STRING Filename;
	  ResultRecord.GetFullFileName (&Filename);
	  // Search for "mount" point
	  STRINGINDEX x = Filename.Search (CacheMnt);
	  if (x)
	    {
	      Filename.EraseBefore (x + CacheMnt.GetLength());
	      Url = "http://";
	      Url.Cat (Filename);
	    }

	  // Is the <BASE HREF="xxx"> value requested?
	  char tmp[12];
	  sprintf (tmp, "BASE%cHREF", ATTRIB_SEP);
	  STRING BaseHref = tmp;
	  if ((ElementSet ^= BaseHref) && Url.GetLength ())
	    {
	      // return URL
	      *StringBuffer = Url;
	      return;		// Done
	    }
	}
      FieldName = ElementSet;
    }

  HTML::Present (ResultRecord, FieldName, RecordSyntax, StringBuffer);

  // If full document selected, return bit after Cache header
  if (FieldName == FULLTEXT_MAGIC && StringBuffer->GetLength () > 10)
    {
      // Search for Cache magic
      char tmp[12];
      char magic[] = CACHE_MAGIC;

      int i;
      for (i = 0; i < 10; i++)
	tmp[i] = StringBuffer->GetChr (i + 1);
      tmp[i] = 0;
      if (strncmp (tmp, magic, sizeof (magic) / sizeof (char) - 1) == 0)
	{
	  PCHR rec = StringBuffer->NewCString ();
	  PCHR tcp;
	  for (tcp = rec+1; *tcp && (*tcp != '<'); tcp++)
	    {
	      if (tcp[0] == '\n' && tcp[-1] == '\n')
		break;
	    }
	  if (*tcp)
	    {
	      *StringBuffer = "";
	      if (Url.GetLength ())
		{
		  StringBuffer->Cat ("<BASE HREF=\"");
		  StringBuffer->Cat (Url);
		  StringBuffer->Cat ("\">");
		}
	      StringBuffer->Cat (tcp);
	    }
	  delete[]rec;
	}
    }
}

HTMLCACHE::~HTMLCACHE ()
{
}
