#pragma ident  "@(#)mailman.cxx  1.3 02/24/01 17:44:55 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		mailman.cxx
Version:	$Revision: 1.1 $
Description:	Class MAILMAN - Mailman (Python) Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "mailman.hxx"

MAILMAN::MAILMAN (PIDBOBJ DbParent, const STRING& Name) :
	MAILDIGEST (DbParent, Name)
{
}

const char *MAILMAN::Description(PSTRLIST List) const
{
  List->AddEntry ("MAILMAN");
  MAILDIGEST::Description(List);
  return "Mailman (Python) Mail Digests";
}


const CHR *MAILMAN::Seperator() const
{
  return "\n--__--__--";
}

MAILMAN::~MAILMAN ()
{
}
