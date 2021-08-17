/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)common.cxx  1.98 08/08/01 15:32:55 BSN"
/************************************************************************
************************************************************************/
/* TODO: mkdir for windows because of UNC */


/*-@@@
File:		common.cxx
Version:	2.00
Description:	Common functions
@@@*/
#include "platform.h"
#include "pathname.hxx"

#ifndef WWW_SUPPORT
#define WWW_SUPPORT
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#if defined(_MSDOS) || defined(_WIN32)
# include <direct.h>
# include <io.h>
#ifndef WIN32
# define WIN32
#endif
#else
# include <unistd.h>
# include <signal.h>
# include <fcntl.h>
#ifdef LINUX
#include <sys/sysinfo.h>
#endif
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "common.hxx"
#include "buffer.hxx"
#include "mmap.hxx"

#if defined(_WIN32)
# include <winsock2.h>
#else
# include <sys/wait.h>
# include <sys/stat.h>
# include <pwd.h>
# include <grp.h>
# include <arpa/inet.h>
# include <dlfcn.h>
#endif


#ifdef _WIN32
# undef WWW_SUPPORT
#endif

#ifndef FILENAME_MAX
# define FILENAME_MAX 1024
#endif

#if defined(_MSDOS) || defined(_WIN32)
static const char SEP = '\\';
static const char dot_ib[] = "_ib";

GDT_BOOLEAN IsPathSep(const int c)
{
  if(c == '\\' || c == '/') return GDT_TRUE;
    return GDT_FALSE;
}


#else /* UNIX */
static const char dot_ib[] = ".ib";
static const char SEP = '/';

GDT_BOOLEAN IsPathSep(const int c)
{
  return (c == SEP);
}

#define MKDIR(_x,_y) mkdir(_x,_y)

#endif

//////////////////////////// CODE //////////////////////////////
static STRLIST     GarbageFileList;
static GDT_BOOLEAN FinalGarbageCollect = GDT_FALSE;

#ifdef _WIN32
#include <fcntl.h>
int EraseFileContents(const char *Filename)
{
  int fd = open(Filename, O_WRONLY | O_TRUNC);
  if (fd)
    {
      close(fd);
      return 0;
     }
   return -1;
}
#else
int EraseFileContents(const char *Filename)
{
  return truncate(Filename, 0);
}
#endif
#if defined(_MSDOS) || defined(_WIN32)
# define ftruncate(_fd,_len) chsize(_fd, _len)
# include <io.h>
#endif

int EraseFileContents(FILE *Fp)
{
  if (Fp)
    return ftruncate(fileno(Fp), 0);
  errno = EINVAL;
  return -1;
}


static GDT_BOOLEAN _remove(const STRING& Fn)
{
  if (Fn.Unlink() && FileExists(Fn))
    {
      if (FinalGarbageCollect)
	message_log (LOG_WARN|LOG_ERRNO, "Could not remove '%s'. Remove by hand.", Fn.c_str());
      return GDT_FALSE;
    }
  return GDT_TRUE;
}

static int _CollectFileGarbage(GDT_BOOLEAN Final = GDT_FALSE)
{
  int count = 0;
  if (!GarbageFileList.IsEmpty())
    {
      if (Final) FinalGarbageCollect = Final;
      if ((count = GarbageFileList.Do(_remove)) != 0 )
	{
	  if (Final)
	    message_log (LOG_WARN|LOG_ERRNO, "Some litter left on disk: '%d' files. Remove by hand.", count);
	}
    }
  return count;
}

static void _FinalCollectFileGarbage()
{
  _CollectFileGarbage(GDT_TRUE);
}

int  AddtoGarbageFileList(const STRING& Fn)
{
  if (Fn.GetLength() && FileExists(Fn))
    {
      if (GarbageFileList.IsEmpty())
	atexit( _FinalCollectFileGarbage ); // Run garbage collection when finished
      GarbageFileList.AddEntry(Fn);
      return EraseFileContents(Fn); // Zap contents
    }
  return 0;
}


///////////////////////////////////////////
#if defined(_MSDOS) || defined(_WIN32)

STRING FixMicrosoftPathNames(const STRING& Path)
{
  STRINGINDEX sPos = Path.Find('/');
  STRINGINDEX bPos = Path.Find('\\');

  if (sPos && bPos)
    {
      STRING newPath(Path);
      //UNC pathes
      if(bPos == 1 && Path.GetChar(2) == '\\')
	newPath.Replace("/", "\\", GDT_TRUE);
      else
	newPath.Replace("\\", "/", GDT_TRUE);
      return newPath;
   }
  return Path;
}


STRING WinGetLongPathName(const STRING& Path)
{
  STRING longPath;
  BUFFER tmp;
  size_t res = 255;
  size_t maxL = 0;
  long   err  = 0;

  if (Path.IsEmpty())
    return GetCwd();

  if (Path.GetLength()<8) return Path;

  // We need to make sure we have it all
  do {
    maxL = res;
    if (tmp.Want(maxL+1) == NULL)
      {
        // Memory error
        message_log (LOG_PANIC, "WinGetLongPathName: Memory exhausted. Wanted %u", maxL+1);
	err = ERROR_NOT_ENOUGH_MEMORY; // 8
        res = 0; // Failure
      }
    else
      res = (size_t)::GetLongPathName(Path.c_str(), (char *)( tmp.Ptr()), maxL);
  } while (res > maxL);

  if (res == 0)
   {
     if (err == 0) err = (long)::GetLastError();
     if (err == 2 || err == 3)
	{
	  // Microsoft is stupid here.. If the file has a short name directory it fails
	  // since the file does not yet exist..
	  STRING dir      = RemoveFileName(Path);
	  STRING filename = RemovePath(Path);
	  res = 255;
	  do {
	    maxL = res;
	    if (tmp.Want(maxL+1) == NULL)
	      {
		message_log (LOG_PANIC, "WinGetLongPathName: Memory exhausted. Wanted %u", maxL+1);
		err = ERROR_NOT_ENOUGH_MEMORY; // 8
		res = 0;
	      }
	    else
	      res = (size_t)::GetLongPathName(dir.c_str(), (char *) tmp.Ptr(), maxL);
	  } while (res > maxL && res != 0);
	  if (res > 0)
	    {
	      longPath = (const char *)tmp;
	      if (filename.GetLength() && (filename != ".")) 
		{
		  AddTrailingSlash(longPath);
		  longPath.Cat(filename);
		}
	    }
	  else err = (long)::GetLastError();
	}
     else if (err <= 3)
	res = (longPath = Path).GetLength(); // Assume its already long format
     else {
        errno = err;
	message_log (LOG_WARN|LOG_ERRNO, "Could not get the Win32 long path for '%s' (Err #%ld).", Path.c_str(), err);
     }
    }
  errno = 0; // Reset error

  if (longPath.IsEmpty())
    {
      if (res && res != Path.Length())
	longPath = (const char *)tmp; 
      else
	longPath = Path;
    }

  // Not UNC?
  if (res > 2)
    {
      if (longPath.GetChr(1) != '\\' || longPath.GetChr(2) != '\\')
	longPath.Replace("\\", "/");
      else 
	longPath.Replace("/", "\\");
    }
  return longPath;
}

#undef getpid
#define getpid (int)GetCurrentProcessId  /* cast DWORD to int */

# ifndef S_ISREG
#  define S_ISREG(_x) (_S_IFREG & _x)
# endif
#define MKDIR(_x,_y) winMkDir(_x) /* Win32 does not support mode arg */

#endif

#ifdef _WIN32
	// Set this in DLLMain of your shared lib
	static HINSTANCE _hglobalSharedLibrary = 0;
#endif

static STRING _sglobalSharedLibraryName;


STRING GlobalSharedLibraryName ()
{
  if (_sglobalSharedLibraryName.GetLength())
    return RemovePath(_sglobalSharedLibraryName);
#ifdef _WIN32
  if(_hglobalSharedLibrary) {
    char tmp[FILENAME_MAX];
    if(GetModuleFileName(_hglobalSharedLibrary, tmp, sizeof(tmp)/sizeof(tmp[0])))
      {
	STRING name = RemovePath (_sglobalSharedLibraryName = tmp);
	size_t x = name.Find('.', GDT_TRUE); if (x) name.EraseAfter(x);
	return name;
      }
  }
#endif
  return "IB";
}

STRING FindSharedLibrary(const STRING& Argv0)
{
	STRING sSharedLibraryName;
#ifdef _WIN32
	if(_hglobalSharedLibrary)
	{
	  char tmp[FILENAME_MAX]; 
	  if(GetModuleFileName(_hglobalSharedLibrary, tmp, sizeof(tmp)/sizeof(tmp[0]))) {
	    if (Argv0.GetLength()) {
	      sSharedLibraryName = RemoveFileName(tmp);
	      AddTrailingSlash(&sSharedLibraryName);
	      sSharedLibraryName.Cat(Argv0);
	    } else sSharedLibraryName = tmp;
	    return sSharedLibraryName;
	  }
	}
#else	
/* ????????????????????????????????????????????????????
 * EDZ: Pls make some changes to better fit UNIX environments
 */
	{
	  Dl_info info;
	  void *addr = (void *)&FindSharedLibrary;
	  if (dladdr(addr, &info))
	    {
	      message_log (LOG_DEBUG, "Base shared Library=%s\n", info.dli_fname);

	      if (_sglobalSharedLibraryName.IsEmpty())
		_sglobalSharedLibraryName = RemovePath(info.dli_fname);
	      if (Argv0.IsEmpty())
		return info.dli_fname; 

	      STRING dir ( AddTrailingSlash(RemoveFileName(info.dli_fname)));
	      STRING exe ( dir + Argv0 );

	      if (FileExists(exe))
		return exe;
	      if (FileExists(exe = dir + "../lib/" + Argv0))
		return exe;
	      if (FileExists(exe = dir + "../contrib/lib/" + Argv0))
		return exe;
	    }
	   else message_log (LOG_DEBUG, "FindSharedLibrary: Could not find own symbol.");
	  }
	//Name was not set
	if(_sglobalSharedLibraryName == NulString) return NulString; 
	
#if 1
	STRING sPath = FindExecutable(Argv0);
	if (!sPath.IsEmpty()) return sPath; 

#else
	//Hunt for it
	STRING sPath = RemoveFileName(FindExecutable(Argv0));
	if(!sPath.IsEmpty())
	{
		sSharedLibraryName = AddTrailingSlash(sPath)+AddTrailingSlash("lib")+_sglobalSharedLibraryName; 
		if(Exists(sSharedLibraryName)) return sSharedLibraryName;
		
		sSharedLibraryName = AddTrailingSlash(sPath)+_sglobalSharedLibraryName; 
		if(Exists(sSharedLibraryName)) return sSharedLibraryName;
	}

	sPath = _IB_LIBPATH();
	if(!sPath.IsEmpty())
	{
		sSharedLibraryName = AddTrailingSlash(sPath)+_sglobalSharedLibraryName; 
		if(Exists(sSharedLibraryName)) return sSharedLibraryName;
	}
#endif
	return NulString;
#endif
}


//
STRING FindExecutable(const STRING& Argv0)
{
 if (!Argv0.IsEmpty() && IsAbsoluteFilePath(Argv0)) return Argv0;
#ifdef _WIN32
  char tmp[FILENAME_MAX+1024];
 
  size_t res = (size_t)::GetModuleFileName(NULL, (LPTSTR)tmp, sizeof(tmp)/sizeof(char));
  if (res > 0)
    {
      if (res >= sizeof(tmp))
	message_log (LOG_PANIC, "Internal path length %u > %u. Contact support.", res, sizeof(tmp));
      STRING exe ( WinGetLongPathName(tmp) );
      if (Argv0.GetLength())
	{
	  const STRING dir (AddTrailingSlash(RemoveFileName(exe)));
	  if (!ExeExists(exe = dir + Argv0))
	    if (!ExeExists(exe = dir + "..\\bin\\" + Argv0))
	      if (!ExeExists(exe = dir + "..\\contrib\\bin\\" + Argv0))
		exe = dir + Argv0;
	  strncpy(tmp, __Realpath(exe).c_str(), sizeof(tmp));
	  tmp[sizeof(tmp)-1]= '\0';
	}
      // remove .exe, .com if last characters
      if (res > 4)
	{
	  char         *tcp = &tmp[res-4];
	  char          ch;
	  if ((tcp[0] == '.') &&
		((ch = tolower(tcp[1])) == 'e' || ch == 'c') &&
		((ch = tolower(tcp[2])) == 'x' || ch == 'o') &&
		((ch = tolower(tcp[3])) == 'e' || ch == 'm'))
	    *tcp = '\0';
	}
      return tmp;
    }
#else

 // Linux, BSD
 Dl_info info;
 void *addr = (void *)NULL;
 // Not const void * since Solaris 10 has it as void *
 if (dladdr(addr, &info))
   {
     // Found something
     STRING   exe (ExpandFileSpec(info.dli_fname));
     if (Argv0.IsEmpty())
	return exe;
     const STRING dir (AddTrailingSlash(RemoveFileName(exe)));
     if (FileExists(exe = dir + Argv0))
       return __Realpath(exe);
     if (FileExists(exe = dir + "../bin/" + Argv0))
       return __Realpath(exe);
     if (FileExists(exe = dir + "../contrib/bin/" + Argv0))
       return __Realpath(exe);
   }
#endif
  if (Argv0.IsEmpty())
    {
      return NulString; // Already
    }

  if (Argv0.GetChr(1) == '.' || Argv0.Search(SEP))
    {
      return ExpandFileSpec(Argv0);
    }

  const char *path = getenv("PATH");
  if (path && *path)
    {
      GDT_BOOLEAN found = GDT_FALSE;
      STRLIST PathList;
      STRING  spec;
#ifdef _WIN32
      STRING  Path = STRING(".;")+path;
#else
      STRING  Path  = path; 
#endif

      PathList.SplitPaths(Path);
      for (const STRLIST *ptr = PathList.Next();
	found == GDT_FALSE && ptr != &PathList; ptr = ptr->Next())
	{
	  spec = ptr->Value();
	  AddTrailingSlash(&spec);
	  spec.Cat (Argv0);
	  found = FileExists (spec);
#ifdef _WIN32
	  if (found == GDT_FALSE)
	    found = Exists (spec + ".exe");
#endif
	}
      if (found) return ExpandFileSpec(spec);
    }
  return NulString; // Did not find!
}


