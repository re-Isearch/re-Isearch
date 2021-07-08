#pragma ident  "@(#)ozsearch.cxx  1.7 04/20/01 14:23:54 BSN"

/************************************************************************
************************************************************************/

/*-@@@
File:		ozsearch.cxx
Version:	$Revision: 1.1 $
Description:	Class OZSEARCH - TSL Tab Sep. List Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

#include "common.hxx"
#include "doc_conf.hxx"
#include "ozsearch.hxx"

OZSEARCH::OZSEARCH (PIDBOBJ DbParent, const STRING& Name):
	TSLDOC (DbParent, Name)
{
}

const char *OZSEARCH::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("OZSEARCH");

  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  TSLDOC::Description(List);
  return "OZSEARCH is a class for Tab Separated List files as used by the\n\
Austalian Search service OZSEARCH (http://www.ozsearch.com.au)";
}


void OZSEARCH::SourceMIMEContent(const RESULT&, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void OZSEARCH::SourceMIMEContent(PSTRING StringBufferPtr) const
{ 
  *StringBufferPtr = "Application/X-OZSEARCH-";
  StringBufferPtr->Cat(Doctype);
}


void OZSEARCH::ParseFields (PRECORD NewRecord)
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
  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  const GPTYPE FileEnd = FileMap.Size();
  if (RecEnd == 0)
    RecEnd = FileEnd; 
  if (RecEnd - RecStart <= 0)
    return; // ERROR
  else if (RecStart > FileEnd)
    return; // ERROR
  else if (RecEnd > FileEnd)
    RecEnd = FileEnd;

  GPTYPE RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)Buffer.Want(RecLength + 1);
  memcpy(RecBuffer, FileMap.Ptr()+RecStart, RecLength);
  RecBuffer[RecLength] = '\0';
  const char *IFS = "\t\r\n";
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
    if (*tcp == '\0' || *tcp == '\t')
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
	while (*tcp && !isdigit(*tcp)) tcp++;
	if (*tcp) NewRecord->SetCategory(tcp);
      }
    else if (fieldName ^= PriorityField)
      {
	while (*tcp && !isdigit(*tcp)) tcp++;
	if (*tcp) NewRecord->SetPriority(tcp);
      }
  } while ((tcp = strtok(NULL, IFS)) != NULL); 
  NewRecord->SetDft (*pdft);

  delete pdft;
}


OZSEARCH::~OZSEARCH ()
{
}

