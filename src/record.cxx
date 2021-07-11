#pragma ident  "@(#)record.cxx  1.37"


/************************************************************************
************************************************************************/

/*-@@@
File:		record.cxx
Version:	1.01
Description:	Class RECORD - Database Record
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include "common.hxx"
#include "magic.hxx"
#include "lang-codes.hxx"
#include <sys/stat.h>

#include "record.hxx"

#define EXPAND_FILE_SPEC(x) ExpandFileSpec(x)


RECORD::RECORD ()
{
  RecordStart = 0;
  RecordEnd   = 0;
  Priority    = 0;
  Category    = 0;
  Segment     = 0;
}

RECORD::RECORD (const STRING& path)
{
  if (!path.IsEmpty())
    SetFullFileName(path);
  RecordStart = 0;
  RecordEnd   = 0;
  Priority    = 0;
  Category    = 0;
  Segment     = 0;
}


RECORD::RECORD (const STRING& PathName, const STRING& FileName)
{
  if (PathName.IsEmpty() || !FileName.IsEmpty())
    SetFullFileName(AddTrailingSlash(PathName)+FileName);
  RecordStart = 0;
  RecordEnd   = 0;
  Priority    = 0;
  Category    = 0;
  Segment     = 0;
}



// Added this creator to make life easier for
// ParseRecords functions
RECORD::RECORD (const RECORD& OtherRecord)
{
  *this = OtherRecord;
}


off_t RECORD::GetLength () const
{
  off_t end = RecordEnd;
  if (end == 0)
    {
      if ((end = GetFileSize(Pathname)) > 0)
	end--;
      else if (RecordStart == 0)
	return 0;
    }
  if (end > 0)
    return end-RecordStart+1;
  return -1; // ERROR
}


RECORD& RECORD::Clear()
{
  static RECORD NulRecord;
  return (*this = NulRecord);
}


RECORD& RECORD::operator =(const RECORD& OtherRecord)
{
  Key          = OtherRecord.Key;
  Pathname     = OtherRecord.Pathname;
  origPathname = OtherRecord.origPathname;
  baseDir      = OtherRecord.baseDir;
  RecordStart  = OtherRecord.RecordStart;
  RecordEnd    = OtherRecord.RecordEnd;
  Dft          = OtherRecord.Dft;
  DocumentType = OtherRecord.DocumentType;
  Date         = OtherRecord.Date;
  DateModified = OtherRecord.DateModified;
  DateCreated  = OtherRecord.DateCreated;
  DateExpires  = OtherRecord.DateExpires;
  Locale       = OtherRecord.Locale;
  Priority     = OtherRecord.Priority;
  Category     = OtherRecord.Category;
  Segment      = OtherRecord.Segment;

  return *this;
}

void RECORD::RelativizePathnames(const STRING& nDir)
{
// cerr << "*****************   RelativizePathanees called... with " << nDir << endl;
  if (baseDir.GetLength())
    {
      if (!Pathname.IsAbsoluteFilePath())
	Pathname.Prepend(baseDir);
      if (!origPathname.IsAbsoluteFilePath())
	origPathname.Prepend(baseDir);
    }
  if ((baseDir = nDir).GetLength())
    {
      if (! IsRootDirectory(baseDir))
	{
	  if (Pathname.IsAbsoluteFilePath())
	    Pathname.RelativizePathname(baseDir);
	  if (origPathname.IsAbsoluteFilePath())
	    origPathname.RelativizePathname(baseDir);
	}
      else baseDir = NulString;
    }
}


PATHNAME RECORD::RelativizePathname(const STRING& RootDir) {
  return (Pathname.RelativizePathname(RootDir));
}

PATHNAME RECORD::RelativizeOrigPathname(const STRING& RootDir) {
//cerr << "Relativize Origal.." << endl;
  return (origPathname.RelativizePathname(RootDir));
}

void RECORD::SetPath (const STRING& newPathName)
{
  Pathname.SetPath(newPathName);
}

STRING RECORD::GetPath() const
{
  return Pathname.GetPath();
}


void    RECORD::SetFileName(const STRING& newName)
{
  Pathname.SetFileName(newName);
}


STRING  RECORD::GetFileName() const
{
  return Pathname.GetFileName();
}

void RECORD::SetFullFileName (const STRING& FullName)
{
  PATHNAME newPathname (FullName);

  if (newPathname != Pathname)
    {
      struct stat stbuf;

      Pathname = newPathname;
      if (stat(FullName, &stbuf) != -1)
	{
	  DateModified.Set(&stbuf.st_mtime);
#if defined(_WIN32) || defined(LINUX)
	  time_t created = stbuf.st_ctime;
#else
#define _min(_x,_y) ((_x < _y) ? (_x) : (_y))
	  time_t created = _min(stbuf.st_birthtime, _min(stbuf.st_ctime, stbuf.st_mtime));
#undef _min
#endif
	  DateCreated.Set( &created);
	  Date = DateModified;
	}
    }
}

static BYTE bMarker[2] =  {'X', 'z'};

void RECORD::Write (PFILE fp) const
{
  if (fp == NULL) return;
  putObjID(objRECORD, fp);
  ::Write (Locale, fp);
  ::Write (Key, fp);
  ::Write (Pathname, fp);
  ::Write (origPathname, fp);
  ::Write (RecordStart, fp);
  ::Write (RecordEnd, fp);
  ::Write (DocumentType, fp);
  ::Write (Dft, fp);
  ::Write (Date, fp);
  ::Write (DateModified, fp);
  ::Write (DateCreated, fp);
  ::Write (DateExpires, fp);
  ::Write (Category, fp);
  ::Write (Priority, fp);
  ::Write (Segment, fp);
  ::Write (bMarker[ IsBadRecord() ? 1 : 0], fp);
}

GDT_BOOLEAN RECORD::Read (PFILE fp)
{
  obj_t obj = getObjID (fp);
  if (obj != objRECORD)
    {
      logf (LOG_DEBUG, "Record:Read() failed: Not a record (id=%d!=%d)",
	(int)obj, (int)objRECORD);
      PushBackObjID (obj, fp);
      return GDT_FALSE;
    }
  ::Read (&Locale, fp);
  ::Read (&Key, fp);
  ::Read (&Pathname, fp);
  ::Read (&origPathname, fp);

  ::Read (&RecordStart, fp);
  ::Read (&RecordEnd, fp);
  ::Read (&DocumentType, fp);
  ::Read (&Dft, fp);
  ::Read (&Date, fp);
  ::Read (&DateModified, fp);
  ::Read (&DateCreated, fp);
  ::Read (&DateExpires, fp);

  ::Read (&Category, fp);
  ::Read (&Priority, fp);
  ::Read (&Segment, fp);
  BYTE b;
  ::Read(&b, fp);
  if ((b != bMarker[IsBadRecord() ? 1 : 0 ])) {
    logf (LOG_ERROR, "Record fastload corrupt! // check=%c(%d)", (char)b, (int)b);
    return GDT_FALSE;
  }
  return GDT_TRUE;
}

RECORD::~RECORD ()
{
}



// Time To Live
int RECORD::TTL() const
{
  return TTL(SRCH_DATE((const CHR*)NULL));
}

  
int RECORD::TTL(const SRCH_DATE& Now) const
{
  if (DateExpires.Ok())
    {
      if (DateExpires <= Now)
        return 0;
      return DateExpires.MinutesDiff ( Now );
    }
  return -1;
}
