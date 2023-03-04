/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)idb.cxx"

/*
    Portions Copyright (c) 1995 CNIDR/MCNC, (c) 1992-2011 BSn/Munich;
    (c) 2011-2020 NONMONOTONIC Networks;
    Copyright (c) 2020-21 Edward C. Zimmermann for the re-iSearch project.

    This software was made possible by a grant of NLnet Foundation and the European
    Union NGI0 (Next Generation Internet Zero) Search initiative.
*/
/************************************************************************
************************************************************************/

#define NEW_HEADLINE_CACHE_CODE 0 /* BROKEN!! */

/*@@@
File:        idb.cxx
Description: Class IDB
@@@ */
#ifdef __GNUG__ 
#pragma implementation "idbobj.hxx"
#endif

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include "dirent.hxx"
#include "common.hxx"
#include "idb.hxx"
#include "nlist.hxx"
#ifdef HAVE_LOCALE
#include <locale.h>
#endif
#include "lock.hxx"
#include "lang-codes.hxx"

//#include <iostream>
#include <fstream>

#ifdef _WIN32
# define PAGESIZE 4096
#endif
#ifndef PAGESIZE
# ifdef _SC_PAGESIZE
#  define PAGESIZE sysconf(_SC_PAGESIZE)
# else
#  ifdef PAGE_SIZE
#    define PAGESIZE PAGE_SIZE
@  else
#    define PAGESIZE (1U<<12) /* 4096 */
#  endif
# endif
#endif
#ifndef PAGEOFFSET
# define  PAGEOFFSET  (PAGESIZE - 1)
#endif

const INT  DEFAULT_MaximumRecordSize = 2147483647L/40; // 50 MB
;
const int   MaxHeadlineLen = 256;


// Once upon a time machines with 64 MB RAM where considered **loaded**
const size_t MinimumMemSize = (((1L << 20)*7L)/(PAGESIZE*3) + 1)*PAGESIZE;
const size_t DefaultMemSize = (((1L << 20)*64L)/(PAGESIZE) + 1)*PAGESIZE;

#ifndef FILENAME_MAX
# define FILENAME_MAX 1024
#endif

#ifndef _NO_LOCKS
# ifdef _WIN32
#  define _NO_LOCKS 1
# else
#  define _NO_LOCKS 0
# endif
#endif

/*
REAL_NAME:     Edward C. Zimmermann
EMAIL_ADDRESS:    edz@nonmonotonic.com
ORGANIZATION: Nonmonotonic Networks 
*/
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

#define DONT_LOCK 1
#define USE_OLD_LOCKS 0

#if DONT_LOCK
# define lockfd(f,nb)
# define unlockfd(f)
#else
#if USE_OLD_LOCKS
/*
 * flock operations.
 */
# ifndef LOCK_SH
#  define LOCK_SH      1  /* shared lock */
#  define LOCK_EX      2  /* exclusive lock */
#  define LOCK_NB      4  /* don't block when locking */
#  define LOCK_UN      8  /* unlock */
# endif
#  include <sys/file.h>
#  define lockfd(f,nb)  flock(f, LOCK_EX | (nb ? LOCK_NB : 0), 0)
#  define unlockfd(f)   flock(f, LOCK_UN)
extern "C" {
  int flock(int fd, int operation);
}
#else          /* new way */
#  include <unistd.h>
#  define lockfd(f,nb)  lockf(f, (nb ? F_TLOCK : F_LOCK), 0)
#  define unlockfd(f)   lockf(f, F_ULOCK, 0)
#endif
#endif

static const STRING CacheReadMeFile   ("#READ.ME#");

static const CHR *RegistrationTitle = "Isearch";

// Profile main section
static const STRING DbInfoSection       ("DbInfo");
static const STRING DbOptionsSection    ("DbOptions");
static const STRING FieldTypeSection    ("FieldTypes");
static const STRING EngineSection       ("Engine");
static const STRING externalSortSection ("External Sort");
//
static const STRING DatabasesEntry      ("Databases");
// Bits
static const STRING IndexBitsEntry      ("IndexBits");
// Indexer control Entries
static const STRING VersionEntry        ("VersionNumber");
//
static const STRING VersionIDEntry      ("IB-Signature");
static const STRING BuildDateEntry      ("Build");
// DB specified Entries
static const STRING DocTypeEntry        ("DocType");
static const STRING TitleEntry          ("Title");
static const STRING SegmentNameEntry    ("Segment");
static const STRING LocaleEntry         ("Locale");
static const STRING StoplistEntry       ("StopList");
static const STRING DateEntry           ("DateCreated");
static const STRING CommentsEntry       ("Comments");
static const STRING CopyrightEntry      ("Copyright");
static const STRING DateModifiedEntry   ("DateLastModified");
static const STRING CacheSizeEntry      ("CacheSize");
static const STRING CacheNameEntry      ("CacheName");
static const STRING CachePersistEntry   ("Persist");
static const STRING SearchCutoffEntry   ("SearchCutoff");
static const STRING SearchTooManyEntry  ("MaxRecordsAdvice");
static const STRING MaximumRecordSizeEntry ("MaxRecordSize");
static const STRING PluginsPathEntry    ("PluginsPath");
static const STRING CacheDirEntry       ("CacheDir");
static const STRING WorkingDirEntry     ("BaseDir");
static const STRING useRelativePathsEntry ("useRelativePaths");

static const STRING autoDeleteExpiredEntry ("AutoDeleteExpired");

static const STRING MaintainerNameEntry ("Maintainer.Name");
static const STRING MaintainerMailEntry ("Maintainer.Email");


static const STRING RankingSection      ("Ranking");
static const STRING PriorityFactorEntry ("PriorityFactor");
static const STRING IndexBoostEntry     ("IndexBoostFactor");
static const STRING FreshnessBoostEntry ("FreshnessBoostFactor");
static const STRING FreshnessDateEntry  ("FreshnessBaseDateLine");
static const STRING LongevityBoostEntry ("LongevityBoostFactor");


void IDB::GetFieldDefinitionList(STRLIST *StrlistPtr) const
{
  if (MainRegistry)
    MainRegistry->GetEntryList(FieldTypeSection, StrlistPtr);
}

IDB::IDB()
{
  Parent = NULL;
  Initialized = false;

  Initialize (NulString, NulString, NulStrlist, false);
}

IDB::IDB( bool SearchOnly)
{
  Parent = NULL;
  Initialized = false;

  Initialize (NulString, NulStrlist, SearchOnly);
}

IDB::IDB (const STRING& DBName, bool SearchOnly)
{
  Parent = NULL;
  Initialized = false;

  Initialize (DBName, NulStrlist, SearchOnly);
}

IDB::IDB(const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  Parent = NULL;
  Initialized = false;

  Initialize (DBName, NewDocTypeOptions, SearchOnly);
}

IDB::IDB(IDBOBJ *myParent, const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  Initialized = false;

  Initialize (myParent, DBName, NewDocTypeOptions, SearchOnly);
}


IDB::IDB (const STRING& NewPathName, const STRING& NewFileName, bool SearchOnly)
{
  Parent = NULL;
  Initialized = false;

  Initialize (NewPathName, NewFileName, NulStrlist, SearchOnly);
}

IDB::IDB (const STRING& NewPathName, const STRING& NewFileName,
	const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  Parent = NULL;
  Initialized = false;

  Initialize (NewPathName, NewFileName, NewDocTypeOptions, SearchOnly);
}

void IDB::Initialize (const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  return Initialize(NULL, DBName, NewDocTypeOptions, SearchOnly);
}

void IDB::Initialize (IDBOBJ *myParent, const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  // Initialize the database
  const STRING  fullpath ( ExpandFileSpec(DBName.IsEmpty() ? __IB_DefaultDbName : DBName));
  if (DirectoryExists(fullpath) && !FileExists( fullpath + DbExtDbInfo)) {
    // The index is probably embedded in a subdirectory...   path/DB/DB
    Initialize (myParent, fullpath,  RemovePath (fullpath), NewDocTypeOptions, SearchOnly);
  } else // path/DB
    Initialize (myParent, RemoveFileName(fullpath),  RemovePath (fullpath), NewDocTypeOptions, SearchOnly);
}

void IDB::ClearWorkingDirectoryEntry()
{
  if (MainRegistry)
   MainRegistry->ProfileWriteString(DbInfoSection,  WorkingDirEntry, NulString); 
}

void IDB::SetWorkingDirectory(const STRING& Dir)
{
  if (PathCompare(Dir, WorkingDirectory) != 0)
    {
      IDBOBJ::SetWorkingDirectory(Dir);
      if (MainRegistry)
	{
	  message_log (LOG_DEBUG, "Working Dir = '%s'", WorkingDirEntry.c_str());
	  MainRegistry->ProfileWriteString(DbInfoSection,  WorkingDirEntry,
		RemoveTrailingSlash(WorkingDirectory));
	  DbInfoChanged = true;
	}
    }
}


bool IDB::setAutoDeleteExpired(bool val)
{
  if (val != autoDeleteExpired && MainRegistry)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, autoDeleteExpired, val);
      DbInfoChanged = true;
      if (val) {
         DeleteExpired(); // We do it now!
      }
    }
  return true;
}



bool IDB::setUseRelativePaths(bool val)
{
  if (val != useRelativePaths && MainRegistry)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, useRelativePathsEntry, val);
      DbInfoChanged = true;
    }
  if ((useRelativePaths = val) == true) return !WorkingDirectory.IsEmpty();
    return true;
}

void IDB::Initialize (const STRING& NewPathName, const STRING& NewFileName,
            const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
 return Initialize(Parent, NewPathName, NewFileName, NewDocTypeOptions, SearchOnly);
}


void IDB::Initialize (IDBOBJ *myParent, const STRING& NewPathName, const STRING& NewFileName,
	    const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  // This is where the real initialization takes place
  __Register_IB_Application(); // Make sure we've registered..

  Parent = myParent;

  lastPeerField = 0;

  Queue1Add = Queue2Add = 0;

  compatible         = true;
  errorCode          = 0;
  MetaDefaults       = NULL;
  //DebugSkip          = 0;
  Override           = false;
  DebugMode          = false;
  autoDeleteExpired  = false; // Default is NO to increase performance.
  TotalRecordsQueued = 0;

  ActiveSortIndex    = -1;
  SortIndexFp        = NULL;

  useRelativePaths   = false;

  MainIndex          = NULL;
  MainMdt            = NULL;
  MainDfdt           = NULL;
  MainRegistry       = NULL;
  DocTypeReg         = NULL;

  SegmentNumber      = 0;
  Open(NewPathName, NewFileName, NewDocTypeOptions, SearchOnly); 
}

bool IDB::Open (const STRING& DBName, bool SearchOnly)
{
  return Open (DBName, NulStrlist, SearchOnly);
}

bool IDB::Open (const STRING& DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  const STRING  fullpath ( ExpandFileSpec(DBName.IsEmpty() ? __IB_DefaultDbName : DBName));
  return Open (RemoveFileName(fullpath),  RemovePath (fullpath), NewDocTypeOptions, SearchOnly);
}


bool IDB::Open (const STRING& NewPathName, const STRING& NewFileName,
            const STRLIST& NewDocTypeOptions, bool SearchOnly)
{
  if (MainIndex || MainMdt || MainMdt || MainRegistry || DocTypeReg || MetaDefaults)
    {
      message_log (LOG_ERROR, "IDB \"%s\" not closed. Close before open \"%s\".", DbFileStem.c_str(), NewFileName.c_str());
      return false;
    }
  SetGlobalCharset();
  message_log (LOG_DEBUG, "Initial charset #%d", (int)((BYTE)GetGlobalCharset()));

  // Set the Path component..
  if (NewPathName.IsEmpty())
    DbPathName = GetCwd();
  else
    DbPathName = AddTrailingSlash (ExpandFileSpec ( AddTrailingSlash( NewPathName ) ));

  // Set the filename component...
  if (NewFileName.IsEmpty())
    {
      DbFileName = __IB_DefaultDbName;
    }
  else
    {
      if (NewFileName.Search(PathSepChar()))
	DbPathName.Cat (RemoveFileName(NewFileName));
      DbFileName     = RemovePath (NewFileName);
      // Set the stem 
    }

  DbFileStem     = DbPathName + DbFileName;

  message_log (LOG_DEBUG, "IDB::Open '%s'", DbFileStem.c_str());


  if (!DirectoryExists(DbPathName))
    {
      message_log (LOG_WARN, "Can't read/write to DB '%s': Directory '%s' does not exist!",
		DbFileName.c_str(), DbPathName.c_str()); 
      SetErrorCode(109); // "Database unavailable";
    }
//cerr << "DbPathName = " << DbPathName << endl;

  IndexingStatus (IndexingStatusInit, DbFileStem);

  DocTypeOptions = NewDocTypeOptions;
/*
  // Make sure it is not read locked
  LockWait(DbFileStem, L_READ, 360);
*/
#if _NO_LOCKS
#else
  if (LockWait(DbFileStem, L_WRITE|L_CHECK))
    message_log (LOG_NOTICE, "Indexing process running!!!!!!!!!");
#endif

  // Load DbInfo file
  DbInfoChanged = false;

  try {
    MainRegistry = new REGISTRY (RegistrationTitle);
  } catch (...) {
    MainRegistry = NULL;
  }
  if (MainRegistry == NULL)
    {
      message_log (LOG_PANIC, "Can't allocate REGISTRY");
      return false; // Can't continue;
    }

  int    have_db_ini = 0;
  STRING ini_file ( ResolveConfigPath("_default.ini") );

  if (FileExists(ini_file))
    {
      message_log(LOG_DEBUG, "Loading %s (%s)", ini_file.c_str(), "Defaults");
      have_db_ini = MainRegistry->ProfileLoadFromFile(ini_file);
    }
  else
    message_log (LOG_DEBUG, "%s (%s) does not exist", ini_file.c_str(), "Defaults");

  if (FileExists( ini_file = ComposeDbFn (DbExtDbInfo)  ))
    {
      message_log (LOG_DEBUG, "Loading %s (%s)", ini_file.c_str(), "Inits");
      have_db_ini += MainRegistry->ProfileAddFromFile (ini_file);
    }
  else if (SearchOnly)
    {
      if (DbFileName != __IB_DefaultDbName)
	message_log (LOG_INFO, "%s (%s) does not exist. New Index?", ini_file.c_str(), "Inits");
    }
  else if (!WritableDir(DbPathName))
    {
      message_log (LOG_WARN, "Don't have permission to write to '%s'.", DbFileStem.c_str()); 
    }
 

  // Add .ini options
  if (have_db_ini)
    {
      STRING Entry, Value;
      INT i;
      const char Option[] = "Option[%d]"; 
      const char nOption[] = "[%d]"; 

      message_log(LOG_DEBUG, "Got %d profile strings", have_db_ini);

      for (i=0;;)
        {
          MainRegistry->ProfileGetString(DbOptionsSection, Entry.form(nOption, ++i), NulString, &Value);
          if (Value.IsEmpty())
            break;
          DocTypeOptions.AddEntry (Value);
        }
     if (--i > 0)
        message_log (LOG_INFO, "Loaded %d Options set using the Obsolete [%s] %s format.",
                i, DbOptionsSection.c_str(), nOption);

      for (i=0;;)
        {
          MainRegistry->ProfileGetString(DbInfoSection, Entry.form(Option, ++i), NulString, &Value);
          if (Value.IsEmpty())
            break;
          DocTypeOptions.AddEntry (Value);
        }
       if (Parent)
	{
	  for (i=0;;)
	    {
	      Parent->ProfileGetString(DbInfoSection, Entry.form(Option, ++i), NulString, &Value);
	      if (Value.IsEmpty())
		break;  
	      DocTypeOptions.AddEntry (Value);
	    }
        }
   }
  else if ( SearchOnly == true && (DbFileName != __IB_DefaultDbName))
   message_log (LOG_INFO, "Empty initialization files (Default and DB specific)??");

  if (!SetLocale ((const char *)NULL))
    message_log(LOG_ERROR, "Could set set default locale, check environment!");

  // Get Indexing Bits
  {
    int index_bits;
    MainRegistry->ProfileGetString(DbInfoSection, IndexBitsEntry, 8*sizeof(GPTYPE), &index_bits);
    if (index_bits != 8*sizeof(GPTYPE))
      {
	compatible = false;
	SetErrorCode(-8*(int)sizeof(GPTYPE));
      }
  }

  // Get PriorityFactor
  MainRegistry->ProfileGetString(RankingSection, PriorityFactorEntry, 0, &PriorityFactor);
  if (PriorityFactor == 0 && Parent)
    PriorityFactor = Parent->ProfileGetString(RankingSection, PriorityFactorEntry);

/*
  DOUBLE      IndexBoostFactor;
  DOUBLE      FreshnessBoostFactor;
  DOUBLE      LongevityBoostFactor;
  SRCH_DATE   FreshnessBaseDateLine;
*/

  // Get Max Record Size
  MainRegistry->ProfileGetString(DbInfoSection, MaximumRecordSizeEntry, DEFAULT_MaximumRecordSize, &MaximumRecordSize);

  if (MaximumRecordSize <= 0)
    MaximumRecordSize = INT_MAX;

  // Get Maintainer (default is name and e-mail address)
  MainRegistry->ProfileGetString(DbInfoSection, MaintainerNameEntry, NulString, &MaintainerName);
  MainRegistry->ProfileGetString(DbInfoSection, MaintainerMailEntry, NulString, &MaintainerMail);
  // Get SegmentName
  MainRegistry->ProfileGetString(DbInfoSection, SegmentNameEntry, DbFileName, &SegmentName);
  // Get Title
  MainRegistry->ProfileGetString(DbInfoSection, TitleEntry, DbFileName, &Title);
  // Get Working Directory
  MainRegistry->ProfileGetString(DbInfoSection, WorkingDirEntry, DbPathName, &WorkingDirectory);
  useRelativePaths = MainRegistry->ProfileGetBoolean(DbInfoSection, useRelativePathsEntry);

  message_log (LOG_DEBUG, "Base Directory (Working)='%s' Use Relative Paths? %s", WorkingDirectory.c_str(),
	useRelativePaths ? "Yes" : "No");

  // Get Comments
  {
    STRLIST Remarks;
    MainRegistry->ProfileGetString(DbInfoSection, CommentsEntry, &Remarks);
    Remarks.Join(' ', &Comments);
  }
  // Get Copyright
  {
    STRLIST Remarks;
    MainRegistry->ProfileGetString(DbInfoSection, CopyrightEntry, &Remarks);
    Remarks.Join(' ', &Copyright);
  }

  // Get Database List (Used by VIDB class)
  MainRegistry->ProfileGetString(DbInfoSection, DatabasesEntry, DbFileName, &DatabaseList);

  // Get BASE of HT tree
  // Get Server Name and Port: http://host:port
  // Read doctype options
  if (DocTypeOptions.GetValue ("HTTP_Server", &HTpath) == 0)
    {
      MainRegistry->ProfileGetString("HTTP", "Server", NulString, &HTpath);
      if (HTpath.GetLength() == 0)
	{
	  const char *host = getenv ("SERVER_NAME");
	  const char *port = getenv ("SERVER_PORT");
	  if (host && *host)
	    {
	      HTpath <<  "http://" << host;
	      if (port && atoi(port) != 80)
		HTpath << ":" << port;
	      HTpath << "/";
	    }
       }
      else
	AddTrailingSlash(&HTpath);
    }
  else
    AddTrailingSlash(&HTpath);

  // Get the Root directory of the HT Tree
  isMirror = false;
  if (DocTypeOptions.GetValue ("WWW_ROOT", &HTDocumentRoot) == 0)
    {
      if (DocTypeOptions.GetValue ("HTTP_PATH", &HTDocumentRoot) == 0)
	{
	  if (DocTypeOptions.GetValue ("HTDOCS", &HTDocumentRoot) == 0)
	    {
	      if (DocTypeOptions.GetValue ("MIRROR_ROOT", &HTDocumentRoot))
		{
		  isMirror = true;
		}
	    }
	}
    }
  // The options OVER-RIDE the environment variables!
  if (HTDocumentRoot.IsEmpty())
    {
      char *tp = getenv("WWW_ROOT"); 
      if (tp == NULL) tp = getenv("HTTP_PATH");
      if (tp == NULL) tp = getenv("HTDOCS");
      if (tp) HTDocumentRoot = tp;
    }
  // Now look in .ini
  // The DB.ini overrides
  STRING s;
  MainRegistry->ProfileGetString("Mirror", "Root", NulString, &s);
  if (s.GetLength())
    {
      HTDocumentRoot = s;
      isMirror = true;
    }
  else
    {
      MainRegistry->ProfileGetString("HTTP", "Pages", NulString, &s);
      if (s.GetLength())
	{
	  isMirror = false;
	  HTDocumentRoot = s;
	}
    }
  if (HTDocumentRoot.GetLength ())
    {
#if 1
      STRING Cwd;
      // Make sure its right
      if ((Cwd = ChDir(HTDocumentRoot)).GetLength())
	{
	  if (!IsAbsoluteFilePath(HTDocumentRoot))
	    HTDocumentRoot = Cwd;
	  else if (PathCompare(HTDocumentRoot.c_str(), Cwd.c_str()) != 0)
	    message_log (LOG_INFO, "Warning: HTDocumentRoot '%s'!='%s'.", HTDocumentRoot.c_str(), Cwd.c_str());
	}
      else if (DirectoryExists( HTDocumentRoot ))
	message_log (LOG_ERROR, "Can't cwd to selected %sRoot \"%s\"",
		isMirror ? "Mirror" : "Page", HTDocumentRoot.c_str());
#endif
      AddTrailingSlash(&HTDocumentRoot);
    }
  message_log (LOG_DEBUG, "Root of htdocs tree='%s'", HTDocumentRoot.c_str());

  INT CacheSize = 4;
  if (DocTypeOptions.GetValue (CacheSizeEntry, &CacheSize) == 0)
    MainRegistry->ProfileGetString(DbInfoSection, CacheSizeEntry, CacheSize, &CacheSize);
  message_log (LOG_DEBUG, "[%s] %s=%d", DbInfoSection.c_str(), CacheSizeEntry.c_str(), CacheSize);

  if (CacheSize > 0)
    {
      MainRegistry->ProfileGetString(DbInfoSection, CacheNameEntry, NulString, &PersistantIrsetCache);
      if (PersistantIrsetCache.IsEmpty())
	{
	  // Do I need a default?
	  STRING tmp;
	  MainRegistry->ProfileGetString(DbInfoSection, CachePersistEntry, NulString, &tmp);
	  message_log (LOG_DEBUG, "[%s] %s=%s", DbInfoSection.c_str(), CachePersistEntry.c_str(), tmp.c_str());
	  if (!tmp.IsEmpty())
	    {
	      if (tmp.GetBool())
		PersistantIrsetCache = ComposeDbFn (DbExtCache);
	    }
	}
      if (!PersistantIrsetCache.IsEmpty())
	message_log (LOG_DEBUG, "Using %s as persistant irset cache (size %d)", PersistantIrsetCache.c_str(), CacheSize);
    }

  // Get the CacheDir
  STRING dir;
  MainRegistry->ProfileGetString(DbInfoSection, CacheDirEntry, NulString, &dir);
  SetCacheDir(dir);


  // Automatically delete expired records?
  autoDeleteExpired = MainRegistry->ProfileGetBoolean(DbInfoSection, autoDeleteExpiredEntry);


  // Create INDEX
  try {
    MainIndex = new INDEX (this, ComposeDbFn (DbExtIndex), CacheSize);
  } catch (...) {
    MainIndex = NULL;
  }
  if (MainIndex != NULL)
    {
      if (MainIndex->CheckIntegrity() == false)
	message_log (LOG_PANIC, "Index '%s' is corrupt. Search may be impaired.", DbFileStem.c_str()) ;

      INT Clip = 0;
      MainRegistry->ProfileGetString(DbInfoSection, SearchCutoffEntry, 0, &Clip);
      message_log (LOG_DEBUG, "[%s] %s=%d", DbInfoSection.c_str(), SearchCutoffEntry.c_str(), Clip);
      if (Clip) SetDbSearchCutoff(Clip);

      MainRegistry->ProfileGetString(DbInfoSection, SearchTooManyEntry, 0, &Clip);
      message_log (LOG_DEBUG, "[%s] %s=%d", DbInfoSection.c_str(), SearchTooManyEntry.c_str(), Clip);
      if (Clip)
	{
	  MainIndex->SetMaxRecordsAdvice(Clip);
	}
      else if ((Clip = MainIndex->GetMaxRecordsAdvice()) != 0 && !SearchOnly)
	{
	  MainRegistry->ProfileWriteString(DbInfoSection, SearchTooManyEntry, Clip);
	  DbInfoChanged = true;
	}
    }
  else
    {
      message_log (LOG_PANIC, "Failed to create INDEX instance!");
      return false;
    }


  if (SearchOnly)
    MainIndex->SetMergeStatus( iNothing );

  // Create and load MDT
  try {  
    MainMdt = new MDT(MainIndex); 
  } catch (...) {
    MainMdt = NULL;
  }
  if (MainMdt == NULL)
    {
      message_log (LOG_PANIC, "Failed to create MainMdt!");
      return false;
    }

  // Create DFD
  try {
    MainDfdt = new DFDT( ComposeDbFn (DbExtDfd) );
  } catch (...) {
    MainDfdt = NULL;
  }
  if (MainDfdt == NULL)
    {
      message_log (LOG_PANIC, "Failed to create MainDfdt!");
      return false;
    }

  // Setup FieldTypes
  const size_t totalDfdtElements=MainDfdt->GetTotalEntries();
  for (size_t i=1;i<=totalDfdtElements;i++)
    {
      DFD dfd;

      if (MainDfdt->GetEntry(i, &dfd))
	{
	  FIELDTYPE FieldType = dfd.GetFieldType();
	  if (!FieldType.IsText())
	    AddFieldType(dfd.GetFieldName(), FieldType);
	}
    }

  IndexingMemory = 0; // Set Memory
  // Set Doctype
  STRING PluginsSearchPath;
  if (DocTypeOptions.GetValue (PluginsPathEntry, &PluginsSearchPath) == 0)
    MainRegistry->ProfileGetString(DbInfoSection, PluginsPathEntry, NulString, &PluginsSearchPath);
  if (PluginsSearchPath.IsEmpty())
    DocTypeReg = new DTREG(this);
  else
    DocTypeReg = new DTREG(this, PluginsSearchPath);

  // Set Default Volume stuff
  SetVolume(DbFileStem, 0);

  // GlobalDoctype;
  IDB::GetGlobalDoctype();

  // Get Global stoplist
  ProfileGetString(DbInfoSection, StoplistEntry, NulString, &StoplistFileName);
  if (StoplistFileName.Equals("C"))
    StoplistFileName.Clear();
  else if (! StoplistFileName.Equals("<NULL>"))
    SetStoplist(StoplistFileName);
  errno = 0;

  // Now if we wanted to automatically delete expired
  if (autoDeleteExpired)
  {
     // Delete Expired
     size_t  deleted = DeleteExpired();
     if (deleted)
       message_log (LOG_INFO, "autoDeleteExpired: deleted %d records.", deleted);
  }

  return true;
}


