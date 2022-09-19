// $Id: sru_diagnostics.hxx 56 2005-06-30 22:28:39Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/
/*@@@
File:           sru_diagnostics.hxx
Version:        1.00
$Revision$
Description:    SRU utilities
Authors:        Archie Warnock, A/WWW Enterprises
@@@*/

#ifndef _SRUDIAGS_HXX
#define _SRUDIAGS_HXX

#define ISRU_OK                   0     // All is well

// General Diagnostics
#define ISRU_GENSYSERROR	  1	// General system error
#define ISRU_SYSTMPUNAVAIL	  2	// System temporarily unavailable
#define ISRU_AUTHERROR		  3	// Authentication error
#define ISRU_BADOP		  4	// Unsupported operation
#define ISRU_BADVERS		  5	// Unsupported version
#define ISRU_BADPPVAL		  6	// Unsupported parameter value
#define ISRU_MANDMISS		  7	// Mandatory parameter not supplied
#define ISRU_BADPARM		  8 	// Unsupported Parameter

// Diagnostics Relating to CQL
#define ISRU_BADQSYNTAX		 10	// Query syntax error
#define ISRU_BADQTYPE		 11	// DEPRECATED. Unsupported query type
#define ISRU_QTOOLONG		 12 	// DEPRECATED. Too many characters in query
#define ISRU_BADPARENS		 13 	// Invalid or unsupported use of parentheses
#define ISRU_BADQUOTES		 14 	// Invalid or unsupported use of quotes
#define ISRU_BADCSET		 15	// Unsupported context set
#define ISRU_BADINDEX		 16	// Unsupported index
#define ISRU_BADCOMB1		 17	// DEPRECATED. Unsupported combination of index and index set
#define ISRU_BADCOMB2		 18	// Unsupported combination of indexes
#define ISRU_BADRELATION	 19	// Unsupported relation
#define ISRU_BADRELMOD		 20	// Unsupported relation modifier
#define ISRU_BADCOMB3		 21	// Unsupported combination of relation modifers
#define ISRU_BADCOMB4		 22	// Unsupported combination of relation and index
#define ISRU_TERMTOOLONG	 23	// Too many characters in term
#define ISRU_BADCOMB5		 24	// Unsupported combination of relation and term
#define ISRU_BADSPCHARS		 25	// DEPRECATED. Special characters not quoted in term
#define ISRU_BADESCAPE		 26	// Non special character escaped in term
#define ISRU_NOEMPTYTERM	 27	// Empty term unsupported
#define ISRU_NOMASKING		 28	// Masking character not supported
#define ISRU_MASKTOOSHORT	 29	// Masked words too short
#define ISRU_MASKTOOLONG	 30	// Too many masking characters in term
#define ISRU_BADANCHORCHAR	 31	// Anchoring character not supported
#define ISRU_BADANCHORPOS	 32	// Anchoring character in unsupported position
#define ISRU_BADCOMB6		 33	// Combination of proximity/adjacency and masking characters not supported
#define ISRU_BADCOMB7		 34	// Combination of proximity/adjacency and anchoring characters not supported
#define ISRU_ALLSTOPS		 35	// Term contains only stopwords
#define ISRU_BADTERMFORMAT	 36	// Term in invalid format for index or relation
#define ISRU_BADBOOLEAN		 37	// Unsupported boolean operator
#define ISRU_TOOMANYBOOLEANS	 38	// Too many boolean operators in query
#define ISRU_NOPROXIMITY	 39	// Proximity not supported
#define ISRU_BADPROXRELATION	 40	// Unsupported proximity relation
#define ISRU_BADPROXDISTANCE	 41	// Unsupported proximity distance
#define ISRU_BADPROXUNIT	 42	// Unsupported proximity unit
#define ISRU_BADPROXORDER	 43	// Unsupported proximity ordering
#define ISRU_BADCOMB8		 44	// Unsupported combination of proximity modifiers
#define ISRU_BADPREFIX		 45	// DEPRECATED. Prefix assigned to multiple identifiers
#define ISRU_BADBOOLMOD		 46	// Unsupported boolean modifier
#define ISRU_BADQUERY		 47 	// Cannot process query; reason unknown
#define ISRU_BADQFEATURE	 48 	// Query feature unsupported
#define ISRU_BADMASKPOS		 49 	// Masking character in unsupported position

