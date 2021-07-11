// $Id: rldcache.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $
/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1996. 

All Rights Reserved.
************************************************************************/

/*@@@
File:		rldcache.cxx
Version:	$Revision: 1.1 $
Description:	Cache & cache entry class
Author:		Kevin Gamiel (CNIDR)
Notes:          Converted to C++ class by A. Warnock (warnock@clark.net)
@@@*/

#define NOCODE_XX 1

//
// Remote/local document caching
//
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>

#define _RLDCACHE_CXX 1

#include "common.hxx"

# include "db.h"

#undef open
#undef close

#include "rldcache.hxx"

char GTime[256];
FILE *rldcache_log;
int GPid;

#define RLDCACHE_LOGFILE "/tmp/rldcache.log"


/*

class BerkeleyDB 
{
public:
        BerkeleyDB(FILE *fp);
        ~BerkeleyDB();
private:
        Db *dbp;
        Dbc *dbcp;
};
*/


const INT NEW_ENTRY     = 0;
const INT UPDATE_ENTRY  = 1;
const INT CORRECT_ENTRY = 2;

// #define USE_PROXY

RLDCACHE* Cache = NULL;

/// Here is the RLDCACHE class
RLDCACHE::RLDCACHE() 
{
  TTL=0;
  dbp = NULL;
  CacheState=NO_CACHE;
  CreateInit(STRING().form("/tmp/rldcache_%ld.db", (long)getpid() ), GDT_FALSE);
}


RLDCACHE::RLDCACHE(const STRING& NewPath) 
{
  TTL=0;
  dbp=NULL;
  CacheState=NO_CACHE;
  CreateInit(NewPath, GDT_FALSE);
}


RLDCACHE::RLDCACHE(const STRING& NewPath, GDT_BOOLEAN ForceNew) 
{
  TTL=0;
  dbp=NULL;
  CacheState=NO_CACHE;
  CreateInit(NewPath,ForceNew);
}


/*
Pre:	Path is the location of the file cache.
	MaxBlocks is recommended max number of 1K blocks of storage to be 
		used by cache
Notes:	Looks in directory Path for a file named rldcache.dat and reads
	current cache information.
*/
void RLDCACHE::CreateInit(const STRING& NewPath, GDT_BOOLEAN ForceNew)
{
  struct stat Stats;
  INT         read_write;
  INT         ret;

  if (NewPath.IsEmpty())
    {
      logf (LOG_ERROR, "RLDCACHE::Init: Invalid Path [NULL]");
      return;
    }
  if (!WritableDir( RemoveFileName (NewPath) ))
    {
      logf (LOG_ERRNO, "RLDCACHE::CreateInit '%s': Not in writable directory.", NewPath.c_str());
      return;
    }
  // Build the class path variable  
  DataFile= NewPath;

#if 0
  if ((ret = db_create(&dbp, NULL, 0)) != 0)
    {
      // Not created - bail out
      logf (LOG_ERROR, "RLDCACHE::CreateInit %s", DataFile.c_str());
      return;
    }
#endif
  if (ForceNew || stat(DataFile.c_str(), &Stats) != 0) {
    // The cache does not exist, co create it
    logf (LOG_DEBUG, "RLDCACHE::CreateInit: Creating new cache [%s]", DataFile.c_str());
    read_write = 0; // DBM does not need any special flags
    if ((CacheState = CacheOpen(read_write)) != OPEN_WRITE) {
      if (CacheState !=  NO_CACHE)
	{
	  // Something is clearly wrong...
	  logf (LOG_ERRNO, "RLDCACHE::CreateInit '%s'",  DataFile.c_str());
	}
      return;
    }
  } else {
    // The file exists
    if (!S_ISREG(Stats.st_mode)) {
      // Something is horribly wrong - not a regular file
      logf (LOG_ERRNO, "RLDCACHE::CreateInit '%s' not a regular file?", DataFile.c_str());
      return;
    } 
#ifdef RLDCACHE_DEBUG
cerr << "Open old.." << endl;
#endif

    // Make sure it opens nicely - we may want to change this to provide
    // a way to open it readonly for non-owners (like the http owner)
    read_write = 0; // DBM does not need any special flags
    CacheState = CacheOpen(read_write);
    if (CacheState != OPEN_WRITE) {
#ifdef RLDCACHE_DEBUG
cerr << "Readonly?" << endl;
#endif
      // Maybe it belongs to someone else - see if we can open
      // it for read
      CacheState = CacheOpenReadonly();
      if (CacheState != OPEN_READ) {
	// Something is clearly wrong...
	logf (LOG_ERRNO, "RLDCACHE::CreateInit '%s' other owner?", DataFile.c_str());
	return;
      }
    } 
  }

#ifdef RLDCACHE_DEBUG
  cerr << "Debug..." << endl;
  GPid = getpid();
  rldcache_log = fopen(RLDCACHE_LOGFILE, "a");
  strbox_TheTime(GTime);
  fprintf(rldcache_log, "%s %i *********** New Process [%i] *********\n",
	  strbox_TheTime(),GPid, GPid);
#endif
  //  CacheClose();
  return;
}


