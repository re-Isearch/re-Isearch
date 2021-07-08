/*-@@@
File:		filter.cxx
Version:	0.01
Description:	Class FILTERDOC - Binary files
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.hxx"
#include "filter.hxx"
#include "process.hxx"
#include "doc_conf.hxx"

#pragma ident  "@(#)filter.cxx  1.11 04/20/01 20:10:17 BSN"

const char *FILTERDOC::GetDefaultClass() const
{
  return "XFILTER:DOCTYPE";
}
const char *FILTERDOC::GetDefaultFilter() const
{
  return "";
}


void FILTERDOC::SourceMIMEContent(STRING *stringPtr) const
{
  DOCTYPE::SourceMIMEContent(stringPtr);
}



FILTERDOC::FILTERDOC (PIDBOBJ DbParent, const STRING& Name) :
        DOCTYPE (DbParent, Name)
{
  HostID = _IB_Hostid(); 

  STRING s (ResolveBinPath(Getoption("FILTER", GetDefaultFilter())));
  if (s.GetLength() && (s != "NULL"))
    {
      Filter = ResolveBinPath(s);
      if (!IsAbsoluteFilePath(Filter))
	{
	  logf (LOG_WARN, "%s: Specified filter '%s' not found. Check Installation.",
		Doctype.c_str(), Filter.c_str()); 
	}
      else if (!ExeExists(Filter))
	{
	  logf (LOG_ERROR, "%s: Filter '%s' %s!", Doctype.c_str(), Filter.c_str(),
	    FileExists(Filter) ?  "is not executable" : "does not exist");
	  Filter.Clear();
	}
      else
	logf (LOG_DEBUG, "%s: External filter set to '%s'", Doctype.c_str(), Filter.c_str());
    }
  else
    Filter = s;
  Classname= Getoption("CLASS", GetDefaultClass());
  if (Doctype ^= Classname)
    {
      logf (LOG_PANIC, "%s: You should NEVER set class as recursive: CLASS=%s",
	Doctype.c_str(), Classname.c_str());
      Classname.Clear();
    }
}

FILTERDOC::~FILTERDOC ()
{
}


const char *FILTERDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("XFILTER");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  DOCTYPE::Description(List);
  return "Plug-in external filter for input\n\
Options:\n\
   FILTER   Specifies the program to use\n\
   CLASS    Specifies which doctype class to use to handle the\n\
            filtered records\n\
Note:\n\
   Filters must take single arguments of the form:\n\
      filter filename\n\
   and write to stdout.\n\
   For output one should also define External/ filters (see DOCTYPE class)\n";
}

void FILTERDOC::BeforeIndexing() {}
void FILTERDOC::AfterIndexing() {}

void FILTERDOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.IsEmpty() || Classname.IsEmpty())
    return;
  if (Filter == "NULL")
    {
      DOCTYPE::ParseRecords(FileRecord);
      return;
    }
  STRING key, s, outfile;
  unsigned long version = 0;

  const STRING Fn = FileRecord.GetFullFileName ();

  INODE Inode (Fn);
  if (!Inode.Ok())
    {
      if (Inode.isDangling())
        logf(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
      else
        logf(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
  if (Inode.st_size == 0)
    {
      logf(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", Fn.c_str());
      return;
    }
  logf (LOG_DEBUG, "%s: Input = '%s'", Doctype.c_str(), Fn.c_str());

  off_t start = FileRecord.GetRecordStart();
  off_t end   = FileRecord.GetRecordEnd();

  key = Inode.Key(start, end);
  while (Db->KeyLookup (key))
    key.form("%s.%ld", s.c_str(), ++version); 
  // Now we have a good key

  logf (LOG_DEBUG, "Key set to '%s'", key.c_str());

  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1)
    {
      logf (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return;
    }
  outfile.form ("%s/%s", s.c_str(), key.c_str());
  // So we pipe Fn bytes Start to End into outfile

  logf (LOG_DEBUG, "Output to '%s'", outfile.c_str());

  MMAP mapping (Fn, start, end, MapSequential);
  if (!mapping.Ok())
    {
       logf(LOG_FATAL|LOG_ERRNO, "Couldn't map '%s' into memory", Fn.c_str());
       return;
    }
  const UCHR *Buffer  = (const UCHR *)mapping.Ptr();
  const size_t MemSize = mapping.Size();

  char tmpfile[L_tmpnam+1];

  char *tempfile = tmpnam(tmpfile);
  FILE *fp = fopen(tempfile, "wb");
  if (fp == NULL)
    {
      logf (LOG_ERRNO, "Could not create temporarily file stream '%s'", tempfile);
      return;
    }

  errno = 0;
  size_t length = fwrite(Buffer, sizeof(char), MemSize, fp);
 
  mapping.Unmap(); // Clear map
  fclose(fp);
  if (length < MemSize)
     logf (LOG_WARN|LOG_ERRNO, "Temp file '%s' short write by %d bytes", tempfile,
	(int)(MemSize-length));

  if ((fp = fopen(outfile, "w")) == NULL)
   {
     logf (LOG_ERRNO, "%s: Could not create '%s'", outfile.c_str());
     UnlinkFile(tempfile);
     return;
   }

  STRING pipe;
  pipe << Filter << " " << tempfile;

  FILE *pp = _IB_popen(pipe, "r");
  if (pp == NULL)
    {
      logf (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), pipe.c_str());
      fclose(fp);
      UnlinkFile(tempfile);
      UnlinkFile(outfile);

      if (!IsAbsoluteFilePath (Filter))
	{
	  logf (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
		Doctype.c_str(), Filter.c_str());
	  Filter.Clear();
	}
      return;
    }

  int ch;
  off_t  len = 0;
  while ((ch = fgetc(pp)) != EOF)
    {
      fputc(ch, fp);
      len++;
    }
  _IB_pclose(pp);
  fclose(fp);
  UnlinkFile(tempfile);

  RECORD NewRecord(FileRecord);
  // We now have a record in outfile from 0 to len
  NewRecord.SetRecordStart (0);
  NewRecord.SetRecordEnd ( len - 1 );
  NewRecord.SetFullFileName ( outfile );
  NewRecord.SetDocumentType ( Classname );
  NewRecord.SetKey( key ); // Set the key since we did the effort

  // Set some default dates
  SRCH_DATE mod_filter, mod_input;
  if (mod_filter.SetTimeOfFile(Filter) && mod_input.SetTimeOfFile(Fn) && mod_filter > mod_input)
    NewRecord.SetDateModified ( mod_filter );
  NewRecord.SetDate ( mod_input ); 

  logf (LOG_INFO, "%s: %s in '%s'(%ld)", Doctype.c_str(), Classname.c_str(),
	outfile.c_str(), len);
  Db->DocTypeAddRecord(NewRecord);
}

void FILTERDOC::ParseFields(RECORD *RecordPtr)
{
  ; // This class should never ParseFields!!
}
