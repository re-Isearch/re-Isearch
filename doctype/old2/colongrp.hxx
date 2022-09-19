/************************************************************************
************************************************************************/

/*@@@
File:		colongrp.hxx
Version:	1.00
Description:	Class COLONGRP - COLONDOC-like Text w/ groups
Author:		Archie Warnock, warnock@clark.net
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

#ifndef COLONGRP_HXX
#define COLONGRP_HXX

#include "defs.hxx"
#include "doctype.hxx"
#include "colondoc.hxx"

class COLONGRP : public COLONDOC {
public:
  COLONGRP(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void AfterIndexing();

  void ParseFields(PRECORD NewRecord);
  void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  ~COLONGRP();

protected:
  PCHR *parse_Groups (PCHR b, size_t len);
private:
  BUFFER recBuffer, tagBuffer, startsBuffer, endsBuffer;
};

typedef COLONGRP* PCOLONGRP;

#endif