METADATA *IDB::GetMetadefaults(const STRING& MdType)
{
  if (MetaDefaults == NULL)
    MetaDefaults = new METADATA (MdType);
  return MetaDefaults;
}


// For complete we could just zap the whole CacheDir directory but
// we won't. Perhaps there is something else there and we don't want
// to delete things we did not know that we created ourselves.
//
// What do we create?
// - The OID files for headlines
// - The *.F files for the fulltext
// 

static int UnlinkFileOrDir(const STRING& Path)
{
  if (::unlink(Path.c_str()) == 0)
    {
      STRING Dir ( Path );
      while ( (Dir = RemoveTrailingSlash(RemoveFileName(Dir))).GetLength() > 2)
	{
	  int lasterror = errno;
	  if (::rmdir( Dir.c_str() ) != 0)
	    {
	      errno = lasterror; 
	      break;
	    }
	}
      return 0; // OK
    }
  return -1;
}

bool IDB::KillCache(bool Complete) const
{
  if (!CacheDir.IsEmpty())
    {
      struct stat     st_buf;
      // Lets make sure the CacheDir exists..
      if (0 == stat(CacheDir.c_str(), &st_buf) && S_ISDIR(st_buf.st_mode))
	{
	  STRLIST list;
	  // All the OIDs start with 1.2.840.10003.5.
	  STRING  Hpattern ("1.2.840.10003.*");
	  STRING  Fpattern ("*.F"); // For cached fulltext files 

          list.AddEntry ( Hpattern );
	  list.AddEntry ( Fpattern ); 

	  message_log (LOG_INFO, "Persistant Cache: Removing '%s' files and '%s' dirs under %s",
		Hpattern.c_str(), Fpattern.c_str(), CacheDir.c_str());
	  ::do_directory(CacheDir, UnlinkFileOrDir, &list, NULL, NULL, NULL, true, Complete);
	  if (Complete)
	    {
	      ::remove(CacheReadMeFile);
	      if (RmDir(CacheDir.c_str()) == -1)
		message_log (LOG_ERRNO|LOG_NOTICE, "Could not remove directory '%s'.", CacheDir.c_str());
	    }
		
	}
    }
  return true;
}

bool IDB::FillHeadlineCache(const STRING& RecordSyntax)
{
  if (!CacheDir.IsEmpty() && MainMdt)
    {
      const size_t Total = GetTotalRecords ();
      if (Total)
	message_log (LOG_INFO, "Filling the %s headline cache (%u slots) in %s",
		RecordSyntax.c_str(), (unsigned)Total, CacheDir.c_str());
      for (size_t i=1; i<= Total; i++)
	{
	  MDTREC mdtrec;
	  if (MainMdt->GetEntry(i, &mdtrec))
	    {
	      STRING S;
	      RESULT Result(mdtrec);

	      Result.SetVirtualIndex( Volume.GetVolume() );
	      Result.SetMdtIndex(i);
	      Headline(Result, RecordSyntax, &S);
	      message_log (LOG_DEBUG, "Filling %u with %s", (unsigned)i, S.c_str());
	    }
	}
      return Total != 0;
    } else message_log(LOG_ERROR, "No directory set for a headline cache.");
  return false;
}

bool IDB::SetCacheDir(const STRING& Dir)
{
  bool Ok = false;
  STRING      newCacheDir;

  message_log (LOG_DEBUG, "SetCacheDir('%s')", Dir.c_str());

  if (!Dir.IsEmpty() && !(Dir ^= "<NULL>"))
    {
      struct stat     st_buf;
      // Lets make sure the CacheDir exists..
      if (0 == stat(Dir.c_str(), &st_buf))
	{
	  if (S_ISDIR(st_buf.st_mode))
	    {
	      newCacheDir = AddTrailingSlash(AddTrailingSlash(Dir) + DbFileName);
	      if (0 == stat(newCacheDir.c_str(), &st_buf) && !S_ISDIR(st_buf.st_mode))
		message_log (LOG_ERROR, "'%s' is a file. Can't use for present cache!", newCacheDir.c_str());
	      else
		Ok = true;
	    }
	  else
	    message_log(LOG_WARN, "'%s' specified for base of present cache is not a directory", Dir.c_str());
	}
      else
	message_log(LOG_WARN, "Present cache base directory '%s' does not exist. Ignoring", Dir.c_str());
    }

  if (Ok)
    {
      if (newCacheDir != CacheDir && MainRegistry)
        {
	  MainRegistry->ProfileWriteString(DbInfoSection, CacheDirEntry, Dir);
	  DbInfoChanged = true;
	  if (!CacheDir.IsEmpty())
	    {
	      message_log (LOG_DEBUG, "Moving %s to %s", CacheDir.c_str(), newCacheDir.c_str());
	      if (RenameFile(CacheDir, newCacheDir) == -1)
		{
		  message_log (LOG_ERRNO, "Could not move cache dir from %s to %s",
			CacheDir.c_str(), newCacheDir.c_str());
		  KillCache(true);
		}
	    }
	  FlushMainRegistry(); // Write it out
	}
      CacheDir = newCacheDir;
      if (!Exists(CacheDir))
	{
	  if (MkDir(CacheDir, 0001777) == -1)
	    {
	      message_log (LOG_ERRNO, "Could not create '%s' (Cache Dir)", CacheDir.c_str());
	    }
	  else
	    {
	      const STRING Readme (CacheDir+CacheReadMeFile);
	      ofstream Ofs (Readme.c_str());
	      if (Ofs && Ofs.good())
		Ofs << "IMPORTANT NOTICE:" << endl << endl <<
" These files and (sub)directories are used to cache \"Presentation\" data" << endl <<
" for an IB " << __IB_Version << " compatible Fulltext Database:" << endl
	<< "\t" << DbFileStem << endl << endl <<
" The files are volatile and will automatically (as long as the IB option" << endl <<
" is set) be re-created/updated as needed so you may remove any or all of" << endl <<
" them. Some files use so-called file-system holes so do not copy!" << endl;
	      else
		message_log (LOG_ERRNO, "Could not create '%s'", Readme.c_str());
	    }
	}
      message_log(LOG_DEBUG, "Using %s for present cache", CacheDir.c_str());
    }
  else
    CacheDir = NulString;
  return Ok;
}


bool IDB::SetDateRange(const DATERANGE& DateRange)
{
  return MainIndex ? MainIndex->SetDateRange(DateRange) : false;
}

bool IDB::SetDateRange(const SRCH_DATE& From, const SRCH_DATE& To)
{
  DATERANGE DateRange (From, To);

  return SetDateRange(DateRange);
}

bool IDB::GetDateRange(DATERANGE *DateRange) const
{
  return MainIndex ? MainIndex->GetDateRange(DateRange) : false;
}


bool IDB::GetHTTP_server(PSTRING path) const
{
  if (path)
    {
      *path = HTpath;
    }
  return HTpath.GetLength() != 0;
}

bool IDB::GetHTTP_root(PSTRING path, bool *Mirror) const
{
  if (path)    *path = HTDocumentRoot;
  if (Mirror)  *Mirror = isMirror;
  return HTDocumentRoot.GetLength() != 0;
}

bool IDB::SetLocale (const LOCALE& Locale)
{
  if (GlobalLocale != Locale)
    {
      message_log (LOG_DEBUG, "Setting DB Locale %s --> %s",
	GlobalLocale.LocaleName().c_str(), Locale.LocaleName().c_str());
      GlobalLocale = Locale;
      SetGlobalCharset(GlobalLocale.GetCharsetId());
    }
  return (INT)GlobalLocale != 0;
}

bool IDB::SetLocale (const CHR *Locale)
{
  STRING LocaleName;
  if (Locale == NULL || *Locale == '\000')
    {
      // Get Locale
      MainRegistry->ProfileGetString (DbInfoSection, LocaleEntry, NulString, &LocaleName);
      if (LocaleName.IsEmpty())
	{
	  const char *def_charset = getenv ("LC_CTYPE");
	  const char *def_lang    = getenv("LANG");

	  if (def_charset == NULL || *def_charset == '\0')
	    {
	      if ((def_charset = getenv ("LC_ALL")) == NULL || *def_charset == '\0')
		def_charset = "iso_8859_1";      // Latin-1 is default
	    }
	  if (def_lang == NULL || *def_lang)
	    def_lang = "en";
	  LocaleName.form ("%s.%s", def_lang, def_charset);
	}
    }
  else
    LocaleName = Locale;
  return SetLocale (LOCALE(LocaleName));
}

void IDB::SetStoplist(const STRING& Filename)
{
  if (MainIndex)
    MainIndex->SetStoplist(Filename);
}

bool IDB::KeyLookup (const STRING& Key, PRESULT ResultBuffer) const
{
  size_t res = MainMdt->LookupByKey (Key);
  if (res && ResultBuffer)
    {
      MDTREC Mdtrec;
      if (MainMdt->GetEntry(res, &Mdtrec))
	{
	  ResultBuffer->SetVirtualIndex( Volume.GetVolume() );
	  ResultBuffer->SetMdtIndex( res );
	  ResultBuffer->SetKey ( Mdtrec.GetKey () );
	  ResultBuffer->SetDocumentType (  Mdtrec.GetDocumentType () );
#if 1
	  ResultBuffer->SetPathname (Mdtrec.GetPathname());
//cerr << "SetOrigPathname ... " << endl;
	  ResultBuffer->SetOrigPathname (Mdtrec.GetOrigPathname());
#else
	  ResultBuffer->SetPath ( Mdtrec.GetPath () );
	  ResultBuffer->SetFileName ( Mdtrec.GetFileName () );
#endif
	  ResultBuffer->SetRecordStart (Mdtrec.GetLocalRecordStart ());
	  ResultBuffer->SetRecordEnd (Mdtrec.GetLocalRecordEnd ());

	  ResultBuffer->SetDate (Mdtrec.GetDate());
	  ResultBuffer->SetDateCreated (Mdtrec.GetDateCreated());
	  ResultBuffer->SetDateModified (Mdtrec.GetDateModified());
	  ResultBuffer->SetLocale (Mdtrec.GetLocale());
	}
    }
  return res != 0;
}

STRING IDB::FirstKey() const
{
  // first record
  MDTREC mdtrec;
  const size_t Total = MainMdt->GetTotalEntries (); 
  for (size_t idx = 1; idx <= Total; idx++) 
    {    
      if (MainMdt->GetEntry (idx,  &mdtrec))
	if (!mdtrec.GetDeleted()) 
	  return mdtrec.GetKey(); 
    }
  return NulString;
}

STRING IDB::LastKey() const
{
  // Last record
  MDTREC mdtrec;
  const size_t Total = MainMdt->GetTotalEntries (); 
  for (size_t idx = Total; idx > 0; idx--) 
    {    
      if (MainMdt->GetEntry (idx,  &mdtrec))
	if (!mdtrec.GetDeleted()) 
	  return mdtrec.GetKey(); 
    }
  return NulString;
}

STRING IDB::NextKey(const STRING& Key) const
{
  MDTREC mdtrec;
  const size_t Total = MainMdt->GetTotalEntries (); 
  for (size_t idx = MdtLookupKey (Key); idx < Total; idx++)
    {
      if (MainMdt->GetEntry (idx + 1,  &mdtrec))
	if (!mdtrec.GetDeleted())
	  return mdtrec.GetKey();
    }
  return NulString;
}

STRING IDB::PrevKey(const STRING& Key) const
{
  MDTREC mdtrec;
  for (size_t idx = MdtLookupKey (Key); idx > 1; idx--)
    {
      if (MainMdt->GetEntry (idx - 1,  &mdtrec))
	if (!mdtrec.GetDeleted())
	  return mdtrec.GetKey();
    }
  return NulString;
}


struct DTAB {
    DFD Dfd;
    FC Fc;
};

// Order in Original File...
static int DfdtCompare (const void *p1, const void *p2)
{
  off_t diff = ((struct DTAB *) p1)->Fc.GetFieldStart() -
	((struct DTAB *) p2)->Fc.GetFieldStart();
  if (diff == 0)
    diff = ((struct DTAB *)p2)->Fc.GetFieldEnd() -
	((struct DTAB *) p1)->Fc.GetFieldEnd();
  return (diff == 0) ? 0 : ( diff > 1 ? 1 : -1);
}

//
// On Disk BSearch into one of the Field Coordinate Tables
//
// First Element is 1... 0 means not found
static inline off_t OnDiskFcSearch(const FC& Fc, FILE *Fp)
{
  register INT         cmp;
  FC                   fc;
  off_t                low = 0, high = GetFileSize(Fp) / sizeof (FC);
  off_t                p = (high+low)/2, op;
  const char           message[] = "On Disk Field Search %s@%ld Error";

//cerr << "OnDiskFcSearch for " << Fc << endl;
// cerr << "low = " << low << endl;
// cerr << "high= " << high << endl;

  do {
    op = p;
    if ( -1 == fseek(Fp, p*sizeof(FC), SEEK_SET)) {
      message_log (LOG_ERRNO, message, "Seek", p);
      cmp = 1; // Seek Error
    } else if (fc.Read (Fp) == false) {
      message_log (LOG_ERRNO, message, "Read", p);
      cmp = 0;  // Read Error (pretend match)
    } else
      cmp = Fc.Compare(fc);

    if (cmp == 0) {
      // Found something...
// cerr << "Found something" << endl;
      return p+1;
    }
// cerr << "cmp=" << cmp << " fc = " << Fc << "-->" << fc << endl;

    if (cmp < 0) high = p;
    if (cmp > 0 && (low = p + 1) > high) low = high; 
  } while ( ( p = (high + low)/2) != op);

// cerr << "NOT FOUND" << endl;
  return 0; // NOT FOUND
}

static inline off_t OnDiskFcZoneSearch(const FC& HitFc, FILE *Fp)
{
  FC                   fc;
  off_t                low = 0, high = GetFileSize(Fp) / sizeof (FC);
  off_t                p = (high+low)/2, op;
  const char           message[] = "On Disk Field Zone Search %s@%ld Error";

// cerr << "Zone Search for " << HitFc << endl;
  do {
// cerr << "p = " << p << endl;
    op = p;
    if ( -1 == fseek(Fp, p*sizeof(FC), SEEK_SET)) {
      message_log (LOG_ERRNO, message, "Seek", p);
      return 0;
    } else if (fc.Read (Fp) == false) {
      message_log (LOG_ERRNO, message, "Read", p);
      return 0;
    } 
// cerr << "fc = " << fc << endl;
    if ( fc.Contains(HitFc) ) {
// cerr << "got something" << endl;
      return p+1;
    } else if (fc.GetFieldStart() >= HitFc.GetFieldStart() )
      high = p;
    else if ((low = p + 1) > high)
      low = high;
  } while ( ( p = (high + low)/2) != op);

// cerr << "Not Found" << endl;
  return 0; // NOT FOUND
}


bool IDB::GetRecordDfdt (const STRING& Key, DFDT *DfdtBuffer)
{
  DfdtBuffer->Clear();

  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  if (TotalEntries == 0)
    {
      return true; // Has nothing, have nothing
    }

  MDTREC Mdtrec;
  if (!MainMdt->GetMdtRecord (Key, &Mdtrec))
    {
      message_log (LOG_DEBUG, "GetRecordDfdt: Record %s not in MDT!", Key.c_str());
      return false;
    }

  const GPTYPE GpStart = Mdtrec.GetGlobalFileStart () +
	Mdtrec.GetLocalRecordStart ();

  const FC MdtFc(GpStart, Mdtrec.GetGlobalFileStart () +
	Mdtrec.GetLocalRecordEnd ());

  DFD dfd;
  STRING Fn;

  size_t count = 0;
  struct DTAB *Table = new struct DTAB[TotalEntries];

  message_log (LOG_DEBUG, "Creating DFDT table of elements");
  for (size_t x = 1; x <= TotalEntries; x++)
    {
      MainDfdt->GetEntry (x, &dfd);
      // Only want TEXT fields
//cerr << "Field Type = " << dfd.GetFieldType().c_str() << endl;
      if (!(dfd.GetFieldType().IsText()))
	{
	  continue;
	}
      DfdtGetFileName (dfd.GetFieldName(), &Fn); // TEXT
      PFILE Fp = fopen (Fn, "rb");
      if (Fp)
	{
	  off_t Pos = OnDiskFcSearch(MdtFc, Fp);
	  if (Pos)
	    {
	      FC Fc2;
	      // work backwards to find first pair
	      while (--Pos >= 0)
		{
		  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		    {
		      Fc2.Read (Fp);
		      if (Fc2.GetFieldStart() < GpStart)
			break;
		    }
		}
	      if (Pos > 0)
		{
		  Fc2.Read (Fp);
		}
	      Table[count].Dfd = dfd;
	      Table[count++].Fc = Fc2;
	    }
	  fclose(Fp);
	}
    }				// for()

  // qsort Table and put into DfdtBuffer so
  // that it represents the "order" in the record
  QSORT (Table, count, sizeof (Table[0]), DfdtCompare);
  DfdtBuffer->Resize(count+1);
  for (size_t i=0; i < count; i++)
    DfdtBuffer->FastAddEntry (Table[i].Dfd);

  delete[]Table;

  if (DebugMode) message_log (LOG_DEBUG, "Document contains %u fields", (unsigned)count);
  return true;
}

static inline off_t OnDiskFcSubZoneSearch(const FC& HitFc, FILE *Fp)
{
  FC                   fc;
  off_t                low = 0, high = GetFileSize(Fp) / sizeof (FC);
  off_t                p = (high+low)/2, op;
  const char           message[] = "On Disk Field Sub-Zone Search %s@%ld Error";

  do {
    op = p;
    if ( -1 == fseek(Fp, p*sizeof(FC), SEEK_SET)) {
      message_log (LOG_ERRNO, message, "Seek", p);
      return 0;
    } else if (fc.Read (Fp) == false) {
      message_log (LOG_ERRNO, message, "Read", p);
      return 0;
    } 
    if ( HitFc.Contains(fc) )
      {
        return p+1; // Found
      }
    else if (fc.GetFieldStart() >= HitFc.GetFieldStart() )
      high = p;
    else if ((low = p + 1) > high)
      low = high;
  } while ( ( p = (high + low)/2) != op);
  return 0; // NOT FOUND
}

// We may have multiple child nodes!
//
FCT IDB::GetDescendentsFCT (const FC& HitFc, const STRING& NodeName)
{
  STRING       Fn;
  FILE        *Fp;
  FCT          Fct;

  if (DfdtGetFileName (NodeName, &Fn) && (Fp = fopen (Fn, "rb")) != NULL)
    {
      Fct = GetDescendentsFCT (HitFc, Fp);
      fclose(Fp);
    }
  return Fct;
}


