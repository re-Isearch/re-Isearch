#ifndef AUTODETECT_HXX
#define AUTODETECT_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

#ifndef USE_LIBMAGIC
# ifndef _WIN32
#  ifndef SVR4
#    define USE_LIBMAGIC 1
#  endif
# endif
#endif

#if USE_LIBMAGIC
# undef _MAGIC_H
# include <magic.h>
#endif


class AUTODETECT:public DOCTYPE
{
 public:
  AUTODETECT (PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void BeforeIndexing();
  virtual void AfterIndexing();

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(RECORD *RecordPtr); // This class should never ParseFields!!

  ~AUTODETECT ();

private:
  STRING      file_magic;
#if USE_LIBMAGIC
  magic_t     magic_cookie;
#else
  STRING      file_cmd;
#endif
  GDT_BOOLEAN kludge;
  INT         kludgeCount;
  STRING      InfoPath;
  off_t        HostID;
  GDT_BOOLEAN ParseInfo;
};

typedef AUTODETECT *PAUTODETECT;

#endif