RLD_State RLDCACHE::CacheOpen(INT read_write)
{
  INT mode=0664;
  INT ret;

#ifdef RLDCACHE_DEBUG
cerr << "CacheOpen.." << endl;
#endif

  logf (LOG_DEBUG, "CacheOpen(%d)", read_write);

  // If the database is open for read and the request is to open it for
  // a write, we have to close it and reopen it.
  if (CacheState == OPEN_READ) 
    CacheClose();

  if (!dbp) {
    CacheState = NO_CACHE;
    return CacheState;
  }

  // If the database is closed, just open it as requested
  if ((CacheState == CLOSED) || (CacheState == NO_CACHE)) {
#ifdef RLDCACHE_DEBUG
cerr << "Open.. " << endl;
#endif
    if (
#if NOCODE_XX 
	1
#else
	(ret = dbp->open(dbp, 
			 DataFile.c_str(), 
			 NULL, 
			 DB_HASH, 
			 DB_CREATE, 
			 0664)) != 0
#endif
					) {
#ifdef RLDCACHE_DEBUG
cerr << "Nope!" << endl;
#endif
#if !NOCODE_XX
      dbp->err(dbp, ret, "%s", DataFile.c_str());
#endif
      CacheState = NO_CACHE;
      logf (LOG_ERRNO, "Could not open cache"); 
      return CacheState;
    }
  } 
#ifdef RLDCACHE_DEBUG
cerr << "Got it!" << endl;
#endif
  CacheState = OPEN_WRITE;

  if (read_write == OPEN_READ) {
    CacheState = OPEN_READ;
#ifdef RLDCACHE_DEBUG
    cerr << "Cache opened readonly" << endl;
#endif
    return CacheState;
  } 

  logf(LOG_DEBUG, "Cache opened for write");
  return CacheState;
}


RLD_State RLDCACHE::CacheOpenReadonly()
{
  //  return(CacheOpen(GDBM_READER));
  return CLOSED;
}


void RLDCACHE::CacheClose()
{
  logf (LOG_DEBUG, "CacheClose()");
  if (dbp)
    {
#if !NOCODE_XX
      dbp->close(dbp, 0);
#endif
      CacheState = CLOSED;
      logf (LOG_DEBUG, "Cache closed");
    } else
      CacheState = NO_CACHE;
}


void RLDCACHE::Delete()
{
  if (this == NULL_RLDCACHE)
    return;
  if (!DataFile.IsEmpty())
    {
      DeleteFiles(0L);
      CacheClose();
      unlink(DataFile);
    }
#ifdef RLDCACHE_DEBUG
  fclose(rldcache_log);
#endif
  return;
}