GDT_BOOLEAN ContainsPathSep(const STRING& Path)
{
  const char *path = Path.c_str();
  while (*path)
    {
      if (IsPathSep(*path)) return GDT_TRUE;
#ifdef _WIN32
      if (*path == ':') return GDT_TRUE;
#endif
      path++;
    }
  return GDT_FALSE;
}


//
GDT_BOOLEAN IsAbsoluteFilePath(const STRING& Path)
{
#ifdef _WIN32

  // Standard drive convention
  if(Path.GetLength() >= 3		&& 
  	 isalpha(Path.GetChr(1))	&&
     Path.GetChr(2) == ':'		&&
     IsPathSep(Path.GetChr(3))	 )
    return GDT_TRUE;

  // Network drive convensiton \\xxxx\ where xxxx is machine id
  if(Path.GetLength() >= 3	&&
     Path.GetChr(1) == '\\' &&
     Path.GetChr(2) == '\\'  )
    return GDT_TRUE;

  //Any path beginning with a path separator is absolute within the current drive only.
  //Therefore I (PS) recommend to not regard it as absolute
  /*
  if((Ch = IsPathSep(Path.GetChr(1)))
    return GDT_TRUE;
  */
  return GDT_FALSE;

#else
  return Path.GetChr(1) == SEP;

#endif
}


//
GDT_BOOLEAN IsRootDirectory(const STRING& Path)
{
#ifdef _WIN32
  // Standard drive convention
  if(Path.GetLength() == 3    && 
  	 isalpha(Path.GetChr(1))  &&
     Path.GetChr(2) == ':'    &&
     IsPathSep(Path.GetChr(3)) )
    return GDT_TRUE;

  // Network drive convensiton \\xxxx\ where xxxx is machine id
  if(Path.GetLength() >= 3  &&
     Path.GetChr(1) == '\\' &&
     Path.GetChr(2) == '\\'  )
  {
  	//Now check if there is no other path
  	size_t i1 = Path.Find("/", 2);
  	size_t i2 = Path.Find("\\", 2);
  	if((i1 == NOT_FOUND || i1 == Path.GetLength()-1) &&
  	   (i2 == NOT_FOUND || i2 == Path.GetLength()-1)  )
  		return GDT_TRUE; 
  }
  return GDT_FALSE;
#else
  return (Path.GetLength() == 1 && Path.GetChr(1) == SEP);
#endif
}

int RmDir(const STRING& Filename)
{
  int my_err = 0;
#ifdef _WIN32
# define _chmod_mode S_IWRITE
#else
# define _chmod_mode  S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#endif
  if (::rmdir(Filename.c_str()) == 0) return 0;
  my_err = errno;
  // No? Try to change the mode and try again
  if (chmod(Filename.c_str(), _chmod_mode) == 0)
    return ::rmdir(Filename.c_str());
  errno = my_err; 
  return -1;
}

int UnlinkFile(const STRING& Filename)
{
  int my_err = 0;
  if (::remove(Filename.c_str()) == 0) return 0;
  my_err = errno;
  if (chmod(Filename.c_str(), _chmod_mode) == 0)
    return ::remove(Filename.c_str());
#undef _chmod_mode
  errno = my_err;
  return -1;
}

static int __filerename(const STRING& From, const STRING& To)
{
  int result = 0;
  if (rename(From.c_str(), To.c_str())) 
    if ((result = FileLink(From.c_str(), To.c_str())) == 0)
      return From.Unlink();
  return result;
}

int RenameFile(const STRING& From, const STRING& To)
{
  STRING FromName (ExpandFileSpec (From));
  if (!Exists(FromName))
    {
      errno = ENOENT;
      return -1;
    }
  STRING ToName (ExpandFileSpec(To));
  if (DirectoryExists(ToName))
    {
      AddTrailingSlash(&ToName);
      ToName.Cat( RemovePath(FromName) );
    }
  errno = 0;
  int result = __filerename (FromName.c_str(), ToName.c_str());
     
  if (result && FileExists(ToName))
    {
      STRING backup;

      _CollectFileGarbage();
      for (size_t i=1; i < 1024; i++)
	{
	  backup.form("%s~%d.DELETE_ME", ToName.c_str(), i);
	  if (!Exists(backup))
	    break;
	}
      errno = 0;
      if ((result = __filerename(ToName.c_str(), backup.c_str())) == 0)
	{
	  if ((result = __filerename(FromName.c_str(), ToName.c_str())) == 0)
	    {
	      if (unlink (backup) == -1)
		{
		  AddtoGarbageFileList(backup);
		}
	    }
	  else
	    {
	      int old_errno = errno;
	      rename (backup.c_str(), ToName.c_str());
	      errno = old_errno;
	    }
	}
    }
  return result;
}


GDT_BOOLEAN FileExists(const STRING& Path)
{
  struct stat st_buf;

  // Does it exists and is it a file?
  if (stat(Path.c_str(), &st_buf) == 0)
    {
      if (!S_ISDIR((st_buf.st_mode))) // Not a directory?
        return GDT_TRUE;
    }
  return GDT_FALSE;
}

// Is the file executable?
GDT_BOOLEAN ExeExists(const STRING& Path)
{
#ifdef _WIN32
  // Like IsExecutable but can look at extensions
  // -- really only a difference under Windows
  return (IsExecutable(Path) || FileExists(Path + ".exe") || FileExists(Path + ".com"));
#else
  return IsExecutable(Path);
#endif
}



// Does it exist and is it a directory?
GDT_BOOLEAN DirectoryExists(const STRING& Path)
{
  struct stat  st_buf;
  const size_t len = Path.GetLength();

  if (len)
    {
#ifdef _WIN32
      if (IsPathSep(Path.GetChr(len)))
	return DirectoryExists(Path + ".");
#endif
      if (stat(Path.c_str(), &st_buf)== 0 && S_ISDIR(st_buf.st_mode))
        return GDT_TRUE;
    }
  return GDT_FALSE;
}

CHR PathSepChar()
{
  return SEP;
}

// Recursive
void RemoveTrailingSlash(PSTRING PathName)
{
  STRINGINDEX x = (PathName ? PathName->GetLength() : 0 );
  if (x && IsPathSep(PathName->GetChr(x)))
    {
      PathName->EraseAfter(x-1);
#ifdef WIN32
      // Need / in C:/
      if (x == 3 && PathName->GetChr(2) == ':') PathName->Cat("/");
#endif
      RemoveTrailingSlash(PathName);
    }
}

STRING RemoveTrailingSlash(const STRING& Pathname)
{
  STRING tmp (Pathname);
  ::RemoveTrailingSlash(&tmp);
  return tmp;
}

void AddTrailingSlash (PSTRING PathName)
{
  STRINGINDEX x = PathName ? PathName->GetLength() : 0;
  if (x && !IsPathSep(PathName->GetChr(x)))
    PathName->Cat (SEP);
#ifdef _WIN32
  *PathName = FixMicrosoftPathNames(*PathName);
#endif

}

STRING AddTrailingSlash (const STRING& PathName)
{
  STRING tmp (PathName);
  AddTrailingSlash(&tmp);
  return tmp;
}


void RemovePath (PSTRING FileName)
{
  if (FileName)
    *FileName = RemovePath(*FileName);
}

STRING RemovePath (const STRING& PathName)
{
#ifdef _WIN32
  // Point to the fullpath
  const char *sp = PathName.c_str();
  // Point to the end of the fullpath
  for (const char *tp = sp + PathName.GetLength() - 1; tp >= sp; tp--)
  {
      if(IsPathSep(*tp))
      {
        if(tp > sp && IsPathSep(*(tp-1))) return NulString;
        return tp+1;
      }
  }
  // Not found
  return PathName;
#else
  return PathName.Right(SEP);
#endif
}


void RemoveFileName (PSTRING FileName)
{
  if (FileName)
    *FileName = RemoveFileName(*FileName);
}


STRING RemoveFileName (const STRING& PathName)
{
  //Don't want that here - inconsistent with RemovePath
  //Didn't work anyhow because caode was using with "PathName" and "fullpath" randomly!!!!!!!
  //STRING fullpath = ExpandFileSpec (PathName); // Get fullpath
#ifdef _WIN32
  STRING fullpath(PathName);

  // Point to the fullpath
  char *sp = (char *)(fullpath.c_str());
  // Point to the end of the fullpath
  for (const char *tp = sp + fullpath.GetLength() - 1; tp >= sp; tp--)
  {
    if (IsPathSep(*tp))
    {
      if(tp > sp && IsPathSep(*(tp-1))) return fullpath;
#ifdef LIBCONTEXT_SHI  //Don\xb4t want trailing slashes
          return fullpath.EraseAfter(tp-sp);
#else
          return fullpath.EraseAfter(tp-sp+1);
#endif
    }
  }
#else
  STRING fullpath (ExpandFileSpec (PathName)); // Get fullpath
  STRINGINDEX x = fullpath.SearchReverse(SEP);
  if (x)
#ifdef LIBCONTEXT_SHI  //Don't want trailing slashes
    return fullpath.EraseAfter(x-1);
#else
    return fullpath.EraseAfter(x);
#endif

#endif
 // Not found
 return NulString;
}


// Make a temporary file name, opens it to write
// whhen OK, closes it and returns the name...
STRING MakeTempFileName(const STRING& Fn)
{
  STRING TmpName = Fn + "~";
  unsigned long pid = (long)getpid();

  // Look for a non-existing....
  for (size_t i =0; FileExists(TmpName); i++)
    {
      TmpName.form ("%s%d.%lu", Fn.c_str(), (int)i, pid);
    }
  // Open it... (Racing condition??)
  FILE *oFp = fopen(TmpName, "wb");
  if (oFp == NULL)
    {
      // Fall into scatch (we might not have writing permission
      char     scratch[ L_tmpnam+1];
      char    *TempName = tmpnam( scratch );

      message_log (LOG_WARN, "Could not create '%s', trying tmp '%s'", TmpName.c_str(),
        TempName);
      if ((oFp = fopen(TempName, "wb")) == NULL)
        {
          message_log (LOG_ERRNO, "Can't create a temporary numlist '%s'", Fn.c_str());
          return NulString;
        }
      TmpName = TempName; // Set it
    }
  fclose(oFp);
 return TmpName;
}

GDT_BOOLEAN WritableDir(const STRING& path)
{
  const size_t len = path.Length();
  if (len == 0) return GDT_TRUE;

#ifdef _WIN32
  if (IsPathSep(path.GetChr(len)))
    return WritableDir(path + ".");

  // _access_s is the security version of _access, its pulled in
  // with the head <io.h>
  if (DirectoryExists(path) && (
	_access(path.c_str(), 4 ) != 0 ||
	_access(path.c_str(), 6) == 0)) {
    return GDT_TRUE;
  } else {
   // We are given a file in a directory
   const STRING  dir (PATHNAME(path).GetPath());
   if (DirectoryExists(dir) && (
	_access(path.c_str(), 4) != 0 ||
	_access(path.c_str(), 6) == 0))
     return GDT_TRUE;
  }
#else
  struct stat st_buf;
  if (stat(path.c_str(), &st_buf) == 0)
    {
      if (S_ISDIR(st_buf.st_mode))
	{
check:
	  const uid_t my_uid = geteuid();
	  const gid_t my_gid = getegid();
	  // const int perms    = st_buf.st_mode & 0x1FF;
	  if (my_uid == 0)
	    return GDT_TRUE;
#define _S_IS(flag) (((st_buf.st_mode)&(flag)) == (flag))
	  if (st_buf.st_uid == my_uid)
	    {
	      if (_S_IS(S_IWUSR))
		return GDT_TRUE;
	    }
	  else if (st_buf.st_gid == my_gid)
	    {
	      if (_S_IS(S_IWGRP) && _S_IS(S_IRGRP))
		return GDT_TRUE;
	    }
	  else if (_S_IS(S_IWOTH) && _S_IS(S_IROTH))
	    {
	      return GDT_TRUE;
	    }
#undef _S_IS
	  // Try to open something?
	  const STRING dir ( AddTrailingSlash(path) );
	  STRING       test;
          for (int i=0; i < 99; i++)  {
	    test.form("%s%d#%lx.tmp", dir.c_str(), i, (long)getpid());
	    if (!Exists(test) )
	      {
		// Something not already around... 
		FILE *fp = fopen(test.c_str(), "wb");
		if (fp != NULL)
		  {
		    // It opens!! OK!
		    fclose(fp);
		    unlink(test);
		    return GDT_TRUE;
		  }
		break; // Nope
            }
	  }
	}
      else if ( S_ISREG(st_buf.st_mode))
	{
	  return WritableDir (RemoveFileName (path));
	}
    }
  else if (stat(RemoveFileName (path).c_str(), &st_buf) == 0)
    {
      if (S_ISDIR(st_buf.st_mode))
	goto check;
    }
#endif
  return GDT_FALSE;
}

