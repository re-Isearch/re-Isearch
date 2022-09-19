#pragma ident  "@(#)sgml.cxx  1.9 02/24/01 17:45:13 BSN"

/* ########################################################################

   File: sgml.cxx
   Version: 1.13
   Description: Class SGML - SGML documents
   Created: Thu Dec 28 21:38:30 MET 1995
   Author: Edward C. Zimmermann, edz@nonmonotonic.net
   Modified: Fri Dec 29 11:57:19 MET 1995
   Last maintained by: Edward C. Zimmermann


   ########################################################################

   Note: None 

   ########################################################################

   Copyright (c) 1995 : Basis Systeme netzerk. All Rights Reserved.

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.

   ######################################################################## */

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <string.h>
//#include <sys/stat.h>
//#include <errno.h>
#include "common.hxx"
#include "sgml.hxx"
#include "process.hxx"
#include "doc_conf.hxx"

static const char *Conversions[] = {"html", NULL};
static const char  sMagic[] = "<!-- SOURCE: ";
static const char  eMagic[] = " -->";

//
// Convert SGML file to DB.cat/KEY.sgm
//
// KEY := inode.hostid
//
//
static long HostID = 0;

static STRING DefaultSgmlCatalog ("sgml_catalog");
static STRING DefaultDSSSL_Spec ("dsssl_spec");

SGML::SGML (PIDBOBJ DbParent, const STRING& Name): SGMLNORM (DbParent, Name)
{
  if (HostID == 0)
    HostID = _IB_Hostid(); 

  if (!FileExists(Catalog = ResolveConfigPath(Getoption("Catalog", DefaultSgmlCatalog))))
    {
      if (Catalog != DefaultSgmlCatalog)
	logf (LOG_WARN, "SGML Catalog '%s' not found", Catalog.c_str());
      Catalog.Clear();
    }
  if (!FileExists(DSSSL_Spec = ResolveConfigPath(Getoption("DSSSL", DefaultDSSSL_Spec))))
    {
      if (DSSSL_Spec != DefaultDSSSL_Spec);
	logf (LOG_WARN, "DSSSL Specification '%s' not found", Catalog.c_str());
      DSSSL_Spec.Clear();
    }

  SGMLNorm_command = ResolveBinPath(Getoption("Sgmlnorm", "sgmlnorm"));
  JadeCommandPath  = ResolveBinPath(Getoption("Jade", "jade"));
}

const char *SGML::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("SGML");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  SGMLNORM::Description(List);
  return "ISO SGML documents\n\
  Options:\n\
    Sgmlnorm=<path> // Path to an alternative SGML normalizer\n\
    Jade=<path>     // Path to the Jade program executable\n\
    Catalog=<path>  // Path to the SGML catalog\n\
    DSSSL=<path>    // Path to the DSSSL specification\n\
  These may also be specified in the sgml.ini (or xxx.ini for xxx=sgml) as\n\
  Catalog=path and DSSSL=path in the [General] section."; 
}


