#ifndef FILEMAP_HXX
#define FILEMAP_HXX

class FILEMAP{
 public:
  struct _table{
    GPTYPE GpStart;
    GPTYPE LocalStart;
    GPTYPE LocalEnd;
    STRING Path;
    DOCTYPE_ID  Doctype;
  };

  FILEMAP(const PIDBOBJ p);

  GPTYPE GetKeyByGlobal(GPTYPE gp) const;
  GPTYPE GetNameByGlobal(GPTYPE gp, PSTRING s = NULL, GPTYPE *size = NULL,
	GPTYPE *LS = NULL, DOCTYPE_ID *Doctype = NULL) const;

  ~FILEMAP();

 private:
  struct _table *Items;
  size_t         MdtCount;
  PMDT           mdt;
  PIDBOBJ        Parent;

};
#endif
