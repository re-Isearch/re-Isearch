#ifndef HARVEST_HXX
#define HARVEST_HXX

#include "soif.hxx"

class HARVEST :  public SOIF {
public:
  HARVEST (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  bool GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const;
  bool URL(const RESULT& ResultRecord, PSTRING StringBuffer,
	bool OnlyRemote) const;
  ~HARVEST();
};

typedef HARVEST* PHARVEST;

#endif