FCT IDB::GetDescendentsFCT (const FC& HitFc, FILE *Fp)
{
  FCT          Children;

  if (Fp && !HitFc.IsEmpty())
    {
      FC           Fc;
      off_t        Pos = OnDiskFcSubZoneSearch(HitFc, Fp);

// cerr << "GetDescendentsFCT SubZoneSearch retuned " << Pos << " for " << HitFc << endl;
      if (Pos) {
        // work backwards to find first FC 
        while (--Pos >= 0) {
          if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
            {
              Fc.Read (Fp);
              if (!HitFc.Contains(Fc)) break;
            }
        } // While()
      }
      if (Pos >= 0) Fc.Read (Fp);

      // Should have the first
      if (!Fc.IsEmpty() && HitFc.Contains(Fc))
	Children.AddEntry(Fc);

      // Read more
      while (Fc.Read(Fp))
        {
          if (!HitFc.Contains(Fc))
            break; // No more
	  Children.AddEntry(Fc);
        }
    }

// cerr << "DEBUG: ***** Children of " << HitFc << "  = " << Children << "----" << endl;
  return Children;
}

// PageNumber caculation:
// GetNodeOffsetCount(HitGp, "PAGE", &Fc)
//

STRING IDB::GetXMLHighlightRecordFormat(const RESULT& Result, const STRING& PageField, const STRING& TagElement)
{
  STRING XML;
  int    pageno = -1;
  GPTYPE gp;
  FC     PageFc;
  STRING Page (PageField);
  off_t  Start;
  const STRING Pg (TagElement.IsEmpty() ? "Pg" : TagElement);

  if (Page.IsEmpty())
    {
      if (DocTypeReg)
	{
	  DOCTYPE *DoctypePtr = DocTypeReg->GetDocTypePtr ( Result.GetDocumentType() );
	  if (DoctypePtr != NULL)
	    Page = DoctypePtr->UnifiedName("Page");
	}
    }
  if (Page.IsEmpty()) Page = "PAGE"; // default

  if (MainDfdt == NULL || (MainDfdt->GetFileNumber (Page)) == 0)
   {
     XML << "<!--- WARNING: No Field \""<< Page << "\" for " << Pg << " element // Use Alt format -->\n";
     XML << XMLHitTable(Result) << "\n";
     return XML;
   }

  const FCT HitTable = Result.GetHitTable();
  const FCLIST *ptr = (const FCLIST*)HitTable;

  XML << "<XML>\n<Body Units=Characters color=#FF00FF Mode=Active version=2>\n <Highlight>\n"; 
  size_t   count = 1;
  for (const FCLIST *p = ptr->Next(); p != ptr ; p = p->Next())
    {
      gp =  p->Value().GetFieldStart();
      if (!PageFc.Contains(gp)) pageno = -1;
      if (pageno < 0) pageno = GetNodeOffsetCount (gp, Page, &PageFc);
      if (pageno >= 0)
	{
	  FC x (p->Value());
	  Start = (off_t)( x.GetFieldStart() - PageFc.GetFieldStart() );
	  XML << "\t<Loc " << Pg << "=" << pageno-1
		<< " pos=" << Start << " len=" << x.GetLength()  << " />\n";
	}
      else
	{
	   XML << "<!-- Unknown coordinate " << gp << " [hit:" << count << "] -->\n";
	}
      count++;
    }
  XML << "</Highlight></Body></XML>\n";
  return XML;
}


// Which node count is the hit in...
int IDB::GetNodeOffsetCount (const GPTYPE HitGp, const STRING& NodeName, FC *ContentFC,
	FC *ParentFC, off_t *FirstInstancePos)
{
  FILE  *Fp;
  STRING Fn;
  STRING nodeName (NodeName);
  FC     contentFC;
  FC     parentFC;
  int    offset = -1;

  // Get the record ID and the range
  const size_t record_idx =  MainMdt->LookupByGp (HitGp, &parentFC);

  if (record_idx == 0) return -1;

  if (nodeName.IsEmpty())
    GetPeerFc (HitGp, &nodeName);


//cerr << "@@@@ Lookup " << NodeName << endl;
  if (DfdtGetFileName (NodeName, &Fn) && (Fp = ffopen(Fn, "rb")) != NULL)
    {
//cerr << "Check " << endl;
      if (GetFieldCache()->ValidateInField (HitGp, Fp))
	{
//cerr << "YES" << endl;
	  off_t Pos = ftell(Fp)/sizeof(FC)+1;
	  FC Fc2;
//cerr << "Pos init = " << Pos <<endl;
	  // work backwards to find first pair
	  while (--Pos >= 0)
	   {
	      if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		{
		  if (Fc2.Read (Fp) && Fc2.Contains( HitGp ))
		    {
		      contentFC = Fc2;
		      break;
		    }
		  if (Fc2.GetFieldStart() < HitGp)
		    break;
		}
	    }
	  if (!contentFC.Contains(HitGp))
	    {
	      // Look forward...
	      if (contentFC.GetLength() == 0) contentFC = Fc2;
	      while (Fc2.Read(Fp))
		{
		  if (Fc2.GetFieldEnd() > HitGp)
		    break;
		  if (Fc2.GetFieldStart() >=  contentFC.GetFieldStart() )
		    contentFC = Fc2;
		  Pos++;
		}
	    }
	  // contentFC is now the coordinates of the page
	  // Now workbackwards to find which page
	  if (!contentFC.Contains(HitGp))
	    {
	      ffclose(Fp);
	      return -1; // NOT FOUND
	    }
	  const off_t  MyPosition = Pos;

	  while (--Pos >= 0)
	    {
              if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
                {
                  if (Fc2.Read (Fp))
		    if (!parentFC.Contains(Fc2)) break;
		}
	    }
	  ffclose(Fp);
	  if (FirstInstancePos) *FirstInstancePos = Pos;
	  offset = (int) ( Pos >=0 ? (MyPosition - Pos ) : MyPosition ) + 1;
	}
    }
  //else cerr << "NO PAGE FIELD: " << Fn << endl;
  if (ContentFC) *ContentFC = contentFC;
  if (ParentFC)  *ParentFC  = parentFC;

  return offset;
}


size_t IDB::GetDescendentsContent (const FC HitFc, const STRING& NodeName, STRLIST *StrlistPtr)
{
  FCT           Hits = GetDescendentsFCT (HitFc, NodeName);
  const FCLIST *list = Hits.GetPtrFCLIST();
  size_t        count = 0;

  for (const FCLIST *ptr = list->Next(); ptr != list; ptr=ptr->Next())
    {
      STRING Buffer ( GetPeerContent(ptr->Value()) );
      if (Buffer.GetLength())
	{
	  count++;
	  if (StrlistPtr) StrlistPtr->AddEntry(Buffer);
	}
    }
  return count;
}

size_t IDB::GetDescendentsContent (const FC HitFc, FILE *Fp, STRLIST *StrlistPtr)
{
  // cerr << "Looking for child content for " << HitFc << endl;
  FCT           Hits = GetDescendentsFCT (HitFc, Fp);
  const FCLIST *list = Hits.GetPtrFCLIST();
  size_t        count = 0;

  // if (list == NULL || list->Next() == list) cerr << "Oops empty" << endl;

  for (const FCLIST *ptr = list->Next(); ptr != list; ptr=ptr->Next())
    {
      STRING Buffer ( GetPeerContent(ptr->Value()) );
      if (Buffer.GetLength())
        {
          count++;
          if (StrlistPtr) StrlistPtr->AddEntry(Buffer);
        }
       else cerr << "Empty peer content" << endl;
    }
  return count;
}

bool IDB::ValidNodeName(const STRING& nodeName) const
{
  STRING       NodeName (nodeName);
  STRINGINDEX  i = NodeName.SearchReverse( __AncestorDescendantSeperator );

  if (i > 2)
    {
      STRING ChildNodeName ( (const char *)NodeName + i);
      NodeName.EraseAfter(i-1);
      if ((MainDfdt->GetFileNumber (ChildNodeName)) == 0)
        {
	  // Search for fieldname sep
          char ch = '\\';
          if (ChildNodeName.Search(ch) == 0)
            {
              ch = 0;
              for (const char *tp = ChildNodeName.c_str(); tp && *tp; tp++)
                // Field names can be Letter | Digit | '.' | '-' | '_' | ':' |
                if (!isalnum(*tp) && *tp != '.' && *tp != '-' && *tp != '_' && *tp != ':')
                  {
                    ch = *tp;
                    break;
                  }
            }
	  if (ch != 0)
	     NodeName << ch << ChildNodeName;
	  else
	     return false;
       }
    }
  return (MainDfdt->GetFileNumber (NodeName) != 0);
}


size_t IDB::GetAncestorContent (RESULT& Result, const STRING& nodeName, STRLIST *StrlistPtr)
{
  size_t    count = 0;

//cerr << "IDB::GetAncestorContent" << endl;

  if (StrlistPtr) StrlistPtr->Clear();

  int offset = 0;
      
  if (MainMdt)
    { 
      MDTREC mdtrec;
      if (MainMdt->GetMdtRecord ( Result.GetKey (), &mdtrec))
	offset = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();
    }

  FILE        *Fp = NULL; // Stream to the ChildNode index 
  STRING       NodeName (nodeName);
  // cerr << "NodeName = " << NodeName << endl;


  STRINGINDEX  i = NodeName.SearchReverse( __AncestorDescendantSeperator );
  if (i > 2)
    {
      STRING ChildNodeName ( (const char *)NodeName + i), Fn;
      // cerr << "ChildNode = " << ChildNodeName << endl;

      // Need to lookup if the childnode exists.
      if (! FieldExists (ChildNodeName)) {
	return 0; 
      }

      NodeName.EraseAfter(i-1);
      // cerr << "NodeName = " << NodeName << endl;

// cerr << "Looking up " << ChildNodeName << endl;
      if (DfdtGetFileName (ChildNodeName, &Fn) == 0 || (Fp = fopen (Fn, "rb")) == NULL)
	{
	  char ch = '\\';
	  if (ChildNodeName.Search(ch) == 0)
	    {
	      ch = 0;
	      for (const char *tp = ChildNodeName.c_str(); tp && *tp; tp++)
		// Field names can be Letter | Digit | '.' | '-' | '_' | ':' |
		if (!isalnum(*tp) && *tp != '.' && *tp != '-' && *tp != '_' && *tp != ':')
		  {
		    ch = *tp;
		    break;
		  }
	    }
	  STRING newNode;
	  newNode << NodeName << ch << ChildNodeName;
// cerr << "Look at Node '" << newNode << "'" << endl;
	  if (DfdtGetFileName (newNode, &Fn) != 0 && (Fp = fopen (Fn, "rb")) != NULL)
	    ChildNodeName = newNode;
	  if (Fp == NULL)
	    return 0; // No such Subnodes
	}
// cerr << "Descedent Field = " << ChildNodeName << endl;
    }
// cerr << "Ancestor Field  = " << NodeName << endl;


// cerr << "Offset = " << offset << endl;

  FC             lastFc, Fc;
  FCT            HitTable = Result.GetHitTable();
  const FCLIST  *listPtr = HitTable.GetPtrFCLIST();
  for (const FCLIST *ptr = listPtr->Next(); ptr != listPtr; ptr=ptr->Next())
    {
      if ((Fc = GetAncestorFc(FC(ptr->Value()) += offset, NodeName)) != lastFc && !Fc.IsEmpty())
	{
	  if (Fp)
	    {
// cerr << "Looking for descendents content" << endl;
	      // Want some child of this parent
	      count += GetDescendentsContent (Fc, Fp, StrlistPtr);
// cerr << "Got " << count << endl;
	    }
	  else
	    {
	      // Want the content
	      STRING Buffer ( GetPeerContent(Fc) );
	      if (Buffer.GetLength())
		{
		  count++;
// cerr << "Add ======" << endl << Buffer << endl << "======" << endl;
		  if (StrlistPtr) StrlistPtr->AddEntry(Buffer); 
		}
	    }
	  lastFc = Fc;
	}
    }
  if (Fp) fclose(Fp); // Close the handle
  return count;
}


FC IDB::GetAncestorFc (const FC& HitFc, const STRING& NodeName)
{
  FC           ParentFc, Fc;
  STRING       Fn;
  FILE        *Fp;

//cerr << "Lookup " << HitFc << endl;

  if (DfdtGetFileName (NodeName, &Fn) && (Fp = fopen (Fn, "rb")) != NULL)
    {
      off_t        Pos = OnDiskFcZoneSearch(HitFc, Fp);

//cerr << "Disk Zone Search = " << Pos << endl;
      if (Pos) {
	// work backwards to find first pair
	while (--Pos >= 0) {
	  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
	    {
	      ParentFc.Read (Fp);
//cerr << "[" << Pos +1 << "] Read: " << ParentFc << endl;
	      if (!ParentFc.Contains(HitFc)) break;
	    }
	} // While()
      }
//cerr << "ftell = " << ftell(Fp)/sizeof(FC) << endl;
      if (Pos >= 0) ParentFc.Read (Fp);
      while (Fc.Read(Fp))
	{
	  if (!Fc.Contains(HitFc))
	    break;
	  ParentFc = Fc;
	}

      fclose(Fp);
    }
//cerr << "[2] ParentFc = " << ParentFc << endl;
  if (!ParentFc.Contains(HitFc) && (MainMdt == NULL ||
	MainMdt->LookupByGp(HitFc.GetFieldStart()) != MainMdt->LookupByGp(ParentFc.GetFieldStart())))
    ParentFc = (GPTYPE)0;

//  if (ParentFc.GetLength())
//    cerr << "Parent of "<< HitFc << " in " << NodeName << " is " << ParentFc << "  length=" << ParentFc.GetLength() << endl;
//  else
//   cerr << "Is outside.. Not an ancestor!" << endl;

#if 1
  if (ParentFc.GetFieldStart() > HitFc.GetFieldStart() ||
	ParentFc.GetFieldEnd() < HitFc.GetFieldEnd() )
    ParentFc = (GPTYPE)0;
#endif

  return ParentFc;
}

#if 0
// Not very efficient...
// Instead..
// A function that returns a FCLIST
// with all the FCs in the same document as the HitGp
//

FC IDB::GetNextFC (const GPTYPE& HitGp, const STRING& fieldname, size_t offset)
{
  STRING Fn;
  off_t  Pos = 0;
  FC     Fc;

  DfdtGetFileName (fieldname, &Fn);
  off_t   
  PFILE Fp = fopen (Fn, "rb");
  if (Fp) {
    if (GetFieldCache()->ValidateInField (HitGp, Fp)) {
      FC Fc2;

      Pos = ftell(Fp);
      // work backwards to find first pair
      while (--Pos >= 0) {
	if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET)) {
	  Fc2.Read (Fp);
	  if (Fc2.GetFieldStart() < HitGp) break;
	}
      }
      if (Pos >= 0) Fc2.Read (Fp);
      else Pos = 0;
      Fc = Fc2;
      while (Fc2.Read(Fp)) {
	if (Fc2.GetFieldEnd() > HitGp) break;
	Pos++;
	if (Fc2.GetFieldStart() >=  Fc.GetFieldStart() )
	  Fc = Fc2;
       }
      // Now we have found it
      if (Fc.GetLength() && offset != 0) {
        if (-1 != fseek(Fp, Pos + offset) * sizeof(FC), SEEK_SET))
	  Fc.Read(Fp);
      }
    }
    fclose(Fp);
  }
  return Fc;
}

#endif


// This code does the heavy lifting to figure out what is the name
// or path (such as XQuery) of where the HitGp is located..
//

FC IDB::GetPeerFc (const GPTYPE& HitGp, STRING *NodeNamePtr)
{
  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  FC           PeerFC;
  STRING       PeerFieldName;
  size_t       looks = 0;
  size_t       PeerCount = 0;
  size_t       fieldNameCount = 0;
  bool  haveCount = false;

  // We cache lastPeerField to speed up a bit
  if (lastPeerField <1 || lastPeerField > TotalEntries)
    if ((lastPeerField = TotalEntries/3) < 1) lastPeerField = 1;

//   lastPeerField = 1; // Just for debugging!!!! Remove this later...

  const size_t start = lastPeerField-1;
  const size_t end   = TotalEntries+lastPeerField-1;

  for (size_t i = start; i < end; i++)
    {
      DFD    dfd;
      size_t x;

      x =  (i % TotalEntries)+1;
 

      MainDfdt->GetEntry (x, &dfd);

      STRING Fn, fieldname ( dfd.GetFieldName () );
      DfdtGetFileName (fieldname, &Fn);

      if ((fieldNameCount = fieldname.Count()) > 0 && PeerCount == 0)
        haveCount = true;
      else if (haveCount && fieldNameCount == 0)
        continue;
      else if (PeerCount && !fieldname.Contains(PeerFieldName))
        continue;

      PFILE Fp = fopen (Fn, "rb");
      if (Fp)
	{
	  looks++;
	  if (GetFieldCache()->ValidateInField (HitGp, Fp))
	    {
	      off_t Pos = ftell(Fp);
	      FC Fc2;
	      // work backwards to find first pair
	      while (--Pos >= 0)
		{
		  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		    {
		      Fc2.Read (Fp);
		      if (Fc2.GetFieldStart() < HitGp)
			break;
		    }
		}
	      if (Pos >= 0)
		Fc2.Read (Fp);
	      PeerFC = Fc2;
	      while (Fc2.Read(Fp))
		{
		  if (Fc2.GetFieldEnd() > HitGp)
		    break;
		  if (Fc2.GetFieldStart() >=  PeerFC.GetFieldStart() )
		    {
		      PeerFC = Fc2;
		      PeerCount = (PeerFieldName = fieldname).Count();
		      lastPeerField = x;
		    }
		}
	    }
	  fclose(Fp);
	}
    }				// for()
  if (NodeNamePtr)
    *NodeNamePtr = PeerFieldName;
  return PeerFC;
}


// This code does the heavy lifting to figure out what is the name
// or path (such as XQuery) of where the range of bytes defined by HitFc is located..
//

FC IDB::GetPeerFc (const FC& HitFc, STRING *NodeNamePtr)
{
  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  FC           PeerFC = HitFc;
  bool  firstTime = true;
  STRING       PeerFieldName;
  size_t       looks = 0;
  size_t       PeerCount = 0;
  size_t       fieldNameCount = 0;
  bool  haveCount = false;

  if (lastPeerField <1 || lastPeerField > TotalEntries)
    if ((lastPeerField = TotalEntries/3) < 1) lastPeerField = 1;

  const size_t start = lastPeerField-1;
  const size_t end   = TotalEntries+lastPeerField-1;

  for (size_t i = start; i < end; i++)
    {
      DFD    dfd;
      size_t x;

      x =  (i % TotalEntries)+1;
      MainDfdt->GetEntry ( x, &dfd);

/*
      if (!dfd.GetFieldType().IsText())
	continue;
*/

      STRING Fn, fieldname ( dfd.GetFieldName () );
      DfdtGetFileName (fieldname, &Fn);

      if ((fieldNameCount = fieldname.Count()) > 0 && PeerCount == 0)
	haveCount = true;
      else if (haveCount && fieldNameCount == 0)
	continue;
      else if (PeerCount && !fieldname.Contains(PeerFieldName))
	continue;

      PFILE Fp = fopen (Fn, "rb");
      if (Fp)
	{
	  looks++;
	  off_t Pos = OnDiskFcZoneSearch(HitFc, Fp);
	  if (Pos)
	    {
	      FC Fc2;
	      // work backwards to find first pair
	      while (--Pos >= 0)
		{
		  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		    {
		      Fc2.Read (Fp);
		      if (!Fc2.Contains(HitFc))
			{
			  break;
			}
		    }
		}
	      if (Pos >= 0)
		{
		  Fc2.Read (Fp);
		}

	      if (firstTime || (PeerFC.Contains(Fc2) && Fc2.Contains(HitFc)))
		{
		  firstTime = false;
		  if ((PeerFC != Fc2) || (fieldname.GetLength() > PeerFieldName.GetLength()) )
		     {
		       lastPeerField = x;
		       PeerCount = (PeerFieldName = fieldname).Count();
		     }
		  PeerFC = Fc2;
		}

	      while (Fc2.Read(Fp))
		{
		  if (!Fc2.Contains(HitFc))
		    break;
		  if (PeerFC.Contains(Fc2))
		    {
		      PeerFC = Fc2;
		      PeerCount = (PeerFieldName = fieldname).Count();
		      lastPeerField = x;
		    }
		}
	    }
	  fclose(Fp);
	}
    }				// for()
//cerr << "(2)Looks = " << looks << "/" << TotalEntries << "  " << PeerFieldName << " x=" << lastPeerField << endl;
  if (NodeNamePtr)
    *NodeNamePtr = PeerFieldName;
  return PeerFC;
}


#if 0
int IDB::GetNodeList (const FC& HitFc, TREENODELIST *NodeList)
{
  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  FC           PeerFC = HitFc;
  bool  found = false;
  STRING       PeerFieldName;
  int          count = 0;

  if (NodeList == NULL) return -1;

  NodeList->Clear();

/*
  GPTYPE       offset;
  {
   MDTREC      mdtrec;
   int         mdt_index = MainMdt->GetMdtRecord (HitFc.GetFieldStart(), &mdtrec);
   offset    = mdtrec.GetGlobalFileStart();
  }
*/

  for (size_t x = 1; x <= TotalEntries; x++)
    {
      DFD    dfd;
      MainDfdt->GetEntry (x, &dfd);

/*
      if (!dfd.GetFieldType().IsText())
	continue;
*/

      STRING Fn, fieldname ( dfd.GetFieldName () );
      DfdtGetFileName (fieldname, &Fn);
      PFILE Fp = fopen (Fn, "rb");
      if (Fp)
	{
	  off_t Pos = OnDiskFcZoneSearch(HitFc, Fp);
	  if (Pos)
	    {
	      FC Fc2;
	      // work backwards to find first pair
	      while (--Pos >= 0)
		{
		  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		    {
		      Fc2.Read (Fp);
		      if (!Fc2.Contains(HitFc))
			break;
		    }
		}
	      if (Pos > 0)
		Fc2.Read (Fp);
	      if (!found || (PeerFC.Contains(Fc2) && Fc2.Contains(HitFc)))
		{
		  found  = true;
		  PeerFC = Fc2;
		  PeerFieldName = fieldname;
		}
	      while (Fc2.Read(Fp))
		{
		  if (!Fc2.Contains(HitFc))
		    break;
		  if (PeerFC.Contains(Fc2))
		    {
		      PeerFC = Fc2;
		      PeerFieldName = fieldname;
		    }
		}
	    }
	  fclose(Fp);
	}
      if (found)
	{
	  NodeList->AddEntry( TREENODE(PeerFC, PeerFieldName) );
	  found = false;
	  count++;
	}
    }				// for()
  if (count > 1)
   NodeList->Sort();
  return count;
}
#endif


