////// PORT TO IB IN PROGESS 


// $Id: iso19115cd2.cxx,v 1.1 2007/05/15 15:47:29 edz Exp $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee.

************************************************************************/

#ifndef DEF_FILTER_PATH
#define DEF_FILTER_PATH "/usr/local/src/xml-xalan/c/"
#endif

#ifndef DEF_ISO19115_FILTER
#define DEF_ISO19115_FILTER "/usr/local/src/xml-xalan/c/bin/XalanTransform"
#endif

#ifndef DEF_ISO19115_STYLE
#define DEF_ISO19115_STYLE  "/home/cnidr/fgdc/ISO/ISO_19115_CD2"
#endif

#ifndef DEF_ISO19115_DTD
#define DEF_ISO19115_DTD  "/home/cnidr/fgdc/ISO/ISO_19115_CD2.dtd"
#endif

#ifndef FULL_HTML_XSL
#define FULL_HTML_XSL "_html.xsl"
#endif

#ifndef FULL_SUTRS_XSL
#define FULL_SUTRS_XSL "_sutrs.xsl"
#endif

#ifndef TEMP_DIR
#define TEMP_DIR "/tmp/"
#endif

#define ISO_NORTH "northBL"
#define ISO_SOUTH "southBL"
#define ISO_EAST  "eastBL"
#define ISO_WEST  "westBL"


/*-@@@
File:		iso19115cd2.cxx
Version:	$Revision: 1.1 $
Description:	Class ISO19115CD2 - ISO 19115 XML metadata
Options:	-o fieldtype=<filename>
			assigns data types to field names
		-o filter=<command-line>
			defines a program to call to dynamically format 
                        output; may be overridden by the environment 
                        variable ISO19115_FILTER
		-o style=<base-filename>
			defines the base name for the style sheets;
			may be overridden by the environment variable
			ISO19115_STYLE
		-o dtd=path/filename
			defines the location of the DTD
			may be overridden by the environment variable
			ISO19115_DTD
Authors:   	Archibald Warnock (warnock@awcubed.com)
Copyright:	A/WWW Enterprises
@@@-*/

#include "iso19115cd2.hxx"

void ParserError(STRING fn, CHR *message);
extern DOUBLE GetNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag);

ISO19115CD2::ISO19115CD2 (PIDBOBJ DbParent): XMLBASE (DbParent)
{
  CHR *env_filter;
  STRING DTDstring;
  STRLIST StrList;

  CaseSensitive = true;
  XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<!DOCTYPE Metadata SYSTEM \"ISO_19115_CD2.dtd\">\n";

  // Now figure out where the DTD is actually located.
  env_filter = getenv("ISO19115_DTD");
  if (env_filter) {
    DTDstring = env_filter;
  } else {
    Db->GetDocTypeOptions(&StrList);
    StrList.GetValue("dtd", &DTDstring);
    if (DTDstring.GetLength() <= 0) {
      DTDstring = DEF_ISO19115_DTD;
    }
  }

  XML_Header.Cat(DTDstring);
  XML_Header.Cat("\">\n");

  delete [] env_filter;

  INT ret;

  // Create the cross-reference database
  if ((ret = db_create(&a_dbp, NULL, 0)) != 0) {
    fprintf(stderr, "db_create: %s\n", db_strerror(ret));
    a_dbp = (DB*)NULL;
    //    exit(1);
  }
#ifdef DEBUG
  else {
    fprintf(stderr, "ISO19115CD2::ISO19115CD2: Headline database created\n");
  }
#endif

}


bool 
ISO19115CD2::UsefulSearchField(const STRING& Field)
{
  STRING FieldName;
  FieldName=Field;
  FieldName.UpperCase();

  if (FieldName.Search("RESTITLE"))
    return true;
  else if (FieldName.Search("REFDATE"))
    return true;
  else if (FieldName.Search("IDABS"))
    return true;
  else if (FieldName.Search("RPORGNAME"))
    return true;
  else if (FieldName.Search("KEYWORD"))
    return true;
  else if (FieldName.Search("IDPURP"))
    return true;
  else if (FieldName.Search("WESTBL"))
    return true;
  else if (FieldName.Search("EASTBL"))
    return true;
  else if (FieldName.Search("NORTHBL"))
    return true;
  else if (FieldName.Search("SOUTHBL"))
    return true;
  else if (FieldName.Search("GEOBOX"))
    return true;
  else if (FieldName.Search("RESREFDATE"))
    return true;
  else if (FieldName.Search("PERIOD"))
    return true;
  else if (FieldName.Search("PROGCD"))
    return true;
  else if (FieldName.Search("EXTENT"))
    return true;
  else if (FieldName.Search("PRESFORMCD"))
    return true;
  else if (FieldName.Search("MDDATEST"))
    return true;
  else if (FieldName.Search("BEGIN"))
    return true;
  else if (FieldName.Search("END"))
    return true;
  else if (FieldName.Search("MDFILEID"))
    return true;
  else
    return false;
}


void 
ISO19115CD2::BeforeIndexing(STRING DbFileName) {
  CHR* name;
  INT ret;

  // Open the headline database for writing
  dbf_name = DbFileName;
  dbf_name.Cat(".xindex");

  name = dbf_name.NewCString();
  if (strlen(name) == 0) {
    fprintf(stderr,"No dbf name!\n");
    exit(1);
  }

  if ((ret = a_dbp->open(a_dbp, 
		       name,
		       NULL, 
		       DB_HASH, 
		       DB_CREATE, 
		       0664)) != 0) {
    fprintf(stderr, "db_open: %s\n", db_strerror(ret));
  }
#ifdef DEBUG
  else {
    a_dbp->sync(a_dbp,NULL);
    fprintf(stderr, "ISO19115CD2::BeforeIndexing: Headline database %s opened.\n",name);
  }
#endif

  delete [] name;
}


