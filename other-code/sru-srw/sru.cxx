// $Id: sru.cxx 155 2006-04-14 18:19:55Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru.cxx
Version:        1.00
$Revision$
Description:    SRU utilities
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#include <iostream.h>
#include "sru.hxx"

#define CONTEXT_MAP_SEPARATOR '='
#define CONTEXT_MAP_OPENER '{'
#define CONTEXT_MAP_CLOSER '}'

SRU::SRU(int argc, char** argv)
  : CGIAPP::CGIAPP(argc, argv) {
  STRING name,value;
  INT entry_count=GetEntryCount();

  DatabaseName="";
  Operation="";
  diags=GDT_FALSE;

  for (INT x=0;x<entry_count;x++) {
    name=GetName(x);
    value=GetValue(x);
    if ((name.GetLength() > 0) && (value.GetLength() > 0)) {
      // We have a real parameter
      if (name.CaseEquals("operation"))
	Operation=value;
	
    } else if (value.GetLength() > 0) {
      // We have no named parameter but we have a value, so it must
      // be the database name
      DatabaseName = value;
    }
  }


  // Handle the null case
  if (!(Operation.GetLength() > 0))
    Operation="explain";

  // Now, load the confuration file
  Defaults = new CONFIG();
  current = (DBINFO*)NULL;
}


GDT_BOOLEAN 
SRU::IsValidDb() {
  STRING path=current->GetDbPath();
  STRING name=current->GetDbIndexName();

  path.Cat("/");
  path.Cat(name);
  path.Cat(".mdt");
  if (IsFile(path))
    return GDT_TRUE;

  return GDT_FALSE;
}


INT
SRU::LoadDbMapFile(STRING& Context) {
  // Make sure current is loaded...
  STRLIST Mapfiles;
  INT n,i;
  STRING Filename,ThisEntry;

  if (current) {
    Mapfiles.Split(',',current->GetDbFieldMaps());
    n = Mapfiles.GetTotalEntries();
    i = 1;
    Mapfiles.GetEntry(i,&ThisEntry);
    while ((ThisEntry.Search(Context) <= 0) && (i<=n)){
      i++;
      Mapfiles.GetEntry(i,&ThisEntry);
    }
    n = ThisEntry.Search(CONTEXT_MAP_SEPARATOR);
    ThisEntry.EraseBefore(n+1);
    n = ThisEntry.Search(CONTEXT_MAP_CLOSER);
    if (n>0)
      ThisEntry.EraseAfter(n-1);
    Filename = ThisEntry;
    AddToMappingTable(Filename);

  } else {
    // we should never get here because current should exist by the time
    // we get into loading the map file
    return ISRU_GENSYSERROR;
  }
  return ISRU_OK;
}


INT
SRU::GetMappedField(const STRING &Requested,STRING *Mapped) {
  STRING    TheMappedValue, NewMapped, String;
  STRLIST   Position, Result;
  IS_SIZE_T n,pos;

  NewMapped="";

  Position.Clear();
  Position.AddEntry("Default");
  Position.AddEntry(Requested);
  
  // Do the lookup in the field map registry, inside of the current db
  current->GetFieldMapping(Position,&Result);

  // Remove the Z39.50 attributes, if any
  n=Result.GetTotalEntries();
  for (UINT i=1;i<=n;i++) {
    Result.GetEntry(i,&String);
    pos = String.Search('/');
    if (pos > 0)
      String.EraseBefore(pos+1);
    if (NewMapped.GetLength() > 0)
      NewMapped.Cat('+');
    NewMapped.Cat(String);
  }

  *Mapped=NewMapped;
  return ISRU_OK;
}


INT
SRU::ValidateContextSet(STRING& Context) {
  // Make sure current is loaded...
  STRING Mapfiles;
  STRING ThisContext = Context;
  // Field maps have the form {<context-set>=<filename>}
  // In order to find out if we have defined a field map file for a
  // particular context set, we just search for the name with the = appended
  ThisContext.Cat(CONTEXT_MAP_SEPARATOR);
  if (current) {
    Mapfiles = current->GetDbFieldMaps();
    if (Mapfiles.Search(ThisContext)) 
      return ISRU_OK;
    else
      return ISRU_BADCSET;
  } else {
    // we should never get here because current should exist by the time
    // we get into loading the map file
    return ISRU_GENSYSERROR;
  }
}


INT
SRU::AddToMappingTable(const STRING &FileToAdd)
{
  STRLIST      FromPos, ResultList, ToPos, TempList;
  STRINGINDEX  count, i;
  STRING       Entry;
  REGISTRY    *FromFile;

  // Build entry into mapping table for the database
  FromPos.Clear();
  FromFile = new REGISTRY("map");
  FromFile->ProfileLoadFromFile(FileToAdd, FromPos);

  // Check old-style mappings for use attributes
  FromPos.AddEntry("Default");
  FromFile->GetData(FromPos, &ResultList);
  count = ResultList.GetTotalEntries();

#ifdef DEBUG
  //  ResultList.Dump();
#endif

  // Now we have a list of all mapping directives ("bib1/4") Step
  // through each one, build a position list, get the value of that
  // directive and SetData on the internal mapping table for the current
  // database.
  ToPos.AddEntry("Default");
  if (count > 0) {
    for(i=1;i <= count;i++) {
      ResultList.GetEntry(i, &Entry);
      FromPos.SetEntry(2, Entry);
      FromFile->GetData(FromPos, &TempList);

#ifdef DEBUG
      //      cout << i << " - " << Entry << endl;
      //      TempList.Dump();
#endif

      ToPos.SetEntry(2, Entry);
      current->SetMappingTable(ToPos, TempList);
    }
  }

  delete FromFile;
  return ISRU_OK;
}



void
SRU::SetDatabase(CHR *DbName) {
  STRING sDbName = DbName;
  SetDatabase(sDbName);
}


void
SRU::SetDatabase(STRING& DbName) {
  STRING Hold;
  current = new DBINFO(DbName);

  // Fill in the object elements
  Defaults->LoadDbInfo(current);
}


void  
SRU::PrintHTTPHeader() {
  cout << "Content-type: text/xml\n\n";
}


void
SRU::PrintXmlHeader() {
  cout << "<?xml version=\"1.0\"?>\n";
}


void
SRU::PrintStylesheetHeader(CHR* XSL) {
  if (XSL) {
    cout << "<?xml-stylesheet type=\"text/xsl\" href=\"" 
	 << XSL
	 << "\"?>\n";
  }
}


void 
SRU::PrintSruVersion() {
  cout << "<zs:version>";
  cout << Defaults->GetVersion();
  cout << "</zs:version>" << endl;
}



SRU::~SRU() {
  if (current)
    delete current;

  if (Defaults)
    delete Defaults;
}
