#pragma ident  "@(#)ftp.cxx	1.5 03/28/98 19:32:26 BSN"
/* ########################################################################

               FTP Binary Multiple Document Handler (DOCTYPE)

   File: ftp.cxx
   Version: $Revision: 1.1 $
   Path: /home/furball/edz/Dist/Networking/CNIDR/Dist/New/Isearch-1.09/doctype/binary.cxx
   Description: Class FTP - Binary and associated IAFA files 
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@bsn.com
   Modified: Fri Dec 29 11:57:19 MET 1995
   Last maintained by: Edward C. Zimmermann

   RCS $Revision: 1.1 $ $State: Exp $


   ########################################################################

   Note: None 

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
#include "ftp.hxx"
#include "doc_conf.hxx"

/* ------- Binary Support --------------------------------------------- */

FTP::FTP (PIDBOBJ DbParent, const STRING& Name):
	IAFADOC (DbParent, Name)
{
  PostFix = (char *)".iafa";
}

const char *FTP::Description(PSTRLIST List) const
{
  List->AddEntry ("FTP");
  IAFADOC::Description(List);
  return "Like BINARY but expects a .iafa for the package";
}



void FTP::SourceMIMEContent(PSTRING StringPtr) const
{
  // Default
  *StringPtr = "Application/Octet-Stream";
}

void FTP::SourceMIMEContent(const RESULT& ResultRecord,
   PSTRING StringPtr) const
{
  IAFADOC::Present(ResultRecord, "Content-type", SutrsRecordSyntax, StringPtr);
  if (StringPtr->IsEmpty() )
    SourceMIMEContent(StringPtr);
}

void FTP::ParseRecords(const RECORD& FileRecord)
{
  STRING Fn (FileRecord.GetFullFileName( ) );
  STRING IafaFn (Fn + PostFix);

  // Add postfix to point to "text" info file
  if (!FileExists(IafaFn))
    IafaFn = Fn.Right('.') + PostFix;

  GPTYPE RecEnd = GetFileSize(IafaFn);
  if (RecEnd == 0) {
    logf (LOG_ERROR, "%s: no '%s' or '%s'", Doctype.c_str(), (Fn+PostFix).c_str(), IafaFn.c_str() );;
  } else {
    RECORD Record (FileRecord);
    Record.SetFullFileName( IafaFn );
    Record.SetDocumentType ( Doctype );
    Record.SetRecordStart( 0 );
    Record.SetRecordEnd( RecEnd );
    Db->DocTypeAddRecord(Record);
  }
}

void FTP::ParseFields (PRECORD NewRecord)
{
  IAFADOC::ParseFields (NewRecord);
}


void FTP::DocPresent (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  STRING Filename, Content;
  *StringBuffer = "";
  if (ElementSet.Equals(SOURCE_MAGIC))
    {
      ResultRecord.GetFullFileName(&Filename);
      Content.ReadFile(Filename);
      if (RecordSyntax == HtmlRecordSyntax)
	*StringBuffer << "Content-type: " << "text/plain" << "\n\n";
    }
  else if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      // Call method to get full file name
      Present(ResultRecord, "FULLPATH", SutrsRecordSyntax, &Filename);
      Content.ReadFile(Filename);
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING mime;
	  SourceMIMEContent(ResultRecord, &mime);
	  *StringBuffer << "Content-type: " << mime << "\n\n";
	}
    }
  else
    {
      Present(ResultRecord, ElementSet, RecordSyntax, &Content);
    }
  *StringBuffer << Content;
}

void FTP::
Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    {
      DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet.Equals ("FULLPATH"))
    {
      STRING Filename;
      ResultRecord.GetFullFileName(&Filename);
      STRINGINDEX x = Filename.SearchReverse(PostFix);
      if (x) Filename.EraseAfter(x-1);
      if (RecordSyntax == HtmlRecordSyntax)
	HtmlCat(Filename, StringBuffer);
      else
	*StringBuffer = Filename;
    }
  else
    IAFADOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

FTP::~FTP ()
{
}