void 
ISO19115CD2::LoadFieldTable() {
  STRLIST StrList;
  STRING  FieldTypeFilename;
  STRING  sBuf;
  CHR    *b, *pBuf;
  STRING  Field_and_Type;

  Db->GetDocTypeOptions(&StrList);
  StrList.GetValue("FIELDTYPE", &FieldTypeFilename);

  if (!IsFile(FieldTypeFilename)) {
    CHR buf[255];
    cerr << "Specified fieldtype file was "
	 << "not found, but is required for this doctype." << endl;
    cerr << "Please enter a new filename: ";
    cin.getline(buf,sizeof(buf));
    FieldTypeFilename = buf;
    if (!IsFile(FieldTypeFilename)) {
      cerr << "Assuming all fields are text." << endl;
      cerr << "Make sure you use the correct doctype option:" << endl;
      cerr << endl;
      cerr << "    -o fieldtype=<filename>" << endl;
      return;
    }
  }

  sBuf.ReadFile(FieldTypeFilename);
  b = sBuf.NewCString();

  pBuf = strtok(b,"\n");

  do {
    Field_and_Type = pBuf;
    Field_and_Type.UpperCase();
#if defined(WIN32) || defined(CYGWIN)
	// We might have to clean up the line endings for text files in Windows
	if (!Field_and_Type.IsPrint()) {
		Field_and_Type.MakePrintable();
		Field_and_Type.Trim();
	}
#endif
    Db->FieldTypes.AddEntry(Field_and_Type);
  } while ( (pBuf = strtok((CHR*)NULL,"\n")) );

  delete [] b;
}


SRCH_DATE ISO19115CD2::ParseDate(const STRING& Buffer)
{
//  CHR *found;
//  CHR tmp[160];
  STRING Hold (Buffer);
  STRINGINDEX Start, End;

  *fStart = DATE_ERROR;
  *fEnd = *fStart;

  Hold.Replace("-","");
  // Check to see if we just got a single numeric value
  // - it might start with a digit, or one of a couple of characters
  // Single values map to trivial intervals (startdate=enddate)
  if (Hold.IsNumber()) {
    *fStart = Hold.GetFloat();
    *fEnd = *fStart;
    return;
  } else if (Hold.CaseEquals("present")) {
    *fStart = DATE_PRESENT;
    *fEnd = *fStart;
    return;
  } else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd = *fStart;
    return;
  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {
    *fStart = Hold.GetFloat();
    *fEnd = *fStart;
    return;
  }

  // It might contain a block of tags instead of a single date string,
  // so we'd better look for sensible tags.  If it's a single date, it'll
  // contains <refDate> (well, and <SNGDATE>, but we don't need that) and
  // if it's a range, it has to contain <begin> and <end>.
  //
  // Let's do the single date case first
  //  Hold.UpperCase();
  Start = Hold.Search("<refDate>");

  if (Start > 0) {              // Found it
    Start += strlen("<refDate>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</refDate");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[ISO19115CD2::ParseDate] <refDate> found, missing </refDate>" << endl;
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = DATE_PRESENT;
      return;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = *fStart;
      return;
    } else if (Hold.IsNumber()) {
      *fStart = Hold.GetFloat();
      *fEnd = *fStart;
      return;
    }
  }

  // Otherwise, it had better be a buffer containing an interval
  // If so, the dates will be tagged with <begdate> and <enddate>
  //  Hold.UpperCase();

  Start = Hold.Search("<begin/>");
  if (Start > 0)
    *fStart = DATE_UNKNOWN;

  else {
    Start = Hold.Search("<begin>");
  
    if (Start > 0) {                 // Found the opening tag
      Start += strlen("<begin>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</begin");
      if (End == 0) {                // but not the closing tag
	cerr << "[ISO19115CD2::ParseDate] <begin> found, missing </begin>" 
	     << endl;
	*fStart = DATE_ERROR;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")
	  || Hold.CaseEquals("9999")
	  || Hold.CaseEquals("999999")
	  || Hold.CaseEquals("99999999")) {
	*fStart = DATE_ERROR;
      } else if (Hold.CaseEquals("unknown")) {
	*fStart = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	*fStart = Hold.GetFloat();
      } else {
	cerr << "[ISO19115CD2::ParseDate] Didn't parse <begin>, value=" 
	     << Hold << endl;
	*fStart = DATE_ERROR;
      }
    }
  }

  // Copy the input buffer again so we can look for the enddate tag
  Hold = Buffer;

  Start = Hold.Search("<end/>");
  if (Start > 0)
    *fEnd = DATE_UNKNOWN;

  else {
    Start = Hold.Search("<end>");
    if (Start > 0) {
      Start += strlen("<end>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</end");
      if (End == 0) {
	cerr << "[ISO19115CD2::ParseDate] <end> found, missing </end>" 
	     << endl;
	*fEnd = DATE_ERROR;
	return;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")) {
	*fEnd = DATE_PRESENT;
      } else if (Hold.CaseEquals("unknown")) {
	*fEnd = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	*fEnd = Hold.GetFloat();
      } else {
	cerr << "[ISO19115CD2::ParseDate] Didn't parse <end>, value=" 
	     << Hold << endl;
	*fEnd = DATE_ERROR;
      }
    } else {
      *fEnd = DATE_ERROR;
    }
#ifdef DEBUG
    cerr << "Buffer: " << Buffer << endl
	 << "\t[" << (INT*)fStart << ", " << (INT*)fEnd << "]" << endl;
#endif
    //  } else {             // We're out of tags to look for...
    //    *fStart = DATE_ERROR;
    //    *fEnd = *fStart;
    //    return;
  }
  return;
}


void 
ISO19115CD2::ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
  CHR *Hold;
  Hold = Buffer.NewCString();
  ParseDate(Hold,fStart,fEnd);

  delete Hold;
  return;
}


