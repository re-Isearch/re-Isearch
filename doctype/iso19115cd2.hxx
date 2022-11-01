/////// PORT TO IB IN PROGRESS

/*-@@@
File:		iso19115cd2.hxx
Version:	1.00
Description:	Class ISO19115CD2 - doctype for ISO 19115 XML metadata
Author:		Archibald Warnock (warnock@awcubed.com), A/WWW Enterprises
Copyright:	A/WWW Enterprises, Crownsville, MD
@@@-*/

#ifndef ISO19115CD2_HXX
#define ISO19115CD2_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "gilsxml.hxx"

class ISO19115CD2 : public XMLBASE
{
public:
  ISO19115CD2 (PIDBOBJ DbParent);
  void        BeforeIndexing(STRING DbFileName);
  void        AfterIndexing() {};
  void        BeforeRset(const STRING& RecordSyntax, STRING DbFileName) {};
  void        AfterRset(const STRING& RecordSyntax) {};

  void        LoadFieldTable();
  SRCH_DATE   ParseDate(const STRING& Buffer);
  void        ParseDateRange(const CHR *Buffer, DOUBLE* fStart, 
			     DOUBLE* fEnd);
  void        ParseDateRange(const STRING& Buffer, DOUBLE* fStart, 
			     DOUBLE* fEnd);
  void        ParseGPoly(const CHR *Buffer, DOUBLE Vertices[]);

  DOUBLE      ParseComputed(const STRING& FieldName, const CHR *Buffer);
  bool HeadlineDb() { return true; }

  void        ParseFields (RECORD *NewRecord) {};
  void        ParseFields (RECORD *NewRecord,CHR* RecBuffer);
  void        Present(const RESULT & ResultRecord, 
		      const STRING & ElementSet,
		      STRING *StringBuffer);
  void        Present (const RESULT & ResultRecord, 
		       const STRING & ElementSet,
		       const STRING & RecordSyntax,
		       STRING *StringBuffer);

  STRING      GetNorthBoundingFieldName() {return ISO_NORTH;}
  STRING      GetSouthBoundingFieldName() {return ISO_SOUTH;}
  STRING      GetEastBoundingFieldName()  {return ISO_EAST;}
  STRING      GetWestBoundingFieldName()  {return ISO_WEST;}

  ~ISO19115CD2 ();

private:
  bool UsefulSearchField(const STRING& Field);
  void PresentBriefXml(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentIsearchBriefXml(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentBriefSutrs(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentSummaryXml(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentFullXml(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentFullHtml(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void PresentFullSutrs(const RESULT& ResultRecord, STRING *StringBufferPtr);
  void ParseExtent(const CHR* Buffer, DOUBLE* extent);

  bool CaseSensitive;
  STRING      XML_Header;
  DB         *a_dbp;
  STRING      dbf_name;

};

typedef ISO19115CD2 *PISO19115CD2;

#endif
