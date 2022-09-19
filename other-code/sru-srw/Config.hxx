// $Id: Config.hxx 154 2006-04-12 18:09:40Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/

/*@@@
File:          	Config.hxx
Version:        1.0
$Revision$
Description:    Generic configuration utilities
Authors:        Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#ifndef _CONFIG_HXX_
#define _CONFIG_HXX_

#include <iostream.h>
#include <stdio.h>
#include <string.h>

#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "strlist.hxx"
#include "registry.hxx"
#include "DbInfo.hxx"

//#define CGI_DIR "/var/www/cgi-bin/"
#define CGI_DIR "/srv/www/cgi-bin/"
#define CONFIG_FILE "Isearch-sru.ini"

/// This is the class for manipulating an ini file
class CONFIG {
public:
  /// CONFIG class constructor
  CONFIG();
  /// CONFIG class destructor
  ~CONFIG();

  ///
  STRING   GetConfigFileName() {return ConfigFileName;}
  ///
  STRING   GetServerName() {return ServerName;}
  ///
  STRING   GetVersion() {return SRU_Version;}
  ///
  STRING   GetServerTitle() {return ServerTitle;}
  ///
  STRING   GetServerDescription() {return ServerDescription;}
  ///
  STRING   GetServerLanguage() {return ServerLanguage;}
  ///
  STRING   GetServerStatus() {return ServerStatus;}
  ///
  STRING   GetServerSruPath() {return ServerSruPath;}
  ///
  STRING   GetIsruDtdUri() {return IsruDtdUri;}
  ///
  STRING   GetIsruXslUri() {return IsruXslUri;}
  ///
  STRLIST& GetDbList() {return DBList;}
  ///
  STRLIST& GetContextSets() {return ContextSets;}

  ///
  INT      GetServerPort() {return Port;}
  ///
  INT      GetDebugLevel() {return DebugLevel;}

  /// Load the information for the specified database from the config file
  void     LoadDbInfo(DBINFO* ThisDb);
  /// Returns the text string associated with the diagnostic code
  STRING   FetchDiagnostic(INT DiagCode);
  /// Returns the identifier for the context set
  STRING   FetchContextUrl(STRING Context);

private:
  /// Object to hold site-wide settings from configuration file
  REGISTRY *Defaults; 
  /// Object to hold database-specific settings from db configuration file
  REGISTRY *dbinfo; 
  /// Name of configuration file
  STRING    ConfigFileName;
  /// Name of database configuration file
  STRING    DbConfigFileName;
  /// Name of this server
  STRING    ServerName;
  /// Version of SRU we recognize
  STRING    SRU_Version;
  /// Text title of this server
  STRING    ServerTitle;
  /// Text description of this server
  STRING    ServerDescription;
  /// Default language for this server
  STRING    ServerLanguage;
  /// Current status of this server (usually, "ok")
  STRING    ServerStatus;
  /// What is the start of the SRU URL for this server?
  STRING    ServerSruPath;
  /// What is the URI for the DTD to parse the XML?
  STRING    IsruDtdUri;
  /// What is the URI for the stylesheet for display?
  STRING    IsruXslUri;

  /// Runtime debug level (controls output)
  INT       DebugLevel;
  /// Port that the SRU server runs on (usually 80)
  INT       Port;

  /// List of databases defined in the configuration file
  STRLIST   DBList;
  /// List of context sets (i.e., schemas) supported by this server
  STRLIST   ContextSets;

  /// Context set URLS - may as well load them from the configuration
  STRING    ContextUrlGeo;
  STRING    ContextUrlGils;
  STRING    ContextUrlDc;
  STRING    ContextUrlCql;
  STRING    ContextUrlRec;

  /// CONFIG class initializer - call by constructor
  void Init();
};

#endif