void 
ISO19115CD2::ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
		DOUBLE* fEnd) {
//  CHR *found;
//  CHR tmp[160];
  STRING Hold;
  STRINGINDEX Start, End;
  SRCH_DATE dStart, dEnd;

  Hold = Buffer;
  Hold.Replace("-","");

  /*
  // Check to see if we just got a single numeric value
  // - it might start with a digit, or one of a couple of characters
  // Single values map to trivial intervals (startdate=enddate)
  if (Hold.IsNumber()) {
    //    *fStart = Hold.GetFloat();
    //    *fEnd = *fStart;
    dStart = Hold.GetFloat();
    dEnd   = dStart;
    if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
      dStart.PromoteToDayStart();
      dEnd.PromoteToDayEnd();
    }
    *fStart = dStart.GetValue();
    *fEnd   = dEnd.GetValue();
    return;
  } else if (Hold.CaseEquals("present")) {
    //    *fStart = DATE_PRESENT;
    //    *fEnd = *fStart;
    *fStart = DATE_UNKNOWN;
    *fEnd   = DATE_PRESENT;
    return;
  } else if (Hold.CaseEquals("unknown")) {
    *fStart = DATE_UNKNOWN;
    *fEnd   = *fStart;
    return;
  } else if (isdigit(Buffer[0]) || (Buffer[0] == '.') 
	     || (Buffer[0] == '+') || (Buffer[0] == '-')) {
    //    *fStart = Hold.GetFloat();
    //    *fEnd = *fStart;
    dStart = Hold.GetFloat();
    dEnd   = dStart;
    if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
      dStart.PromoteToDayStart();
      dEnd.PromoteToDayEnd();
    }
    *fStart = dStart.GetValue();
    *fEnd   = dEnd.GetValue();
    return;
  }
  */

  // It might contain a block of tags instead of a single date string,
  // so we'd better look for sensible tags.  If it's a single date, it'll
  // contains <refDate> (well, and <SNGDATE>, but we don't need that) and
  // if it's a range, it has to contain <begin> and <end>.
  //
  // Let's do the single date case first
  //  Hold.UpperCase();
  Start = Hold.Search("<refDate>");

  if (Start > 0) {              // Found it
    Start += strlen("<refDate>");
    Hold.EraseBefore(Start);
    End = Hold.Search("</refDate");

    if (End == 0) {            // But didn't find the ending tag
      cerr << "[ISO19115CD2::ParseDate] <refDate> found, missing </refDate>" << endl;
      *fStart = DATE_ERROR;
      *fEnd = *fStart;
      return;
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("present")
	|| Hold.CaseEquals("9999")
	|| Hold.CaseEquals("999999")
	|| Hold.CaseEquals("99999999")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = DATE_PRESENT;
      return;
    } else if (Hold.CaseEquals("unknown")) {
      *fStart = DATE_UNKNOWN;
      *fEnd = *fStart;
      return;
    } else if (Hold.IsNumber()) {
      //      *fStart = Hold.GetFloat();
      //      *fEnd = *fStart;
      dStart = Hold.GetFloat();
      dEnd   = dStart;
      if ((dStart.IsYearDate()) || dStart.IsMonthDate()) {
	dStart.PromoteToDayStart();
	dEnd.PromoteToDayEnd();
      }
      *fStart = dStart.GetValue();
      *fEnd   = dEnd.GetValue();
      return;
    }
  }

  // Otherwise, it had better be a buffer containing an interval
  // If so, the dates will be tagged with <begdate> and <enddate>
  //  Hold.UpperCase();

  Start = Hold.Search("<begin/>");
  if (Start > 0) {                 // Found the opening tag
    *fStart = DATE_UNKNOWN;
  } else {
    Start = Hold.Search("<begin>");
    if (Start > 0) {                 // Found the opening tag
      Start += strlen("<begin>");
      Hold.EraseBefore(Start);
      End = Hold.Search("</begin");
      if (End == 0) {                // but not the closing tag
	cerr << "[ISO19115CD2::ParseDate] <begin> found, missing </begin>" 
	     << endl;
	*fStart = DATE_ERROR;
      }
      Hold.EraseAfter(End-1);
      if (Hold.CaseEquals("present")
	  || Hold.CaseEquals("9999")
	  || Hold.CaseEquals("999999")
	  || Hold.CaseEquals("99999999")) {
	*fStart = DATE_ERROR;
      } else if (Hold.CaseEquals("unknown")) {
	*fStart = DATE_UNKNOWN;
      } else if (Hold.IsNumber()) {
	//      *fStart = Hold.GetFloat();
	dStart = Hold.GetFloat();
	if ((dStart.IsYearDate()) || dStart.IsMonthDate()) 
	  dStart.PromoteToDayStart();
	
	*fStart = dStart.GetValue();
	
      } else {
	cerr << "[ISO19115CD2::ParseDate] Didn't parse <begin>, value=" 
	     << Hold << endl;
	*fStart = DATE_ERROR;
      }
    }
  }

  // Copy the input buffer again so we can look for the enddate tag
  Hold = Buffer;

    Start = Hold.Search("<end/>");
    if (Start > 0)
      *fEnd = DATE_UNKNOWN;

    else {
      Start = Hold.Search("<end>");
      if (Start > 0) {
	Start += strlen("<end>");
	Hold.EraseBefore(Start);
	End = Hold.Search("</end");
	if (End == 0) {
	  cerr << "[ISO19115CD2::ParseDate] <end> found, missing </end>" 
	       << endl;
	  *fEnd = DATE_ERROR;
	  return;
	}
	Hold.EraseAfter(End-1);
	if (Hold.CaseEquals("present")) {
	  *fEnd = DATE_PRESENT;
	} else if (Hold.CaseEquals("unknown")) {
	  *fEnd = DATE_UNKNOWN;
	} else if (Hold.IsNumber()) {
	  //	*fEnd = Hold.GetFloat();
	  dEnd = Hold.GetFloat();
	  if ((dEnd.IsYearDate()) || dEnd.IsMonthDate()) {
	    dEnd.PromoteToDayEnd();
	  }
	  *fEnd   = dEnd.GetValue();
	} else {
	  cerr << "[ISO19115CD2::ParseDate] Didn't parse <end>, value=" 
	       << Hold << endl;
	  *fEnd = DATE_ERROR;
	}
#ifdef DEBUG
	cerr << "Buffer: " << Buffer << endl
	     << "\t[" << (INT)*fStart << ", " << (INT)*fEnd << "]" << endl;
#endif
      } else {
	*fEnd = DATE_ERROR;
      }
      //  } else {             // We're out of tags to look for...
      //    *fStart = DATE_ERROR;
      //    *fEnd = *fStart;
      //    return;
    }

    return;
}


// The ISO19115CD2 metadata records only has one computed field - the physical
// extent of the bounding box.  In general, this routine will walk through
// the list of computed fields desired, and call the routine that does the
// actual calculation
DOUBLE 
ISO19115CD2::ParseComputed(const STRING& FieldName, const CHR *Buffer)
{
  DOUBLE extent;
  if (FieldName.CaseEquals("EXTENT")) {
    ParseExtent(Buffer, &extent);
    return(extent);
  } else {
    return(0.0);
  }
}

/*
// Handy routine to make sure we always get back a numeric value where
// appropriate and where expected.  It correctly handles the (obviously)
// non-numeric value UNKNOWN by sending back a 0.
DOUBLE
GetNumericValue(const CHR* Buffer, const CHR* Tag, const CHR* eTag) {
  STRING Hold;
  STRINGINDEX Start, End;
  DOUBLE fValue;

  Hold = Buffer;
  Hold.UpperCase();
  Start = Hold.Search(Tag);

  if (Start > 0) {                 // Found the opening tag
    Start += strlen(Tag);
    Hold.EraseBefore(Start);
    End = Hold.Search(eTag);
    if (End == 0) {                // but not the closing tag
      return(0.0);
    }
    Hold.EraseAfter(End-1);
    if (Hold.CaseEquals("UNKNOWN")) {
      return(0.0);
    } else if (Hold.IsNumber()) {
      fValue = Hold.GetFloat();
      return(fValue);
    }
  }
  return(0.0);
}
*/

