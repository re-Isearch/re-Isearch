/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

#include "platform.h"

#define EVALULATION 0
static long timeout = -1; // Expires Fri Jan 19 09:00:00 2001

/*-@@@
File:		iindex_main.cxx
Version:	4.0
Description:	Command-line indexer
@@@-*/

static const int _iindex_main_version = 3;

/*-
  TODO:

Write a indexing log including
  - The options, doctype specified etc.
  - The filename
  - The MD5 hash of the file file

When someone does a delete
  - Log the operation

Tool:
  - Go through the log and reindex everything

*/

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#if defined(_MSDOS) || defined(_WIN32) 
# include <direct.h> 
# ifdef O_BUILD_IB64
#  define REG "Iindex64.reg"
# else
#  define REG "Iindex.reg"
# endif
#else 
# include <unistd.h> 
# ifdef O_BUILD_IB64
#  define REG ".Iindex64_reg"
# else
#  define REG ".Iindex_reg"
# endif
#endif
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
//#include <sys/file.h>
#include "common.hxx"
#include "vidb.hxx"
#include "string.hxx"
#include "record.hxx"
#include "dtreg.hxx"
#include "lock.hxx"
#ifdef HAVE_LOCALE
#include <locale.h>
#endif
#include "lang-codes.hxx"
#include "thesaurus.hxx"
#include "process.hxx"

#ifndef NO_MMAP
#include "mmap.hxx"
#endif

#include "stoplist.hxx"

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC    1000000
#endif

#ifdef _WIN32
# define SHOW_RUSAGE 1
# ifndef HAVE_SYSLOG
#   define HAVE_SYSLOG 0
# endif
#endif
#ifndef SHOW_RUSAGE
# define SHOW_RUSAGE 1
#endif
#ifndef HAVE_SYSLOG
# define HAVE_SYSLOG 1
#endif

#if SHOW_RUSAGE
#ifndef _WIN32
# include <sys/resource.h>
#endif
#endif

#ifndef DEFAULT_BROWSER
#define DEFAULT_BROWSER "firefox"
#endif

class IDBC:public IDB
{
  public:
  IDBC (const STRING &DBname, const STRLIST& NewDocTypeOptions): IDB (DBname, NewDocTypeOptions)
  {
  };
  IDBC (const STRING & NewPathName, const STRING& NewFileName, const STRLIST& NewDocTypeOptions):
    IDB (NewPathName, NewFileName, NewDocTypeOptions)
  {
  };
protected:
  void IndexingStatus (const t_IndexingStatus StatusMessage, const STRING& FileName,
		       const long arg)
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
	  message_log (LOG_INFO, "Parsing records ...");
	  break; 
	case IndexingStatusParsingRecord:
	  message_log (LOG_DEBUG, "Parsing %s ..", FileName.c_str());
          break;
	case IndexingStatusIndexingDocument:
	  message_log (LOG_INFO, "Indexing %s ...", FileName.c_str());
	  break;
	case IndexingStatusIndexing:
	  message_log (LOG_INFO, "Adding %d words to index ...", arg);
	  break;
        case IndexingStatusRecordAdded:
          message_log (LOG_DEBUG, "Index Record Nr.%d created ('%s')", arg, FileName.c_str());
          break;
	case IndexingStatusFlushing:
	  message_log (LOG_INFO, "Writing index ...");
	  break;
	case IndexingStatusMerging:
	  message_log (LOG_INFO, "Merging indexes (%d indices)...", arg);
	  break;
      }
  };
};

typedef IDBC *PIDBC;

static STRING Separator;
static bool include_sep = true;
static STRING DocumentType;

static LOCALE Locale;
static IDBC *pdb = NULL;
static off_t Start = 0;
static off_t End = 0;

static int addFile(const STRING& Fn)
{
  if (pdb == NULL)
    return -1;
  off_t From = Start, To = End;

  if (pdb->IsSystemFile(Fn)) {
    message_log (LOG_INFO, "Skipping file '%s' (System file)", Fn.c_str());
    return -1;
  }

  message_log (LOG_DEBUG, "Adding '%s' to indexer queue", Fn.c_str());

  RECORD Record (Fn);

  Record.SetDocumentType (DocumentType);
  Record.SetLocale (Locale);
  if (Separator.IsEmpty())
    {
      if ((From < 0) || (To < 0) || (To == 0))
        {
          const off_t FileSize = GetFileSize(Fn);
          if (FileSize <= 5)
            {
              message_log (LOG_NOTICE, "Skipping %s (contains %s%ld byte%s)", Fn.c_str(),
		FileSize > 0 ? "only " : "",
		FileSize,
		FileSize != 1 ? "s" : "");
              return 0; // Nothing to do...
            }
	  if (To   <= 0)  To  += FileSize-1;
          if (From <  0) From += FileSize-1;
        }
      Record.SetRecordStart (From);
      Record.SetRecordEnd (To);
      if (pdb->AddRecord (Record) == false)
	return -1;
    }
  else
    {
	       message_log (LOG_INFO, "Parsing for sep '%s'", Separator.c_str());
		unsigned matches = 0;
		MMAP MemoryMap(Fn, MapSequential);

 		if (!MemoryMap.Ok()) {
 			return -1;
 		}
 		off_t FileSize = MemoryMap.Size();
		if (From >= FileSize) {
#ifdef _WIN32
		  message_log (LOG_ERROR|LOG_FATAL, "Starting position %lu exceeds file size %lu. Can't continue!",
			(unsigned long)From, (unsigned long)FileSize);
#else
		  message_log (LOG_ERROR|LOG_FATAL, "Starting position %lld exceeds file size %lld. Can't continue!",
			(long long)From, (long long)FileSize);
#endif
		  return -1;
		}
		if (From < 0)     {From += FileSize;}
		if (To <= 0)      {To   += FileSize;}
		if (To <= From) {
#ifdef _WIN32
		  message_log (LOG_ERROR|LOG_FATAL, "Starting position %lu is after ending position %lu.",
			(unsigned long)From, (unsigned long)To) ;
#else
		  message_log (LOG_ERROR|LOG_FATAL, "Starting position %lld is after ending position %lld.",
			(long long)From, (long long)To);
#endif
		  return -1;
		}
		FileSize -= From; // Decrement size by From offset 
 		CHR* Buffer =  (char *)MemoryMap.Ptr() + From;

 		Record.SetRecordStart(0);
 		CHR* Position = Buffer;
 		CHR* Found;
 		bool Done;
 		CHR* EndOfBuffer = Buffer + (To - From); // FileSize;
 		const CHR* Sep = Separator.c_str();
 		CHR SepChar = Sep[0];
 		GPTYPE Offset;
 		const size_t SepLength = Separator.GetLength();
		const size_t soffset = (include_sep ? 0 : SepLength);
		do {
	 		Done = false;
 			while (Done == false) {
 				while ( (Position < EndOfBuffer) &&
 						(*Position != SepChar) ) {
 					Position++;
 				}
 				if (Position >= EndOfBuffer) {
 					Done = true;
 					Found = 0;
 				} else {
 					if ((Position + SepLength) <= EndOfBuffer) {
						// was strncmp()
 						if (memcmp(Sep, Position, SepLength) == 0) {
  							Found = Position;
  							Done = true;
  						}
  					}
  				}
  				if (Done == false) {
  					Position++;
  				}
  			}
  			if (Found) {
				matches++;
  				Offset = (GPTYPE)(Found - Buffer);
				// the separator marks the beginning of the next 
				// record. (offset - 1), then marks the end of 
				// the current record. we must make sure that the
				// end of the current record is past the beginning 
				// of the current record.

				// BUGFIX: Offset can be ZERO!
  				if (Offset>0 && ((Offset-1) > Record.GetRecordStart())) {
  					Record.SetRecordEnd(Offset - 1);
  					pdb->AddRecord(Record);
  					Record.SetRecordStart(Offset + soffset);
  					Position = Found + SepLength;
  				} else {
  					Position++;
  				}
  			}
  		} while (Found);

  		if ((FileSize - 1) > Record.GetRecordStart()) {
  			Record.SetRecordEnd(FileSize - 1);
  			pdb->AddRecord(Record);
  		}
		if (matches == 0)
			message_log (LOG_WARN, "\
Seperator pattern %s not found in file %s. Passing whole file (%ld kb) to [%s].",
	Separator.c_str(), Fn.c_str(), FileSize/1024, DocumentType.c_str());
    }
  return 0;
}

