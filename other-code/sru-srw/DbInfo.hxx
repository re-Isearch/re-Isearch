// $Id: DbInfo.hxx 88 2005-08-03 20:34:05Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/

/*@@@
File:          	DbInfo.hxx
Version:        1.0
$Revision$
Description:    Container for database-specific info from the config file
Authors:        Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#ifndef _DBINFO_HXX_
#define _DBINFO_HXX_

#include <iostream.h>
#include <stdio.h>
#include <string.h>

#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"
#include "strlist.hxx"
#include "registry.hxx"

/// Object to hold database-specific information - loaded by class CONFIG
class DBINFO {
public:
  /// DBINFO class constructor
  DBINFO(STRING& DbName);
  /// DBINFO class destructor
  ~DBINFO();

  ///
  STRING GetDbName() {return DbName;}

  ///
  void   PutDbPath(STRING& NewDbPath) {DbPath=NewDbPath;}
  ///
  STRING GetDbPath() {return DbPath;}

  ///
  void   PutDbTitle(STRING& NewDbTitle) {DbTitle=NewDbTitle;}
  ///
  STRING GetDbTitle() {return DbTitle;}

  ///
  void   PutDbType(STRING& NewDbType) {DbType=NewDbType;}
  ///
  STRING GetDbType() {return DbType;}

  ///
  void   PutDbIndexName(STRING& NewDbIndexName) {DbIndexName=NewDbIndexName;}
  ///
  STRING GetDbIndexName() {return DbIndexName;}

  ///
  void   PutDbDescription(STRING& NewDbDescription) {
    DbDescription=NewDbDescription;
  }
  ///
  STRING GetDbDescription() {return DbDescription;}

  ///
  void   PutDbLanguage(STRING& NewDbLanguage) {DbLanguage=NewDbLanguage;}
  ///
  STRING GetDbLanguage() {return DbLanguage;}

  ///
  void   PutDbFieldMaps(STRING& NewDbFieldMaps) {DbFieldMaps=NewDbFieldMaps;}
  ///
  STRING GetDbFieldMaps() {return DbFieldMaps;}

  ///
  void   PutDbSynonyms(STRING& NewDbSynonyms) {DbSynonyms=NewDbSynonyms;}
  ///
  STRING GetDbSynonyms() {return DbSynonyms;}

  ///
  void   PutNumDocs(INT NewCount) {count=NewCount;}
  ///
  INT    GetNumDocs() {return count;}
  ///
  void   SetMappingTable(const STRLIST& Position, const STRLIST& Value);
  ///
  void   GetFieldMapping(const STRLIST& Position, STRLIST *StrlistBuffer);

private:
  /// Public name of this database
  STRING DbName;
  /// Text title for this database
  STRING DbTitle;
  /// Local file path to the database files
  STRING DbPath;
  /// Type of database (usually ISEARCH)
  STRING DbType;
  /// Name of the database files (internal - not necessarily DbName)
  STRING DbIndexName;
  /// Longer text description of this database
  STRING DbDescription;
  /// Default language of this database
  STRING DbLanguage;
  /// Filenames of the map files (fieldname-to-attribute numbers)
  STRING DbFieldMaps;
  /// Do we try to clean up synomyms?
  STRING DbSynonyms;
  /// Number of documents in the database
  INT    count;
  /// Contains the field mappings for the current requested context set
  REGISTRY* mapping_table;
};

#endif