/*
Name:	RLDCACHE::GetFile
Desc:	Accepts queries for files via URL or local file name and returns
	the requested portion of the document.
Pre:	File is a URL or local file name
	Start is the starting byte offset into the file
	Count is the number of bytes to return from Start position
	Len returns the actual length of the file read
	TTL is the time to live in seconds.
	No flags defined yet
Post:
Notes:	If you want the entire document, request a Count of -1
*/
BYTE* RLDCACHE::GetFile(const STRING& File, INT Start, INT Count, size_t *Len, time_t ttl, INT Flags)
{
  CHR       Author[1024],Response[10];
  BYTE     *buf;
  CHR      *FileName;
  RLDENTRY *NewEntry;
  FILE     *fp;
  time_t    CurTime;
  INT       err, State, OldSecs, CurSecs;
  int      InCache;
	
  // Do we already have the requested URL (or filename) in cache?
  InCache = EntryExists(File);

  if (!InCache) {

#ifdef RLDCACHE_DEBUG
cerr << File << " Not in Cache" << endl;
#endif

#ifdef RLDCACHE_DEBUG
    strbox_TheTime(GTime);
    fprintf(rldcache_log, "%s %i Checking in/out [%s]\n",
	    GTime, GPid, File);
#endif

    State = NEW_ENTRY;

  } else {
#ifdef RLDCACHE_DEBUG
cerr << "Get " << File << " from Cache" << endl;
#endif
    // Has the TTL expired?
    NewEntry = GetEntryByName(File);

    if (NewEntry == NULL_RLDENTRY) {
      State = NEW_ENTRY;
    } else {
      time(&CurTime);
      if((CurTime - NewEntry->_TimeStamp) > ttl) {
	// Yes, document has expired.  Get a fresh copy

#ifdef RLDCACHE_DEBUG
	strbox_TheTime(GTime);
	fprintf(rldcache_log, "%s %i Refreshing [%s]\n",
		GTime, GPid, File);
#endif

	State = UPDATE_ENTRY;
      } else {

#ifdef RLDCACHE_DEBUG
	strbox_TheTime(GTime);
	fprintf(rldcache_log, "%s %i Checking out [%s]\n",
		GTime, GPid, File);
#endif
	
	State = CORRECT_ENTRY;
      }
    }	
  }

  if ((State == NEW_ENTRY) || (State == UPDATE_ENTRY)) {
    INT      err;
    RLDENTRY Entry;
#if 0
    int fd = mkstemp("rldcacheXXXXXX");

#else
    char     scratch[ L_tmpnam+1];
    char    *TmpName = tmpnam( scratch );
    if ((fp = fopen(TmpName, "w")) == NULL) {
      logf (LOG_ERRNO, "RLDCACHE::GetFile: Can''t open '%s'", TmpName);
      return NULL;
    }
#endif
    // Retrieve the file by URL and stuff in into the file TmpName
    if ((err=ResolveURL(File,fp, Len)) <= 0) {
      if (err==URL_NOT_AVAILABLE)
	logf (LOG_ERROR, "Resolve '%s': Non-existent URL.", File.c_str());
      else 
	logf (LOG_ERROR, "RLDCACHE::GetFile: Bad URL type in '%s'", File.c_str());
      fclose(fp);
      return NULL;
    }
    fclose(fp);
    time(&CurTime);

    //  Now, build the entry and stuff it into the cache index
    NewEntry = new RLDENTRY(File, TmpName, *Len, CurTime);
    if (NewEntry == NULL_RLDENTRY) {
      return NULL;
    }

    // Update the cache data file
    if (UpdateEntry(NewEntry) == -1) {
      return NULL;
    }
  }
  // Requested file is in the cache now
#ifdef RLDCACHE_DEBUG
cerr << "File in the cache now.." << endl;
#endif

#ifdef RLDCACHE_DEBUG
  rldentry_Print(NewEntry, stdout);
#endif

  buf = new BYTE[NewEntry->_Length];
  if (buf == NULL)
    logf (LOG_PANIC|LOG_ERRNO, "RLDCACHE::GetFile: Out of memory");

  if ((fp = fopen(NewEntry->_FileName, "r")) == NULL) {
    perror("RLDCACHE::GetFile");
    delete [] buf;
    return NULL;
  }

  if (fread(buf, sizeof(char), NewEntry->_Length, fp) != NewEntry->_Length) {
    perror("RLDCACHE::GetFile");
    delete [] buf;
    return NULL;
  }

  *Len=NewEntry->_Length;
  fclose(fp);	

#ifdef USE_PROXY
  unlink(NewEntry->_FileName);
  DeleteEntry(NewEntry);
#endif

  return buf;
}