// Parses the buffer, looking for the coordinates of the corners of
// the bounding box, and returns them in the array Vertices.  The order
// is significant - it is the ISO19115CD2-prescribed ordered-pair syntax 
void
ISO19115CD2::ParseGPoly(const CHR *Buffer, DOUBLE Vertices[])
{

  DOUBLE North,South,East,West;
  DOUBLE Left;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<westBL>");
  strcpy(eTag,"</westBL>");
  West = GetNumericValue(Buffer,Tag,eTag);
  Vertices[0] = West;

  strcpy(Tag,"<northBL>");
  strcpy(eTag,"</northBL>");
  North = GetNumericValue(Buffer,Tag,eTag);
  Vertices[1] = North;

  strcpy(Tag,"<eastBL>");
  strcpy(eTag,"</eastBL>");
  East = GetNumericValue(Buffer,Tag,eTag);
  Vertices[2] = East;

  strcpy(Tag,"<southBL>");
  strcpy(eTag,"</southBL>");
  South = GetNumericValue(Buffer,Tag,eTag);
  Vertices[3] = South;

  //  sprintf(Vertices,"%10.5f,%10.5f %10.5f,%10.5f\n",West,North,East,South);
  
  return;
}


// Calculates the extent from the BOUNDING text buffer.  Right now, it is
// hideously inaccurate, because it assumes that the bounding box is an
// actual rectangle, rather than projected on the surface of a sphere, and 
// it does the calculations in decimal degrees instead of radians, but 
// that is what the GEO profile specifies right now.
void 
ISO19115CD2::ParseExtent(const CHR* Buffer, DOUBLE* extent)
{
  DOUBLE North,South,East,West;
  DOUBLE NewEast;
  CHR Tag[12];
  CHR eTag[12];

  strcpy(Tag,"<northBL>");
  strcpy(eTag,"</northBL>");
  North = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<southBL>");
  strcpy(eTag,"</southBL>");
  South = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<eastBL>");
  strcpy(eTag,"</eastBL>");
  East = GetNumericValue(Buffer,Tag,eTag);

  strcpy(Tag,"<westBL>");
  strcpy(eTag,"</westBL>");
  West = GetNumericValue(Buffer,Tag,eTag);

  if (East < West) {
    //    Left = -180 + West;
    NewEast = East + 360;
  } else {
    NewEast = East;
  }

  *extent = fabs((NewEast - West) * (North - South));

#ifdef DEBUG
  cerr 
    << "North=" << North
    << ", South=" << South
    << ", East=" <<East
    << ", West=" <<West
    << endl;
  cerr
    << "Extent " << *extent
    << " = (" << NewEast
    << " - " << West
    << ") * (" << North
    << " - " << South
    << ") = " << NewEast-West
    << " * " << North-South
    << endl;
#endif

  return;
}


