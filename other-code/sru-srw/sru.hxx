// $Id: sru.hxx 155 2006-04-14 18:19:55Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru.hxx
Version:        1.00
$Revision$
Description:    SRU utilities
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#ifndef _SRU_HXX
#define _SRU_HXX

#include "defs.hxx"
#include "common.hxx"
#include "string.hxx"

#include "version.hxx"
#include "Config.hxx"
#include "cgi-util.hxx"
#include "DbInfo.hxx"

#include "sru_diagnostics.hxx"

/// Class for responding to SRU requests
class SRU : public CGIAPP {

public:
  /// SRU class constructor
  SRU(int argc, char** argv);
  /// SRU class destructor
  ~SRU();

  /// Check to see if the database actually has database files present
  GDT_BOOLEAN IsValidDb();

  /// Prints the HTTP header to cout
  void PrintHTTPHeader();
  /// Prints the default Stylesheet header
  void PrintStylesheetHeader(CHR *XSL);
  /// Prints the default XML document header
  void PrintXmlHeader();
  /// Prints the SRU version, including XML tags
  void PrintSruVersion();
  /// Store a diagnostic code, including some descriptive text
  void SetDiagnostic(INT DiagCode, STRING& details);
  /// Store a diagnostic code, including some descriptive text
  void SetDiagnostic(INT DiagCode, CHR* details);
  /// Print the SRU diagnostic block in XML
  void PrintDiagnostics();

  //
  //Basic info methods
  //
  /// Print the SRU ConfigInfo block in XML
  void PrintConfigInfo();
  /// Print the SRU RecordSchema block in XML
  void PrintRecordSchemaInfo();
  /// Print the SRU IndexInfo block in XML
  void PrintIndexInfo();
  ///
  void PrintDatabaseInfo();
  ///
  void PrintDatabaseDescription ();
  ///
  void PrintDatabaseDescriptionParameters();
  ///
  void PrintDatabaseTitle();
  ///
  void PrintDatabaseName();
  ///
  void PrintPort();
  ///
  void PrintHost();
  ///
  void PrintTitle();
  ///
  void PrintDescription();
  ///
  void PrintStatus();
  ///
  void PrintDebug();

  //
  // Explain handlers
  //
  ///
  void PrintExplainResponse();
  ///
  void PrintExplainResponseHeader();
  ///
  void PrintExplainResponseFooter();
  ///
  void PrintExplainRecord();
  ///
  void PrintExplainRecordData();
  ///
  void PrintExplainServerInfo();
  ///
  void PrintExplainRecordPacking();
  ///
  void PrintExplainRecordSchema();
  ///
  void PrintExplainVersion();

  //
  // SearchPresent handlers
  //
  void PrintSearchRetrieveResponse();
  ///
  void PrintSearchRetrieveResponseHeader();
  ///
  void PrintSearchRetrieveResponseFooter();
  ///
  void PrintSearchRetrieveRecord();
  ///
  void SetResponse(STRING& NewResponse) {Response = NewResponse;}
  ///
  void AddToResponse(STRING& NewResponse) {Response.Cat(NewResponse);}
  ///
  void AddToResponse(CHR* NewResponse) {Response.Cat(NewResponse);}

  //
  // Individual member handlers
  //
  ///
  void    PutOperation(STRING& NewOp) { Operation=NewOp; }
  ///
  STRING& GetOperation() { return Operation; }

  ///
  void    PutDatabaseName(CHR *NewDbName) { DatabaseName=NewDbName; }
  ///
  void    PutDatabaseName(STRING& NewDbName) { DatabaseName=NewDbName; }
  ///
  STRING& GetDatabaseName() { return DatabaseName; }

  /** Grab information on the specified database from the configuration
      and store it away in the current SRU object - uses char argument
  */
  void    SetDatabase(CHR *NewDbName);
  /** Grab information on the specified database from the configuration
      and store it away in the current SRU object - uses STRING argument
  */
  void    SetDatabase(STRING& NewDbName);

  /** Save the user's stylesheet request
  */
  //  void    PutStylesheet(CHR *XSL) { Stylesheet = XSL; }
  //  STRING  GetStylesheet() { return Stylesheet; }

  ///
  STRING   GetDatabasePath() {return current->GetDbPath();}
  ///
  STRING   GetIndexName() {return current->GetDbIndexName();}
  ///
  STRLIST& GetDbList() {return Defaults->GetDbList();}
  ///
  STRLIST& GetContextSets() {return Defaults->GetContextSets();}

  ///
  STRING   GetServerName() {return Defaults->GetServerName();}
  ///
  STRING   GetServerPort() {return Defaults->GetServerPort();}
  ///
  STRING   GetServerSru() {return Defaults->GetServerSruPath();}
  ///
  STRING   GetIsruDtdUri() {return Defaults->GetIsruDtdUri();}
  ///
  STRING   GetIsruXslUri() {return Defaults->GetIsruXslUri();}
  ///
  STRING   GetServerSruVersion() {return Defaults->GetVersion();}

  /// Save the number of docs in the current database with the DbInfo object
  void     SetDbNumDocs(INT NumDocs) {current->PutNumDocs(NumDocs);}
  /// Return the number of docs in the current database from the DbInfo object
  INT      GetDbNumDocs() {return current->GetNumDocs();}
  /// Load the map file for the given context set for the current DbInfo obj
  INT      LoadDbMapFile(STRING& Context);
  /// Validates whether the context set is supported or not
  INT      ValidateContextSet(STRING& Context);
  /// Loads the field mappings from the specified file into the DBINFO obj.
  INT      AddToMappingTable(const STRING &FileToAdd);
  ///
  INT      GetMappedField(const STRING &Requested,STRING *Mapped);

private:
  /// The current requested operation
  STRING       Operation;
  /// The current requested database
  STRING       DatabaseName;
  /// The current requested stylesheet
  //  STRING       Stylesheet;
  /// Object holding the defaults read from the configuration file
  CONFIG      *Defaults;
  /// Object containing information about the currently requested database
  DBINFO      *current;

  /// The SRU diagnostic block, in XML
  STRING       Diagnostics;
  /// Flag indication whether we have diagnostics to print or not
  GDT_BOOLEAN  diags;

  /// The XML response record of search results
  STRING       Response;

};

#endif
