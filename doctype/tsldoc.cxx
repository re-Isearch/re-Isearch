#pragma ident  "@(#)tsldoc.cxx  1.12 02/24/01 17:45:21 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		csldoc.cxx
Version:	$Revision: 1.1 $
Description:	Class TSLDOC - TSL Tab Sep. List Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "tsldoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

TSLDOC::TSLDOC (PIDBOBJ DbParent, const STRING& Name):
	COLONDOC (DbParent, Name)
{
  STRING Value =  Getoption("UseFirstRecord", "Auto"); 
  if (Value == "Auto")
    UseFirstRecord = -1;
  else
    UseFirstRecord = Value.GetBool() ? 1 : 0;
  Lineno = 0;
  Columns = 0;
  IFS = "\t\r\n";
}

const char *TSVDOC::Description(PSTRLIST List) const
{
  TSLDOC::Description(List);
  return "Tab delimited lists where the first line defines the field names.\n\
  Uses TSLDOC services but has the \"UseFirstRecord\" option set by default to True\n\
  and not auto to force its use.";
}

const char *TSLDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("TSLDOC");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  COLONDOC::Description(List);
  return "TSLDOC is a base class for Tab Separated List (Tab delimited) files:\n\
 - Each line is a record\n\
 - Each line contain fields separated from each other by TAB characters\n\
   (horizontal tab, HT, Ascii control code 9).\n\
 - \"Field\" means here just any string of characters, excluding TABs.\n\
 -  Each line must contain the same number of fields.\n\
 - The first line may contain the names for the fields (on all lines), i.e.\n\
   column headers.\n\
\n\
 [General]\n\
 TabFields=Comma separated list of field names (e.g. field1,field2,field3)\n\
   alternatively in the doctype.ini to define the \"column\" field names.\n\
   If the option \"UseFirstRecord\" is specified (True) it is used\n\
   as the fieldnames.\n\
   If no TabFields are specified and UseFirstRecord was not specified\n\
   True of False then UseFirstRecord will be assumed as True.\n\
 CategoryField=<Field contains an integer for record category>\n\
 PriorityField=<Field contains an integer to skew scores for record>\n\
   For priority to have an effect one must define in \"Db.ini\"'s\n\
   [DbInfo] PriorityFactor=NN.NN (a floating point number)\n\
Alternatively in the \"Db.ini\" file [Doctype] section";
}


void TSLDOC::SourceMIMEContent(const RESULT&, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void TSLDOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  if (UseFirstRecord)
    {
      *StringBufferPtr = "text/tab-separated-values";
    }
  else
    {
      *StringBufferPtr = "Application/X-TSL-";
      StringBufferPtr->Cat(Doctype);
    }
}

void TSLDOC::AddFieldDefs ()
{
  static const STRING Entry ("TabFields");
  DOCTYPE::AddFieldDefs ();
  STRING string;
  if (Db)
    Db->ProfileGetString(Doctype, Entry, NulString, &string);
  if (string.IsEmpty())
    {
      if (tagRegistry)
	tagRegistry->ProfileGetString("General", Entry, NulString, &string);
    }
  if (!string.IsEmpty())
    {
      STRLIST List;
      List.Split(',', string);
      DFD dfd;
      Columns = 0;
      for (const STRLIST *p = List.Next(); p != &List; p = p->Next())
	{
	  Columns++;
	  message_log (LOG_DEBUG, "%s Column #%d is \"%s\"", Doctype.c_str(), Columns, p->Value().c_str());
	  FieldNames.Add(p->Value());

	  dfd.SetFieldType( Db->GetFieldType(p->Value()) ); // Get the type added 30 Sep 2003
	  dfd.SetFieldName (p->Value());
	  Db->DfdtAddEntry (dfd);
	}
      if (UseFirstRecord == -1)
	UseFirstRecord = 0;
    }
  else if (UseFirstRecord == -1)
    UseFirstRecord = 1;
}

