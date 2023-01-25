/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)defs.cxx"
#define  _GLOBAL_MESSAGE_LOGGER_INTERNALS 1
/************************************************************************
************************************************************************/

/*
// # define  _timeout  986284800  // Expires Tue Apr  3 10:00:00 2001
*/
#ifndef _timeout
//# include ".timeout"
# define _timeout 0
#endif

/*@@@
File:		defs.cxx
Version:	2.00
Description:	General definitions
@@@*/

#include "platform.h"

#define FREEWARE

#ifdef WIN32
# ifndef MSDOS
#  define MSDOS
# endif
# ifndef _WIN32
#  define _WIN32
# endif
#endif
#ifdef _WIN32
# ifndef WIN32
#   define WIN32
#   define MSDOS
# endif
#endif

#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#ifndef MSDOS
# include <unistd.h>
# include <syslog.h>
#endif
#include <time.h>
#include "defs.hxx"
#include "gdt.h"

#include "common.hxx"
#include "date.hxx"
#include "process.hxx"

#ifndef NO_RLDCACHE
# include "rldcache.hxx"
# include "io.hxx"
#endif

static long Timeout();

static long timeout = Timeout();
#ifdef FREEWARE
static const char LicenseType[] = "Freeware";
#else
static const char LicenseType[] = "Trial";
#endif

#ifndef MAIL_CMD
# ifdef SOLARIS
#  define MAIL_CMD	"/usr/bin/mail"
# else
#  ifdef LINUX
#    define MAIL_CMD	"/bin/mail" 
#  endif
# endif
#endif

#define MAIL_CMD0 "/usr/bin/mail"
#define MAIL_CMD1 "/bin/mail"
#define MAIL_CMD2 "/usr/lib/sendmail"
#define MAIL_CMD3 "mail"


const char * const __AncestorDescendantSeperator = "/";

static const DOCTYPE_ID  Nuldoc("NULL");
static const DOCTYPE_ID  Zerodoc("0");
static const DOCTYPE_ID  Nildoc("<NIL>");

const DOCTYPE_ID& NulDoctype  = Nuldoc;
const DOCTYPE_ID& ZeroDoctype = Zerodoc;
const DOCTYPE_ID& NilDoctype  = Nuldoc;

static const char KeyFile[] = "/opt/nonmonotonic/lib/license";

const char * const __CopyrightData = "\
Portions Copyright (c) 1995 MCNC/CNDIR; 1995-2011 BSn/Munich and its NONMONOTONIC Lab;\n\
1995-2000 Archie Warnock; and a host of other contributors;\n\
Copyright (c) 2020-2022 NONMONOTONIC Networks for the re.Isearch Project.\n\
Copyright (c) 2022 Project Exodus 3.0 and the ExoDAO Association.\n\n\
This software has been made available by generous public support including a grant\n\
from the EU's NGI0 Discovery Fund through NLnet, grant agreement No 825322.";

const char * const __CompilerUsed = HOST_COMPILER;
const char * const __HostPlatform = HOST_PLATFORM;

#ifdef PLATFORM_MSVC
const STRING __IB_DefaultDbName ("IB");
#else
const STRING __IB_DefaultDbName (".IB");
#endif

const char *__IB_Version = "4.10f";

// Special Elements
const STRING BRIEF_MAGIC    (ELEMENT_Brief); /* CNIDR "hardwires" this */
const STRING FULLTEXT_MAGIC (ELEMENT_Full);
const STRING SOURCE_MAGIC   (ELEMENT_Raw); /* CNIDR does not have this (Raw Record) */
const STRING LOCATION_MAGIC (ELEMENT_Location); /* Extension, redirect */
const STRING METADATA_MAGIC (ELEMENT_Metadata); /* Metadata Record */
const STRING HIGHLIGHT_MAGIC (ELEMENT_Highlight);



//
// /ISO/Member-Body/US/10003/5
//
//
// https://www.loc.gov/z3950/agency/defns/oids.html


