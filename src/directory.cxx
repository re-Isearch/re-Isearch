/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)directory.cxx  1.6 02/05/01 00:33:04 BSN"

#include "common.hxx"
#include <sys/stat.h>
#include <sys/types.h>
#include "dirent.hxx"
#include "strstack.hxx"

#ifdef _WIN32
# define FNMATCH(_x_, _y_) FileGlob((const UCHR *)(_x_), (const UCHR *)(_y_))
#else
# include <fnmatch.h>
# define FNMATCH(_x_, _y_) (::fnmatch(_x_, _y_, 0) == 0)
# define FixMicrosoftPathNames(_x) _x
#endif

static struct stat stat_buf;


inline GDT_BOOLEAN is_regular()
{
  return (stat_buf.st_mode & S_IFMT) == S_IFREG;
}

inline GDT_BOOLEAN is_regular(char const * path)
{
  return ::stat(path, &stat_buf) != -1 && is_regular();
}

inline GDT_BOOLEAN is_regular(const STRING& path)
{
  return is_regular(path.c_str()); 
}

inline GDT_BOOLEAN is_directory()
{
  return (stat_buf.st_mode & S_IFMT) == S_IFDIR;
}

inline GDT_BOOLEAN is_directory(char const * path)
{
  return ::stat(path, &stat_buf) != -1 && is_directory();
}

inline GDT_BOOLEAN is_directory(STRING const & path)
{
  return is_directory(path.c_str());
}

inline GDT_BOOLEAN is_symbolic_link()
{
#ifdef _WIN32
  return GDT_FALSE;
#else
  return (stat_buf.st_mode & S_IFLNK) == S_IFLNK;
#endif
}

inline GDT_BOOLEAN is_symbolic_link(char const * path)
{
  return ::lstat(path, &stat_buf) != -1 && is_symbolic_link();
}

inline GDT_BOOLEAN is_symbolic_link(STRING const & path)
{
  return is_symbolic_link(path.c_str());
}


// TRUE  := At least one element of list matches name
// FALSE := No elements in list match name 
//
inline GDT_BOOLEAN FnmatchAny (const STRLIST *List, const char *name)
{
  for (const STRLIST *lptr = List->Next(); lptr != List; lptr = lptr->Next())
    {
      const char * value  = lptr->Value().c_str();
      if (FNMATCH(value, name) || (value[0] == '.' && value[1] == '\0'))
	return GDT_TRUE;
    }
  return GDT_FALSE;
}



void do_directory(const STRING& Dir, int (*do_file)(const STRING&),
  const STRLIST *filePatternList,
	const STRLIST *excludeList,
	const STRLIST *inclDirList,
	const STRLIST *excludeDirList, GDT_BOOLEAN recurse, GDT_BOOLEAN follow)
{
  static STRSTACK dir_queue;
  static int      recursion;
  STRING dir      (Dir);

  if (dir.IsEmpty()) dir = ".";

  if (is_regular(dir))
    {
      do_file(dir);
      return;
    }

  if (is_symbolic_link(dir) && !follow)
    return;

  // Watch out for files with wildcard names!
  if (dir.IsWild() && !Exists(dir))
    {
      STRLIST List;
      for (const STRLIST *lptr = filePatternList->Next();
		lptr != filePatternList; lptr = lptr->Next())
	List.AddEntry(lptr->Value());
      List.AddEntry(RemovePath(dir));

      // contains wildcards
      do_directory(RemoveFileName (dir), do_file, &List,
	excludeList, inclDirList, excludeDirList, recurse, follow);
      return;
    }
  STRING dirName ( AddTrailingSlash(dir) );
  DIR            *const dir_p = ::opendir(dirName.c_str());
  if (!dir_p)			// can't open: skip
    {
      if (is_directory(dirName))
        message_log (LOG_ERRNO, "(2) Can't open directory '%s'", dirName.c_str());
       else
        message_log (LOG_ERRNO, "Having trouble accessing '%s'. Not a directory?", dir.c_str());
      return;
    }

  struct dirent const *dir_ent;

  while ((dir_ent = ::readdir(dir_p)) != NULL) {
    if (*dir_ent->d_name == '.')// skip dot files
       continue;
    STRING          path = FixMicrosoftPathNames(dirName + dir_ent->d_name);

    if (is_directory(path)) {
      if (recurse &&
	(excludeDirList == NULL || excludeDirList->IsEmpty() || !FnmatchAny (excludeDirList, dir_ent->d_name)) &&
	(inclDirList    == NULL || inclDirList->IsEmpty()    || FnmatchAny(inclDirList, dir_ent->d_name)) ) {
	  dir_queue.Push(path);
	}
    } else if (
	// An empty pattern matches anything
	(filePatternList == NULL || filePatternList->IsEmpty() || FnmatchAny(filePatternList, dir_ent->d_name)) &&
	(excludeList   == NULL  || excludeList->IsEmpty()      || !FnmatchAny(excludeList, dir_ent->d_name))) {
      do_file(path);
    }
  }

  ::closedir(dir_p);
  if (recursion)
    return;

  // //////// Do all subdirectories //////////////////////////////////////
  while (!dir_queue.IsEmpty()) {
    dir_queue.Pop(&dirName);
    ++recursion;
    do_directory(dirName, do_file, filePatternList, excludeList, inclDirList, excludeDirList, recurse, follow);
    --recursion;
  }
}


