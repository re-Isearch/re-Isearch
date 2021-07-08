#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "defs.hxx"
#include "string.hxx"
#include "common.hxx"
#include "mime.hxx"
#include "process.hxx"

/* ASSOCIATE EXTENTSION WITH FILE TYPE/DOCTYPE */
static struct {
  const char *extension;
  const char *mimeType;
  const char *description;
  const char *doctype;
} Builtin[] = {
  {".txt",	"text/plain", "Plain Text", "PLAINTEXT"},
  {".gif",	"image/gif",  "GIF Image",  "IMAGE"},
  {".jpg",	"image/jpep", "JPEG Image", "IMAGE"},
  {".mpg",      "image/mpeg", "MPEG Video", "IMAGE"},
  {".tif",	"image/x-tiff", "TIFF Image", "IMAGE"},
  {".tiff",	"image/x-riff", "TIFF Image", "IMAGE"},
  {".fax",	"image/x-tiff", "TIFF Image", "IMAGE"},
  {".bmp",	"image/x-bmp",  "Windows BMP Image", "IMAGE"},
  {".pcx",	"image/x-pcx",  "PC Paintbrush Image", "IMAGE"},
  {".htm",	"text/html",  "W30 HTML", "HTML"},
  {".html",	"text/html",  "W3O HTML", "HTML"},
  {".sgm",	"text/sgml",  "Normalized SGML", "SGMLNORM"},
  {".sgml",	"text/sgml",  "Normalized SGML", "SGMLNORM"},
  {".xml",	"text/xml",   "W3C XML",  "XML"},
  {".gz",	"application/x-gzip", "Gnu Zipped", NULL},
  {".tgz",	"application/x-gzip", "Gnu Zipped", NULL},
  {".Z",	"application/x-compress", "Unix Compressed", NULL},
  {".tar",	"application/x-tar",  "Tar Archive", NULL},
  {".cpio",	"application/x-cpio", "CPIO Archive", NULL},
  {".dvi",	"application/x-dvi",  "TeX DVI", NULL},
  {".tex",	"application/x-tex",  "TeX Source", "PLAINTEXT"},
  {".ltx",	"application/x-latex","LaTeX Source", "PLAINTEXT"},
  {".latex",	"application/x-latex","LaTeX Source", "PLAINTEXT"},
  {".zip",	"application/zip",    "Zip Archive", NULL},
  {".zoo",	"application/x-zoo",  "Zoo Archive", NULL},
  {".lha",	"application/x-lharc","LHarc Archive", NULL},
  {".arc",	"application/x-arc",  "ARC Archive", NULL},
  {".ps",	"application/postscript", "Postcript", "IMAGE"},
  {".eps",      "application/postscript", "Postscript", "IMAGE"},
  {".mif",	"application/x-mif",  "Framemaker MIF", NULL},
  {".wp5",	"application/x-wp5",  "WordPerfect 5.x", NULL},
  {".wp4",      "application/x-wp4",  "WordPerfect 4.x", NULL},
  {".doc",      "application/x-msword", "MS Word", NULL},
  {".xls",      "application/x-excel", "Excel spreadsheet", NULL},
  {NULL, NULL, NULL}
};