GDT_BOOLEAN Exists(const STRING &path)
{
  struct stat st_buf;
  int res = stat(path.c_str(), &st_buf);
  return (res == 0);
}

//
// Does file exist and can it be run?
//
GDT_BOOLEAN IsExecutable(const STRING &path)
{
  struct stat st_buf;
  int         res = stat(path.c_str(), &st_buf);

#if defined(_MSDOS) || defined(_WIN32)
  // if it does not stat or if its a directory its not a bin
  if (res < 0 || S_ISDIR(st_buf.st_mode))
    return GDT_FALSE;
  // WIN32  stat does not implement S_IXUSR
  // Windows executables are .COM, .BAT or .EXE
  STRING ext ( path.Left('.') );
  return (ext != path && ext.GetLength() == 3 &&
	(ext ^= "exe") || (ext ^= "com") || (ext ^= "bat"));
#else

  return res == 0 ?  (st_buf.st_mode & S_IXUSR) == S_IXUSR : GDT_FALSE;
#endif
}


off_t GetFileSize (FILE *fp)
{
  return fp ? GetFileSize (fileno (fp)) : 0;
}


//@@@ edz: use fstat instead of seek
off_t GetFileSize (int fd)
{
  struct stat sb;

  if ((::fstat (fd, &sb) >= 0) && ((sb.st_mode & S_IFMT) == S_IFREG))
    {
#if HOST_MACHINE_64
      // OK
#else
      if (sb.st_size > (off_t)(0x7fffffffU))
	message_log (LOG_WARN, "File possibly too large (%ldMB)",
		(long)(sb.st_size/(1024*1024L)) );
#endif
      return sb.st_size;
    }
  return 0;                     // Error (return 0 instead of -1)
}

off_t GetFileSize (const STRING& path)
{
  return GetFileSize((const CHR *)path);
}

off_t GetFileSize (const CHR *path)
{
  struct stat sb;

  if ((stat (path, &sb) >= 0) && ((sb.st_mode & S_IFMT) == S_IFREG))
    {
#if HOST_MACHINE_64
      // OK
#else
      if (sb.st_size > (off_t)(0x7fffffffU))
	message_log (LOG_WARN, "File %s possibly too large (%ldMB)", path, (long)(sb.st_size/(1024*1024L)) );
#endif
      return sb.st_size;
    }
  return 0;                     // Error (return 0 instead of -1)
}

GDT_BOOLEAN SetUserName(const STRING& Id)
{
#if defined(_MSDOS) || defined(_WIN32)
  // Could use NIS, MS-Mail or other site specific programs
  // Use wxWindows configuration data
//  SetProfileString(IB_SECTION, eUSERNAME, Id.c_str()); 
  // Now check that its set
  char buf[256];
  GDT_BOOLEAN b = _IB_GetUserName(buf, sizeof(buf)/sizeof(buf[0])-1);
  if (b) b = Id.Equals(buf);
  return b;
#else
  uid_t uid = (uid_t) Id.GetLong();
  errno = 0;
  if (uid <= 0)
    {
      struct passwd  *pwent = getpwnam(Id);
      if (pwent != NULL)
	uid = pwent->pw_uid;
      else
	{
	  if (errno == 0)
	    errno =  EINVAL;
	  return GDT_FALSE; // No user
	}
    }
  if (uid >= 0 && setuid (uid) >= 0)
    return GDT_TRUE;
  return GDT_FALSE;
#endif
}


GDT_BOOLEAN SetUserGroup(const STRING& Id)
{
#if defined(_MSDOS) || defined(_WIN32)
  return GDT_FALSE;
#else
  gid_t gid = (gid_t) Id.GetLong();
  errno = 0;
  if (gid <= 0)
    {
      struct group  *grent = getgrnam(Id);
      if (grent != NULL)
        gid = grent->gr_gid;
      else
        {
          if (errno == 0)
            errno =  EINVAL;
          return GDT_FALSE; // No group
        }
    }
  if (gid >= 0 && setgid (gid) >= 0)
    return GDT_TRUE;
  return GDT_FALSE;
#endif
}

STRING GetUserGroup()
{
#if defined(_MSDOS) || defined(_WIN32)
  return NulString;
#else
  struct group *grent = getgrgid( getgid() );
  if (grent != NULL)
    return grent->gr_name;
  return NulString;
#endif
}

#ifdef _WIN32
STRING _winRegKey(HKEY hBaseKey, const char *Key, const char *Value)
{
	HKEY _hkey = NULL;
	
	//Open the registry key
	if(RegOpenKeyEx(hBaseKey, Key, 0, KEY_QUERY_VALUE, &_hkey) == ERROR_SUCCESS && _hkey)
    {
		// Get the value
		DWORD _dw =  FILENAME_MAX;
		unsigned char buf[FILENAME_MAX];
		long _nRet = (long)RegQueryValueEx(_hkey, Value, NULL, NULL, buf, &_dw);
		RegCloseKey(_hkey); // Close the key
		if(_nRet == ERROR_SUCCESS) return buf;
    }
	return NulString;
}
STRING _winRegKeyUser(const char *Key, const char *Value)
{
	return _winRegKey(HKEY_CURRENT_USER, Key, Value);
}
STRING _winRegKeyMachine(const char *Key, const char *Value)
{
	return _winRegKey(HKEY_LOCAL_MACHINE, Key, Value);
}

STRING _win_MyFiles()
{
  static STRING myFiles; // Cache it

  if (myFiles.GetLength()) return myFiles;

  // Fetch it
  myFiles = _winRegKeyUser("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal");

  if(myFiles.IsEmpty()) myFiles = getenv("USERPROFILE");
  if(myFiles.IsEmpty())
    {
      const char *drive = getenv("HOMEDRIVE");
      const char *path  = getenv("HOMEPATH");
      const char *share = getenv("HOMESHARE"); 
      GDT_BOOLEAN   want_msg = (drive && path) || share ? GDT_FALSE : GDT_TRUE;

      if (drive == NULL && path == NULL && share && DirectoryExists(share))
	{
	  myFiles = share;
	}
      else
	{
	  if (drive == NULL || *drive == '\0')
	    {
	      // Check SYSTEMDRIVE... if not use C:
	      const char *system_drive = getenv("SYSTEMDRIVE");
	      drive = system_drive && *system_drive ? system_drive : "C:";
	    }
	  if (path  == NULL || *path  == '\0') path  = "\\MyFiles";

	  myFiles = drive;
	  myFiles.Cat(path);
	  if (want_msg) message_log (LOG_WARN, "Home directory unavailable! Using %s", myFiles.c_str());
	}
    } 
  if(!DirectoryExists(myFiles))
    {
      message_log (LOG_WARN, "Home directory '%s' does not exist! Creating '%s'", myFiles.c_str(), myFiles.c_str());
      MkDirs(myFiles);
    }
  if(!DirectoryExists(myFiles))
    {
      message_log (LOG_ERROR, "Home directory '%s' unavailable!", myFiles.c_str());
      return NulString;
    }
   return myFiles;
}
#endif


const char *_GetUserHome(const char *user)
{
#if defined(_MSDOS) || defined(_WIN32)
  /* SHI Suggestion */
  STRING        homeDir (_win_MyFiles());

  if (homeDir.IsEmpty()) {
    static int tries = 0;
    if (tries++ < 2)
      message_log (LOG_ERROR, "Can't find a home directory! Check Windows installation.");
     return NULL;
  } else if(user != NULL && *user != '\0')
    message_log (LOG_WARN, "Home directory for '%s' can not be retrieved. Using '%s'", user, homeDir.c_str());

   return (char *)(homeDir.c_str());
#else

  struct passwd  *pwent;

  if (user == NULL || *user == '\0')
    pwent = getpwuid(getuid());
  else
    pwent = getpwnam(user);
        
  if (pwent != NULL)
    return pwent->pw_dir;
  return NULL;
#endif
}

static const char *_GetUserHome(char *buf, const char *user)
{
  const char *dir = _GetUserHome(user);
  if (dir == NULL || *dir == '\0')
    {
      if (buf) *buf = '\0';
      return NULL;
    }
  strcpy(buf, dir);
  return buf;
}


//
// Public Function
//
// If it can't find then -> ~user/
//
STRING GetUserHome(const char *user)
{
  const char   *dir = _GetUserHome(user);
  STRING        home;

  if (dir != NULL && *dir)
    home = dir;
  else if (user)
    home << "~" << user;
  else
    return NulString;

#ifdef LIBCONTEXT_SHI  //Don\xb4t want trailing slashes
  return home;
#else
  return AddTrailingSlash( home );
#endif
}

// Destructive removal of /./ and /../ stuff
static char *_Realpath (char *path)
{
  char Sep = '/';
 	if(path[0] && path[1])
  	{
#ifdef _WIN32
  		GDT_BOOLEAN bPathChanged = FALSE;
#endif
		for (char *p = &path[2]; *p; p++)
		{
			if (IsPathSep(*p))
			{
				Sep = *p; // Set the path sep
				if (p[1] == '.' && p[2] == '.' && (IsPathSep(p[3]) || p[3] == '\0'))
				{
					char *q = p - 1;
					while (q >= path && !IsPathSep(*q)) q--;
					if(IsPathSep(q[0])									&& 
					   (q[1] != '.' || q[2] != '.' || IsPathSep(q[3]))	&&
					   (q - 1 <= path || !IsPathSep(q[-1]))				 )
					{
#ifdef _WIN32
						bPathChanged = TRUE;
#endif
						strcpy (q, p + 3);
						if(path[0] == '\0')
						{
							path[0] = Sep;
							path[1] = '\0';
						}
						p = q - 1;
					}
				}
	    		else if (p[1] == '.' && (IsPathSep(p[2]) || p[2] == '\0'))
	    		{
#ifdef _WIN32
					bPathChanged = TRUE;
#endif
					strcpy (p, p + 2);
	    		}
			}
		}
#ifdef _WIN32
		//Should have no problem, because length is not changed 
		if(bPathChanged)
		{
			STRING fixedpath = FixMicrosoftPathNames(path);
			strcpy (path, fixedpath); 
		} 
#endif
  	}
	
	return path;
}

/*-
 Handles:
   ~/ => home dir
   ~user/ => user's home dir
   If the environment variable a = "foo" and b = "bar" then:
	$a	=>	foo
	$a$b	=>	foobar
	$a.c	=>	foo.c
	xxx$a	=>	xxxfoo
	${a}!	=>	foo!
	$(b)!	=>	bar!
	\$a	=>	\$a
 */

//ExpandPath helpers
#ifdef _WIN32
void EscapePath(STRING& filePath) { filePath.Replace("\\", "\\\\"); }
void UnEscapePath(STRING& filePath)	{ filePath.Replace("\\\\", "\\"); }
#else
void EscapePath(const STRING& filePath) {}
void UnEscapePath(const STRING& filePath) {}
#endif

/* input name in name, pathname output to buf. */

