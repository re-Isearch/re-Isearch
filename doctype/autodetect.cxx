
/* ########################################################################

   ########################################################################

   Note: The more DOCTYPEs defined the better! This module *should* be
	designed to determine on the basis of the available doctypes the
	correct doctype!

   ####################################################################### */
#include "autodetect.hxx"
#include "mmap.hxx"
#include "common.hxx"
#include "process.hxx"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "reclist.hxx"

#include "dtreg_list.hxx"


#ifdef _WIN32
inline FILE *fopen_rt(const STRING& file) { return file.Fopen("rt"); }
inline FILE *fopen_rb(const STRING& file) { return file.Fopen("rb"); } 
inline FILE *fopen_wb(const STRING& file) { return file.Fopen("wb"); } 
inline FILE *fopen_ab(const STRING& file) { return file.Fopen("ab"); } 

#else

inline FILE *fopen_rt(const STRING& file) { return file.Fopen("r"); }
inline FILE *fopen_rb(const STRING& file) { return file.Fopen("r"); }
inline FILE *fopen_wb(const STRING& file) { return file.Fopen("w"); }
inline FILE *fopen_ab(const STRING& file) { return file.Fopen("a"); }

#endif

/*
#define strncasecmp StrNCaseCmp
*/

// We want to use HTMLZERO if we don't know any better.
static STRING DefaultHTMLClass = "HTMLZERO";
static STRING HTMLClass = "HTML";

// XML default
static STRING XMLClass   = "XML";
static STRING RSSClass   = "RSS2";
static STRING AtomClass  = "ATOM";
static STRING CentroidClass = "CENTROID:XMLBASE";
static STRING SHAKESPEAREClass = "PLAY:XMLBASE";
static STRING DOCBOOKClass = "DOCBOOK:XMLBASE";
static STRING TEIClass     = "TEI:XMLBASE";
static STRING CAPClass= "CAP";

#ifndef GILS_ISOTEIA_HXX
# define GILS_ISOTEIA_HXX 0
#endif
#if GILS_ISOTEIA_HXX
static STRING ISOTEIAclass = "ISOTEIA"; // Use builtin
#else
static STRING ISOTEIAclass = "ISOTEIA:"; // Use plugin 
#endif

AUTODETECT::AUTODETECT (PIDBOBJ DbParent, const STRING& Name):
	DOCTYPE (DbParent, Name)
{
#if USE_LIBMAGIC
  magic_cookie = NULL;
#endif
  kludge = false;
  kludgeCount = 0;
  HostID = _IB_Hostid(); 

  if (tagRegistry)
    {
      STRING S;
      tagRegistry->ProfileGetString("General", "ParseInfo", "Y", &S);
      ParseInfo = S.GetBool();
    }
  else
    ParseInfo = true;
}

const char *AUTODETECT::Description(PSTRLIST List) const
{
  List->AddEntry ("AUTODETECT");
  DOCTYPE::Description(List);

#if USE_LIBMAGIC
  return "\
Doctype format autodection (callback hooks)\n\
Identifies (or tries to) many of the numerous document formats and types\n\
supported inclusive of variants. Documents not identified by the internal\n\
logic are handled by libmagic's magic.\n\n\
Options in .ini:\n\n\
[General]\n\
Magic=<path>     # Path of optional magic file\n\
ParseInfo=[Y|N]  # Parse Info for binary files (like images)\n\
\n\
[Use]\n\
<DoctypeClass>=<DoctypeClassToUse> # example HTML=HTMLHEAD\n\
<DoctypeClass>=NULL  # means don't index <DoctypeClass> files\n";

#else
  return "Doctype format autodection (callback hooks)\n\
Identifies (or tries to) many of the numerous document formats and types\n\
supported inclusive of variants. Documents not identified by the internal\n\
logic are searched for file \"magic\".\n\n\
Options in .ini:\n\n\
[General]\n\
File=<path>      # Path to alternative file(1) command\n\
Magic=<path>     # Path of optional magic file\n\
ParseInfo=[Y|N]  # Parse Info for binary files (like images)\n\
\n\
[Use]\n\
<DoctypeClass>=<DoctypeClassToUse> # example HTML=HTMLHEAD\n\
<DoctypeClass>=NULL  # means don't index <DoctypeClass> files\n";
#endif

}



// Extensions are case dependent
// [NOTE: We include some typical Unix->DOS->Unix maps]
static struct {
  const char *extension;
  const char *doctype;
  const char *mimeType;
 } Builtin[] = {
  // Add to list..
#ifdef PLAINTEXT_HXX
  {"text",  "PLAINTEXT"},
  {"TEXT",  "PLAINTEXT"},
#else
  {"text", ""},
#endif
#ifdef MEDLINE_HXX
  {"med",     "MEDLINE"},
  {"medline", "MEDLINE"},
  {"MED",     "MEDLINE"},
#endif
#ifdef FILMLINE_HXX
  {"flm",  "FILMLINE"},
  {"film", "FILMLINE"},
  {"FLM",  "FILMLINE"},
#endif
#ifdef DVBLINE_HXX
  {"dvb",  "DVBLINE"},
  {"DVB",  "DVBLINE"},
#endif
#ifdef EUROMEDIA_HXX
  {"eur",  "EUROMEDIA"},
  {"EUR",  "EUROMEDIA"},
#endif
#ifdef HARVEST_HXX
  {"soif",   "HARVEST"},
#endif
#ifdef TSLDOC_HXX
  {"tsv",    "TSV"},
  {"tsl",    "TSLDOC"},
#endif
#ifdef BIBTEX_HXX
  {"btx",     "BIBTEXT"},
  {"bibtex",  "BIBTEX"},
  {"BTX",     "BIBTEX"},
#endif
#ifdef ISI_CIW_HXX
  {"ciw",     "ISI-CIW"},
  {"CIW",     "ISI-CIW"},
#endif
  {".cgm",   "CGM:IMAGE",  "image/cgm"},
#ifdef REFERBIB_HXX
  {"refer", "REFERBIB"},
  {"REF",   "REFERBIB"},
  {"end",   "ENDNOTE"},
  {"END",   "ENDNOTE"},
#endif
#ifdef HTML_HXX
  {"htm",  "HTML"},
  {"html", "HTML"},
  {"HTM",  "HTML"},
#endif
  {"gils", "GILS"},
  {"gil",  "GILS"},
  {"GIL",  "GILS"},
#ifdef SGMLNORM_HXX
  {"dtd",  "\"SGML Document Type Definition (DTD)\""},
  {"ent",  "\"SGML Document Entities (DTD)\""},
/*
  {"sgm",  "SGMLNORM"},
  {"sgml", "SGMLNORM"},
  {"SGM",  "SGMLNORM"},
*/
#endif
#ifdef XML_HXX
//  {"xml", "XML"},
//  {"XML", "XML"},
#endif
#ifdef IAFADOC_HXX
  {"iafa", "IAFADOC"},
  {"IAF",  "IAFADOC"},
#endif
#ifdef DIF_HXX
  {"dif", "DIF"},
  {"DIF", "DIF"},
#endif
#ifdef ROADSDOC_HXX
  {"roads", "ROADS++"},
  {"rds",   "ROADS++"},
#endif
#ifdef IKNOWDOC_HXX
  {"whois++", "IKNOWDOC"},
  {"WHO",     "IKNOWDOC"},
  {"who",     "IKNOWDOC"}, // Added 2008
#endif
#ifdef CSVDOC_HXX
  {"csv", "CSVDOC"},
#endif
#ifdef TSLDOC_HXX
  {"tsv", "TSVDOC"},
#endif

  {"ignore", "NULL"},
  {"cc", "\"C++ Program Source\"", "text/plain"},
  {"CC", "\"C++ Program Source\"", "text/plain"},
  {"cpp", "\"C++ Program Source\"", "text/plain"},
  {"C++", "\"C++ Program Source\"", "text/plain"},
  {"cxx", "\"C++ Program Source\"", "text/plain"},
  {"hpp", "\"C++ Program Source\"", "text/plain"},
  {"hxx", "\"C++ Program Source\"", "text/plain"},
  {"hh", "\"C++ Program Source\"", "text/plain"},
  {"c", "\"C Program Source\"", "text/plain"},
  {"h", "\"C Program Source\"", "text/plain"},
  {"y", "\"Yacc Program Source\"", "text/plain"},
  {"l", "\"Lex Program Source\"", "text/plain"},
  {"p", "\"Pascal Program Source\"", "text/plain"},
  {"pas", "\"Pascal Program Source\"", "text/plain"},
  {"py", "\"Python Program Source\"", "text/plain"},
  {"pyo", "\"Python Fastloads\""},
  {"for", "\"Fortran Program Source\"", "text/plain"},
  {"xd", "\"X Developer Source\"", "text/plain"},
  {"el", "\"Emacs Lisp (EL)\"", "text/plain"},
  {"elc", "\"Emacs Bytecode (ELC)\""},
  {"mlo", "\"Gosling Emacs Bytecode\""},
  {"stl", "\"Style File (STL)\"", "text/plain"},
  {"nvm", "\"NaviServer Maps (NVM)\"", "text/plain"},
  {"mdt", "\"IB Multiple Document Table (MDT)\""},
  {"mdi", "\"IB MDT index cache (MDI)\""},
  {"dfd", "\"IB Document Field Definitions (DFD)\""},
  { NULL, NULL}
};