/* ASSOCIATE CONTENT MAGIC WITH FILE TYPE/DOCTYPE */
/* string magic */
static struct {
  unsigned char offset;
  const char *str;
  unsigned short len;
  const char *doctyp; 
  const char *description;
  const char *mimeType;
} Magic[] = {
   {0, "\151\006", 2, "\"IB 2.0 Index\""},
   {0, "\151\007", 2, "\"IB 2.0 Index (hyphenation)\""},
   {0, "\037\213", 2, "\"gzip compressed data\"", "application/gzip"},
   {0, "\037\235", 2, "\"Unix compressed data\"", "application/compress"},
   {0, "PK\003",   3, "\"Zip archive\"", "application/zip"},
   {0, "\177ELF", 4, NULL, "Executable  and  Linking Format (ELF) object"},
   {0, "From ", 5, "MAILFOLDER", "Unix Mailfolder"},
   {0, "Article ", 8, "NEWSFOLDER", "Usenet News folder"},
   {0, "Entry_ID:", 9, "DIF", "Directory Interchange Format (DIF)"},
   {0, "This is Info file ", 18, "EMACSINFO", "GNU Emacs Info file"},
   /* TeX System stuff */
   {0, "\367\002", 2, NULL, "TeX DVI file"},
   {0, "\367\203", 2, NULL, "TeX generic font data"},
   {0, "\367\131", 2, NULL, "TeX packed font data"},
   {0, "\367\312", 2, NULL, "TeX virtual font data"},
   {0, "This is TeX", 11, NULL, "TeX transcript text"},
   {0, "This is METAFONT", 16, NULL, "METAFONT transcript text"},

   /* Adobe */
   {0, "%PDF-2", 6, NULL, "Adobe Portable Document Format (PDF) v2.x"},
   {0, "%PDF-1", 6, NULL, "Adobe Portable Document Format (PDF) v1.x"},
   {0, "%PDF-",  5, NULL, "Adobe Portable Document Format (PDF)"},
   {0, "%!PS-Adobe-4", 12, NULL, "Adobe Postscript (PS) v4.x"},
   {0, "%!PS-Adobe-3", 12, NULL, "Adobe Postscript (PS) v3.x"},
   {0, "%!PS-Adobe-2", 12, NULL, "Adobe Postscript (PS) v2.x"},
   {0, "%!PS-Adobe-1", 12, NULL, "Adobe Postscript (PS) v1.x"},
   {0, "%!PS-Adobe-", 11, NULL, "Adobe Postscript (PS)"},
   {0, "%!PS-", 5, NULL, "Postscript (PS)"},

   /* FrameMaker */
   {0, "<MakerFile",  10,      NULL, "FrameMaker document"},
   {0, "<MIFFile", 8,          NULL, "FrameMaker MIF file"},
   {0, "<MakerDictionary", 16, NULL, "FrameMaker Dictionary text"},
   {0, "<MakerScreenFon", 15,  NULL, "FrameMaker Font file"},
   {0, "<MML", 4,      NULL, "FrameMaker MML file"},
   {0, "<Book", 5,     NULL, "FrameMaker Book file"},
   {0, "<Maker", 6,    NULL, "FrameMaker Intermediate Print File (IPL)"},
   {0, "<!SQ DTD>", 9, NULL, "SoftQuad Compiled SGML rules file"},
   {0, "<!SQ A/E>", 9, NULL, "SoftQuad A/E SGML Document binary"},
   {0, "<!SQ STS>", 9, NULL, "SoftQuad A/E SGML binary styles file"},

   /* Misc junk */
   {0, "Interpress/Xerox", 16, NULL, "Xerox Interpress document"},
   {0, "\037\235", 2, NULL, "Compressed data"},
   {0, "WORKSET VERSION ", 16, NULL, "SunSoft Tools Workset"},
   {0, "! Generated Code", 16, NULL, "SunSoft Visual Resource"},
   {0, "#!/bin/sh", 9, NULL, "Borne Shell Script", "application/x-script"},
   {0, "#!/bin/csh", 9, NULL, "'C' Shell Script", "application/x-script"},
   {0, "#!/bin/ksh", 9, NULL, "Korn Shell Script", "application/x-script"},
   {0, "#!/bin/zsh", 9, NULL, "'Z' Shell Script", "application/x-script"},
   {0, "#!/usr/local", 12, NULL, "Local Script", "application/x-script"},
   {0, "#!/usr/contrib", 14, NULL, "Contrib Script", "application/x-script"},
   {0, "#!/opt/", 7, NULL, "Option Script", "application/x-script"},
   {0, "#!/", 12, NULL, "Script Program", "application/x-script"},
   {0, "=<ar>", 5, NULL, "Unix .a archive (lib)", },
   {0, "!<arch>", 7, NULL, "Unix .a archive (lib)"},
   {0, "070701", 6, NULL, "ASCII cpio archive", "application/x-cpio"},
   {0, "070702", 6, NULL, "ASCII cpio archive - CRC header", "application/x-cpio"},
   {0, "070707", 6, NULL, "ASCII cpio archive - CHR (-c) header", "application/x-cpio"},
   {0, "xbtoa Begin", 11, NULL, "btoa'd file"},
   {0, "SIT!", 4, NULL, "StuffIt (Macintosh) archive"},
   {0, "\115\115", 2, "IMAGE", "TIFF file, big-endian", "image/x-tiff"},
   {0, "\111\111", 2, "IMAGE", "TIFF file, little-endian", "image/x-tiff"},
   {0, "GIF87a", 6, "IMAGE", "GIF file, v87", "image/gif"},
   {0, "GIF89a", 6, "IMAGE", "GIF file, v89", "image/gif"},
   {0, "\377\330\377\340", 4, "IMAGE", "JPEG file", "image/jpeg"},
   {0, "\377\330\377\356", 4, "IMAGE", "JPG file", "image/jpeg"},
   {8, "ILBM", 4, NULL, "IFF ILBM file", "application/x-iff"},
   {0, "ZyXEL\002", 6, NULL, "ZyXEL voice data", "application/x-zyxel-voice"},
   {0, "BEGMF", 5, NULL, "CGM clear text encoding", "application/cgm"},
   {0, "BegMF", 5, NULL, "CGM clear text encoding", "application/cgm"},
   {0, "\060\040\176", 3, NULL, "CGM character encoding", "application/cgm"},
   {0, "\060\040\033\134\033\130", 6, NULL, "CGM character encoding", "application/cgm"},
   {0, ".snd", 4, NULL, "Audio File", "audio/basic"},
   {36, "acsp", 4, NULL, "Kodak Color Management System, ICC Profile", "application/x-icc-profile"},
   {0, "#SUNPC_CONFIG", 13, NULL, "SunPC 4.0 Properties Values", "application/x-sunpc-config"},
   {0, NULL, 0, NULL, NULL}
};