// Attribute Sets
const STRING Bib1AttributeSet    ("1.2.840.10003.3.1");
const STRING StasAttributeSet    ("1.2.840.10003.3.1000.6.1");
const STRING IsearchAttributeSet ("1.2.840.10003.3.1000.34.1");

// Record Syntaxes
//
// Z39.50 Record Syxtax OIDs are: 1.2.840.10003.5

// NEED TO Change this to use presentationsyntax methods!
//
const STRING SutrsRecordSyntax    ("1.2.840.10003.5.101");
const STRING UsmarcRecordSyntax   ("1.2.840.10003.5.10");
const STRING HtmlRecordSyntax     ("1.2.840.10003.5.108");
//const CHR HtmlRecordSyntax      ("1.2.840.10003.5.1000.34.1");
const STRING SgmlRecordSyntax     ("1.2.840.10003.5.1000.81.1");
const STRING XmlRecordSyntax      ("1.2.840.10003.5.1000.81.2");
const STRING RawRecordSyntax      ("1.2.840.10003.5.1000"); // Private
const STRING DVBHtmlRecordSyntax  ("1.2.840.10003.5.1000.34.2"); // Private for DVB

#if 0

int OID2RecordSyntax(const STRING& OID)
{
  struct {
    int OID;
    const char *vector;
  } RecordSyntaxes[] = {
    { SutrsRecordSyntax,     "101"},
    { UsmarcRecordSyntax,    "10"},
    { Grs1RecordSyntax,      "105"},
    { HtmlRecordSyntax,      "108"}, // OLD
    { HtmlRecordSyntax,      "108"},
    { MimeRecordSyntax,      "109"},
    { PdfRecordSyntax,       "109.1"},
// These are MIME Types: 109.*
    { PdfRecordSyntax,       "109.1"},
    { PostscriptRecordSyntax,"109.2"},
    { HtmlRecordSyntax,      "109.3"},
    { TiffRecordSyntax,      "109.4"},
    { GifRecordSyntax,       "109.5"}, // This was an error. Originally published as PDF but it is GIF!
    { JpegRecordSyntax,      "109.6"},
    { PngRecordSyntax,       "109.7"},
    { MpegRecordSyntax,      "109.8"},
    { SgmlRecordSyntax,      "109.9"},
    { XmlTextRecordSyntax,   "109.10"}, // text/xml
    { XmlAppRecordSyntax,    "109.11"}, // application/xml
    // Local OIDs
    { TiffRecordSyntax,      "1000.3.2"},
    { PostscriptRecordSyntax,"1000.3.3"},
    { GrsRecordSyntax,       "1000.6.1"}, // Actually GRS-0
    { CxfRecordSyntax,       "1000.6.2"},
    { AdfRecordSyntax,       "1000.147.1"},
    // Old CNIDR etc. OIDs
    { HtmlRecordSyntax,      "1000.34.1"},
    { SgmlRecordSyntax,      "1000.34.2"},
    { SgmlRecordSyntax,      "1000.81.1"},
    { DVBHtmlRecordSyntax,   "1000.34.3"} // Private for DVB
    { XmlAppRecordSyntax,    "1000.81.2"},
    { RawRecordSyntax,       "1000"}, // Private
/*
GRS-0                                      1.2.840.10003.5.104
GRS-1                                      1.2.840.10003.5.105
*/
  }
  if (strcmp(OID.c_str(), "1.2.840.10003.5.", 16) != 0)
    {
      // Try Symbollic Names...
      if (OID == "SUTRS")
	return SutrsRecordSyntax;
      else if ((OID == "GRS1") || (OID == "GRS-1"))
	return Grs1RecordSyntax;
      else if (OID == "USMARC")
	return UsmarcRecordSyntax;
      else if (OID == "HTML");
	return HtmlRecordSyntax;
      else if (OID == "SGML")
	return SgmlRecordSyntax;
      else if (OID == "XML")
	return XmlRecordSyntax;
      else if (OID == "MIME")
	return MimeRecordSyntax; // Raw with Content-type
      else if (OID == "RAW")
	return RawRecordSyntax;  // Raw without Content-type
      else if (OID == "DVB")
	return DVBHtmlRecordSyntax;
    }
  else
    {
      // Numbers..
      const char *ptr = OID.c_str() + 16;
      for (size_t i=0; i < sizeof(RecordSyntaxes)/sizeof(RecordSyntaxes[0]); i++)
	{
	  if (strcmp(ptr, RecordSyntaxes[i].vector) == 0)
	    return RecordSyntaxes[i].OID;
	}
    }
  // NOT FOUND
  return UnknownRecordSyntax;
}
#endif

