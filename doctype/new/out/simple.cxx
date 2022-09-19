#pragma ident  "@(#)simple.cxx  1.6 02/24/01 17:45:18 BSN"

/*@@@
File:		simple.cxx
Version:	1.00
Description:	Class SIMPLE - Simple headline document type
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <ctype.h>
#include "simple.hxx"

SIMPLE::SIMPLE(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
  NumLines = DOCTYPE::Getoption("LINES", "1").GetInt();
  if (NumLines < 1)
    NumLines = 1;
}

const char *SIMPLE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("SIMPLE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "Simple headline document type. \n\
Runtime search options: LINES N to define to number of lines for the default headline.";
}


// We don't handle either RecordSyntax or DocPresent(), instead we
// let DOCTYPE:: do the work
void SIMPLE::Present(const RESULT& ResultRecord, const STRING& ElementSet, 
		PSTRING StringBuffer) const {
	if (ElementSet.Equals("B")) {
		// Return first non-empty line of text
		*StringBuffer = "";
		STRING tmp;
		ResultRecord.GetRecordData(&tmp);
		STRINGINDEX x = 1;
		const STRINGINDEX len = tmp.GetLength();
		while (x <= len && isspace(tmp.GetChr(x))) {
			x++;	// Loop past non-alphanumeric characters
		}

		UCHR c;
		for (size_t y=1; y<=NumLines && x <= len; y++) {
			if (y > 1) StringBuffer->Cat ('\n');
			while ( x <= len ) {
				if ((c = tmp.GetChr(x++)) != '\0' &&
				  c != '\n' && c != '\r')
					StringBuffer->Cat ( c );
				if (c == '\n') break;
			}
		}
	} else {
		DOCTYPE::Present(ResultRecord, ElementSet, StringBuffer);
	}
}

SIMPLE::~SIMPLE() {
}
