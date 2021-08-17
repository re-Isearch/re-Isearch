/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

#ifndef RESOURCE_HXX
#define RESOURCE_HXX

#include "defs.hxx"
#include "string.hxx"
#include "inode.hxx"

#ifndef STANDALONE


class RECORD;

class RESOURCE {
public:
  RESOURCE ();
  RESOURCE (const STRING& Path);
  RESOURCE (const RESOURCE& Other);
  RESOURCE (const RECORD& Record);

  RESOURCE& operator=(const RESOURCE& OtherResource);

  void    Clear();

  STRING  GetMimeType() const        { return MimeType; }
  void    SetMimeType(const STRING& newType) { MimeType = newType; }

  void    SetPath(const STRING& NewPathName);
  STRING  GetPath() const;
  STRING  GetPath(PSTRING Buffer) const { return *Buffer = GetPath(); }

  void    SetFileName(const STRING& NewName);
  STRING  GetFileName() const;
  STRING  GetFileName(PSTRING Buffer) const   { return *Buffer = GetFileName(); }

  void    SetFullFileName (const STRING& FullName);
  STRING  GetFullFileName() const                  { return FullPath;                    }
  STRING  GetFullFileName(STRING *Buffer) const    { return *Buffer = GetFullFileName(); }

  void    SetStart(const GPTYPE NewStart){ Start = NewStart; }
  GPTYPE  GetStart() const               { return Start;     }

  void    SetEnd(const GPTYPE NewEnd) { End = NewEnd; }
  GPTYPE  GetEnd() const             { return End;   }

  off_t   GetLength () const       { return End-Start;}

  GDT_BOOLEAN    Ok() const; // Check Inode

  void           Write(PFILE fp) const;
  GDT_BOOLEAN    Read(PFILE fp);

  ~RESOURCE() {};

private:
  STRING         FullPath;
  UINT8          Start;
  UINT8          End;
  DOCTYPE_ID     Doctype;
  LOCALE         Locale;
  STRING         MimeType;
  UINT8          Inode;
};


// Common Functions
inline void Write (const RESOURCE& Resource, PFILE Fp)
{
  Resource.Write (Fp);
}

inline GDT_BOOLEAN Read (RESOURCE *Ptr, PFILE Fp)
{
  return Ptr->Read (Fp);
}


#endif
#endif