/* string magic */
static struct Magic {
  unsigned char offset;
  const char *str;
  unsigned char len;
  const char *doctyp; 
  const char *mimeType;
  const char *sub_doc;
 } Magic[] = {
    {0, "\151\006", 2, "\"IB 2.0 Index\""},
    {0, "\151\007", 2, "\"IB 2.0 Index (hyphenation)\""},
    {0, "\011\042", 2, "\"IB 3.0-pre Index\""},
    {0, "\111\042", 2, "\"IB 3.0-pre 64-bit Index\""},
    {0, "\377\377\377\377\000\000\000\000\000\000", 10, "\"IB 3.0-pre strings\""},
    {0, "\037\213", 2, "\"gzip compressed data\"", "application/x-gzip",
	"GZIP:OBJ"},
    {0, "\037\235", 2, "\"Unix compressed data\"",
	"application/x-compress", "COMPRESS"},
#if 0 /* Don't look since we want to handle .jar, .odt etc. files */
    {0, "PK\003",   3, "\"Zip archive\"", "application/zip",
	"ZIP:OBJ"},
#endif
#ifdef PSDOC_HXX
    {0, "%!PS-Adobe-", 11, "PS"},
#endif
/*#ifdef PDFDOC_HXX */
    {0, "%PDF-",    5, "PDF"},
/*#else
    {0, "%PDF-",    5, "\"Adobe Portable Document Format (PDF)\"",
	"application/pdf", "PDF:OBJ"},
#endif */
#ifdef PSDOC_HXX
    {0, "%!PS-",  5, "PS"},
#else
    {0, "%!PS-",    5, "\"Adobe PostScript (PS)\"",
	"application/postscript", "PS:OBJ"},
#endif
    {0, "\177ELF",  4, "\"Executable  and  Linking Format (ELF) object\""},
    {0, "From ", 5, 
#ifdef MAILFOLDER_HXX
	"MAILFOLDER"
#else
	"\"Unix Mailfolder\""
#endif
	},

#ifdef MAILFOLDER_HXX
   {0, "Return-Path: <", 14, "RFC2822:MAILFOLDER" }, // RFC2822
#endif

    {0, "AT&TFORM", 8, "DJVU:"}, 

    {0, "Article ", 8,
#ifdef NEWSFOLDER_HXX
	"NEWSFOLDER"
#else
	"\"Usenet News folder\""
#endif
	},
    {0, "Entry_ID:", 9,
#ifdef DIF_HXX
	"DIF"
#else
	"\"Directory Interchange Format (DIF) Metarecord\""
#endif
	},
   {0, "This is Info file ", 18,
#ifdef EMACSINFO_HXX
	"EMACSINFO"
#else
	"\"GNU Emacs Info file\""
#endif
	},
/* Microsoft Crap */
   // Pre-2007 Word files. Later use DOCX which is XMLish in ZIP
   {0, "\376\000\043", 3, "MSWORD:", "application/msword"},
   {0, "\233\245",     2, "MSWORD:", "application/msword"},
   {65,"W6BNMSWD",     8, "MSWORD:", "application/msword"},
   {65,"W7BNMSWD",     8, "MSWORD:", "application/msword"},
   {65,"W8BN",         4, "MSWORD:", "application/msword"},
   {65,"WDBNMSWD",     8, "MSWORD:", "application/msword"},
   {65,"WDBNWORD",     8, "MSWORD:", "application/msword"},

   {0, "\320\317\021\340\241\261\032\341", 8, "MSOLE:", "application/ms-ole"},
   {0, "\320\317\021", 3, "MSWORD:", "application/msword"},
   // Rich Text Format
   {0, "{\rtf", 5, "MSRTF:", "text/rtf"},

/* File: zsh.info,  Node: Miscellaneous,  Prev: Completion,  Up: Zsh Line Editor */
   /* TeX System stuff */
   {0, "\367\002", 2, "\"TeX DVI file\"", "application/x-tex-dvi"},
   {0, "\367\203", 2, "\"TeX generic font data\""},
   {0, "\367\131", 2, "\"TeX packed font data\""},
   {0, "\367\312", 2, "\"TeX virtual font data\""},
   {0, "This is TeX", 11, "\"TeX transcript text\"", "text/plain"},
   {0, "This is METAFONT", 16, "\"METAFONT transcript text\"", "text/plain"},

   /* FrameMaker */
   {0, "<MakerFile",  10, "\"FrameMaker document\"",
	"application/vnd.framemaker"},
   {0, "<MIFFile", 8, "\"FrameMaker MIF file\"", "application/vnd.mif"},
   {0, "<MakerDictionary", 16, "\"FrameMaker Dictionary text\""},
   {0, "<MakerScreenFon", 15, "\"FrameMaker Font file\""},
   {0, "<MML", 4, "\"FrameMaker MML file\"", "application/vnd.mml"},
   {0, "<Book", 5, "\"FrameMaker Book file\""},
   {0, "<Maker", 6, "\"FrameMaker Intermediate Print File (IPL)\""},
   {0, "<!SQ DTD>", 9, "\"SoftQuad Compiled SGML rules file\""},
   {0, "<!SQ A/E>", 9, "\"SoftQuad A/E SGML Document binary\""},
   {0, "<!SQ STS>", 9, "\"SoftQuad A/E SGML binary styles file\""},

   /* Misc junk */
   {0, "WORKSET VERSION ", 16, "\"SunSoft Tools Workset\""},
   {0, "! Generated Code", 16, "\"SunSoft Visual Resource\""},
   {0, "#!/bin/sh", 9, "\"Borne Shell Script\"", "text/plain"},
   {0, "#!/bin/csh", 9, "\"'C' Shell Script\"", "text/plain"},
   {0, "#!/bin/ksh", 9, "\"Korn Shell Script\"", "text/plain"},
   {0, "#!/bin/zsh", 9, "\"'Z' Shell Script\"", "text/plain"},
   {0, "#!/usr/local", 12, "\"Local Script\"", "text/plain"},
   {0, "#!/usr/contrib", 14, "\"Contrib Script\"", "text/plain"},
   {0, "#!/opt/", 7, "\"Option Script\"", "text/plain"},
   {0, "#!/", 12, "\"Script Program\"", "text/plain"},
   {0, "=<ar>", 5, "\"Unix .a archive (lib)\""},
   {0, "!<arch>", 7, "\"Unix .a archive (lib)\""},
   {0, "070701", 6, "\"ASCII cpio archive\"", "application/x-cpio"},
   {0, "070702", 6, "\"ASCII cpio archive - CRC header\"",
	"application/x-cpio"},
   {0, "070707", 6, "\"ASCII cpio archive - CHR (-c) header\"",
	"application/x-cpio"},
   {0, "xbtoa Begin", 11, "\"btoa'd file\"", "application/x-btoa"},
   {0, "SIT!", 4, "\"StuffIt (Macintosh) archive\"",
	"application/x-stuffit"},
   {0, "GIF87a", 6, "\"GIF file, v87\"", "image/gif", "GIF:IMAGE"},
   {0, "GIF89a", 6, "\"GIF file, v89\"", "image/gif", "GIF:IMAGE"},
#if 0
   {0, "\377\330\377\340", 4, "\"JPEG file\"", "image/jpeg", "JPEG:IMAGE"},
   {0, "\377\330\377\356", 4, "\"JPG file\"", "image/jpeg", "JPG:IMAGE"},
#else
  {0, "\377\330\377\340", 4, "JPEG:EXIF:", "image/jpeg", "JPEG:EXIF:"},
  {0, "\377\330\377\356", 4, "JPEG:EXIF:", "image/jpeg", "JPEG:EXIF:"},
#endif
   {0, "\131\246\152\225", 4, "\"Rasterfile\"", "image/x-raster", "RASTER:IMAGE"},
   {8, "ILBM", 4, "\"IFF ILBM file\"", "image/iff", "IFF:IMAGE"},
   {0, "ZyXEL\002", 6, "\"ZyXEL voice data\""},
   {0, "BEGMF", 5, "\"CGM clear text encoding\"", "image/cgm", "CGM:IMAGE"},
   {0, "BegMF", 5, "\"CGM clear text encoding\"", "image/cgm", "CGM:IMAGE"},
   {0, "\060\040\176", 3, "\"CGM character encoding\""},
   {0, "\060\040\033\134\033\130", 6, "\"CGM character encoding\""},
   {0, ".snd", 4, "\"Audio File\""},
   {36, "acsp", 4, "\"Kodak Color Management System, ICC Profile\""},
   {0, "#SUNPC_CONFIG", 13, "\"SunPC 4.0 Properties Values\""}
// {0, NULL, 0, NULL}
};

