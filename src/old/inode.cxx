#include "platform.h"
#include "defs.hxx"
#include "gdt.h"
#include "common.hxx"
#include "inode.hxx"
#include "magic.hxx"

#ifdef _WIN32
#ifndef _HAVE_LSTAT
# define _HAVE_LSTAT 0
#endif

#include <sys/stat.h>
#include <io.h>
#include <stdint.h>
#include <windows.h>
#define MAKEDWORDLONG(a,b) ((DWORDLONG)(((DWORD)(a))|(((DWORDLONG)((DWORD)(b)))<<32)))

#define SEQNUMSIZE (16)


/*
struct _BY_HANDLE_FILE_INFORMATION {
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD dwVolumeSerialNumber;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD nNumberOfLinks;
  DWORD nFileIndexHigh;
  DWORD nFileIndexLow;
}

   dwVolumeSerialNumber

    The serial number of the volume that contains a file.

    The number of links to this file. For the FAT file system this member is always 1. For the NTFS file system, it can be more than 1.
nFileIndexHigh

    The high-order part of a unique identifier that is associated with a file. For more information, see nFileIndexLow.
nFileIndexLow

*/

static _ib_ino_t getinode (HANDLE hFile)
{
  BY_HANDLE_FILE_INFORMATION FileInformation;
  UINT8                      ino;

  ZeroMemory (&FileInformation, sizeof(FileInformation));
  GetFileInformationByHandle (hFile, &FileInformation);

  ino = cons_UINT8( FileInformation.nFileIndexHigh, FileInformation.nFileIndexLow);
  return  ino & ((~(0ULL)) >> SEQNUMSIZE);  /* remove sequence number */
}


ino_t getino (const char *path)
{
  HANDLE hFile;
  ino_t ino;
/* obtain handle to file "path"
   FILE_FLAG_BACKUP_SEMANTICS is used to open directories
*/
  if (!path || !*path) 
    return 0;
  if (access (path, F_OK)) /* path does not exist */
    return -1;
  hFile = CreateFile (path, 0, 0, NULL, OPEN_EXISTING,
	FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_READONLY, NULL);
  ino = getinode(hFile);
  CloseHandle(hFile);
  return ino;
}

ino_t getino (int fd)
{
  /* obtain handle to file descriptor "fd" */
  return getinode ((HANDLE)_get_osfhandle (fd));
}

/*
 NOTE: This fstat and stat calls MIGHT have been trapped to a lib
 where inodes are set. If so we leave them if not we create our own
 inodes.
*/

int _IB_fstat(int fd, struct stat *sb)
{
  int result = fstat(fd, sb);
  if (result != -1 && sb->st_ino == 0)
   sb->st_ino = getino(fd);
  return result;
}

int _IB_stat(const char *path, struct stat *sb)
{
  int result = stat(path, sb);
  if (result != -1 && sb->st_ino == 0)
   sb->st_ino = (ino_t)getino(path);
  return result;
}

int _IB_lstat(const char *path, struct stat *sb)
{
  return _IB_stat(path, sb);
}

#else

#ifndef _HAVE_LSTAT
# define _HAVE_LSTAT 1
#endif


int _IB_fstat(int fd, struct stat *sb)
{
  return fstat(fd, sb);
}

int _IB_stat(const char *path, struct stat *sb)
{
  return stat(path, sb);
}

int _IB_lstat(const char *path, struct stat *sb)
{
  return lstat(path, sb);
}


#endif


INODE::INODE()
{
  Clear();
}

//
// st_nlink == 0 means dangling symbollic link
//

INODE::INODE(const STRING& Path)
{
  Set(Path);
}


GDT_BOOLEAN INODE::Set(const STRING& Path)
{
#ifdef _WIN32
  if (Path.GetLength() && access (Path, F_OK) == 0) 
    {
      BY_HANDLE_FILE_INFORMATION FileInformation;
      UINT8                      ino;
      HANDLE                     hFile;
      SYSTEMTIME                 stime;
      FILETIME                   mtime, ctime, atime;
    
      hFile = CreateFile (Path, 0, 0, NULL, OPEN_EXISTING,
	FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_READONLY, NULL);
      ZeroMemory (&FileInformation, sizeof(FileInformation));
      GetFileInformationByHandle (hFile, &FileInformation);
      CloseHandle(hFile);

      ctime = FileInformation.ftCreationTime;
      atime = FileInformation.ftLastAccessTime;
      mtime = FileInformation.ftLastWriteTime;

      if (FileTimeToSystemTime(&ctime, &stime))
	cdate.Set(&stime); 
      if (FileTimeToSystemTime(&atime, &stime))
        adate.Set(&stime);
      if (FileTimeToSystemTime(&mtime, &stime))
        mdate.Set(&stime);

      ino      = cons_UINT8( FileInformation.nFileIndexHigh, FileInformation.nFileIndexLow);
      st_ino   = ino & ((~(0ULL)) >> SEQNUMSIZE);  /* remove sequence number */
      st_dev   = FileInformation.dwVolumeSerialNumber;
      st_nlink = FileInformation.nNumberOfLinks;
      st_size  = cons_UINT8( FileInformation.nFileSizeHigh, FileInformation.nFileSizeLow);

      return GDT_TRUE;
    }
#else

  struct stat sb;
  time_t      mtime, ctime, atime;
#if _HAVE_LSTAT
  if (lstat(Path.c_str(), &sb) == 0)
    {
      st_ino   = sb.st_ino;
      st_dev   = sb.st_dev;
      st_nlink = sb.st_nlink;
      st_size  = sb.st_size;
      mtime = sb.st_mtime;
      ctime = sb.st_ctime;
      atime = sb.st_atime;

      if (sb.st_mode & S_IFLNK) {
#endif
      if (_IB_stat(Path.c_str(), &sb) == 0)
	{
	  st_ino   = sb.st_ino;
	  st_dev   = sb.st_dev;
	  st_size  = sb.st_size;
	  mtime = sb.st_mtime;
          ctime = sb.st_ctime;
          atime = sb.st_atime;
	  if ((st_nlink = sb.st_nlink) == 0) // Can't be
	    st_nlink = 1;
	  /* Now set dates */
	  cdate.Set(&ctime);
	  mdate.Set(&mtime);
	  adate.Set(&atime);
	  /* -------------*/

	}
#if _HAVE_LSTAT
      else
	 st_nlink = 0; // Dangling symbollic link
      } else
	{
	  /* Now set dates */
	  cdate.Set(&ctime);
	  mdate.Set(&mtime);
	  adate.Set(&atime);
	  /* -------------*/
	}
      return GDT_TRUE;
    }
#endif
#endif
  else Clear();
  return GDT_FALSE;
}