//Pre:	Name is a URL or local file name
PRLDENTRY RLDCACHE::GetEntryByName(const STRING& Name)
{
/*
  RLDENTRY *Entry =  new RLDENTRY;
  INT       err;
  datum     dbm_key, return_data;
  CHR      *pkey;
  CHR      *pData;
 
  if (Entry == NULL_RLDENTRY) 
    return NULL_RLDENTRY;

  if (CacheState == CLOSED)
    CacheState = CacheOpenReadonly();

  if (CacheState == CLOSED) {
    logf (LOG_ERRNO, "RLDCACHE::GetEntryByName: %s", DataFile.c_str());
    return NULL_RLDENTRY;
  }

  pkey = Name;
  dbm_key.dptr = pkey;
  dbm_key.dsize = strlen(pkey)+1;
  err = gdbm_exists(dbf, dbm_key);

  if (err == 1)
    return_data = gdbm_fetch(dbf, dbm_key);
  else
    return NULL_RLDENTRY;

  if (return_data.dptr != NULL) {
    Entry->Name = Name;

    Entry->Unpack( return_data.dptr, return_data.dsize);

    free (return_data.dptr);
    
    return Entry;
  }
*/
  return NULL_RLDENTRY;
}


//Pre:	Name is a URL or local file name
GDT_BOOLEAN RLDCACHE::EntryExists(const STRING& Name)
{
/*
  RLDENTRY *Entry;
  INT       ret;
  datum     dbm_key;
  CHR      *pkey;
  CHR      *pData;
 
  if (CacheState == CLOSED)
    CacheState = CacheOpenReadonly();

  if (CacheState == CLOSED) {
    logf (LOG_ERRNO, "RLDCACHE::EntryExists: %s", DataFile.c_str());
    return GDT_FALSE;
  }

  pkey = Name;
  dbm_key.dptr = pkey;
  dbm_key.dsize = strlen(pkey)+1;
  ret = gdbm_exists(dbf, dbm_key);

  if (ret==1)
    return GDT_TRUE;
  else
    return GDT_FALSE;
*/
  return GDT_FALSE;
}


INT RLDCACHE::UpdateEntry(RLDENTRY *e)
{
/*
  INT       err;
  RLDENTRY  Entry;
  datum     dbm_key;
  datum     dbm_data;

  if (CacheState != OPEN_WRITE) 
    CacheState = CacheOpen( CacheState );

  if (CacheState == CLOSED) {
    logf (LOG_ERRNO, "RLDCACHE::UpdateEntry:Can't open cache %s", DataFile.c_str());
    return -1;
  }

  char *pkey = e->Name;

  char cLength[128], cTimeStamp[128];
  // Make the data buffer
  size_t l1 = strlen(e->FileName);
  size_t l2 = sprintf(cLength,"%ld", (long)e->Length);
  size_t l3 = sprintf(cTimeStamp,"%ld",(long)e->TimeStamp);

  // Pack into 
  // filename|Length|Timestamp
  CHR *dbm_buffer = new CHR [l1 + l2 + l3 +3];

  memcpy(dbm_buffer, e->FileName, l1); dbm_buffer[l1] = '|';
  memcpy(dbm_buffer+l1, cLength, l2); dbm_buffer[l1+l2+1] = '|';
  mempcy(dbm_buffer + l1+l2+2, cTimeStamp, l3); // Copy also the '\0'

  dbm_key.dptr = pkey;
  dbm_key.dsize = strlen(pkey)+1;
  dbm_data.dptr = dbm_buffer;
  dbm_data.dsize = l1+l2+l3+2;

#ifdef GDBM_DEBUG
  if (dbm_data.dptr != NULL) {
    printf ("key is  ->%s<-\n", dbm_key.dptr);
    printf ("data is ->%s<-\n\n", dbm_data.dptr);
  }
#endif
  logf (LOG_DEBUG, "Writing %s to cache.", e->Name.c_str());
  if(EntryExists(e->Name)) {
    // Open up the cache index and replace the entry
    err = gdbm_store(dbf, dbm_key, dbm_data, GDBM_REPLACE);

  } else {
    // Open it up and add the new entry
    err = gdbm_store(dbf, dbm_key, dbm_data, GDBM_INSERT);
  }

  if (err < 0)
    cout << "Error writing DBF record.\n";
  else if (err > 0)
    cout << "Record already in DBF.\n";
*/
  return 0;
}


