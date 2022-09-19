/* ########################################################################

			   FIRSTLINE Doctype

		     Basis Systeme netzwerk/Munich
			   Brecherspitzstr. 8
			D-81541 Munich, Germany

	(c) Copyright 1995,1996,1997 Basis Systeme netzwerk

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.


   ########################################################################

   Note: Before using this code you must register.
	http://www.bsn.com/Z39.50/License.html 

   ####################################################################### */

#pragma ident  "@(#)firstline.cxx	1.9 02/24/01 17:45:23 BSN"
/************************************************************************
************************************************************************/
/*-@@@
File:		firstline.cxx
Version:	1.00
Description:	Class FIRSTLINE - TEXT with headline as first line 
Author:		Edward C. Zimmermann, edz@bsn.com
Modified:	Sun Jan 26 21:05:30 MET 1997
Maintained by:	Edward C. Zimmermann, edz@bsn.com
@@@-*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "firstline.hxx"
#include "doc_conf.hxx"

#define HEADLINE_FIELD_NAME "Headline"

// Should Firstline be an option of type "TEXT"?
// For now it is a distinct document type.

FIRSTLINE::FIRSTLINE (PIDBOBJ DbParent, const STRING& Name):
	DOCTYPE (DbParent, Name)
{
  const char option[] = "Startline";
  StartLine = DOCTYPE::Getoption(option, "1"). GetInt();
  if (StartLine < 1)
    {
      logf (LOG_ERROR, "%s: Option %s specified with value < 1 (%d)", Doctype.c_str(), option, StartLine);
      StartLine = 1;
    }
}

const char *FIRSTLINE::Description(PSTRLIST List) const
{
  List->AddEntry ("FIRSTLINE");
  DOCTYPE::Description(List);
  return "Text with headline as Nth line or sentence.\n\
Index-time options:\n\
  Startline N\tSpecifies which line to use (default is 1)";
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
void FIRSTLINE::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      logf (LOG_ERRNO, "Could not access %s", fn.c_str() );
      return;		// ERROR
    }

  GPTYPE start = NewRecord->GetRecordStart();
  GPTYPE End = NewRecord->GetRecordEnd();

  fseek(fp, start, SEEK_SET);

  int Ch;
  GPTYPE end = 0;
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan, Start, Newline, Punct} State = Scan;
  int lineno = 0;
loop:
  while ((Ch = getc(fp)) != EOF) {
    if (++end >= End && End != 0) break;
    if (State == Scan)
      {
	if (isalnum(Ch))
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
  if (++lineno <  StartLine && (End <= 0 || end < End) && Ch != EOF)
    {
      State = Scan;
      goto loop;
    }

  if (State == Newline) end--; // Leave off NL
  if (isspace(Ch)) end--; // Leave off trailing white space
  Db->ffclose (fp);

  if (End && start >= End) return; // Nothing

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;

  STRING FieldName;
  FieldName = HEADLINE_FIELD_NAME;
  dfd.SetFieldName (FieldName);
  Db->DfdtAddEntry (dfd);
  fc.SetFieldStart (start);
  fc.SetFieldEnd (end-1);
  df.SetFct (fc);
  df.SetFieldName (FieldName);
  pdft->AddEntry (df);

  NewRecord->SetDft (*pdft);
  delete pdft;
}

void FIRSTLINE::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
         STRING *StringBufferPtr) const
{
  if (ElementSet == BRIEF_MAGIC)
    DOCTYPE::Present (ResultRecord, HEADLINE_FIELD_NAME, StringBufferPtr);
  else
    DOCTYPE::Present (ResultRecord, ElementSet, StringBufferPtr);
}


FIRSTLINE::~FIRSTLINE ()
{
}
