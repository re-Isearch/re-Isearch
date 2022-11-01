/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/* Cache & cache entry class */

#ifndef _RLDCACHE_HXX
#define _RLDCACHE_HXX

#include <stdio.h>
#include <sys/time.h>

#include "common.hxx"
#include "protman.hxx"
#include "plist.hxx"

#define NULL_RLDCACHE (RLDCACHE *)NULL
#define NULL_RLDENTRY (RLDENTRY *)NULL
const INT RLDCACHE_MAXNAMELEN=1024;

//@ManMemo: Class of RLDCACHE entries
/*@Doc: Each object in this class represents a remote file which has been
  retrieved and stashed into a local temporary file.  The RLDENTRY object 
  allows the program to retrieve information necessary to get file 
  information without retrieving the file again from the net.
*/
class RLDENTRY {
friend class RLDCACHE;

public:
//@ManMemo: 
  RLDENTRY();
/*@Doc: */

//@ManMemo:
  RLDENTRY(const STRING& Name, const STRING& FileName, size_t Len);
/*@Doc: */

//@ManMemo: 
  RLDENTRY(const STRING& Name, const STRING& FileName, size_t Len, time_t Time);
/*@Doc: */

//@ManMemo: 
  void Create();
/*@Doc: */

//@ManMemo: 
  void CreateInit(const STRING& Name, const STRING& FileName, size_t Len, time_t Time);
/*@Doc: */

//@ManMemo: 
  void Print(FILE *fp);
/*@Doc: */

//@ManMemo: 
  ~RLDENTRY();
/*@Doc: */

private:
  STRING _Name;
  STRING _FileName;
  size_t _Length;
  time_t _TimeStamp;
  void   Unpack(const char* pData, size_t length);
};

typedef RLDENTRY *PRLDENTRY;

//@ManMemo: Disk cache class for holding remote files locally
/*@Doc: The RLDCACHE class is used to maintain directory information on 
  files retrieved by URL and stored in temporary files.
*/

enum RLD_State { NO_CACHE, CLOSED, OPEN_READ, OPEN_WRITE };

class RLDCACHE {

public:
//@ManMemo: RLDCACHE constructor
  RLDCACHE();
/*@Doc: Cache location defaults to /tmp */

//@ManMemo: RLDCACHE constructor
  RLDCACHE(const STRING& NewPath);
/*@Doc: Cache location is specified in NewPath */

//@ManMemo: RLDCACHE constructor
  RLDCACHE(const STRING& NewPath, bool ForceNew);
/*@Doc: Cache location is specified in NewPath */

//@ManMemo: Close the RLDCACHE and delete the cache file from disk
  void      Delete();
/*@Doc: Normally, we want the cache file to stick around for the duration
  of the program execution in case new files need to be accessed.  This is
  the final cleanup routine to delete the cache file and not just destroy 
  the current object. */

//@ManMemo: Retrieves a file from the RLDCACHE
  BYTE*      GetFile(const STRING& File, INT Start, INT Count, size_t *Len, 
		    time_t TTL, INT Flags);
/*@Doc: The file is retrieved by URL if it's not already in the cache. */

//@ManMemo: Get the entry by specifying the desired file by name
  RLDENTRY *GetEntryByName(const STRING& Name);
/*@Doc: The name is the retrieval key and the RLDENTRY associates Name
  with the local temp file name */

//@ManMemo: 
  INT       UpdateEntry(RLDENTRY *e);
/*@Doc: */

//@ManMemo: 
  bool  EntryExists(const STRING& NAME);
/*@Doc: */

//@ManMemo: 
  INT       DeleteEntry(RLDENTRY *e);
/*@Doc: */

//@ManMemo: 
  RLDENTRY *WalkCache();
/*@Doc: */

//@ManMemo: 
  void      DeleteFiles(off_t Minutes);
/*@Doc: */

//@ManMemo: 
  RLDENTRY *CleanCache(off_t Minutes);
/*@Doc: */

//@ManMemo: 
  ~RLDCACHE();
/*@Doc: */

private:
  // Variables
  STRING     DataFile;
  time_t     TTL;
  PLIST     *Entries;
#ifdef _RLDCACHE_CXX
  DB        *dbp;
#endif
  RLD_State  CacheState;

  // Methods
  void       CreateInit(const STRING& Path, bool ForceNew);
  void       CreateInit(const STRING& Path, const STRING& AppName, bool ForceNew);
  RLD_State  CacheOpenReadonly();
  RLD_State  CacheOpen(INT Mode);
  void       CacheClose();
};

typedef RLDCACHE *PRLDCACHE;
#endif