void do_directory(const STRING& dir, int (*do_file)(const STRING&),
  const STRING& filePattern,
	const STRING& exclude, const STRING& inclDir,
	const STRING& excludeDir, GDT_BOOLEAN recurse, GDT_BOOLEAN follow)
{
  static STRSTACK dir_queue;
  static int      recursion;

  if (is_regular(dir))
    {
      do_file(dir);
      return;
    }

  if (is_symbolic_link(dir) && !follow)
    return;

  STRING dirName ( AddTrailingSlash(dir) );

  DIR            *const dir_p = ::opendir(dirName.c_str());
  if (!dir_p)			// can't open: skip
    {
      if (is_directory(dirName))
	message_log (LOG_ERRNO, "(1) Can't open directory '%s'", dirName.c_str());
       else
	message_log (LOG_ERRNO, "Having trouble accessing '%s'", dir.c_str()); 
      return;
    }

  struct dirent const *dir_ent;
  STRING          pattern(filePattern);

  if (pattern == ".")
    pattern.Clear();

  while ((dir_ent = ::readdir(dir_p)) != NULL) {
    if (*dir_ent->d_name == '.')// skip dot files
       continue;
    STRING          path = FixMicrosoftPathNames(dirName + dir_ent->d_name);

    if (is_directory(path)) {
      if (recurse &&
	(excludeDir.IsEmpty() || !FNMATCH(excludeDir.c_str(), dir_ent->d_name)) &&
	(inclDir.IsEmpty() || FNMATCH(inclDir.c_str(), dir_ent->d_name)) ) {
	  dir_queue.Push(path);
	}
    } else if ((pattern.IsEmpty() || FNMATCH(pattern.c_str(), dir_ent->d_name)) &&
	(exclude.IsEmpty() || !FNMATCH(exclude.c_str(), dir_ent->d_name))) {
      do_file(path);
    }
  }

  ::closedir(dir_p);
  if (recursion)
    return;

  // //////// Do all subdirectories //////////////////////////////////////
  while (!dir_queue.IsEmpty()) {
    dir_queue.Pop(&dirName);
    ++recursion;
    do_directory(dirName, do_file, pattern, exclude, inclDir, excludeDir, recurse, follow);
    --recursion;
  }
}


void do_directory( const STRING& dir, int (*do_file)(const STRING&),
  const char *pattern,
  const char *exclude,
  const char *inclDirpattern,
  const char *exclDirpattern,
  GDT_BOOLEAN recurse, GDT_BOOLEAN follow)
{

  return do_directory (dir, do_file, 
	pattern ? STRING(pattern) : NulString,
	exclude ? STRING(exclude) : NulString,
	inclDirpattern ? STRING (inclDirpattern) : NulString,
	exclDirpattern ? STRING (exclDirpattern) : NulString, 
	recurse, follow);
}