#if SUPPORT_PDF
static void GetPDFTitle(const STRING& FileName, PSTRING Title)
{
  // Subject
  // Author
  // Keywords
  // Creator
  // Producer
  // CreationDate
  // ModDate
  // Search for << /CreationDate (
  MMAP MemoryMap(FileName, MapSequential);

  Title->Clear();
  if (MemoryMap.Ok() == false)
   return;

  const UCHR *Buffer = (UCHR *)MemoryMap.Ptr();
  const size_t size = MemoryMap.Size();

  enum { SCANING, LOOKING, EXTRACTING } state = SCANING;
  for (size_t i=10; i < size - 50; i++)
    {
      if (state != SCANING &&
	(Buffer[i] == '\015' || Buffer[i] == '\012'))
	{
	  if (state == EXTRACTING &&
		Buffer[i+1] == '>' && Buffer[i+2] == '>')
	    {
	      break; // Done with group
	    }
	  if (Buffer[i+1] == '/')
	    {
	      i += 2;
	      if (state == EXTRACTING)
		{
		  if (memcmp(&Buffer[i], "Title (", 7) == 0)
		    {
		      char title[256];
		      // Have the Title
		      size_t j = (i += 7) + 1; // Skip Title (
		      for (;j < s.st_size; j++)
			{
			   if (Buffer[j] == '\\')
			      j++;
			   else if (Buffer[j] == ')')
			      break;
			}
		      size_t len = j - i;
		      if (len > 256)
			len = 255;
		      strncpy(title, &Buffer[i], len);
		      title[len] = '\0';
		      *Title = title;
		      break; // Done;
		    }
	        }
	      else if (memcmp(&Buffer[i], "CreationDate (", 14) == 0)
		{
		  state = EXTRACTING;
		}
	    }
	}
      if (state == SCANING &&
	  Buffer[i] == '<' && Buffer[i+1] == '<' &&
	 (Buffer[i+3] == '\015' || Buffer[i+3] == '\012'))
	{
	  i += 3;
	  state = LOOKING;
	}
    }
}
#endif

static void GetGIFTitle(const STRING& FileName, PSTRING Title)
{
  MMAP MemoryMap(FileName, MapSequential);

  Title->Clear();
  if (MemoryMap.Ok() == false)
   return; 

  const UCHR *Buffer = (UCHR *)MemoryMap.Ptr();
  const size_t size = MemoryMap.Size();

  size_t i = 6;
  UCHR ch = Buffer[i++];
  while (i < (size - 3))
      {
	if (ch == '\0')
	  {
	    if ((ch = Buffer[i++]) == '!')
	      {
		if ((ch = Buffer[i++]) == (unsigned char)'\376')
		  {
		     char buf[255];
		     if ((ch = Buffer[i++]) >= (sizeof(buf)/sizeof(char)))
			ch = (UCHR)(sizeof(buf)/sizeof(char) - 1); 
		     strncpy(buf, (char *)&Buffer[i], (size_t)ch); 
		     buf[ch] = '\0';
		     // Clip first line of comment
		     char *tp = buf;
		     while (isspace(*tp))
			tp++;
		     if (*tp)
			{
			  char *tcp;
			  Title->Clear();
			  while ((tcp = strchr(tp,'\n')) != NULL)
			    {
			      *tcp++ = '\0';
			      if (*tp)
				{
				  Title->Cat (tp);
				  // Skip multiple LFs
				  while (*tcp == '\n') tcp++;
				  // Cat New line
				  Title->Cat ('\n');
				  // Need field extension?
				  if (*tcp != ' ' && *tcp != '\t')
				    Title->Cat(' ');
				}
			      tp = tcp;
			    }
			  if (*tp) Title->Cat (tp);
			}
		     break;
		  }
	      }
	    else if (ch != '\0')
	      ch = Buffer[i++];
	  }
	else ch = Buffer[i++];
      }
}


static int CountCh(const char *buf, char Ch)
{
  int count = 0;
  if (buf)
    {
      for (const char *tcp = buf; *tcp; tcp++)
	{
	  if (*tcp == Ch)
	    count++;
	}
    }
  return count;
}


void AUTODETECT::BeforeIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie != NULL)
    {
      magic_close (magic_cookie);
      magic_cookie = NULL;
    }
  file_magic = ResolveConfigPath(Getoption("Magic", "magic") );
  if (!::FileExists(file_magic))
    file_magic.clear();
  else
    message_log (LOG_DEBUG, "libmagic db set to '%s'", file_magic.c_str());  
#else
  file_cmd = ResolveBinPath(Getoption("File", "file") );

  if (file_cmd.IsEmpty() || !::ExeExists(file_cmd))
    {
      if (!::ExeExists( file_cmd = "/bin/file" ))
	file_cmd = "file";
    }
  message_log (LOG_DEBUG, "file(1) command set to '%s'", file_cmd.c_str());

  file_magic = ResolveConfigPath(Getoption("Magic", "magic") );
  if (!::FileExists(file_magic))
    {
      file_magic.clear();
    }
  else
    message_log (LOG_DEBUG, "file(1) magic set to '%s'", file_magic.c_str());
#endif
}

void AUTODETECT::AfterIndexing()
{
#if USE_LIBMAGIC
  if (magic_cookie != NULL)
    {
      magic_close (magic_cookie);
      magic_cookie = NULL;
    }
#endif
}

