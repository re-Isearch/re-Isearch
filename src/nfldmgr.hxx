#ifndef NUMERICFLDMGR_HXX
#define NUMERICFLDMGR_HXX

class NUMERICFLDMGR {
private:
  PNUMERICLIST fields;
  INT NumFields;
  INT MaxEntries;

public:
  NUMERICFLDMGR();
  ~NUMERICFLDMGR();
  INT LoadFields(PCHR dbName);
  void GetResult(INT use, PIRSET s,PIDBOBJ p);
  INT LocateFieldByAttribute(INT Attribute);  // return index for field
	// list for this attribute - a search will
  INT Find(INT Attribute, INT4 Relation, FLOAT Key);
};

typedef NUMERICFLDMGR* PNUMERICFLDMGR;
	
#endif