void 
ISO19115CD2::ParseFields (RECORD* NewRecord, CHR *RecBuffer)
{
  FILE* fp;
  STRING fn;

  if (NewRecord == NULL) return; // Error

  // Open the file
  NewRecord->GetFullFileName (&fn);
  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  IS_size_t ActualLength=strlen(RecBuffer);

  STRING FieldName;
  STRING FullFieldname;

  FC fc, fc_full;
  DF df, df_full;
  DFD dfd, dfd_full;
  STRING doctype;
  NUMERICFLD nc, nc_full;

  FullFieldname = "";
  NewRecord->GetDocumentType(&doctype);

  CHR **tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL) {
    cerr << "Unable to parse `" << doctype << "' tags in file " << fn << "\n";
    return;
  }


  GSTACK Nested;
  IS_size_t LastEnd=(IS_size_t)0;
  DFT* pdft = new DFT ();

  bool InCustom;
  IS_size_t val_start;
  int val_len;
  IS_size_t val_end;
  IS_size_t tag_end;

  InCustom = false;
  for (CHR **tags_ptr = tags; *tags_ptr; tags_ptr++) {
    XML_Element *pTmp;
    if ((*tags_ptr)[0] == '/') {

      STRING Tag;
      STRINGINDEX x;

      Tag = *tags_ptr;
      x=Tag.Search('/');
      Tag.EraseBefore(x+1);

      // We keep a stack of the fields we have currently open.  This
      // handles nested fields by making a long field name out of the
      // nested values.
      pTmp = (XML_Element*)Nested.Top();
      if (Tag == pTmp->get_tag()) {
	pTmp = (XML_Element*)Nested.Pop();
	// cerr << "Popped " << pTmp->get_tag() << " off the stack.  ";
	delete pTmp;
	if (Nested.GetSize() != 0) {
	  pTmp = (XML_Element*)Nested.Top();
	  // cerr << "Still inside " << pTmp->get_tag() << ".\n";
	  x = FullFieldname.SearchReverse('|');
	  FullFieldname.EraseAfter(x-1);
	  // cerr << "Full fieldname is now " << FullFieldname << ".\n";
	}
      }
    } 

    if ((*tags_ptr)[0] == '/') // closing tag
      continue;

    tag_end = strlen(*tags_ptr);
    if ((*tags_ptr)[tag_end-1] == '/') // empty tag
      continue;

    const CHR *p = find_end_tag (tags_ptr, *tags_ptr);
    IS_size_t tag_len = strlen (*tags_ptr);
    int have_attribute_val = (NULL != strchr (*tags_ptr, '='));
    
    if (p != NULL) {
      // We have a tag pair
      IS_size_t val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
      int val_len = (p - *tags_ptr) - tag_len - 2;

      // Skip leading white space
      while (isspace (RecBuffer[val_start]))
	val_start++, val_len--;
      // Leave off trailing white space
      while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	val_len--;
      
      // Don't bother storing empty fields
      if (val_len > 0) {
	// Cut the complex values from field name
	CHR orig_char = 0;
	XML_Element *pTag = new XML_Element();
	char* tcp;

	for (tcp = *tags_ptr; *tcp; tcp++)
	  if (isspace (*tcp)) {
	    orig_char = *tcp;
	    *tcp = '\0';
	    break;
	  }

	if (*tags_ptr[0] == '\0') {
	  // Tag name is the name of the last element
	  if (FieldName.GetLength () == 0) {
	    // Give some information
	    cerr << doctype << " Warning: \""
		 << fn << "\" offset " << (*tags_ptr - RecBuffer) << ": "
		 << "Bad use of empty tag feature, skipping field.\n";
	    continue;
	  }
	} else {
	  FieldName = *tags_ptr;
	}

	//	cerr << "Got tag: " << FieldName << endl;
	// Probably ought to check useful field here...

	if (orig_char)
	  *tcp = orig_char;

	val_end = val_start + val_len - 1;
	  
	pTag->set_tag(FieldName);
	pTag->set_start(val_start);
	pTag->set_end(val_end);
	
	if (Nested.GetSize() != 0) {
	  XML_Element *pTmp;
	  if (val_start < LastEnd) {
	    pTmp = (XML_Element*)Nested.Top();
	  }
	}
	if (FullFieldname.GetLength() > 0)
	  FullFieldname.Cat("|");
	FullFieldname.Cat(FieldName);
	if (!(FullFieldname.IsPrint())) {
	  FullFieldname.MakePrintable();
	  cerr << "Non-ascii characters found in " << FullFieldname << endl;
	}

	STRING FieldType;

	if (UsefulSearchField(FieldName)) {
	  STRING FieldNameU = FieldName;
	  FieldNameU.UpperCase();
	  Db->FieldTypes.GetValue(FieldNameU, &FieldType);
	
	  if (FieldType.Equals(""))
	    FieldType = "text";
#ifdef DEBUG
	  else
	    cerr << "Got type " << FieldType << " for field " << FieldName << endl;

	  cerr << "Found field name " << FieldName << endl;
#endif

	  dfd.SetFieldName (FieldName);
	  dfd.SetFieldType (FieldType);
	  Db->DfdtAddEntry (dfd);
	  fc.SetFieldStart (val_start);
	  fc.SetFieldEnd (val_start + val_len - 1);
	  FCT *pfct = new FCT ();
	  pfct->AddEntry (fc);
	  df.SetFct (*pfct);
	  df.SetFieldName (FieldName);
	  //	dfd.SetFieldType (FieldType); Duplicate code???
	  pdft->AddEntry (df);
	  delete pfct;

	  // Now, do the same for the long fieldname
	  STRING NewType;
	  NewType = FullFieldname;
	  NewType.Cat("=");
	  NewType.Cat(FieldType);
	  NewType.UpperCase();
	  Db->FieldTypes.AddEntry(NewType);

#ifdef DEBUG
	  cerr << "full field name " << FullFieldname << endl;
#endif

	  dfd_full.SetFieldName (FullFieldname);
	  dfd_full.SetFieldType (FieldType);

	  Db->DfdtAddEntry (dfd_full);
	  fc_full.SetFieldStart (val_start);
	  fc_full.SetFieldEnd (val_end);
	  FCT *pfct1 = new FCT ();
	  pfct1->AddEntry (fc_full);
	  df_full.SetFct (*pfct1);
	  df_full.SetFieldName (FullFieldname);
	  pdft->AddEntry (df_full);
	  delete pfct1;
	}
	Nested.Push(pTag);
	LastEnd = val_end;
      }
    }
    if (have_attribute_val) {
      store_attributes (pdft, RecBuffer, *tags_ptr);
    } else if (p == NULL) {
#if 1
      // Give some information
      cerr << doctype << " Warning: \""
	   << fn << "\" offset " << (*tags_ptr - RecBuffer) << ": "
	   << "No end tag for <" << *tags_ptr << "> found, skipping field.\n";
#endif
    }
  }

  NewRecord->SetDft (*pdft);

  // Clean up;
  delete tags;
  delete pdft;
}


void 
ISO19115CD2::Present (const RESULT& ResultRecord, const STRING& ElementSet, 
		      STRING* StringBufferPtr)
{
  STRING RecordSyntax = SutrsRecordSyntax;
  Present(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}


void 
ISO19115CD2::Present (const RESULT& ResultRecord, 
		      const STRING& ElementSet, 
		      const STRING& RecordSyntax, 
		      STRING* StringBufferPtr)
{
  STRING ESet = ElementSet;
  if (ElementSet.Equals("B")) {
    
    if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      PresentBriefXml(ResultRecord,StringBufferPtr);

    } else if (RecordSyntax.CaseEquals(IxmlRecordSyntax)) {
      PresentIsearchBriefXml(ResultRecord,StringBufferPtr);

    } else {
      PresentBriefSutrs(ResultRecord,StringBufferPtr);
    }
    return;
  
  } else if (ElementSet.Equals("S")) { 
    //    if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      PresentSummaryXml(ResultRecord,StringBufferPtr);
      //    }

  } else if (ElementSet.Equals("F")) { 
    if (RecordSyntax.CaseEquals(XmlRecordSyntax)) {
      PresentFullXml(ResultRecord,StringBufferPtr);

    } else if (RecordSyntax.CaseEquals(HtmlRecordSyntax)) {
      PresentFullHtml(ResultRecord,StringBufferPtr);

    } else {
      PresentFullSutrs(ResultRecord,StringBufferPtr);
    }
  }
}


void
ISO19115CD2::PresentBriefXml(const RESULT& ResultRecord, 
			STRING *StringBufferPtr) 
{
  // This is the brief record XML encoding for ISO 19115 metadata
  STRLIST     Strlist;
  STRING      Key, TitleTag, Title, EntryIDTag, EntryID;
  bool Status;

  ResultRecord.GetKey(&Key);
  TitleTag = "METADATA|DATAIDINFO|IDCITATION|RESTITLE";
  Status = Db->GetFieldData(ResultRecord, TitleTag, &Strlist);
  if (Status) {
    Strlist.Join("",&Title);
  } else {
    Title = "No Title";
  }

  EntryIDTag = "METADATA|MDFILEID";
  Status = Db->GetFieldData(ResultRecord, EntryIDTag, &Strlist);
  if (Status) {
    Strlist.Join("",&EntryID);
  } else {
    EntryID = "";
  }

  Title.Trim();

  *StringBufferPtr = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  StringBufferPtr->Cat("<!DOCTYPE Metadata SYSTEM \"ISO_19115_b.dtd\">\n");
  StringBufferPtr->Cat("<Metadata>\n");
  StringBufferPtr->Cat("  <mdFileID>");  
  StringBufferPtr->Cat(EntryID);  
  StringBufferPtr->Cat("</mdFileID>\n");
  StringBufferPtr->Cat("  <dataIdInfo>\n");  
  StringBufferPtr->Cat("    <idCitation>\n");  
  StringBufferPtr->Cat("      <resTitle>");  
  StringBufferPtr->Cat(Title);
  StringBufferPtr->Cat("</resTitle>\n");  
  StringBufferPtr->Cat("    </idCitation>\n");  
  StringBufferPtr->Cat("  </dataIdInfo>\n");  
  StringBufferPtr->Cat("</Metadata>\n");  
  return;
}


