#ifndef HARVEST_HXX
#define HARVEST_HXX

#include "soif.hxx"

class HARVEST :  public SOIF {
public:
  HARVEST (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  GDT_BOOLEAN URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	GDT_BOOLEAN OnlyRemote) const;
  ~HARVEST();
};

typedef HARVEST* PHARVEST;

#endif
