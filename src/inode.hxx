/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _INODE_HXX
# define _INODE_HXX

#include "gdt.h"
#include "string.hxx"
#include "date.hxx"
#include <stdio.h>
#include <sys/stat.h>

#ifndef _IB_INO_T_DECLARED
typedef  UINT8 _ib_ino_t;
#define _IB_INO_T_DECLARED
#endif

#ifndef _IB_DEV_T_DECLARED
typedef  INT8 _ib_dev_t;
#define _IB_DEV_T_DECLARED
#endif

#ifndef _IB_NLINK_T_DECLARED
typedef UINT4 _ib_nlink_t;
#define  _IB_NLINK_T_DECLARED
#endif

class INODE {
public:
  enum filetypes { unknown=-1, plain=0, executable, directory};

  INODE();
  INODE(const STRING& Path); 
  INODE(FILE *fp);
  INODE(int fd);

  bool Ok() const { return  st_ino != 0 && st_nlink != 0; }

  void Clear();

  INODE& operator =(const INODE& Other) {
    st_ino   = Other.st_ino;
    st_dev   = Other.st_dev;
    st_nlink = Other.st_nlink;
    st_size  = Other.st_size;
    cdate    = Other.cdate;
    mdate    = Other.mdate;
    adate    = Other.adate;
    ftype    = Other.ftype;
    return *this;
  }

  bool Equals(const INODE& Other) const {
    return st_ino == Other.st_ino && st_dev == Other.st_dev;
  }

  bool Set(const STRING& Path);
  bool Set(FILE *fp);
  bool Set(int fd);

  STRING      Key() const;
  STRING      Key(off_t start, off_t end) const;


  bool isLinked() const { return st_nlink > 0; }

  bool isDangling() const { return st_nlink == 0; }
  _ib_ino_t   inode() const      { return st_ino; }
  _ib_dev_t   device() const     { return st_dev; }
  off_t       size() const       { return st_size; }
  ino_t       _get_ino_t() const;

// IO Streams  
  friend ostream& operator <<(ostream&, const INODE&);

  void           Write(PFILE fp) const;
  bool    Read(PFILE fp);

  _ib_ino_t   st_ino;   // Inode or file index 
  _ib_dev_t   st_dev;   // Device or volume of file
  _ib_nlink_t st_nlink; // Number of links (Unix file system or NTFS)
  off_t       st_size;  // Size

  SRCH_DATE   mdate;    // Date modified
  SRCH_DATE   cdate;    // Date created
  SRCH_DATE   adate;    // Date accessed
private:
  enum filetypes  ftype; // File type (file, directory, etc )
// WIN32: FILE_ATTRIBUTE_DIRECTORY
// FILE_ATTRIBUTE_NORMAL,
/* Others in Win32
#FileAttribute_Compressed=#FILE_ATTRIBUTE_COMPRESSED 
#FileAttribute_Directory =#FILE_ATTRIBUTE_DIRECTORY 
#FileAttribute_Hidden    =#FILE_ATTRIBUTE_HIDDEN 
#FileAttribute_ReadOnly  =#FILE_ATTRIBUTE_READONLY 
#FileAttribute_System    =#FILE_ATTRIBUTE_SYSTEM 
#FileAttribute_Temporary =#FILE_ATTRIBUTE_TEMPORARY 
*/
} ;


inline bool operator!=(const INODE& s1, const INODE& s2) { return !s1.Equals(s2); }
inline bool operator==(const INODE& s1, const INODE& s2) { return s1.Equals(s2); }


// Common Functions
inline void Write (const INODE& Inode, FILE *Fp)
{
  Inode.Write(Fp);
}

inline bool Read (INODE *Ptr, FILE *Fp)
{
  return Ptr->Read (Fp);
}


#endif