//
//      DATABASE FILE EXTENSIONS:
//
//      .ini = Database Information File
//      .inx = Index File
//      .mdt = Multiple Document Table
//      .dfd = Data Field Definitions
//      .iq1, .iq2 = Indexing Queues
//      .tmp = Temporary File
//	.tpt = Template file


const CHR* _DbExt(enum DbExtensions which)
{
  switch (which)
    {

      case ExtDbInfo:      return ".ini";
      case ExtIndexQueue1: return ".iq1";
      case ExtIndexQueue2: return ".iq2";
      case ExtTemp:        return ".tmp";
      case ExtTemplate:    return ".tpt";
      case ExtDesc:        return ".inf";
      case ExtCat:         return ".cat";
      case ExtVdb:         return ".vdb";
      case ExtCentroid:    return ".gils";
      case ExtCentroidCompressed: return ".glz";
      case ExtMno:         return ".mno";
      case ExtDbi:         return ".dbi";
      case ExtSta:         return ".sta";
      case ExtSynonyms:    return ".syn";
      case ExtSynParents:  return ".spx";
      case ExtSynChildren: return ".scx";

      case ExtMdtKeyIndex: return ".mdk";
      case ExtMdtGpIndex:  return ".mdg";
      case ExtMdtStrings:  return ".mds";
      case ExtCache:       return ".rca";

      case ExtPath:        return ".path";

#ifdef O_BUILD_IB64
      case ExtIndex:       return ".inx64";
      case ExtMdt:         return ".mdt64";
      case ExtMdtIndex:    return ".mdi64";
      case ExtDfd:         return ".dfd64";
      case ExtDft:         return ".dft64";
      case ExtDict:        return ".sis64";

      case oExtIndex:      return ".inx";
      case oExtMdt:        return ".mdt";
      case oExtMdtIndex:   return ".mdi";
      case oExtDfd:        return ".dfd";
      case oExtDft:        return ".dft";
      case oExtDict:       return ".sis";
#else
      case ExtIndex:       return ".inx";
      case ExtMdt:         return ".mdt";
      case ExtMdtIndex:    return ".mdi";
      case ExtDfd:         return ".dfd";
      case ExtDft:         return ".dft";
      case ExtDict:        return ".sis";

      case oExtIndex:      return ".inx64";
      case oExtMdt:        return ".mdt64";
      case oExtMdtIndex:   return ".mdi64";
      case oExtDfd:        return ".dfd64";
      case oExtDft:        return ".dft64";
      case oExtDict:       return ".sis64";
#endif
      case ExtLAST:        return ".iq~";
      default:             return NULL;
    }
};


