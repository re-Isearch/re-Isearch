/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		common.hxx
Description:	Common functions
@@@*/

#ifndef COMMON_HXX
#define COMMON_HXX

#include "defs.hxx"
#include "string.hxx"
#include "ctype.hxx"

#ifndef _WIN32
#include <sys/resource.h>
#endif


#ifdef _WIN32
# undef lstat
# define lstat stat
#endif

// Own wrappers over open/close(2)
extern "C" int  _IB_open(const char *path, int oflag = 0, ...);

int _IB_close(int fd);

int _IB_kernel_max_files();

int _IB_kernel_max_file_descriptors();
int _IB_kernel_max_streams();

/*
#define open	_IB_open
#define close	_IB_close
#define fopen   _IB_fopen
#define freopen _IB_freopen
*/


// Truncate
int EraseFileContents(const char *Filename);
int EraseFileContents(FILE *Fp);


UINT8 CRC64(const char *ptr, size_t length);
UINT8 CRC64(const STRING& Contents);
UINT8 CRC64(const char *ptr);


int _IB_fstat(int fd, struct stat *sb);
int _IB_stat(const char *path, struct stat *sb);
int _IB_lstat(const char *path, struct stat *sb);

#ifdef _WIN32
STRING FixMicrosoftPathNames(const STRING& Path);
STRING WinGetLongPathName(const STRING& Path);

#include <sys/time.h>
/* We don't have getrusage() */
struct rusage {
    struct timeval ru_stime;
    struct timeval ru_utime;
    int   ru_majflt;  // page faults requiring physical I/O (*)
    int   ru_minflt;  // page faults not requiring physical I/O
    int   ru_nswap;   // swaps
    int   ru_inblock; // block input operations
    int   ru_oublock; // block output operations
    int   ru_nvcsw;   // voluntary context switches
    int   ru_nivcsw;  // involuntary context switches
    int   ru_maxrss;  // maximum resident set size (*)
    int   ru_ixrss;   // currently 0
    int   ru_idrss;   // integral resident set size
    int   ru_isrss;   // 
    // Above: (*) are the only ones implemented in Win32
};
#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1
int getrusage(int who, struct rusage *usage);
#endif


STRING SnefruHash(const STRING& Input);
STRING SnefruHash(const char *CString);
STRING SnefruHash(const char *ptr, size_t length);

STRING OneWayHash(const STRING& Input);
STRING OneWayHash(const char *CString);
STRING OneWayHash(const char *ptr, size_t length);

int FileLink(const STRING& Source, const STRING& Dest);

void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const char *pattern = NULL,
  const char *exclude = NULL,
  const char *inclDirpattern = NULL,
  const char *excludeDirpattern = NULL,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);


void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const STRING& pattern,
  const STRING& exclude = NulString,
  const STRING& inclDirpattern = NulString,
  const STRING& excludeDirpattern = NulString,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);

void do_directory(const STRING& dir,
  int (*do_file)(const STRING&),
  const STRLIST *filePatternList,
  const STRLIST *excludeList = NULL,
  const STRLIST *inclDirList = NULL,
  const STRLIST *excludeDirList = NULL,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);

STRING FindExecutable(const STRING& Argv0=NulString);
STRING FindSharedLibrary(const STRING& Argv0=NulString);

#ifdef _WIN32
STRING GlobalSharedLibraryName ();
#endif

GDT_BOOLEAN IsAbsoluteFilePath(const STRING& Path);
GDT_BOOLEAN IsRootDirectory(const STRING& Path);
GDT_BOOLEAN DirectoryExists(const STRING& Path);
GDT_BOOLEAN FileExists(const STRING& Path); // Exists and not a directory
GDT_BOOLEAN ExeExists(const STRING& Path); // Is executable (script or bin)


CHR         PathSepChar();
GDT_BOOLEAN IsPathSep(const int c);
GDT_BOOLEAN ContainsPathSep(const STRING& Path);

void  AddTrailingSlash(PSTRING PathName);
void  RemoveTrailingSlash(PSTRING PathName);
void  RemovePath(PSTRING FileName);
void  RemoveFileName(PSTRING PathName);

int         RenameFile(const STRING& FromName, const STRING& ToName);
int         UnlinkFile(const STRING& Filename);
int         RmDir(const STRING& Dirname);

int         AddtoGarbageFileList(const STRING& Fn);


STRING      ChDir();
STRING      ChDir(const STRING& Dir);
STRING      GetCwd(); // Current Working Directory
STRING      SetCwd(const STRING& Dir); // Set the Working Directory

STRING      GetTempDir();
STRING      GetTempFilename(const char *Prefix=NULL, GDT_BOOLEAN Secure=GDT_FALSE);

GDT_BOOLEAN WritableDir(const STRING& path);

STRING GetUserHome(const char *user);

GDT_BOOLEAN SetUserName(const STRING& Id);

GDT_BOOLEAN SetUserGroup(const STRING& Id);
STRING      GetUserGroup();


