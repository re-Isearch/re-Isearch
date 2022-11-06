/*@@@
File:		rldcache.hxx
Version:	1.0
$Revision$
Description:	Cache & cache entry class
@@@*/

#ifndef _RLDCACHE_HXX
#define _RLDCACHE_HXX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef UNIX
#include <sys/time.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>

#define DBM44 1

#if defined(BDB) || defined(DBM) || defined(DBM41) || defined (DBM42)  || defined (DBM44) || defined (DBM44)
#include <db.h>
#elif defined(GDBM)
#include <gdbm.h>
#else
#include <dbm.h>
#endif

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "strlist.hxx"
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
  //  RLDENTRY(char* Name, char* FileName, size_t Len, time_t Time);
  //  RLDENTRY(STRING& Name, STRING& FileName, size_t Len, time_t Time);
  RLDENTRY(const STRING& Name, const STRING& FileName, time_t Time);
  RLDENTRY(const STRING& Name, const STRING& FileName, size_t Len, time_t Time);
/*@Doc: */

//@ManMemo: 
  void Create();
/*@Doc: */

//@ManMemo: Initialize the RLDENTRY object
  void CreateInit(const STRING& Name, const STRING& FileName, size_t Len, time_t Time);
/*@Doc: */

//@ManMemo: Turn debugging output on
  void  debug_on(void) { debug = 1; }

//@ManMemo: Turn debugging output off
  void  debug_off(void) { debug = 0; }

//@ManMemo: Print the entry on the specified file
  void Print(FILE *fp);
/*@Doc: */

//@ManMemo: Set the entry name
  void SetName(const STRING& NewName) { Name = NewName; }
/*@Doc: */

//@ManMemo: Get the entry name
  STRING GetName() { return Name; }
/*@Doc: */

//@ManMemo: Set the name of the local cache entry
  void SetFileName(const STRING& NewFileName) { FileName = NewFileName; }
/*@Doc: */

//@ManMemo: Get the name of the local cache entry
  STRING GetFileName() { return FileName; }
/*@Doc: */

//@ManMemo: Set the file length for the entry
  void SetLength(const size_t NewLength) { Length = NewLength; }
/*@Doc: */

//@ManMemo: Retrieve the file length for the entry
  size_t GetLength() { return Length; }
/*@Doc: */

//@ManMemo: Set the timestamp for the entry
  void SetTimeStamp(const time_t NewTimeStamp) { TimeStamp = NewTimeStamp; }
/*@Doc: */

//@ManMemo: Retrieve the timestamp for the entry
  time_t GetTimeStamp() { return TimeStamp; }
/*@Doc: */

//@ManMemo: Opens the cached file associated with the entry
  FILE* OpenFile(const char *mode);
/*@Doc: */

//@ManMemo: Checks to see if the cache entry has expired
  bool Expired(time_t TTL);
/*@Doc: Returns GDT_TRUE if the file has expired (that is, if the entry 
        older than the TTL */

//@ManMemo: 
  ~RLDENTRY();
/*@Doc: */

private:
  INT        debug;      // 1 = show debugging, 0=hide
  STRING     Name;
  STRING     FileName;
  size_t  Length;
  time_t  TimeStamp;
  void       Unpack(char *pData);
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
  //  RLDCACHE(char *NewPath);
 /*@Doc: Cache location defaults to /tmp */
  RLDCACHE(const STRING& NewPath = NulString, bool ForceNew = false);
/*@Doc: Cache location is specified in NewPath */

//@ManMemo: RLDCACHE constructor
  RLDCACHE(STRING& NewPath, STRING& CacheName, bool ForceNew);
/*@Doc: Cache location is specified in NewPath */

//@ManMemo: Turn debugging output on
  void  debug_on(void) { debug = 1; }

//@ManMemo: Turn debugging output off
  void  debug_off(void) { debug = 0; }

//@ManMemo: Close the RLDCACHE and delete the cache file from disk
  void      Delete();