STRING __ExpandPath(const STRING& filePath)
{
  STRING name (filePath);
  if (name.IsEmpty())
    return name;
/*
  WIN32 Problem:
  "\" is path separator but is also escape character
  Solving this conflict here is impossible, because we can never decide if a "\"
  is meant as path separator or as escape character.
  We expect all expandible pathes to have either "/" as path separators
  or have the "\" properly escaped and supply the functions "EscapePath"
  and "UnEscapePath" for this reason.
*/ 

  char            path[FILENAME_MAX];
  char            lnm[FILENAME_MAX];
#ifndef _WIN32
  char            buffer[FILENAME_MAX];
#endif
  register char  *name_copy = name.NewCString (); // Make a scratch copy

  register const char  *s = name_copy;
  register char  *d = lnm;

  path[0] = '\0';
  /* Expand inline environment variables */
  while ((*d++ = *s) != 0) {
    if (*s == '\\') {
      if ((*(d - 1) = *++s) != 0) {
	s++;
	continue;
      } else
	break;
    } else if (*s++ == '$') {
      register char  *start = d;
      register int    braces = (*s == '{' || *s == '(');
      register char  *value;
      while ((*d++ = *s) != 0)
	if (braces ? (*s == '}' || *s == ')') : !(isalnum(*s) || *s == '_'))
	  break;
	else
	  s++;
      *--d = 0;
#ifdef _WIN32
	  //Hadle some special "environment" here
	  //Beware:
	  //The paths returened are all absolute
	  /* ??????????????????????????????
	   * Maybe edz could supply some UNIX mapping?
	  */
	  STRING sEnv(braces ? start + 1 : start);
	  if(sEnv.CaseEquals("MyFiles"))
	  	value =  GetUserHome(NulString).GetCString();
	  else if(sEnv.CaseEquals("ProgramDir"))
	  	value = _winRegKeyMachine("Software\\Microsoft\\Windows\\CurrentVersion", "ProgramFilesDir").GetCString();
	  else if(sEnv.CaseEquals("CommonAppDir")||sEnv.CaseEquals("CommonAppData"))
	  	value = _winRegKeyMachine("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Common AppData").GetCString();
	  else if(sEnv.CaseEquals("CommonFilesDir")) 
	  	value = _winRegKeyMachine("Software\\Microsoft\\Windows\\CurrentVersion", "CommonFilesDir").GetCString();
	  else
#endif
      value = getenv(braces ? start + 1 : start);
      if (value) {
	for (d = start - 1; (*d++ = *value++) != 0;);
	d--;
	if (braces && *s)
	  s++;
      }
    }
  }

  /* Expand ~ and ~user */
  register char *nm = lnm;
  s = "";
#ifndef _WIN32
  int             q = (name_copy[0] == '\\' && name_copy[1] == '~');
  /* Note:
   * It is dangerous to handle ~ in Windows, because it has no special
   * meaning and may be part of many pathes and filenames. I is especially
   * used by the system when converting lon pathes to short 8.3 pathes
  */ 
  if (nm[0] == '~' && !q) {
    /* prefix ~ */
    if(IsPathSep(nm[1]) || nm[1] == 0) {	/* ~/filename */
      if ((s = _GetUserHome(buffer, NULL)) != NULL) {
	if (*++nm)
	  nm++;
      }
    } else {			/* ~user/filename */
	register const char  *home;
	char                 *tcp;
	for (tcp = nm; *tcp && !IsPathSep(*tcp); tcp++)
	  /* loop */;
	char *nnm = *tcp ? tcp + 1 : tcp;
	*tcp = '\0';
	if ((home = _GetUserHome(buffer, nm + 1)) != NULL)
	  nm = nnm;
	s = home;
    }
  }
  else if(!IsPathSep(nm[0])) {
    s = GetCwd(); 
  }
#else
  if(!IsAbsoluteFilePath(nm)) {
    s = GetCwd(); 
  }
#endif

  // Copy the bits
  d = path;
  if (s && *s) {
    /* Copy home dir */
    while ('\0' != (*d++ = *s++))
       /* loop */ ;
    // Handle root home
    if (d - 1 > path && !IsPathSep(*(d - 2)) && !IsPathSep(nm[0]))
    {
       *(d - 1) = SEP;
    }
    else if(d > path)
    	d--;
  }
  
  s = nm;
  while ((*d++ = *s++) != 0) /* loop */;

  /* Now clean up the buffer */
  delete[]          name_copy;		// clean up alloc

#ifdef _WIN32
  return STRING(FixMicrosoftPathNames(_Realpath(path)));
#else
  return STRING(_Realpath(path));
#endif
}

void ExpandFileSpec(STRING *Ptr)
{
  if (Ptr)
    *Ptr = ExpandFileSpec(*Ptr);
}

STRING ChDir()
{
  STRING Cwd (GetCwd());
  if (Cwd.IsEmpty()) return NulString;
  return ChDir(Cwd);
}

STRING ChDir(const STRING& Dir)
{
  if (Dir.IsEmpty())
    return ChDir();
  if (chdir(Dir) == 0) {
    char        Cwd[FILENAME_MAX];
    if (getcwd (Cwd, sizeof(Cwd)) != NULL)
      {
#ifdef _WIN32
#endif
	return Cwd;
      }
  }
  return NulString; // Can't
}


// This gets called early to cache the dir
STRING GetCwd()
{
  return SetCwd(NulString);
}

STRING SetCwd(const STRING& Dir)
{
  static STRING cwd; // Cache Current Working Directory for whole run!

  if (Dir.GetLength())
    cwd = ChDir(Dir);

  if (cwd.IsEmpty())
    {
      char        Cwd[FILENAME_MAX];
      const char *wd = getcwd(Cwd, sizeof(Cwd)-2);
      if (wd == NULL || *wd == '\0')
	{
	  message_log (LOG_ERRNO, "Can't determine current working directory!");
	}
      else
	{
#ifdef _WIN32
	  cwd = WinGetLongPathName(wd);
#else
	  cwd = wd;
#endif
	}
    }
  return cwd;
}


////

STRING ExpandFileSpec (const STRING& Fullpath)
{
  static STRING oldSpec, oldPath; // Cache for Multiple Record Documents!
#ifdef _WIN32
  static STRING oldShortSpec;  // Windows can have 2 names for same..

  if (Fullpath == oldSpec || Fullpath == oldShortSpec)
    return oldPath; // Cached Spec

#else
  if (Fullpath == oldSpec) return oldPath; 
#endif

  if (Fullpath.IsEmpty())
    {
      // Free allocations
      oldSpec.Empty();
#ifdef _WIN32
      oldShortSpec.Empty();
#endif
      oldPath.Empty();
      return NulString; // "" -> "" 
    }

  STRING        FileSpec;
#ifdef _WIN32
  if ((FileSpec = WinGetLongPathName(Fullpath)) == oldSpec)
    {
     oldShortSpec = Fullpath;
      return oldPath;
    }
#else
  FileSpec = Fullpath;
#endif

  oldSpec = FileSpec;

#ifdef _WIN32
  oldShortSpec = Fullpath;

  if(IsAbsoluteFilePath(FileSpec))      return oldPath = FixMicrosoftPathNames(FileSpec);

  //If it is not an absolute path, but it is containing ":" we can not handle it
  //The result would probably be incorrect
  if(FileSpec.Find(":") >= 0)		return FileSpec;
  FileSpec.Replace("\\", "/");  // Make \ into /
#else

  if(IsAbsoluteFilePath(FileSpec))      return oldPath = FileSpec;

#endif
 
#ifndef _WIN32
  /* Note:
   * It is dangerous to handle ~ in Windows, because it has no special
   * meaning and may be part of many pathes and filenames. I is especially
   * used by the system when converting lon pathes to short 8.3 pathes
  */ 
  if (FileSpec.GetChr(1) == '~')
    return oldPath = __ExpandPath(FileSpec);
#endif

  // Get the Current Working Directory to map blah/foo to $pwd/blah/foo
  const STRING cwd (GetCwd());

  // Just the current directory?
  if (FileSpec.GetChr(1) == '.')
    {
      // ".", "./" are current directory
      size_t len = FileSpec.GetLength();
      if (len == 1 || (len == 2 && IsPathSep(FileSpec.GetChr(2))))
	return oldPath = cwd;
    }

#ifdef _WIN32
  if(IsPathSep(FileSpec.GetChr(1)))
  	oldPath = STRING(_Realpath((char *)(cwd + (FileSpec)).c_str()));
  else
  	oldPath = STRING(_Realpath((char *)(AddTrailingSlash(cwd) + (FileSpec)).c_str()));
  while(oldPath.Replace("//", "/"));
  while(IsPathSep((oldPath.Right((size_t)1)).GetChar(0))) {oldPath = oldPath.EraseAfter(oldPath.GetLength()-1);}
  oldPath = FixMicrosoftPathNames(oldPath);
  return oldPath;
#else
  return oldPath = STRING(_Realpath((char *)(AddTrailingSlash(cwd) + (FileSpec)).c_str()));
#endif
}


STRING __Realpath(const STRING& path)
{
  PCHR    scratch = path.NewCString();
  STRING  result (_Realpath (scratch) );
  delete[] scratch;
  return result;
}

void __Realpath(STRING* path)
{
  if (path && path->GetLength() > 1)
    {
      STRING realpath ( __Realpath(*path) );
      *path = realpath;
    }
}


#ifdef _WIN32
int winMkDir(const STRING& Dir)
{
	STRING  sDir(Dir);
	sDir.Replace("/", "\\");

	int nLastError = 0;
	GDT_BOOLEAN bIsUNC = Dir.Left((size_t)2) == "\\\\"; 
	if(bIsUNC)	sDir = sDir.Mid(2);
	
	ArraySTRING	aDir;
	size_t n = sDir.GetLength();
	size_t iSep = 0;
	for(size_t i=0; i<n; i++)
	{
		if(sDir.GetChar(i) == '\\' && i>0)
		{
			aDir.Add(sDir.Mid(iSep, i-iSep));
			iSep = i+1;
		}
	}
	//Remainder
	if(iSep < n) aDir.Add(sDir.Mid(iSep));

	//No entries
	n = aDir.Count();
	if(n == 0) return 0;
	
	//Some specials
	size_t iStart = 0;
	if(bIsUNC)
	{
		aDir.SetEntry(1, "\\\\" + aDir.GetEntry(1));
		sDir = aDir.GetEntry(1);
		iStart = 2;
	}
	else if(aDir.GetEntry(1).Find(':') > 0)
	{
		sDir = aDir.GetEntry(1);
		iStart = 2;
	}
	else
	{
		sDir = "";
		iStart = 1;
	}

	for(size_t i=iStart;i<=n;i++)
    {
		if(sDir.IsEmpty())	sDir  = aDir.GetEntry(i);
		else				sDir += "\\" + aDir.GetEntry(i);
		if(CreateDirectory(sDir, NULL))
			nLastError = 0;
		else
		{
			if(_access(sDir, 0) == 0)
				nLastError = EEXIST;
			else
			{
				nLastError = ENOENT;
				break;
			}
		}
	}
	if(nLastError)
	{
		errno = nLastError;
		return -1;
	} 
	return 0;
}
#endif

int MkDir(const STRING& Dir, int Mask, GDT_BOOLEAN Forced)
{
  if (Forced == GDT_FALSE)
    return MkDir(Dir, Mask);

  struct stat stbuf;
  // Here we do forced
  if (stat(Dir, &stbuf) == -1 || !S_ISDIR(stbuf.st_mode))
    {
      unlink(Dir); // ignore result
#ifdef _WIN32
# define DefMask 0666 
#else
# define DefMask (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif
      const int mask =  Mask == 0 ? DefMask : Mask;
      if (MKDIR(Dir, mask) == -1)
        {
          return -1;
        }
    }
  return 0;
}

int MkDir(const STRING& Dir, int Mask)
{
  char       *dir = Dir.NewCString();
  struct stat st_buf;
  int         res;
  const int   mask = ( Mask == 0 ? DefMask : Mask);

  if (0 == stat(Dir.c_str(), &st_buf))
    res = S_ISDIR(st_buf.st_mode) ? 0 : -1;
  else
    res = MKDIR(Dir.c_str(), mask);
  delete [] dir;
  return res;
}

#ifdef _WIN32
GDT_BOOLEAN MkDirs(const STRING& Path, int Mask)
{
  int mask = (Mask == 0 ? DefMask : Mask);
  /*
   * Path expansion is not supported in WIN32 here.
   * Escape path, expand and unescape it before calling.
   */
  if(MKDIR(Path, mask) == 0 || errno == EEXIST) return GDT_TRUE;
    return GDT_FALSE;
}
#else
GDT_BOOLEAN MkDirs(const STRING& Path, int Mask)
{
  int         res = 0;
  struct stat st_buf;
  STRING      Scratch (Path);
  const int   mask = ( Mask == 0 ? DefMask : Mask);

  ExpandFileSpec(&Scratch);

  PCHR fname = Scratch.NewCString ();
  for(PCHR ptr=strchr(fname,'/');ptr;ptr=strchr(ptr+1,'/'))
    {
      *ptr='\0';
      if (*fname) {
      if ((res=stat((const char *)fname, &st_buf)) < 0)
	{
	  if ((res = MKDIR(fname,mask|S_IEXEC)) < 0)
	    {
	      break; // Could not create
	    }
         }
      else if (! S_ISDIR(st_buf.st_mode))
	{
	  res = -1; // Not a directory so can't continue
	  break;
	}
      }
      *ptr='/';
   }
   delete[] fname;
   return (res == 0);
}
#endif
#undef DefMask



#if 0
/*-
 Handles:
   ~/ => home dir
   ~user/ => user's home dir
   If the environment variable a = "foo" and b = "bar" then:
	$a	=>	foo
	$a$b	=>	foobar
	$a.c	=>	foo.c
	xxx$a	=>	xxxfoo
	${a}!	=>	foo!
	$(b)!	=>	bar!
	\$a	=>	\$a
 */

/* input name in name, pathname output to buf. */

