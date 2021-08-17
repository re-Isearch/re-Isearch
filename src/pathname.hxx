/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

/*@@@
File:           pathname.hxx
Version:        1.00
Description:    Paths class
@@@*/

#ifndef PATHNAME_HXX
#define PATHNAME_HXX

#include "common.hxx"
#include "inode.hxx"

#ifndef WANT_FILE_ID_CLASS  
# define  WANT_FILE_ID_CLASS  0
#endif

// Class for Pathsnames
/* Advantage: Easier (more efficient) to changes file names in the same path)
*/

// Class for Pathsnames
class PATHNAME {
friend class MDTREC;
public:
  PATHNAME();
  PATHNAME(const STRING& FullFileName);
  PATHNAME(const STRING& Path, const STRING& FileName) {
    SetPath(Path); SetFileName(FileName);
  }

  GDT_BOOLEAN IsAbsoluteFilePath() const {
    return ::IsAbsoluteFilePath(Path);
  }

  GDT_BOOLEAN Exists() const {
    return FileExists(AddTrailingSlash(Path) + File);
  }

  STRING SetPath (const STRING& newPath);
  STRING SetFileName(const STRING& newFileName);

  void Prepend(const STRING& path) {
    Path.Prepend(AddTrailingSlash(path)); 
  }

  // Add to path
  void CatPath(const STRING& subpath) {
    if (subpath.GetLength() && subpath != ".") {
      if (ContainsPathSep(subpath)) {
	STRING full (GetFullFileName());
	if (!IsPathSep(subpath.GetChr(1)))
	  AddTrailingSlash(&full);
	SetFullFileName(full + subpath);
      } else {
	AddTrailingSlash(&Path);
	Path.Cat(subpath);
      }
    }
  }

  STRING GetFileName() const;
  STRING GetPath() const;

  GDT_BOOLEAN  SetFullFileName (const STRING& newFullPath);
  STRING GetFullFileName() const;

  PATHNAME RelativizePathname(const STRING& Dir);
  operator STRING () const { return GetFullFileName(); }

  PATHNAME& operator=(const PATHNAME& Other) {
    File = Other.File;
    Path = Other.Path;
    return *this;
  }
  INT Compare(const PATHNAME& Other) const;
  void Clear() { File.Clear(); Path.Clear(); }
  // We read and write as fullpath to be compatible with
  // the other version of this class
  void Write(FILE *fp) const;
  GDT_BOOLEAN Read(FILE *fp);

// IO Streams
  friend ostream& operator <<(ostream& os, const PATHNAME& str);
  friend istream& operator >>(istream& os, PATHNAME& str);

  ~PATHNAME();
private:
  STRING File;
  STRING Path;
};

// Common Functions
inline void Write (const PATHNAME& Pathname, PFILE Fp)
{
  Pathname.Write (Fp);
}

inline GDT_BOOLEAN Read (PATHNAME *PathnamePtr, PFILE Fp)
{
  return PathnamePtr->Read (Fp);
}


//
inline GDT_BOOLEAN operator==(const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) == 0; }
inline GDT_BOOLEAN operator!=(const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) != 0; }
inline GDT_BOOLEAN operator<=(const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) <= 0; }
inline GDT_BOOLEAN operator>=(const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) >= 0; }
inline GDT_BOOLEAN operator< (const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) < 0; }
inline GDT_BOOLEAN operator> (const PATHNAME& s1, const PATHNAME& s2) { return s1.Compare(s2) > 0; }

/* ALTERNATIVE SELECTION */
class PATHNAME_B {
public:
  PATHNAME_B() {}
  PATHNAME_B(const STRING& Path, const STRING& File) {
    FullPath = ExpandFileSpec(AddTrailingSlash(Path)) + File;
  }
  PATHNAME_B(const STRING& Fullpath) {
    SetFullFileName(Fullpath);
  }

  GDT_BOOLEAN Exists() const { return FileExists(FullPath); }

  GDT_BOOLEAN IsAbsoluteFilePath() const {
    return ::IsAbsoluteFilePath(FullPath);
  }

  void SetPath (const STRING& NewPath) {
    FullPath = ExpandFileSpec(AddTrailingSlash(NewPath)) + GetFileName();
  }
  STRING SetFileName(const STRING& newName) {
    STRING oldName ( GetFileName() );
    if (newName != oldName)
      {
        FullPath =  GetPath() + newName;
	if (::DirectoryExists(FullPath)) {
	  // Make a file path
	  AddTrailingSlash(&FullPath);
	  FullPath.Cat(".");
	  return ".";
	}
      }
    return newName;
  }

  void Prepend(const STRING& path) {
    FullPath.Prepend(AddTrailingSlash(path));
  }

  // Add to path
  void CatPath(const STRING& subpath) {
    if (subpath.GetLength() && subpath != ".")
      SetPath(AddTrailingSlash((GetPath())) + AddTrailingSlash(subpath) + GetFileName());
  }

  STRING  GetFileName() const {
    STRING name ( RemovePath(FullPath) );
    if (name.IsEmpty()) return ".";
    return name;
  }
  STRING GetPath() const {
    return RemoveFileName(FullPath);
  }
  GDT_BOOLEAN SetFullFileName (const STRING& newName) {
    if (newName != FullPath)
      FullPath = ExpandFileSpec(newName);
    return !FullPath.IsEmpty();
  }
  STRING GetFullFileName() const {
    return FullPath;
  } 
  STRING RelativizePathname(const STRING& Dir) const {
    return ::RelativizePathname(FullPath, Dir);
  }
  operator STRING () const { return FullPath; }
  PATHNAME_B& operator=(const PATHNAME_B& Other) {
    FullPath = Other.FullPath;
    return *this;
  }
  INT Compare(const PATHNAME_B& Other) const {
#if _WIN32
    return FullPath.CaseCompare(Other.FullPath);
#else
    return FullPath.Compare(Other.FullPath);
#endif
  }
  void Clear() { FullPath.Clear(); }
  void Write(FILE *fp) const {
    FullPath.Write(fp);
  }
  GDT_BOOLEAN Read(FILE *fp) {
    return FullPath.Read(fp);
  }
private:
  STRING FullPath;
};


#if WANT_FILE_ID_CLASS
/// ID a file
class FILE_ID {
public:
  FILE_ID ();
  FILE_ID (const INODE& Inode, const STRING& Filename = NulString)
    { inode = Inode; filename = Filename; }
  FILE_ID(STRING& Filename);

  FILE_ID&    operator=(const STRING& nFilename);
  GDT_BOOLEAN operator== (const FILE_ID& OtherId) const;

  long        Inode() const    { return inode.inode();    }
  long        Device() const   { return inode.device();   }
  STRING      Filename() const { return filename; }
  STRING      Id() const;
  const char *c_str() const { return Id().c_str(); }

  ~FILE_ID();

private:
  STRING filename;
  INODE  inode;
};
#endif

#endif