static void sig_size(int sig)
{
  signal (sig, SIG_IGN);
  message_log(LOG_PANIC, "File size capacity exceeded (see getrlimit(2)), Index process aborted.");
  if (pdb) delete pdb;
  exit (sig);
};

static void sig_cpu(int sig)
{
  signal (sig, SIG_IGN);
  message_log(LOG_PANIC, "CPU time limit exceeded (see getrlimit(2)), Index process aborted.");
  if (pdb) delete pdb;
  exit (sig);
};


static void sig_sys(int sig)
{
  signal (sig, SIG_IGN);
  message_log(LOG_PANIC, "Caught a bad system call! Index process aborted.");
  if (pdb) delete pdb;
  exit (sig);
}

static void seg_fault (int sig)
{
  void (*func)(int) = signal (sig, SIG_IGN);
  message_log(LOG_PANIC, "Caught a signal (%d).", sig);
  signal (sig, func);
  message_log(LOG_NOTICE, "Shutting down....");
  signal (sig, SIG_DFL);
  if (pdb) delete pdb;
  pdb = NULL;
  exit (-1);
}

static void sig_int (int sig)
{
  signal (sig, SIG_IGN);
  const char *name = (sig == SIGTERM ? "Terminate" : "Interrupt");

  message_log(LOG_FATAL, "Index process aborted by %s signal (#%d).", name, sig);
  if (pdb) delete pdb;
  exit (-1);
}

#define ISO_8859_1 1

extern size_t GetLangCodes(char ***codes, char ***values);
extern size_t GetLocaleCodes(char ***codes, char ***values);

static STRING prognam;

static void HelpLanguage();
static void HelpLocale();
static void HelpTypes();
static void Usage();
static void IniUsage();

extern "C" {
  int _Iindex_main(int argc, char **argv);
}