NODETREE IDB::GetNodeTree (const FC& HitFc)
{
  const size_t TotalEntries = MainDfdt->GetTotalEntries ();
  FC           PeerFC = HitFc;
  bool  found = false;
  STRING       PeerFieldName;
  int          count = 0;
  NODETREE     Tree;
  const STRING Dot ("."); // Special field

  message_log (LOG_DEBUG, "Looking for Nodetree (%ld,%ld)", 
	HitFc.GetFieldStart(), HitFc.GetFieldEnd());

  for (size_t x = 1; x <= TotalEntries; x++)
    {
      DFD    dfd;
      MainDfdt->GetEntry (x, &dfd);

/*
      if (!dfd.GetFieldType().IsText())
	continue;
*/
      const STRING fieldname ( dfd.GetFieldName () );

#if 0
      // Feb 2008 : Ignore . example <name type="list">foo</name> 
      // "." contains both "list" and "foo", name contains only "foo" and name@type contains "list"
      if (fieldname == Dot) continue; 
#endif

      STRING Fn;
      DfdtGetFileName (fieldname, &Fn);
      PFILE Fp = fopen (Fn, "rb");
      if (Fp)
	{
	  off_t Pos = OnDiskFcZoneSearch(HitFc, Fp);
	  if (Pos)
	    {
	      FC Fc2;
	      // work backwards to find first pair
	      while (--Pos >= 0)
		{
		  if (-1 != fseek (Fp, Pos * sizeof (FC), SEEK_SET))
		    {
		      Fc2.Read (Fp);
		      if (!Fc2.Contains(HitFc))
			break;
		    }
		}
	      if (Pos > 0)
		Fc2.Read (Fp);
	      if (!found || (PeerFC.Contains(Fc2) && Fc2.Contains(HitFc)))
		{
		  found  = true;
		  PeerFC = Fc2;
		  PeerFieldName = fieldname;
		}
	      while (Fc2.Read(Fp))
		{
		  if (!Fc2.Contains(HitFc))
		    break;
		  if (PeerFC.Contains(Fc2))
		    {
		      PeerFC = Fc2;
		      PeerFieldName = fieldname;
		    }
		}
	    }
	  fclose(Fp);
	}
      if (found)
	{
	  message_log (LOG_DEBUG, "Add Node %s", PeerFieldName.c_str());
	  Tree.AddEntry( TREENODE(PeerFC, PeerFieldName) );
	  found = false;
	  count++;
	}
    }				// for()

  if (count > 1)
   Tree.Sort();
  return Tree;
}

STRING IDB::ComposeDbFn (const CHR* Suffix) const
{
  STRING Fn (DbFileStem);
  if (Suffix && Suffix[0])
    {
      Fn.Cat (Suffix);
    }
  return Fn;
}

STRING IDB::ComposeDbFn (INT FileNumber) const
{
  static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz"; // Portable
  static const int  modulo   = sizeof(digits)/sizeof(char) - 1;

  // We limit ourselves to 12959 fields
  if (FileNumber > ((modulo+1)*(modulo+1)*10-1) || FileNumber < 0)
    FileNumber = 0;
  char s[5];
  s[0] = '.';
  s[1] = digits[(FileNumber/ (modulo*modulo)) % 10];
  s[2] = digits[(FileNumber/ modulo) % modulo];
  s[3] = digits[FileNumber % modulo];
  s[4] = '\0';
  return ComposeDbFn ((const CHR *)s);
}

void  IDB::SetDefaultPriorityFactor(DOUBLE x)
{
  if (x != PriorityFactor)
    {
      MainRegistry->ProfileWriteString(RankingSection, PriorityFactorEntry, x);
      DbInfoChanged = true;
    }
}

void  IDB::SetDefaultDbSearchCutoff(size_t x)
{
  if (x != GetDbSearchCutoff())
    {
      MainRegistry->ProfileWriteString(DbInfoSection, SearchCutoffEntry, x);
      DbInfoChanged = true;
    }
}



void IDB::SetMirrorBaseDirectory(const STRING& Mirror)
{
  if (!Mirror.IsEmpty())
    {
      if (Mirror != HTDocumentRoot)
	{
	  STRING val;
	  MainRegistry->ProfileWriteString("Mirror", "Root", Mirror);
	  MainRegistry->ProfileGetString("HTTP", "Pages", NulString, &val);
	  if (val.GetLength())
	    MainRegistry->ProfileWriteString("HTTP", "Pages", NulString);
	  HTDocumentRoot = Mirror;
	  DbInfoChanged = true;
	}
      if (!isMirror)
	isMirror = true;
    }
  else if (isMirror)
    isMirror = false;
}

void IDB::SetHTTPServer(const STRING& Server)
{
  if (Server != HTpath)
    {
      if (!(HTpath = Server).IsEmpty())
	AddTrailingSlash(&HTpath);
      MainRegistry->ProfileWriteString("HTTP", "Server", HTpath);
      DbInfoChanged = true;
    }
}

void IDB::SetHTTPPages(const STRING& Pages)
{
  if (Pages != HTpath)
    {
      if (!(HTDocumentRoot = Pages).IsEmpty())
        AddTrailingSlash(&HTDocumentRoot);
      MainRegistry->ProfileWriteString("HTTP", "Pages", HTDocumentRoot);
      MainRegistry->ProfileWriteString("Mirror", "Root", NulString);
      DbInfoChanged = true;
    }
}

void IDB::SetSegment(const STRING& newName, int newNumber)
{
  if (SegmentName != newName)
    {
      SegmentName = newName;
      MainRegistry->ProfileWriteString(DbInfoSection, SegmentNameEntry, newName);
      DbInfoChanged = true;
    }
  if (newNumber >= 0)
    SegmentNumber = newNumber;
}



void IDB::SetTitle(const STRING& NewTitle)
{
  if (Title != NewTitle)
    {
      Title = NewTitle;
      MainRegistry->ProfileWriteString(DbInfoSection, TitleEntry, NewTitle);
      DbInfoChanged = true;
    }
}

void IDB::SetComments(const STRING& NewComments)
{
  if (Comments != NewComments)
    {
      Comments = NewComments;
      MainRegistry->ProfileWriteString(DbInfoSection, CommentsEntry, NewComments);
      DbInfoChanged = true;
    }
}

void IDB::SetCopyright(const STRING& NewCopyright)
{
  if (Copyright != NewCopyright)
    {
      Copyright = NewCopyright;
      MainRegistry->ProfileWriteString(DbInfoSection, CopyrightEntry, NewCopyright);
      DbInfoChanged = true;
    }
}

void IDB::SetMaintainer(const STRING& NewName, const STRING& NewAddress)
{
  if (MaintainerName != NewName)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, MaintainerNameEntry,  MaintainerName = NewName);
      DbInfoChanged = true;
    }
  if (MaintainerMail != NewAddress)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, MaintainerMailEntry, MaintainerMail = NewAddress);
      DbInfoChanged = true;
    }
}

void IDB::GetMaintainer(PSTRING Name, PSTRING Address) const
{
  *Name = MaintainerName;
  *Address = MaintainerMail;
}

STRING IDB::GetMaintainer() const
{
  STRING Mailto, Name, Address;
  GetMaintainer(&Name, &Address);
  if (Name.GetLength() == 0)
    Name = Address;
  else if (Address.GetLength() == 0)
    Address = Name;
  if (Name.GetLength() == 0)
    MainRegistry->ProfileGetString(DbInfoSection, "Maintainer", NulString, &Mailto);
  else
    Mailto << "<A HREF=\"mailto:" << Address << "?subject="
	<< DbFileName << "\">" << Name << "</A>";
  return Mailto;
}


PDOCTYPE IDB::GetDocTypePtr (const DOCTYPE_ID& DocType) const
{
  static DOCTYPE_ID Autodetect ("AUTODETECT");

//cerr << "Get Pointer: " << DocType << endl;
  PDOCTYPE DoctypePtr = DocTypeReg->GetDocTypePtr (DocType);
  if (DoctypePtr == NULL)
    {
      if ((DocType == GlobalDoctype) || (GlobalDoctype == Autodetect))
	{
	  if (DocType.IsDefined())
	    message_log(LOG_ERROR|LOG_INFO, "Doctype \"%s\" is not registered.", DocType.c_str());
	  else
	    message_log(LOG_ERROR|LOG_INFO, "No default Doctype is available.");
	}
      else 
	{
	  if (GlobalDoctype.IsDefined())
	    message_log(LOG_NOTICE, "\"%s\" is not registered, using the global default document type %s.",
		DocType.c_str(), GlobalDoctype.c_str());
	   else if (DocType.IsDefined())
	    message_log(LOG_NOTICE, "\"%s\" is not registered, using the default document type.", DocType.c_str());
	   else
	    message_log (LOG_NOTICE, "No doctype specified, using the default.");
	  return GetDocTypePtr(GlobalDoctype);
	}
    }
//cerr << "Returning" << endl;
  return DoctypePtr;
}

bool IDB::ValidateDocType(const STRING& DocType) const
{
  return DocTypeReg->ValidateDocType(DocType);
}
bool IDB::ValidateDocType(const DOCTYPE_ID& Id) const
{
  return DocTypeReg->ValidateDocType(Id);
}
  

void IDB::IndexingStatus (const t_IndexingStatus StatusMessage, const STRING& Filename, const long arg)
{
    switch (StatusMessage)
      {
        case IndexingStatusReading:
          message_log (LOG_INFO, "Reading files...");
          break; 
        case IndexingStatusParsingFiles:
          message_log (LOG_INFO, "Parsing files ...");
          break;
	case IndexingStatusParsingRecords:
	  message_log (LOG_INFO, "Parsing Records ...");
	  break;   
	case IndexingStatusParsingRecord:
	  message_log (LOG_DEBUG, "Parsing %s ..", Filename.c_str());
	  break;
        case IndexingStatusIndexingDocument:
          message_log (LOG_INFO, "Indexing %s ...", Filename.c_str()); 
          break;
        case IndexingStatusIndexing:
          message_log (LOG_INFO, "Adding %u words to index ...", (unsigned)arg);
          break;
	case IndexingStatusRecordAdded:
	  message_log (LOG_DEBUG, "Index Record Nr.%ld created ('%s')", arg, Filename.c_str());
	  break;
        case IndexingStatusFlushing:
          message_log (LOG_INFO, "Flushing index ...");
          break;
        case IndexingStatusMerging:
          message_log (LOG_INFO, "Merging indexes (%u indices)...", (unsigned)arg);
          break;
	case IndexingStatusInit:
	  message_log (LOG_DEBUG, "Opening '%s'", Filename.c_str());
	  break;
	case IndexingStatusClose:
	  message_log (LOG_DEBUG, "'%s' closed", Filename.c_str());
	  break;
#if 0
	default:
	  message_log (LOG_INFO, "Status#%d: %s : %d", (int)StatusMessage , Filename.c_str(), (int)arg);
	  break;
#endif
      }
}


int IDB::BitVersion() const
{
  return compatible ? 8*sizeof(GPTYPE) :  (sizeof(GPTYPE) == 4 ? 64 : 32);
}


bool IDB::IsDbCompatible() const
{
  // cerr << "compatible = " << (int)compatible << endl;
  // cerr << "MainIndex address = " << (long)MainIndex << endl;
  // cerr << "MainMdt address = " << (long)MainMdt << endl;
  // cerr << "MDT OK = " << (int)(MainMdt != NULL && MainMdt->Ok()) << endl;
  // cerr << "INDEX OK = " << (int)(MainIndex != NULL && MainIndex->Ok()) << endl;
  const bool ok = compatible && MainIndex && MainIndex->Ok() && MainMdt && MainMdt->Ok();
  // cerr << "IsDbCompatible = " << ok << endl;
  return ok;
}


bool IDB::IsEmpty() const
{
  if (MainIndex && MainIndex->IsEmpty()) {
    return true;
  }
  if (MainMdt && MainMdt->IsEmpty()) {
    message_log (LOG_ERROR, "MDT is empty or corrupt!");
    return true;
  }
  return false;
}

off_t IDB::GetTotalWords() const
{
  return MainIndex ? MainIndex->GetTotalWords() : 0;
}

off_t IDB::GetTotalUniqueWords() const
{
  return MainIndex ? MainIndex->GetTotalUniqueWords() : 0;
}

size_t IDB::GetTotalRecords () const
{
  return MainMdt ? MainMdt->GetTotalEntries () : 0;
}


size_t IDB::GetTotalDocumentsDeleted() const
{
  return MainMdt->GetTotalDeleted();
}

// Set Memory in bytes
void IDB::SetIndexingMemory (const size_t MemorySize, bool Force)
{
  const rlim_t totalMemory        = _IB_GetTotalMemory();
  const rlim_t freeMemory         = _IB_GetFreeMemory();
  message_log (LOG_DEBUG, "OS Reports Memory: Total = %lu, Free = %lu (%u bytes/Page)",
	(unsigned long)(totalMemory/PAGESIZE), (unsigned long)(freeMemory/PAGESIZE), (unsigned)PAGESIZE);
  if (Force && MemorySize > 0)
    {
      if  (MemorySize < 4096)                IndexingMemory = (size_t) (MemorySize *(1L << 20)); // MB
      else if (MemorySize < (1 << 20))       IndexingMemory = (size_t) (MemorySize *1024L); // KB
      else if (MemorySize < MinimumMemSize)  IndexingMemory = MinimumMemSize; // Min.
      else                                   IndexingMemory = MemorySize; // Bytes
      IndexingMemory += (IndexingMemory & PAGEOFFSET);

      if (IndexingMemory < MemorySize)
	message_log (LOG_ERROR, "Memory size overflow (%lu < %lu)", (unsigned long)IndexingMemory, (unsigned long)MemorySize);
      if ((totalMemory > 0) && ((GPTYPE)IndexingMemory >= (GPTYPE)totalMemory))
	message_log (LOG_WARN, "More indexing memory, %lu pages, was specified than is available system-wide (%lu)",
                (unsigned long)(IndexingMemory/PAGESIZE), (unsigned long)(totalMemory/PAGESIZE));
      else if ((totalMemory > 0) && ((GPTYPE)IndexingMemory > (GPTYPE)(totalMemory/2)))
	message_log (LOG_WARN, "Dangerously high amount of indexing memory, %lu pages, was specified (%.1f%%)", 
	 	(unsigned long)(IndexingMemory/PAGESIZE), 100.0 * ((double)IndexingMemory/(double)totalMemory));
      else if ((freeMemory > 0) && (GPTYPE)((3*IndexingMemory) > (GPTYPE)freeMemory))
	message_log (LOG_WARN, "Indexing process with %lu pages may swap (%lu/%lu)",
		(unsigned long)(IndexingMemory/PAGESIZE), (unsigned long)(freeMemory/PAGESIZE), (unsigned long)(totalMemory/PAGESIZE));
      else
	message_log (LOG_DEBUG, "Indexing memory set to %lu kb (%lu pages).",
		(unsigned long)(IndexingMemory/1024), (unsigned long)(IndexingMemory/PAGESIZE));
      return;
    }
  const rlim_t installed = (totalMemory > 0 && totalMemory > freeMemory)
	? totalMemory : 64L*(1L<<20);
  const rlim_t physical  = (freeMemory > 0)
	? freeMemory : (installed - 32L*(1L<<20))/3;

  if (MemorySize == 0)
    {
      const char msg[] = "Using (%s Memorysize) %lu MB";
      if (freeMemory <= 0)
	{
#define MB (1000L*1000L)
	  IndexingMemory = DefaultMemSize;
	  message_log (LOG_DEBUG, msg, "Default", IndexingMemory / MB);
	}
      else if ((IndexingMemory = physical/2) < MinimumMemSize)
	{
	  IndexingMemory = MinimumMemSize;
	  message_log (LOG_DEBUG, msg, "Minimum", IndexingMemory /MB);
	}
      else if ((UINT8)IndexingMemory > ((UINT8)DefaultMemSize)*4)
	{
	  IndexingMemory = DefaultMemSize + physical/8;
	  const char *factor = "Double the default";
	  if (IndexingMemory > ((UINT8)DefaultMemSize)*4)
	    {
	      if (IndexingMemory > ((UINT8)DefaultMemSize)*16) 
		{
		  factor = "8x the default";
		  IndexingMemory = DefaultMemSize*8;
		}
	      else if (IndexingMemory > ((UINT8)DefaultMemSize)*8)
		{
		  factor = "4x the default";
		  IndexingMemory = DefaultMemSize*4;
		}
	      else
		IndexingMemory = DefaultMemSize*2;
	      message_log (LOG_DEBUG, msg, factor, IndexingMemory /MB);
	    }
	}
    }
  else if ((INT4)MemorySize < 0)
    {
      if (freeMemory)
	message_log (LOG_DEBUG, "Free Memory = %ld kb, Total Memory = %ld kb",
		freeMemory/1024, totalMemory/1024);
      IndexingMemory = physical + installed/16 + MemorySize*PAGESIZE;
      if (IndexingMemory < MinimumMemSize)
	IndexingMemory = MinimumMemSize; 
      message_log (LOG_INFO, "Indexing memory auto-selected to %ld Kb (%ld pages).",
	IndexingMemory/1024, IndexingMemory/PAGESIZE);
      return;
    }
  else if  (MemorySize < 4096)           IndexingMemory = MemorySize * (1L << 20);
  else if (MemorySize < (1 << 20))       IndexingMemory = MemorySize * 1024L; // KB
  else if (MemorySize < MinimumMemSize)  IndexingMemory = MinimumMemSize;
  else                                   IndexingMemory = MemorySize;
  // Use Whole pages..
  IndexingMemory += (IndexingMemory & PAGEOFFSET);

  if ((freeMemory > 0) && ((((UINT8)IndexingMemory)*2L) > (((UINT8)physical)*3L)) && (physical > (installed/8)) )
    {
      if ((((UINT8)IndexingMemory)*2L) > (((UINT8)physical)*3L))
	{
	  message_log (LOG_WARN, "RAM Resources are low (only %ld kb free).", physical/1024);
	}
      else
	{
	  long wanted = IndexingMemory/(1L << 20);
	  IndexingMemory = ((((physical/PAGESIZE)*7)/4) - 10)*PAGESIZE;
	  if (IndexingMemory < MinimumMemSize)
	    IndexingMemory = MinimumMemSize;
	  message_log (LOG_WARN, "System has not enough free RAM (%lu kbytes) for %lu MB., adjusting to %ld kbytes.",
		freeMemory/1024, wanted, IndexingMemory/1024);
	}
    }
  message_log (LOG_DEBUG, "Indexing memory set to %ld kb (%ld pages).",
	IndexingMemory/1024, IndexingMemory/PAGESIZE);
}

/* void IDB::GenerateKeys() { MainMdt->GenerateKeys(); } */

void IDB::SetDebugMode (bool OnOff)
{
  DebugMode = OnOff;
  if (MainIndex)
    MainIndex->SetDebugMode(OnOff);
}


// Check Locks
INT IDB::GetLocks() const
{
#if _NO_LOCKS
  return 0;
#else
  // Check lock
  return Lock (DbFileStem, L_WRITE|L_READ|L_CHECK);
#endif
}

PRSET IDB::VSearch(const QUERY& Query)
{
  IRSET *irset = Search (Query);
  if (irset == NULL)
    {
      message_log (LOG_DEBUG, "Search --> Undefined result set");
      return NULL; // Undefined result set
    }

  const size_t y     = irset->GetTotalEntries ();
  const size_t Total = Query.GetMaximumResults();

  // Cut-off at Total
  if (y > Total && Total > 0)
    {
      irset->SortBy(Query.Sort);
      irset->SetTotalEntries( Total );
    }
  const PRSET Prset = irset->GetRset ();
  delete irset;

  Prset->SortBy(Query.Sort);
  return Prset;
}


PRSET IDB::VSearchSmart(QUERY *Query, const STRING& DefaultField)
{
  IRSET *irset = Query ? SearchSmart (Query, DefaultField) : NULL;
  if (irset == NULL)
    {
      message_log (LOG_DEBUG, "SearchSmart --> Undefined result set");
      return NULL; // Undefined result set
    }
  const size_t y     = irset->GetTotalEntries ();
  const size_t Total = Query->GetMaximumResults();

  // Cut-off at Total
  if (y > Total && Total > 0)
    {
      irset->SortBy(Query->Sort);
      irset->SetTotalEntries( Total );
    }
  const PRSET Prset = irset->GetRset ();
  delete irset;

  Prset->SortBy(Query->Sort);
  return Prset;
}


PIRSET IDB::Search (const QUERY& nQuery)
{
#if _NO_LOCKS
  // Check lock
  if (LockWait(DbFileStem, L_READ) == L_READ)
    {
      message_log(LOG_NOTICE|LOG_ERROR, "Database \"%s\" is locked.", DbFileName.c_str());
      return (new IRSET(this));
    }
#endif

  if (!IsDbCompatible ())
    {
      if (!compatible)
	SetErrorCode(-8*(int)sizeof(GPTYPE));
      return (new IRSET(this));
    }

  QUERY Query (nQuery);

  PDOCTYPE DoctypePtr = GlobalDoctype.IsDefined() ? GetDocTypePtr (GlobalDoctype) : NULL;
  if (DoctypePtr != NULL)
    {
      DoctypePtr->BeforeSearching (&Query);
    }
  IRSET *IrsetPtr = MainIndex->Search (Query);
  if (DoctypePtr != NULL) 
    DoctypePtr->AfterSearching (IrsetPtr);

// if (IrsetPtr->GetTotalEntries() == GetTotalRecords ())
//   "12 Too many records retrieved    (unspecified)"
  if (IrsetPtr)
    IrsetPtr->SortBy(Query.Sort);
  return IrsetPtr;
}

void IDB::BeginRsetPresent (const STRING& RecordSyntax)
{
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->BeforeRset (RecordSyntax);
    }
}


void IDB::BeforeIndexing ()
{
  {
    // Init the fields as defined in the DB.ini
    STRLIST fieldslist;
    FIELDTYPE fieldType;
  
    GetFieldDefinitionList(&fieldslist);
    for (STRLIST *p = fieldslist.Next(); p!=&fieldslist ; p=p->Next())
      if (!(fieldType = ProfileGetString(FieldTypeSection, p->Value())).IsText())
	AddFieldType(p->Value(), fieldType);
  }

  // Now do it for the global doctype
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->BeforeIndexing ();
    }
}

void IDB::AfterIndexing ()
{
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->AfterIndexing ();
    }
}

void IDB::BeforeSearching (QUERY* QueryPtr)
{
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->BeforeSearching (QueryPtr);
    }
}

IRSET *IDB::AfterSearching (IRSET* ResultSetPtr)
{
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->AfterSearching (ResultSetPtr);
    }
  return ResultSetPtr;
}


void IDB::EndRsetPresent (const STRING& RecordSyntax)
{
  if (GlobalDoctype.IsDefined())
    {
      PDOCTYPE DoctypePtr = GetDocTypePtr (GlobalDoctype);
      if (DoctypePtr != NULL)
	DoctypePtr->AfterRset (RecordSyntax);
    }
}

bool IDB::DfdtGetFileName (const STRING& Fieldname, STRING *StringBuffer) // const
{
  return DfdtGetFileName(Fieldname, FIELDTYPE::text, StringBuffer);
}

