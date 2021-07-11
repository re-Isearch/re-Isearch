#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.hxx"
#include "pathname.hxx"

#ifdef _WIN32
# define CASEINSENSITIVE 1
#endif
#ifndef CASEINSENSITIVE
# define CASEINSENSITIVE 0
#endif


PATHNAME::PATHNAME()
{
}

PATHNAME::~PATHNAME()
{
}


PATHNAME::PATHNAME(const STRING& nFile)
{
  SetFullFileName( ExpandFileSpec(nFile) );
}


STRING PATHNAME::SetPath (const STRING& NewPath)
{
  return Path = ExpandFileSpec(NewPath);
}

STRING PATHNAME::SetFileName(const STRING& newName)
{
  if (ContainsPathSep(newName))
    {
      if (::IsAbsoluteFilePath(newName))
	{
	  SetFullFileName(newName);
	}
      else
	{
	  const CHR Ch = newName.GetChr(1);
#ifdef _WIN32
	  if (newName.GetChr(2) == ':' && isalpha(Ch) )
	    {
	      // Change the drive
	      Path.SetChr(1, Ch);
	      return SetFileName(Path.c_str() + 2); // Recurse
	    }
        else
#endif
	  {
	    STRING full (GetPath());
	    if (!IsPathSep(Ch)) AddTrailingSlash(&full);
	    SetFullFileName(full + newName);
	  }
	}
    }
  else
    {
#ifdef _WIN32
      File = WinGetLongPathName(newName);
#else
      File = newName;
#endif
    }
  return File;
}


STRING PATHNAME::GetFileName() const { if (File.IsEmpty()) return "." ; return File; }
STRING PATHNAME::GetPath() const { return Path; }

PATHNAME PATHNAME::RelativizePathname(const STRING& Dir)
{
  STRING relPath (::RelativizePathname(GetFullFileName(), Dir));
  STRINGINDEX len = relPath.GetLength();
  const char  *tp  = relPath.c_str();
  char  *tcp = (char *)(tp + len);
  do {
    if (IsPathSep(*tcp)) {
      File = tcp+1;
      *tcp = '\0';
      Path = tp;
      break;
    }
  } while (--tcp >= tp);
  return *this;
}



GDT_BOOLEAN  PATHNAME::SetFullFileName (const STRING& newFullPath)
{
  Path.Clear();
  File.Clear();

  if (newFullPath.IsEmpty()) {
    Path = GetCwd();
  } else {
#ifdef _WIN32
    STRING fullpath ( WinGetLongPathName(newFullPath) );
#else
    STRING fullpath (newFullPath);
#endif
    if (::DirectoryExists(fullpath)) {
      Path = fullpath;
    } else if (ContainsPathSep(fullpath)) {
      Path = RemoveFileName(fullpath);
      File = RemovePath(fullpath);
    } else {
      Path = GetCwd();
      File = fullpath;
    }
  }
  return Path.GetLength() > 0 && File.GetLength() > 0;
}

STRING PATHNAME::GetFullFileName() const
{
  return ( AddTrailingSlash(Path)+GetFileName() );
}

INT PATHNAME::Compare(const PATHNAME& Other) const
{
#ifdef _WIN32
    // We want to compare the LongPathNames!
    const STRING path1 (GetFullFileName());
    const STRING path2 (Other.GetFullFileName());
    return ::PathCompare(path1, path2);
#else
    INT result = PathCompare(Path, Other.Path);
    if (result) return result;
    result = ::PathCompare(File, Other.File);
    return result;
#endif
}

// We read and write as fullpath to be compatible with
// the other version of this class

void PATHNAME::Write(FILE *fp) const
{
  GetFullFileName().Write(fp);
}

GDT_BOOLEAN PATHNAME::Read(FILE *fp)
{
  STRING S;
  if (S.Read(fp) == GDT_TRUE) {
    return SetFullFileName(S);
  }
  Clear();
  return GDT_FALSE;
}


#if WANT_FILE_ID_CLASS
FILE_ID::FILE_ID () { inode = device = 0; }
FILE_ID::FILE_ID(STRING& Filename) { *this = Filename; }
FILE_ID::~FILE_ID() {;}


static char *encode64(char *ptr, size_t siz, unsigned long num)
{
  char tmp[13];
  /* 64 characters */
  char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-";
  const size_t modulo = sizeof(chars)/sizeof(char) - 1;
  const size_t pad    = siz - 1;
  unsigned long val = num;
  size_t i = 0, j;

  do {
    tmp[i++] = chars[ val % modulo ];
    val /= modulo;
  } while (val);
  for (j =i; j < pad; j++)
    tmp[j] = '0';
  for (j = 0; j < pad; j++)
    ptr[j] = tmp[pad -  j - 1];
  ptr[j] = '\0';
  return ptr;
}



STRING FILE_ID::Id() const {
  STRING Hash;
  char   ptr[8];

  Hash  = encode64(ptr, sizeof(ptr)/sizeof(char), inode);
  Hash += encode64(ptr, sizeof(ptr)/sizeof(char), device);
  Hash += encode64(ptr, sizeof(ptr)/sizeof(char), _IB_Hostid());
  return Hash;
}

FILE_ID& FILE_ID::operator=(const STRING& nFilename)
{
  STRING Filename = ExpandFileSpec (nFilename);
  if (Filename != filename)
    {
      inode = INODE(filename);
      filename = Filename;
    }
  return *this;
}

GDT_BOOLEAN FILE_ID::operator== (const FILE_ID& OtherId) const
{
  return (inode == OtherId.inode) || (filename == OtherId.filename);
}
#endif


ostream& operator<<(ostream& os, const PATHNAME& path)  
{
  return os << path.GetFullFileName();
}

