/************************************************************************
************************************************************************/

/*@@@
File:		zdefs.hxx
Version:	1.00
Description:	Z General definitions
Author:		Edward C. Zimmermann
@@@*/

#ifndef ZDEFS_HXX
#define ZDEFS_HXX

extern const CHR *Bib1AttributeSet;
extern const CHR *StasAttributeSet;
extern const CHR *IsearchAttributeSet;

const INT IsearchFieldAttr	= 1;
const INT IsearchWeightAttr	= 7;
const INT IsearchTypeAttr	= 8;

// Record Syntaxes
extern const CHR *SutrsRecordSyntax;
extern const CHR *UsmarcRecordSyntax;
extern const CHR *HtmlRecordSyntax;
extern const CHR *SgmlRecordSyntax;
extern const CHR *XmlRecordSyntax;
extern const CHR *RawRecordSyntax;
extern const CHR *DVBHtmlRecordSyntax;
// Added for re-Isearch
extern const CHR *JsonRecordSyntax;

// Converted..
enum IBRecordSyntax {
  IB_Raw = 0,
  IB_Sutrs,
  IB_Html,
  IB_Sgml,
  IB_Xml,
  IB_Highlight,
  IB_Json, // Added re-Isearch
  IB_Usmarc == 100,
};

// Z39.50 AttributeValues (also other profiles)
enum ZRelations {
  ZRelNoop = 0,
  ZRelLT,
  ZRelLE,
  ZRelEQ,
  ZRelGE,
  ZRelGT,
  ZRelNE,
  ZRelOverlaps,
  ZRelEnclosedWithin,
  ZRelEncloses,
  ZRelOutside,
  ZRelNear,
  ZRelMembersEQ,
  ZRelMembersNE,
  ZRelBefore,
  ZRelBeforeDuring,
  ZRelDuring,
  ZRelDuringAfter,
  ZRelAfter,
// Strict date searching for GEO profile tests
// The default date search for GEO matches if any date in the target
// interval matches the query date.  These attributes change the default
// to match only if all dates in the target interval matches the query.
  ZRelBefore_Strict       = 3014,
  ZRelBeforeDuring_Strict = 3015,
  ZRelDuring_Strict       = 3016,
  ZRelDuringAfter_Strict  = 3017,
  ZRelAfter_Strict        = 3018
};

enum ZStruct {
  ZStructPhrase      = 1,
  ZStructWord        = 2,
  ZStructKey         = 3,
  ZStructYear        = 4,
  ZStructDate        = 5,
  ZStructWordList    = 6,
  ZStructDateTime    = 100,
  ZStructNameNorm    = 101,
  ZStructNameUnnorm  = 102,
  ZStructStructure   = 103,
  ZStructURx         = 104,
  ZStructText        = 105, // FreeFormText
  ZStructDocText     = 106, // DocumentText 
  ZStructLocalNumber = 107,
  ZStructCoord       = 108,
  ZStructCoordString = 109,
  ZStructCoordRange  = 110,
  ZStructBounding    = 111,
  ZStructComposite   = 112,
  ZStructRealMeas    = 113,
  ZStructIntMeas     = 114,
  ZStructDateRange   = 115,
  ZGEOStructCoord       = 200,
  ZGEOStructCoordString = 201,
  ZGEOStructBounding    = 202,
  ZGEOStructComposite   = 204,
  ZGEOStructRealMeas    = 205,
  ZGEOStructIntMeas     = 206,
  ZGEOStructDateRange   = 207,
  ZLOCALGlob            = 300 // Local Extension
};

enum ZTrunc {
  ZTruncRight     = 1,
  ZTruncLeft      = 2,
  ZTruncLeftRight = 3,
  ZTruncNone      = 100,
  ZTruncProcess   = 101,
  ZTruncRE1       = 102,
  ZTruncRE2       = 103
};

enum ZComplete {
  ZCompleteIncSub    = 1,
  ZCompleteCompSub   = 2,
  ZCompleteCompField = 3
};

#endif