bool IDB::DfdtGetFileName (const DFD& dfd, STRING *StringBuffer)
{
  return DfdtGetFileName(dfd.GetFieldName(), dfd.GetFieldType(), StringBuffer);
}

bool IDB::DfdtGetFileName (const STRING& FieldName, const FIELDTYPE& FieldType,
	STRING *StringBuffer)
{
  STRING name, typ, path;

  // Check cache...
  if (FieldName.IsEmpty())
    {
      if (StringBuffer)
	StringBuffer->Clear();
      return false; // No Field...
    }

#if 1 /* WORKAROUND */
    // Handle mangeled Fieldnames
   int pos;
   if ((pos = FieldName.Search("\\![CDATA")) > 0) {
     return DfdtGetFileName( FieldName.Left((size_t)(pos-1)), FieldType, StringBuffer);
  }
#endif

  if (!FieldType.IsText())
    {
      typ  = FieldType.datatype();
      name = FIELDOBJ(FieldName, FieldType).GetName();
    }
  else
    name = FieldName;

  if (FileNames.GetValue(name, &path) == 0)
    {
      // Look up..
      INT filenumber;

      if ((filenumber = MainDfdt->GetFileNumber (FieldName)) == 0)
	return false; // Nope..
      FileNames.AddEntry(name,  path  = ComposeDbFn(filenumber) + typ );
    }
  if (StringBuffer) *StringBuffer = path;
  return true;
}


bool IDB::GetFieldData (const RESULT &ResultRecord, const FC& Fc,
        STRING * StringBuffer, const DOCTYPE *DoctypePtr)
{
  STRING   data;
  MDTREC   MdtRecord;
  FILE     *fpd;

  MainMdt->GetMdtRecord ( ResultRecord.GetKey (), &MdtRecord);

  const STRING   path ( ResolvePathname(MdtRecord.GetFullFileName()) );

  if (DoctypePtr == NULL)
    DoctypePtr = DocTypeReg->GetDocTypePtr (MdtRecord.GetDocumentType());

  if ((fpd = ffopen(path, "rb")) == NULL)
    {
      message_log (LOG_DEBUG, "IDB::GetFieldData: Could not open '%s'.", path.c_str());
      return false;
    }

  GPTYPE Start   = Fc.GetFieldStart();
  GPTYPE End     = Fc.GetFieldEnd();

  size_t res = (data.Fread(fpd, End-Start+1, Start) != 0);

//cerr << "Read " << res << " ---> " << data << endl;
  
// bool res = (ReadIndirect(fpd, StringBuffer, Start,  End - Start + 1, DoctypePtr) != 0);
  ffclose(fpd);

  if (StringBuffer) *StringBuffer = data.Strip(STRING::both);

  return res;
}


// edz: speed optimizations and support for indexing into list arrary
//      -- single elements and ranges..
//
// ElementSet
//      returns all the elements as a list
// [n]ElementSet
//      returns the n-th element as a list with one element
// [n-m]Element
//	returns the n-th to m-th elements as a list
//
// IDEA:  Field1/Field2/Field3 for ESet
//  1) look for FCs for Field1..
//  2) Find those in Field2 contained in Field1 
//  3) Find those in Field3 cotained in above
//  4) ...
// Present those Field3 in String list..

// TODO: Rewrite to use common FC read code!
bool IDB::GetFieldData (const RESULT &ResultRecord, const STRING& ESet,
	PSTRLIST StrlistBuffer, const DOCTYPE *DoctypePtr)
{
//cerr << "HERE I AM(2)" << endl;

  if (StrlistBuffer == NULL)
    return false;

  StrlistBuffer->Clear();
  if (ESet.IsEmpty())
    return false;

  int    start = 0;
  int    end = 0;
  STRING FieldName;
  // Was an index specified?
  if (ESet.GetChr(1) == '[')
    {
      char s[256 /*FieldName.GetLength()+1*/];
      // Note: uses overloaded string
      if (3 == sscanf(ESet.c_str(), "[%d-%d]%s", &start, &end, s))
        {
          FieldName = s;
        }
      else if (2 == sscanf(ESet.c_str(), "[%d]%s", &start, s))
        {
          end = start; // Single element
          FieldName = s;
        }
      else
	{
	  start = end = 0;
	  FieldName = ESet;
	}
    }
  else
    FieldName = ESet;

  STRING DfFileName;

//cerr << "XXXXXXX GetFieldData for " << ESet <<  " start=" << start << "  end=" << end << endl;

  if (DfdtGetFileName (FieldName, &DfFileName))
    {
//cerr << "DfFileNAme = " << DfFileName << endl;
      FILE *fp = ffopen(DfFileName, "rb"); // was ffopen()
      if (fp != NULL)
	{
	  MDTREC MdtRecord;

	  MainMdt->GetMdtRecord ( ResultRecord.GetKey (), &MdtRecord);

	  const GPTYPE GpStart = MdtRecord.GetGlobalFileStart () +
		MdtRecord.GetLocalRecordStart ();
	  const GPTYPE GpEnd   = MdtRecord.GetGlobalFileStart () +
		MdtRecord.GetLocalRecordEnd ();
	  const FC Fc (GpStart, GpEnd);

//cerr << "Search... " << DfFileName << " fd=" << fileno(fp) << " Size=" << GetFileSize(fp) <<  endl;
	  off_t Pos = OnDiskFcSearch(Fc, fp);
//cerr << "Pos = " << Pos << endl;
	  if (Pos)
	    {
	      // Get Document path..
//cerr << "Resolve " << endl;
	      PFILE fpd = ffopen( ResolvePathname(MdtRecord.GetFullFileName()), "rb"); // was fopen()
//cerr << "Opened!" << endl;
	      if (fpd != NULL) {
		if (DoctypePtr == NULL)
		  DoctypePtr = DocTypeReg->GetDocTypePtr (MdtRecord.GetDocumentType());
		
//cerr << "Extract" << endl;
		// extract field from documenty
		FC Fc2;
		GPTYPE FcStart = 0;
		// work backwards to find first pair
		while (--Pos >= 0)
		  {
		    if (-1 != fseek (fp, Pos * sizeof (FC), SEEK_SET))
		      {
			Fc2.Read (fp);
//cerr << "Read Pos=" << Pos << "  FC2 = " << Fc2 << endl;
			FcStart = Fc2.GetFieldStart();
			if (FcStart < GpStart) break;
		      }
		     else
		      message_log (LOG_DEBUG, "Seek to %ld'th FC failed", Pos);
		  }
//cerr << "DONE" << endl;
		int count = 0; // Keep track of the element count for the range
		STRING readBuffer;
		// Now collect all the data into a list
		do {
		  if (FcStart > GpEnd)
		    break; // Done 
		  if (FcStart >= GpStart)
		    {
//cerr << "FcStart = " << FcStart << "   GpStart=" << GpStart << endl;
		      // Extract Field from document
		      const GPTYPE offset = FcStart - MdtRecord.GetGlobalFileStart ();

//cerr << "XXXXXXX offset=" << offset << " count = " << count << " start=" << start << "  end=" << end <<  endl;
		      if ((++count < start && start != 0)) // Not yet at start
			continue;

		      const size_t x = Fc2.GetLength();

//cerr << ESet << " Offset=" << offset << "  x=" << x << endl;

		      // Callback to doctype to read..
		      if (ReadIndirect(fpd, &readBuffer, offset, x, DoctypePtr) != 0)
			StrlistBuffer->AddEntry (readBuffer);	// Add to list

		      if (count >= end && end !=0) //Done?
                        break;
		    }
		  Fc2.Read (fp);
//cerr << "Fc2 Read again" << Fc2 << endl;
		  FcStart = Fc2.GetFieldStart();
		  if (FcStart == 0)
		    {
		      message_log (LOG_ERROR, "FcStart == 0? (line %u)", __LINE__);
		      break;
		    }
		} while (!feof(fp));
	        ffclose (fpd);
	      }
	    }
	  ffclose (fp); // was ffclose(fp);
//cerr << "@@@@@@ Done" << endl;
	  return true;
	}
      else
	message_log (LOG_ERRNO, "Could not open %s (%s)", DfFileName.c_str(), FieldName.c_str());
    }
  else
    {
      message_log (LOG_DEBUG, "Field %s not available", FieldName.c_str());
    }
  return false;
}



bool IDB::GetFieldData(const FC& FieldFC, const STRING& FieldName, STRING* Buffer)
{
  FIELDTYPE ft = GetFieldType(FieldName);

//cerr << "FIELDTYPE = " << ft << endl;
  if (ft.IsDate())
    {
      SRCH_DATE date;
      if (IDB::GetFieldData(FieldFC, FieldName, &date))
        {
          *Buffer = date.ISOdate();
          return true;
        }
    }
  else if (ft.IsNumerical() || ft.IsComputed())
    {
      DOUBLE fVal;
      if (IDB::GetFieldData(FieldFC, FieldName, &fVal))
        {
          *Buffer = fVal;
          return true;
        }
    }
  return false;
}


// edz: Support for indexing into the Field Data list.
bool IDB::GetFieldData (const RESULT& ResultRecord, const STRING& FieldName,
  PSTRING StringBuffer, const DOCTYPE *DoctypePtr)
{
  if (StringBuffer == NULL)
    return false;

  FIELDTYPE ft = GetFieldType(FieldName);

//cerr << "FIELDTYPE = " << ft << endl;
  if (ft.IsDate())
    {
      SRCH_DATE date;
      if (IDB::GetFieldData(ResultRecord, FieldName, &date))
	{
	  *StringBuffer = date.ISOdate();
	  return true;
	}
    }
  else if (ft.IsNumerical() || ft.IsComputed())
    {
      DOUBLE fVal;
//cerr << "Try numeric" << endl;
      if (IDB::GetFieldData(ResultRecord, FieldName, &fVal))
	{
	  *StringBuffer = fVal;
	  return true;
	}
    }
  
  if (!FieldName.IsEmpty())
    {
      static const CHR glue[] = " , "; // ","-separated list
      STRLIST StrlistBuffer;
      // Get Field Data string list
      if (GetFieldData (ResultRecord, FieldName, &StrlistBuffer, DoctypePtr))
	{
	  if (!StrlistBuffer.IsEmpty())
	    {
	      StrlistBuffer.Join(glue, StringBuffer);
	      return true;
	    }
	}
    }
  else
    StringBuffer->Clear();
  return false;
}

// edz: Support for indexing into the Field Data list.
bool IDB::GetFieldData (const RESULT& ResultRecord, const STRING& FieldName,
  const FIELDTYPE& FileType, STRING *StringBuffer, const DOCTYPE *DoctypePtr)
{
  return false;
}



bool IDB::GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
  DOUBLE* Buffer)
{
  MDTREC      MdtRecord;

  if (MainMdt)
    {
      MainMdt->GetMdtRecord(ResultRecord.GetKey(), &MdtRecord);
      const GPTYPE gStart  = MdtRecord.GetGlobalFileStart();
      const GPTYPE GpStart = gStart + MdtRecord.GetLocalRecordStart();
      const GPTYPE GpEnd   = gStart + MdtRecord.GetLocalRecordEnd();

      return GetFieldData( FC(GpStart, GpEnd), FieldName, Buffer);
    }
  return false;
}

bool IDB::GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
  SRCH_DATE* Buffer)
{
  MDTREC      MdtRecord;

  if (MainMdt)
    {
      MainMdt->GetMdtRecord(ResultRecord.GetKey(), &MdtRecord);
      const GPTYPE gStart  = MdtRecord.GetGlobalFileStart();
      const GPTYPE GpStart = gStart + MdtRecord.GetLocalRecordStart();
      const GPTYPE GpEnd   = gStart + MdtRecord.GetLocalRecordEnd();

      return GetFieldData (FC(GpStart, GpEnd), FieldName, Buffer);
    }
  return false;
}


STRING IDB::XmlMetaPresent(const RESULT &ResultRecord) const 
{
  STRING String;
  PDOCTYPE DoctypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DoctypePtr)
    DoctypePtr->XmlMetaPresent(ResultRecord, NulString, &String); 
  return String;
}   


#if 1

bool IDB::GetFieldData(GPTYPE gp, DOUBLE* Buffer)
{
  STRING      FieldName;
  FC          HitFc ( GetPeerFc (gp, &FieldName) );

  return GetFieldData(HitFc, FieldName, Buffer);
}


bool IDB::GetFieldData(const FC& FieldFC, const STRING& FieldName, DOUBLE* Buffer)
{
  STRING      DfFileName;

  DfdtGetFileName(FieldName,  FIELDTYPE::numerical, &DfFileName);
  if (!FileExists(DfFileName))
    {
      // Not a numerical field
      if (Buffer) *Buffer = 0;
      return false;
    }

  // Start is the smallest index in the table
  // for which GpStart is <= to the table value
  NUMERICLIST List;
  INT4        Start=-1;
  SearchState Status;

#if 1
  if ((Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelLE, &Start)) == TOO_LOW)
    return false;
  if (Status != MATCH)
    Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelGE, &Start);
#else
  if ((Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelGE, &Start)) == TOO_LOW)
    {
      // This case is when the position is at the very start!
      Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelLE, &Start);
    }
#endif
  if (Status == MATCH)
    {
      const int count    = List.GetCount();
      const int nrecs    = List.LoadTable(Start, Start, GP_BLOCK);

#if 1
      for (int i=0; i < nrecs; i++) {
         if (FieldFC.Contains( List.GetGlobalStart(i+count)))
	    {
	      if (Buffer) *Buffer = List.GetNumericValue(i+count);
	      return true;
	    }
	} // for 
       message_log (LOG_PANIC, "Could not find %s data for '%s' in %s",
                "numerical", FieldName.c_str(), (const char *)((STRING)FieldFC));
    }
#else
      GPTYPE start = List.GetGlobalStart(0);

      if (FieldFC.Contains(start))
	{
	  if (Buffer) *Buffer = List.GetNumericValue(0); 
	  return true;
	}
       message_log (LOG_PANIC, "Could not find %s data for '%s' %ld not in %s",
		"numerical", FieldName.c_str(), (long)start, (const char *)((STRING)FieldFC));
    }
#endif
  return false; 
}


bool IDB::GetFieldData(GPTYPE gp, SRCH_DATE* Buffer)
{
  STRING      FieldName;

  return GetFieldData (GetPeerFc (gp, &FieldName), FieldName, Buffer);
}



bool IDB::GetFieldData(const FC& FieldFC, const STRING& FieldName, SRCH_DATE* Buffer)
{
  SearchState Status;
  DATELIST    List;
  INT4        Start=-1;
  STRING      DfFileName;

  DfdtGetFileName(FieldName,  FIELDTYPE::date, &DfFileName);
  if (!FileExists(DfFileName))
    {
      // Not a numerical field
      if (Buffer) *Buffer = 0;
      return false;
    }

  // Start is the smallest index in the table
  // for which GpStart is <= to the table value
  if ((Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelGE, &Start)) == TOO_LOW)
    {
      // This case is when the position is at the very start!
      Status = List.Find(DfFileName, FieldFC.GetFieldStart(), ZRelLE, &Start);
    }

  if (Status == MATCH)
    {
      const int count    = List.GetCount();
      List.LoadTable(Start, Start, GP_BLOCK);
      GPTYPE start = List.GetGlobalStart(count);
      if (FieldFC.Contains(start))
	{
	  if (Buffer) *Buffer = List.GetValue(count); 
	  return true;
	}
       message_log (LOG_PANIC, "Could not find %s data for '%s' %s",
		"date", FieldName.c_str(), (const char *)((STRING)FieldFC));
    }
  return false;
}


bool IDB::GetFieldData(GPTYPE gp, STRING* Buffer)
{
  STRING      FieldName;
  FC FieldFC  (GetPeerFc (gp, &FieldName));
  FIELDTYPE ft = GetFieldType(FieldName);

  if (ft.IsNumerical() || ft.IsComputed())
    {
      DOUBLE fVal;
      if (GetFieldData(FieldFC, FieldName, &fVal))
	{
	  if (Buffer) *Buffer = fVal;
	  return true;
	}
    }

  if (ft.IsDate())
    {
      SRCH_DATE date;
      if (GetFieldData(FieldFC, FieldName, &date))
        {
          if (Buffer) *Buffer = date.ISOdate();
          return true;
        }
    }
  return false;

    
}



#endif


STRING IDB::GetPeerContent(const FC& HitFc)
{
  STRING fieldname, Result;
  FC     PeerFc = GetPeerFc (HitFc, &fieldname);
  if (!fieldname.IsEmpty())
    {
      STRING Fn;
      DfdtGetFileName (fieldname, &Fn); // Coordinates are TEXT type
      MDTREC mdtrec;
      int mdt_index = MainMdt->GetMdtRecord (HitFc.GetFieldStart(), &mdtrec);
      if (mdt_index != 0)
	{
	  // Found the record
	  const STRING filename (ResolvePathname( mdtrec.GetFullFileName() ) );
	  const GPTYPE Start = PeerFc.GetFieldStart() - mdtrec.GetGlobalFileStart();
	  const GPTYPE Length =  PeerFc.GetFieldEnd()-PeerFc.GetFieldStart()+1; 
#if 1
	  if (Length > 0 && Start >= 0)
	    ::GetRecordData(filename, &Result, Start, Length, GetDocTypePtr(mdtrec.GetDocumentType()) );
#else /* Old code */
	  Result.Fread(filename, Length, Start);
#endif
	}
    }
  return Result;
}

STRING IDB::GetPeerContentXMLFragement(const FC& HitFc)
{
#if 1
  return GetNodeTree(HitFc).XMLNodeTree( GetPeerContent(HitFc)  );
#else
  STRING       Content ( GetPeerContent(HitFc) );
  TREENODELIST List;

  if (GetNodeList (HitFc, &List) > 0)
    return List.XMLNodeTree( Content ); 
  return Content; 
#endif
}

//
// Writes the Headline to the cache
//
bool IDB::CacheHeadline(GPTYPE MdtIndex, const STRING& Headline,
	const STRING& RecordSyntax) const
{
#if NEW_HEADLINE_CACHE_CODE 
  // New format <OID>.idx contains the address offset and <OID>
  // contains the headline string
  STRING Result;
  STRING Filename (CacheDir+RecordSyntax);

  FILE *fp = MainIndex->ffopen(Filename, "ab+");
  if (fp != NULL)
    {
      UINT8 offset = ftell(fp);
      Headline.Write(fp);
      MainIndex->ffclose(fp);

      // We use low level I/O and exploit file holes here
      int fd = open(Filename+".idx", O_WRONLY|O_CREAT, 0666);
      if (fd != -1)
	{
#ifdef _WIN32
	  setmode(fd, O_BINARY);
#endif
	  if (lseek(fd, (off_t)MdtIndex*sizeof(offset), SEEK_SET) != -1)
	    ::write(fd, &offset, sizeof(offset));
	  else
	    message_log (LOG_ERRNO, "Could not store Headline cache address");
	  close(fd);
	  return true;
	}
      message_log (LOG_ERRNO, "Could not write to headline cache %s", RecordSyntax.c_str());
    }
  else message_log (LOG_ERRNO, "Could not write to headline cache %s", RecordSyntax.c_str());
  return false;
#else
  STRING s;
  int res = -1;
  const int HeadlineLen = Headline.GetLength()+1;
  STRING    cacheName (CacheDir+RecordSyntax);

  int fd = open(cacheName, O_WRONLY|O_CREAT, 0000666);
  if (fd != -1)
    {
#ifdef _WIN32
      setmode(fd, O_BINARY);
#endif
#define min(x,y) ((x > y) ? y : x)
      if (lseek(fd, ((off_t)MdtIndex)*MaxHeadlineLen, SEEK_SET) != -1)
	res = write(fd, Headline.c_str(), min(HeadlineLen, MaxHeadlineLen));
#undef min
#ifndef _WIN32
      fchmod (fd, 0000666);
#endif
      close(fd);
    }
  else
    message_log (LOG_ERRNO, "Could not write to headline cache  '%s'", cacheName.c_str());
  return res != -1;
#endif
}

//
// Looks up to see if the Headline (in the appropriate Record Syntax) is
// available. If so.. returns the headline from the cache
//
STRING IDB::LookupHeadlineCache(GPTYPE MdtIndex, const STRING& RecordSyntax) const
{
#if NEW_HEADLINE_CACHE_CODE 
  // New format <OID>.idx contains the address offset and <OID>
  // contains the headline string
  STRING Result;
  STRING Filename (CacheDir+RecordSyntax);
  FILE *fp = MainIndex->ffopen(Filename + ".idx", "rb");
  if (fp != NULL)
    {
      UINT8    Offset;
      if (fseek(fp, (off_t)MdtIndex*sizeof(Offset), SEEK_SET) == 0)
	{
	  ::read(fileno(fp), &Offset, sizeof(Offset));

	  FILE *Sfp = MainIndex->ffopen(Filename, "ab+");
	  if (Sfp != NULL)
	    {
	      if (Offset > 0 && fseek(Sfp, Offset, SEEK_SET) == 0)
		Result.Read(Sfp);
	      MainIndex->ffclose(Sfp);
	    }
	}
      MainIndex->ffclose(fp);
    }
  return Result;

#else
  STRING Result;
  STRING cacheName ( CacheDir+RecordSyntax );
  if (FileExists(cacheName))
    {
      FILE *fp = MainIndex->ffopen(cacheName, "rb");
      if (fp != NULL)
	{
	  Result.Fread(fp, MaxHeadlineLen, ((off_t)MdtIndex)*MaxHeadlineLen);
	  MainIndex->ffclose(fp);
	}
    }
  return Result;
#endif
}

bool IDB::Headline(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  if (RecordSyntax.IsEmpty())
    return Headline(ResultRecord, StringBuffer);

//cerr << "Want Headline in Syntax " << RecordSyntax << endl;
  if (!CacheDir.IsEmpty())
    {
      *StringBuffer = LookupHeadlineCache(ResultRecord.GetMdtIndex(), RecordSyntax);
      if (!StringBuffer->IsEmpty())
	return true;
    }
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    {
//cerr << "XXXXXXXX Lookup Headline for " << DocTypePtr->Name() << endl;
      if (DocTypePtr->Headline (ResultRecord, RecordSyntax, StringBuffer))
	{
	  if (!CacheDir.IsEmpty())
	    CacheHeadline(ResultRecord.GetMdtIndex(), *StringBuffer, RecordSyntax);
	  return true;
	}
    }
//cerr << "No Doctype pointer.. Failed" << endl;
  StringBuffer->Clear();
  return false;
}

// Show Headline ("B")..
bool IDB::Headline(const RESULT& ResultRecord, PSTRING StringBuffer) const
{
#if 1
  return Headline(ResultRecord, SutrsRecordSyntax, StringBuffer);
#else
// cerr << "IndexID = " << ResultRecord.GetMdtIndex() << endl;
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    return DocTypePtr->Headline (ResultRecord, StringBuffer);
  return false;
#endif
}


// Context Match
bool IDB::Context(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
	const STRING& Before, const STRING& After, STRING *TagName) const
{
  return (&ResultRecord)->PresentBestContextHit(Line, Term, Before, After,
        GetDocTypePtr( ResultRecord.GetDocumentType() ), TagName);
}