void
ISO19115CD2::PresentIsearchBriefXml(const RESULT& ResultRecord, 
			STRING *StringBufferPtr) 
{
  // This is the Isearch XML encoding for zsearch/zpresent
  STRING Path,File,Hold,Key,DocSource;
  STRING      HoldPath;
  STRINGINDEX colon,dot;
  STRLIST Strlist1,Strlist2;
  STRING TitleTag, Title, EntryIDTag, EntryID;
  bool Status1,Status2;
  INT ndb;
  CHR ndb_string[8];

  ResultRecord.GetKey(&Key);
  TitleTag = "METADATA|DATAIDINFO|IDCITATION|RESTITLE";
  Status1 = Db->GetFieldData(ResultRecord, TitleTag, &Strlist1);
  if (Status1) {
    Strlist1.Join("",&DocSource);
  }
  //  DocSource = "STScI"; // replace this with <namespace>

  TitleTag = "METADATA|DATAIDINFO|IDCITATION|RESTITLE";
  Status2 = Db->GetFieldData(ResultRecord, TitleTag, &Strlist2);
  if (Status2) {
    Strlist2.Join("",&Title);
  } else {
    Title = "No Title";
  }

  ResultRecord.GetPath(&Path);
  ResultRecord.GetFileName(&File);
  ndb = ResultRecord.GetDbNum();
  sprintf(ndb_string,"%d",ndb);

  Path.Cat(File);
  *StringBufferPtr = "\t\t\t<isearch:result docid=\"";
  if (ndb > 0) {
    StringBufferPtr->Cat(ndb_string);
    StringBufferPtr->Cat(':');
  }
  
  Title.XmlCleanup();

  StringBufferPtr->Cat(Key);
  StringBufferPtr->Cat("\" status=\"OK\"");
  if (DocSource.GetLength() > 0) {
    StringBufferPtr->Cat(" source=\"");
    StringBufferPtr->Cat(DocSource);
  }
  StringBufferPtr->Cat("\">\n");
  StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"title\" name=\"Title\">");
  StringBufferPtr->Cat(Title);
  StringBufferPtr->Cat("</isearch:field>\n");
  StringBufferPtr->Cat("\t\t\t\t<isearch:field type=\"documenturl\" name=\"documenturl\">\n");
  StringBufferPtr->Cat("\t\t\t\t\t<xlink type=\"link\" ref=\"");
  StringBufferPtr->Cat(Path);
  StringBufferPtr->Cat("\" />\n");
  StringBufferPtr->Cat("\t\t\t\t</isearch:field>\n");
  StringBufferPtr->Cat("\t\t\t</isearch:result>\n");
  return;
}


void
ISO19115CD2::PresentBriefSutrs(const RESULT& ResultRecord, STRING *StringBufferPtr)
{
  STRLIST Strlist1,Strlist2,Strlist3;
  STRING TitleTag, Title, EntryIDTag, EntryID;
  STRING ParentTag, ParentEntryID;
  bool Status1,Status2,Status3;

  TitleTag = "METADATA|DATAIDINFO|IDCITATION|RESTITLE";
  Status1 = Db->GetFieldData(ResultRecord, TitleTag, &Strlist1);
  if (Status1) {
    Strlist1.Join("",&Title);
  } else {
    Title = "No Title";
  }
  *StringBufferPtr = Title;
}


// S: includes the following elements: 
//   Title (4 - title) = METADATA|DATAIDINFO|IDCITATION|RESTITLE
//   Edition (3815 - edition, not in GEO spec) = 
//   Geospatial Data Presentation Form (3805 - geoform) = 
//   Beginning Date (2072 - begtime) = METADATA|DATAIDINFO|DATAEXT|TEMPELE|TEMPEXTENT|EXTEMP|TM_GEOMETRICPRIMITIVE|TM_PERIOD|BEGIN
//   Ending Date (2073 - enddate) = METADATA|DATAIDINFO|DATAEXT|TEMPELE|TEMPEXTENT|EXTEMP|TM_GEOMETRICPRIMITIVE|TM_PERIOD|END
//   Maintenance_and_Update_Frequency (3109 - update, not in GEO) = METADATA|MDMAINT|MAINTFREQ
//   Bounding Coordinates (2060 - bounding) = METADATA|DATAIDINFO|GEOBOX
//   including:
// METADATA|DATAIDINFO|GEOBOX|WESTBL
// METADATA|DATAIDINFO|GEOBOX|EASTBL
// METADATA|DATAIDINFO|GEOBOX|NORTHBL
// METADATA|DATAIDINFO|GEOBOX|SOUTHBL

//   Browse Graphic (3137 - browse) = ?
//
//
// These are defined in the GEO profile as part of the S element set but not
// currently used by the DTD developed by Blue Angel
//   Publication Date (31 - pubdate) = METADATA|DATAIDINFO|IDCITATION|RESREFDATE|REFDATE
//   Indirect Spatial Reference (3301 - indspref)
//   Online Linkage (2021 - onlink) = ?
//   Extent (3148 - extent) = EXTENT (computed)
//   Entity Type Label (3503 - enttypl) = ?
//   Attribute Label (3507 - attrlabl) = ?
//   Data Set G-Polygon (3116 - dsgpoly) = ?
//
// The Browse Graphic (3137 - browse) should appear as groups of Browse
// Graphic File Name (3138 - browsen), Browse Graphic File Description
// (3139 - browsed), and Browse Graphic File Type (3140 - browset).