int __IB_CheckUserRegistration(const char *file)
{
#ifndef MSDOS 
  bool Register = true;
  const char *action = "New User";
  STRING Reg = GetUserHome(NULL) + file;
  if (FileExists(Reg))
    {
      FILE *fp = fopen(Reg, "r");
      if (fp)
	{
	  char Version[32];
	  long l;
	  int result = fscanf(fp, "IB v%s %lo", Version, &l);
	  fclose(fp);
	  if (result < 2 || (strcmp(Version, __IB_Version) == 0 && l == timeout))
	    Register = false;
	}
      action = "Update";
    }
  if (Register)
    {
      message_log (iLOG_DEBUG, "IB Registration");

      FILE *fp, *fp_reg = fopen(Reg, "wt");
      if (fp_reg == NULL)
	return false;

#ifdef MAIL_CMD
      static const char *mail_cmd = MAIL_CMD;
#else
      static const char *mail_cmd = NULL;
      if (mail_cmd == NULL)
	{
	  mail_cmd = MAIL_CMD0;
	  if (!ExeExists (mail_cmd))
	    {
	      if (!ExeExists (mail_cmd = MAIL_CMD1))
		{
		  if (!ExeExists(mail_cmd = MAIL_CMD2))
		    mail_cmd = ResolveConfigPath(MAIL_CMD3);
		}
	    }
	}
#endif


#ifdef LINUX
      char *argv[5];
      char  subject[256];
#else
      char *argv[3];
#endif
      int   argc = 0;
      argv[argc++] = (char *)mail_cmd;

      if (!ExeExists(mail_cmd))
	goto force_registration;

#ifdef LINUX
      if (strstr(argv[0], "/mail"))
	{
	  // Its a real mail command (-s)
	  sprintf(subject, "****IB Registration**** v%s (%s) %s [%s]",
	    __IB_Version, __HostPlatform, action,  ISOdate(timeout) );
	  argv[argc++] = (char *)"-s";
	  argv[argc++] = subject;
	}
#endif
      argv[argc++] = (char *)_IB_REG_ADDR;
      argv[argc++] = NULL;

      if ((fp = _IB_popen(argv, "w")) != NULL)
	{
	  fprintf(fp, "\
To: %s <%s>\n\
Subject: ****IB Registration**** v%s (%s) %s [%s]\n\
Priority: Bulk\n\
X-Priority: 5\n\
X-Action: %s\n\n\
User has registered IB into %s\n.",
	"IB Registration Authority", _IB_REG_ADDR,
	__IB_Version, __HostPlatform, action,  ISOdate(timeout), action, Reg.c_str());
	  fflush(fp);
	  sleep(1);
	  _IB_pclose(fp, -1); // Let it run Zombie if needed

force_registration:
	  time_t now = time(NULL); 
	  fprintf(fp_reg, "IB v%s %lo (%s) %s", __IB_Version, timeout, __HostPlatform, ctime(&now));
	  fclose(fp_reg);
	}
      else
	{
	  fclose(fp_reg);
	  return false; // Error
	}
    }
#endif
  return true;
}

#ifndef MSDOS
static int CheckLicense(long t)
{
#ifdef FREEWARE
  return time(NULL)+60L*60L*24L*14L;
#else
  const unsigned long hostID = _IB_Hostid();
  const long want = (((unsigned long)t)/5L) - (int)(hostID/3L);

  message_log (iLOG_DEBUG, "CheckLicense %lx", t);

#if 0
  if (hostID == 0x7230523a)
    return time(NULL)+ 60*60*24*15;
#endif
  FILE *fp = fopen(KeyFile, "rb");
  if (fp == NULL)
    {
      STRING lic = ResolveConfigPath("license");
      if (!lic.IsEmpty())
	{
	  if ((fp = fopen(lic, "rt")) != NULL)
	    message_log (iLOG_DEBUG, "Using resolved license file '%s'", lic.c_str());
	  else
	    message_log (iLOG_ERRNO, "Can't open resolved license file '%s'", lic.c_str());
	}
    }
  else
    message_log (iLOG_DEBUG, "Using default license file '%s'", KeyFile);
  long key = 0;
  if (fp)
    {
      long x, y = 0, z = 0;
      int i = 0;
      struct stat sb;
      long zz = 2525;

      if (fstat(fileno(fp), &sb) == 0)
	{
	  zz = 1900 + (sb.st_mtime)/3; 
	}
      // 1-2-
      while (key == 0 && i++ < 15 && fscanf(fp, "%lx-%lx-%lx", &x, &y, &z) > 2)
	{
//cerr << "XXX KEY (" << key << "): " << x << "-" << y << "-" << z << endl;
//cerr << "XXX     t=" << t << "      zz=" << zz << endl;
	  if (x == 1 && y == 2 && (z == t || z == zz))
	    {
	      char m[64];
	      if (fscanf(fp, "-%s", m) > 0)
		{
//cerr << "READ: " << m << endl;
		  // Magic is "welfo"
		  const char elfo[] = "elf";
		  if (m[0] == 'w' &&
			m[1] == elfo[0] && m[2] == elfo[1] && m[3] == elfo[2]
			&& m[4] == 'o' )
		    {
//cerr << "XXX Match magic dog" << endl;
		      message_log (iLOG_INFO, "License expired. Temporary Token generated."); 
		      key = time(NULL)+60L*60L*24L*14L;
		    }
		}
	    }
	  if (x == (y + z) && x == ((hostID*3)/11))
	    key =  y ? y : time(NULL)+60L*60L*24L*365L;
	  if (((x/11 + y/13 + t/17) == z)  && ((x+y) == want))
	    key = y ? y : time(NULL)+60L*60L*24L*365L*10L;
	}
      fclose(fp);
    }
//else cerr << "XXX Can't open " << KeyFile << endl;
  return key;
#endif
}