bool IDB::XMLContext(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
        const STRING& Tag) const
{
  return (&ResultRecord)->XMLPresentFirstHit(Line, Tag, Term,
        GetDocTypePtr( ResultRecord.GetDocumentType() ));
}


// Record Summary (if available)
bool IDB::Summary(const RESULT& ResultRecord,
  const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    return DocTypePtr->Summary (ResultRecord, RecordSyntax, StringBuffer);
  return false;
}

bool IDB::URL(const RESULT& ResultRecord, PSTRING StringBuffer,
   bool OnlyRemote) const
{
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    {
      return DocTypePtr->URL (ResultRecord, StringBuffer, OnlyRemote);
    }
  return false;
}

void IDB::HighlightedRecord(const RESULT& ResultRecord,
   const STRING& BeforeTerm, const STRING& AfterTerm, PSTRING StringBuffer) const
{
  ResultRecord.GetHighlightedRecord(BeforeTerm, AfterTerm, StringBuffer,
	GetDocTypePtr( ResultRecord.GetDocumentType()) );
}


STRING IDB::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 const STRING& RecordSyntax) const
{
  STRING Presentation;
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    DocTypePtr->Present (ResultRecord, ElementSet, RecordSyntax, &Presentation);
  return Presentation;
}

void IDB::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
         const STRING& RecordSyntax, PSTRING StringBuffer,
	 const QUERY& Query)
{
  STRING Cache;
  STRING ESet = ElementSet;
  // Use Cache is available...
  if (ESet == HIGHLIGHT_MAGIC)
    {
      if (StringBuffer)
	*StringBuffer = GetXMLHighlightRecordFormat(ResultRecord);
      return;
    }
  if (ESet == LOCATION_MAGIC)
    {
      if (URL(ResultRecord, &Cache, true))
	{
	  // Redirect (this is also for proxy)
          StringBuffer->form("Status-Code: 302\nServer: NONMONOTONIC IB %s\nLocation: %s\r\n\r\n",
		__IB_Version, Cache.c_str());
	  return;
	}
      ESet = FULLTEXT_MAGIC; // Pretend Fulltext
    }
  if (ESet == FULLTEXT_MAGIC)
    {
      if (CacheDir.GetLength())
	{
	  struct stat st_buf;
	  STRING      Key;

	  ResultRecord.GetKey(&Key);
	  Cache.form("%s%02X.F/%s/%s", CacheDir.c_str(), 
		Key.Hash() & 0xFF, Key.c_str(), (const char *)RecordSyntax);
	  if (0 == stat(Cache.c_str(), &st_buf))
	    {
	      // Check Time
	      time_t mtime = st_buf.st_mtime;
	      if (0 == stat(ResultRecord.GetFullFileName().c_str(), &st_buf))
		{
		  if (st_buf.st_mtime <= mtime)
		    {
		      StringBuffer->ReadFile(Cache);
		      if (Cache.GetLength())
			return; // Got a cached Present..
		    }
		}
	    }
	}
    }
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    {
      DocTypePtr->DocPresent (ResultRecord, ESet, RecordSyntax, StringBuffer, Query);
      if (Cache.GetLength())
	{
	  StringBuffer->WriteFile(Cache);
	}
    }
  else
    {
      StringBuffer->Empty(); // No DocTypePtr!
    }
}


void IDB::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer)
{
  DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer, QUERY());
}

void IDB::DocHighlight (const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  PDOCTYPE DocTypePtr = GetDocTypePtr ( ResultRecord.GetDocumentType () );
  if (DocTypePtr)
    {
      DocTypePtr->DocHighlight (ResultRecord, RecordSyntax, StringBuffer);
    }
  else
    StringBuffer->Empty(); // No DocTypePtr!
}

void IDB::GetDbVersionNumber (PSTRING StringBuffer) const
{
  MainRegistry->ProfileGetString(DbInfoSection, VersionEntry, NulString, StringBuffer); 
}



STRING IDB::Description() const
{
// XXXXXX
  static STRING result;

  if (result.GetLength()) return result;

  result = "Physical database level <database>.ini Options:\n";
  result << "[" << DbInfoSection << "]\n";
  result << DatabasesEntry << "=<Path to db stem (Physical Indexes)> # Default: same directory as .ini\n";
  result << WorkingDirEntry << "=<Base Directory> # WARNING: CRITICAL VALUE\n";
  result << useRelativePathsEntry << "=<bool> # Use relative paths (0 or 1)\n";
  result << autoDeleteExpiredEntry << "=<bool> # Automatically delete expired records (0 or 1)\n";
  result << MaximumRecordSizeEntry << "=nnnn # Max. Record size (bytes) to index (default ";
  if (DEFAULT_MaximumRecordSize >= (1024*1024))
    result << (DEFAULT_MaximumRecordSize/(1024*1024L)) << "mb).\n";
  else 
    result << (DEFAULT_MaximumRecordSize/1024) << "kb).\n";
  result << "Headline[/RecordSyntax]=<format of headline>\n";
  result << "Summary[/RecordSyntax]=<format of summary>\n";
  result << CacheSizeEntry << "=nnn  # Size of cache\n";
  result << CachePersistEntry << "=<bool>  # Should the cache persist?\n";
  result << SearchCutoffEntry << "=nnn    # Stop searching after nnn hits\n";
  result << SearchTooManyEntry<< "=nnnn   # Suggest limit for set complements.\n";
  result << CacheDirEntry << "=<Directory to store cache>\n";
  result << VersionEntry << "=<Version>\n";
  result << LocaleEntry << "=<Global Locale Name>\n";
  result << SegmentNameEntry << "=<Short DB Title for use as virtual segment name>\n";
  result << TitleEntry << "=<Database Title> # Complete (exported) title\n";
  result << CommentsEntry << "=<Comments>\n";
  result << CopyrightEntry << "=<Copyright Statement>\n";
  result << StoplistEntry << "=<Language or Path/Format to stopwords list file>\n";
  result << DateEntry << "=<DateCreated>\n";
  result << DateModifiedEntry << "=<Date of last modification>\n";
  result << MaintainerNameEntry << "=<Name of DB maintainer>\n";
  result << MaintainerMailEntry << "=<Email address for maintainer>\n";
  result << PluginsPathEntry << "=<path to directory where plugins are installed>\n";

  result << "\n[" << externalSortSection << "}\n";
  result << "<nn>=<path> # <nn> is number and path is to the external sort file\n";
  result << "            # if not defined it looks for <DB>.__<nn>\n";
  result << "\n[" << RankingSection << "]\n";
  result << PriorityFactorEntry << "=fff.ff   # Priority factor\n";
  result << IndexBoostEntry << "=fff.ff # Boost score by index position\n";
  result << FreshnessBoostEntry << "=fff.ff # Boost score by freshness\n";
  result << FreshnessDateEntry << "=date # Date/time, Records newer than this date\n";
  result << "# get " << FreshnessBoostEntry << " added, older get substracted. The unit\n\
# of resolution is defined by the precision of the specified date. Default is the date\n\
# specified in " << DateModifiedEntry << " [" << DbInfoSection <<  "] (Minutes resolution)\n";
  result << LongevityBoostEntry << "=fff.fff # Boost score by difference in days between\n\
# the date created and date modified of the records.\n";
  result << "\n[HTTP]\nPages=<Path to root of htdoc tree>\n\
IP-Name=<ip address of server>\n\
Server=<Server Name, e.g. www.nonmonotonic.com>\n";

  result << "\n[Mirror]\nRoot=<Path to root of mirror tree>\n";
  result << "\n[<Doctype>/Metadata-Maps]\n<Tag>=<TagValue> # TagValue of <Ignore> means Ignore it\n";


  if (MainIndex)
    result << "\n" << MainIndex->Description();

  result << "\nNOTE: .INI files may contain other .ini files via an include directive:\n\
#include <path>     # alternative include \"path\" (may be .ini or XML/SGML format)\n\n";

  return result;
}


//
// Parent can override my values
//
void IDB::ProfileGetString(const STRING& Section, const STRING& Entry,
	const STRING& Default, PSTRING StringBuffer) const
{
  STRING value;

  if (MainRegistry)
    MainRegistry->ProfileGetString(Section, Entry, Default, &value);
  if (Parent)
    Parent->ProfileGetString(Section, Entry, value, StringBuffer);
  else if (StringBuffer)
   *StringBuffer = value;
}

void IDB::ProfileWriteString(const STRING& Section, const STRING& Entry,
	const STRING& Value)
{
  MainRegistry->ProfileWriteString(Section, Entry, Value);
}


SRCH_DATE IDB::DateOf(const STRING& Entry) const
{
  SRCH_DATE datum;
  STRING Value;
  if (MainRegistry)
    MainRegistry->ProfileGetString(DbInfoSection, Entry, NulString, &Value);
  if (Value.IsEmpty() || !datum.Set (Value))
    datum.SetNow();
  return datum;
}

SRCH_DATE IDB::DateLastModified() const
{
  SRCH_DATE LastMod ( DateOf(DateModifiedEntry) );
  if (MainMdt)
    {
      const SRCH_DATE Datum ( MainMdt->GetTimestamp()) ;
      if (!LastMod.Ok())
	LastMod = Datum;
      else if (Datum.Ok() && Datum > LastMod)
	LastMod = Datum;
    }
  return LastMod;
}

SRCH_DATE IDB::DateCreated() const
{
  SRCH_DATE datum( DateOf(DateEntry) );
  if (!datum.Ok())
    datum = DateOf(DateModifiedEntry);
  if (MainMdt)
    {
      SRCH_DATE stamp (MainMdt->GetTimestamp());
      // We want the older
      if (stamp.Ok() && datum > stamp)
	datum = stamp;
    }

  // Now when was the .dfd created?
  SRCH_DATE cdfd;
  cdfd.SetTimeOfFileCreation( ComposeDbFn (DbExtDfd));

  if (cdfd < datum)
    datum = cdfd;

  return datum;
}
 


STRING IDB::GetVersionID () const
{
  STRING Version;
  Version << __IB_Version << "-" << (INT)MainIndex->Version() << "." << MainMdt->Version()
	 << "-" << 8*sizeof(GPTYPE) << "bit";
  return Version;
}

// Note: if you change this symbol you MUST also change it
// in index.cxx
//
#define BOUNDARY '#'

bool IDB::AddRecord (const RECORD& NewRecord)
{
  if (Parent && (NewRecord.GetSegment() != SegmentNumber))
    return Parent->AddRecord(NewRecord);

  if (DbFileStem.IsEmpty()) return false;


  bool Result = false;
  const STRING FileName ( NewRecord.GetFullFileName() );
  const off_t fileLength = GetFileSize(FileName);
  UINT4       End        = NewRecord.GetRecordEnd();
  UINT4       Start      = NewRecord.GetRecordStart();

  bool        remote     = FileName.Search("://") != 0;

//cerr << "AddRecord " << FileName << endl;


  if (!remote && fileLength == 0)
    message_log (LOG_INFO, "Skipping '%s' (empty or non-accesible)", FileName.c_str());
  else if (End > fileLength || Start > fileLength || (End >0 && Start > End))
    {
      message_log (LOG_NOTICE, "Skipping '%s'[%u-%lu], specified record out of bounds.",
	FileName.c_str(), Start, End == 0 ? fileLength : End);
    }
  else if (remote || !IsSystemFile (FileName)) // Only add non-system files
    {
      const STRING IndexingQueueFn (ComposeDbFn (DbExtIndexQueue1));
      PFILE fp = IDB::ffopen (IndexingQueueFn, "ab");
      if (fp)
	{
	  errno = 0;
	  lockfd(fileno (fp), 0);
	  Write((BYTE)BOUNDARY, fp);
	  NewRecord.Write (fp);

	  unlockfd(fileno (fp));
	  if (ferror(fp)) {
	    message_log (LOG_ERRNO, "I/O Error appending to Indexing Queue \"%s\"!", IndexingQueueFn.c_str());
	  } else {
	    Queue1Add++;
	    Result = true;
	  }
	  IDB::ffclose (fp);
	} else
	  message_log (LOG_ERRNO, "Could not append to \"%s\", skipping!", IndexingQueueFn.c_str()); 
    }
  else
    message_log (LOG_INFO, "Skipping '%s' (System file)", FileName.c_str());
  return Result;
}

void IDB::SetMaximumRecordSize(INT value)
{
  INT oldMaximumRecordSize = MaximumRecordSize;
  if (value < 0)
    MaximumRecordSize = INT_MAX;
  else if (value < 1024)
    MaximumRecordSize = value*1024L*1024L; // MB
  else 
    MaximumRecordSize = value;
  if (oldMaximumRecordSize != MaximumRecordSize)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, MaximumRecordSizeEntry, 
	MaximumRecordSize);
      DbInfoChanged = true;
    }
}

void IDB::DocTypeAddRecord (const RECORD& NewRecord)
{
//cerr << "IDB::DocTypeAddRecord" << endl;
  if (Parent)
    {
      if (NewRecord.GetSegment() != SegmentNumber)
	{
          Parent->DocTypeAddRecord(NewRecord);
	  return;
	}
    }

  if (!NewRecord.Exists())
    {
      if (!NewRecord.IsBadRecord())
        message_log (LOG_INFO, "Record '%s' does not exist", NewRecord.GetFullFileName ().c_str());
      return;
    }

  const UINT4 Start = NewRecord.GetRecordStart ();
  UINT4       End   = NewRecord.GetRecordEnd ();

  if (End > 0 && ((INT)(End-Start) >= MaximumRecordSize))
    {
      message_log (LOG_NOTICE, "Record '%s'[%u-%u] is larger than MaxRecordSize (%u). Skipping!",
	  NewRecord.GetFullFileName ().c_str(), (unsigned)Start, (unsigned)End, MaximumRecordSize);
      return;
    }
  else if (End > 0 && End <= Start)
    {
      message_log (LOG_NOTICE, "Record '%s' End(%u)<=Start(%u). Skipping!",
	(unsigned)End, (unsigned)Start);
      return;
    }

  FILE *fp;
  const STRING IndexingQueueFn( ComposeDbFn (DbExtIndexQueue2) );

  if (IndexingMemory == 0)
    SetIndexingMemory (0, false); // Set Memory

  bool Ok = false;
  if ((fp = IDB::ffopen (IndexingQueueFn, "ab")) != NULL)
    {
      lockfd(fileno (fp), 0);
      if (End == 0)
        {
	  // Get the real end...
	  RECORD record (NewRecord);
          // Length of file..
          const STRING fn (NewRecord.GetFullFileName ());
          off_t fileSize = GetFileSize (fn); 
	  if (fileSize > 0)
	    {
	      if ((fileSize-Start) > MaximumRecordSize)
		{
		  message_log (LOG_NOTICE, "Record '%s' is larger than MaxRecordSize (%uk). Skipping!",
		 	fn.c_str(), (unsigned)(MaximumRecordSize/1024));
		  fileSize = 0;
		}
	    }
	  else message_log (LOG_NOTICE, "Record '%s' is empty!", fn.c_str());
	  if ((End = (UINT4)fileSize) != 0 && ((off_t)End - (off_t)Start) > 2)
	    {
	      record.SetRecordEnd(End-1);
	      // Write it...
	      Write ((BYTE)BOUNDARY, fp);
	      record.Write (fp);
	      Ok = true;
	    }
        }
      else if ((off_t)End - (off_t)Start > 2)
	{
	  Write ((BYTE)BOUNDARY, fp);
	  NewRecord.Write (fp);
	  Ok = true;
	}

      unlockfd(fileno (fp));

      if (ferror(fp))
	message_log (LOG_ERRNO, "I/O Error appending to Indexing Queue \"%s\"!", IndexingQueueFn.c_str());
      else if (Ok) // Make sure we added something
	Queue2Add++;

      IDB::ffclose (fp);

      // Is now in Queue
      if (Ok)
	{
	  TotalRecordsQueued++;
	  const UINT4 IndexingSize = (End - Start +
		StringCompLength + 1)*(3 + sizeof(GPTYPE))/3; // See index.cxx
	  if (IndexingSize > IndexingMemory && End > 0)
	    {
	      // Adjust Memory...
	      IndexingMemory = (IndexingSize/PAGESIZE + 1)*PAGESIZE;
	      message_log (LOG_INFO, "Increasing Indexing memory buffer setting to %ld pages (%u)",
		(long)(IndexingMemory/PAGESIZE), (unsigned)PAGESIZE);
	    }
	}
    }
  else
    message_log (LOG_ERRNO, "Could not append to \"%s\", skipping!", IndexingQueueFn.c_str());

//cerr << "DocTypeAdd done" << endl;
}


//
//

bool IDB::Index (bool newIndex)
{
  STRING IqFnBak =  ComposeDbFn(_DbExt( ExtLAST ));
  if (FileExists(IqFnBak))
    {   
      message_log (LOG_INFO, "Old '%s' left from stale process?", IqFnBak.c_str());
      if (UnlinkFile(IqFnBak) == -1)
	{
	  const STRING bak (IqFnBak + ".DELETE_ME");
	  if (RenameFile(IqFnBak, bak) == 0)
	    {
	      AddtoGarbageFileList(bak);
	    }
	  else
	    message_log (LOG_ERROR|LOG_ERRNO, "Could not remove '%s'.", IqFnBak.c_str());
	}
    }

  if (newIndex) 
    {
      int bitChange = (compatible == false);

      if (MainMdt == NULL)
	message_log (LOG_ERROR, "Index with Nil MainMdt?");
      else if (MainMdt->GetTotalEntries ())
	{
	  // Need to save queue1 if it exists
	  STRING IqFn = ComposeDbFn (DbExtIndexQueue1);
	  if (FileExists(IqFn)) // Move
	    if (RenameFile(IqFn, IqFnBak) == -1)
	      message_log (LOG_ERROR|LOG_ERRNO, "Could not rename '%s' to '%s'", IqFn.c_str(), IqFnBak.c_str()); 
	  if (KillAll() == false)
	    return false;
	  if (FileExists(IqFnBak) && !FileExists(IqFn))
	    if (RenameFile(IqFnBak, IqFn) == -1)
	      message_log (LOG_ERROR|LOG_ERRNO, "Could not re-install '%s' to '%s'", IqFnBak.c_str(), IqFn.c_str());
	}
      else if (bitChange)
	{
	  compatible = true;
	}
      if (bitChange)
	message_log (LOG_INFO, "Indexing with %u-bits", (unsigned)( 8*sizeof(GPTYPE)));
    }
  if (!IsDbCompatible ())
    {
      if (!compatible)
	SetErrorCode(-8*(int)sizeof(GPTYPE));
      return false;
    }
  if (Index1() == false)
    return false;
  return Index2();
}

bool IDB::Index1()
{
#if _NO_LOCKS
#else
  // Set lock
  if ((Lock(DbFileStem, L_WRITE) & L_WRITE) != L_WRITE)
    {
      message_log (LOG_NOTICE|LOG_ERROR, "Can't set lock for \"%s\".", (const char *)DbFileName);
      return false;
    }
#endif

  IndexingStatus (IndexingStatusParsingFiles, 0, 0);

  bool setGlobalDoctype = GlobalDoctype.IsDefined() ? false : true;

  if (!setGlobalDoctype)
    BeforeIndexing ();

  message_log(LOG_DEBUG, "Pass(1) Queue passed min. %lu elements", Queue1Add);

  STRING IqFn = ComposeDbFn (DbExtIndexQueue1);
  PFILE fp = IDB::ffopen (IqFn, "rb");

  // Read the Records in the Queue
  if (fp) {
    RECORD Record;

    if (DebugMode) message_log (LOG_DEBUG, "Reading Pass(1) Indexing Queue '%s'", IqFn.c_str());
    for (;;) {
      BYTE c;
      Read (&c, fp);
      if (c == BOUNDARY)
	{
	   // Read a record from file queue
	  int errs = 0;
	  do {
	    if (Record.Read (fp) == true) {
	      errs = 0;
	      break;
	    }
	    do {
	      ::Read(&c, fp);
	    } while (c != BOUNDARY && !feof(fp));
	  } while (++errs < 5 && !feof(fp));
	  if (errs)
	    {
	      message_log (LOG_PANIC, "Indexer input record queue corrupted?");
	      break;
	    }
	  if (setGlobalDoctype)
	    {
	      DOCTYPE_ID Doctype = Record.GetDocumentType();
	      if (Doctype.IsDefined())
		{
		  SetGlobalDoctype (Doctype);
		  BeforeIndexing();
		  setGlobalDoctype = false;
		}
	    }
//cerr << "ParseRecords loop in IDB" << endl;
	  ParseRecords(Record);
	}
      else
	break; // Done
    }
    IDB::ffdispose (fp); // Hard Close
    if (UnlinkFile (IqFn) == -1)
      {
	STRING bak (IqFn + ".DELETE_ME");
	if (RenameFile(IqFn, bak) != 0)
	  bak = IqFn;
        AddtoGarbageFileList(bak);
       }
  } else {
    if (SegmentNumber == 0)
	message_log (LOG_NOTICE, "No documents, nothing to do!");
  }
/// End Queue1
#if _NO_LOCKS
#else
  if (UnLock(DbFileStem, L_WRITE) != L_WRITE)
    message_log (LOG_NOTICE|LOG_ERROR, "Can't reset lock for \"%s\".", (const char *)DbFileName);
#endif
  return true;
}


bool IDB::Index2()
{
#if _NO_LOCKS
#else
  bool readlocked = false;
  if (MainMdt->GetTotalEntries() == 0)
    readlocked = ( (Lock(DbFileStem, L_READ|L_WRITE) & L_READ) != 0);
  else
    Lock(DbFileStem, L_WRITE);
#endif

  IndexingStatus (IndexingStatusParsingRecords);

  MainMdt->Resize (MainMdt->GetTotalEntries () + TotalRecordsQueued);
  STRING IqFn = ComposeDbFn (DbExtIndexQueue2);

  if (DebugMode) message_log(LOG_INFO, "Pass(2) Queue contains %lu records. Total Records Queued = %lu",
	Queue2Add, TotalRecordsQueued);

  bool result = true;
  FILE       *fp;

  ffdispose(IqFn); // Don't want a cache, make sure closed
#if 1
  fp = fopen (IqFn, "rb");
#else
  fp = ffopen (IqFn, "rb"); // Cach'd opens
#endif
  if (fp != NULL) 
    {
      if (!PersistantIrsetCache.IsEmpty())
	{
	  if (Exists(PersistantIrsetCache))
	    {
	      message_log (LOG_DEBUG, "Zapping persistant cache \"%s\"", PersistantIrsetCache.c_str());
	      if (UnlinkFile(PersistantIrsetCache) == -1)
		message_log (LOG_ERRNO, "Could not remove '%s' (Persistant IRSET Cache)", PersistantIrsetCache.c_str());
	    }
	}
      if (DebugMode) message_log (LOG_INFO, "Reading Pass(2) Indexing Queue '%s'", IqFn.c_str());
      // Process the Queue...
      if (MainIndex == NULL)
	{
	  message_log (LOG_PANIC, "MainIndex got lost in core space!?");
	  result = false;
	}
      else
	result = MainIndex->AddRecordList (fp);
      if (DebugMode) message_log (LOG_INFO, "Finished with Pass(2) Indexing Queue");
#if 1
      fclose(fp);
#else
      ffdispose (fp); // Hard close
#endif
      if (DebugMode) message_log (LOG_INFO, "Removing Queue '%s'", IqFn.c_str());
      if (UnlinkFile (IqFn) == -1)
	message_log (LOG_ERRNO, "Could not remove queue file '%s'. Please remove by hand.", IqFn.c_str());
    }
  else
    {
      message_log (LOG_NOTICE, "No (new) records, nothing to do!");
      result = false;
    }
  TotalRecordsQueued = 0;
  MainFpt.CloseAll ();

  AfterIndexing ();

#if _NO_LOCKS
#else
  int lock = readlocked? L_READ| L_WRITE : L_WRITE; 
  if (UnLock(DbFileStem, lock) != lock)
    {
      message_log (LOG_NOTICE|LOG_ERROR, "Can't reset locks for \"%s\".", (const char *)DbFileName);
    }
#endif

   // Added 15 Feb 2004
  MainMdt->FlushMDTIndexes();
  FlushMainRegistry();

  return result;
}