void
ISO19115CD2::PresentSummaryXml(const RESULT& ResultRecord, 
			STRING *StringBufferPtr) 
{
  // This is the brief record XML encoding for ISO 19115 metadata
  STRLIST     Strlist;
  STRING      Key, Element, Value, FieldType;
  bool Status;
  STRING XML_Header = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n";
  STRING SGML_Header = "<!DOCTYPE METADATA PUBLIC \"-//FGDC//DTD METADATA 2.0//EN\">\n";
  ResultRecord.GetKey(&Key);

  Element = "METADATA|DATAIDINFO|IDCITATION|RESTITLE";
  Status = Db->GetFieldData(ResultRecord, Element, &Strlist);
  if (Status) {
    Strlist.Join("",&Value);
  } else {
    Value = "No Title";
  }
  Value.Trim();

  *StringBufferPtr = XML_Header;
  StringBufferPtr->Cat("<metadata>\n");
  StringBufferPtr->Cat("  <idinfo>\n");
  StringBufferPtr->Cat("    <citation>\n");
  StringBufferPtr->Cat("      <citeinfo>\n");

  StringBufferPtr->Cat("        <title>");  
  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat("</title>\n");

  //  StringBufferPtr->Cat("        <edition>");  
  //  StringBufferPtr->Cat("        </edition>\n");

  //  StringBufferPtr->Cat("        <geoform>");  
  //  StringBufferPtr->Cat("        </geoform>");  

  StringBufferPtr->Cat("      </citeinfo>\n");
  StringBufferPtr->Cat("    </citation>\n");

  StringBufferPtr->Cat("    <timeperd>\n");
  StringBufferPtr->Cat("      <timeinfo>\n");
  StringBufferPtr->Cat("        <rngdates>\n");

  Value = "";
  STRING Dates, Begin, End;
  STRINGINDEX n;
  //  Element = "METADATA|DATAIDINFO|DATAEXT|TEMPELE|TEMPEXTENT|EXTEMP|TM_GEOMETRICPRIMITIVE|TM_PERIOD|BEGIN";
  Element = "METADATA|DATAIDINFO|DATAEXT|TEMPELE|TEMPEXTENT|EXTEMP|TM_GEOMETRICPRIMITIVE|TM_PERIOD";
  Db->FieldTypes.GetValue(Element,&FieldType);
  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &Dates);
  if (!(Status)) {
    Begin = "";
    End = "";
  } else {
    Begin = Dates;
    n = Begin.Search(",");
    if (n>0)
      Begin.EraseAfter(n-1);
    End = Dates;
    n = End.Search(",");
    if (n>0)
      End.EraseBefore(n+1);
  }
  StringBufferPtr->Cat("          <begdate>");  
  //  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat(Begin);
  StringBufferPtr->Cat("</begdate>\n");  

  //  Value = "";
  //  Element = "METADATA|DATAIDINFO|DATAEXT|TEMPELE|TEMPEXTENT|EXTEMP|TM_GEOMETRICPRIMITIVE|TM_PERIOD|END";
  //  Db->FieldTypes.GetValue(Element,&FieldType);
  //  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &End);
  //  if (!(Status)) {
  //    Value = "";
  //  }
  StringBufferPtr->Cat("          <enddate>");  
  //  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat(End);
  StringBufferPtr->Cat("</enddate>\n");  

  StringBufferPtr->Cat("        </rngdates>\n");
  StringBufferPtr->Cat("      </timeinfo>\n");
  StringBufferPtr->Cat("    </timeperd>\n");

  StringBufferPtr->Cat("    <status>\n");
  //  StringBufferPtr->Cat("      <update>\n");
  //  StringBufferPtr->Cat("      </update>\n");
  StringBufferPtr->Cat("    </status>\n");

  StringBufferPtr->Cat("    <spdom>\n");
  StringBufferPtr->Cat("      <bounding>\n");

  Value = "";
  Element = "METADATA|DATAIDINFO|GEOBOX|WESTBL";
  Db->FieldTypes.GetValue(Element,&FieldType);
  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &Value);
  if (!(Status)) {
    Value = "";
  }
  StringBufferPtr->Cat("        <westbc>");
  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat("</westbc>\n");

  Value = "";
  Element = "METADATA|DATAIDINFO|GEOBOX|EASTBL";
  Db->FieldTypes.GetValue(Element,&FieldType);
  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &Value);
  if (!(Status)) {
    Value = "";
  }
  StringBufferPtr->Cat("        <eastbc>");
  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat("</eastbc>\n");

  Value = "";
  Element = "METADATA|DATAIDINFO|GEOBOX|NORTHBL";
  Db->FieldTypes.GetValue(Element,&FieldType);
  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &Value);
  if (!(Status)) {
    Value = "";
  }
  StringBufferPtr->Cat("        <northbc>");
  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat("</northbc>\n");

  Value = "";
  Element = "METADATA|DATAIDINFO|GEOBOX|SOUTHBL";
  Db->FieldTypes.GetValue(Element,&FieldType);
  Status = Db->GetFieldData(ResultRecord, Element, FieldType, &Value);
  if (!(Status)) {
    Value = "";
  }
  StringBufferPtr->Cat("        <southbc>");
  StringBufferPtr->Cat(Value);
  StringBufferPtr->Cat("</southbc>\n");
  StringBufferPtr->Cat("      </bounding>\n");
  StringBufferPtr->Cat("    </spdom>\n");

  StringBufferPtr->Cat("  <browse>\n");
  //  StringBufferPtr->Cat("    <browsen>\n");
  //  StringBufferPtr->Cat("    </browsen>\n");
  StringBufferPtr->Cat("  </browse>\n");

  StringBufferPtr->Cat("  </idinfo>\n");
  StringBufferPtr->Cat("</metadata>\n\n");

  return;
}


void
ISO19115CD2::PresentFullXml(const RESULT& ResultRecord, 
			STRING *StringBufferPtr)
{
  ResultRecord.GetRecordData(StringBufferPtr);
  return;
}