STRING __ExpandPath(const STRING& filePath)
{
  STRING name (filePath);
  if (name.IsEmpty())
    return name;

#ifdef _WIN32
  /* \\drive\ must remain in that form */
  if (name.GetChr(1) == '\\' && name.GetChr(2) == '\\')
    {
       name.Replace("/", "\\");
       return name;
    }
  name.Replace("\\", "\\\\"); // make \xx\xxx -> \\xx\\xxx
#endif

  char            path[FILENAME_MAX];
  char            lnm[FILENAME_MAX];
  char            buffer[FILENAME_MAX];
  register char  *name_copy = name.NewCString (); // Make a scratch copy

  register char  *s = name_copy;
  register char  *d = lnm;

  path[0] = '\0';
  /* Expand inline environment variables */
  while ((*d++ = *s) != 0) {
    if (*s == '\\') {
      if ((*(d - 1) = *++s) != 0) {
	s++;
	continue;
      } else
	break;
    } else if (*s++ == '$') {
      register char  *start = d;
      register int    braces = (*s == '{' || *s == '(');
      register char  *value;
      while ((*d++ = *s) != 0)
	if (braces ? (*s == '}' || *s == ')') : !(isalnum(*s) || *s == '_'))
	  break;
	else
	  s++;
      *--d = 0;
      value = getenv(braces ? start + 1 : start);
      if (value) {
	for (d = start - 1; (*d++ = *value++) != 0;);
	d--;
	if (braces && *s)
	  s++;
      }
    }
  }

  /* Expand ~ and ~user */
  register char *nm = lnm;
  int             q = (name_copy[0] == '\\' && name_copy[1] == '~');
  s = "";
  if (nm[0] == '~' && !q) {
    /* prefix ~ */
    if (nm[1] == SEP || nm[1] == 0) {	/* ~/filename */
      if ((s = (char *)_GetUserHome(buffer, NULL)) != NULL) {
	if (*++nm)
	  nm++;
      }
    } else {			/* ~user/filename */
      register char  *nnm;
      register char  *home;
      for (s = nm; *s && *s != SEP; s++) /* loop */;
      nnm = *s ? s + 1 : s;
      *s = 0;
      if ((home = (char *)_GetUserHome(buffer, nm + 1)) != NULL)
	nm = nnm;
      s = home;
    }
  } else if (nm[0] != SEP) {
    s = GetCwd();
  }

  // Copy the bits
  d = path;
  if (s && *s) {
    /* Copy home dir */
    while ('\0' != (*d++ = *s++))
       /* loop */ ;
    // Handle root home
    if (d - 1 > path && *(d - 2) != SEP)
      *(d - 1) = SEP;
  }
  s = nm;
  while ((*d++ = *s++) != 0) /* loop */;

  delete[]          name_copy;		// clean up alloc
  /* Now clean up the buffer */
  STRING Result (_Realpath(path));
#ifdef _WIN32
  return FixMicrosoftPathNames(Result);
#else
  return Result;
#endif
}
#endif

static int FileCopy(const char *name1, const char *name2)
{
  FILE *infp;
  FILE *outfp;

  if ((infp = fopen(name1, "rb")) == NULL)
    return -1;

  if ((outfp = fopen(name2, "wb")) != NULL)
    {
      int ch;
      while ((ch = getc(infp)) != EOF)
        putc(ch, outfp);
      fclose(infp);
      fclose(outfp);
      return 0; // OK
    }
  fclose(infp);
  return -1;
}

// Fn 
int FileLink(const STRING& Source, const STRING& Dest)
{
#ifndef _WIN32
  if (link(Source, Dest) < 0)
#endif
    {
      // Can't link so copy
      MMAP mapping (Source);
      if (!mapping.Ok())
        {
          message_log(LOG_FATAL|LOG_ERRNO, "Couldn't map '%s' into memory", Source.c_str());
          return FileCopy(Source, Dest);
        }
      mapping.Advise(MapSequential);
      const UCHR *Buffer  = (const UCHR *)mapping.Ptr();
      const size_t MemSize = mapping.Size();

      FILE *fp;
  
      if ((fp = fopen(Dest, "wb")) == NULL)
        {
          message_log (LOG_ERRNO, "Could not create file stream '%s'", Dest.c_str());
          return -1;
        }
      errno = 0;
      size_t length = fwrite(Buffer, sizeof(char), MemSize, fp);

      mapping.Unmap(); // Clear map

      fclose(fp);
      if (length < MemSize)
	{
          message_log (LOG_WARN|LOG_ERRNO, "File: '%s' short write by %d bytes", Dest.c_str(),
                (int)(MemSize-length));
	}
    }
  return 0;
}


#ifndef bswap16
# define bswap16(_x)  (((_x) << 8) & 0xff00) | (((_x) >> 8) & 0x00ff);
#endif
#ifndef bswap32
# define bswap32(_x) (((_x) << 24) & 0xff000000 ) | \
	(((_x) <<  8) & 0x00ff0000 ) | ((_x >>  8) & 0x0000ff00 ) | \
	(((_x) >> 24) & 0x000000ff )
#endif

UINT2 Swab (UINT2 *Ptr)
{
 return  *Ptr =  bswap16(*Ptr);
}

INT2 Swab (INT2 *Ptr)
{
 return  *Ptr =  bswap16(*Ptr);
}

UINT4 Swab (UINT4 *Ptr)
{
  return *Ptr = bswap32(*Ptr);
}

INT4 Swab (INT4 *Ptr)
{
  return *Ptr = bswap32(*Ptr);
}

UINT8 Swab (UINT8 *Ptr)
{
#ifdef bswap64
  return *Ptr = bswap64(*Ptr);
#else /* Don't have a bswap64 call */
#if HOST_MACHINE_64
  /* 64-bit implementation */
  register UINT8   x = *Ptr;
  return *Ptr =
	( (x << 56) & 0xff00000000000000UL ) |
	( (x << 40) & 0x00ff000000000000UL ) |
	( (x << 24) & 0x0000ff0000000000UL ) |
	( (x <<  8) & 0x000000ff00000000UL ) |
	( (x >>  8) & 0x00000000ff000000UL ) |
	( (x >> 24) & 0x0000000000ff0000UL ) |
	( (x >> 40) & 0x000000000000ff00UL ) |
	( (x >> 56) & 0x00000000000000ffUL );
#else
  /* 32-bit implementation */
  UINT8 x = *Ptr;
  UINT4 th = bswap32((UINT4)(x         & 0x00000000ffffffffULL));
  UINT4 tl = bswap32((UINT4)((x >> 32) & 0x00000000ffffffffULL));
  return *Ptr = ((UINT8)th << 32) | tl;
#endif
#endif
}

INT8 Swab (INT8 *Ptr)
{
#ifdef bswap64
  return *Ptr = bswap64(*Ptr);
#else
  return (INT8)Swab((UINT8 *)Ptr);
#endif
}

ssize_t _sys_read(int fildes, void *buf, size_t nbyte)
{
  size_t  length = 0;
  ssize_t x      = 0;
  do {
    if ((x = read (fildes, buf, nbyte)) == -1) // Read Error
      break;
    length += x;  
    nbyte  -= x; 
  } while (x> 0 && nbyte>0);
  if (length == 0 && x == -1)
      return -1; 
  return length;   
}

#ifdef NEED_PREAD

ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset)
{
  if (-1 == lseek (fildes, offset, SEEK_SET))
    return -1;
  return _sys_read(fildes, buf, nbyte);
}

#endif

size_t pfread(FILE *fp, void *buf, size_t nbyte, off_t offset)
{
  size_t length = 0;
  if (fseek (fp, offset, SEEK_SET) != -1)
    {
      size_t x = 0;
      do {
	x = (size_t)fread (((char *)buf) + length, sizeof(char), nbyte, fp);
	length += x;
	nbyte  -= x;
     } while (x>0 && nbyte>0);
   } 
  ((char *)buf)[length] = '\0';
  return length;
}

#ifndef _WIN32

struct timeval *tread_timeval = NULL;

struct timeval *tReadSetTimeout(struct timeval *timeval)
{
  if (timeval == (struct timeval *)(-1))
    {
      static struct timeval tv;
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      tread_timeval = &tv;
    }
  else if (timeval)
    {
      tread_timeval = timeval;
    }
  return tread_timeval;
}

size_t tRead(int fd, void *bufptr, size_t length)
{
  if (tread_timeval == NULL)
    {
      tReadSetTimeout((struct timeval *)-1);
    }
  return tRead(fd, bufptr, length, tread_timeval);
}

size_t tRead(int fd, void *bufptr, size_t length, struct timeval *tv)
{
  size_t want = length;

  for (char *buf = (char *)bufptr; want > 0; )
    {
      int bytesRead;

      if (tv)
	{
	  fd_set readSet;

	  FD_ZERO(&readSet);
	  FD_SET(fd, &readSet);

	  if (select(fd+1, &readSet, NULL, NULL, tv) != 1) 
	    break; // Error
	}
      if ((bytesRead = (int)read(fd, buf, want)) != -1)
	{
	  if (bytesRead == 0)
	    break;
	  buf   += (size_t)bytesRead;
          want  -= (size_t)bytesRead;
	}
      else
	break; // Error
    }
  return length - want;
}
#endif

///////////////////////////////////////////////////////////////////


GDT_BOOLEAN IsBigEndian ()
{
  UINT2 Test = 1;
  return (*((PUCHR) (&Test)) == 0) ? GDT_TRUE : GDT_FALSE;
}

// Like strdup()
char *Copystring (const char *s)
{
  return Copystring(s == NULL ? "" : s, strlen ((const char *)s));
}

// Like strdup()   
char *Copystring (const char *s, size_t len)
{
  char *news;

  if (s == NULL || (len == 0 && *s))
    return Copystring(s);

  len++;
  try {
    news = new CHR[len];
  } catch (...) {
    news = NULL;
  }   
  if (news)
    {
      memcpy (news, s, len);      // Should be the fastest
      news[len] = '\0';
    }
  return news;  
}




// General C String functions

// Thanks to Edward C. Zimmermann for fixes to these functions.

INT StrCaseCmp (const CHR *s1, const CHR *s2)
{
  return StrCaseCmp ((UCHR *) s1, (UCHR *) s2);
}

INT StrCaseCmp (const UCHR *p1, const UCHR *p2)
{
  INT diff;

  if (p1 == NULL || p2 == NULL)
    { 
      message_log (LOG_PANIC, "Nilpointer passed to StrCaseCmp (\"%s\", \"%s\")!",   
        p1 ? (const char *)p1 : "NULL",   
        p2 ? (const char *)p2 : "NULL");
      return p1 - p2;
    }
  while ((diff = ((UCHR) _ib_tolower (*p1) - (UCHR) _ib_tolower (*p2))) == 0)
    {
      if ((*p1 == '\0') && (*p2 == '\0'))
	break;
      p1++, p2++;
    }
  return diff;
}

INT StrNCaseCmp (const CHR *s1, const CHR *s2, const INT n)
{
  return StrNCaseCmp ((UCHR *) s1, (UCHR *) s2, n);
}

INT StrNCaseCmp (const UCHR *p1, const UCHR *p2, const INT n)
{
  if (p1 == NULL || p2 == NULL)
    {
      message_log (LOG_PANIC, "Nilpointer passed to StrNCaseCmp (\"%s\", \"%s\", %d)!",
	p1? (const char*)p1 : "NULL",
	p2? (const char*)p2 : "NULL",
	(int)n);
      return p1 - p2;
    }

  INT diff = 0;
  INT    x = 0;
  while ( (isspace(*p1) && isspace(*p2)) ||
        (diff = ((UCHR) _ib_tolower (*p1) - (UCHR) _ib_tolower (*p2))) == 0)
    {  
      if ((++x >= n) || ((*p1 == '\0') && (*p2 == '\0')))
        break;
      p1++, p2++;
    }
  return diff;
}


INT StrNCmp (const CHR *s1, const CHR *s2, const INT n)
{
  return StrNCmp ((UCHR *) s1, (UCHR *) s2, n);
}

INT StrNCmp (const UCHR *p1, const UCHR *p2, const INT n)
{
  INT diff;
  INT x = 0;
  while ((diff = (*p1 - *p2)) == 0)
    {
      if ((++x >= n) || ((*p1 == '\0') && (*p2 == '\0')))
	{
	  break;
	}
      p1++, p2++;
    }
  return diff;
}


GDT_BOOLEAN FileGlob(const UCHR *pattern, const UCHR *str)
{
#ifdef _WIN32
  //Allways convert "\" in str to "/"
  // ==> For globbing allways use "/" for path separators in patterns
  STRING sstr(str);
  sstr.Replace("\\", "/");
  return Glob(pattern, sstr.ToLower(), GDT_TRUE);
#else
  return Glob(pattern, str, GDT_TRUE);
#endif
}




