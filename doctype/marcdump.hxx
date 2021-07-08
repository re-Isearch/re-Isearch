/*-@@@
File:		marcdump.hxx
Version:	$Revision: 1.1 $
Description:	Class MARCDUMP - output from Yaz utility marcdump
Author:		Archibald Warnock (warnock@clark.net), A/WWW Enterprises
Copyright:	A/WWW Enterprises, Columbia, MD
@@@-*/

#ifndef MARCDUMP_HXX
#define MARCDUMP_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

class MARCDUMP 
  : public DOCTYPE {
public:
    MARCDUMP(PIDBOBJ DbParent);
    void AddFieldDefs();
    CHR* UnifiedName (CHR *tag);
    void ParseRecords(const RECORD& FileRecord);
    void ParseFields(RECORD *NewRecord);

    void BeforeSearching(QUERY* SearchQueryPtr);

    void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 STRING *StringBuffer);
    void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING* StringBufferPtr);
    void PresentSutrs(const RESULT& ResultRecord, 
		      const STRING& ElementSet,
		      STRING *StringBufferPtr);
    void PresentHtml(const RESULT& ResultRecord, 
		     const STRING& ElementSet, 
		     STRING *StringBufferPtr);

    ~MARCDUMP();

};
typedef MARCDUMP* PMARCDUMP;

#endif