void
ISO19115CD2::PresentFullHtml(const RESULT& ResultRecord, 
			    STRING *StringBufferPtr)
{
  STRLIST StrList;
  STRING filterCommand, StyleSheet;
  STRING s_cmd;
  STRING OrigDataBuffer, DataBuffer, TmpBuffer;
  STRINGINDEX n;
  CHR MyTmpIn[]="/tmp/iso19115_inXXXXXX";
  CHR *env_filter, *env_style;
  INT in, out, status;
  
  ResultRecord.GetRecordData(StringBufferPtr);

  // Right now, we need to erase the line pointing to the DTD because we
  // do not want the filter program trying to retrieve it and doing any 
  // validation.  So, we erase everything before the <Metadata> tag.
  n = StringBufferPtr->Search("<Metadata>");
  StringBufferPtr->EraseBefore(n-1);
  TmpBuffer = XML_Header;
  TmpBuffer.Cat(*StringBufferPtr);
  *StringBufferPtr = TmpBuffer;

  if ((in = mkstemp(MyTmpIn)) < 0)
    return;

  //  if ( (status = putenv("LD_LIBRARY_PATH=/usr/local/src/xml-xalan/c/lib:/usr/local/src/xerces-c1_6_0-linux/lib")) != 0) {
  //    fprintf(stderr, "Failed to add environment variable\n");
  //  }

  // Set up the command line for calling the transformation program
  env_filter = getenv("ISO19115_FILTER");
  if (env_filter) {
    filterCommand = env_filter;
  } else {
    Db->GetDocTypeOptions(&StrList);
    StrList.GetValue("filter", &filterCommand);
    if (filterCommand.GetLength() <= 0) {
      filterCommand = DEF_ISO19115_FILTER;
    }
  }

  // Set up the command line parameter for the style sheet
  env_style = getenv("ISO19115_STYLE");
  if (env_style) {
    StyleSheet = env_style;
  } else {
    StrList.GetValue("style", &StyleSheet);
    if (StyleSheet.GetLength() <= 0) {
      StyleSheet = DEF_ISO19115_STYLE;
    }
  }
  StyleSheet.Cat(FULL_HTML_XSL);

  // Dump the XML buffer to the temporary input file
  StringBufferPtr->WriteFile(MyTmpIn);

  // Build the command line
  s_cmd = filterCommand;
  s_cmd.Cat(' ');
  s_cmd.Cat(MyTmpIn);
  s_cmd.Cat(' ');
  s_cmd.Cat(StyleSheet);
  //  s_cmd.Cat(' ');
  //  s_cmd.Cat(MyTmpOut);

#ifdef DEBUG
  cerr << "Command: " << s_cmd << endl;
#endif
  // Try to read back from stdout, ala Stevenson
  FILE *fpin;
  CHR line[10240];

  if ( (fpin = popen(s_cmd,"r")) == NULL) {
      DataBuffer = "popen error";
      return;
  }

  status = 0;
  DataBuffer = "";
  for ( ; ; ) {
    if (fgets(line, 10240, fpin) == NULL) {
      break;
    }
    status++;
    DataBuffer.Cat(line);
  }

  if (pclose(fpin) == -1) {
    fprintf(stderr,"pclose error");
  }

  // Clean up the temp files
  unlink(MyTmpIn);

  // Pass the transformed data back
  if (DataBuffer.GetLength() > 0) {
    *StringBufferPtr = DataBuffer;
  } else {
    sprintf(line,"Read %d lines\n",status);
    *StringBufferPtr = line;
  }
  return;
}

/* Simplified version - just returns XML */
void
ISO19115CD2::PresentFullSutrs(const RESULT& ResultRecord, 
			 STRING *StringBufferPtr)
{
  ResultRecord.GetRecordData(StringBufferPtr);
  return;
}

/* Full version
void
ISO19115CD2::PresentFullSutrs(const RESULT& ResultRecord, 
			 STRING *StringBufferPtr)
{
  STRLIST StrList;
  STRING filterCommand, StyleSheet;
  STRING s_cmd;
  STRING OrigDataBuffer, DataBuffer, TmpBuffer;
  STRINGINDEX n;
  CHR MyTmpIn[]="/tmp/iso19115_inXXXXXX";
  CHR *env_filter, *env_style;
  INT in, out, status;
  
  ResultRecord.GetRecordData(StringBufferPtr);

  // Right now, we need to erase the line pointing to the DTD because we
  // do not want the filter program trying to retrieve it and doing any 
  // validation.  So, we erase everything before the <Metadata> tag.
  n = StringBufferPtr->Search("<Metadata>");
  StringBufferPtr->EraseBefore(n-1);
  TmpBuffer = XML_Header;
  TmpBuffer.Cat(*StringBufferPtr);
  *StringBufferPtr = TmpBuffer;

  if ((in = mkstemp(MyTmpIn)) < 0)
    return;

  //  if ( (status = putenv("LD_LIBRARY_PATH=/usr/local/src/xml-xalan/c/lib:/usr/local/src/xerces-c1_6_0-linux/lib")) != 0) {
  //    fprintf(stderr, "Failed to add environment variable\n");
  //  }

  // Set up the command line for calling the transformation program
  env_filter = getenv("ISO19115_FILTER");
  if (env_filter) {
    filterCommand = env_filter;
  } else {
    Db->GetDocTypeOptions(&StrList);
    StrList.GetValue("filter", &filterCommand);
    if (filterCommand.GetLength() <= 0) {
      filterCommand = DEF_ISO19115_FILTER;
    }
  }

  // Set up the command line parameter for the style sheet
  env_style = getenv("ISO19115_STYLE");
  if (env_style) {
    StyleSheet = env_style;
  } else {
    StrList.GetValue("style", &StyleSheet);
    if (StyleSheet.GetLength() <= 0) {
      StyleSheet = DEF_ISO19115_STYLE;
    }
  }
  StyleSheet.Cat(FULL_SUTRS_XSL);

  // Dump the XML buffer to the temporary input file
  StringBufferPtr->WriteFile(MyTmpIn);

  // Build the command line
  s_cmd = filterCommand;
  s_cmd.Cat(' ');
  s_cmd.Cat(MyTmpIn);
  s_cmd.Cat(' ');
  s_cmd.Cat(StyleSheet);
  //  s_cmd.Cat(' ');
  //  s_cmd.Cat(MyTmpOut);

#ifdef DEBUG
  cerr << "Command: " << s_cmd << endl;
#endif
  // Try to read back from stdout, ala Stevenson
  FILE *fpin;
  CHR line[10240];

  if ( (fpin = popen(s_cmd,"r")) == NULL) {
      DataBuffer = "popen error";
      return;
  }

  status = 0;
  DataBuffer = "";
  for ( ; ; ) {
    if (fgets(line, 10240, fpin) == NULL) {
      break;
    }
    status++;
    DataBuffer.Cat(line);
  }

  if (pclose(fpin) == -1) {
    fprintf(stderr,"pclose error");
  }

  // Clean up the temp files
  unlink(MyTmpIn);

  // Pass the transformed data back
  if (DataBuffer.GetLength() > 0) {
    *StringBufferPtr = DataBuffer;
  } else {
    sprintf(line,"Read %d lines\n",status);
    *StringBufferPtr = line;
  }
  return;
}
*/

ISO19115CD2::~ISO19115CD2 ()
{
}