// Diagnostics Relating to Result Sets
#define ISRU_NORSETS		 50	// Result sets not supported
#define ISRU_NORSETEXIST	 51	// Result set does not exist
#define ISRU_RSETUNAVAIL	 52	// Result set temporarily unavailable
#define ISRU_RSETRETRONLY	 53	// Result sets only supported for retrieval
#define ISRU_RSETEXISTONLY	 54	// DEPRECATED. Retrieval may only occur from an existing result set
#define ISRU_BADRSETCOMB	 55	// Combination of result sets with search terms not supported
#define ISRU_SINGLERSETONLY	 56	// DEPRECATED. Only combination of single result set with search terms supported
#define ISRU_RSETEMPTY		 57	// DEPRECATED. Result set created but no records available
#define ISRU_RSETPARTBAD	 58	// Result set created with unpredictable partial results available
#define ISRU_RSETPARTVALID	 59	// Result set created with valid partial results available
#define ISRU_RSETTOOBIG		 60	// Result set not created: too many matching records

// Diagnostics Relating to Records
#define ISRU_RSETBADRANGE	 61	// First record position out of range
#define ISRU_NRECSNEG	 	 62	// DEPRECATED. Negative number of records requested
#define ISRU_RETRSYSERR	 	 63	// DEPRECATED. System error in retrieving records
#define ISRU_RECUNAVAIL	 	 64	// Record temporarily unavailable
#define ISRU_NOREC	 	 65	// Record does not exist
#define ISRU_BADSCHEMA	 	 66	// Unknown schema for retrieval
#define ISRU_NORECSCHEMA	 67	// Record not available in this schema
#define ISRU_NOSEND		 68	// Not authorised to send record
#define ISRU_NOSENDSCHEMA	 69	// Not authorised to send record in this schema
#define ISRU_RECTOOBIG		 70	// Record too large to send
#define ISRU_BADRECPACK		 71	// Unsupported record packing
#define ISRU_NOXPATH		 72	// XPath retrieval unsupported
#define ISRU_XPATHBADFEATURE	 73	// XPath expression contains unsupported feature
#define ISRU_XPATHNOEVAL	 74	// Unable to evaluate XPath expression

// Diagnostics Relating to Sorting
#define ISRU_NOSORT		 80	// Sort not supported
#define ISRU_NOSORTTYPE		 81	// DEPRECATED. Unsupported sort type
#define ISRU_BADSORTSEQ		 82	// Unsupported sort sequence
#define ISRU_SORTTOOMANYRECS	 83	// Too many records to sort
#define ISRU_SORTTOOMANYKEYS	 84	// Too many sort keys to sort
#define ISRU_SORTDUPKEYS	 85	// DEPRECATED. Duplicate sort keys
#define ISRU_SORTBADFMT		 86	// Cannot sort: incompatible record formats
#define ISRU_SORTBADSCHEMA	 87	// Unsupported schema for sort
#define ISRU_SORTBADPATH	 88	// Unsupported path for sort
#define ISRU_SCHEMABADPATH	 89	// Path unsupported for schema
#define ISRU_BADDIRECTION	 90	// Unsupported direction
#define ISRU_BADCASE		 91	// Unsupported case
#define ISRU_BADMISSINGVAL	 92	// Unsupported missing value action

// Diagnostics Relating to Explain
#define ISRU_NOEXPLAIN		100	// DEPRECATED. Explain not supported (Use 4)
#define ISRU_BADEXPLAINTYPE	101	// DEPRECATED. Explain request type not supported
#define ISRU_NOEXPLAINREC	102	// DEPRECATED. Explain record temporarily unavailable (Use 64)

// Diagnostics relating to Stylesheets
#define ISRU_NOSTYLE		110	// Stylesheets not supported
#define ISRU_BADSTYLE	 	111	// Unsupported stylesheet

// Diagnostics relating to Scan
#define ISRU_RESPOUTOFRANGE	120	// Response position out of range
#define ISRU_TOOMANYTERMS	121 	// Too many terms requested

#endif
