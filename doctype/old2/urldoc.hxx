#ifndef URLDOC_HXX
#define URLDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

class URLDOC:public DOCTYPE
{
 public:
  URLDOC (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseRecords(const RECORD& FileRecord);
  ~URLDOC ();
};

typedef URLDOC *PURLDOC;

#endif
