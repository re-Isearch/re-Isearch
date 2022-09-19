// $Id: sru_explain.cxx 147 2006-03-20 12:58:08Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru_explain.cxx
Version:        1.00
$Revision$
Description:    SRU utilities for Explain
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#include <iostream.h>

#include "sru.hxx"

void
SRU::PrintExplainResponse() {
  PrintExplainResponseHeader();
  PrintExplainVersion();
  PrintExplainRecord();
  if (Diagnostics.GetLength() > 0)
    PrintDiagnostics();
  PrintExplainResponseFooter();
}


void
SRU::PrintExplainResponseHeader() {
  cout << "<zs:explainResponse xmlns:zs=\"http://www.loc.gov/zing/srw/\">" << endl;
}


void
SRU::PrintExplainVersion() {
  PrintSruVersion();
}


void
SRU::PrintExplainRecord() {
  cout << "<zs:record>" << endl;
  PrintExplainRecordSchema();
  PrintExplainRecordPacking();
  PrintExplainRecordData();
  cout << "</zs:record>" << endl;
}


void
SRU::PrintExplainRecordSchema() {
  cout << "<zs:recordSchema>";
  cout << "http://explain.z3950.org/dtd/2.0/";
  cout << "</zs:recordSchema>" << endl;
}


void 
SRU::PrintExplainRecordPacking() {
  cout << "<zs:recordPacking>";
  cout << "xml";
  cout << "</zs:recordPacking>" << endl;
}


void
SRU::PrintExplainRecordData() {
  cout << "<zs:recordData>" << endl;
  cout << "  <explain xmlns=\"http://explain.z3950.org/dtd/2.0/\">" << endl;
  PrintExplainServerInfo();
  PrintDatabaseInfo();
  PrintIndexInfo();
  PrintRecordSchemaInfo();
  PrintConfigInfo();
  cout << "    </explain>" << endl;
  cout << "  </zs:recordData>" << endl;
}


void
SRU::PrintHost() {
  cout << "        <host>";
  cout << Defaults->GetServerName();
  cout << "</host>" << endl;
}


void
SRU::PrintPort() {
  cout << "        <port>";
  cout << Defaults->GetServerPort();
  cout << "</port>" << endl;
}


void
SRU::PrintTitle() {
  cout << "        <title>";
  cout << Defaults->GetServerTitle();
  cout << "</title>" << endl;
}


void
SRU::PrintDescription() {
  cout << "        <description>";
  cout << Defaults->GetServerDescription();
  cout << "</description>" << endl;
}


void
SRU::PrintStatus() {
  cout << "        <status>";
  cout << Defaults->GetServerStatus();
  cout << "</status>" << endl;
}


void
SRU::PrintDatabaseName() {
  cout << "        <database>";
  cout << current->GetDbName();
  cout << "</database>" << endl;
}

void
SRU::PrintExplainServerInfo() {
  cout << "<serverInfo>" << endl;
  PrintHost();
  PrintPort();
  PrintDatabaseName();
  cout << "</serverInfo>" << endl;
}


void
SRU::PrintDatabaseTitle() {
  cout << "        <title>";
  cout << current->GetDbTitle();
  cout << "</title>" << endl;
}


void
SRU::PrintDatabaseDescriptionParameters() {
  cout << "lang=\"en\" primary=\"true\"";
}


void
SRU::PrintDatabaseDescription() {
  cout << "        <description ";
  PrintDatabaseDescriptionParameters();
  cout << ">";
  cout << current->GetDbDescription();
  cout << "</description>" << endl;
}


void
SRU::PrintDatabaseInfo() {
  cout << "      <databaseInfo>" << endl;
  PrintDatabaseTitle();
  PrintDatabaseDescription();
  cout << "      </databaseInfo>" << endl;
}


void
SRU::PrintIndexInfo() {
  cout << "      <indexInfo>" << endl;
/*
        <set identifier=\"info:srw/cql-context-set/1/cql-v1.1\" name=\"cql\"/>
        <set identifier=\"info:srw/cql-context-set/1/dc-v1.1\" name=\"dc\"/>
        <set identifier=\"http://zing.z3950.org/cql/bath/2.0/\" name=\"bath\"/>

        <index id=\"4\">
          <title>title</title>
          <map><name set=\"dc\">title</name></map>
        </index>
        <index id=\"21\">
          <title>subject</title>
          <map><name set=\"dc\">subject</name></map>
        </index>
        <index id=\"1003\">
          <title>creator</title>
          <map><name set=\"dc\">creator</name></map>
          <map><name set=\"dc\">author</name></map>
        </index>

        <index id=\"1020\">
          <title>editor</title>
          <map><name set=\"dc\">editor</name></map>
        </index>

        <index id=\"1018\">
          <title>publisher</title>
          <map><name set=\"dc\">publisher</name></map>
        </index>

        <index id=\"62\">
          <title>description</title>
          <map><name set=\"dc\">description</name></map>
        </index>

        <index id=\"30\">
          <title>date</title>
          <map><name set=\"dc\">date</name></map>
        </index>
*/
  cout << "      </indexInfo>" << endl;
}


void
SRU::PrintRecordSchemaInfo() {
  cout << "     <schemaInfo>" << endl;
  //  cout << "        <schema identifier=\"info:srw/schema/1/marcxml-v1.1\" sort=\"false\" name=\"marcxml\">" << endl;
  //  cout << "          <title>MARCXML</title>" << endl;
  //  cout << "        </schema>" << endl;
  //  cout << endl;
  //  cout << "        <schema identifier=\"info:srw/schema/1/dc-v1.1\" sort=\"false\" name=\"dc\">" << endl;
  //  cout << "          <title>Dublin Core</title>" << endl;
  //  cout << "        </schema>" << endl;
  cout << "        <schema identifier=\"http://www.blueangeltech.com/Standards/GeoProfile/\" sort=\"false\" name=\"geo\">" << endl;
  cout << "          <title>GEO</title>" << endl;
  cout << "        </schema>" << endl;
  cout << "        <schema identifier=\"http://www.gils.net/context-set.html\" sort=\"false\" name=\"gils\">" << endl;
  cout << "          <title>GILS</title>" << endl;
  cout << "        </schema>" << endl;
  cout << "      </schemaInfo>" << endl;
}


void
SRU::PrintConfigInfo() {
  cout << "      <configInfo>" << endl;
  cout << "        <default type=\"numberOfRecords\">";
  cout << GetDbNumDocs(); // for now, the default is all records
  cout << "</default>" << endl;
  cout << "      </configInfo>" << endl;
}


void
SRU::PrintExplainResponseFooter() {
  cout << "</zs:explainResponse>" << endl;
}


void
SRU::PrintDebug() {
  cout << "<debugInfo>" << endl;
  cout << "  <config_filename>";
  cout << Defaults->GetConfigFileName() << endl;
  cout << "</config_filename>" << endl;
  cout << "</debugInfo>" << endl;
}


