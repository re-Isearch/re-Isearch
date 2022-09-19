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

GDT_BOOLEAN RESOURCE::Ok() const
{
  struct stat st_buf;
  if (_IB_stat(FullPath, &st_buf) == 0)
   {
      if (Inode != 0 && Inode != st_buf.st_ino)
	return GDT_FALSE;
      if ( (Start > 0 && (off_t)Start > st_buf.st_size) ||
		(End > 0 && Start > End) ||
		(off_t)End > st_buf.st_size )
	return GDT_FALSE;
      return GDT_TRUE;
    }
  return GDT_FALSE;
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


GDT_BOOLEAN RESOURCE::Read(PFILE fp)
{
  obj_t obj = getObjID (fp);
  GDT_BOOLEAN res = GDT_TRUE;

  Clear();
  if (obj != objRESOURCE)
    {
      // Not a RSET!
      PushBackObjID (obj, fp);
      return GDT_FALSE;
    }
  res &= ::Read (&FullPath,fp);
  ::Read (&Start,   fp);
  ::Read (&End,     fp);
  res &= ::Read (&Doctype, fp);
  res &= ::Read (&Locale,  fp);
  ::Read (&Inode,   fp);
  res &= ::Read (&MimeType,fp);
  if (res == GDT_FALSE || Ok() == GDT_FALSE)
    {
      Clear();
      return GDT_FALSE;
    }
  return GDT_TRUE;
}


#if 0

GDT_BOOLEAN RESOURCE::LinkWrite(const STRING& filepath, STRING *path) const
{
  STRING urifile = filepath + _DbExt(ExtPath);
  FILE  *fp; 

  if (path) *path = urifile;
  if ((fp = fopen(urifile, "wb")) != NULL)
    {
      Write(fp);
      fclose(fp);  
      return GDT_TRUE;
    }
  return GDT_FALSE;   
}


GDT_BOOLEAN RESOURCE::LinkRead(const STRING& filepath, STRING *path) const
{    
  STRING urifile = filepath + _DbExt(ExtPath);
  FILE  *fp;
 
  if (path) *path = urifile;
  Clear();
  if ((fp = fopen(urifile, "rb")) != NULL)
    {    
      Read(fp); 
      fclose(fp);
      return GDT_TRUE;
    }
  return GDT_FALSE;
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