STRING MakeTempFileName(const STRING& Fn);

GDT_BOOLEAN IsRunningProcess(long pid);
GDT_BOOLEAN IsMyProcess(long uid);

STRING ProcessOwner(long uid = 0);
STRING ResourceOwner(const STRING& Path);
STRING ResourcePublisher(const STRING& Path);

STRING AddTrailingSlash(const STRING& PathName);
STRING RemoveTrailingSlash(const STRING& Pathname);

STRING RemovePath(const STRING& FileName);
STRING RemoveFileName(const STRING& PathName);

#ifndef STANDALONE
STRING      ResolveBinPath(const STRING& Filename);
STRING      ResolveConfigPath(const STRING& Filename);
GDT_BOOLEAN ResolveConfigPath(PSTRING Filename);
#ifndef _WIN32
STRING      ResolveHtdocPath(const STRING& Filename, GDT_BOOLEAN AsUrl = GDT_FALSE);
#endif
#endif

STRING RelativizePathname(const char *Path, const char *Dir);
STRING RelativizePathname(const STRING& Path, const STRING& Dir);

int PathCompare(const char *Path1, const char *Path2);

void   __Realpath(STRING * pathPtr);
STRING __Realpath(const STRING& path);

const char *_IB_LIBPATH();
const char *_IB_BASEPATH();

off_t GetFileSize(PFILE FilePointer);
off_t GetFileSize(const STRING& filename);
off_t GetFileSize(const CHR *filename);
off_t GetFileSize(int fd);

GDT_BOOLEAN Exists(const STRING& path);

GDT_BOOLEAN IsExecutable(const STRING &path);

STRING ExpandFileSpec(const STRING& FileSpec);
void   ExpandFileSpec(PSTRING FileSpec);

int MkDir(const STRING& Dir, int mask, GDT_BOOLEAN Forced);
int MkDir(const STRING& Dir, int mask = 0);
GDT_BOOLEAN MkDirs(const STRING& Path, int mask = 0);

INT2  Swab (INT2 *Ptr);
UINT2 Swab (UINT2 *Ptr);
INT4  Swab (INT4 *Ptr);
UINT4 Swab (UINT4 *Ptr);
INT8  Swab (INT8 *Ptr);
UINT8 Swab (UINT8 *Ptr);


inline GPTYPE GpSwab (PGPTYPE GpPtr) { return Swab(GpPtr); }
inline GPTYPE GpSwab (GPTYPE gp) { return Swab( &gp ); }


// Printable data formats, ISO, obsolete ANSI
// and RFC (for WWW) formats
char *ISOdate (time_t t = 0); // Not MT-Safe
char *ANSIdate(time_t t = 0); // Not MT-Safe
char *RFCdate (time_t t = 0); // Not MT-Safe
char *LCdate  (time_t t = 0); // Not MT-Safe

// I/O
#ifndef _WIN32
struct timeval *tReadSetTimeout(struct timeval *tv); // Set timer
size_t tRead(int fd, void *bufptr, size_t length);
size_t tRead(int fd, void *bufptr, size_t length, struct timeval *tv);
#endif


GDT_BOOLEAN IsBigEndian();

char *Copystring (const char *s);
char *Copystring (const char *s, size_t len);

INT StrCaseCmp(const CHR* s1, const CHR* s2);
INT StrCaseCmp(const UCHR* s1, const UCHR* s2);
INT StrNCaseCmp(const CHR* s1, const CHR* s2, const INT n);
INT StrNCaseCmp(const UCHR* s1, const UCHR* s2, const INT n);
INT StrNCmp(const CHR* s1, const CHR* s2, const INT n);
INT StrNCmp(const UCHR* s1, const UCHR* s2, const INT n);

GDT_BOOLEAN FileGlob(const UCHR *pattern, const UCHR *str);
GDT_BOOLEAN Glob(const UCHR *pattern, const UCHR *str, GDT_BOOLEAN dot_special=GDT_FALSE);

int getHostname (char *buf, int len);

//
int getOfficialHostName(char *buf, int len);

// 

GDT_BOOLEAN _IB_GetPlatformName(char *buf, int maxSize);
GDT_BOOLEAN _IB_GetHostName(char *buf, int maxSize);
GDT_BOOLEAN _IB_GetUserId(char *buf, int maxSize);
GDT_BOOLEAN _IB_GetUserName(char *buf, int maxSize);
GDT_BOOLEAN _IB_GetmyMail (PSTRING Name, PSTRING Address);
long        _IB_Hostid();
rlim_t      _IB_GetFreeMemory();
rlim_t      _IB_GetTotalMemory();


ssize_t _sys_read(int fildes, void *buf, size_t nbyte);
#ifdef NEED_PREAD
ssize_t pread(int fildes, void *buf, size_t nbyte, off_t offset);
#endif
size_t pfread(FILE *fp, void *buf, size_t nbyte, off_t offset);

#endif
