/* $Id: isrch_sru.cxx 178 2006-07-15 15:12:28Z warnock $ */
/************************************************************************
Copyright (c) A/WWW Enterprises, 2006

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee.

************************************************************************/

/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1996-2006.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact.

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*@@@
File:          	isrch_sru.cxx
Version:        1.00
$Revision$
Description:    CGI app that searches against Iindex-ed databases using
                SRU protocol (Z39.50 over http with CQL query syntax).
		Adapted from isearch_cgi
Authors:        Kevin Gamiel, kgamiel@cnidr.org
		Archie Warnock, warnock@awcubed.com
@@@*/

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <iostream>

#ifdef UNIX
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "isrch_sru.hxx"
#include "irset.hxx"

void  PutHTTPHeader(void);
void  PutHTMLHead(void);
void  PutHTMLBodyStart(void);
void  PutHTMLBodyEnd(void);

/**  This is the main program for isearch-sru.  It reads an SRU command
     string from the command line or stdin and executes it.  

     See http://www.loc.gov/z3950/agency/zing/srw/ for information on
     SRU and SRW.
 */
int
main(int argc, char **argv)
{
  INT   status;
  CHR   *db;
  CHR   *query;
  CHR   *XSL;
  SRU   *srudata;
  IRSET *pirset;


//  srudata = new SRU(argc, argv);

#ifdef CGI_DUMP
  // Good for debugging form values
    PutHTTPHeader();
    PutHTMLHead();
    PutHTMLBodyStart();
#endif // CGI_DUMP

    srudata = new SRU(argc, argv);

#ifdef CGI_DUMP
//    fprintf(stdout,"%d args<br>\n",argc);
//    srudata->Dump(stdout);
    srudata->Display();
    PutHTMLBodyEnd();
    exit(0);
#endif // CGI_DUMP

  // First, figure out how we were called.  If our name is
  // "isearch-sru.cgi" then send back the list of databases.  If
  // it is anything else, take the program name as the requested
  // database.
  STRING ARG0=argv[0];
  STRINGINDEX n;
  n = ARG0.SearchReverse('/');
  ARG0.EraseBefore(n+1);
  if (ARG0.CaseEquals("isearch-sru.cgi")) {
    // If isearch-sru.cgi was called with either parameter - x-db or
    // x-database - use the parameter value as the db name.  Otherwise,
    // just print out the list of databases.
    if ((db = srudata->GetValueByName("x-db")) == NULL) {
	if ((db = srudata->GetValueByName("x-database")) == NULL) {
	PrintDbListDoc(srudata); // send back the list in XML
	exit(0);                 // and exit
      }
    }

  } else {
    // So, the name must be what we want to use as the database
    db = ARG0.NewCString();
  }

  // If we get here, we have a database name and this must be an SRU
  // request.  First off, store the database name and load the info
  // about the database from the config file.
  srudata->SetDatabase(db); 

  // Have we got a request to return a stylesheet?
  if ((XSL = srudata->GetValueByName("stylesheet")) == NULL) {
    XSL = (CHR*)NULL;
  }
  //  if (XSL)
  //    srudata->PutStylesheet(XSL); 

  // Figure out what operation they are asking for
  STRING request=srudata->GetOperation();
  if (request.CaseEquals("explain")) { // Asked for explain

    // Check to see if the database actually exists
    if (!(srudata->IsValidDb())) {
      srudata->PrintHTTPHeader();
      srudata->PrintXmlHeader();
      if (XSL)
	srudata->PrintStylesheetHeader(XSL);
      srudata->SetDiagnostic(ISRU_BADPPVAL,db);
      srudata->PrintExplainResponse();
      exit(0);
    }

    status=doExplain(srudata); // Fill in the db stuff not in the ini file
    if (status != ISRU_OK)
      srudata->SetDiagnostic(status,db);

    srudata->PrintHTTPHeader();
    srudata->PrintXmlHeader();
    if (XSL)
      srudata->PrintStylesheetHeader(XSL);
    srudata->PrintExplainResponse();

  } else if (request.CaseEquals("searchRetrieve")) {
    //  Make sure the database they asked for actually exists.
    if (!(srudata->IsValidDb())) {
      srudata->PrintHTTPHeader();
      srudata->PrintXmlHeader();
      if (XSL)
	srudata->PrintStylesheetHeader(XSL);
      srudata->SetDiagnostic(ISRU_BADPPVAL,db);
      srudata->PrintExplainResponse();
      exit(0);
    }

    // Asked for search
    if ((query = srudata->GetValueByName("query")) == NULL) {
      // No query parameter
      srudata->PrintHTTPHeader();
      srudata->PrintXmlHeader();
      if (XSL)
	srudata->PrintStylesheetHeader(XSL);
      srudata->SetDiagnostic(ISRU_MANDMISS,"query");
      srudata->PrintExplainResponse();

    } else {
      // We have a query - execute the search
      status=doSearchRetrieve(srudata,query);
      if (status == ISRU_OK) {
	// Got results, so print them out
	srudata->PrintHTTPHeader();
	srudata->PrintXmlHeader();
	if (XSL)
	  srudata->PrintStylesheetHeader(XSL);
	srudata->PrintSearchRetrieveResponse();

      } else {
	// Search failed, so generate the diagnostics with explain
	srudata->PrintHTTPHeader();
	srudata->PrintXmlHeader();
	if (XSL)
	  srudata->PrintStylesheetHeader(XSL);
	srudata->PrintExplainResponse();
      }
    }

  } else if (request.CaseEquals("scan")) {
    // Asked for scan, but we don't do that yet
    srudata->PrintHTTPHeader();
    srudata->PrintXmlHeader();
    if (XSL)
      srudata->PrintStylesheetHeader(XSL);
    srudata->SetDiagnostic(ISRU_BADOP,request);
    srudata->PrintExplainResponse();

  } else {
    // Asked for something we know nothing about
    srudata->PrintHTTPHeader();
    srudata->PrintXmlHeader();
    if (XSL)
      srudata->PrintStylesheetHeader(XSL);
    srudata->SetDiagnostic(ISRU_BADOP,request);
    srudata->PrintExplainResponse();
  }

  //  if (db)
  //    delete [] db;
  //  if (query)
  //    delete [] query;
  if (srudata)
    delete srudata;
  exit(0);
}


void
PutHTTPHeader()
{
  cout << "Content-type: text/html\n\n";
}


void
PutHTMLHead()
{
  cout << "<html>\n<head>" << endl;
  cout << "<title>Isearch-sru Debugging</title>" << endl;
  cout << "</head>" << endl;
}


void
PutHTMLBodyStart()
{
  cout << "<body>" << endl;
}


void
PutHTMLBodyEnd()
{
  cout << "</body>\n</html>" << endl;
}