void INODE::Clear()
{
  st_ino   = 0;
  st_dev   = 0;
  st_nlink = 0;
  st_size  = 0;
  ftype    = unknown;
}

INODE::INODE(FILE *fp)
{
  Set(fp);
}


INODE::INODE(int fd)
{
  Set(fd);
}

GDT_BOOLEAN INODE::Set(FILE *fp)
{
  if (fp) return Set(fileno(fp));
  else Clear();
  return GDT_FALSE;
}


GDT_BOOLEAN INODE::Set(int fd)
{
  struct stat sb;

  if (_IB_fstat(fd, &sb) == 0)
    {
      st_ino   = sb.st_ino;
      st_dev   = sb.st_dev;
      st_size  = sb.st_size;
      if ((st_nlink = sb.st_nlink) == 0)
	st_nlink = 1;
      return GDT_TRUE;
    }
  Clear();
  return GDT_FALSE;
}

#ifndef NAMESPACE
# define NAMESPACE
#endif

NAMESPACE ostream& operator<<(NAMESPACE ostream& os, const INODE& inode)
{
  return os << inode.Key();
}

void  INODE::Write(PFILE fp) const
{
  putObjID(objINODE, fp);
  ::Write((UINT8)st_ino, fp);
  ::Write((INT4)st_dev, fp);
  ::Write((INT4)st_nlink, fp);
  ::Write((INT8)st_size, fp);
  ::Write(cdate, fp);
  ::Write(mdate, fp);
  ::Write(adate, fp);
}

GDT_BOOLEAN INODE::Read(PFILE fp)
{
  if ( getObjID(fp) == objINODE)
    {
      UINT8 x8;
      INT4  x;
      ::Read(&x8, fp); st_ino   = x8;
      ::Read(&x, fp);  st_dev   = x;
      ::Read(&x, fp);  st_nlink = x;
      ::Read(&x8, fp); st_size  = x8;
      ::Read(&cdate, fp);
      ::Read(&mdate, fp);
      ::Read(&adate, fp);
      return GDT_TRUE;
    }
  Clear();
  PushBackObjID(objINODE, fp);
  return GDT_FALSE;
}

static char *encode64(char *ptr, size_t siz, UINT8 num)
{ 
  char tmp[13];
  /* 64 characters */
  char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-";
  const size_t modulo = sizeof(chars)/sizeof(char) - 1;
  const size_t pad    = siz - 1;  
  UINT8        val = num;
  size_t i = 0, j; 

  do
    {
      tmp[i++] = chars[ val % modulo ];
      val /= modulo;
    } 
  while (val);
  for (j =i; j < pad; j++)
    tmp[j] = '0';
  for (j = 0; j < pad; j++)
    ptr[j] = tmp[pad -  j - 1];
  ptr[j] = '\0';
//  while (*ptr == '0') ptr++;
  return ptr;
}

static INT8 host = -1; 

STRING  INODE::Key() const
{
  STRING key;
  char ptr[10];

  key = "I$";
  key +=  encode64(ptr, 9, st_ino);
  if (st_dev == 0 || st_dev == -1)
    {
      if (host == -1) host = _IB_Hostid();
      key += ".";
      key += encode64(ptr, 9, host);
    }
  else
     key += encode64(ptr, 5, st_dev);
  return key;
}


STRING INODE::Key(off_t start, off_t end) const
{
  STRING key = Key(); // Get Key
  if (start != 0 || (end != 0 &&  end != st_size))
    {
      char ptr[8];
      key += "-";
      if (start != 0)
	key += encode64(ptr, 7, start);
      if (end != st_size)
	{
	  key += "-";
	  key += encode64(ptr, 5, start-end);
	}
    }
  return key;
}



#define INOSIZE  (8*sizeof(ino_t))


ino_t    INODE::_get_ino_t() const
{
#ifdef _WIN32
#if (SIZEOF_LONG_INT != SIZEOF_LONG_LONG_INT)
  if (sizeof(ino_t) < sizeof(_ib_ino_t)) /* transform 64-bits ino into 16-bits by hashing */
    {
       UINT4 low  = UINT8_low_part(st_ino);
       UINT4 high = UINT8_high_part(st_ino);
       return (ino_t)( ( (low) ^ ((low) >> INOSIZE)) ^ ((high ) ^ ( high >> INOSIZE)) ) ;
     }
#endif
#endif
  return st_ino;
}

