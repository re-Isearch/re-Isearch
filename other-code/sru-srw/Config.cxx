// $Id: Config.cxx 154 2006-04-12 18:09:40Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/


/*@@@
File:          	isrch_util.cxx
Version:        1.0
$Revision$
Description:    Generic utilities for Isearch-cgi
Authors:        Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include "Config.hxx"
#define __DC_URL "info:srw/cql-context-set/1/dc-v1.1"


CONFIG::CONFIG() {
  Init();
}


void
CONFIG::Init() {
  STRING DefaultPath, Path, ConfigFileName;
  STRING Section, Entry, Hold;
  STRING File;
  STRLIST Position;

  // Create the registry to hold the init parameters
  Defaults = new REGISTRY("Isearch-sru");

  // Look for the config file
  DefaultPath = "./"; // we need some default if no config file

  // look in the current directory
  File = DefaultPath;
  File.Cat(CONFIG_FILE);
  if (IsFile(File)) {
    ConfigFileName = File;

  } else {
    // not there, so look in /cgi-bin
    File = CGI_DIR;
    File.Cat(CONFIG_FILE);
    if (IsFile(File)) {
      ConfigFileName = File;

    } else {
      // look look in /etc
      File = "/etc/";
      File.Cat(CONFIG_FILE);
      if (IsFile(File))
	ConfigFileName = File;
      else {
	// Nothing to load, so bail out
	ConfigFileName = "";
	return;
      }
    }
  }

  // Now, load the global defaults from the init file into the registry
  Position.Clear();
  Defaults->ProfileLoadFromFile(ConfigFileName, Position);

  // Fill in the object elements
  Section = "Default";

  Entry="Server";
  Defaults->ProfileGetString(Section, Entry, "localhost", &ServerName);
  
  Entry="Version";
  Defaults->ProfileGetString(Section, Entry, "1.1", &SRU_Version);

  Entry="Title";
  Defaults->ProfileGetString(Section, Entry, "(no title)", &ServerTitle);

  Entry="Description";
  Defaults->ProfileGetString(Section, Entry, "(no description)", &ServerDescription);

  Entry="SRU";
  Defaults->ProfileGetString(Section, Entry, "/cgi-bin", &ServerSruPath);

  Entry="Lang";
  Defaults->ProfileGetString(Section, Entry, "en", &ServerLanguage);

  Entry="DTD";
  Defaults->ProfileGetString(Section, Entry, "isru.dtd", &IsruDtdUri);

  Entry="XSL";
  Defaults->ProfileGetString(Section, Entry, "isru.xsl", &IsruXslUri);

  Entry="Context";
  Defaults->ProfileGetString(Section, Entry, "dc", &Hold);
  if (Hold.GetLength() > 0)
    ContextSets.Split(',',Hold);

  Entry="DebugLevel";
  Defaults->ProfileGetString(Section, Entry, "0", &Hold);
  DebugLevel = Hold.GetInt();

  Entry="Port";
  Defaults->ProfileGetString(Section, Entry, "80", &Hold);
  Port = Hold.GetInt();

  Entry="Status";
  Defaults->ProfileGetString(Section, Entry, "ok", &ServerStatus);

  // Now, find and load the database registry
  // and the registry for the database-specific info
  dbinfo = new REGISTRY("database");

  // Now load in the URLs for the context sets
  Section = "context";

  Entry="geo";
  Defaults->ProfileGetString(Section, Entry, "", &ContextUrlGeo);

  Entry="gils";
  Defaults->ProfileGetString(Section, Entry, "", &ContextUrlGils);

  Entry="dc";
  Defaults->ProfileGetString(Section, Entry, "", &ContextUrlDc);

  Entry="cql";
  Defaults->ProfileGetString(Section, Entry, "", &ContextUrlCql);

  Entry="rec";
  Defaults->ProfileGetString(Section, Entry, "", &ContextUrlRec);

  // Now load the database configuration file
  Entry="DBConfig";
  Defaults->ProfileGetString(Section, Entry, "Isearch-db.sru", 
			     &Hold);

#ifdef DEBUG
  FILE* fp = fopen("/tmp/isearch-sru.txt", "a");
#endif

  // First see if they just gave us a valid location
  if (IsFile(Hold)) {
    DbConfigFileName = Hold;

  } else {
    // look in the current directory
    File = DefaultPath;
    File.Cat(Hold);
    if (IsFile(File)) {
      DbConfigFileName = File;

    } else {
      // not there, so look in /cgi-bin
      File = CGI_DIR;
      File.Cat(Hold);
      if (IsFile(File)) {
	DbConfigFileName = File;

      } else {
	// look look in /etc
	File = "/etc/";
	File.Cat(Hold);
	if (IsFile(File))
	  DbConfigFileName = File;
	else {
	  // Nothing more to load, so bail out
	  DbConfigFileName = "";
	  return;
	}
      }
    }
  }

#ifdef DEBUG
  CHR *lbuf;
  lbuf=DbConfigFileName.NewCString();
  fprintf(fp, "Loading DB Config from %s\n",lbuf);
  delete [] lbuf;
#endif

  // Now, load the global defaults from the init file into the registry
  Position.Clear();
  dbinfo->ProfileLoadFromFile(DbConfigFileName, Position);

  Section="database";
  Entry="DBList";
  dbinfo->ProfileGetString(Section, Entry, "", &Hold);
  if (Hold.GetLength() > 0)
    DBList.Split(',',Hold);

#ifdef DEBUG
  lbuf=Hold.NewCString();
  fprintf(fp, "DBList= %s\n",lbuf);
  delete [] lbuf;
#endif

#ifdef DEBUG
  fclose(fp);
#endif

  return;
}


STRING
CONFIG::FetchDiagnostic(INT DiagCode) {
  if (DiagCode < 0) return "";

  STRING Section, Entry, Hold;
  STRING DefaultPath = "General system error"; // we need some default if no config file

  Section = "diagnostics";
  Entry=DiagCode;
  Defaults->ProfileGetString(Section, Entry, DefaultPath, &Hold);
  return Hold;
}


STRING
CONFIG::FetchContextUrl(STRING Context) {
  if (Context.GetLength() <= 0) return "";

  STRING Section, Entry, Hold;
  STRING DefaultPath = __DC_URL; // we need some default if no config file

  Section = "context";
  Entry=Context;
  Defaults->ProfileGetString(Section, Entry, DefaultPath, &Hold);
  return Hold;
}


void
CONFIG::LoadDbInfo(DBINFO* ThisDb) {
  STRING Section, Entry, Hold;
  STRING DefaultPath = "./"; // we need some default if no config file

  Section = ThisDb->GetDbName();
  Entry="Path";
  //Defaults->ProfileGetString(Section, Entry, DefaultPath, &Hold);
  dbinfo->ProfileGetString(Section, Entry, DefaultPath, &Hold);
  if (Hold.GetLength() > 0)
    ThisDb->PutDbPath(Hold);

  Entry="Title";
  //Defaults->ProfileGetString(Section, Entry, "(no title)", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "(no title)", &Hold);
  ThisDb->PutDbTitle(Hold);

  Entry="Type";
  //Defaults->ProfileGetString(Section, Entry, "Isearch", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "Isearch", &Hold);
  ThisDb->PutDbType(Hold);

  Entry="IndexName";
  //Defaults->ProfileGetString(Section, Entry, "XXXXX", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "XXXXX", &Hold);
  ThisDb->PutDbIndexName(Hold);

  Entry="Description";
  //  Defaults->ProfileGetString(Section, Entry, "(no description)", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "(no description)", &Hold);
  ThisDb->PutDbDescription(Hold);

  Entry="Language";
  //  Defaults->ProfileGetString(Section, Entry, "en", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "en", &Hold);
  ThisDb->PutDbLanguage(Hold);

  Entry="FieldMaps";
  //  Defaults->ProfileGetString(Section, Entry, "", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "", &Hold);
  ThisDb->PutDbFieldMaps(Hold);

  Entry="Synonyms";
  //  Defaults->ProfileGetString(Section, Entry, "no", &Hold);
  dbinfo->ProfileGetString(Section, Entry, "no", &Hold);
  ThisDb->PutDbSynonyms(Hold);
}


CONFIG::~CONFIG() {
  if (dbinfo)
    delete dbinfo;
  if (Defaults)
    delete Defaults;
}