void TSLDOC::ParseRecords(const RECORD& FileRecord)
{
#if 1
  FileName = FileRecord.GetFullFileName();
  FileMap.CreateMap(FileName, MapSequential);
  if (!FileMap.Ok())
    {
      message_log (LOG_ERRNO, "%s could not access '%s'", Doctype.c_str(), FileName.c_str());
      return;			// File not accessed
    }
  off_t RecStart = FileRecord.GetRecordStart();
  off_t RecEnd = FileRecord.GetRecordEnd();
  const off_t FileEnd = FileMap.Size();

  if (RecEnd == 0) RecEnd = FileEnd;

  if (RecEnd - RecStart <= 0)
    {
      message_log (LOG_WARN, "zero-length record '%s'[%ld-%ld] -- skipping",
	FileName.c_str(), (long)RecStart, (long)RecEnd);
      return;
    }
  else if (RecStart > FileEnd)
    {
      message_log (LOG_ERROR, "%s::ParseRecords(): Seek '%s' to %ld failed",
	Doctype.c_str(), FileName.c_str(), RecStart);
      return;
    }
  else if (RecEnd > FileEnd)
    {
      message_log (LOG_WARN, "%s::ParseRecord(): End after EOF (%d>%d) in '%s'?",
	Doctype.c_str(), RecEnd, FileEnd, FileName.c_str());
      RecEnd = FileEnd;
    }
  RECORD Record (FileRecord);
  off_t Position = RecStart;
  off_t Pos = 0;
  
  unsigned char  ci = 0;
  const unsigned char *ptr = FileMap.Ptr();
  STRING keyBase = Record.GetKey();

  long count = 0;
  if (keyBase.IsEmpty())
    Db->GetMainMdt()->GetUniqueKey(&keyBase);
  STRING key;
  while (Position < RecEnd)
    {
      for (; (ci != '\n') && Position <= RecEnd; ci=*ptr++, Position++)
	/* loop */;
      if (RecStart != Position)
	{
	  Record.SetRecordStart(RecStart);
	  if (Position != FileEnd) 
	    Pos = Position-1;
	  else
	    Pos = Position-2;
	  Record.SetRecordEnd(Pos);
	  key = keyBase;
	  key << ':' << ++count;
	  Record.SetKey (key);
	  
	  Db->DocTypeAddRecord(Record);
	}
      RecStart=Position;
      if (ci=='\n')
	ci=0;// save an EOF, but hide a newline so it will loop again
    }
#else
  STRING Fn (FileRecord.GetFullFileName());
  PFILE fp = Db->ffopen (Fn, "rb");
  if (!fp)
    {
      message_log (LOG_ERRNO, "Could not access '%s'", Fn.c_str());
      return;			// File not accessed
    }

  off_t RecStart = FileRecord.GetRecordStart();
  off_t RecEnd = FileRecord.GetRecordEnd();

  if (RecEnd == 0)
    RecEnd = GetFileSize (fp);

  if (RecEnd - RecStart <= 0)
    {
      message_log (LOG_WARN, "zero-length record '%s'[%ld-%ld] -- skipping",
	Fn.c_str(), (long)RecStart, (long)RecEnd);
      Db->ffclose (fp);
      return;
    }
  if (fseek (fp, RecStart, 0) == -1)
    {
      message_log (LOG_ERRNO, "%s::ParseRecords(): Seek '%s' to %ld failed",
	Doctype.c_str(), Fn.c_str(), RecStart);
      Db->ffclose (fp);
      return;
    }
  RECORD Record (FileRecord);
  off_t Position = RecStart;
  off_t Pos = 0;
  
  int ci = 0;
  while (ci != EOF && Position < RecEnd)
    {
      for (; (ci != '\n') && (ci != EOF) && Position <= RecEnd; ci=fgetc(fp), Position++)
	/* loop */;
      if (RecStart != Position)
	{
	  Record.SetRecordStart(RecStart);
	  if (ci != EOF) 
	    Pos = Position-1;
	  else
	    Pos = Position-2;
	  Record.SetRecordEnd(Pos);
	  Db->DocTypeAddRecord(Record);
	}
      RecStart=Position;
      if (ci=='\n')
	ci=0;// save an EOF, but hide a newline so it will loop again
    }
  Db->ffclose(fp);
#endif
}


