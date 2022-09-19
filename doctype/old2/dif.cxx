/*-@@@
File:		dif.cxx
Version:	1.00
Description:	Class DIF - DIF data
Author:		Archie Warnock, warnock@clark.net
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "common.hxx"
#include "doc_conf.hxx"

#include "strstack.hxx"
#include "gstack.hxx"
#include "dif.hxx"

#define AW_DEBUG 0

#define MaxDIFSize 10240

// Local prototypes
DIF::DIF (PIDBOBJ DbParent, const STRING& Name):
	COLONGRP (DbParent, Name)
{
  if (DateField.IsEmpty())
    DateField = "DIF_Revision_Date";
}

const char *DIF::Description(PSTRLIST List) const
{
  List->AddEntry ("DIF");
  COLONGRP::Description(List);
  return "DIF Document Type";
}


void DIF::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-DIF";
}

void DIF::
DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    const STRING& RecordSyntax, PSTRING StringBufferPtr) const
{
  if (ElementSet == SOURCE_MAGIC)
    DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
  else
    COLONGRP::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}

#define  ENTRY_ID       "Entry_ID"
#define  ENTRY_TITLE    "Entry_Title" 
#define  START_DATE     "Start_Date"
#define  STOP_DATE      "Stop_Date"
#define  SOURCE         "Source"
#define  SENSOR         "Sensor"
#define  ORIG_CENTER    "Originating_Center"
#define  LOCATION       "Location"
#define  DISCIPL        "Discipline"
#define  PARAMETER      "Parameter"
#define  REVIEW_DATE    "Revision_Date"
#define  LAST_NAME      "Last_Name"
#define  FIRST_NAME     "First_Name"
#define  MID_NAME       "Middle_Name"
#define  PHONE          "Phone"
#define  EMAIL          "Email"
#define  DATA_CENTER_N  "Data_Center_Name"
#define  DATASET_ID     "Dataset_ID"
#define  KEYWORD        "Keyword"
#define  SCI_REVIEW_DATE  "Science_Review_Date" 
#define  FUT_REVIEW_DATE  "Future_Review_Date"
#define  QUALITY        "Quality"
#define  PROGECT        "Project"
#define  CAMPAIGN       "Campaign"
#define  STOR_MED       "Storage_Medium"

static void GetPersGroup(const STRING& Input, PSTRING Output)
{
  STRLIST Lines;

  Lines.Split ("\n", Input);

  STRING FirstName, MidName, LastName, Phone;
  STRING val;
  GDT_BOOLEAN look = GDT_TRUE;
  GDT_BOOLEAN inName = GDT_FALSE;

  STRING lp;

  *Output = "";

  const size_t len = Lines.GetTotalEntries();
  for (size_t i=1; i<=len; i++)
    {
	Lines.GetEntry(i, &val);
	const char *tcp = val;
	while (isspace(*tcp)) tcp++;
	if (look && strncasecmp("<PRE>", tcp, 5) == 0) {
	  tcp += 5;
	  Output->Cat("<PRE>");
	  while(isspace(*tcp)) tcp++;
	  look = GDT_FALSE;
	}

	if (inName == GDT_FALSE) {
	  if (LastName.GetLength()) {
	    Output->Cat (LastName);
	    Output->Cat (", ");
	    Output->Cat (FirstName);
	    if (MidName.GetLength())
	      {
		Output->Cat (" ");
		Output->Cat (MidName);
	      }
	    Output->Cat ("\n");
	  }
	  LastName = MidName = FirstName = "";
	}
	if (lp.GetLength()) {
	  Output->Cat(lp);
	  lp = "";
	}

	if (strncasecmp(LAST_NAME,tcp, strlen(LAST_NAME)) == 0) {
	    tcp += strlen(LAST_NAME) + 2; 
	    while(isspace(*tcp)) tcp++;
	    LastName = tcp;
	    inName = GDT_TRUE;
	} else if (strncasecmp(FIRST_NAME, tcp, strlen(FIRST_NAME)) == 0) {
	    tcp += strlen(LAST_NAME) + 2; 
	    while(isspace(*tcp)) tcp++;
	    FirstName = tcp;
	    inName = GDT_TRUE;
	} else if (strncasecmp(MID_NAME, tcp, strlen(MID_NAME)) == 0) {
	    tcp += strlen(MID_NAME) + 2;
	    while(isspace(*tcp)) tcp++;
	    MidName = tcp;
	    inName = GDT_TRUE;
	} else if (strncasecmp("Group: ", tcp, 7) == 0) {
	  tcp += 7;
	  while (isspace(*tcp)) tcp++;
	  lp = tcp;
	  lp.Cat(": ");
	  inName = GDT_FALSE;
	} else if (strncasecmp("End_Group", tcp, 9) == 0 ) {
	  tcp += 9;
	  if (*tcp) lp = tcp;
	  inName = GDT_FALSE;
	} else {
	  lp = tcp;
	  lp.Cat ("\n");
	  inName = GDT_FALSE;
	}
    }

  if (lp.GetLength()) Output->Cat(lp);
}


void DIF::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 const STRING& RecordSyntax, PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "";
  if (ElementSet == BRIEF_MAGIC)
    {
      STRING Headline;

      // Brief headline is "title"
      DOCTYPE::Present (ResultRecord, "Entry_Title", &Headline);
      if (Headline.GetLength() == 0)
	Headline = "<Untitled Record>";
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  HtmlCat(Headline, StringBufferPtr);
	}
      else
	{
	  *StringBufferPtr = Headline;
	}
    }
#if 1
  // Handle Personal Groups
  else if ((ElementSet ^= "Investigator") || (ElementSet ^= "Technical_Contact") ||
	(ElementSet ^= "Data_Center"))
    {
      STRING val;
      COLONGRP::Present (ResultRecord, ElementSet, RecordSyntax, &val); 
      GetPersGroup(val, StringBufferPtr);
    }
#endif
  else
    COLONGRP::Present (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}

DIF::~DIF ()
{
}

