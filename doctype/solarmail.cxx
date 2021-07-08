static const char RCS_Id[]="$Id: solarmail.cxx,v 1.1 2007/05/15 15:47:29 edz Exp $";
/* ########################################################################

		       Usenet News Folder Doctype

			 Basis Systeme netzwerk
			   Brecherspitzstr. 8
			D-81541 Munich, Germany

   File: sunfolder.cxx
   Description: Class SOLARMAIL - Sun Solaris 2.x Mailfolder Document Type
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

#include "solarmail.hxx"

SOLARMAIL::SOLARMAIL (PIDBOBJ DbParent): MAILFOLDER::MAILFOLDER (DbParent)
{
}

void SOLARMAIL::ParseRecords (const RECORD& FileRecord)
{
#if BSN_EXTENSIONS
  RECORD Record (FileRecord); // Easy way
#else
  /* CNIDR must do it the hard way! (see above ) */
  RECORD Record;
  STRING s;
  FileRecord.GetPath(&s);
  Record.SetPath( s );
  FileRecord.GetFileName(&s);
  Record.SetFileName( s );
  FileRecord.GetDocumentType(&s);
#endif
  Record.SetDocumentType ("MAILFOLDER");
  MAILFOLDER::ParseRecords (Record);
}

SOLARMAIL::~SOLARMAIL ()
{
}