/*@Doc: Normally, we want the cache file to stick around for the duration
  of the program execution in case new files need to be accessed.  This is
  the final cleanup routine to delete the cache file and not just destroy 
  the current object. */

//@ManMemo: Retrieves a file from the RLDCACHE
  FILE     *Fopen(const STRING& File, const char* mode);
/*@Doc: The file is retrieved by URL if it's not already in the cache. */

//@ManMemo: Retrieves a file from the RLDCACHE
  RLDENTRY *GetFile(const STRING& URL);
/*@Doc: The file is retrieved by URL if it's not already in the cache. */

//@ManMemo: Get the entry by specifying the desired file by name
  //  RLDENTRY *GetEntryByName(const char *Name);
  RLDENTRY *GetEntryByName(const STRING& Name);
/*@Doc: The name is the retrieval key and the RLDENTRY associates Name
  with the local temp file name */

//@ManMemo: Updates the existing entry in the RLDCACHE
  INT       UpdateEntry(RLDENTRY *e);
/*@Doc: */

//@ManMemo: Checks to see if the entry already exists in the RLDCACHE
  bool EntryExists(STRING& NAME);
/*@Doc: */

//@ManMemo: Deletes an entry from the RLDCACHE
  INT       DeleteEntry(RLDENTRY *e);
/*@Doc: */

//@ManMemo: Walks the cache, visiting each entry/key one time
  RLDENTRY *WalkCache();
/*@Doc: */

//@ManMemo: Deletes temp files older than Minutes from the RLDCACHE
  void      DeleteFiles(time_t Minutes);
/*@Doc: */

//@ManMemo: Cleans out the RLDCACHE of expired entries
  RLDENTRY *CleanCache(time_t Minutes);
/*@Doc: */

//@ManMemo: Sets the Time To Live for entries in the RLDCACHE
  void      SetTTL(time_t new_ttl) { TTL = new_ttl; }
/*@Doc: */

//@ManMemo: Retrieves the current value of TTL
  time_t    GetTTL() { return TTL; }
/*@Doc: */

//@ManMemo: Sets the path to the stored local files
  void SetPath(const STRING& NewPath) { Path = NewPath; }
/*@Doc: */

//@ManMemo: Returns the path to the stored local files
  STRING GetPath() const { return Path; }
/*@Doc: */

//@ManMemo: Sets the name of the RLDCACHE file (not the contents)
  void SetCacheFilename();
  void SetCacheFilename(const STRING& NewCacheName) 
  { DataFile = NewCacheName; }
/*@Doc: */

//@ManMemo: Returns the name of the RLDCACHE file
  STRING GetCacheFilename() const { return DataFile; }
/*@Doc: */

//@ManMemo: Object destructor
  ~RLDCACHE();
/*@Doc: */

private:
  // Variables
  bool       debug;      // 1 = show debugging, 0=hide
  STRING     Path;
  STRING     DataFile;
  time_t     TTL;
  PLIST     *Entries;
  RLD_State  CacheState;

#if defined(GDBM)
  GDBM_FILE *Cache_dbp;
#else
  DB        *Cache_dbp;
#endif

  // Methods
//@ManMemo: 
  void       CreateInit(STRING& Path, bool ForceNew);
/*@Doc: */

//@ManMemo: 
  RLD_State  CacheOpenReadonly();
/*@Doc: */

//@ManMemo: 
  RLD_State  CacheOpen();
/*@Doc: */

//@ManMemo: 
  RLD_State  CacheOpen(RLD_State mode);
/*@Doc: */

//@ManMemo: 
  RLD_State  CacheReopen();
/*@Doc: */

//@ManMemo: 
  RLD_State  CacheReopen(RLD_State mode);
/*@Doc: */

//@ManMemo: 
  void       CacheClose();
/*@Doc: */
};

typedef RLDCACHE *PRLDCACHE;
#endif