INT RLDCACHE::DeleteEntry(RLDENTRY *e)
{
/*
  INT    err;
  datum  dbm_key, return_data;
  CHR   *pkey;
 
  if (CacheState == CLOSED)
    CacheState = CacheOpen( CacheState );

  if (!CacheState) {
    logf (LOG_ERRNO, "RLDCACHE::DeleteEntry: %s", DataFile);
    return -1;
  }

  if (EntryExists(e->Name)) {
    pkey = e->Name;
    dbm_key.dptr = pkey;
    dbm_key.dsize = strlen(pkey)+1;

    // Delete the entry from the cache index
    err = gdbm_delete (dbf, dbm_key);
  } 
*/
  return 0;
}


// This method deletes the temp files associated with expired cache
// entries.  It is called separately so that one can delete the files
// before just deleting the cache file instead of reorganizing the 
// cache file.  Files older than Minutes minutes will be deleted.
//
// Eventually, we may want to fix this so it only deletes files in /tmp
void RLDCACHE::DeleteFiles(off_t Minutes)
{
  /*
  RLDENTRY *Entry;
  time_t    time_now;
  time_t    MaxTime;
  datum     dbm_key, return_data;
  CHR      *pkey;
  CHR      *pData;

  dbm_key.dptr=(CHR*)NULL;
  
  MaxTime  = Minutes * 60L;
  time_now = time((time_t*)NULL);
  Entry    = new RLDENTRY;

  if (Entry == NULL_RLDENTRY) {
    logf (LOG_ERRNO, "RLDCACHE::DeleteFiles: failed to allocate RLDENTRY");
    return;
  }

  //  CacheClose();
  
  if (CacheState == CLOSED) {
    CacheState = CacheOpen(GDBM_READER);

    if (CacheState == CLOSED) {
      logf (LOG_ERRNO, "RLDCACHE::CleanCache: %s",  DataFile.c_str());
      return;
    }
  }

  return_data = gdbm_firstkey(dbf);
  
  // Walk through the cache and save the keys of items which need to 
  // be deleted.  We do not delete them yet because gdbm_delete may change
  // the hash table and cause some items to be missed.
  while (return_data.dptr != NULL) {
    // Now, we have the key - read the data
    dbm_key = return_data;
    return_data = gdbm_fetch(dbf, dbm_key);

    if (return_data.dptr != NULL) {

      Entry->Unpack( return_data.dptr, return_data.dsize );

      if ((time_now - Entry->TimeStamp) >= MaxTime) {
	// Delete the temp file now, and clean up the cache index next
	unlink(Entry->FileName);
      }
    }

    free (return_data.dptr); // this was mallocated by the gdbm_fetch call
    return_data = gdbm_nextkey(dbf, dbm_key);
  }
  delete Entry;
  */  
}


// This method deletes the temp files associated with expired cache
// entries, then cleans up the cache entries.  Files older than 
// Minutes minutes will be deleted.
// 
// Eventually, we may want to fix this so it only deletes files in /tmp
RLDENTRY *RLDCACHE::CleanCache(off_t Minutes)
{
  /*
  RLDENTRY *Entry;
  INT       err;
  time_t    time_now;
  time_t    MaxTime;
  datum     dbm_key, return_data;
  CHR      *pkey;
  CHR      *pData;
  STRLIST   KeyToDeleteList;
  STRING    KeyToDelete;
  size_t    delete_count;
  
  dbm_key.dptr = (CHR*)NULL;
  MaxTime  = Minutes * 60L;
  time_now = time((time_t*)NULL);
  Entry    = new RLDENTRY;

  if (Entry == NULL_RLDENTRY) {
    logf (LOG_ERRNO, "RLDCACHE::CleanCache: failed to allocate RLDENTRY");
    return NULL_RLDENTRY;
  }

  if (CacheState == CLOSED) {
    CacheState = CacheOpen(GDBM_WRITER);

    if (CacheState == CLOSED) {
      logf (LOG_ERRNO, "RLDCACHE::CleanCache: %s", DataFile.c_str());
      return NULL_RLDENTRY;
    }
  }

  return_data = gdbm_firstkey(dbf);

  // Walk through the cache and save the keys of items which need to 
  // be deleted.  We do not delete them yet because gdbm_delete may change
  // the hash table and cause some items to be missed.
  while (return_data.dptr) {

    Entry->Unpack( return_data.dptr, return_data.dsize );
    if ((time_now - Entry->TimeStamp) >= MaxTime) {
      KeyToDelete = Entry->Name;
      KeyToDeleteList.AddEntry(KeyToDelete);

      // Delete the temp file now, and clean up the cache index next
      unlink(Entry->FileName);
    }

    dbm_key = return_data;
    return_data = gdbm_nextkey(dbf, dbm_key);
    free (dbm_key.dptr);
  }

  // Now, walk through the STRLIST and delete each entry in
  delete_count = KeyToDeleteList.GetTotalEntries();

  for (size_t i=0; i<delete_count; i++) {
    KeyToDeleteList.GetEntry(i, &KeyToDelete);
    pData = KeyToDelete.NewCString();
    dbm_key.dptr = pData;
    dbm_key.dsize = strlen(pData)+1;
    err = gdbm_delete(dbf, dbm_key);
    delete [] pData;
  }

  delete Entry;
  */
  return NULL_RLDENTRY;
}