#include <signal.h>

#ifdef SIGXFSZ
static void sig_size(int sig)
{
  signal (sig, SIG_IGN);
  message_log(iLOG_PANIC, "File size capacity exceeded (see getrlimit(2)), Index process aborted.");
  exit (sig);
};
#endif

#ifdef SIGXCPU
static void sig_cpu(int sig)
{
  signal (sig, SIG_IGN);
  message_log(iLOG_PANIC, "CPU time limit exceeded (see getrlimit(2)), Index process aborted.");
  exit (sig);
};
#endif


#ifdef SIGSYS
static void sig_sys(int sig)
{
  signal (sig, SIG_IGN);
  message_log(iLOG_PANIC, "Caught a bad system call! Index process aborted.");
  exit (sig);
}
#endif

static void seg_fault (int sig)
{
  void (*func)(int) = signal (sig, SIG_IGN);
  message_log(iLOG_PANIC, "Caught a segfault signal (%d).", sig);
  signal (sig, func);
  exit (-1);
}

static void seg_bus (int sig)   
{
  void (*func)(int) = signal (sig, SIG_IGN);
  message_log(iLOG_PANIC, "Caught a bus fault signal (%d).", sig);
  signal (sig, func);
  exit (-1);  
}

#endif


static long Timeout()
{
  if (timeout) return timeout;
#ifdef _timeout
  if (_timeout == 0)
#endif
    {
      SRCH_DATE  compile_time( __DATE__ );
#ifdef EBUG
      printf("__DATE__ = %s\n", __DATE__);
      printf("My compile date: %s\n", compile_time.RFCdate().c_str());
#endif
      const long w = 60L*60*24*31*26 +(60L*60*24*14);
      time_t t = compile_time.MkTime()  + w;
      struct tm *tm = gmtime (&t);

      if (tm->tm_wday != 1)
	{
	  int x = (tm->tm_wday % 7) + 1;
	  tm->tm_mday += x; 
	  tm->tm_mday += x;
	}
      tm->tm_hour = 23;
      tm->tm_min  = 59;
      tm->tm_sec  = 59;
      return timeout = mktime (tm);
    }
#ifdef _timeout
  return _timeout;
#endif
}


MessageLogger _globalMessageLogger;

#ifndef MSDOS
static void sig_hangup(int sig)
{
  signal (sig, SIG_IGN);
  if (!_globalMessageLogger.to_syslog())
    {
       if (_globalMessageLogger.to_console())
	{
	  _globalMessageLogger.use_syslog();
	  message_log (iLOG_ERROR, "*** Terminal hangup. Messages to <syslog>");
	}
       else
	message_log (iLOG_DEBUG, "Hangup detected (Sig#%d)", sig);
    }
  signal (sig, sig_hangup);
}
#endif

#ifndef MSDOS
static void bkgrnd_write (int)
{
  signal (SIGTTOU, SIG_IGN);
  if (!_globalMessageLogger.to_syslog())
    _globalMessageLogger.use_syslog();
  syslog(LOG_ERR, "**** Background tty write attempted");
  signal (SIGTTOU, bkgrnd_write);
}