static const STRING commaInfo    (",info");
static const STRING commaInfoTxt (",info.txt");
static const STRING commaInfoXml (",info.xml");


void AUTODETECT::ParseRecords (const RECORD& FileRecord) 
{
  RECORD Record (FileRecord);
  STRING doctype;
  STRING subdoctype, mime;
  struct stat stbuf;
  const STRING s (FileRecord.GetFullFileName());
  static const char SGML_magic[] = "!DOCTYPE";

  if (s.GetLength() > 6)
    {
      STRING c ( s.Right(',') );
      if (c.GetLength() >= 4 &&
		strncmp(c.c_str(), commaInfo.c_str()+1, commaInfo.GetLength()-1) == 0)
	{
	  message_log (LOG_INFO, "Skipping %s (%s[*] File)", s.c_str(), commaInfo.c_str()) ;
	  return;
 	}
    }

  // s now contains the filename
  if (_IB_lstat(s, &stbuf) == -1)
    {
      message_log(LOG_ERRNO, "Can't access '%s'", s.c_str() );
      return;
    }
  const int inode = stbuf.st_ino;
#ifndef _WIN32
  if (stbuf.st_mode & S_IFLNK)
    {
      if (stat(s, &stbuf) == -1)
	{
	  message_log(LOG_ERROR, "'%s' is a dangling symbollic link", s.c_str());
	  return;
	}
    }
#endif
  if (stbuf.st_size == 0)
    {
      message_log(LOG_ERROR, "'%s' has ZERO (%ld) length? Skipping.", s.c_str(), stbuf.st_size);
      return;
    }
  PFILE fp;
  STRING beforeExt (s.Before('.'));
  STRING ext = s.Right('.');

#ifdef FTP_HXX
 if (FileExists(beforeExt + ".iafa"))
   doctype = "FTP";
  else
#endif

#ifdef BINARY_HXX
  // Binary type?
  if (FileExists (s + commaInfoXml))
    {
      message_log (LOG_INFO, "Pair '%s'/*%s detected.", s.c_str(), commaInfoXml.c_str()); 
      doctype = "XBINARY";
    }
  else if (FileExists (s + commaInfoTxt))
    doctype = "OCR";
  else if (FileExists (s + commaInfo))
    doctype = "BINARY";
  else
#endif
#ifdef FTP_HXX
  if (FileExists (s + ".iafa"))
    doctype = "FTP";
  else
#endif
#ifdef BINARY_HXX
   if (!ext.IsEmpty() && (ext == "tiff" || ext == "tif" || ext == "fax") && FileExists(beforeExt+".txt"))
     doctype = "BINARY";
   else
#endif
  if ((stbuf.st_mode & S_IFDIR) == S_IFDIR)
    {
      doctype="\"Directory\"";
    }
#if defined(MAILFOLDER_HXX) || defined(MEMODOC_HXX) || defined(EMACSINFO_HXX) || defined(DIF_HXX)
  // Check some content
  else if ((fp = fopen_rt(s)) != NULL)
    {
      char buf[BUFSIZ+2];

      //message_log (LOG_DEBUG, "Checking contents...");

/*
      if (ext == "inx")
	{
	  short IndexMagic = getINT2(fp);
	  if (IndexMagic == 2338)
	     doctype = "\"IB 2.x .inx (index)\"";
	}
*/
      if (fseek(fp, FileRecord.GetRecordStart(), SEEK_SET) == -1)
	{
#ifdef _WIN32
          message_log (LOG_PANIC|LOG_ERRNO, "Could not seek on %s!(%I64d)!",
                (const char *)s, (long long)FileRecord.GetRecordStart());
#else
	  message_log (LOG_PANIC|LOG_ERRNO, "Could not seek on %s!(%lld)!",
		(const char *)s, (long long)FileRecord.GetRecordStart());
#endif
	}
      else
 	{
	  size_t i = 0;
	  int    ch;
	  while (i < BUFSIZ && ((ch = getc(fp)) != EOF))
	    {
	      if (ch == '\r' || ch == '\n')
		{
		  if (i > 3) break;
		}
	      else
		buf[i++] = ch;
	    }
	  buf[i] = '\0';
	  if (i == 0)
	    {
	      // Empty Record
	      // message_log (LOG_DEBUG, "Empty text. skip");
	      fclose(fp);
	      return; // Skip
	    }
	  size_t buf_len = strlen(buf);

	  // Here we use our own hardwired magic data. We do this since we have a few issues
	  for (size_t i=0; i < (sizeof(Magic)/sizeof(struct Magic)); i++)
	    {

	      if ((buf_len > Magic[i].offset) && memcmp(&buf[Magic[i].offset], Magic[i].str, Magic[i].len) == 0)
		{
		  message_log(LOG_DEBUG, "Signature \"%.*s\" matches %s", Magic[i].len, &buf[Magic[i].offset],  Magic[i].doctyp);

                  doctype = Magic[i].doctyp;
		  if (Magic[i].sub_doc)
		    subdoctype = Magic[i].sub_doc;
		  if (Magic[i].mimeType)
		    mime = Magic[i].mimeType;
		  break;
		}
	    }
     {
#ifdef METADOC_HXX 
	char *tcp = buf;
	while (buf[0] )
	  {
	    while (isspace(*tcp)) tcp++; // Loop to first character

	    if (*tcp == '\0' || *tcp == ';' || *tcp == '[')
	      {
		if ((tcp = fgets(buf, BUFSIZ, fp)) == NULL)
		  buf[0] = '\0';
		continue;
	      }
	    while (isalnum(*tcp)) tcp++; // to end of alphanumerics 

	    if (*tcp == '<')
	      {
		// <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN"> ?
		if (strncmp(tcp+1, SGML_magic, sizeof(SGML_magic)-1) == 0 && strstr(tcp, "HTML"))
		  {
		     doctype = HTMLClass; 
		  }
		else if (strncmp(tcp+1, "rss", 3) == 0 && isspace(tcp[4]))
		  {
		     doctype = RSSClass;
		  }
		// <?php
		else if (tcp[1] == '?' && tcp[2] == 'p' && tcp[3] == 'h' && tcp[4] == 'p' &&
			 (isspace(tcp[5]) || tcp[5] == '?'))
		  {
		    doctype = HTMLClass;
		  }
		// <?xml
		else if (tcp[1] == '?' && tcp[2] == 'x' && tcp[3] == 'm' && tcp[4] == 'l' &&
			(isspace(tcp[5]) || tcp[5] == '?'))
		  {
		    doctype = XMLClass;
		  }
		else if (tcp[1] == '-' && tcp[2] == '-')
		  {
		     // Probably HTML since should not start SGML/XML with a comment
		     tcp += 3;
		     while (*tcp && *tcp != '-' && *(tcp+1) != '-')
			tcp++;
		     while (*tcp == '\0')
		       {
			  // Read another line
			  if (fgets(buf, BUFSIZ, fp) == NULL)
			    buf[0] = '\0';
			  tcp = buf;
			  while (*tcp && *tcp != '-' && *(tcp+1) != '-')
			    tcp++;
		       }
		   if (*tcp == '>')
		      {
			tcp++;
			continue;
		      }
		    // we should be looking at -- or nothing
		   if (*tcp != '-')
		     continue;
		   while (*tcp && *tcp != '<') tcp++; // look for the <

		  }
		// In D-A-S-H we have seen loads of BAD HTML
#if 1
		// Don't want to see ? or ! since this can be XML or SGML
		if (isalpha(tcp[1]) && isalnum(tcp[2]))
#else

		if ( (strncasecmp(&tcp[1], "script", 6) == 0 && (isspace(tcp[7]) || tcp[7] == '>')) ||
			strncasecmp(&tcp[1], "html", 4) == 0 ||
			strncasecmp(&tcp[1], "head>", 5) == 0 ||
			strncasecmp(&tcp[1], "title>", 6) == 0 ||
			strncasecmp(&tcp[1], "body", 4) == 0 )
#endif
		  {
		    if (doctype != RSSClass)
		      {
#if 1
			if ( (strncasecmp(&tcp[1], "script", 6) == 0 && (isspace(tcp[7]) || tcp[7] == '>')) ||
			  strncasecmp(&tcp[1], "html", 4) == 0 || strncasecmp(&tcp[1], "head>", 5) == 0 ||
			  strncasecmp(&tcp[1], "title>", 6) == 0 || strncasecmp(&tcp[1], "body", 4) == 0 )
			  {
			    doctype = HTMLClass;
			  }
			else
#endif
		         doctype = XMLClass;
		     }
		  }
	      }
	    else if (*tcp == '=')
	      {
		while (isspace(*++tcp)) /* loop */;
		if (isalnum(*tcp))
		  doctype = "METADOC";
	      }
	    break;
	  }
#endif
    }

#ifdef MEMODOC_HXX
	  if (doctype.IsEmpty() || (doctype ^= "COLONDOC"))
	    {
	      if (strncasecmp(buf, "Path:", 5) == 0)
		{
		  if (fgets(buf, BUFSIZ, fp) == NULL)
		    doctype = "COLONDOC";
		  // 1st line after Path: is OrigDOCTYPE: then its from a filter
		  else if (strncmp(buf, "OrigDOCTYPE:", 11) == 0)
		    {
		      doctype = "MEMODOC"; // A special kind
		    }
		}
	    }
	  if (doctype.IsEmpty() &&
		(  strncasecmp(buf, "Subject:", 8) == 0 ||
		   strncasecmp(buf, "Title:",   6) == 0 ||
		   strncasecmp(buf, "From:",    5) == 0 ||
		   strncasecmp(buf, "Author:",  7) == 0 ||
		   strncasecmp(buf, "Date:",    5) == 0 ||
		   strncasecmp(buf, "To:",      3) == 0))
	    {
	      // Look for ------
	      char seps[] = "_-=+";
	      char *tcp;
	      while (fgets(buf, BUFSIZ, fp) != NULL)
		{
		  if ((tcp = strchr(seps, buf[0])) != NULL)  
		    if (buf[1] == *tcp && buf[2] == *tcp && buf[3] == *tcp)
		      {
			doctype = "MEMODOC";
			break;
		      }
		}
	    } 
#endif
	}
      fclose(fp);
    }
#endif

  if (doctype.IsEmpty())
    {
      if (!ext.IsEmpty())
	{
	  // Now look for extension
	  for (size_t i = 0; Builtin[i].extension; i++)
	    {
	      if (ext.Equals(Builtin[i].extension))
		{
		  doctype = Builtin[i].doctype;
		  if (Builtin[i].mimeType)
		    mime = Builtin[i].mimeType;
		  break;
		}
	    }
	}
    }
#if defined(BINARY_HXX) || defined(FTP_HXX)
  else if (doctype.GetChr(1) == '"')
    {
      /* Is this to be processed as a binary of FTP? */
#ifdef BINARY_HXX
      // Binary type?
      if (FileExists (s + commaInfo))
	doctype = "BINARY";
      else if (FileExists(s+commaInfoXml))
	doctype = "XBINARY";
      else
	{
#endif
#ifdef FTP_HXX
      if (FileExists (s + ".iafa"))
	doctype = "FTP";
#endif
#ifdef BINARY_HXX
	}
#endif
    }
#endif

  if (doctype.IsEmpty())
    {
#ifdef HTML_HXX
      // Re-open and check if maybe HTML, SGMLNORM or SGMLTAG
      // -- we do this now since GILS and SGMLTAG look alike
      // <!DOCTYPE HTML means HTML, <HTML> means HTML, <HEAD> means HTML
      if ((fp = fopen_rt(s)) != NULL)
	{
	  char buf[BUFSIZ];
	  char *tcp, *tp;
	  fseek(fp, FileRecord.GetRecordStart(), SEEK_SET);
	  while (fgets(buf, BUFSIZ-1, fp) != NULL)
	    {
	      if (buf[0] == '#')
		{
		  // Is it a C, C++ program?
		  for (tcp = buf+1; *tcp == ' ' || *tcp == '\t'; tcp++)
		    /* loop */;
		  if (strncmp(tcp, "pragma", 6) == 0 ||
		      strncmp(tcp, "define", 6) == 0 ||
		      strncmp(tcp, "include", 7) == 0 ||
		      strncmp(tcp, "line", 4) == 0)
		    {
		      doctype = "\"C Program Source\"";
		      break;
		    }
		}

	      for (tcp = buf; *tcp && *tcp != '<'; tcp++)
		{
		  if (!isspace(*tcp))
		    break;
		}
	      if (isspace(*tcp))
		continue; // Blank line
#ifdef REFERBIB_HXX
	      if (buf[0] == '%' && isupper(buf[1]))
		{
		  doctype = "REFERBIB";
		  break;
		}
#endif
#ifdef MEDLINE_HXX
	      if (isupper(buf[0]) &&
		isupper(buf[1]) &&
		buf[2] == ' ' &&
		buf[3] == ' ' && 
		(buf[4] == '-' || buf[4] == ' '))
		{
#ifdef DVBLINE_HXX
		  if (strncmp(buf, "SI  -", 5) == 0)
		    {
		      doctype = "DVBLINE";
		    }
		   else
#endif /* DVBLINE_HXX */
		    {
		      doctype = "MEDLINE";
		    }
		}
#endif /* MEDLINE_HXX */
	     if (*tcp != '<')
		{
#ifdef BIBTEX_HXX
		  if (*tcp == '@' || *tcp == '%')
		    {
#ifdef SOIF_HXX
		      if (strncmp(tcp+1, "FILE", 4) == 0)
			doctype = "SOIF";
		      else
#endif
		      doctype = "BIBTEX";
		    }
#endif
#ifdef COLONDOC_HXX
		  // Alphanum + -_$. 
		  do {
		    tcp++;
		  } while (isalnum(*tcp) || *tcp == '-' || *tcp == '_' ||
			*tcp == '$' || *tcp == '.');
		  if (*tcp == ':')
		    {
#ifdef IAFADOC_HXX
		      // Check if IAFA is in name
		      STRING base = s;
		      // Strip path
		      RemovePath(&base);
		      // Uppercase it
		      base.ToUpper();
		      if (base.Search("IAFA"))
			doctype = "IAFADOC";
		      else
#endif
#ifdef ROADSDOC_HXX
		      if (strncmp(buf, "Template-Type", 13) == 0)
			doctype = "ROADS++";
		      else
#endif
#ifdef IKNOWDOC_HXX
		      if (strncmp(buf, "Template", 8) == 0)
			doctype = "IKNOWDOC";
		      else
#endif
		      if (strncasecmp(buf, "Ipnode", 6) == 0)
			doctype = "SHI:MEMODOC";
		      else
#ifdef EUROMEDIA_HXX
		      if (strncmp(buf, "Local_number", 12) == 0)
			{
			  // Check next line..
			  fgets(buf, BUFSIZ-1, fp);
			  if (strncmp(buf, "Date_added_to_database", 22) == 0)
			    doctype = "EUROMEDIA";
			  else
			    doctype = "COLONGRP";
			} else
#endif
		      doctype = "COLONDOC";
		    }
#endif
		  break; // Not SGML-like family
		}
#ifdef XML_HXX
	      // Want <?XML> <?XML ..> or <?XMLDOC ..>
	      if ((tp = strstr(buf, "<?")) != NULL)
		{
		  if (strncasecmp(tp+2, "XML", 3) == 0)
		    {
		      if (isspace(tp[5]) || tp[5] == '>' ||
			strncasecmp(tp+5, "DOC ", 4) == 0)
			{
			  if (s.SearchAny("RSS") != 0)
			    doctype = RSSClass;
			  else if (s.SearchAny("isoteia") != 0)
			    doctype = ISOTEIAclass;
			  else
			    doctype = XMLClass;
			  continue; // may 2005
			}
		      break;
		    }
		}
#endif
	      // Look to match <!DOCTYPE or <!doctype or.. 
	      if ( (tp = strstr(buf, "<!")) != NULL &&
		    strncasecmp(tp+2, &SGML_magic[1], sizeof(SGML_magic)-2) == 0)
		{
		  tp += sizeof(SGML_magic) +1; // Pass it
		  while (isspace(*tp)) tp++;

		  if (strncmp(tp, "lewis SYSTEM ", 13) == 0)
		    {
		      doctype = "REUTERS:XMLBASE";
		    }
		  else if (strncasecmp(tp, "NewsML SYSTEM",  13) == 0)
		    {
		      //<!DOCTYPE NewsML SYSTEM ... ]>
		      doctype = "NewsML";
		    }
		   else if (strncasecmp(tp, "TEI SYSTEM", 10) == 0)
                    {
                      doctype = TEIClass;
                    }

		  else if (strncasecmp(tp, "PLAY SYSTEM", 11) == 0)
		    {
		      doctype = SHAKESPEAREClass;
		    }
		  else if (strncasecmp(tp, "Book",  4) == 0 && strstr(tp, "DTD DocBook"))
		    {
		      // <!DOCTYPE Book PUBLIC "-//OASIS//DTD DocBook V4.2//EN" [...]>
		      doctype = DOCBOOKClass;
		    }
		  else if (strncasecmp(tp, "HTML",  4) == 0 ||
		      strncasecmp(tp, "XHTML", 5) == 0 /* BAD XHTML */ )
		    {
		      doctype = HTMLClass;
		    }
		  else if (strncmp(tp, "rss", 3) == 0)
		    {
		      doctype = RSSClass;
		    }
		  else if (strstr(tp, "isoteia"))
		    {
		      doctype = ISOTEIAclass;
		    } 
		  else if (doctype != XMLClass)
		    {
		      doctype = "SGMLNORM";
		    }
		  break;
		}
	      else if (*(tcp + 1) != '!')
		{
		  // Looks like we start we a tag..
		  if (isalpha(tcp[1]) &&
		     (strncasecmp(tcp+1, "HTML", 4) == 0 ||
		    /* Bad HTML but.. */
		      strncasecmp(tcp+1, "HEAD", 4) == 0 ||
		      strncasecmp(tcp+1, "BODY", 4) == 0 ||
		      strncasecmp(tcp+1, "BASE", 4) == 0 ))
		    {
		      doctype = HTMLClass;
		    }
#ifdef XML_HXX
		  else if (strncasecmp(tcp, "<?XML", 5) == 0)
		    {
		      doctype = XMLClass;
		      break;
		    }
		   else if (strncasecmp(tcp, "<rss version=", 13) == 0)
		    {
		      doctype = RSSClass;
		      break;
		    }
		   else if (
			strncasecmp(tcp, "<atom:feed", 10) == 0 ||
			(strncasecmp(tcp, "<feed", 6) == 0 &&
			(isspace(tcp[5]) || tcp[5] == '>')) )
		    {
		      doctype = AtomClass;
		      break;
		    }
		   else if (strncmp(tcp, "<atom:", 6) == 0)
		    {
		      message_log (LOG_WARN, "Odd Atom namespaced document: '%s'?", (const char *)s);
		      doctype = AtomClass;
		      break;
		    }
#endif
#ifdef GILS_HXX
		  else if (strncasecmp(tcp, "<REC", 4) == 0 ||
		      strncasecmp(tcp, "<GILS", 5) == 0)
		    {
		      doctype = "GILS";
		    }
#endif
#ifdef SGMLTAG_HXX
		  else
		    {
		      doctype = "SGMLTAG";
		    }
#endif
		  break;
		}
	    } // while
#ifdef TSLDOC_HXX
	  // Try to see if tab format?
	  fseek(fp, FileRecord.GetRecordStart(), SEEK_SET);
	  if (fgets(buf, BUFSIZ-1, fp) != NULL)
	    {
	      int tab_count   = CountCh(buf, '\t');
	      int comma_count = CountCh(buf, ',' );

	      if (tab_count) 
		{
		  // Looks good (Have some tabs)
		  if (fgets(buf, BUFSIZ-1, fp) != NULL)
		    {
		      // Same number of Tabs?
		      if (tab_count == CountCh(buf, '\t'))
			doctype = "TSLDOC";
		    }
		}
	      else if (comma_count >= 3)
	 	{
		  if (tab_count != 0 || (fgets(buf, BUFSIZ-1, fp) != NULL))
		    {
		      if (comma_count == CountCh(buf, ','))
			{
			  // Make really sure
			  if (fgets(buf, BUFSIZ-1, fp) != NULL)
			    {
			      if (comma_count == CountCh(buf, ','))
				doctype = "CSVDOC";
			    }
			}
                    }   

		}	
	    }
#endif
	  fclose(fp);
	}
#endif
    }

  if (doctype.IsEmpty())
    {
#if USE_LIBMAGIC

      if (magic_cookie == NULL)
	{
	  magic_cookie = magic_open( MAGIC_SYMLINK|MAGIC_ERROR  );
	  if (magic_cookie)
	    magic_load (magic_cookie, file_magic.IsEmpty() ? NULL : file_magic.c_str() );
	}
      if (magic_cookie)
	{
	  const char *typ = magic_file (magic_cookie, s.c_str());

	  if (typ && *typ)
	    {
	      while (isspace(*typ)) typ++;
              // Map
              if (tagRegistry)
                tagRegistry->ProfileGetString("File", typ, NulString, &doctype);
              if (doctype.IsEmpty())
		{
		  doctype = typ;
		}

              if (doctype.SearchAny("OpenDocument"))
		{
		  message_log (LOG_DEBUG, "Recognized %s as ODF", s.c_str());
		  // OpenDocument Text
		  if (doctype.SearchAny("text")) {
		    if (Db->ValidateDocType("ODT"))
		        doctype = "ODT";
		    else if (PluginExists("ODT"))
			doctype = "ODT:";
		    else if (PluginExists("ODF"))
			doctype = "ODF:";
		  } else if (PluginExists("ODF")) doctype = "ODF:"; // Let it handle the rest
	          // else we can't so handle probably as a resource
		  else message_log (LOG_DEBUG, "No Open Document Format handler plugin installed.");
		}
              else if (doctype.SearchAny("Microsoft Word"))
		{
		  //  Microsoft Word 2007+ (Microsoft Office files)
		  if (PluginExists("MSOFFICE")) doctype = "MSOFFICE:";
		  else message_log (LOG_INFO, "No Microsoft Word 2007+ format handler plugin installed.");
		}
              else if (doctype.SearchAny("OOXML"))
		{
		  //  Microsoft OOXML (Microsoft Office files)
		  if (PluginExists("OOXML")) doctype = "OOXML:";
		  else message_log (LOG_INFO, "No Microsoft OOXML format handler plugin installed.");
		}
	      if (doctype.SearchAny("tiff"))
		doctype = "TIFF";
	      else if (doctype.SearchAny("text") 
		&& ( doctype.SearchAny("ascii") || doctype.SearchAny("english") ||
		doctype.Search("UTF-8" ) || doctype.Search("8859") ) )
		{
		  doctype = "PLAINTEXT";
		  message_log(LOG_INFO, "Identified %s as %s, using %s", (const char *)s, typ, (const char *)doctype );
		}
	    }

	}
#else
message_log (LOG_DEBUG, "BEFORE INDEXING");
      if (file_cmd.IsEmpty())
        BeforeIndexing();
message_log (LOG_DEBUG, "AFTER INDEXING");

#ifndef _WIN32

      // Lets see what file(1) says..
      char *argv[5];
      int   argc = 0;
      argv[argc++] = (char *)file_cmd.c_str();
      if (file_magic.GetLength())
	{
	  argv[argc++] = (char *)"-m";
	  argv[argc++] = (char *)file_magic.c_str();
	}

      argv[argc++] = (char *)s.c_str();
      argv[argc++] = NULL;

      FILE *fpipe = _IB_popen(argv, "r");
      if (fpipe)
	{
	  STRING lineBuf;
	  message_log (LOG_DEBUG, "External Command: %s", argv[0]);
	  if (lineBuf.FGet(fpipe, BUFSIZ))
	    {
	      const char *typ = lineBuf.c_str() + ((lineBuf.GetLength() > s.GetLength()) ? s.GetLength() + 1 : 0);
	      while (isspace(*typ)) typ++;
	      // Map
	      if (tagRegistry)
		tagRegistry->ProfileGetString("File", typ, NulString, &doctype);
	      if (doctype.IsEmpty())
		{
		  doctype = typ;
		  if (doctype.SearchAny("tiff")) 
		     doctype = "TIFF";
		  else if (doctype.Compare("English ", 8) == 0 ||
		       // doctype.SearchAny("ascii text") || doctype.SearchAny("ISO-8859 text") ||
			doctyoe.Search("text")) 
		    {
		      doctype = "PLAINTEXT";
		      message_log(LOG_INFO, "%s identified '%s', using %s",
                        argv[0], (const char *)lineBuf, (const char *)doctype );
		    }
		  else if (doctype.Compare("data", 4) == 0)
		    {
		       doctype="\"Misc. Binary data format\"";
		    }
		  else if (doctype.Search(' ')) // Quote if contains space
		    {
		      // Make --> "blah blah"
		      doctype.form("\"%s\"", typ);
		    }
		}
	    }
	  _IB_pclose(fpipe);
	}
      else {
	message_log (LOG_ERRNO, "Could not open pipe to file command: '%s'", argv[0]);
      }
#endif
#endif
    }

  if (doctype.GetLength())
    {
#ifdef XML_HXX
      if (doctype ==  XMLClass)
	{
	  //message_log (LOG_DEBUG, "Looking at XML to see what kind");
	  // Need to see what kind of XML
	  //
#ifdef NO_MMAP
	  STRING contents;
	  size_t checkL = ReadFile(s, &contents, 0, 4096);
	  if (checkL)
	  {

            const char *ptr = (const char *)contents.data();

#else
	  MMAP   mmap(s, MapSequential);
	  size_t checkL;
	  if (mmap.Ok() && (checkL=mmap.Size()) > 100)
	    {
	      if (checkL > 4096) checkL = 4095; // Check only first page 
	      const char *ptr = (const char *)mmap.Ptr();
#endif
	      size_t pos    = 0;

	      while (pos < checkL)
		{
		  // Scan for <
		  while (*ptr != '<' && pos < checkL) { pos++; ptr++; }
		  // if we are looking at a < 
		  if (pos < checkL)                   { pos++, ptr++; }
		  // then look for first char (skip spaces)
		  while (isspace(*ptr))               { pos++; ptr++; }
		  // Is it a charcter (not ? or !) ??
		  if (*ptr == '!' && (pos < checkL - 16))
		    {
		      // to handle some bad HTML markup that wants to be XHTML
		      if (strncasecmp(ptr, SGML_magic, sizeof(SGML_magic)-1) == 0)
			{
			  ptr += 8; pos += 8;
			  while (isspace(*ptr)) {pos++; ptr++; }
			  if (*ptr == '\'' || *ptr == '"') {pos++; ptr++;} // <!DOCTYPE "html"
			  if (strncasecmp(ptr, "html", 4) == 0 && !isalnum(ptr[4]))
			    doctype = HTMLClass;
			  else if (strncasecmp(ptr, "xhtml", 5) == 0 && !isalnum(ptr[5]))
			    doctype = HTMLClass;
			  else if (strncasecmp(ptr, "rss", 3) == 0 && !isalnum(ptr[3]))
			    doctype = RSSClass;
			  else if (strncasecmp(ptr, "gils", 4) == 0) // also support gilsxml
			    doctype = "GILSXML";
			  else if (strncasecmp(ptr, "play", 4) == 0 && !isalnum(ptr[4]))
			    doctype = SHAKESPEAREClass;
			  else if (strncasecmp(ptr, "docbook", 7) == 0)
			    doctype = DOCBOOKClass;
			  else if (strncasecmp(ptr, "tei", 3) == 0)
			    doctype = TEIClass;
			  else if (strncasecmp(ptr, "cap:", 4) == 0)
			    doctype = CAPClass;
#if 1
			  else if (strncasecmp(ptr, "Locator SYSTEM \"centroid.dtd\"" , 28) == 0)
			    doctype = CentroidClass;
			  else {
			    STRING subclass;
			    while (*ptr &&  isalnum(*ptr))
			      {
				subclass.Cat(*ptr++);
			      }
			    subclass.Cat(":XMLBASE");
			    doctype = subclass;
			  }
#endif
			}
		    }
		  else if (isalnum(*ptr))
		    break;
		  pos++, ptr++;
		}
	      if (pos < checkL - 16)
		{
		  // Have something is it <rss ...> or <rss> ?
		  //message_log(LOG_DEBUG, "Line = '%s'", ptr);
		  if (strncasecmp(ptr, "rss", 3) == 0 && (isspace(ptr[3]) || ptr[3] == '>'))
		    doctype = RSSClass;
		  else if (strncasecmp(ptr, "rdf:RDF", 7) == 0)
		    doctype ="RDF"; 
                  else if (strncasecmp(ptr, "gils", 4) == 0 && (isspace(ptr[4]) || ptr[4] == '>'))
		    doctype = "GILSXML";
		  else if (strncasecmp(ptr, "html", 4) == 0 && (isspace(ptr[4]) || ptr[4] == '>'))
		    doctype = HTMLClass;
		  else if (strncasecmp(ptr, "xhtml", 5) == 0 && (isspace(ptr[5]) || ptr[5] == '>'))
		    doctype = HTMLClass;
		  else if (strncmp(ptr, "newsitem", 8) == 0 && isspace(ptr[8]))
		    doctype = "REUTERS2:XMLBASE";
		  else if (strncmp(ptr, "NewsML", 6) == 0 && isspace(ptr[6]))
		    doctype = "NewsML";
		  else if (strncasecmp(ptr, "play", 4) == 0 && !isalnum(ptr[4]))
		    doctype = SHAKESPEAREClass;
		  else if (strncasecmp(ptr, "docbook", 7) == 0)
		    doctype = DOCBOOKClass;
		  else if (strncasecmp(ptr, "cap:", 4) == 0)
		    doctype = CAPClass;
		  else if (strncasecmp(ptr, "tei", 3) == 0)
		    doctype = TEIClass;
		  else if (strncasecmp(ptr, "atom:", 5) == 0 || (strncmp(ptr, "feed", 4) == 0 && !isalnum(ptr[4])))
		    doctype = AtomClass;

		}
	    }
	}
      else
#endif
#ifdef BINARY_HXX
      if (doctype == "TIFF")
	{
	  // Do we have a matching .txt ?
	  if (FileExists(beforeExt+".txt"))
	    doctype = "BINARY";
	  else if (FileExists(beforeExt+".xml"))
            doctype = "XBINARY";
	}
#else
	{}
#endif
      if (tagRegistry)
	{
	  STRING S; 

	  tagRegistry->ProfileGetString("Use", doctype,
		doctype == HTMLClass ? DefaultHTMLClass : doctype, &S);
	  // Make sure that our default HTML class has not been mapped!
	  if ((S == DefaultHTMLClass) && (DefaultHTMLClass != HTMLClass))
	    tagRegistry->ProfileGetString("Use", DefaultHTMLClass, DefaultHTMLClass, &S);

	  if (doctype != S)
	    {
	      if ((S ^= "NULL") || (S  ^= "<Ignore>")) // <Ignore> is for fields (see doctype.cxx) but we'll look for it here too
		{
		  message_log(LOG_INFO, "Identified %s as %s, Skipping ([Use] Request).",
			s.c_str(), doctype.c_str());
		  return;
		}
		message_log (LOG_INFO, "Using '%s' instead of '%s' [tagRegistry]", S.c_str(), doctype.c_str());
	       doctype = S;
	    }
	}
      else if (doctype == HTMLClass)
	doctype = DefaultHTMLClass;

      if (doctype.GetChr(1) == '"' || doctype.Search(' '))
	{

#if 0
          // Kludge
	  if (doctype.Search("JPEG")) {
	     doctype = "EXIF:"; // need to check beforehand that the plugin is available
	     // cerr << "USE EXIF: " << endl;
	     goto done;
	  } 
#endif

	  PFILE Fp = NULL;
	  off_t pos;
	  RECORD NewRecord (FileRecord);
          STRING key;

         if (!ParseInfo)
	   {
	     message_log (LOG_INFO, "Skipping '%s' (%s) [ParseInfo=Off]", s.c_str(), doctype.c_str() );
	     return;
	   }


	  if (FileRecord.GetRecordStart() == 0)
	    key.form("%lX-%lX", inode, (long)HostID);
	  else
            key.form("%lX-%lX-%lX", inode, (long)HostID, FileRecord.GetRecordStart());

	  if (Db->KeyLookup (key))
	    {
	      message_log (LOG_INFO, "Skipping '%s', Record already in Resource database?", (const char *)s );
	      return;
	    }
	  else if (kludge == false)
	    {
	      Db->ComposeDbFn(&InfoPath, DbExtDesc);
	      InfoPath.Cat (":");
	      InfoPath.Cat (++kludgeCount);
	      if (!Exists(InfoPath))
		{
		  if ((Fp = fopen_wb(InfoPath)) != NULL)
		    {
		      pos = 0;
#if 0 /* BREAKS THINGS */
		      static const char Warning[] = "\
D \010O \010N \010' \010T  \010E \010D \010I \010T\
  \010O \010R  \010R \010E \010M \010O \010V \010E\
  \010T \010H \010I \010S  \010F \010I \010L \010E\
 !!!!\000\000\004\032\n\032\n";
		      pos = fwrite(Warning, 1, sizeof(Warning)-1, Fp);
#if 1 /* NOT SURE THIS IS STILL NEEDED */
		      NewRecord.SetRecordStart (0);
		      NewRecord.SetRecordEnd ( pos - 1 );
		      NewRecord.SetFullFileName ( InfoPath );
	//	      NewRecord.SetDocumentType ( NilDoctype );
		      NewRecord.SetBadRecord();
		      Db->DocTypeAddRecord(NewRecord);
#endif
#endif
		    }
		}
	      kludge = true;
	    }
	  if (Fp == NULL)
	    {
	      if ((Fp = fopen_ab(InfoPath)) != NULL)
		pos = ftell(Fp);
	    }
	  if (Fp != NULL)
	    {
	      // Add a record.. When done the indexer looks to see
	      // if a .bin file exists and then parses and indexes
	      // it.
	      STRING basename (RemovePath(s));

	      NewRecord.SetRecordStart ( pos );
	      pos += fprintf(Fp, "Template-Type: Resource\n");
#if SUPPORT_PDF
	      if (mime == "application/pdf")
		GetPDFTitle(s, &basename);
	      else
#endif
	      if (mime == "image/gif")
		GetGIFTitle(s, &basename);
	      pos += fprintf(Fp, "Title: %s %s\n", basename.c_str(), doctype.c_str());

	      if ((stbuf.st_mode & S_IFREG) == S_IFREG)
		{
		  pos += fprintf(Fp, "Handle: %s\n", (const char *)key);
		  NewRecord.SetKey (key);

		  STRING Owner ( ResourceOwner ( s) );
		  if (Owner.GetLength())
		    {
		      pos += fprintf(Fp, "Author-Name-v1: %s\n", Owner.c_str());
		    }
		  STRING Publisher ( ResourcePublisher(NulString)); // Publisher HERE
		  if (Publisher.GetLength())
		    {
		      pos += fprintf(Fp, "Publisher-Name-v1: %s\n", Publisher.c_str());
		    }
		  pos += fprintf(Fp, "Last-Revision-Date-v1: %s (%s)\n",
			RFCdate(stbuf.st_mtime),
			ISOdate(stbuf.st_mtime));
		  NewRecord.SetDate ( stbuf.st_mtime );
		  pos += fprintf(Fp, "Size-v1: %ld bytes\n", stbuf.st_size);
		}
	      pos += fprintf(Fp, "Record-Last-Modified-Date: %s\n", RFCdate(0));
	      pos += fprintf(Fp, "Local-Path: %s\n", s.c_str());

	      if (mime.GetLength())
		{
		  pos += fprintf(Fp, "Format-v1: %s\n", mime.c_str());
		}
	      pos += fprintf(Fp, "\n"); // End-Of-Record
	      NewRecord.SetRecordEnd ( pos-1 );
	      NewRecord.SetFullFileName ( InfoPath );
	      if (subdoctype.GetLength())
		NewRecord.SetDocumentType ( subdoctype);
	      else
		NewRecord.SetDocumentType ( "RESOURCE" );
	      NewRecord.SetBadRecord(false); // Need to set OK
	      NewRecord.SetLanguage ("en");
	      fclose (Fp);
	      Db->DocTypeAddRecord(NewRecord);
	      message_log(LOG_NOTICE, "Identified %s as %s, parsing info only (%s).", s.c_str(),
		doctype.c_str(), NewRecord.GetDocumentType().ClassName(true).c_str());
	      // doctype = "0";
	      return; // 9 June 2003 edz, BUGFIX
	    }
	  else
	    {
	      message_log(LOG_NOTICE, "Identified %s as %s, format not supported", s.c_str(), doctype.c_str());
	      doctype = NulDoctype;
	    }
	}
      else
	{
	  message_log(LOG_INFO, "Processing %s as %s", s.c_str(), doctype.c_str() );
	  kludge = false;
	}

    }
  else
    {
      message_log(LOG_NOTICE, "%s not identified or doctype not supported.", s.c_str());
      doctype = NulDoctype;
    }

done:

  DOCTYPE_ID Id (doctype);
  if (Id.IsDefined())
    { 
      if (Db && Db->ValidateDocType(Id))
	{
	  message_log (LOG_DEBUG, "%s Handling as %s", Doctype.c_str(),
		 doctype.c_str());
	  Record.SetDocumentType (Id);
	  Db->ParseRecords (Record);
	}
      else
	message_log (LOG_INFO, "%s: DOCTYPE '%s' is not available. Check installation.", doctype.c_str()); 
    }
  else if (doctype != NulDoctype) {
    message_log (LOG_ERROR, "Can't handle %s files", doctype.c_str());
  } else
   message_log (LOG_DEBUG, "Null doctype");
}


void AUTODETECT::ParseFields(RECORD *RecordPtr)
{
  if (RecordPtr)
    {
      // Should never ParseFields here!!
      message_log (LOG_ERROR, "%s::ParseFields: thowing out record %s[%ld-%ld]",
	Doctype.c_str(),
	RecordPtr->GetFullFileName().c_str(),
	(long)RecordPtr->GetRecordStart(), (long)RecordPtr->GetRecordEnd() );
      RecordPtr->SetBadRecord();
    }
}


AUTODETECT::~AUTODETECT () 
{
}