// Walks through the cache (by UID) and prints
// out all the file info found there
RLDENTRY *
RLDCACHE::WalkCache()
{
  /*
  RLDENTRY *Entry;
  FILE *fp;
  int err;

  Entry = new RLDENTRY;

  if(Entry == NULL_RLDENTRY) 
    return NULL_RLDENTRY;

  if((fp = fopen(DataFile, "r")) == NULL) {
    logf (LOG_ERRNO, "RLDCACHE::WalkCache: %s",  DataFile.c_str());
    return NULL_RLDENTRY;
  }
  do {
    if((err=fread(Entry,1,sizeof(RLDENTRY),fp))!=sizeof(RLDENTRY)) {
      fclose(fp);
      return NULL_RLDENTRY;
    }
    Entry->Print(stdout);
  } while(err > 0);
  fclose(fp);	
  */
  return NULL_RLDENTRY;
}


RLDCACHE::~RLDCACHE() {
  CacheClose();
}


////////////////////////////////////////////////////////////////////
// Here is the RLDENTRY Class
RLDENTRY::RLDENTRY()
{
  _Length   = 0;
  _TimeStamp = -1;
}

RLDENTRY::RLDENTRY(const STRING& Name, const STRING& FileName, size_t Len)
{
  CreateInit(Name, FileName, Len, time(NULL));
}



RLDENTRY::RLDENTRY(const STRING& Name, const STRING& FileName, size_t Len, time_t Time)
{
  CreateInit(Name, FileName, Len, Time);
}


void RLDENTRY::CreateInit(const STRING& NewName, const STRING& NewFileName, size_t NewLen, time_t NewTime)
{

  _Name      = NewName;
  _FileName  = NewFileName;
  _Length    = NewLen;
  _TimeStamp = NewTime;
}


void RLDENTRY::Print(FILE *fp)
{
  if(this == NULL_RLDENTRY)
    return;

  if(!_Name.IsEmpty())
    fprintf(fp, "Name:\t\t%s\n", _Name.c_str());
  if(_FileName.IsEmpty())
    fprintf(fp, "FileName:\t%s\n", _FileName.c_str());
	
  fprintf(fp, "Length:\t\t%ld\n", (long)_Length);
  fprintf(fp, "Time:\t\t%s\n", ctime(&_TimeStamp));
}


void RLDENTRY::Unpack(const char *pData, size_t Length)
{
  // Get Filename
  size_t i;
  for (i = 0; pData[i] != '|' && i< Length; i++)
    /* loop */;
  _FileName.Assign(pData, i++);

  // Get Length 
  char tmp[128];
  size_t j = 0; 
  while (pData[i] != '|' && i<Length)
    tmp[j++] = pData[i++];
  tmp[j] = '\0';
  _Length = (size_t)atol(tmp);
// i++;

  // Get Timestamp 
  _TimeStamp = (time_t) atol (&pData[++i]);

// j=0;
//  while ( i < Length)
//    tmp[j++] = pData[i++];
//  tmp[j] = '\0';
//  _TimeStamp = (time_t) atol(tmp);
}


RLDENTRY::~RLDENTRY()
{
}