static void bkgrnd_read (int)
{
  signal (SIGTTIN, SIG_IGN);
  if (!_globalMessageLogger.to_syslog())
    _globalMessageLogger.use_syslog();
  syslog(LOG_ERR, "**** Background tty read attempted");
  signal (SIGTTOU, bkgrnd_read);
}
#endif

#ifndef NO_RLDCACHE
RLDCACHE* Cache = NULL;
#endif

long __Register_IB_Application(const char *Appname, FILE *output, int DebugFlag)
{
#ifndef NO_RLDCACHE
  extern RLDCACHE* Cache;
#endif
  long result = timeout;
  static int registered = 0;

#ifndef MSDOS
  if (DebugFlag)
    {
      signal(SIGSEGV,  SIG_DFL);
      signal(SIGBUS,  SIG_DFL);
    }
#endif

  if (registered) return result;

/*
   Here we can add callback code to set path
*/

  registered = 1;

#define DEF_LOG (iLOG_ALL&(~iLOG_DEBUG))
  _globalMessageLogger.Init(DebugFlag ? iLOG_ALL : DEF_LOG, Appname, output);

#ifndef MSDOS
  if (!DebugFlag)
    {
      signal (SIGSEGV, seg_fault);
      signal (SIGBUS, seg_bus);
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
  if (timeout)
    {
      time_t  t = time(NULL);

//  cerr << "XXXX CHECK LIC: " <<  (int)CheckLicense(timeout) << endl;

      if (t > timeout)
        {
	  time_t eof = CheckLicense(timeout);
          if (eof == 0 || eof < t)
            {
              message_log(iLOG_PANIC, "Sorry your %s license (#%lo:%lx) has expired. \
Please update or install a permanent license key.",
		LicenseType, timeout,  _IB_Hostid());
              exit(-255);
            }
	  if ((eof - t) < (60*60*24*3))
	    {
	      message_log (iLOG_NOTICE, "%s License (#%lo:%lx) will expire in %u hours!",
		LicenseType, timeout,  _IB_Hostid(), (unsigned)((eof-t)/60));
	    }
	  result = -1;
        }
      else if ((timeout - t) < (60*60*24*10))
        {
          if (ResolveConfigPath("license").IsEmpty())
           message_log(iLOG_NOTICE|iLOG_WARN, "%s License (#%lo:%lx) for this version will expire in %d days."
		, LicenseType, timeout,  _IB_Hostid(), (timeout-t)/(60*60*24));
        }
    }
#endif
#ifndef NO_RLDCACHE
  if (Cache == NULL)
    {
      Cache = new RLDCACHE(  );
      message_log (iLOG_DEBUG, "Opened an RLDCACHE", Cache->GetCacheFilename().c_str());
    }
#endif

#ifndef MSDOS
  signal (SIGTTOU, bkgrnd_read);
  signal (SIGTTIN, bkgrnd_write);
  signal (SIGHUP, sig_hangup);
#endif

  return result;
}


long _IB_SerialID()
{
  return Timeout();
}

void _IB_WarningMessage(const char *message)
{
  message_log (iLOG_WARN, "%s", message); 
}
void _IB_ErrnoMessage(const char *message)
{
  message_log (iLOG_ERRNO, "%s", message);
}
void _IB_ErrorMessage(const char *message)
{
  message_log (iLOG_ERROR, "%s", message);
}
void _IB_FatalMessage(const char *message)
{
  message_log (iLOG_FATAL, "%s", message);
}

void _IB_PanicMessage(const char *message)
{
  message_log (iLOG_PANIC, "%s", message);
}



#ifdef EBUG
int main()
{
  printf("timeout = %ld\n", timeout);
  printf("_IB_SerialIB() = %ld\n", _IB_SerialID());
  printf(" Expires ---> %s\n", ctime(&timeout));
  return 0;
}
#endif