#include "dtreg_list.hxx"

/*
#define strncasecmp StrNCaseCmp
*/

// PATH TO OUR FILE COMMAND
#define FILE_CMD "/opt/BSN/bin/file"

GDT_BOOLEAN FILETYP::Doctype(PSTRING StringBufferPtr)
{
  *StringBufferPtr = doctype;
  return StringBufferPtr->GetLength() != 0;
}

GDT_BOOLEAN FILETYP::Author(PSTRING StringBufferPtr)
{
  *StringBufferPtr = author;
  return StringBufferPtr->GetLength() != 0;
}

GDT_BOOLEAN FILETYP::MimeType(PSTRING StringBufferPtr)
{
  *StringBufferPtr = mimeType;
  return StringBufferPtr->GetLength() != 0;
}

GDT_BOOLEAN FILETYP::FormatDesc(PSTRING StringBufferPtr)
{
  *StringBufferPtr = description;
  return StringBufferPtr->GetLength() != 0;
}

GDT_BOOLEAN FILETYP::Summary(PSTRING StringBufferPtr)
{
  *StringBufferPtr = summary;
  return StringBufferPtr->GetLength() != 0;
}

// Creation
FILETYP::FILETYP (const STRING& FileName, const char *FileCmd) 
{
  STRING s;
  // Check some content
  PFILE fp = fopen(FileName, "rb");
  if (fp)
    {
      char buf[128];
      if (fgets(buf, sizeof(buf)/sizeof(char)-1, fp) != NULL)
	{
	  for (size_t i=0; Magic[i].str; i++)
	    {
	      if (strncmp(buf+Magic[i].offset, Magic[i].str, Magic[i].len) == 0)
		{
		  doctype = Magic[i].doctyp;
		  description = Magic[i].description;
		  mimeType = Magic[i].mimeType ? Magic[i].mimeType :
			"application/octet-stream";
		}
	    }

	  if (doctype.GetLength() == 0 &&
		(strncasecmp(buf, "Subject:", 8) == 0 ||
		   strncasecmp(buf, "From:", 5) == 0 ||
		   strncasecmp(buf, "Date:", 5) == 0))
	    {
	      // Look for ------
	      char seps[] = "_-=+";
	      char *tcp;
	      while (fgets(buf, sizeof(buf)/sizeof(char)-1, fp) != NULL)
		{
		  if ((tcp = strchr(seps, buf[0])) != NULL)  
		    if (buf[1] == *tcp && buf[2] == *tcp && buf[3] == *tcp)
		      {
			doctype = "MEMODOC";
			break;
		      }
		}
	    } 
	}
      fclose(fp);
    }

  if (!doctype.GetLength())
    {
      STRINGINDEX pos = s.SearchReverse ('.');
      if (pos)
	{
	  STRING ext = s;
	  ext.EraseBefore(pos);
	  // Now look for extension
	  for (size_t i = 0; Builtin[i].extension; i++)
	    {
	      if (ext.Equals(Builtin[i].extension))
		{
		  doctype = Builtin[i].doctype;
		  description = Builtin[i].description;
		  mimeType = Builtin[i].mimeType;
		  break;
		}
	    }
	}
      if (!doctype.GetLength())
	{
	  // Binary type?
	  if (Exist (s + ",info"))      doctype = "BINARY";
	  else if (Exist (s + ".iafa")) doctype = "FTP";
	}
    }
  else if (doctype.GetChr(1) == '"')
    {
      /* Is this to be processed as a binary of FTP? */
      // Binary type?
      if (Exist (s + ",info"))      doctype = "BINARY";
      else if (Exist (s + ".iafa")) doctype = "FTP";
    }
  if (!doctype.GetLength())
    {
      // Re-open and check if maybe HTML, SGMLNORM or SGMLTAG
      // -- we do this now since GILS and SGMLTAG look alike
      // <!DOCTYPE HTML means HTML, <HTML> means HTML, <HEAD> means HTML
      static const char SGML_magic[] = "<!DOCTYPE";
      if ((fp = fopen(s, "rb")) != NULL)
	{
	  char buf[512];
	  char *tcp, *tp;
	  while (fgets(buf, sizeof(buf)/sizeof(char)-1, fp) != NULL)
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
	      if (buf[0] == '%' && isupper(buf[1]))
		{
		  doctype = "REFERBIB";
		  break;
		}
	      if (isupper(buf[0]) && isupper(buf[1]) &&
			buf[2] == ' ' && buf[3] == ' ' && 
			(buf[4] == '-' || buf[4] == ' '))
		{
		  if (strncmp(buf, "SI  -", 5) == 0) doctype = "DVBLINE";
		  else                               doctype = "MEDLINE";
		}
	     if (*tcp != '<')
		{
		  if (*tcp == '@' || *tcp == '%')
		    {
		      doctype = "BIBTEX";
		    }
		  // Alphanum + -_$. 
		  do {
		    tcp++;
		  } while (isalnum(*tcp) || *tcp == '-' || *tcp == '_' ||
			*tcp == '$' || *tcp == '.');
		  if (*tcp == ':')
		    {
		      // Check if IAFA is in name
		      STRING base = s;
		      // Strip path
		      RemovePath(&base);
		      // Uppercase it
		      base.ToUpper();
		      if (base.Search("IAFA"))
			doctype = "IAFADOC";
		      else if (strncmp(buf, "Template-Type", 13) == 0)
			doctype = "ROADS++";
		      else if (strncmp(buf, "Template", 8) == 0)
			doctype = "IKNOWDOC";
		      else
			doctype = "COLONDOC";
		    }
		  break; // Not SGML-like family
		}
	      // Want <?XML> <?XML ..> or <?XMLDOC ..>
	      if ((tp = strstr(buf, "<?XML")) != NULL)
		{
		  if (isspace(tp[5]) || tp[5] == '>' ||
			strncasecmp(tp+5, "DOC ", 4) == 0)
		  doctype = "XML";
		  break;
		}
	      if ((tp = strstr(buf, SGML_magic)) != NULL)
		{
		  tp += sizeof(SGML_magic);
		  while (isspace(*tp)) tp++;
		  if (strncasecmp(tp, "HTML",  4) == 0)
		    doctype = "HTML";
		  else
		    doctype = "SGMLNORM";
		  break;
		}
	      else if (*(tcp + 1) != '!')
		{
		  // Looks like we start we a tag..
		  if (strncasecmp(tcp, "<HTML", 5) == 0 ||
		      strncasecmp(tcp, "<HEAD", 5) == 0)
		    {
		      doctype = "HTML";
		    }
		  else if (strncasecmp(tcp, "<?XML", 5) == 0)
		    {
		      doctype = "XML";
		    }
		  else if (strncasecmp(tcp, "<REC", 4) == 0 ||
		      strncasecmp(tcp, "<GILS", 5) == 0)
		    {
		      doctype = "GILS";
		    }
		  else
		    {
		      doctype = "SGMLTAG";
		    }
		  break;
		}
	    }
	  fclose(fp);
	}
    }

  if (doctype.GetLength() == 0)
    {
      // Lets see what file(1) says..
      char *cmd[3];
      const char *file_cmd = FileCmd ? FileCmd : FILE_CMD;
      if (!::Exist(file_cmd))
 	{
	  file_cmd = ResolveBinPath(file_cmd);
	  if (!::Exist(file_cmd))
	    file_cmd = ResolveBinPath("file");
	}
      cmd[0] = file_cmd;
      cmd[1] = s.c_str();
      cmd[2] = NULL;

      FILE *fp = _IB_popen(cmd, "r");
      if (fp)
	{
	  if (cmd.FGet(fp, 512))
	    {
	      if (cmd.Search(" program text"))
		{
		  logf(LOG_INFO, "Identified %s as '%s', not supported",
			(const char *)s, (const char *)cmd);
		  description = cmd;
		  doctype = "NULL";
		}
              else if (cmd.Search(" text"))
		{
		  description = cmd;
		  doctype = "PLAINTEXT";
		  logf(LOG_INFO, "Identified %s as %s, using %s",
                        (const char *)s, (const char *)cmd,
                        (const char *)doctype );
		}
	      else
		logf(LOG_INFO, "Using default for %s: %s",
			(const char *)s, (const char *)cmd);
	    }
	  _IB_pclose(fp);
	}
      else
	logf(LOG_NOTICE, "%s not identified or doctype not supported.",
		(const char *)s);
    }

  author = ResourceOwner (FileName); 

  if (doctype.GetLength() == 0)
    {
      mimeType = "Application/Octet-Stream";
      doctype = "NULL";
      description = "Unrecognized document format";
    }
  else if (description.GetLength() == 0)
    {
      if ((doctype == "NULL") || (doctype == NULL))
	{
	  if (mimeType.GetLength() == 0)
	    mimeType = "Application/Octet-Stream";
	  description = "Misc. resource";
	}
      else
	{
	  DTREG dtreg (NULL);
	  PDOCTYPE DoctypePtr = dtreg.GetDocTypePtr (doctype);
	  // Associate description and mimeType
	  if (DoctypePtr)
	    {
	      DoctypePtr->SourceMIMEContent(&mimeType);
	    }
	}
    }
}

FILETYP::~FILETYP () 
{
}
