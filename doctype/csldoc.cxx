/************************************************************************
************************************************************************/

/*-@@@
File:		csldoc.cxx
Version:	$Revision: 1.1 $
Description:	Class CSLDOC - CSL Tab Sep. List Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "tsldoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

CSLDOC::CSLDOC (PIDBOBJ DbParent, const STRING& Name):
	COLONDOC (DbParent, Name)
{
  STRING Value =  Getoption("UseFirstRecord", "Auto"); 
  if (Value == "Auto")
    UseFirstRecord = -1;
  else
    UseFirstRecord = Value.GetBool() ? 1 : 0;
  Lineno = 0;
  Columns = 0;
}

const char *CSLDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("CSLDOC");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  COLONDOC::Description(List);
  return "CSLDOC is a base class for ; Separated List files: Each line is a\n\
record and each field in the record is separted by ; (semi-colon).\n\
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


void CSLDOC::SourceMIMEContent(const RESULT&, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void CSLDOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  if (UseFirstRecord)
    {
      *StringBufferPtr = "text/c-separated-values";
    }
  else
    {
      *StringBufferPtr = "Application/X-CSL-";
      StringBufferPtr->Cat(Doctype);
    }
}

void CSLDOC::AddFieldDefs ()
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
	  logf (LOG_DEBUG, "%s Column #%d is \"%s\"", Doctype.c_str(), Columns, p->Value().c_str());
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

void CSLDOC::ParseRecords(const RECORD& FileRecord)
{
  FileName = FileRecord.GetFullFileName();
  FileMap.CreateMap(FileName, MapSequential);
  if (!FileMap.Ok())
    {
      logf (LOG_ERRNO, "%s could not access '%s'", Doctype.c_str(), FileName.c_str());
      return;			// File not accessed
    }
  off_t RecStart = FileRecord.GetRecordStart();
  off_t RecEnd = FileRecord.GetRecordEnd();
  const off_t FileEnd = FileMap.Size();

  if (RecEnd == 0) RecEnd = FileEnd;

  if (RecEnd - RecStart <= 0)
    {
      logf (LOG_WARN, "zero-length record '%s'[%ld-%ld] -- skipping",
	FileName.c_str(), (long)RecStart, (long)RecEnd);
      return;
    }
  else if (RecStart > FileEnd)
    {
      logf (LOG_ERROR, "%s::ParseRecords(): Seek '%s' to %ld failed",
	Doctype.c_str(), FileName.c_str(), RecStart);
      return;
    }
  else if (RecEnd > FileEnd)
    {
      logf (LOG_WARN, "%s::ParseRecord(): End after EOF (%d>%d) in '%s'?",
	Doctype.c_str(), RecEnd, FileEnd, FileName.c_str());
      RecEnd = FileEnd;
    }
  RECORD Record (FileRecord);
  off_t Position = RecStart;
  off_t Pos = 0;
  
  unsigned char  ci = 0;
  const unsigned char *ptr = FileMap.Ptr();
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
	  Db->DocTypeAddRecord(Record);
	}
      RecStart=Position;
      if (ci=='\n')
	ci=0;// save an EOF, but hide a newline so it will loop again
    }
}


void CSLDOC::ParseFields (PRECORD NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName ());
  if (fn != FileName)
    {
      FileMap.CreateMap(fn, MapSequential);
      if (!FileMap.Ok())
	{
	  logf (LOG_ERROR, "%s:ParseFields Could not address file '%s'", Doctype.c_str(), fn.c_str());
	  return;
	}
      Lineno = 0;
    }
  off_t RecStart = NewRecord->GetRecordStart ();
  off_T RecEnd = NewRecord->GetRecordEnd ();
  const off_T FileEnd = FileMap.Size();
  if (RecEnd == 0)
    RecEnd = FileEnd; 
  if (RecEnd - RecStart <= 0)
    return; // ERROR
  else if (RecStart > FileEnd)
    return; // ERROR
  else if (RecEnd > FileEnd)
    RecEnd = FileEnd;

  off_t RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)Buffer.Want(RecLength + 1);
  memcpy(RecBuffer, FileMap.Ptr()+RecStart, RecLength);
  RecBuffer[RecLength] = '\0';
  const char *IFS = ";\r\n";
  char *tcp = strtok(RecBuffer, IFS);

  if (RecStart == 0)
    {
      Lineno = 1;
      if (UseFirstRecord == 1)
	{
	  FieldNames.Empty();
	  DFD dfd;
	  Columns = 0;
	  if (tcp) do {
	    Columns++;
	    logf (LOG_DEBUG, "%s Column #%d is \"%s\"", Doctype.c_str(), Columns, tcp);
	    FieldNames.Add(tcp);

	    dfd.SetFieldType( Db->GetFieldType(tcp) ); // Get the type added 30 Sep 2003
	    dfd.SetFieldName (tcp);
	    Db->DfdtAddEntry (dfd);
	  } while ((tcp = strtok(NULL, IFS)) != NULL);
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
    if (*tcp == '\0' || *tcp == ';')
      {
	i++;
	continue;
      }
    if (i >= FieldNames.Count())
      {
	if (RecStart != 0)
	  {
	    if (Lineno > 0)
	      logf (LOG_WARN, "File '%s', line %ld (%ld-%ld), has irregular number of columns (%d), adjusting.",
		fn.c_str(), Lineno, RecStart, RecEnd, i+1);
	    else
	      logf (LOG_WARN, "File '%s' (%ld-%ld) has irregular number of columns (%d), adjusting.",
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
	     logf (LOG_WARN, "File '%s', line %ld (%ld-%ld), has more than %ld columns (%d).",
		fn.c_str(), Lineno, RecStart, RecEnd, Columns, i);
	    else
	      logf (LOG_WARN, "File '%s' (%ld-%ld) has more than %d columns (%d).",
		fn.c_str(), RecStart, RecEnd, Columns, i);
	  }
        fieldName = FieldNames[i++];
      }
    val_start = tcp - RecBuffer;
    val_end   = val_start + strlen(tcp);
    df.SetFct ( FC(val_start, val_end) );
    df.SetFieldName (fieldName);
    pdft->AddEntry (df);
    if (fieldName ^= KeyField)
      {
	if (Db->KeyLookup (tcp))
	  {
	    logf (LOG_ERROR, "%s record in \"%s\" uses a non-unique %s '%s'",
		Doctype.c_str(), fn.c_str(), fieldName.c_str(), tcp);
	  }
	else
	  NewRecord->SetKey (tcp);
      }
    else if (fieldName ^= DateField)
      {
	SRCH_DATE Datum = tcp;
	if (Datum.Ok())
	  NewRecord->SetDate (Datum);
      }
    else if (fieldName ^= LanguageField)
      {
	// Only if valid do we over-ride
	SHORT code = Lang2Id (tcp);
	if (code != 0) NewRecord->SetLanguage (code);
      }
    else if (fieldName ^= CategoryField)
      {
	NewRecord->SetCategory(tcp);
      }
    else if (fieldName ^= PriorityField)
      {
	NewRecord->SetPriority(tcp);
      }
  } while ((tcp = strtok(NULL, IFS)) != NULL); 
  NewRecord->SetDft (*pdft);

  delete pdft;
}

void CSLDOC::AfterIndexing()
{
  FileMap.Unmap();
}

CSLDOC::~CSLDOC ()
{
}