#if 1
/*
Modified strtrok to not skip over all delimiters
*/

static char * my_strtok(char *str, const char *delims)
{
    static char *pos = NULL;
    char        *start = NULL;

    if (str)    /* Start a new string? */
	pos = str;

    if (pos) {
#if 0 /* ORIGINAL STRTROK */
	/* Skip delimiter */
	while (*pos && strchr(delims, *pos))
	    pos++;
#else
	while (*pos == '\r' || *pos == '\n') pos++; // Skip line stuff
#endif
	if (*pos) {
	    start = pos;
	    /* Skip non-delimiters */
	    while (*pos && !strchr(delims, *pos))
		pos++;
	    if (*pos)
		*pos++ = '\0';
	}
    }

    return start;
}

#endif

void TSLDOC::ParseFields (PRECORD NewRecord)
{
#if 1
  const STRING fn ( NewRecord->GetFullFileName ());
  if (fn != FileName)
    {
      FileMap.CreateMap(fn, MapSequential);
      if (!FileMap.Ok())
	{
	  message_log (LOG_ERROR, "%s:ParseFields Could not address file '%s'", Doctype.c_str(), fn.c_str());
	  return;
	}
      Lineno = 0;
    }
  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  const off_t FileEnd = FileMap.Size();
  if (RecEnd == 0)
    RecEnd = FileEnd; 
  if (RecEnd - RecStart <= 0)
    return; // ERROR
  else if (RecStart > FileEnd)
    return; // ERROR
  else if (RecEnd > FileEnd)
    RecEnd = FileEnd;

  off_t RecLength = RecEnd - RecStart + 1 ;
  PCHR RecBuffer = (PCHR)Buffer.Want(RecLength + 1);
  memcpy(RecBuffer, FileMap.Ptr()+RecStart, RecLength);
  RecBuffer[RecLength] = '\0';
  char *tcp = my_strtok(RecBuffer, IFS);

  if (RecStart == 0 || Columns == 0)
    {
      if (RecStart == 0)
	Lineno = 1;
      else if (Lineno > 0)
	Lineno++;
      if (UseFirstRecord == 1)
	{
	  FieldNames.Empty();
	  DFD dfd;
	  Columns = 0;
	  if (tcp)
	    {
	      if (*tcp)
		do {
		  Columns++;
		  FieldNames.Add( *tcp ? STRING(tcp) : STRING().form("Field%d", Columns) );
		} while ((tcp = my_strtok(NULL, IFS)) != NULL);
	      // We don't want to handle 1 column tables
	      if (Columns <= 1)
		{
		  message_log (LOG_WARN, "%s: Skipping line #%llu (%d columns)?",
			Doctype.c_str(), (long long)Lineno, Columns);
		  FieldNames.Empty();
		  NewRecord->SetBadRecord();
		}
	      else
		{
		  for (size_t i=0; i < Columns; i++)
		    {
		      STRING t (FieldNames[i]);
		      message_log (LOG_DEBUG, "%s Column #%d is \"%s\"", Doctype.c_str(), i, t.c_str());
		      dfd.SetFieldType( Db->GetFieldType(t) ); // Get the type added 30 Sep 2003
		      dfd.SetFieldName (t);
		      Db->DfdtAddEntry (dfd);
		    }
		}
	    }
	  return; // Done this
	}
    }
  else if (Lineno > 0)
    Lineno++;

  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  int i = 0;
  size_t val_start, val_end;
  STRING fieldName;
  if (tcp) do {
    if (*tcp == '\0' || *tcp == IFS[0])
      {
	i++;
	message_log (LOG_WARN, "%s line: %lu, column %d (%s), is empty.",
		fn.c_str(), (unsigned long)Lineno,
		i, 
		i >= FieldNames.Count() ? "" : FieldNames[i-1].c_str());
	continue;
      }
    if (i >= FieldNames.Count())
      {
	if (RecStart != 0)
	  {
	    if (Lineno > 0)
	      message_log (LOG_WARN, "File '%s', line %llu (%ld-%ld), has irregular number of columns (%d), adjusting.",
		fn.c_str(), (long long)Lineno, RecStart, RecEnd, i+1);
	    else
	      message_log (LOG_WARN, "File '%s' (%ld-%ld) has irregular number of columns (%d), adjusting.",
                fn.c_str(), RecStart, RecEnd, i+1);
	  }
	else
	  Columns++;
	fieldName.form("Field%d", ++i);
	FieldNames.Add(fieldName);
	dfd.SetFieldType( Db->GetFieldType(fieldName) ); // Get the type added 30 Sep 2003
	dfd.SetFieldName (fieldName);
	Db->DfdtAddEntry (dfd);
      }
    else
      {
	if (i > Columns && RecStart != 0)
	  {
	    if (Lineno > 0)
	     message_log (LOG_WARN, "File '%s', line %llu (%ld-%ld), has more than %ld columns (%d).",
		fn.c_str(), (long long)Lineno, RecStart, RecEnd, Columns, i);
	    else
	      message_log (LOG_WARN, "File '%s' (%ld-%ld) has more than %d columns (%d).",
		fn.c_str(), RecStart, RecEnd, Columns, i);
	  }
        fieldName = FieldNames[i++];
      }
    val_start = tcp - RecBuffer;
    val_end   = val_start + strlen(tcp);

    while (strchr(IFS, RecBuffer[val_end]))
	val_end--; // Trim end 2004 August

    df.SetFct ( FC(val_start, val_end) );
    df.SetFieldName (fieldName);
    pdft->AddEntry (df);
    if (IsSpecialField(fieldName))
      HandleSpecialFields(NewRecord, fieldName, tcp);
  } while ((tcp = my_strtok(NULL, IFS)) != NULL); 
  NewRecord->SetDft (*pdft);

  delete pdft;
#else
  const STRING fn ( NewRecord->GetFullFileName ());

  PFILE fp = Db->ffopen(fn, "rb");
  if (fp == NULL)
    {
      return;
    }
#if 0
  // Start off with the date set to the date of file...
  SRCH_DATE Datum(fp);
  if (Datum.Ok())
    {
      NewRecord->SetDate( Datum );
      NewRecord->SetDateModified (Datum);
      Datum.Clear(); // Now clear it!  Will set to other when it gets set.
    }
#endif

  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp); 
  if (RecEnd <= RecStart)
    {
      Db->ffclose(fp);
      return;		// ERROR
    }
  off_t RecLength = RecEnd - RecStart+1;
  PCHR RecBuffer = (PCHR)Buffer.Want(RecLength + 1);
  off_t ActualLength = pfread (fp, RecBuffer, RecLength, RecStart);
  if (ActualLength != RecLength)
    message_log (LOG_WARN, "Short record. Expected %ld bytes, got %ld.", RecLength, ActualLength);
  Db->ffclose (fp);
  RecBuffer[ActualLength] = '\0';
  char *tcp = strtok(RecBuffer, IFS);

  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  int i = 0;
  size_t val_start, val_end;
  STRING fieldName;
  do {
    if (*tcp == '\0' || *tcp == IFS[0])
      {
	i++;
	continue;
      }
    if (i >= FieldNames.Count())
      {
	fieldName.form("Field%d", ++i);
	dfd.SetFieldType( Db->GetFieldType(fieldName) ); // Get the type added 30 Sep 2003
	dfd.SetFieldName (fieldName);
	Db->DfdtAddEntry (dfd);
      }
    else
      fieldName = FieldNames[i++];
    val_start = tcp - RecBuffer;
    val_end   = val_start + strlen(tcp);
    df.SetFct ( FC(val_start, val_end) );
    df.SetFieldName (fieldName);
    pdft->AddEntry (df);
    if (IsSpecialField(fieldName))
      HandleSpecialFields(NewRecord, fieldName, tcp);
  } while ((tcp = strtok(NULL, IFS)) != NULL); 
  NewRecord->SetDft (*pdft);
  // if (Datum.Ok()) NewRecord->SetDate (Datum);

  delete pdft;
#endif
}

void TSLDOC::AfterIndexing()
{
  FileMap.Unmap();
}

TSLDOC::~TSLDOC ()
{
}