int _Iindex_main (int argc, char **argv)
{
  int   quiet = 0;
  const char *argv0 = argv[0];

#ifdef WIN32
//  WSADATA wsa;
//  WSAStartup(MAKEWORD(1,1),&wsa);
#endif   

#ifdef HAVE_LOCALE
  {
    char *locale = getenv("LC_CTYPE");
    if (locale == NULL || *locale == 'C')
      {
	putenv((char *)"LC_CTYPE=iso_8859_1");
      }
  }
  setlocale (LC_ALL, "");
#endif

  if (argv0 == NULL)
    argv0 = "Iindex";

  prognam = RemovePath(argv0);
  timeout = __Register_IB_Application(argv0,  stdout, false);
  if (argc < 2)
    {
      std::cout << "IB indexer " <<  _iindex_main_version  << "." << SRCH_DATE(__DATE__).ISOdate()  << "." << __IB_Version << " " <<
	sizeof(GPTYPE)*8 << "-bit edition (" << __HostPlatform << ")";
#if EVALULATION
      if (timeout > 0) std::cout << " Try-and-Buy.";
#endif
      std::cout << endl << __CopyrightData  << endl << endl;
      Usage();
    }

  STRLIST DocTypeOptions;
  STRING Flag;
  STRING DBName;
  STRING FileList;
  STRING Title;
  STRING Copyright;
  STRING Comment;
  t_merge_status Merge=iMerge;
  size_t      SisLimit = 0;
  off_t        common_words = 0;
  INT         MaximumRecordSize = 0;
  bool DebugFlag = false;
  bool _signals  = true;
  bool _core_dump= false;
  bool Override = true;
  bool Recursive = false;
  bool autoRecursive = false;
  bool Follow    = false;
  bool AppendDb = false;
  bool Synonyms = false;
  bool createCentroid = false;
  bool useRelativePaths = false;
  STRING      basePath;
  bool useIndexPath = false;

  STRING  SynonymFileName;
  STRLIST  includeStrlist;
  STRLIST  excludeStrlist;
  STRLIST  inclDirlist;
  STRLIST  excludeDirlist;
  const char *homeDirectory = NULL;

  STRING Stoplist =  "<NULL>";
  UINT4 MemoryUsage = 0;
  bool ForceMem = false; 
  const char *Lang = NULL;
  INT x = 0;
  INT LastUsed = 0;

  LOCALE NewLocale;
  bool CharsetSet = false, LanguageSet = false;


  while (x < argc)
    {
      if (argv[x][0] == '-')
	{
	  Flag = argv[x];
	  if (Flag.Equals ("-o"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No option specified after -o.");
		  return 2;
		}
	      STRING S;
	      S = argv[x];
	      DocTypeOptions.AddEntry (S);
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-d"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No database name specified after -d.");
		  return 2;
		}
	      DBName = argv[x];
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-setuid"))
	    {
             if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No User-id/Name specified after %s.", Flag.c_str());
                  return 2;
                }
	      if (!SetUserName( argv[x]) )
		{
		  message_log (LOG_WARN|LOG_ERRNO, "Could not set user-id '%s'.", argv[x]);
		}
              LastUsed = x;
	    }
          else if (Flag.Equals ("-setgid"))
            {
             if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No User-id/Name specified after %s.", Flag.c_str());
                  return 2;
                }
	     if (!SetUserGroup( argv[x]) )
                {
                  message_log (LOG_WARN|LOG_ERRNO, "Could not set group-id '%s'.", argv[x]);
                }
              LastUsed = x;
            }
	  else if (Flag.Equals ("-T"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No database title specified after -T.");
		  return 2;
		}
	      Title = argv[x];
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-C"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No comment specified after -C.");
                  return 2;
                }
              Comment = argv[x];
              LastUsed = x;
            }
	  else if (Flag.Equals ("-R"))
	    {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No copyright specified after -R.");
                  return 2;
                }
              Copyright = argv[x];
              LastUsed = x;
	    }
	  else if (Flag.Equals ("-f"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No file name specified after -f.");
		  return 2;
		}
	      FileList = argv[x];
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-t") || Flag.Equals("-dt"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No document type name specified after %s.", Flag.c_str());
		  return 2;
		}
	      DocumentType = argv[x];
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-centroid"))
	   {
	     createCentroid = true;
	     LastUsed = x;
	   }
	  else if (Flag.Equals ("-stop"))
	    {
	      if (Lang)
		{
		  Stoplist = Lang;
		}
	      else
		{
		  const char *tcp = Locale2Lang( getenv("LANG") );
		  if (tcp)
		    Stoplist = tcp;
		}
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-l"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No stoplist name specified after -l.");
		  return 2;
		}
	      Stoplist = argv[x];
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-serial_id"))
	    {
	      std::cout << "Serial: " << _IB_SerialID() << endl;
	      std::cout << "Hostid: " << _IB_Hostid()  << endl;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-license_timeout"))
	    {
#if EVALULATION
	      if (timeout > 0)
		std::cout << "Please update before " << ISOdate(timeout) << endl;
#endif
	      return timeout <= 0 ? 0 : (int)timeout;
	    }
#if EVALULATION 
	  else if (Flag.Equals("-license"))
	    {
	      if (timeout > 0)
		std::cout << "License will expire " << RFCdate(timeout) << endl;
	      else
		std::cout << "Full license installed for Host (#" << (UINT4)_IB_Hostid() << ")!" << endl;
	      LastUsed = x;
	    }
#endif
	  else if (Flag.Equals("-copyright"))
            {
              LastUsed = x;
	      //extern const char * const __CopyrightData;
	      std::cout << "Copyright Statement:\n--------------------\n" << __CopyrightData << endl << endl << endl;
	    }
	  else if (Flag.Equals ("-s") || Flag.Equals("-sep") || Flag.Equals("-xsep"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No separator string specified after -%s.", Flag.c_str());
		  return 2;
		}
	      Separator = argv[x];
	      if (!Flag.Equals("-s"))
		Separator.unEscape ();
	      include_sep = (Flag != "-xsep");
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-start"))
	    {
	      if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No position specified after %s.", Flag.c_str());
                  return 2;
                }
	      Start = strtol (argv[x], NULL, 10);
	      LastUsed = x;
	    }
          else if (Flag.Equals("-end"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No position specified after %s.", Flag.c_str());
                  return 2;
                }
              End = strtol (argv[x], NULL, 10);
              LastUsed = x;
            }
	  else if (Flag.Equals("-override"))
	    {
	      Override = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-no-override"))
            {
              Override = false;
              LastUsed = x;
            }
          else if (Flag.Equals("-mdt"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No record usage specified after %s.", Flag.c_str());
                  return 2;
                }
              _IB_MDT_SEED = strtol (argv[x], NULL, 10);
              LastUsed = x;
            }
	  else if (Flag.Equals ("-cd") || Flag.Equals("-chdir") || Flag.Equals("-cwd"))
	   {
	    if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No directory specified after %s.", Flag.c_str());
                  return 2;
                }
	     const char *path = argv[x];
	     if (! DirectoryExists(path))
	       {
		  message_log (LOG_ERROR, "Usage: directory specified in %s \"%s\" does not exist or is not accessible.",
			Flag.c_str(), path);
	       }
	    else
	      homeDirectory = path;
	     LastUsed = x;
	   }
	  else if (Flag.Equals ("-ds") || Flag.Equals("-sis"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No number advisory specified after %s.", Flag.c_str());
                  return 2;
                }
              SisLimit = (size_t)strtol (argv[x], NULL, 10);
              LastUsed = x;
            }
	  else if (Flag.Equals ("-syn") || Flag.Equals("-thes"))
	    {
	      if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No file specified after %s.", Flag.c_str());
                  return 2;
                }
	      SynonymFileName = argv[x];
	      message_log (LOG_DEBUG, "Thes = %s", SynonymFileName.c_str());
	      Synonyms = 1;
	      LastUsed = x;
	    }
          else if (Flag.Equals ("-common"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No number specified after %s.", Flag.c_str());
                  return 0;
                }
              common_words = atol(argv[x]);
              LastUsed = x;
            }
	  else if (Flag.Equals ("-m") || Flag.Equals("-mem") || Flag.Equals("-memory"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No memory usage specified after %s.", Flag.c_str());
		  return 2;
		}
	      MemoryUsage = strtol (argv[x], NULL, 10);
	      ForceMem = Flag.Equals("-memory");
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-lang") ||
		(Flag.Equals("-help") && argv[x+1] && strncasecmp(argv[x+1], "lang", 4) == 0) )
	    {
	      INT LanguageCode = 0;
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No language specified after -lang.");
		  return 2;
		}
	      if (strlen(argv[x]) < 2)
		{
		  message_log (LOG_FATAL, "Illegal language code %s.", argv[x]);
		  return 2;
		}
	      if (strncasecmp(argv[x], "help", 4) == 0 || strncasecmp(argv[x], "lang", 4) == 0)
		{
		  HelpLanguage();
		  return 1;
		}
	      Lang = argv[x];
	      if ((LanguageCode = Lang2Id (Lang)) == 0)
		{
		  message_log (LOG_WARN, "Document language code '%s' undefined", Lang);
		  LanguageCode = Lang2Id ("und"); // Set undefined
		}
	      message_log(LOG_DEBUG, "Assuming document language '%s' (%d)", Id2Language(LanguageCode), LanguageCode);
	      NewLocale.SetLanguage(LanguageCode);
	      LanguageSet = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-charset"))
	    {
	      BYTE CharsetCode = 0xFF;
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No charset specified after -charset.");
                  return 2;
                }
              if ((CharsetCode = Charset2Id (argv[x])) == 0xFF)
                {
                  CharsetCode = Charset2Id (NULL);
                  message_log (LOG_WARN, "Unknown charset '%s', using '%s'",
                        argv[x], Id2Charset (CharsetCode) );
                }
	      if (CharsetCode == 0xFF)
		CharsetCode = Charset2Id("Latin-1");
	      if (CharsetCode != 0xFF)
		{
		  message_log (LOG_DEBUG, "Using %s (%d) character set.", Id2Charset(CharsetCode), CharsetCode);
		  NewLocale.SetCharset(CharsetCode);
		  CharsetSet = true;
		}
              LastUsed = x;
	    }
	  else if (Flag.Equals ("-locale"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No locale specified after -locale.");
		  return 2;
		}
	      if (strncasecmp(argv[x], "help", 4) == 0)
                {
		  HelpLocale();
                  return 1;
                }
	      NewLocale = argv[x];
	      CharsetSet = LanguageSet = true;
	      LastUsed = x;
	    }
          else if (Flag.Equals ("-sort") || Flag.Equals("-qsort"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No algorithm name specified after -%d.", Flag.c_str());
                  return 2;
                }
	      switch (*(argv[x]))
		{
		  case 'b': case 'B': _IB_Qsort =  BentleyQsort; break;
		  case 's': case 'S': _IB_Qsort =  SedgewickQsort; break;
		  case 'd': case 'D': _IB_Qsort =  DualPivotQsort; break;
		}
              LastUsed = x;
            }
	  else if (Flag.Equals ("-level"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No level specified after -level.");
		  return 2;
		}
	      log_init((int)(strtol (argv[x], NULL, 10) & 0xFF));
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-log") || Flag.Equals("-e"))
	    {
	      if (Flag.Equals("-e"))
		log_init(LOG_PANIC|LOG_FATAL|LOG_ERROR|LOG_ERRNO);
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No log file specified after %s.", Flag.c_str());
		  return 2;
		}
	      if ((argv[x])[0] == '-' && (argv[x])[0] == '\0')
		log_init(stderr);
	      else
		log_init("", argv[x]);
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-syslog"))
	    {
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: No facility specified after %s.", Flag.c_str());
		  return 2;
		}
	      if (set_syslog(argv[x]) == false)
		message_log (LOG_ERROR, "Unknown syslog facility '%s' specified (-%s).", argv[x], Flag.c_str());
	      LastUsed = x; 
	    }
	  else if (Flag.Equals ("-fast"))
	    {
	      Merge = iNothing;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-O"))
	    {
	      Merge = iOptimize;
	      MemoryUsage = (UINT4)-1;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-Z"))
	    {
	      Merge = iMerge;
	      MemoryUsage = (UINT4)-1;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-optimize") || Flag.Equals("-optimise"))
	    {
	      Merge = iOptimize;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-merge"))
	    {
	      Merge = iMerge;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-a"))
	    {
	      AppendDb = true;
	      Merge = iNothing;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-add") || Flag.Equals ("-append"))
	    {
	      AppendDb = true;
	      Merge = iOptimize;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-incr"))
	    {
	      AppendDb = true;
	      Merge = iIncremental;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-collapse"))
	    {
	      Merge = iCollapse;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-recursive"))
	    {
	      Recursive = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-r"))
	    {
	      Recursive = true;
	      autoRecursive = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-follow"))
	    {
	      Follow = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-include"))
	    {
	      if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No pattern specified after %s.", Flag.c_str());
                  return 2;
                }
	      includeStrlist.AddEntry ( argv[x] );
	      LastUsed = x;
	    }
          else if (Flag.Equals("-exclude"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No pattern specified after %s.", Flag.c_str());
                  return 2;
                }
              excludeStrlist.AddEntry (argv[x]);
              LastUsed = x;
            }
          else if (Flag.Equals("-incldir"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No pattern specified after %s.", Flag.c_str());
                  return 2;
                }
              inclDirlist.AddEntry (argv[x]);
              LastUsed = x;
            }
          else if (Flag.Equals("-excldir"))
            {
              if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No pattern specified after %s.", Flag.c_str());
                  return 2;
                }
              excludeDirlist.AddEntry (argv[x]);
              LastUsed = x;
	    }
	  else if (Flag.Equals ("-name"))
	    {
	      if (++x >= argc)
		{
                  message_log (LOG_FATAL, "Usage: No pattern specified after %s.", Flag.c_str());
		  return 2;
		}
	      Recursive = true;
	      includeStrlist.AddEntry ( argv[x] );
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-abs") || Flag.Equals ("-absolute_paths"))
	   {
	      useRelativePaths = false;
	      LastUsed = x;
	   }
	  else if (Flag.Equals ("-rel") ||  Flag.Equals("-relative_paths"))
	    {
	      if (Flag.Equals("-rel"))
		useIndexPath = true;
	      useRelativePaths = true;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-base"))
	    {
	      useRelativePaths = true;
	      if (++x >= argc)
		{
		  message_log (LOG_FATAL, "Usage: no path specified after %s.", Flag.c_str());
		  return 2;
		}
	      basePath = argv[x];
	      if (!DirectoryExists(basePath))
		message_log (LOG_WARN, "Specified base path directory '%s' does not exist on this machine.", basePath.c_str());
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-quiet"))
	    {
	      quiet++;
	      log_init (LOG_FATAL|LOG_ERROR|LOG_ERRNO|LOG_WARN|LOG_NOTICE);
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-silent") || Flag.Equals ("-q"))
	    {
	      quiet++;
	      log_init (0);
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-nomax"))
	   {
	      MaximumRecordSize = -1;
	      LastUsed = x;
	   }
	  else if (Flag.Equals("-maxsize"))
	    {
             if (++x >= argc)
                {
                  message_log (LOG_FATAL, "Usage: No number specified after %s.", Flag.c_str());
                  return 0;
                }
              MaximumRecordSize = (INT)atol(argv[x]);
              LastUsed = x;
	    }
          else if (Flag.Equals ("-kernel"))
	    {
	      LastUsed = x;
	      cout
		<< "Max file handles: "
		<< _IB_kernel_max_file_descriptors() << endl
		<< "Max file streams: "
		<< _IB_kernel_max_streams()<<endl;
	    }
	  else if (Flag.Equals ("-capacities"))
	    {
	       cout << "Physical Index Capacities for this " << sizeof(GPTYPE)*8 << "-bit edition:" << endl <<

#ifdef  O_BUILD_IB64
			"  Max Input:   " << ((MAX_GPTYPE)/(1024UL*1024UL*1024UL*1024ULL*1024ULL)+1) << " Tbytes (Total of all files)" << endl <<

#else
		       "  Max Input:   <" << (((MAX_GPTYPE)/(1024UL*1024UL*1024UL))+1) << " Gbytes (Total of all files)" << endl <<
#endif
	               "  Max Words:   " << ((MAX_GPTYPE)/(2*sizeof(GPTYPE)*1024LL*1024LL*1000LL*1000LL)) << " trillion (optimized), >" <<
				((MAX_GPTYPE)/(2UL*1024UL*1024UL*1000UL*1000UL)) << " trillion (non-optimized)" << endl <<
		       "  Max Unique:  " << ((MAX_GPTYPE)/(1024L*1024L*2*1000ULL*1000ULL*(StringCompLength + sizeof(GPTYPE)+1))) <<
			                   " trillion words (optimized), >" <<
				((MAX_GPTYPE)/(2ULL*1024ULL*1024ULL*1000ULL*1000ULL)) << " trillion (non-optimized)" << endl <<
		       "  Word Freq:   Maximum same as \"Max Words\"" << endl <<
		       "  Max Records: " << (((_index_id_t)-1) / (2*1024L*1024L*sizeof(MDTREC))) << " million." << endl <<
		       "  Min. disk requirements: some fixed and variable amounts plus each" << endl <<
                       "    record (" << sizeof(MDTREC) << "); word (" << sizeof(GPTYPE) << "); unique word (~"<< (DefaultSisLength+1+2*sizeof(GPTYPE)) << "); field (" << sizeof(FC) << ")." << endl <<
		       "Virtual Database Search Capacities:" << endl <<
		       "  Max Input:   aprox. " << (((MAX_GPTYPE)/(1024L*1024L*(1024L/VolIndexCapacity)))/(1024UL)) / 1024.0 
				<< " Terabyte(s) " << endl <<
		       "  Max Words:   unlimited (limit only imposed by physical index)" << endl <<
		       "  Max Unique:  unlimited (limit only imposed by physical index)" << endl <<
		       "  Max Indexes: " << VolIndexCapacity << endl <<
                       "  Max Records: " << (MdtIndexCapacity/(1024L*1024L)) << " million (Total all indexes)" << endl <<
		       "Preset Per-Record Limits:" << endl <<
#if !USE_MDTHASHTABLE
                       "  Max Path:    " << MaxDocPathNameSize << " characters" << endl <<
#endif
		       "  Max Key:     " << DocumentKeySize << " characters" << endl << 
		       "  Max Doctype Child Names: " << DocumentTypeSize << " characters" << endl;
		if (sizeof(size_t) == 4) cout <<
		       "INX Chunk max: ~2 GB (32-bit kernel limitations)" << endl; 
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-api"))
	    {
	      cout << "API " << __IB_Version << "/" << (DoctypeDefVersion & 0xFFF)
		<< "  built with: " << __CompilerUsed  << " (" <<  __HostPlatform << ")" << endl;
	      LastUsed = x;
	      if (x + 1 == argc) return 0;
	    }
	  else if (Flag.Equals ("-version") || Flag.Equals("-v"))
	    {
	      char tmp[64];
	      if (!_IB_GetPlatformName(tmp, sizeof(tmp)))
		tmp[0] = '\0';
	      cout << "IB indexer v" <<  _iindex_main_version  <<  "." << SRCH_DATE(__DATE__).ISOdate()  << "."
		<< __IB_Version << " " << sizeof(GPTYPE)*8 << "-bit edition (" << __HostPlatform << ")" ;
	      if (tmp[0])
		cout << "  [" << tmp << "]";
	      cout << endl;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-help"))
	    {
	      LastUsed = 0;
	      if (x < argc && argv[x+1] != NULL && *argv[x+1])
		{
		  size_t len = strlen(argv[x+1]);
		  if (len > 4) len = 4;
		  if (strncmp(argv[x+1], "doctypes", len) == 0)
		    {
		      LastUsed = ++x; 
		      goto print_class_list;
		    }
		  if ( strncmp(argv[x+1], "options", len) == 0)
		    {
		      IniUsage();
		      LastUsed = x;
		    }
		  if (strncmp(argv[x+1], "lang", len) == 0)
		    {
		      HelpLanguage();
		      LastUsed = x;
		    }
		  if (strncmp(argv[x+1], "types", len) == 0)
		    {
		      HelpTypes();
		      LastUsed = x;
		    }
		  if (strncmp(argv[x+1], "locale", len) == 0)
		    {
		      if (LastUsed)
			cout << endl;
		      HelpLocale();
		      LastUsed = x;
		    }
		}
	      if (LastUsed == 0)
		{
		  Usage();
		  LastUsed = x;
		}
	       else
		{
		  if (++x > argc) break;
		  LastUsed = x;
		}
	    }
	  else if (Flag.Equals ("-thelp"))
    {
      if ((x+1) < argc && isalnum(*(argv[x+1])) ) {
	PrintDoctypeHelp(argv[++x]);
      } else {
print_class_list:


	PrintDoctypeList();
        cout << "Format Documentation: " <<  _IB_HTDOCS_HOME << "[DOCTYPE].html" << endl <<
	"  Example: " <<  _IB_HTDOCS_HOME << "MAILFOLDER.html" << endl << endl;

      	cout << "Usage Examples:"<< endl
   		<< "  " << prognam << " -d POETRY *.doc *.txt" << endl
   		<< "  " << prognam << " -d SITE -t MYHTML:HTMLHEAD -r /var/html-data" << endl
   		<< "  find /public/htdocs -name \"*.html\" -print | " << prognam << "-d WEB -t HTMLHEAD -f -" << endl
   		<< "  " << prognam << " -d DVB -include \"*.dvb\" -locale de_DE -recursive /var/spool/DVB" << endl
   		<< "  " << prognam << " -d WEB -name \"*.[hH][tT][mM]*\" -excldir SCCS /var/spool/mirror" << endl
   		<< endl;
	}
      cout << endl;
      LastUsed = x;
    }
#ifndef _WIN32
	  else if (Flag.Equals ("-xhelp"))
	    {
	      char    *browser = getenv("WWW_BROWSER");
	      STRING   s;
	      STRING   index_html = ResolveHtdocPath("Welcome.html", true);
	      if (browser == NULL || *browser == '\0')
		{
		  if (!IsAbsoluteFilePath(s=ResolveBinPath("www") ))
		    s=ResolveBinPath( DEFAULT_BROWSER );

		  browser = (char *)s.c_str();
		  message_log (LOG_INFO, "Environment variable WWW_BROWSER not set. \
Using '%s' as default.", browser);
		}
	      char *gargv[3];
	      gargv[0] = browser;
	      gargv[1] = (char *)index_html.c_str();
	      gargv[2] = NULL;
	      if (_IB_system (gargv, 1) < 0)
		message_log (LOG_ERRNO, "Could not run '%s'", browser);
	      LastUsed = x;
	    }
#endif
	  else if (Flag.Equals ("-verbose"))
	    {
	      log_init (LOG_ALL);
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-core"))
	    {
	      _signals = true;
	      _core_dump = true;
	    }
	  else if (Flag.Equals ("-trap"))
	    {
	      _signals = false;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-debug"))
	    {
//cerr << "DEBUG Gla" << endl;
	      DebugFlag = true;
	      __Register_IB_Application(argv0, stdout, DebugFlag);
	      log_init (LOG_ALL);
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-erase"))
	    {
	      message_log (LOG_FATAL, "Usage: -erase no longer supported, use Iutil.");
	      return 2;
	    }
	}
      x++;
    }

  x = LastUsed + 1;

  if ((DebugFlag == false || _core_dump == true) && _signals == true)
    {
#ifdef SIGSEGV
      signal (SIGSEGV, seg_fault);
#endif
#ifdef SIGBUS
      signal (SIGBUS, seg_fault);
#endif
      signal (SIGINT, sig_int);
      signal (SIGTERM, sig_int);
#ifdef SIGSYS
      signal (SIGSYS, sig_sys);
#endif
#ifdef SIGXFSZ
      signal (SIGXFSZ, sig_size);
#endif
#ifdef SIGXCPU
      signal (SIGXCPU, sig_cpu);
#endif
    }

  INT NumFiles = argc - x;
  INT z = x;

  if (FileList.IsEmpty() && (NumFiles == 0))
    {
      message_log (LOG_FATAL, "Usage: No files specified for indexing!");
      return 0;
    }

  if ((!FileList.IsEmpty()) && (NumFiles != 0))
    {
      message_log (LOG_FATAL, "Usage: Unable to handle -f and file names at the same time.");
      return 2;
    }

  message_log (LOG_NOTICE, "%s (Iindex) %s (%s)", prognam.c_str(), __IB_Version,  __DATE__ );

  if (DBName.IsEmpty())
    {
      DBName = __IB_DefaultDbName;
      message_log (LOG_WARN, "No database name specified: Using '%s'.", DBName.c_str());
//    cout << "ERROR: No database name specified!" << endl;
//    return 0;
    }

  if (DocumentType.IsEmpty())
    {
      DocumentType = "AUTODETECT";
      message_log(LOG_INFO, "No doctype specified, using Auto-detection (%s).", DocumentType.c_str());
    }
  else
    {
      STRINGINDEX pos = DocumentType.Search('=');
      if (pos)
	{
	  message_log (LOG_WARN, "Document class was specified using the obsolete '%s' convention (pos=%d).", DocumentType.c_str(), pos);
	  DocumentType.SetChr(pos, ':');
	  message_log (LOG_NOTICE, "Using %s as -dt class", DocumentType.c_str());
	}
    }
  if (!WritableDir(DBName))
    {
      message_log (LOG_FATAL, "Don't have read/write permissions to write to '%s'.",
	PATHNAME(DBName). GetPath().c_str());
      return -1;
    }

  //__Register_IB_Application(argv0,  stdout, DebugFlag);;
  if (timeout > 0)
    __IB_CheckUserRegistration(REG);

  //-------------------------------------------------------
#ifndef _WIN32
  while (Lock (DBName, L_APPEND) != L_APPEND)
    {
      if (errno == EACCES)
	message_log (LOG_FATAL, "Don't have read/write permissions for database '%s'", DBName.c_str());
#ifdef _WIN32
      else message_log (LOG_FATAL, "Datebase '%s' is currently locked", DBName.c_str());
      return -2;
#else
      sleep(60); // Wait untill ready...
#endif
    }
#endif

  //-----------------------------------------

  pdb = new IDBC (DBName, DocTypeOptions);

  if (pdb == NULL)
    {
      message_log(LOG_PANIC, "%s open failed", DBName.c_str());
      exit(-1);
    }
  else if (!pdb->ValidateDocType(DocumentType))
    {
      DTREG dtreg (0);
      message_log (LOG_ERROR, "Document type %s is not available (v%.2f).",
	DocumentType.c_str(), dtreg.Version()/1000.0);
      delete pdb;
      return 3;
    }
  if (basePath.GetLength())
    pdb->SetWorkingDirectory(basePath);

  if (!AppendDb)
    {
      if (!pdb->KillAll ())
	{
	  message_log (LOG_ANY, "The specified database is locked. Try again later...");
	  message_log (LOG_INFO, "The database is being rebuilt by another re-indexing process.");
	  delete pdb;
	  return 4;
	}
    }
  else
    {
      if (!pdb->IsDbCompatible ())
	{
	  message_log (LOG_ANY, "The specified database is not compatible with this version of %s.", prognam.c_str());
	  message_log (LOG_INFO , "You cannot append to a database created with a different version.");
	  delete pdb;
	  return 5;
	}
    }
  if (common_words > 0) pdb->SetCommonWordsThreshold(common_words);
  if (MaximumRecordSize) pdb->SetMaximumRecordSize ( MaximumRecordSize );
  if (SisLimit) pdb->SetDbSisLimit(SisLimit);
  if (DebugFlag) pdb->DebugModeOn ();

  pdb->setUseRelativePaths(useRelativePaths);

  if ((LanguageSet || CharsetSet ) && !(LanguageSet && CharsetSet))
    {
      Locale = pdb->GetLocale();
      if (!CharsetSet)
	NewLocale.SetCharset(Locale.Charset());
      if (!LanguageSet)
	NewLocale.SetLanguage(Locale.Language());
    }

  if ((INT)NewLocale != 0 && !pdb->SetLocale(NewLocale))
    message_log (LOG_ERROR, "Could not set Locale '%s'", Locale.LocaleName().c_str()); 
  Locale = pdb->GetLocale();

  message_log(LOG_INFO, "Indexing using character set '%s'", Locale.GetCharsetName());
  message_log(LOG_INFO, "Assuming default document language '%s'", Locale.GetLanguageName());
  if (!Title.IsEmpty())
    pdb->SetTitle(Title);
  if (!Copyright.IsEmpty())
    pdb->SetCopyright(Copyright);
  if (!Comment.IsEmpty())
    pdb->SetComments (Comment);

  // Set Global Stoplist to match -l if there isn't already one
  if (!Stoplist.IsEmpty())
    {
      if (Stoplist == "-")
	Stoplist=NulString;
      if ( AppendDb )
	pdb->SetStoplist (Stoplist);
      else
	pdb->SetGlobalStoplist (Stoplist);
    }

  if (Override)
    pdb->SetOverride(Override);

  if (Synonyms) {
    THESAURUS MyThesaurus;

    message_log (LOG_INFO, "Building THESAURUS from %s", SynonymFileName.c_str());
    MyThesaurus.Compile(SynonymFileName, DBName, true);
  }

  if (homeDirectory && chdir(homeDirectory) == -1)
    message_log (LOG_ERRNO, "Could not chdir to \"%s\".", homeDirectory);

  if (FileList.IsEmpty())
    {
      message_log (LOG_INFO, "Building document list ...");

      if (includeStrlist.IsEmpty() && autoRecursive)
	{
	  if (DocumentType.SearchAny("HTML"))
	    includeStrlist.AddEntry ( "*.[hH][tT][mM]*" );
	  else if (DocumentType.SearchAny("PDF"))
	    includeStrlist.AddEntry ( "*.[pP][dD][fF]");
	  else if (DocumentType.SearchAny("XML"))
	    includeStrlist.AddEntry ( "*.[xX][mM][lL]");
	  else if (DocumentType.SearchAny("SGML"))
	    includeStrlist.AddEntry ( "*.[sS][gG][mM]*");
	  else if (DocumentType.SearchAny("MSDOC"))
	    includeStrlist.AddEntry ( "*.[dD][oO][dC]");
	  else if (DocumentType.SearchAny("MSPPT"))
	    includeStrlist.AddEntry ( "*.[pP][pP][tT]");
	  else if (DocumentType.SearchAny("MSEXCEL"))
	    includeStrlist.AddEntry ( "*.[xX][lL][sS]");
	  else if (DocumentType.SearchAny("TEXT"))
	    includeStrlist.AddEntry ( "*.txt");
	  else if (DocumentType.SearchAny("BIBTEX"))
	    includeStrlist.AddEntry ( ".btx");
	}
      for (z = 0; z < NumFiles; z++)
	{
	  ::do_directory(argv[z + x], addFile,
		&includeStrlist, &excludeStrlist, &inclDirlist, &excludeDirlist,
		Recursive, Follow);
	}
    }
  else
    {
      message_log (LOG_INFO, "Reading document list ...");
      // Stdin or file
      PFILE fp = (FileList == "-" ? stdin : fopen (FileList, "r"));
      char s[BUFSIZ];
      if (!fp)
	{
	  message_log (LOG_ERRNO, "Can't find file list (-f).");
	  delete pdb;
	  return -1;
	}
      while (fgets (s, sizeof(s)/sizeof(char) - 1, fp) != NULL)
	{
	 if (s[0] == ';')
	   {
	     // Throw out a comment....
	     char *ptr = &s[1];
	     while (isspace(*ptr)) ptr++;
	     if (*ptr)
		message_log (LOG_INFO, "%s", ptr);
	     continue;
	   }
	  for (char *ptr = &s[strlen(s) -1]; isspace(*ptr) && ptr >= s; ptr--)
	    *ptr = '\0';
	  if (s[0])
	    addFile( s );
        }
      if (fp != stdin) fclose (fp);
    }
  message_log (LOG_INFO, "%s database %s",  AppendDb ? "Adding to" : "Building", DBName.c_str());

  if (MemoryUsage != 0)
    {
      pdb->SetIndexingMemory (MemoryUsage, ForceMem);
    }
  pdb->SetMergeStatus(Merge);

  long start_total_words  = pdb->GetTotalWords();
  long start_unique_words = pdb->GetTotalUniqueWords();
  long start_records      = pdb->GetTotalRecords ();

  time_t start_seconds    = time(NULL);
  clock_t start_time      = clock();

  if (!pdb->Index ())
    {
      message_log (LOG_NOTICE, "Indexing error encountered: %s", pdb->ErrorMessage());
    }
  clock_t final_time      = clock();
  time_t  final_seconds   = time(NULL);
  long final_total_words  = pdb->GetTotalWords();
  long final_unique_words = pdb->GetTotalUniqueWords();
  long final_records      = pdb->GetTotalRecords ();

  if (useIndexPath && basePath.IsEmpty()) pdb->ClearWorkingDirectoryEntry();

  if (createCentroid)
    pdb->CreateCentroid();

  const int indexingMemoryMB = (int) ((long long)pdb->GetIndexingMemory() + 512L*1024L)/(1024L*1024L);

  delete pdb;
  pdb = NULL;
  message_log (LOG_INFO, "Database files saved to disk.");

#ifndef _WIN32
  UnLock (DBName, L_APPEND);
#endif

  if ( (double)(final_seconds - start_seconds) > 2147)
    {
      message_log (LOG_NOTICE, "Added %ld words (%ld unique) in %ld records in %u min. [indexed %.2f words/s, %.2f records/s (Real)]",
        (long)(final_total_words - start_total_words),
        (long)(final_unique_words - start_unique_words),
	(long)(final_records - start_records),
	(unsigned int)(0.5 + (final_seconds-start_seconds)/60.0), 
        (final_total_words - start_total_words)/(double)(final_seconds - start_seconds),
	(final_records - start_records)/(double)(final_seconds - start_seconds) );
    }
  else if (final_time - start_time > 1)
    {
#if 1
     const double secs_used  = (double)final_time/CLOCKS_PER_SEC - (double)start_time/CLOCKS_PER_SEC;
     const double fdiv       = 60.0/secs_used /* min. used */;
#else
      const double factor    = sqrt(CLOCKS_PER_SEC);
      const double fdiv      = 60.0*factor/((final_time/factor) - (start_time/factor));
#endif
      const long words_min   = (long)((final_total_words - start_total_words)*fdiv
				+ 0.5);
      const long recs_min    = (long)((final_records - start_records)*fdiv + 0.5);

      message_log (LOG_NOTICE, "Added %ld words (%ld unique) in %ld records in %d sec. [indexed %ld%swords/min, %ld records/min (CPU), %dMB Memory]",
	final_total_words - start_total_words,
	final_unique_words - start_unique_words,
	final_records - start_records,
	final_seconds-start_seconds,
	(words_min > 1024L*1024L) ? words_min/1024L : words_min,
	(words_min > 1024L*1024L) ? "k " : " ",
	recs_min,
	indexingMemoryMB);
    }
  else
    message_log (LOG_NOTICE, "Added %ld words (%ld unique) in %ld records [< %d seconds (Real), %dMB Memory]",
        final_total_words - start_total_words,
        final_unique_words - start_unique_words,
	final_records - start_records,
        final_seconds - start_seconds,
	indexingMemoryMB);

#if SHOW_RUSAGE

  if (quiet) return 0;

  struct rusage rusage;
  int    ru = RUSAGE_SELF;
rusage:
  if (getrusage(ru, &rusage) == 0)
    {
      long double val = 0, cpu_time = rusage.ru_utime.tv_sec +
                rusage.ru_utime.tv_usec/1000000.0 +
		rusage.ru_stime.tv_sec + rusage.ru_stime.tv_usec/1000000.0;
#ifdef _WIN32
      int ticks = 1;
#else
      long ticks = sysconf(_SC_CLK_TCK) ;
#endif
      if (rusage.ru_utime.tv_usec || cpu_time ||  rusage.ru_maxrss) {

      int elapsed = final_seconds - start_seconds;
      cerr << endl << endl << ((ru == RUSAGE_SELF) ? "Main" : "Subprocess" ) << " Job Statistics:" << endl;
	// tv_sec // tv_usec;   
	if (ru == RUSAGE_SELF)
	  cerr << "Elapsed \"real\" time:    " << elapsed << " seconds" << endl;
      cerr << "CPU time:               " << cpu_time << " seconds" << endl <<
	"    User time:          " << ((val = (rusage.ru_utime.tv_sec + 
		rusage.ru_utime.tv_usec/1000000.0)) ? val : 0)  << " seconds" << endl <<
        "    System time:        " << ((val = (rusage.ru_stime.tv_sec + 
		rusage.ru_stime.tv_usec/1000000.0)) ? val : 0)  << " seconds" << endl <<
	"CPU/Real:               " << (
		cpu_time > elapsed ? 100.0 :
		100.0*cpu_time/elapsed ) << "%" << endl;
	if (rusage.ru_maxrss)
	  cerr << "Max resident size:      " << rusage.ru_maxrss << "k" << endl;
	if (rusage.ru_ixrss)
	  cerr << "Shared text memory:     " << (rusage.ru_ixrss)/ticks << "k" << endl;
	if (rusage.ru_idrss)
	  cerr << "Unshared data:          " << (rusage.ru_idrss)/ticks << "k" << endl;
	if (rusage.ru_isrss)
	  cerr << "Unshared stack:         " << (rusage.ru_isrss)/ticks << "k" << endl;
	if (rusage.ru_minflt || rusage.ru_majflt)
	  {
	    cerr << "Page reclaims:          " << rusage.ru_minflt << endl <<
	            "       faults:          " << rusage.ru_majflt << endl;
	  }
	cerr << "Swaps:                  " << rusage.ru_nswap << endl;
	if (rusage.ru_inblock || rusage.ru_oublock)
	  {
	    cerr << "File system in events:  " << rusage.ru_inblock << endl <<
	            "           out event:   " << rusage.ru_oublock << endl;
	  }
	if (rusage.ru_nvcsw ||  rusage.ru_nivcsw)
	  {
	    cerr << "Context switches Vol.:  " << rusage.ru_nvcsw << endl <<
	            "               Invol.:  " << rusage.ru_nivcsw << endl;
	  }
	if (ru == RUSAGE_SELF) {
	  cerr << "Total records added:    " << (final_records - start_records) << endl <<
	  "Total words added:      " << (final_total_words - start_total_words) << endl;
	}
	cerr << endl;
       }
#ifndef _WIN32
       if (ru == RUSAGE_SELF)
	{
	  ru = RUSAGE_CHILDREN;
	  goto rusage;
	}
#endif
     };
#endif

#ifdef WIN32
//  WSACleanup();
#endif

  return 0;
}


static void HelpLanguage()
{
  char **codes, **values;
  size_t count = GetLangCodes(&codes, &values);
  cout << "The following language codes are available:\n";
  cout << " Code\tLanguage\n ----\t---------\n";
  for (size_t i = 0; i < count; i++)
    cout << " " << codes[i] << "\t" << values[i] << endl;
}

static void HelpLocale()
{
  char **codes, **values;
  size_t count = GetLocaleCodes(&codes, &values);
  cout << "The following locale \"shorthand\" codes are available:" << endl
       << " Code            Equivalent Locale" << endl
       << " ----            -----------------" << endl;
  for (size_t i = 0; i < count; i++)
    cout << " " << codes[i] << setw(16-strlen(codes[i])) << " " << values[i] << endl;
  cout << "Other locales may be specified as: <language>.<charset>" << endl;
}

static void HelpTypes()
{
  FIELDTYPE ft;
  ft.AvailableTypesHelp(cout);
}


#undef LOG_PANIC
#undef LOG_FATAL
#undef LOG_ERROR
#undef LOG_ERRNO
#undef LOG_WARN
#undef LOG_NOTICE
#undef LOG_INFO
#undef LOG_DEBUG 
#undef LOG_ALL
#undef LOG_ANY

#if HAVE_SYSLOG
#include <sys/syslog.h>
#endif



static void IniUsage()
{
#if 0
  IDB  *db = new IDB();
#else
  VIDB  *db = new VIDB();
#endif
  cout << "Ini file (<database>.ini) options:" << endl;

  cout << db->Description();

  cout << "Doctype.ini options may also be embeded into database.ini files as:" << endl;
  cout << "[<Doctype>]  # Doctype, e.g. [TEXT]" << endl;
  cout << "# Options are those from the <doctype>.ini [General] section <key>=<value>. Example:" << endl <<
	"FieldType=<file to load for field definitions>  # default <doctype>.fields" << endl <<
	"DateExpiresField=<Date of record expiration>" << endl <<
	"# Consult the individual Doctype documentation:  -thelp <doctype>" << endl << endl;

#ifndef _WIN32
  cout << "Note: If the software has NOT been installed in /opt/nonmonotonic please confirm that" << endl <<
          "you have created either a user \"asfadmin\" (if you are running ASF) or \"ibadmin\"" << endl <<
          "whose HOME directory points to where the software has been installed." << endl << endl;
  delete db; // Don't delete on WIN32 ( WIN32 WORKAROUND!!!!!!)
#endif
}

static void Usage()
{
  cout << "Usage is: " << prognam << " [-d db] [options] [file...]" << endl
  << "Options:" << endl
  << " -d db              // Use database db." << endl
  << " -setuid X          // Run under user-id/name X (when allowed)." << endl
  << " -setgid X          // Run under group-id/name X (when allowed)." << endl
  << " -cd X              // Change working directory to X before indexing." << endl
  << " -thes source       // Compile search thesaurus from file source." << endl
  << " -T Title           // Set Title as database title." << endl
  << " -R Rights          // Set Rights as Copyright statement." << endl
  << " -C Comment         // Set Comment as comment statement." << endl
  << " -mem NN            // Advise min. of NN RAM." << endl
  << " -memory NN         // Force min. of NN RAM." << endl
  << "                    // Note: Specifying more memory than available process RAM can" << endl
  << "                    // have a detrimental effect on performance." << endl
  << " -relative_paths    // Use relative paths (relative to index path)." << endl
  << " -base path         // Specify a base path for relative paths." << endl 
  << " -rel               // Use relative paths and assume relation between index location" << endl
  << "                    // and files remains constant." << endl
  << " -absolute_paths    // Make file paths absolute (default)." << endl
  << " -ds NN             // Set the sis block to NN (max " << StringCompLength << ")." << endl
  << " -mdt NN            // Advise NN records for MDT." << endl
  << " -common NN         // Set common words threshold at NN." << endl
  << " -sep sep           // Use C-style sep as record separator." << endl
  << " -s sep             // Same as -sep but don't escape (literal)." << endl
  << " -xsep sep          // Use C-style sep as record separator but ignore sep." << endl
  << " -start NN          // Start from pos NN in file (0 is start)." << endl
  << " -end nn            // End at pos nn (negative to specify bytes from end-of-file)." << endl
  << " -override          // Override Keys: Mark older key duplicates as deleted (default)." << endl
  << " -no-override       // Don't override keys." << endl
  << " -centroid          // Create centroid." << endl
  << " -t [name:]class[:] // Use document type class to process documents." << endl
  << " -charset X         // Use charset X (e.g. Latin-1, iso-8859-1, iso-8859-2,..)." << endl
  << " -lang ISO          // Set the language (ISO names) for the records." << endl
  << "                    // Specify help for a list of registered languages." << endl
  << " -locale X          // Use locale X (e.g. de, de_CH, pl_PL, ...)" << endl
  << "                    // These set both -charset and -lang above." << endl
  << "                    // Specify help for a list of registered locales." << endl
  << " -stop              // Use stoplist during index (default is none)" << endl
  << " -l name            // Use stoplist file name; - for \"builtin\"." << endl
  << " -f list            // File containing list of filenames; - for stdin." << endl
  << " -recursive         // Recursively descend subdirectories." << endl
  << " -follow            // Follow links." << endl
  << " -include pattern   // Include files matching pattern." << endl
  << " -exclude pattern   // Exclude files matching pattern." << endl
  << " -incldir pattern   // Include dirs matching pattern." << endl
  << " -excldir pattern   // Exclude dirs matching pattern." << endl
  << " -name pattern      // Like -recursive -include." << endl
  << "                    // pattern is processed using Unix \"glob\" conventions:" << endl
  << "                    // * Matches any sequence of zero or more characters." << endl
  << "                    // ? Matches any single character, [chars] matches any single" << endl
  << "                    // character in chars, a-b means characters between a and b." << endl
  << "                    // {a,b...} matches any of the strings a, b etc." << endl
  << "                    // -include, -incldir, -excldir and -name may be specified multiple" << endl
  << "                    // times (including is OR'd and excluding is AND'd)." << endl
  << " -r                 // -recursive shortcut: used with -t to auto-set -name." << endl
  << " -o opt=value       // Document type class specific option." << endl
  << "                    // Generic: HTTP_SERVER, WWW_ROOT, MIRROR_ROOT, PluginsPath" << endl 
  << " -log file          // Log messages to file; <stderr> (or -) for stderr, <stdout>" << endl
  << "                    // for stdout or <syslog[N]> for syslog facility using LOG_LOCALN when" << endl
#ifdef LOG_LOCAL9
  << "                    // N is 0-9, if N is 'D' then use LOG_DAEMON, and 'U' then LOG_USER." << endl 
#elif defined(LOG_LOCAL8)
  << "                    // N is 0-8, if N is 'D' then use LOG_DAEMON, and 'U' then LOG_USER." << endl 
#elif defined(LOG_LOCAL7)
  << "                    // N is 0-7, if N is 'D' then use LOG_DAEMON, and 'U' then LOG_USER." << endl 
#elif defined(LOG_LOCAL6)
  << "                    // N is 0-6, if N is 'D' then use LOG_DAEMON, and 'U' then LOG_USER." << endl 
#elif defined(LOG_LOCAL5)
  << "                    // N is 0-5, if N is 'D' then use LOG_DAEMON, and 'U' then LOG_USER." << endl 
#endif
  << " -e file            // like -log above but only log errors." << endl
  << " -syslog facility    // Define an alternative facility (default is LOG_LOCAL2) for <syslog> where" << endl
#ifdef LOG_LOCAL9
  << "                    // facility is LOG_AUTH, LOG_CRON, LOG_DAEMON, LOG_KERN, LOG_LOCALN (N is 0-9), etc." << endl
#elif defined(LOG_LOCAL8)
  << "                    // facility is LOG_AUTH, LOG_CRON, LOG_DAEMON, LOG_KERN, LOG_LOCALN (N is 0-8), etc." << endl
#elif defined(LOG_LOCAL7)
  << "                    // facility is LOG_AUTH, LOG_CRON, LOG_DAEMON, LOG_KERN, LOG_LOCALN (N is 0-7), etc." << endl
#elif defined(LOG_LOCAL6)
  << "                    // facility is LOG_AUTH, LOG_CRON, LOG_DAEMON, LOG_KERN, LOG_LOCALN (N is 0-6), etc." << endl
#elif defined(LOG_LOCAL5)
  << "                    // facility is LOG_AUTH, LOG_CRON, LOG_DAEMON, LOG_KERN, LOG_LOCALN (N is 0-5), etc." << endl
#endif
  << " -level NN          // Set message mask to NN (0-255) where the mask is defined as (ORd): PANIC ("
	<< iLOG_PANIC << ")," << endl
  << "                    // FATAL (" << iLOG_FATAL << "), ERROR (" << iLOG_ERROR << "), ERRNO ("
	<< iLOG_ERRNO << "), WARN (" << iLOG_WARN << "), NOTICE (" << iLOG_NOTICE << "), INFO ("
	<< iLOG_INFO << "), DEBUG (" << iLOG_DEBUG << ")." << endl
  << " -quiet             // Only important messages and notices." << endl
  << " -silent            // Silence messages (same as -level 0)." << endl
  << " -verbose           // Verbose messages." << endl
  << " -maxsize NNNN      // Set Maximum Record Size (ignore larger)[-1 for unlimited]." << endl
  << " -nomax             // Allow for records limited only by system resources [-maxsize -1]." << endl
  << " -a                 // (Fast) append to database." << endl
  << " -O                 // Optimize in max. RAM. (-optimize -mem -1)" << endl
  << " -Z                 // Optimize in max. RAM but minimize disk (-merge -mem -1)" << endl
  << " -fast              // Fast Index (No Merging)." << endl
  << " -optimize          // Merge sub-indexes (Optimize)" << endl
  << " -merge             // Merge sub-indexes during indexing" << endl
  << " -collapse          // Collapse last two database indexes." << endl
  << " -append            // Add and merge (like -a -optimize)" << endl
  << " -incr              // Incremental Append" << endl
  << " -qsort B[entley]|S[edgewick]|D[ualPivot] // Which variation of Qsort to use" << endl 
// << " -debug             // Debug Indexer" << endl
#if EVALULATION
  << " -license           // Display expiration date" << endl
#endif
  << " -copyright         // Print the copyright statement." << endl
  << " -version           // Print Indexer version." << endl
  << " -api               // Print API Shared libs version." << endl
  << " -capacities        // Print capacities." << endl
  << " -kernel            // Print O/S kernel capacities." << endl
  << " -help              // Print options (this list)." << endl
  << " -help d[octypes]   // Print the doctype classes list (same as -thelp)" << endl
  << " -help l[ang]       // Print the language help (same as -lang help)" << endl
  << " -help l[ocale]     // Print the locale list (same as -locale help)" << endl
  << " -help o[ptions]    // Print the options (db.ini) help." << endl
  << " -help t[types]     // Print the currently supported data types." << endl
  << " -thelp             // Show available doctype base classes." << endl
  << " -thelp XX          // Show Help for doctype class XX." << endl
#ifndef _WIN32
  << " -xhelp             // Show the information Web." << endl
#endif
  << endl
  << "NOTE: Default \"database.ini\" configurations may be stored in _default.ini" << endl
  << "in the configuration locations." << endl
 << "Options are set in section [DbInfo] as:" <<endl
  << "   Option[<i>]= # where <i> is an integer counting from 1 to 1024" << endl
  << "Example:  Option[1]=WWW_ROOT=/var/httpd/htdocs/" << endl
  << endl;
}


