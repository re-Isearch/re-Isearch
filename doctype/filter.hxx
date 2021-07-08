/*-@@@
File:		filter.hxx
Version:	0.01
Description:	Class FILTERDOC - Binary files
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef FILTER_HXX
#define FILTER_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

class FILTERDOC:public DOCTYPE
{
 public:
  FILTERDOC (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void SourceMIMEContent(STRING *stringPtr) const;

  virtual const char *GetDefaultClass() const;
  virtual const char *GetDefaultFilter() const;

  virtual void BeforeIndexing();
  virtual void AfterIndexing();

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(RECORD *RecordPtr); // This class should never ParseFields!!

  ~FILTERDOC ();
private:
  off_t   HostID;
  STRING Filter;
  STRING Classname;
  STRING MIMEtype;
};

typedef FILTERDOC *PFILTERDOC;

#endif
