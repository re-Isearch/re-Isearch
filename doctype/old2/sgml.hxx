/************************************************************************
************************************************************************/

/*@@@
File:		sgml.hxx
Version:	1.00
Description:	Class SGML - SGML Document Format
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

#ifndef SGML_HXX
#define SGML_HXX

#include "defs.hxx"
#include "sgmlnorm.hxx"

class SGML : public SGMLNORM {
public:
  SGML(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseRecords(const RECORD& FileRecord);
  void DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& REcordSyntax, PSTRING StringBuffer) const;
  ~SGML();
private:
  STRING SGMLNorm_command, JadeCommandPath;
  STRING Catalog; // Path to Catalog?
  STRING DSSSL_Spec; // Path to DSSSL Spec
};

typedef SGML* PSGML;

#endif
