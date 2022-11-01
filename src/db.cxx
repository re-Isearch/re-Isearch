/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "db.hxx"
#include <errno.h>
#include <stdlib.h>
#include <fstream>
#include <malloc.h>
#include <unistd.h>

// Where do I need this for? I don't know.
int errno;

// Default cache size in kilobytes.
// Maybe this should be an config option, just for easy testing and
//   determination for best system performance
// NOTE: page size is 1KB - do not change!!
#define CACHE_SIZE_IN_KB 64

DB::DB()
{
  isOpen = false;
}


DB::~DB()
{
  if (isOpen)
    Close();
}


int DB::OpenReadWrite(const STRING& filename, int mode)
{
  // Initialize the database environment.
  dbenv = db_init((char *)NULL);
  memset(&dbinfo, 0, sizeof(dbinfo));
// dbinfo.db_cachesize = CACHE_SIZE_IN_KB * 1024;	// Cachesize: 64K.
  dbinfo.db_pagesize = 1024;      			// Page size: 1K.

  // Create the database.
  if (access(filename, F_OK) == 0)
    errno = db_open(filename, DB_BTREE, 0, 0, dbenv, &dbinfo, &dbp);
  else
    errno = db_open(filename, DB_BTREE, DB_CREATE, mode, dbenv, &dbinfo, &dbp);
  if (errno == 0)
    {
      // Acquire a cursor for the database.
      if ((seqrc = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
	{
          seqerr = seqrc;
	  isOpen = 0;
          Close();
	  return NOTOK;
        }
      Path = filename;
      isOpen = 1;
      return OK;
    }
  Path.Clear();
  return NOTOK;
}


int DB::OpenRead(const STRING& filename)
{
    //
    // Initialize the database environment.
    //
    dbenv = db_init((char *)NULL);
    memset(&dbinfo, 0, sizeof(dbinfo));
//    dbinfo.db_cachesize = CACHE_SIZE_IN_KB * 1024;	// Cachesize: 64K.
    dbinfo.db_pagesize = 1024;				// Page size: 1K.

    //
    // Open the database.
    //
    if ((errno = db_open(filename, DB_BTREE, DB_RDONLY, 0, dbenv,
			 &dbinfo, &dbp)) == 0)
    {
        //
	// Acquire a cursor for the database.
	//
        if ((seqrc = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0)
	{
            seqerr = seqrc;
	    isOpen = 0;
            Close();
	    return NOTOK;
        }
	isOpen = 1;
	return OK;
    }
    else
    {
	return NOTOK;
    }
}


int DB::Close()
{
    if (isOpen)
    {
	//
	// Close cursor, database and clean up environment
	//
        (void)(dbcp->c_close)(dbcp);
	(void)(dbp->close)(dbp, 0);
	(void) db_appexit(dbenv);
    }
    isOpen = 0;
    return OK;
}


void DB::Start_Get()
{
    DBT	nextkey;

    //
    // skey and nextkey are just dummies
    //
    memset(&nextkey, 0, sizeof(DBT));
    memset(&skey, 0, sizeof(DBT));

//    skey.data = "";
//    skey.size = 0;
//    skey.flags = 0;
    if (isOpen && dbp)
    {
	//
	// Set the cursor to the first position.
	//
        seqrc = dbcp->c_get(dbcp, &skey, &nextkey, DB_FIRST);
	seqerr = seqrc;
    }
}


STRING DB::Get_Next()
{
    //
    // Looks like get Get_Next() and Get_Next_Seq() are pretty much the same...
    //
    DBT	nextkey;
	
    memset(&nextkey, 0, sizeof(DBT));

    if (isOpen && !seqrc)
    {
	STRING lkey((char *)skey.data, 0, skey.size);
	skey.flags = 0;
        seqrc = dbcp->c_get(dbcp, &skey, &nextkey, DB_NEXT);
	seqerr = seqrc;
	return lkey;
    }
    else
	return NulString;
}

void DB::Start_Seq(const STRING& Key)
{
    DBT	nextkey;

    memset(&skey, 0, sizeof(DBT));
    memset(&nextkey, 0, sizeof(DBT));

    skey.data = Key.c_str();
    skey.size = Key.length();
    if (isOpen && dbp)
    {
	//
	// Okay, get the first key. Use DB_SET_RANGE for finding partial
	// keys also. If you set it to DB_SET, and the words book, books
	// and bookstore do exists, it will find them if you specify
	// book*. However if you specify boo* if will not find
	// anything. Setting to DB_SET_RANGE will still find the `first'
	// word after boo* (which is book).
	//
        seqrc = dbcp->c_get(dbcp, &skey, &nextkey, DB_SET_RANGE);
	seqerr = seqrc;
    }
}


STRING DB::Get_Next_Seq()
{
    DBT	nextkey;
	
    memset(&nextkey, 0, sizeof(DBT));

    if (isOpen && !seqrc)
    {
        STRING lkey((char *)skey.data, 0, skey.size);

	skey.flags = 0;
        seqrc = dbcp->c_get(dbcp, &skey, &nextkey, DB_NEXT);
	seqerr = seqrc;
	return lkey;
    }
  return 0;
}

int DB::Put(const STRING &key, const STRING &data)
{
    DBT	k, d;

    memset(&k, 0, sizeof(DBT));
    memset(&d, 0, sizeof(DBT));

    if (!isOpen)
	return NOTOK;

    k.data = key.get();
    k.size = key.length();

    d.data = data.get();
    d.size = data.length();

    //
    // A 0 in the flags in put means replace, if you didn't specify DB_DUP
    // somewhere else...
    //
    return (dbp->put)(dbp, NULL, &k, &d, 0) == 0 ? OK : NOTOK;
}


int DB::Get(const STRING &key, STRING &data)
{
    DBT	k, d;

    memset(&k, 0, sizeof(DBT));
    memset(&d, 0, sizeof(DBT));

    k.data = key.get();
    k.size = key.length();

    int rc = dbp->get(dbp, NULL, &k, &d, 0);
    if (rc)
	return NOTOK;

    data = 0;
    data.append((char *)d.data, d.size);
    return OK;
}


int DB::Exists(const STRING &key)
{
    STRING data;

    if (!isOpen)
	return 0;

    return Get(key, data);
}


int DB::Delete(const STRING &key)
{
    DBT	k;

    memset(&k, 0, sizeof(DBT));

    if (!isOpen)
	return 0;

    k.data = key.get();
    k.size = key.length();

    return (dbp->del)(dbp, NULL, &k, 0);
}


DB *DB::getDatabaseInstance()
{
    return new DB();
}


/*
 * db_init --
 *      Initialize the environment. Only returns a pointer
 */
DB_ENV *DB::db_init(char *home)
{
  DB_ENV *dbenv;

  // Rely on calloc to initialize the structure.
  if ((dbenv = (DB_ENV *)calloc(sizeof(DB_ENV), 1)) == NULL)
    message_log (LOG_PANIC, "Insufficent Core!");
  else
    {
      dbenv->db_errfile = stderr;
      dbenv->db_errpfx = progname;
      if ((errno = db_appinit(home, NULL, dbenv, DB_CREATE)) != 0)
	{
	  message_log (LOG_PANIC, "db_appinit %s failed!", home.c_str())
	}
    }
    return (dbenv);
}

