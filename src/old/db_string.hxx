#include <gdbm.h>
#include "common.hxx"
#include "irset.hxx"
#include "index.hxx"
#include "attrlist.hxx"

class DB_STRING {
  DB_STRING(IDBOBJ *Db) {
    dbp = NULL; db = Db; ft=FIELDTYPE::db_string;
  }

  DB_STRING(IDBOBJ *Db, const FIELDTYPE& FieldType) {
    dbp = NULL; db = Db; ft=FieldType;
  }

  void   *Open(const STRING& Filename, const char *mode=NULL) {
    if (Filename == filename && dbp && *dbp) return (void *)dbp;
    // We don't use mode
    if (dbp == NULL) dbp = new GDBM_FILE;
    else Close(); 
    if ((*dbp=gdbm_open((char *)(Filename.c_str()),0,GDBM_WRCREAT,0666,0)) == NULL)
      return NULL;
    return (void *)dbp;
  }

  void   *OpenForAppend(const STRING& Filename) {
    return Open(Filename);
  }
  void *OpenForRead(const STRING& Fieldname) {
    return OpenForAppend(Filename); // No difference with GDB
  }

  void   *OpenFieldForAppend(const STRING& Fieldname) {
    STRING Fn;
    if (db && db->DfdtGetFileName(Fieldname, ft, &Fn))
      return OpenForAppend(Fn);
    return NULL;
  }
  void   *OpenFieldForRead(const STRING& Fieldname) {
     return OpenFieldForAppend(Fieldname); // No difference with GDB
  }

  GDT_BOOLEAN Close() {
    if (dbp && *dbp) {
      gdbm_close(*dbp);
      *dbp = NULL;
      return GDT_TRUE;
    }
    return GDT_FALSE;
  }

  PIRSET Search(const STRING& Term, const STRING& Field) {
    if (OpenForRead(Field)) return Search(Term);
    // ERROR
    return NULL;
  }

  PIRSET Search(const STRING& Term) {
    datum term, data;

    if (dbp == NULL || *dbp == NULL)
     return NULL; // ERROR

    PIRSET pirset = new IRSET();

    term.dptr  = (char *) Term.c_str();
    term.dsize = Term.GetLength();
    data = gdbm_fetch(*dbp, term);
    if (data.dsize > 0) {
      // Got something
      STRING Key('\0', data.dsize);

      memcpy((char *)Key.c_str(), data.dptr, data.dsize); 
      MDT *ThisMdt = db->GetMainMdt();
      size_t  w = ThisMdt->LookupByKey(Key);
      if (w) {
	IRESULT iresult;
        iresult.SetMdtIndex(w);
        iresult.SetHitCount(1);
        iresult.SetScore(0);
        iresult.SetMdt(ThisMdt);
        pirset->AddEntry(iresult, 1);
       }
    }
   return pirset;
  }

  int Write(const STRING& Value, const STRING& Key) {
    int ret;
    datum key, data;

    // The GDBM key is the value of the field
    // the data is the Key of the record!
    data.dptr = (char *)Key.c_str();
    data.dsize = Key.GetLength();
    key.dptr = (char *)Value.c_str();
    key.dsize = Value.GetLength();
    if ((ret = gdbm_store(*dbp,key,data,GDBM_REPLACE)) != 0)
      logf (LOG_ERROR, "DB_STRING: store failed: %s", gdbm_strerror(ret));
    else
      gdbm_sync(*dbp);
    return ret;
  };

  const char *Name() const        { return "db_string"; }
  const char *Description() const { return "read and write GDBM Strings"; }

  ~DB_STRING() { 
    Close();
    delete dbp;
  }
private:
  STRING       filename;
  FIELDTYPE    ft;
  GDBM_FILE   *dbp;
  IDBOBJ      *db;
};