void IDB::ParseRecords(RECORD& FileRecord)
{
  if (DebugMode) message_log (LOG_DEBUG, "IDB::ParseRecords");

  bool needDate         = !FileRecord.GetDate().Ok();
  bool needDateCreated  = !FileRecord.GetDateCreated().Ok();
  bool needDateModified = !FileRecord.GetDateModified().Ok();

  if (needDate || needDateCreated || needDateModified)
    {
      SRCH_DATE       Datum;
      const STRING    fn ( FileRecord.GetFullFileName() );

      if (DebugMode)
	message_log (LOG_DEBUG, "Seeting date metadata for '%s' Record", fn.c_str());

      if (needDate || needDateModified)
	{
	  Datum.SetTimeOfFile(fn);
	  if (Datum.Ok())
	    {
	      if (needDate)         FileRecord.SetDate( Datum );
	      if (needDateModified) FileRecord.SetDateModified (Datum);
	    }
	}
      if (needDateCreated)
	{
	  Datum.SetTimeOfFileCreation(fn);
	  if (Datum.Ok()) FileRecord.SetDateCreated (Datum);
	}
  }

  DOCTYPE_ID  doctype (FileRecord.GetDocumentType());

  DOCTYPE *DocTypePtr = GetDocTypePtr ( doctype );
  if (DebugMode) message_log (LOG_DEBUG, "Back from got pointer");
  if (DocTypePtr)
    {
      if (DebugMode)
	message_log (LOG_DEBUG, "IDB::ParseRecords passing to '%s'", ((STRING)doctype).c_str());
      MainIndex->SetDocTypePtr(DocTypePtr);  // added edz
      DocTypePtr->AddFieldDefs ();
      DocTypePtr->ParseRecords (FileRecord);
    }
  else
    {
      message_log (LOG_DEBUG, "IDB::ParseRecords: No Doctype Pointer");
      DocTypeAddRecord (FileRecord);
    }
}

void IDB::ParseFields (RECORD *Record)
{
  if (Record)
    {
      DOCTYPE *DocTypePtr = GetDocTypePtr ( Record->GetDocumentType () );
      if (DocTypePtr)
	{
	  Record->SetBadRecord(false);
	  DocTypePtr->ParseFields (Record);
#if 0
	  if (__afterParseFieldsCallBack)
	    __afterParseFieldsCallBack(Record);
#endif
	}
      else message_log (LOG_PANIC, "Can't get Document Class pointer for %s",
		Record->GetDocumentType().ClassName(true).c_str());
    }
}

void IDB::SelectRegions(const RECORD& Record, FCT* RegionsPtr)
{
  DOCTYPE *DocTypePtr = GetDocTypePtr( Record.GetDocumentType());
  if (DocTypePtr)
    {
      if (RegionsPtr) RegionsPtr->Clear();
      DocTypePtr->SelectRegions(Record, RegionsPtr);
    }
  else if (RegionsPtr)
    {
      // Select the entire document as one region.
      *RegionsPtr = FC(0, Record.GetLength());
    }
}

// Returns number of words
// Note: We want to include ' in words (always)
size_t IDB::ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const
{
  const char  *base  = Buffer.c_str();
  const char  *ptr   = base;
  const char  *word  = NULL;
  GPTYPE       count = 0;

  while (*ptr) {
    while (IsWordSep(*ptr)) ptr++;
    word = ptr;
    while (*ptr && (IsTermChr(ptr) || *ptr == '\'')) ptr++;
    if (ptr > word)
      {
        if (ListPtr) 
          ListPtr->AddEntry(  WORDSOBJ(STRING(word, ptr-word), word-base));
        count++;
      }
  }
  return count;
}


// This is typically called from the INDEX class..
// Since the words depend upon how they are encoded in the document
// format it needs to pass this off to the doctype handler for the
// records type.
GPTYPE IDB::ParseWords(const DOCTYPE_ID& Doctype, UCHR* DataBuffer,
	GPTYPE DataLength, GPTYPE DataOffset,
	GPTYPE* GpBuffer, GPTYPE GpLength)
{
  // Redirect the call to this method to the appropriate doctype.
  PDOCTYPE DocTypePtr = GetDocTypePtr(Doctype);
  if (DocTypePtr)
    return ( DocTypePtr->ParseWords(DataBuffer, DataLength,
		DataOffset, GpBuffer, GpLength) );
  return 0; // Error
}


bool IDB::IsSystemFile (const STRING& FileName)
{
#ifndef _WIN32
  if (FileName == "core") return true ;
#endif
  // Does the bases match?
  const STRINGINDEX pos = FileName.SearchReverse('.') - 1;
  if (pos == DbFileStem.GetLength()) // Same length..
    {
      // Now really check...
      if (FileName.Compare(DbFileStem, pos))
	{
	  return false;
	}
      const char *tcp = FileName.c_str() + pos;
       // Try to see if its a field table..
      // FCTs have 3 character extensions all digits
      if (_ib_isdigit(tcp[1]) && _ib_isdigit(tcp[2]) && _ib_isdigit(tcp[3]) && tcp[4] == '\0')
	{
	  return true;
	}

      // Try all extensions..
      const STRING ext (tcp); // String comparisons are faster

#ifndef _WIN32
      // Don't want core dumps
      if (ext.Equals(".core")) return true;
#endif

      for (int x = (int)ExtFIRST; x <= (int)ExtLAST; x++)
	{
	  if (ext.Equals(_DbExt((DbExtensions)x)))
	    return true;
	}
      // A index related file?
      if (MainIndex && MainIndex->IsSystemFile(FileName))
	return true;
      // DFDT related file?
      if (MainDfdt &&  MainDfdt->IsSystemFile(FileName))
	return true;
    }
  return false;
}

bool IDB::MergeIndexFiles()
{
  return MainIndex ? MainIndex->MergeIndexFiles() : false;
}

bool IDB::CollapseIndexFiles()
{
  return MainIndex ? MainIndex->CollapseIndexFiles() : false;
}

bool IDB::KillAll ()
{
  SetErrorCode(0);
#if _NO_LOCKS
#else
  // Lock?
  // Check lock
  if (LockWait(DbFileStem, L_WRITE|L_READ))
    {
      SetErrorCode(29);
//    message_log(LOG_NOTICE|LOG_INFO, "Database is locked by another process");
      return false; // Sorry, locked...
    }
#endif
  MainFpt.CloseAll (); // Close All files

  STRING Created;
  MainRegistry->ProfileGetString(DbInfoSection, DateEntry, ISOdate(0), &Created);

  // Announce if we have some old queue garbage.
  { const char msg[] = "%s Queue#%d was not empty.";
  if (FileExists( ComposeDbFn (DbExtIndexQueue1))) message_log (LOG_WARN, msg, DbFileName.c_str(), 1);
  if (FileExists( ComposeDbFn (DbExtIndexQueue2))) message_log (LOG_WARN, msg, DbFileName.c_str(), 2);
  }

  // Zap the Persistant Cache if we have one
  if (!PersistantIrsetCache.IsEmpty() && Exists(PersistantIrsetCache))
    UnlinkFile(PersistantIrsetCache);

  // Delete files
  if (MainDfdt)
    {
      MainDfdt->KillAll(this);
      delete MainDfdt;
    }
  if (MainIndex)
    {
      MainIndex->KillAll();
      delete MainIndex;
      MainIndex = NULL;
    }
  if (MainMdt)
    {
      MainMdt->KillAll();
      delete MainMdt;
      MainMdt = NULL;
    }

/*
  if (DocTypeReg) delete DocTypeReg;
*/

  // Get rid of the rest of the extensions...
  STRING s;
  for (/*enum DbExtensions*/ int x = (int)ExtFIRST; x < (int)ExtLAST; x++)
    {
      s = ComposeDbFn (_DbExt((enum DbExtensions)x));
      // Remove Filtered Data
      if (Exists(s) && UnlinkFile(s) == -1)
	{
	  // if directory root get EISDIR others get EPERM
	  if ((errno == EISDIR || errno == EPERM || errno == ENOTEMPTY) && x == ExtCat)
	    {
	      const char *cwd = GetCwd();
	      int         chdir_res = chdir(s);

	      if (chdir_res != 0)
		{
		   // Try to change the mode
		   if (chmod(s, 0777) == 0) chdir_res = chdir(s);
		}
	      if (chdir_res == 0)
		{
		  DIR *dir = opendir(s.c_str());
		  if (dir)
		    {
		      struct dirent *dp;
		      while ((dp = readdir(dir)) != NULL)
			{
			  if (*(dp->d_name) == '.')
			    /* nothing */;
			  else if (UnlinkFile(dp->d_name) != 0)
			    {
			      if (x == ExtCat)
				{
				  /* Is a directory */
				  DIR *dir2 = opendir(dp->d_name);
				  if (dir2)
				    {
				      /* Is a directory */
				      struct dirent *dp2;
				      STRING         path = AddTrailingSlash (dp->d_name);
				      STRING         tp;
				      while ((dp2 = readdir(dir2)) != NULL)
					{
					  tp = path;
					  tp << dp2->d_name;
					  if (UnlinkFile (tp) != 0 && *(dp2->d_name) != '.')
					    message_log (LOG_ERRNO, "Can't remove '%s' (Filtered data)", tp.c_str());
					}
				      closedir(dir2);
				      if (RmDir (dp->d_name) != 0)
					message_log (LOG_ERRNO, "Can't remove dir '%s'", dp->d_name);
				    }
				  else message_log (LOG_ERRNO, "Can't open '%s' (Filtered data)", dp->d_name);
				}
			      else
				message_log (LOG_ERRNO, "Can't remove '%s'", dp->d_name);
			    }
			}
		      closedir( dir );
		    }
		  else message_log (LOG_ERRNO, "Could not open directory '%s' (Filter data)[2]", s.c_str());
		  if (cwd == NULL)
		    message_log (LOG_ERRNO, "Could not get/restore current working directory!");
		}
	      else message_log (LOG_ERRNO, "Can't access directory '%s' (Filtered data)", s.c_str());
	      if (cwd && chdir(cwd) != 0)
		message_log (LOG_ERRNO, "Can't chdir('%s')", cwd);
	      if (RmDir (s) != 0)
		message_log (LOG_ERRNO, "Can't rmdir '%s' (Filtered data)", s.c_str());
	    }
	  else
	    {
	      message_log(LOG_ERRNO, "Could not remove '%s'", s.c_str());
	    }
	}
    }
  // Get Rid of the Parsing info files
  {
    const STRING catalog ( ComposeDbFn(DbExtDesc) );
    for (size_t y=1;; y++)
      { 
	// See autodetect.cxx !!!!
	s.form("%s:%d", catalog.c_str(), y);
	if (UnlinkFile (s) == -1)
	  {
	    if (Exists(s))
	      message_log(LOG_ERRNO, "Could not remove '%s' (Resource Info)", s.c_str());
	    else
	      break;
	  }
      }
  }
  KillCache(false);

#ifndef _WIN32
  sync(); // Sync file system! (added edz Thu Sep  4 13:40:17 MET DST 1997) 
#endif

  FieldTypes.Clear();

  // Re-init objects

  compatible = true;

  // Index
  MainIndex = new INDEX (this, ComposeDbFn (DbExtIndex) );
  // MDT
  MainMdt = new MDT(MainIndex);
  // DFDT
  MainDfdt = new DFDT ();
/*
  // DTREG
  DocTypeReg = new DTREG (this);
*/

  // Register Isearch Version Number
  MainRegistry->ProfileWriteString(DbInfoSection, VersionEntry, __IB_Version);

  // Register Index Type 
  MainRegistry->ProfileWriteString(DbInfoSection, IndexBitsEntry, STRING(8*sizeof(GPTYPE)));

  // Register Max index
  MainRegistry->ProfileWriteString(DbInfoSection, MaximumRecordSizeEntry, MaximumRecordSize);

  // Register Doctype
  SetGlobalDoctype(NulString);

  // Register Locale
  MainRegistry->ProfileWriteString(DbInfoSection, LocaleEntry, GlobalLocale.LocaleName());
  // Register SegmentName
  MainRegistry->ProfileWriteString(DbInfoSection, SegmentNameEntry, SegmentName);
  // Register Title
  MainRegistry->ProfileWriteString(DbInfoSection, TitleEntry, Title);
  // Register Comments
  if (!Comments.IsEmpty())
    MainRegistry->ProfileWriteString(DbInfoSection, CommentsEntry, Comments);
  // Register Copyright
  if (!Copyright.IsEmpty())
    MainRegistry->ProfileWriteString(DbInfoSection, CopyrightEntry, Copyright);
  // Store global stoplist
  if (!StoplistFileName.IsEmpty())
    MainRegistry->ProfileWriteString(DbInfoSection, StoplistEntry, StoplistFileName); 
  // Register creation/modification time
  if (!Created.IsEmpty())
    MainRegistry->ProfileWriteString(DbInfoSection, DateEntry, Created);
//MainRegistry->ProfileWriteString(DbInfoSection, DateModifiedEntry, Now); 
  MainRegistry->ProfileWriteString(DbInfoSection, DatabasesEntry, DatabaseList);

  MainRegistry->ProfileWriteString(DbInfoSection, WorkingDirEntry, RemoveTrailingSlash(WorkingDirectory));
  if (useRelativePaths)
    MainRegistry->ProfileWriteString(DbInfoSection, useRelativePathsEntry, useRelativePaths);
  if (autoDeleteExpired)
    MainRegistry->ProfileWriteString(DbInfoSection, autoDeleteExpiredEntry, autoDeleteExpired);

  DbInfoChanged = false;
  return true;
}

void IDB::SetDocumentInfo (const INT Index, const RECORD& Record)
{
  MDTREC Mdtrec;
  if (MainMdt->GetEntry (Index, &Mdtrec))
    {
      if (Record.IsBadRecord())
	Mdtrec.SetDeleted ( true ); 
      Mdtrec.SetDate( Record.GetDate() );
      Mdtrec.SetDateCreated ( Record.GetDateCreated() );
      Mdtrec.SetDateModified ( Record.GetDateModified() );
      Mdtrec.SetDateExpires  ( Record.GetDateExpires() );
      Mdtrec.SetKey ( Record.GetKey());

      STRING fullPath (Record.GetFullFileName());
//cerr << "Fullpath = " << fullPath;
      if (useRelativePaths) fullPath = RelativizePathname(fullPath);
//cerr << " --> " << fullPath << endl;
      Mdtrec.SetFullFileName ( fullPath );

      Mdtrec.SetLocalRecordStart (Record.GetRecordStart ());
      Mdtrec.SetLocalRecordEnd (Record.GetRecordEnd ());
      Mdtrec.SetDocumentType ( Record.GetDocumentType ());
      Mdtrec.SetLocale (Record.GetLocale());
    }
  // Do we just ignore the DFT???
  MainMdt->SetEntry (Index, Mdtrec);
}

bool IDB::GetDocumentInfo (const INT Index, RECORD *RecordBuffer) const
{
  MDTREC Mdtrec;
  if (MainMdt && MainMdt->GetEntry (Index, &Mdtrec))
    {
      if (RecordBuffer)
	{
	  RecordBuffer->SetDate (Mdtrec.GetDate());
	  RecordBuffer->SetDateCreated (Mdtrec.GetDateCreated());
	  RecordBuffer->SetDateModified (Mdtrec.GetDateModified());
	  RecordBuffer->SetDateExpires (Mdtrec.GetDateExpires());
	  RecordBuffer->SetKey (  Mdtrec.GetKey () );
	  RecordBuffer->SetPathname ( Mdtrec.GetPathname () );
	  RecordBuffer->SetRecordStart (Mdtrec.GetLocalRecordStart ());
	  RecordBuffer->SetRecordEnd (Mdtrec.GetLocalRecordEnd ());
	  RecordBuffer->SetDocumentType ( Mdtrec.GetDocumentType ());
	  RecordBuffer->SetLocale (Mdtrec.GetLocale());
	  // Here needs to go a call to a function that builds the DFT.
	}
      return true;
    }
  if (RecordBuffer)
    RecordBuffer->Clear();
  return false;
}

bool IDB::GetDocumentDeleted (const INT Index) const
{
  return MainMdt->IsDeleted(Index);
}

bool IDB::DeleteByIndex (const INT Index)
{
  return MainMdt->Delete(Index);
}

bool IDB::UndeleteByIndex (const INT Index)
{
  return MainMdt->UnDelete(Index);
}



bool IDB::DeleteByKey (const STRING& Key)
{
  const size_t x = MainMdt->LookupByKey (Key);
  if (x)
    return DeleteByIndex(x);
  return false;
}

bool IDB::UndeleteByKey (const STRING& Key)
{
  const size_t x = MainMdt->LookupByKey (Key);
  if (x)
    return UndeleteByIndex(x);
  return false;
}

void IDB::MdtSetUniqueKey(RECORD *NewRecord, const STRING& Key)
{
  STRING NewKey (Key);
  MainMdt->GetUniqueKey(&NewKey, Override);
  NewRecord->SetKey(NewKey);
}

// Cleanup and remove deleted records
INT IDB::CleanupDb ()
{
  return MainIndex ? MainIndex->Cleanup () : 0;
}

void IDB::SetGlobalDoctype (const DOCTYPE_ID& NewGlobalDocType)
{
  if (GlobalDoctype != NewGlobalDocType)
    {
      MainRegistry->ProfileWriteString(DbInfoSection, DocTypeEntry, NewGlobalDocType.Get());
      GlobalDoctype = NewGlobalDocType;
      DbInfoChanged = true;
    }
}

const DOCTYPE_ID& IDB::GetGlobalDoctype ()
{
  if (!GlobalDoctype)
    {
      // For multiple doctypes return only the first in the list
      STRLIST StringList;
      MainRegistry->ProfileGetString(DbInfoSection, DocTypeEntry, NulString, &StringList);
      if (!StringList.IsEmpty())
        {
	  STRING  String;
	  StringList.GetEntry(1, &String);
	  GlobalDoctype.Set(String);
	}
    }
  return GlobalDoctype;
}

void IDB::GetGlobalDocType (PSTRING StringBuffer) const
{
  *StringBuffer = GlobalDoctype.DocumentType();
}

void IDB::SetServerName(const STRING& ServerName)
{
  MainRegistry->ProfileWriteString(DbInfoSection, "IP-Name", ServerName);
  DbInfoChanged = true;
}

// Return hostname[:port]
// The Optional :port is ONLY when IP-Name is not defined and
// when running under HTTPD
STRING IDB::GetServerName() const
{
  STRING ServerName;
  // IP-Name overrides SERVER_NAME
  MainRegistry->ProfileGetString(DbInfoSection, "IP-Name", NulString, &ServerName);
  if (ServerName.IsEmpty())
    {
      // Don't have an IP-Name so get one..
      char buf[120];
      const char *host = getenv ("SERVER_NAME"); // Running under HTTPD?
      if (host == NULL || *host == '\0' || strchr(host, '.') == NULL)
	{
          // Nope.. So get hostname
          if (getHostname (buf, sizeof (buf) / sizeof (char) - 1) != -1)
	    ServerName = buf;
	  else
	    ServerName = "127.0.0.1"; // Loopback
	}
      else
	ServerName = host;

      // Only look for the pot if the server name was defined
      if (host && *host)
	{
	  char *port = getenv("SERVER_PORT");
	  if (port && *port && 80 != atoi(port))
	    {
	      ServerName.Cat (':');
	      ServerName.Cat (port);
	    }
	}
    }
  return ServerName;
}


void IDB::SetGlobalStoplist (const STRING& NewStoplist) 
{
  if (StoplistFileName != NewStoplist)
    {
      StoplistFileName = NewStoplist;
      MainRegistry->ProfileWriteString(DbInfoSection, StoplistEntry, NewStoplist);
      DbInfoChanged = true;  
      SetStoplist(StoplistFileName);
    }
} 

STRING IDB::GetGlobalStoplist () const
{
  return StoplistFileName;
}

size_t IDB::Scan(PSTRLIST ListPtr, const STRING& FieldName,
        const STRING& Term, const INT TotalTermsRequested) const
{
  if (FieldName.GetLength() && 0 == MainDfdt->GetFileNumber(FieldName))
    {
      return 0;
    }
  return  MainIndex->Scan(ListPtr, FieldName, Term, TotalTermsRequested);
}

size_t IDB::Scan(PSTRLIST ListPtr, const STRING& FieldName,
	const size_t Position, const INT TotalTermsRequested) const
{
  if (FieldName.GetLength() && 0 == MainDfdt->GetFileNumber(FieldName))
    return 0;
  return  MainIndex->Scan(ListPtr, FieldName, Position, TotalTermsRequested);
}

size_t IDB::ScanGlob(PSTRLIST ListPtr, const STRING& FieldName,
        const STRING& Term, const INT TotalTermsRequested) const
{
  if (FieldName.GetLength() && 0 == MainDfdt->GetFileNumber(FieldName))
    return 0;
  return  MainIndex->ScanGlob(ListPtr, FieldName, Term, TotalTermsRequested);
}



void IDB::AddTemplate (const STRING& foo)
{
  if (TemplateTypes.SearchCase (foo) == 0)
    TemplateTypes.AddEntry (foo);
}

void IDB::MergeTemplates ()
{
  // Load the Templates
  PFILE Fp = fopen(ComposeDbFn (DbExtTemplate), "r");
  if (Fp)
    {    
      STRING temp;
      while (temp.FGet (Fp, 256))
        AddTemplate(temp);
      fclose(Fp);
    }
}


#if 0
void IDB::SetDebugSkip(const INT Skip)
{
  DebugSkip = Skip;
}
#endif

size_t IDB::DeleteExpired()
{
  SRCH_DATE now;

  now.SetNow();
  return DeleteExpired(now);
}

