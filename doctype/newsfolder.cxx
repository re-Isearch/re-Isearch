#pragma ident  "@(#)newsfolder.cxx	1.5 02/24/01 17:45:34 BSN"
/* ########################################################################

		       Usenet News Folder Doctype

			 Basis Systeme netzwerk
			   Brecherspitzstr. 8
			D-81541 Munich, Germany

   File: newsfolder.cxx
   Description: Class NEWSFOLDER - RN News folder Document Type
   Version: $Revision: 1.1 $

   Created: Sun Dec 24 23:11:21 MET 1995
   Author: Edward C. Zimmermann <edz@nonmonotonic.net>
   Modified: Sun Dec 24 23:11:22 MET 1995
   Last maintained by: Edward C. Zimmermann <edz@nonmonotonic.net>


			   (c) Copyright 1995
		     Basis Systeme netzwerk, Munich

   ########################################################################

   Note: This is really just an alias for mailfolder.... 

   ####################################################################### */

#include "newsfolder.hxx"

NEWSFOLDER::NEWSFOLDER (PIDBOBJ DbParent, const STRING& Name) :
	MAILFOLDER (DbParent, Name)
{
}

const char *NEWSFOLDER::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("NEWSFOLDER");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  MAILFOLDER::Description(List);
  return "RN News folders (Usenet news)";
}


void NEWSFOLDER::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for Newsfolder records
  *StringPtr = "message/news";
}


NEWSFOLDER::~NEWSFOLDER ()
{
}
