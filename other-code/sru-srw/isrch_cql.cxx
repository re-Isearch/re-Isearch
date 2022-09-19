  // $Id: isrch_cql.cxx 85 2005-08-03 20:31:02Z warnock $
  /***********************************************************************
  Copyright (c) A/WWW Enterprises, 2001-2005

  Permission to use, copy, modify, distribute, and sell this software and
  its documentation, in whole or in part, for any purpose is hereby
  granted without fee.

  ************************************************************************/

  /*@@@
  File:           isrch_cql.cxx
  Version:        1.0
  $Revision$
  Description:    CQL-to-XCQL utilities for Isearch-sru
  Author:         Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
  History:	  Adapted from cql2xcql, part of the Yaz toolkit
  @@@*/

#include "isrch_sru.hxx"
#include "isrch_cql.hxx"
#include "sru_diagnostics.hxx"

INT
cql2xcql(CHR *cqlquery,CHR *xcqlquery)
{
    CQL_parser cp;
    INT        r = 0;
    INT        status = ISRU_OK;

    if (cqlquery) {
      cp = cql_parser_create();
      r = cql_parser_string(cp, cqlquery);

      if (r) {
	//    fprintf (stderr, "Syntax error\n");
	status = ISRU_BADQSYNTAX;

      } else {
	r = cql_to_xml_buf(cql_parser_result(cp), xcqlquery, XMLMAXLEN);
      }
      cql_parser_destroy(cp);

    } else {
	status = ISRU_BADQSYNTAX;
    }
    return status;
}