// Not MT Safe (ISO 8601 date/time format)
// <YYYY>-<MM>-<DD>T<HH>:<MM>:[<SS>]Z
char *ISOdate(time_t t)
{
  // Ring of buffers to try to outwit side-effects
  static int const   bufSize        = 24;
  static int const   numBuffers     = 8;
  static char        buffer[ numBuffers ][ bufSize ];
  static int         n;              // which buffer to use
  char   *buf        = buffer[n];

  if (t == 0) t = time(NULL);
  struct tm *tm = gmtime (&t);
  sprintf(buf, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  sprintf(buf+10, "T%02d:%02d", tm->tm_hour, tm->tm_min);
  if (tm->tm_sec)
    sprintf(buf+16, ":%02d", tm->tm_sec);
  strcat(buf, "Z"); // For GMT
  n++; n = (n % numBuffers);
  return buf;
}

// Not MT Safe obsolete ANSI-style
// YYYYMMDD HH:MM[:SS] GMT
char *ANSIdate(time_t t)
{
  // Ring of buffers to try to outwit side-effects
  static int const   bufSize        = 24;
  static int const   numBuffers     = 8;
  static char        buffer[ numBuffers ][ bufSize ];
  static int         n;              // which buffer to use
  char   *buf        = buffer[n];

  if (t == 0)
    t = time(NULL);

  struct tm *tm = gmtime (&t);
  // YYYYMMDD HH:MM GMT
  sprintf(buf, "%04d%02d%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  if (tm->tm_hour || tm->tm_min || tm->tm_sec)
    {
      sprintf(buf+8, " %02d:%02d", tm->tm_hour, tm->tm_min);
      if (tm->tm_sec)
	sprintf(buf+14, "%02d", tm->tm_sec);
      strcat(buf, " GMT");
    }
  n++; n = (n % numBuffers);
  return buf;
}

// Not MT Safe
char *RFCdate(time_t t)
{
  // Ring of buffers to try to outwit side-effects
  static int const   bufSize        = 40;
  static int const   numBuffers     = 6;
  static char        buffer[ numBuffers ][ bufSize ];
  static int         n;              // which buffer to use
  char   *buf        = buffer[n];


  // Need US names */
  const char *days[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
     "Aug", "Sep", "Oct", "Nov", "Dec" };
  if (t == 0) t = time(NULL);
  struct tm *tm = gmtime (&t);

  sprintf(buf, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
	days[tm->tm_wday],
	tm->tm_mday,
	months[tm->tm_mon],
	tm->tm_year+1900,
	tm->tm_hour, tm->tm_min, tm->tm_sec);
  return buf;
}

// Not MT Safe
char *LCdate(time_t t)
{
  // Ring of buffers to try to outwit side-effects
  static int const   bufSize        = 60;
  static int const   numBuffers     = 4;
  static char        buffer[ numBuffers ][ bufSize ];
  static int         n;              // which buffer to use
  char  *buf         = buffer[n];

  if (t == 0) t = time(NULL);
  struct tm *tm = gmtime (&t);
// 31. Oktober 1997, 18:56:47 Uhr GMT
  if (tm->tm_hour == 0 && tm->tm_min == 0 && tm->tm_sec == 0)
    strftime (buf, sizeof (buf), "%a, %d %h %Y", tm);
  else
    strftime (buf, sizeof (buf), "%a, %d %h %Y %X %Z", tm);
  return buf;
}

#ifdef WWW_SUPPORT
#include <string.h>
#ifndef _WIN32
# include <netdb.h>
#endif

#if defined(SVR4) && !defined(LINUX)
#include <sys/systeminfo.h>
#endif

#if defined(SVR4)
//extern "C"
//{
//  int getdomainname (char *domain, unsigned const namelen);
//}
#endif

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

int getHostname (char *buf, int len)
{
  static char name[MAXHOSTNAMELEN+1];

  if (name[0])
    {
      strncpy(buf, name, len);
      buf[len] = '\0';
      return 0;
    }
  struct hostent *h;
  // Do we have a WWW server 
  char *hostname = getenv ("SERVER_NAME");
  if (hostname && hostname)
    {
      // Virtual Servers should ALWAYS return fully qualified
     // server names!
      if (strchr (hostname, '.') == NULL)
	{
	  // Could not get domainname so get official hostname
	  strncpy (name
	      ,(h = gethostbyname (hostname)) != NULL ? h->h_name : hostname
		       ,sizeof (name) - 1);
	  hostname = name;
	}
    }
  else
    {
      // Get hostname
#if defined(SVR4) && !defined(LINUX)
#define gethostname(_name, _size) sysinfo (SI_HOSTNAME, _name, _size)
#endif
      if (gethostname (name, sizeof (name) / sizeof (char) - 1) != -1)
	{
	  if ((h = gethostbyname (name)) != NULL)
	    {
	      hostname = h->h_name;
	      // Search if WWW Alias exists
	      for (size_t i = 0; h->h_aliases[i]; i++)
		{
		  if (StrNCaseCmp (h->h_aliases[i], "WWW", 3) == 0
		      && ((h->h_aliases[i])[3] == '.' || (h->h_aliases[i])[3] == '\000'))
		    {
		      hostname = h->h_aliases[i];
		      break;
		    }
		}
	    }
	  // Make sure that we have a domain name
	  if (hostname && strchr (hostname, '.') == NULL)
	    {
	      // Fetch our domain
	      char domain[64];
	      if (getdomainname (domain, sizeof (domain) / sizeof (char) - 1) != -1)
		{
		  strcpy (name, hostname);
		  if (domain[0] != '.')
		    strcat (name, ".");
		  strcat (name, domain);
		  hostname = name;	// Now point to name

		}
	    }
	}
    }

  if (hostname == NULL || *hostname == '\0')
    return -1;
  strncpy(name, hostname, MAXHOSTNAMELEN);
  name[MAXHOSTNAMELEN] = '\0';
  strncpy(buf, name, len);
  buf[len] = '\0';
  return 0;
}

#else


int getHostname (char *buf, int len)
{
  return -1;
}

#endif

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 1024
#endif

#ifdef _WIN32
int getOfficialHostName(char *buf, int len)
{
  strncpy(buf, "localhost", len);
  return 0;
}

#else
int getOfficialHostName(char *buf, int len)
{
  static char name[MAXHOSTNAMELEN+1];

  if (name[0])
    {
      strncpy(buf, name, len);
      buf[len] = '\0';
      return 0;
    }
  // Get fully qualified hostname e.g. foo.bar.edu
  if (gethostname (name, sizeof (name) / sizeof (char) - 1) != -1)
    {
      // Get official full name of host
      struct hostent *h = gethostbyname (name);
      if (h != NULL)
	{
	  strncpy(name, h->h_name, MAXHOSTNAMELEN);
	  name[MAXHOSTNAMELEN] = '\0';
	  strncpy(buf, name, len);
	  buf[len] = '\0';
	  return 0;
	}
    }
  return -1;
}
#endif

////////////////////////////// MERGE FROM HERE ////////////////////////////////////
///


STRING GetTempDir()
{
	static STRING tempDir;
	static const char *dirs[] = {
#ifdef _WIN32
	"$TEMP",
	"/temp",
#else
	"$TMPDIR",
#endif
	"/tmp",
	"/var/tmp",
	NULL
	};

	if (tempDir.IsEmpty())
	{
#ifdef _WIN32
		char tempDirShort[FILENAME_MAX];
		if(::GetTempPath(sizeof(tempDirShort)/sizeof(char), tempDirShort))
		{
		  tempDir = WinGetLongPathName(tempDirShort);
		}
		else
#endif
		{
			STRING S;
			for (size_t i=0; dirs[i]; i++)
			{
				if (WritableDir (S = __ExpandPath(dirs[i])))
				{   
#ifdef LIBCONTEXT_SHI
#else
					AddTrailingSlash(&S);
#endif
					// Test that we can indeed write a file..
					STRING testfile;
					for (int i=0; i < 1000; i++)
					{
						testfile.form("%stmptest.%d", S.c_str(), i);
						if (!Exists(testfile)) break;
					}
					// will assume we have a name if not then its the 1000
					FILE *fp = testfile.fopen("w");
					if (fp != NULL)
					{
						// It opens!! OK!
						fclose(fp);
						unlink(testfile);
						tempDir = S;
						break;
					}
				}
			}
		}
    }
	return tempDir;
}

STRING GetTempFilename(const char *prefix, GDT_BOOLEAN Secure)
{
#ifdef _WIN32
    STRING filename;
    char tmp[FILENAME_MAX];
    if(GetTempFileName(GetTempDir(), prefix, 0, tmp))
    	return tmp;
    else
    	filename = GetTempDir();
#else
    STRING       filename = GetTempDir();
#endif

  STRING       tfilename;
  long         pid = getpid();
  int          clk = (int)((clock() + random()) & 0xFFFF); //added random
  const char   * const alphabet = "abcdefghijklmnopqrstuvwxyz";
  static short cache = 0;
  if (prefix && *prefix) filename.Cat (prefix);

  tfilename.form("%04x%c%c", (int)pid,
	alphabet[clk % 26],       alphabet[(clk >> 8) % 26]);
  filename.Cat (tfilename);
  for (size_t i=0; i < 1000; i++)
    {
      int j = (i + cache) % 1000;
      tfilename = filename;
      if (j)
	{
	  // tfilename.Cat(".");
	  tfilename.Cat(j);
	}
      if (!Exists(tfilename))
	{
	  if (Secure)
	    {
#ifdef WIN32
	      int fd = open(filename, O_CREAT|O_TRUNC|O_RDWR);

#else
	      int fd = open (tfilename, O_CREAT|O_EXCL|O_TRUNC|O_RDWR, 0600);
#endif
	      if (fd == 0) close(fd);
	    }
	  cache = (short)(j+1);
	  return tfilename; 
	}
    }
  message_log (LOG_PANIC, "Problems creating temporary file names.");
  if (prefix && *prefix)
    {
      // Maybe path too long?
      return GetTempFilename(NULL);
    }
  return filename + "nonmonotonic.tmp";
}



#ifdef _WIN32
GDT_BOOLEAN _IB_GetPlatformName(char *buf, int maxSize)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(!GetVersionEx(&osvi))
	{
		char *sysname;
		if((sysname = getenv("OS")) == NULL)
		{
			buf[0] = '\0';
			return GDT_FALSE;
		}
		strncpy(buf, sysname, maxSize);
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//WARNING:
		//Shouldn´t this be buf[maxSize-1] = '\0'; ???????????? 
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		buf[maxSize] = '\0';
		return GDT_TRUE;
	}

	STRING sosvi;
	if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sosvi = "Windows 95";
	else if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10 && osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sosvi = "Windows 98";
	else if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90 && osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		sosvi = "Windows ME";
	else if(osvi.dwMajorVersion == 3)
		sosvi = "Windows NT 3";
	else if(osvi.dwMajorVersion == 4 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		sosvi = "Windows NT 4";
	else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		sosvi = "Windows 2000";
	else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
		sosvi = "Windows XP";
	else if(osvi.dwMajorVersion == 6)
		sosvi = "Windows VISTA";

	STRING stmp(osvi.szCSDVersion);
	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if(!stmp.IsEmpty()) sosvi += " " + stmp;
		stmp = osvi.dwBuildNumber;	sosvi += " Build " + stmp;
	}
	else
	{
		if(!stmp.IsEmpty()) sosvi += " " + stmp;
		stmp = (DWORD)LOBYTE(HIWORD(osvi.dwBuildNumber));	sosvi += " " + stmp;
		stmp = (DWORD)LOBYTE(HIWORD(osvi.dwBuildNumber));	sosvi += "." + stmp;
		stmp = LOWORD(osvi.dwBuildNumber);					sosvi += " Build " + stmp;
	}	

	strncpy(buf, sosvi, maxSize);

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//WARNING:
	//Shouldn´t this be buf[maxSize-1] = '\0'; ???????????? 
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	buf[maxSize] = '\0';

 	return GDT_TRUE;
}

// Get full hostname (eg. icebear.nonmonotonic.net)
GDT_BOOLEAN _IB_GetHostName(char *buf, int maxSize)
{
  //Use win api
  DWORD msize = maxSize;
  if(GetComputerName(buf, &msize))
  	return GDT_TRUE;
  	
  //Try with env
  char *sysname;
  if ((sysname = getenv("COMPUTERNAME")) == NULL)	strncpy(buf, "localhost", maxSize);
  else												strncpy(buf, sysname, maxSize);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //WARNING:
  //Shouldn´t this be buf[maxSize-1] = '\0'; ???????????? 
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  buf[maxSize] = '\0';
  return *buf ? GDT_TRUE : GDT_FALSE;
}

// Get user ID e.g. jacs
GDT_BOOLEAN _IB_GetUserId(char *buf, int maxSize)
{
  /* Note:
   * We could get a real ID by using GetUserNameEx.
   * GetUserNameEx, however, is unsupported in Win9x
  */
	
  //Use win api
  DWORD msize = maxSize;
  if(GetUserName(buf, &msize))
  	return GDT_TRUE;

  //Try with env
  char *username;
  if ((username = getenv("USERNAME")) == NULL)	strncpy(buf, "anonymous", maxSize);
  else											strncpy(buf, username, maxSize);
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //WARNING:
  //Shouldn´t this be buf[maxSize-1] = '\0'; ???????????? 
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  buf[maxSize] = '\0';
  return buf && *buf ? GDT_TRUE : GDT_FALSE;
}

// Get user name e.g. Edward Zimmermann 
GDT_BOOLEAN _IB_GetUserName(char *buf, int maxSize)
{
	return _IB_GetUserId(buf, maxSize);
}

long  _IB_Hostid()
{
  /* Note:
   * We could do better, but don´t see any need for it here.
  */
  return 0xFF;
}

UINT winGetPageSize()
{
  static UINT pagesize = 0;
  if(pagesize > 0) return pagesize;
	
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  pagesize = si.dwPageSize; 
  if(!pagesize) pagesize = 4096L;
  return pagesize;
}
#ifndef PAGESIZE
#  define PAGESIZE winGetPageSize()
#endif

// Get free memory in bytes, or -1 if cannot determine amount (e.g. on UNIX)
//
rlim_t  _IB_GetFreeMemory(void)
{
	MEMORYSTATUS _memstat;
	GlobalMemoryStatus(&_memstat);
	//??????????????????????????????????????????????????????????????
	//What do you expect here physical or virtual?
	//For virtual return the dwAvailVirtual member
	return (rlim_t)(_memstat.dwAvailPhys);
}

rlim_t _IB_GetTotalMemory()
{
	MEMORYSTATUS _memstat;
	GlobalMemoryStatus(&_memstat);
	//??????????????????????????????????????????????????????????????
	//What do you expect here physical or virtual?
	//For virtual return the dwTotalVirtual member
	return (rlim_t)(_memstat.dwTotalPhys);
}

GDT_BOOLEAN _IB_GetmyMail (PSTRING Name, PSTRING Address)
{
	/* No way to have an "official" E-Mail adress in Windows*/
	return GDT_FALSE;
}

#else		/* UNIX */

#include <sys/utsname.h>

// UNIX
GDT_BOOLEAN _IB_GetPlatformName(char *buf, int maxSize)
{
  struct utsname name;
  if (uname(&name) == 0)
    {
      char tmp[SYS_NMLN*3+3];
      strcpy(tmp, name.sysname);
      strcat(tmp, " ");
      strcat(tmp, name.release);
      strcat(tmp, " "); 
      strcat(tmp, name.machine);
      strncpy(buf, tmp, maxSize);
      buf[maxSize] = '\0';
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


GDT_BOOLEAN _IB_GetHostName(char *buf, int maxSize)
{
#if defined(SI_HOSTNAME)
  return sysinfo (SI_HOSTNAME, buf, maxSize);
#else
  return gethostname (buf, maxSize - 1) != -1;
#endif
}


// Determine the acurate hostid
long  _IB_Hostid()
{
  static UINT4 hostid =  0x7f000001U; // Loopback
  if (hostid ==          0x7f000001U)
    {
#ifdef SI_HW_SERIAL
      char serial[14];
      if (-1 != sysinfo (SI_HW_SERIAL, serial, sizeof(serial)))
        hostid = atol(serial);
#else
#ifdef HAVE_GETHOSTID
      hostid = gethostid();
#endif
#endif
      if (hostid == 0)
	{
	  message_log (LOG_WARN, "\
Platform seems to be missing a unique hostid! PC Platform? \
Set hostid to the Ethernet MAC address of its main networking card.");
	}
      if (hostid == 0 || hostid == (UINT4)-1 || hostid == 0x7f000001U)
	{
	  // Use the internet address as the hostid
	  char name[MAXHOSTNAMELEN+1];
	  hostid = 0xFF;
	  if (gethostname(name, sizeof(name)/sizeof(char) -1) != -1)
	    {
	      struct hostent *h = gethostbyname (name);
	      if (h)
		{
		  const char *iptr;
		  hostid = 0;
		  for (size_t i = 0; hostid == 0 && (iptr = (const char *)(h->h_addr_list[i])) != NULL; i++)
		    {
		      UINT4 id = 
#ifdef LINUX
			((const struct in_addr*)iptr)->s_addr;
#else
			inet_addr(iptr);
#endif
		      if (id != 0x7f000001U && id != 0x100007fU)
			hostid = id;
		    }
		  if (hostid == 0)
		    {
		      message_log (LOG_ERROR, "Can't determine a valid hostid for this platform!");
		      hostid = STRING(name).Hash() +  0x7f000001U;
		    }
		}
	      else
		{
		  message_log (LOG_ERROR, "No hostid or network address on this platform !?");
		}
	    }
	}
    }
  return hostid; // Return what we've got
}


GDT_BOOLEAN _IB_GetUserId(char *buf, int maxSize)
{
#ifdef BSD
  struct passwd  *pwent = getpwuid(getuid());
  const char *tty = pwent ? pwent->pw_name  : "nobody";
  strncpy(buf, tty, maxSize-1);
  buf[maxSize] = '\0';
  return pwent->pw_name != NULL;
#else
  char tty[L_cuserid+1];
  char *ptr;
  strncpy(buf, ptr = cuserid(tty) ? tty : (char *)"nobody", maxSize-1);
  buf[maxSize] = '\0';  
  return ptr && tty[0];
#endif
}

// UNIX
GDT_BOOLEAN _IB_GetUserName(char *buf, int maxSize)
{
  static char tmp[256];

  if (tmp[0])
    {
      strncpy(buf, tmp, maxSize-1);
      buf[maxSize] = '\0';
      return buf[0] != '\0';
    }
  const char *name = getenv("REAL_NAME");
  if (name == NULL || *name == '\0')
    {
      struct passwd *pwent = getpwuid (getuid ());
      if (pwent)
	{
	  const char *user_id = pwent->pw_name;
	  if (user_id == NULL || *user_id == '\0')
	    {
	      if ((user_id = getenv("USER")) == NULL ||
		(user_id = getenv("LOGNAME")) == NULL || *user_id == '\0')
		pwent = getpwnam(user_id); 
	    }
	  if (pwent == NULL || (name = pwent->pw_gecos) == NULL || *name == '\0')
	    {
	      if (pwent == NULL || (name = pwent->pw_name) == NULL || *name == '\0')
		name = "Unknown User Identity";
	    }
	}
    }
  if (name == NULL || *name == '\0')
    {
      buf[0] = tmp[0] = '\0';
      return GDT_FALSE;
    }
  strncpy(tmp, name, maxSize-1);
  if ((name = getenv("ORGANIZATION")) != NULL)
    {
      strcat(tmp, " (");
      strcat(tmp, name);
      strcat(tmp, ")");
    }
  strncpy(buf, tmp, maxSize-1);
  buf[maxSize] = '\0';
  return GDT_TRUE;
}
#endif


#ifdef _WIN32

//
// In the WIN.INI file

//static const char IB_SECTION[] = "IB";
//static const char eUSERID[]    = "UserId";
//static const char eUSERNAME[]  = "UserName";

// Get free memory in bytes, or -1 if cannot determine amount (e.g. on UNIX)
//rlim_t  _IB_GetFreeMemory(void)
//{
//  return (rlim_t)GetFreeSpace(0);
//}
//
//rlim_t _IB_GetTotalMemory()
//{
// return (rlim_t)(-1);
//}


#else		/* UNIX */


/*
**	Function name : myEmailURL
**
**	Description : returns the current users mail address as URL
**	Input : void
**	Output : char * to allocated URL
*/
// UNIX
GDT_BOOLEAN _IB_GetmyMail (PSTRING Name, PSTRING Address)
{
  Address->Clear();
  Name->Clear();
  // Return a URL for the maintainer

  char user_id  [256];
  char user_name[512];

  if (_IB_GetUserId(user_id, sizeof(user_id)-1)  &&
	_IB_GetUserName(user_name, sizeof(user_name)-1))
    {
      char  *env;
      *Name  = user_name;
      if ((env = getenv("ORGANIZATION")) != NULL)
	{
	  Name->Cat (" (");
	  Name->Cat (env);
	  Name->Cat (")");
	}
      if ((env = getenv("EMAIL_ADDRESS")) != NULL && *env)
	*Address = env;
      else if ((env = getenv("MAILDOMAIN")) != NULL && *env)
	{
	  Address->form("%s@%s", user_id, env);
	}
      else
	{
	  char buf[MAXHOSTNAMELEN+1];
	  // Get official full name of host
	  if (getOfficialHostName(buf, sizeof (buf) / sizeof (char) - 1) != -1)
	    {
	      Address->form("%s@%s", user_id, buf);
	    }
	}
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


#ifndef PAGESIZE
# ifdef _SC_PAGESIZE
#  define PAGESIZE sysconf(_SC_PAGESIZE)
# else
#  define PAGESIZE 4096L
# endif
#endif

rlim_t _IB_GetFreeMemory()
{
#ifdef _SC_AVPHYS_PAGES

#if defined(SOLARIS) || defined(BSD)
  long physical = sysconf(_SC_PHYS_PAGES);
  long free     = sysconf(_SC_AVPHYS_PAGES);
  if (physical > 2*free)
    return (free + (physical-free)/4) * PAGESIZE;
  return free * PAGESIZE;
#else
  return sysconf(_SC_AVPHYS_PAGES) * PAGESIZE;
#endif
#elif defined(LINUX)
  return  get_phys_pages() * PAGESIZE;
#else
  return (rlim_t)(-1);
#endif
}

rlim_t _IB_GetTotalMemory()
{
#ifdef _SC_AVPHYS_PAGES
  return sysconf(_SC_PHYS_PAGES) * PAGESIZE;
#elif defined(LINUX)
  return get_avphys_pages() * PAGESIZE;
#else
  return (off_t)-1;
#endif
}

#endif

#undef open
#undef close

extern "C" {
  extern FILE* (*_IB_Extern_fopen) (const char *Path, const char* mode);
  extern FILE* (*_IB_Extern_freopen) (const char *Path, const char* mode, FILE *stream);
  extern int   (*_IB_Extern_fclose) (FILE *stream);
  extern int   (*_IB_Extern_open) (const char *Path, int flags, mode_t mode);
  extern int   (*_IB_Extern_close) (int fd);
};


FILE *_IB_fopen(const char *Path, const char *mode)
{
#ifdef _WIN32
  // For windows we want to replace all / with \\ especially
  // do to UNC and other path junk
  STRING newPath(Path);

  newPath.Replace("/", "\\");
  return _IB_Extern_fopen(newPath.c_str(), mode);
#else
  return _IB_Extern_fopen(Path, mode);
#endif
}

FILE *_IB_freopen(const char *Path, const char *mode, FILE *stream)
{
#ifdef _WIN32
  // For windows we want to replace all / with \\ especially
  // do to UNC and other path junk
  STRING newPath(Path);

  newPath.Replace("/", "\\");
  return _IB_Extern_freopen(newPath.c_str(), mode, stream);
#else
  return _IB_Extern_freopen(Path, mode, stream);
#endif
}


int _IB_fclose(FILE *stream)
{
  return _IB_Extern_fclose(stream);
}

static int open_file_handles = 0;


int _IB_open(const char *
#ifdef _WIN32
	Path,
#else
	path,
#endif
	int oflag, ...)
{
  int       fd;

  va_list   ap;
  va_start(ap, oflag);
  mode_t mode = (mode_t)va_arg(ap, int);
  va_end(ap);

#ifdef _WIN32
  // For windows we want to replace all / with \\ especially
  // do to UNC and other path junk
  STRING newPath (Path);

  newPath.Replace("/", "\\");
  const char *path = newPath.c_str();
#endif

  if ((fd = _IB_Extern_open(path, oflag, mode)) == -1)
    {
      if (errno == EMFILE)
        message_log (LOG_PANIC, "Can't open(%s,%d,0x%lx) : Aleady have >%d open files!",
                path, oflag, (long)mode, open_file_handles) ;
    }
  else if ( ++open_file_handles > _IB_kernel_max_file_descriptors() - 20)
    message_log (LOG_NOTICE,  "Too many descriptors %d. Increase hard/soft file handle limit. Contact your Sysadmin!",
	open_file_handles);
  return fd;
}


int _IB_close(int fd)
{
  int res = -1;
  if (fd >= 0)
    res = _IB_Extern_close(fd);
  if (res == 0)
    open_file_handles--;
  return res;
}

int _IB_kernel_max_file_descriptors()
{
#if defined(_WIN32)
 /* The total number of open handles in the system is limited only
    by the amount of memory available. However, a single process can
    have no more than 65,536 handles.
    These handles are, however, ALL handles and not just C file handles .  */
  return 2048; // _open_osfhandle() can fail at 2048.
#else
  static int m = 0;

  if (m == 0)
    {
#ifdef OPEN_MAX
       m = OPEN_MAX;
#elif defined(_SC_OPEN_MAX) && !defined(__UNIXOS2__)
      m = sysconf(_SC_OPEN_MAX);
#else
      m = 1024;
#endif
    }
  return m;
#endif
}

int _IB_kernel_max_streams()
{
#if defined(_WIN32)
  /* WIN32 can have a stream for each handle */
  return  _IB_kernel_max_file_descriptors() - 4;
#else
  static int m = 0;

  if (m == 0)
    {
#ifdef _SC_OPEN_MAX
      m = sysconf( _SC_STREAM_MAX); 
# ifdef LINUX
      // The Linux people lie! :-)
      if (m < 256)
	m = 1020; // This is what we expirically found
# endif	/* LINUX */
#else

# ifdef STREAM_MAX
      m = STREAM_MAX;
# else
      m = 256;
# endif
#endif
    }
  return m;
#endif
}

#if WANT_FILE_ID_CLASS   /* Don't need this now */

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
  return (inode == OtherId.inode) && (device == OtherId.device);
}

#endif

STRING ProcessOwner(long uid)
{
#ifdef _WIN32
  /* Note: Could be implemented - maybe later.  */
  return NulString;
#else
  STRING       String;
  const uid_t  Uid = (uid != 0 ? getuid() : (uid_t)uid);

  struct passwd *pass = getpwuid (Uid);
  if (pass)
    {
      GDT_BOOLEAN have_gecos = (pass->pw_gecos && *(pass->pw_gecos));

      String << (have_gecos ? pass->pw_gecos : pass->pw_name);
      if (have_gecos)
	String << " <" << pass->pw_name << ">";
      struct group * groupwd =  getgrgid(pass->pw_gid);
      if (groupwd) String << " (" << groupwd->gr_name << ")";
    } 
  return String;
#endif
}



STRING ResourceOwner(const STRING& Path)
{
#if defined(_MSDOS) || defined(_WIN32)
  char tmp[256];
  tmp[0] = '\0';
  if (!_IB_GetUserName(tmp, sizeof(tmp)-1))
    _IB_GetUserId(tmp, sizeof(tmp)-1); 
  return tmp;
#else
  STRING String;
  // Print the owner of the resource...
  struct stat stbuf;
  if (::lstat(Path, &stbuf) != -1)
    {
      struct passwd *pass = getpwuid (stbuf.st_uid);
      if (pass)
        {
	  GDT_BOOLEAN have_gecos = (pass->pw_gecos && *(pass->pw_gecos));

	  String << (have_gecos ? pass->pw_gecos : pass->pw_name);
	  if (have_gecos)
	    {
	      const char     *maildomain = getenv("MAILDOMAIN");
	      String << " <" << pass->pw_name;
	      if (maildomain && *maildomain)
		String << "@" << maildomain;
	      String <<  ">";
	    }
	  struct group * groupwd =  getgrgid(pass->pw_gid);
	  if (groupwd) String << " (" << groupwd->gr_name << ")";
        }
    }
  return String;
#endif
}


STRING ResourcePublisher(const STRING& Path)
{
#if defined(_MSDOS) || defined(_WIN32)
  return ResourceOwner(Path);
#else
  STRING         String;
  struct stat    stbuf;
  struct passwd *pass = NULL;
  struct group  *groupwd = NULL;

  if (Path.IsEmpty())
    {
      pass    = getpwuid (getuid ());
      groupwd =  getgrgid(getgid());
    }
  else if (::lstat(Path, &stbuf) != -1)
    {
      pass = getpwuid (stbuf.st_uid);
      groupwd =  getgrgid(pass->pw_gid);
    }
  if (pass)    String << pass->pw_gecos;
  if (groupwd) String << " (" << groupwd->gr_name << ")";
  return String;
#endif
}


GDT_BOOLEAN IsRunningProcess(long pid)
{
#if defined(_MSDOS) || defined(_WIN32)
  /* Note: Could do better - maybe later.  */
  //Only need to check processes that are no me!
  if(pid == GetCurrentProcessId()) return GDT_TRUE;

  HANDLE hProc = NULL;
  if(!(hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))) return GDT_FALSE;
  CloseHandle(hProc);
  return GDT_TRUE;
#else
  // Only need to check processes that are no me!
  if (pid <= 0 || (pid != (long)getpid() && kill (pid, 0) == -1 &&  errno == ESRCH))  
    return GDT_FALSE;
  return GDT_TRUE; // Looks OK
#endif
}
  

GDT_BOOLEAN IsMyProcess(long uid)
{
#if defined(_MSDOS) || defined(_WIN32)
  /*Note: * Can't check because we currently have no implementation for getuid() */
  return GDT_TRUE;
#else
  return ((uid_t)uid == getuid ()) ;
#endif
}


#ifdef _WIN32
# define C(_x) _tolower(_x)
#else
# define C(_x) (_x)
#endif


STRING RelativizePathname(const STRING& Path, const STRING& Dir)
{
  return RelativizePathname(Path.c_str(), RemoveTrailingSlash(Dir).c_str());
//return RelativizePathname(Path.c_str(), AddTrailingSlash(Dir).c_str());
}


// KNOWN BUGS:
// Dirs with more than 1 trailing / breaks things!
//
STRING RelativizePathname(const char *Path, const char *Dir)
{
  const char *tp1 = Path;
  const char *tp2 = Dir;
  const char *slash = NULL;

  if (Path == NULL || *Path == '\0') return NulString;
  if (Dir == NULL || *Dir == '\0') return Path;

  // We can only relativize absolute paths so we expand
  // all paths and dirs first
  if (!IsAbsoluteFilePath(Path) || !IsAbsoluteFilePath(Dir))
    return RelativizePathname(ExpandFileSpec(Path), ExpandFileSpec(Dir));

#ifdef _WIN32
  if (tp1[0] != tp2[0]) // Since absolute paths then different drives
    return Path; // Can't relativize across drives

  // Do we have UNC paths?
  if (tp1[0] == '\\' && tp1[1] == '\\')
    {
      if (tp2[0] != '\\' || tp2[1] != '\\')
	return Path; // Nothing we can do since different drives
      tp1++, tp2++; // Skip \\ bits
      while ((*++tp1 == *++tp2) != '\0')
	{
	  if (*tp1 == '\\')
	    {
	      slash = tp2;
	      break;
	    }
	}
      if (slash == NULL) // Meaning that the network name does not match
	return Path; // Can't relativize across network drives
    }
#endif
  // Scan for the start of differences
  if (slash == NULL)
    do {
      if (IsPathSep(*tp1) && IsPathSep(*tp2))
	{
	  while (IsPathSep (*(tp1+1))) tp1++;
	  while (IsPathSep (*(tp2+1))) tp2++;
	}
      else if (C(*tp1) != C(*tp2))
	{
	  if (*tp1 && *tp2 && IsPathSep(*tp1) != IsPathSep(*tp2))
	    {
	      break;
	    }
	  if (!IsPathSep(*tp1) && *tp1 && !IsPathSep(*tp2) && *tp2) 
	    {
	      break;
	    }
	  if (IsPathSep(*tp1)) while (IsPathSep (*(tp1+1))) tp1++;
	  if (IsPathSep(*tp2)) while (IsPathSep (*(tp2+1))) tp2++;
	}
      if (IsPathSep(*tp1)) slash = tp2; // Remember last slash
      if (*tp1) tp1++;
      if (*tp2) tp2++;
   } while (*tp1 || *tp2); 
  // Did we perhaps not get to a last slash? In dir its still a dir

  // Nothing in common?
  if (slash == NULL)
#if 0 /* Strategy 1:  Return the Fullpath */
    return Path;
#else /* Strategy 2: Still make relative */
    slash = Dir;
#endif

#ifdef _WIN32
  char  slashChar = *slash; // Need to keep things consistant
#else
  char  slashChar = SEP;
#endif

  // Have something in common
  size_t      offset = slash-Dir+1;

  if ((offset > 1) && !IsPathSep(Path[offset-1]))
    {
      while (IsPathSep(Path[offset-2]))
	offset--;
    }

  const char *rest = Path + offset; // Point to last /


  STRING rPath;
  // Watch out for missing tailing /
  if (*slash) while (*++slash)
    {
      if (IsPathSep(*slash) || *(slash+1) == '\0')
	{
	  rPath << ".." << slashChar;
	}
    }

  rPath.Cat(rest);
  RemoveTrailingSlash(&rPath);
//cerr << "Returning " << rPath << endl;
  return rPath;
}

int PathCompare(const char *Path1, const char *Path2)
{
  // Handle NULL and same pointer
  if (Path1 == Path2) return 0;
  if (Path1 == NULL) return -1;
  if (Path2 == NULL) return  1;

  GDT_BOOLEAN s = GDT_FALSE;
  // Find first difference
  while (*Path1 && (((s = IsPathSep(*Path1)) && IsPathSep(*Path2)) || (C(*Path1) == C(*Path2))))
    {
      if (s)
	{
	  // Note:  x//y//z matches x/y/z 
	  while (IsPathSep(*++Path1)) /* loop */;
	  while (IsPathSep(*++Path2)) /* loop */;
	  s = GDT_FALSE;
	}
      else
	Path1++, Path2++;
    }
  if (*Path1 == '\0' && IsPathSep(*Path2))
    return 0;
  if (*Path2 == '\0' && IsPathSep(*Path1))
    return 0;
  return *Path1 - *Path2;
}

#ifdef _WIN32

#include <psapi.h>

int getrusage(int who, struct rusage *usage)
{
  if (usage == NULL) return -1;

  // Zero values
  usage->ru_utime.tv_sec  = 0;
  usage->ru_utime.tv_usec = 0;
  usage->ru_stime.tv_sec  = 0;
  usage->ru_stime.tv_usec = 0;
  usage->ru_majflt = 0;  // page faults requiring physical I/O (*)
  usage->ru_minflt = 0;  // page faults not requiring physical I/O
  usage->ru_nswap = 0;   // swaps
  usage->ru_inblock = 0; // block input operations
  usage->ru_oublock = 0; // block output operations
  usage->ru_nvcsw = 0;   // voluntary context switches
  usage->ru_nivcsw = 0;  // involuntary context switches
  usage->ru_maxrss = 0;  // maximum resident set size (*)
  usage->ru_ixrss = 0;   // currently 0
  usage->ru_idrss = 0;   // integral resident set size
  usage->ru_isrss = 0;   // currently 

//  if ((WIN32_OS_version == _WIN_OS_WINNT) || (WIN32_OS_version == _WIN_OS_WIN2K)
//       || (WIN32_OS_version == _WIN_OS_WINXP) || (WIN32_OS_version == _WIN_OS_WINNET))
    {
        HANDLE hProcess;
        PROCESS_MEMORY_COUNTERS pmc;
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, GetCurrentProcessId());
        {
	    // Requires Kernel32.dll.
            FILETIME ftCreate, ftExit, ftKernel, ftUser;
            if (GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftKernel, &ftUser))
	      {
                PFILETIME p_ftUser = &ftUser;
                PFILETIME p_ftKernel = &ftKernel;
                int64_t tUser64 = (*(int64_t *) p_ftUser / 10);
                int64_t tKernel64 = (*(int64_t *) p_ftKernel / 10);
                usage->ru_utime.tv_sec = (long) (tUser64 / 1000000);
                usage->ru_stime.tv_sec = (long) (tKernel64 / 1000000);
                usage->ru_utime.tv_usec = (long) (tUser64 % 1000000);
                usage->ru_stime.tv_usec = (long) (tKernel64 % 1000000);
              }
	    else
	      {
                CloseHandle(hProcess);
                return -1;
              }
        }
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
	  {
            usage->ru_maxrss = (DWORD) (pmc.WorkingSetSize / winGetPageSize());
            usage->ru_majflt = pmc.PageFaultCount;
	  }
	else
	  {
            CloseHandle(hProcess);
            return -1;
          }
      CloseHandle(hProcess);
    }
  return 0;
}


static STRING GetRegistryEntry(HKEY dwHKEY, const char *Key, const char *Value)
{
    HKEY _hkey = NULL;
    if((RegOpenKeyEx(dwHKEY, Key, 0, KEY_QUERY_VALUE, &_hkey) == ERROR_SUCCESS) && _hkey)
    {
        DWORD _dw = 1024;
        unsigned char buf[1024];
        LONG _nRet = RegQueryValueEx(_hkey, Value, NULL, NULL, buf, &_dw);
        RegCloseKey(_hkey);
        if(_nRet == ERROR_SUCCESS)
            return buf;
    }
    return NulString;
}

static STRING GetProgramFilesDir()
{
    static STRING sPrFilesDir = NulString;
    if(NulString!=sPrFilesDir)
        return sPrFilesDir;
    sPrFilesDir = GetRegistryEntry(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", "ProgramFilesDir");
    if (sPrFilesDir == NulString)
        sPrFilesDir = "c:\\Program Files";
    return sPrFilesDir;
} 

/* This is the action !!! */

BOOL IsRealFile(STRING sAbsFN)
{
    if(sAbsFN.GetLength() < 4)
    {
        return TRUE;
    }
    static STRING sVirtDir(getenv("LocalAppData"));
    if(sVirtDir.GetLength() < 2)
    {
        return TRUE;
    }
    
    static STRING sprogramfilesdir = NulString;
    if(NulString==sprogramfilesdir)
    {
        sprogramfilesdir = GetProgramFilesDir();
        sprogramfilesdir.MakeLower();
        sprogramfilesdir.Replace("/","\\",TRUE);
    }
    
    STRING sabsfn(sAbsFN);
    sabsfn.MakeLower();
    sabsfn.Replace("/","\\",TRUE);
    
    int iPF = sabsfn.Find(sprogramfilesdir);
    if((iPF != 0) && (iPF != 1))
    {
        return TRUE;
    }
    
    STRING sVirtAbsFN (sVirtDir + "\\VirtualStore\\" + sAbsFN.Mid((sAbsFN.GetChar(1) == ':') ? 3 : 2));
    if(_access(sVirtAbsFN, 00) == 0)
    {
        return FALSE;
    }
    return TRUE;
}

#endif
