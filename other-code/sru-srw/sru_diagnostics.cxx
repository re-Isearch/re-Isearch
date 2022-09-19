// $Id: sru_diagnostics.cxx 84 2005-08-03 20:30:09Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru_diagnostics.cxx
Version:        1.00
$Revision$
Description:    SRU diagnostic methods
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#include <iostream.h>

#include "sru.hxx"


void
SRU::PrintDiagnostics() {
  if (Diagnostics.GetLength() > 0) {
    cout << "<zs:diagnostics xmlns=\"info:srw/schema/1/diagnostic-v1.1\">";
    cout << Diagnostics;
    cout << "</zs:diagnostics>\n";
  }
}


void
SRU::SetDiagnostic(INT DiagCode, CHR* details) {
  STRING diag=details;
  SetDiagnostic(DiagCode,diag);
}


void
SRU::SetDiagnostic(INT DiagCode, STRING& details) {
  STRING message;
  if (DiagCode >0) {
    Diagnostics.Cat("\n<diagnostic>\n");
    Diagnostics.Cat("<uri>info:srw/diagnostic/1/");
    message=DiagCode;
    Diagnostics.Cat(message);
    Diagnostics.Cat("</uri>\n");
    message=Defaults->FetchDiagnostic(DiagCode);
    Diagnostics.Cat("<message>");
    Diagnostics.Cat(message);
    Diagnostics.Cat("</message>\n");
    if (details.GetLength() > 0) {
      Diagnostics.Cat("<details>");
      Diagnostics.Cat(details);
      Diagnostics.Cat("</details>\n");
    }
    Diagnostics.Cat("</diagnostic>\n");
  }
}

