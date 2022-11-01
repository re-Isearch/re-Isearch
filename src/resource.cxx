/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "common.hxx"
#include "magic.hxx"
#include "resource.hxx"
#include "record.hxx"
#include <sys/stat.h>


RESOURCE::RESOURCE()
{
  Clear();
}

RESOURCE::RESOURCE(const RECORD& Record)
{
  Start   = Record.GetRecordStart();
  End     = Record.GetRecordEnd();
  Doctype = Record.GetDocumentType();
  Locale  = Record.GetLocale();
 
  SetFullFileName (Record.GetFullFileName());
}

RESOURCE::RESOURCE(const STRING& Path)
{
  Start = 0;
  End   = 0;
  Inode = 0;
  Locale= GlobalLocale;
  SetFullFileName (Path);
}

void RESOURCE::Clear()
{
  Start = 0;
  End   = 0;
  Inode = 0;
  Locale = GlobalLocale;
  FullPath.Clear();
  MimeType.Clear();
  Doctype.Clear();
}

void  RESOURCE::SetFullFileName (const STRING& Path)
{
  if (Path == FullPath) return; // Already set
  FullPath = Path;
  if (Start == 0 && End == 0)
    {
      struct stat st_buf;
      if (_IB_stat(FullPath, &st_buf) == 0)
	{
	  Inode = st_buf.st_ino;
	  Start = 0;
	  End   = st_buf.st_size;
	}
    }
}


RESOURCE::RESOURCE(const RESOURCE& Other)
{
 *this = Other;
}

RESOURCE& RESOURCE::operator =(const RESOURCE& Other)
{
  Start    = Other.Start;
  End      = Other.End;
  Inode    = Other.Inode;
  FullPath = Other.FullPath;
  MimeType = Other.MimeType;
  Doctype  = Other.Doctype;
  Locale   = Other.Locale;
  return *this;
}

void RESOURCE::SetPath(const STRING& NewPath)
{
  SetFullFileName (AddTrailingSlash (NewPath) + RemovePath(FullPath));
}

STRING RESOURCE::GetPath() const
{
  return RemoveFileName( FullPath );
}

void RESOURCE::SetFileName(const STRING& NewName)
{
  SetFullFileName (::RemoveFileName (FullPath) + NewName);
}

STRING RESOURCE::GetFileName() const
{
  return RemovePath( FullPath );
}

bool RESOURCE::Ok() const
{
  struct stat st_buf;
  if (_IB_stat(FullPath, &st_buf) == 0)
   {
      if (Inode != 0 && Inode != st_buf.st_ino)
	return false;
      if ( (Start > 0 && (off_t)Start > st_buf.st_size) ||
		(End > 0 && Start > End) ||
		(off_t)End > st_buf.st_size )
	return false;
      return true;
    }
  return false;
}


void RESOURCE::Write(PFILE fp) const
{
  putObjID(objRESOURCE, fp);
  ::Write (FullPath,fp);
  ::Write (Start,   fp);
  ::Write (End,     fp);
  ::Write (Doctype, fp);
  ::Write (Locale,  fp);
  ::Write (Inode,   fp);
  ::Write (MimeType,fp);
}


bool RESOURCE::Read(PFILE fp)
{
  obj_t obj = getObjID (fp);
  bool res = true;

  Clear();
  if (obj != objRESOURCE)
    {
      // Not a RSET!
      PushBackObjID (obj, fp);
      return false;
    }
  res &= ::Read (&FullPath,fp);
  ::Read (&Start,   fp);
  ::Read (&End,     fp);
  res &= ::Read (&Doctype, fp);
  res &= ::Read (&Locale,  fp);
  ::Read (&Inode,   fp);
  res &= ::Read (&MimeType,fp);
  if (res == false || Ok() == false)
    {
      Clear();
      return false;
    }
  return true;
}


#if 0

bool RESOURCE::LinkWrite(const STRING& filepath, STRING *path) const
{
  STRING urifile = filepath + _DbExt(ExtPath);
  FILE  *fp; 

  if (path) *path = urifile;
  if ((fp = fopen(urifile, "wb")) != NULL)
    {
      Write(fp);
      fclose(fp);  
      return true;
    }
  return false;   
}


bool RESOURCE::LinkRead(const STRING& filepath, STRING *path) const
{    
  STRING urifile = filepath + _DbExt(ExtPath);
  FILE  *fp;
 
  if (path) *path = urifile;
  Clear();
  if ((fp = fopen(urifile, "rb")) != NULL)
    {    
      Read(fp); 
      fclose(fp);
      return true;
    }
  return false;
}

#endif



#if 0
STRING HTTP_Content() const
{
  const STRING endl ("\n");
  STRING content;

  content << "Content-type: " << (MimeType.IsEmpty() ? "application/binary" : MimeType ) << endl;
  content << "Content-length: " <<  GetLength() << endl;


#endif
