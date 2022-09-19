// $Id: cgi-util.hxx 67 2005-07-05 21:11:03Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/

/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1996-2005

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
File:          	cgi-util.hxx
Version:        1.04
$Revision$
Description:    CGI utilities
Authors:        Kevin Gamiel, kgamiel@cnidr.org
		Tim Gemma, stone@k12.cnidr.org
@@@*/

#ifndef _CGIUTIL_HXX
#define _CGIUTIL_HXX

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <ctype.h>
#include "defs.hxx"

#define CGI_MAXENTRIES 100
#define POST 0
#define GET 1

/// CGI Utilities
class CGIAPP {
  /// Array to hold CGI parameter names
  CHR *name[CGI_MAXENTRIES];
  /// Array to hold CGI parameter values
  CHR *value[CGI_MAXENTRIES];
  /// Number of parameters read from CGI stream
  INT entry_count;
  /// Get or Post?
  INT Method;
  /// Grabs the CGI input and parses it into the various members
  void GetInput(int argc, char** argv);

public:
  /// CGIAPP Constructor
  CGIAPP(int argc, char** argv);
  /// Displays the CGI input as HTML, for debugging
  void DisplayAsHTML();
  /// Dumps the CGI parameters out as nice text
  void Display();
  /// Dumps the CGI parameters to a file
  void Dump(FILE *fp);
  /// Dumps the CGI parameters to a file in HTML
  void DumpToHTML(FILE *fp);
  /** Gets the name of the ith parameter (useful for iterating over all
      of the parameters
  */
  CHR *GetName(INT4 i) {return name[i];}
  /** Gets the value of the ith parameter (useful for iterating over all
      of the parameters
  */
  CHR *GetValue(INT4 i) {return value[i];}
  /// Retrieves the value of the CGI parameter specified by name
  CHR *GetValueByName(const CHR *name);
  /// Returns the number of entries read from the CGI stream
  INT GetEntryCount() { return entry_count; }
  /// CGIAPP Destructor
  ~CGIAPP();

};

/// Utility to convert + character to a blank
void plustospace(CHR *p);
/// Converts escaped characters back to their raw equivalents
void unescape_url(CHR *p);
/// Converts a string to one with bad characters safely escaped
void escape_url(CHR *url, CHR *out);
/// Converts blank characters in a string into + character
void spacetoplus(CHR *str);
/// Converts hex codes into characters
CHR x2c(CHR *p);
/// Converts characters into the equivalent hex string
CHR* c2x(CHR what);

#endif