size_t IDB::DeleteExpired(const SRCH_DATE Now)
{
  if (MainMdt == NULL)
    return 0;
  if (Now.Ok())
    {
      size_t         expired = 0;
      const size_t   total = MainMdt->GetTotalEntries() ;
        
      for (size_t i=1; i<= total; i++)
	{
	  const MDTREC *ptr = MainMdt->GetEntry (i);
	  if (ptr == NULL || ptr-> TTL(Now) == 0)
	    {
	      if (MainMdt->Delete(i))
		expired++;
	    }
	}
      return expired;
    }
  return DeleteExpired();
}


void IDB::FlushMainRegistry()
{
  message_log (LOG_DEBUG, "FlushMainRegistry");

//cerr << "MainRegistry = " << (long)MainRegistry << endl;
  if (IsDbCompatible () && MainRegistry)
    {
      if ((MainMdt && MainMdt->GetChanged ()) || (MainDfdt && MainDfdt->GetChanged ()) || DbInfoChanged)
	{
#ifndef _WIN32 
	  // Register maintainer
	  if (MaintainerName.IsEmpty() || MaintainerMail.IsEmpty())
	    {
	      STRING name, address;
	      _IB_GetmyMail(&name, &address);
	      SetMaintainer(name, address);
	    }
#endif  /* UNIX */
	  // Register Isearch Version Number
	  MainRegistry->ProfileWriteString(DbInfoSection, VersionEntry, __IB_Version);
          MainRegistry->ProfileWriteString(EngineSection, VersionIDEntry, GetVersionID());
	  MainRegistry->ProfileWriteString(EngineSection, BuildDateEntry,  SRCH_DATE(__DATE__).ISOdate());

	  // Register Index Type
	  MainRegistry->ProfileWriteString(DbInfoSection, IndexBitsEntry, STRING(8*sizeof(GPTYPE)));
	  // Register Locale
	  MainRegistry->ProfileWriteString(DbInfoSection, LocaleEntry, GlobalLocale.LocaleName());
	  // Register modification time
	  MainRegistry->ProfileWriteString(DbInfoSection, DateModifiedEntry, ISOdate(0));
	  PFILE Fp = ComposeDbFn (DbExtDbInfo).Fopen("w");
	  if (Fp)
	    {
	      MainRegistry->Write(Fp);
	      fclose(Fp);
	    }
	}
    }
}

// Destructor
IDB::~IDB ()
{
  Close();
}

bool IDB::Close()
{
//cerr << "** IDB Close " << endl;

  if (DbFileStem.IsEmpty() && MainRegistry == NULL && MainIndex == NULL && DocTypeReg == NULL)
    return false; // Already closed

//cerr << "Close " << DbFileStem << endl;
 
  message_log (LOG_DEBUG, "IDB::Close %s", DbFileStem.c_str());
  if (SortIndexFp)
    {
      ffclose(SortIndexFp);
      SortIndexFp = NULL;
    }

  Queue1Add = Queue2Add = 0;
//cerr << "Destroy"  << endl;
  if (MetaDefaults)
    {
      delete MetaDefaults;
      MetaDefaults = NULL;
    }
#if 0
  if (DebugMode)
    {
      MainMdt->Dump ();
      MainIndex->Dump (DebugSkip);
    }
#endif
//cerr << "Delete MainRegistry" << endl;
  if (MainRegistry)
    {
      const size_t total = MainMdt ? MainMdt->GetTotalEntries () : 0;
      if (total > 10)
	{
	  STRING mirror;
	  MainRegistry->ProfileGetString("Mirror", "Root", NulString, &mirror);
	  if (mirror.IsEmpty()) // Lets see if its a mirror?
	    {
		  MDTREC mdtrec;
		  int    pos;
		  size_t i = total/3;
		  STRING oldName;
		  while (i<total && (i < (total/3+50)))
		    {
		      if (MainMdt->GetEntry (i++, &mdtrec))
			{
			  if (oldName == mdtrec.GetFullFileName() && oldName.Search("http") == 0)
			    break; // 
			  mirror = mdtrec.GetPath();
			  if ((pos = mirror.SearchReverse("/http/")) > 0)
			    {
			      mirror.EraseAfter(pos);
			      message_log (LOG_DEBUG, "Testing for spider root='%s'", mirror.c_str());
			      break;
			    }
			}
		    }
		  for (i=total/10 + 1; i< (27+total/10) && i < total; i++)
		    {
		      MainMdt->GetEntry (i, &mdtrec);
		      if (strncmp(mdtrec.GetPath(), mirror, mirror.GetLength()) != 0)
			{
			  break;
			}
		    }
		  if (i == total)
		    {
		      message_log (LOG_INFO, "Looking like an index of a spider crawl: Root='%s'",
			mirror.c_str());
		      SetMirrorBaseDirectory(mirror);
		    }
	    }
	}
      FlushMainRegistry();
#if 0
      MainRegistry->PrintSgml( ComposeDbFn (".xml") );
#endif
      delete MainRegistry;
      MainRegistry = NULL;
    }
//cerr << "Delete MainIndex" << endl;
  if (MainIndex)
    {
      delete MainIndex;
      MainIndex = NULL;
    }

  // Dump the Templates
  if (!TemplateTypes.IsEmpty())
    {
      MergeTemplates (); // Load what we have 
      // Dump
      ofstream out (ComposeDbFn (DbExtTemplate).c_str());
      for (const STRLIST *p = TemplateTypes.Next(); p != &TemplateTypes; p = p->Next() )
	out << p->Value() << endl;
    }

//cerr << "Delete MainMdt" << endl;
  if (MainMdt)
    {
      delete MainMdt;
      MainMdt = NULL;
    }

//cerr << "Delete MainDfdt" << endl;
  if (MainDfdt)
    {
      if (MainDfdt->GetChanged ())
	MainDfdt->SaveTable ( ComposeDbFn (DbExtDfd) );
      delete MainDfdt;
      MainDfdt = NULL;
    }
//cerr << "Delete DocTypeReg" << endl;
  if (DocTypeReg)
    {
      delete DocTypeReg;
      DocTypeReg = NULL;
    }
//cerr << "OK" << endl;

  MainFpt.CloseAll ();

  IndexingStatus (IndexingStatusClose, DbFileStem);

  DbFileStem.Clear(); // Closed!

#ifdef DEBUG_MEMORY
  if (DebugMode)
    {
      message_log (LOG_DEBUG, "RESULTs (globally) still allocated: %ld", __IB_RESULT_allocated_count);
      message_log (LOG_DEBUG, "IRESULTs (globally) still allocated: %ld", __IB_IRESULT_allocated_count);
      message_log (LOG_DEBUG, "FCTs (globally) still allocated: %ld // %ld FCLIST itmes", __IB_FCT_allocated_count,   __IB_FCLIST_allocated_count);
    }
#endif


  UnLock(DbFileStem, L_WRITE); // Unlock

//cerr << "** CLOSED" << endl;
  return true;
}

INT IDB::GetVolume(PSTRING StrBufferPtr) const
{
  if (StrBufferPtr)
    *StrBufferPtr = Volume.GetName();
  return Volume.GetVolume();
}

void IDB::SetVolume(const STRING &Dbname, INT vol)
{
  Volume.SetName(Dbname);
  Volume.SetVolume(vol);
}

STRING IDB::DbName() const
{
  return Volume.GetVolume() ? Volume.GetName() : DbFileStem;
}

// example ?TEST+ or ?TEST+1@
STRING IDB::URLfrag() const
{
  STRING DBname, Buffer;
  const INT volNr = Volume.GetVolume();

  if (volNr)
    DBname = Volume.GetName();
  else
    DBname = DbFileStem;

  RemovePath(&DBname); // Strip path

  Buffer << "?" << DBname << "+";
  if (volNr)
    Buffer << (INT)volNr << "@";
  return Buffer;
}

// Get the URL fragement for the record
// example ?TEST+1@1234 or ?TEST+1234
STRING IDB::URLfrag(const RESULT& ResultRecord) const
{
  return URLfrag() + ResultRecord.GetKey();
}


void IDB::SetDbState(enum DbState DbState) const
{
  FILE* fp = ComposeDbFn(DbExtDbState).Fopen("wb");
  if (fp)
    {
      ::Write((INT4)DbState, fp);
      fclose(fp);
    }
}


enum DbState IDB::GetDbState() const 
{
  FILE* fp = ComposeDbFn(DbExtDbState).Fopen("rb");
  if (fp)
    {
      INT4 DbState;
      ::Read(&DbState, fp);
      fclose(fp);
      return (enum DbState)DbState;
    }
  return DbStateReady;
}



/////////////////////////////////////////////////////////

IDBVOL::IDBVOL()
{
  Volume = 0;
}

IDBVOL::IDBVOL(const STRING& dbname, INT volume)
{
  Volume = volume;
  DbName = dbname;
}

// assigns one to another
IDBVOL& IDBVOL::operator =(const IDBVOL& Other)
{
  Volume = Other.Volume;
  DbName = Other.DbName;
  return *this;
}



///////////////////////////////////////////////
/// IDB Error Codes
///////////////////////////////////////////////

int IDB::SetErrorCode(int Error)
{
  //int old_err = errorCode;
  errorCode = Error;
  return errorCode;
}

int IDB::GetErrorCode() const
{
  return errorCode;
}

const char *IDB::ErrorMessage() const
{
  return ErrorMessage(errorCode);
}

const char *IDB::ErrorMessage(int ErrorCode) const
{
  switch (ErrorCode) {
    case -64: return "32-bit indexes are not compatible with 64-bit libs.";
    case -32: return "64-bit indexes are not compatible with 32-bit libs. ";
    case 0:   return "No Error";
    case 1:   return "Permanent system error";
    case 2:   return "Temporary system error";
    case 3:   return "Unsupported search";
    case 4:   return "Terms only exclusion (stop) words";
    case 5:   return "Too many argument words";
    case 6:   return "Too many boolean operators";
    case 7:   return "Too many truncated words";
    case 8:   return "Too many incomplete subfields";
    case 9:   return "Truncated words too short";
    case 10:  return "Invalid format for record number (search term)";
    case 11:  return "Too many characters in search statement";
    case 12:  return "Too many records retrieved";
    case 13:  return "Present request out of range";
    case 14:  return "System error in presenting records";
    case 15:  return "Record no authorized to be sent intersystem";
    case 16:  return "Record exceeds Preferred-message-size";
    case 17:  return "Record exceeds Maximum-record-size";
    case 18:  return "Result set not supported as a search term";
    case 19:  return "Only single result set as search term supported";
    case 20:  return "Only ANDing of a single result set as search term supported";
    case 21:  return "Result set exists and replace indicator off";
    case 22:  return "Result set naming not supported";
    case 23:  return "Combination of specified databases not supported";
    case 24:  return "Element set names not supported";
    case 25:  return "Specified element set name not valid for specified database";
    case 26:  return "Only a single element set name supported";
    case 27:  return "Result set no longer exists - unilaterally deleted by target";
    case 28:  return "Result set is in use";
    case 29:  return "One of the specified databases is locked";
    case 30:  return "Specified result set does not exist";
    case 31:  return "Resources exhausted - no results available";
    case 32:  return "Resources exhausted - unpredictable partial results available";
    case 33:  return "Resources exhausted - valid subset of results available";
    case 100: return "Unspecified error";
    case 101: return "Access-control failure";
    case 102: return "Security challenge required but could not be issued - request terminated";
    case 103: return "Security challenge required but could not be issued - record not included";
    case 104: return "Security challenge failed - record not included";
    case 105: return "Terminated by negative continue response";
    case 106: return "No abstract syntaxes agreed to for this record";
    case 107: return "Query type not supported";
    case 108: return "Malformed query (unspecified)";
    case 109: return "Database unavailable";
    case 110: return "Operator unsupported";
    case 111: return "Too many databases specified";
    case 112: return "Too many result sets created";
    case 113: return "Unsupported attribute type";
    case 114: return "Unsupported Use attribute";
    case 115: return "Unsupported value for Use attribute";
    case 116: return "Use attribute required but not supplied";
    case 117: return "Unsupported Relation attribute";
    case 118: return "Unsupported Structure attribute";
    case 119: return "Unsupported Position attribute";
    case 120: return "Unsupported Truncation attribute";
    case 121: return "Unsupported Attribute Set";
    case 122: return "Unsupported Completeness attribute";
    case 123: return "Unsupported attribute combination";
    case 124: return "Unsupported coded value for term";
    case 125: return "Malformed search term";
    case 126: return "Illegal term value for attribute";
    case 127: return "Unparsable format for un-normalized value";
    case 128: return "Illegal result set name";
    case 129: return "Proximity search of sets not supported";
    case 130: return "Illegal result set in proximity search";
    case 131: return "Unsupported proximity relation";
    case 132: return "Unsupported proximity unit code";
    default:  return "Unknown Error";
  }
}


// Something for the INDEX:: class Not()
FCT IDB::GetFieldFCT (const MDTREC& mdtrec, const STRING& FieldName)
{
  STRING DfFileName;
  FCT fct;

  if (DfdtGetFileName (FieldName, &DfFileName))
    {
      FILE *fp = ffopen(DfFileName, "rb");
      if (fp != NULL)
	{
	  const GPTYPE gStart =  mdtrec.GetGlobalFileStart ();

	  const GPTYPE GpStart = gStart + mdtrec.GetLocalRecordStart ();
	  const GPTYPE GpEnd   = gStart + mdtrec.GetLocalRecordEnd ();
	  const FC Fc (GpStart, GpEnd);

	  off_t Pos = OnDiskFcSearch(Fc, fp);
	  if (Pos)
	    {
		FC Fc2;
		GPTYPE FcStart = 0;
		// work backwards to find first pair
		while (--Pos >= 0)
		  {
		    if (-1 != fseek (fp, Pos * sizeof (FC), SEEK_SET))
		      {
			Fc2.Read (fp);
			FcStart = Fc2.GetFieldStart();
			if (FcStart < GpStart) break;
		      }
		  }

//		if (Pos >= 0) Fc2.Read (fp); // Fiexed???? 25 OCT 2003?

		STRING readBuffer;
		// Now collect all the data into a list
		for(;Fc2.GetFieldEnd() <= GpEnd; FcStart = Fc2.GetFieldStart()) {
		  if (FcStart >= GpStart)
		    {
		      fct.AddEntry( Fc2 );
		    }
		  if (feof(fp)) // End of file?
		    break;
		  Fc2.Read (fp);
		}
	    }
	  
	  ffclose (fp);
	}
      else
	message_log (LOG_ERRNO, "Could not open %s (%s)", DfFileName.c_str(), FieldName.c_str());
    }
  else
    message_log (LOG_DEBUG, "Field %s not available", FieldName.c_str());
  return fct;
}

//
// [External Sort]
// 1=filename path

bool IDB::BeforeSortIndex(int Which)
{
  if (Which != ActiveSortIndex)
    {
      STRING filename;

      if (SortIndexFp)
	{
	  message_log (LOG_DEBUG, "Offloading external sort #%d", ActiveSortIndex);
	  ffclose(SortIndexFp); // Close Old
	  SortIndexFp = NULL;
	}

      ProfileGetString(externalSortSection, Which, NulString, &filename);
      if (filename.IsEmpty())   
	{
	  filename.form("%s.__%d", DbFileStem.c_str(), Which);
	  if (!FileExists(filename))
	    {
	      message_log (LOG_DEBUG, "Default '%s' for external sort #%d not found", filename.c_str(), Which);
	      filename.Clear();
	    }
	}
       if (filename.IsEmpty())
	 message_log (LOG_WARN, "No External Sort is available for #%d", Which);
      else if (!FileExists(filename))
	message_log (LOG_ERROR, "External Sort for #%d = '%s' does not exist!", Which, filename.c_str());
      else if ((GetFileSize(filename)/sizeof(SORT_INDEX_ID)) < GetTotalRecords())
	message_log (LOG_ERROR, "External Sort for #%d (='%s') too short.", Which, filename.c_str());
      else if ((SortIndexFp = ffopen(filename, "rb")) == NULL)
	message_log (LOG_ERRNO, "Could not open external sort '%s'", filename.c_str());
      else
	message_log (LOG_DEBUG, "External sort %d loaded (%p)", filename.c_str(), Which, SortIndexFp);
      ActiveSortIndex = Which;
    }
  else message_log (LOG_DEBUG, "Reuse open SortIndex handle: %d (%p)", ActiveSortIndex, SortIndexFp);
  return (SortIndexFp != NULL);
}

bool IDB::AfterSortIndex(int Which)
{
  if (Which == ActiveSortIndex || Which <= 0)
    {
      if (SortIndexFp) ffclose(SortIndexFp); // Close
      SortIndexFp = NULL;
      ActiveSortIndex = -1; // Bye Bye
      return true;
    }
  return false;
}

bool IDB::SetSortIndexes(int Which, atomicIRSET *Irset)
{
  // Aren't a child of VIDB? Then we need/should handle it all..
  if (Parent == NULL)
    {
      BeforeSortIndex(Which);

      const size_t TotalEntries = Irset->GetTotalEntries();
      for (size_t i=1; i<=TotalEntries; i++)
	Irset->SetSortIndex(i, GetSortIndex(Which,  Irset->GetIndex(i)));

      AfterSortIndex(Which);
      return true;
    }
 else // Father knows best
    return Parent->SetSortIndexes(Which, Irset);
}

SORT_INDEX_ID IDB::GetSortIndex(int Which, INDEX_ID index_id)
{
  SORT_INDEX_ID sort;
  if (ActiveSortIndex == -1 && Which != ActiveSortIndex)
    BeforeSortIndex(Which);
  if (Which != ActiveSortIndex)
    message_log (LOG_PANIC, "Resource deadlock on SortIndex process. Want #%d but #%d is running.", Which, ActiveSortIndex);
  else sort.Set(SortIndexFp, index_id);
  return sort; 
}


#if 1
STRING IDB::XMLHitTable(const RESULT& Result)
{
  STRING XML;
  MDTREC mdtrec;
  const char *endl = "\n";

  if ( MainMdt == NULL || MainMdt->GetEntry (Result.GetMdtIndex(), &mdtrec) == false)
    {
      message_log (LOG_ERROR, "IDB::XMLHitTable Can't resolve record!");
      return NulString;
    } 
  int offset = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();

  FCT HitTable ( Result.GetHitTable() );
  size_t z = HitTable.GetTotalEntries();

  if (z)
    {
      const FCLIST *hitList = (const FCLIST *)HitTable;
      FC            Fc;
      STRING        Tag;
      STRING        lastTag;
      FC            lastPeerFC;
      bool   fulltext = false;

      bool firstTime = true;
      XML << "<HITS UNITS=\"characters\" NUMBER=\"" << z << "\">" << endl;
      for (const FCLIST* ptr = hitList->Next(); ptr != hitList; ptr = ptr->Next())
        {
	  Fc = ptr->Value();
          GPTYPE Start = Fc.GetFieldStart();
          GPTYPE End   = Fc.GetFieldEnd();
	  FC     PeerFC = GetPeerFc(FC(Fc)+=offset,&Tag);

	      if (! (PeerFC == lastPeerFC))
		{
		  if (! firstTime)
		    XML << ( lastTag.GetLength() ? "  </CONTAINER>" : "  </FULLTEXT>" ) << endl;
		  else
		    firstTime = false;
		  if (Tag.GetLength())
		    {
			FIELDTYPE ft = GetFieldType(Tag);
			STRING    value;

			fulltext = false;
			XML << "  <CONTAINER NAME=\"" << Tag
				<< "\" TYPE=\"" << ft.c_str()
				<< "\" FC=\"(" << PeerFC.GetFieldStart() << ","
				<< PeerFC.GetFieldEnd() << ")";
			if (!ft.IsText() && GetFieldData(PeerFC, Tag, &value)) 
			  XML << "\" VALUE=\"" << value;
			XML << "\">" << endl;
		    }
		  else
		    {
		      XML << "  <FULLTEXT>" << endl;
		      fulltext = true;
		    }
		  lastPeerFC = PeerFC;
		  lastTag    = Tag;
		}
          XML << "    <LOC POS=\"" << Start << "\" LEN=\"" << End-Start+1 << "\"/>" << endl;
        } // for(;;)
      if (lastTag.GetLength())
	XML << "  </CONTAINER>" << endl;
      else if (fulltext)
	XML << "  </FULLTEXT>" << endl;
      XML << "</HITS>" << endl;
    }
  return XML;
}
#endif


PIRSET IDB::SearchSmart(QUERY *Query, const STRING& DefaultField)
{
  PIRSET pIrset = NULL;
  if (Query)
    {
      SQUERY Squery;
      pIrset = SearchSmart(*Query, DefaultField, &Squery);
      Query->Squery = Squery;
    }
  return pIrset;
}

PIRSET IDB::SearchSmart(const QUERY& Query, SQUERY *SqueryPtr) 
{ 
  return SearchSmart(Query, NulString, SqueryPtr);

} 

PIRSET IDB::SearchSmart(const QUERY& Query, const STRING& DefaultField, SQUERY *SqueryPtr)
{
  PIRSET pIrset = NULL;
  QUERY  nQuery (Query);
  SQUERY squery = nQuery.Squery;
  STRING QueryString;


  if (squery.isPlainQuery(&QueryString) == false)
    {
      if (SqueryPtr) *SqueryPtr = squery;
      return Search(nQuery);
    }

  const size_t terms = Query.GetTotalTerms();
  if (terms == 1)
    return Search(nQuery);

   // Search as literal phrase?
   if (terms >= 2)
    {
      nQuery.Squery.SetLiteralPhrase(QueryString);
     
      if ((pIrset = Search(nQuery)) != NULL)
	{
	  if (pIrset->GetTotalEntries() == 0)
	    {
	      delete pIrset;
	      pIrset = NULL; // Nothing found
	    }
	}
    }
  if (pIrset == NULL)
    {
      bool res;
      STRING      field (DefaultField);
      // Search as Peer

      nQuery.SetSQUERY(QueryString); // 2023
      if (field.Trim(STRING::both).IsEmpty())
	res = nQuery.Squery.SetOperatorPeer();
      else
	res = nQuery.Squery.SetOperatorAndWithin(field);

      if (res)
	{
	  if ((pIrset = Search(nQuery)) != NULL)
	    {
	      if (pIrset->GetTotalEntries() == 0)
		{
		  delete pIrset;
		  pIrset = NULL; // Nothing found
		}
	    }
	  // Search
	  if (pIrset == NULL)
	    {
	      // nQuery.SetSQUERY(QueryString); // 2023
	      nQuery.Squery.SetOperatorOr();
	      if ((pIrset = Search(nQuery)) != NULL)
		{
		  size_t total = pIrset->GetTotalEntries();
		  pIrset->Reduce(terms);
		  if (pIrset->GetTotalEntries() != total)
		    nQuery.Squery.PushReduce(terms);
		}
	    }
	}
	else message_log (LOG_WARN, "Ill formed search expression: %s", QueryString.c_str());
    }
  if (SqueryPtr) *SqueryPtr = nQuery.Squery;
  return pIrset;
}
