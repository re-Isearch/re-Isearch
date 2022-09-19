// $Id: sru_searchretrieve.cxx 56 2005-06-30 22:28:39Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru_searchretrieve.cxx
Version:        1.00
$Revision$
Description:    SRU utilities
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#include "sru.hxx"

/*
void 
SRU::SetRetrieve(IRSET *pirset) {
}
*/


void
SRU::PrintSearchRetrieveResponse() {
  PrintSearchRetrieveResponseHeader();
  PrintSruVersion();
  PrintSearchRetrieveRecord();
  if (Diagnostics.GetLength() > 0)
    PrintDiagnostics();
  PrintSearchRetrieveResponseFooter();
}


void 
SRU::PrintSearchRetrieveResponseHeader() {
  cout << "<zs:searchRetrieveResponse xmlns:zs=\"http://www.loc.gov/zing/srw/\">" << endl;
}


void 
SRU::PrintSearchRetrieveResponseFooter() {
  cout << "</zs:searchRetrieveResponse>" << endl;
}


void 
SRU::PrintSearchRetrieveRecord() {
  cout << Response << endl;
}
