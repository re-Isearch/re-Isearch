// $Id: DbInfo.cxx 88 2005-08-03 20:34:05Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/


/*@@@
File:          	DbInfo.cxx
Version:        1.0
$Revision$
Description:    Container for database-specific info from the config file
Authors:        Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include "DbInfo.hxx"

DBINFO::DBINFO(STRING& NewDbName) {
  DbName = NewDbName;
  mapping_table = new REGISTRY("map");
  count = 0;
}


void
DBINFO::SetMappingTable(const STRLIST& Position, const STRLIST& Value) {
  mapping_table->SetData(Position,Value);
}

void
DBINFO::GetFieldMapping(const STRLIST& Position, STRLIST *StrlistBuffer) {
  mapping_table->GetData(Position,StrlistBuffer);
}

DBINFO::~DBINFO() {
  if (mapping_table)
    delete mapping_table;
}