void SGML::ParseRecords(const RECORD& FileRecord)
{
  STRING Fn;
  struct stat stbuf;
  STRING s;

  if (SGMLNorm_command.IsEmpty())
    {
      Db->DocTypeAddRecord(FileRecord);
      return;
    }


  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1)
    {
      logf (LOG_ERRNO, "Can't create filter directory '%s'", (const char *)s );
      return;
    }

  FileRecord.GetFullFileName (&Fn);
  if (_IB_lstat(Fn, &stbuf) == -1)
    {
      logf(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
#ifndef _WIN32
  if (stbuf.st_mode & S_IFLNK)
    {
      if (stat(Fn, &stbuf) == -1)
        {
          logf(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
          return;
        }
    }
#endif


  STRING key, outfile, outbase;
  key.form("%lx%lx", stbuf.st_ino, HostID);
  outbase.form ("%s/%s.", (const char *)s, (const char *)key);
  outfile = outbase;
  outfile.Cat ("sgm");

  PFILE oFp = fopen(outfile, "w");
  if (oFp == NULL)
    {
      logf (LOG_ERRNO, "Can open output for SGML normalization: '%s'",
	(const char *)outfile);
      return;
    }

  const char *argv[6];
  int   argc = 0;
  argv[argc++] = SGMLNorm_command.c_str();
  argv[argc++] = "-d";

  if (Catalog.GetLength())
    {
      argv[argc++] = "-c";
      argv[argc++] = (char *)Catalog.c_str();
    }
  argv[argc++] = (char *)Fn.c_str();
  argv[argc++] = NULL;

  logf(LOG_DEBUG, "Running %s", argv[0]);
  /*
    need to set SP_ENCODING=iso-8859-x
  */
  PFILE Fp = _IB_popen(argv, "r");
  if (Fp == NULL)
    {
      logf(LOG_ERRNO, "Can't open pipe to %s (SGML Normalizer).", argv[0]);
      SGMLNorm_command.Clear();
      return;
    }
  int ch;
  long end = 0;

  end += fprintf(oFp, "%s%s%s\n", sMagic, (const char *)Fn, eMagic);
  if (Catalog.GetLength())
    end += fprintf(oFp, "<!-- CATALOG: %s -->\n", (const char *)Catalog);
  while ((ch = getc(Fp)) != EOF)
    {
      putc(ch, oFp);
      end++;
    }
  _IB_pclose (Fp);
  fclose(oFp);
  logf(LOG_DEBUG, "Wrote %ld bytes to %s", end, (const char *)outfile);

  if (end < 3)
    {
      logf (LOG_ERRNO, "Normalization failed!");
      return;
    }

  STRING Dsssl;
  if (DSSSL_Spec.GetLength())
    {
      Dsssl = DSSSL_Spec;
    }
  else
    {
      STRING Tmp (Fn);
      STRINGINDEX pos = Tmp.SearchReverse('.');
      if (pos > 0)
	Tmp.EraseAfter(pos-1);
      Tmp.Cat (".dsl");
      // Do we have a xxxx.dsl?
      if (!FileExists (Tmp))
	{
	  // No? Do we have a "this.dsl"?
	  RemoveFileName(&Tmp);
	  Tmp.Cat ("this.dsl");
	  if (!FileExists (Tmp))
	    {
	      logf (LOG_ERROR, "-o DSSSL=??? Not set to DSSSL Specification.");
	      return;
	    }
	}
      if (Tmp.GetLength())
	Dsssl = Tmp;
    }
  for (int i=0; Conversions[i]; i++)
    {
      const char  *argv[11];
      int    argc;
      STRING s;

      argc = 0;
      argv[argc++] = JadeCommandPath.c_str();
      if (Catalog.GetLength())
	{
	  argv[argc++] = "-c";
	  argv[argc++] = Catalog.c_str();
	}
      argv[argc++] = "-t";
      argv[argc++] = (char *)Conversions[i];
      if (Dsssl.GetLength())
	{
	  argv[argc++] = "-d";
	  argv[argc++] = Dsssl.c_str();
	}
      argv[argc++] = "-o";
      argv[argc++] = (char *) (s = outbase + Conversions[i]).c_str(); 
      argv[argc++] = Fn.c_str();
      argv[argc] = NULL;

      logf(LOG_DEBUG, "Running %s", argv[0]);

      if (_IB_system(argv) < 0)
 	logf (LOG_ERRNO, "Could not run '%s'", argv[0]);

      if (strcmp(Conversions[i], "html") == 0)
	{
	  // Mangle html..
	  //<LINK REL=STYLESHEET TYPE="text/css" HREF="4ef7230523a.css">
	  STRING Html;
	  STRING FileName;
	  FileName = outbase;
	  FileName.Cat ("html");
	  Html.ReadFile (FileName);
	  if (Html.GetLength())
	    {
	      static const char Magic[] = "<LINK REL=STYLESHEET TYPE=\"text/css\" HREF=\"";
	      STRINGINDEX pos = Html.Search(Magic);
	      if (pos > 0)
		{
		  Html.Insert(pos+sizeof(Magic)-1, "ifetch?XXX+");
		  if ((pos = Html.Search(".css", pos+sizeof(Magic)-1)) > 0)
		    {
		      Html.SetChr(pos, '+');
		    }
		}
	      Html.WriteFile (FileName);
	    }
	}
    }


  RECORD Record (FileRecord);
  Record.SetKey (key);
  Record.SetFullFileName (outfile);
  Record.SetRecordStart (0);
  Record.SetRecordEnd (end);
  Db->DocTypeAddRecord(Record);
}

void SGML::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
   const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  *StringBuffer = "";
  if (ElementSet ^= "css")
    {
      STRING fn;
      ResultRecord.GetFullFileName(&fn);
      if (RecordSyntax == HtmlRecordSyntax)
	StringBuffer->Cat ("Content-type: text/css\n\n");
      fn.Replace(".sgm", ".css");
      StringBuffer->CatFile (fn);
      return;
    }
  else if (ElementSet == SOURCE_MAGIC)
    {
      STRING fn;
      ResultRecord.GetFullFileName(&fn);
      PFILE fp = fopen(fn, "rb");
      if (fp)
	{
	  const GPTYPE rs = ResultRecord.GetRecordStart();
	  if (fseek(fp, rs, SEEK_SET) != -1)
	    {
	      char tmp[1024];
	      if (fgets(tmp, sizeof(tmp), fp) != NULL)
		{
		  STRING mime;
		  if (RecordSyntax == HtmlRecordSyntax)
		    {
		      SGMLNORM::SourceMIMEContent(ResultRecord, &mime);
		      *StringBuffer << "Content-type: " << mime << "\n\n";
		    }
		  if (strncmp(tmp, sMagic, sizeof(sMagic)-1) == 0)
		    {
		      char *tcp = strstr(tmp, eMagic);
		      if (tcp)
			*tcp = '\0';
		      StringBuffer->CatFile (tmp+sizeof(sMagic)-1);
		      return;
		    }
		}
	    }
	}
    }
  SGMLNORM::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


SGML::~SGML ()
{
}
