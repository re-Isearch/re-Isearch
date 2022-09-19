// NOTE: this is not a traditional hash table...
#ifndef MDTHASHTABLE_HXX
#define MDTHASHTABLE_HXX

#include "mmap.hxx"

class sHASHOBJ {
 friend class sHASHTABLE;

public:
  sHASHOBJ() {
    next = NULL;
    offset = 0;
  }
  sHASHOBJ(const STRING& p, UINT4 off = 0, sHASHOBJ *ptr = NULL) {
    str    = p;
    offset = off;
    next   = ptr;
  }

  sHASHOBJ    *AddEntry(sHASHOBJ *List) {
    next = List;
    return this;
  }
  sHASHOBJ    *Next() const   { return next;   }
  STRING       Value() const  { return str;    }
  off_t        Offset() const { return offset; }

  ~sHASHOBJ() { ; }

private:
  STRING      str;
  off_t       offset;
  sHASHOBJ   *next;
};

class INDEX;
class IDBOBJ;

class sHASHTABLE {
public:
  sHASHTABLE();
  sHASHTABLE(const STRING& Fn, int size_prime=0);

  void         Init(const STRING& Fn, int size = 0);  // intialize the hashtable

  void         KillAll();

  long         Add(const STRING& Str);            // add a string to the table, return file offset

  STRING       Get(long i);        // given offset, return string
  STRING       operator[](long n) { return Get(n); }
  long         operator[](const STRING& Str) { return Add(Str); }
  STRING       Filename() const { return filename; }

 ~sHASHTABLE();

private:
  void         AllocTable();
  GDT_BOOLEAN  LoadOnDiskHashTable();     // load on disk hash into memory
  void         DeleteTable();
  size_t       Hash(const STRING& Str) const;   // hash function for in memory hashtable
  MMAP         DiskTable;
  GDT_BOOLEAN  loaded;

  STRING       lastStr;
  off_t        lastOffset;

  STRING       filename;    // filename of table values
  int          fd;          // file handle
  off_t        offset;      // current offset into on-disk file
  off_t        file_length; // length of file

  size_t       tableSize;   // size of in memory hashtable (prime)
  sHASHOBJ   **Table;       // in memory hashtable of value-index pairs
};


class MDTHASHTABLE {
friend class MDT;
public:
  MDTHASHTABLE(const IDBOBJ *Idb);
  MDTHASHTABLE(const INDEX  *Index); 
  MDTHASHTABLE(const STRING& Stem);
  MDTHASHTABLE(const STRING& Stem, GDT_BOOLEAN Fast);

  void      KillAll();

  long      AddFileName(const STRING& NewFileName) {
    return fileNameTable.Add(NewFileName);
  }
  STRING    GetFileName(long i) {
    return fileNameTable.Get(i);
  }
  long      AddPathName(const STRING& NewPathName) {
    return pathNameTable.Add(NewPathName);
  }
  STRING    GetPathName(long i) {
    return pathNameTable.Get(i);
  }

 ~MDTHASHTABLE();

private:
  void       Init(const STRING& Stem);
  void       Init(const STRING& FileStem, size_t FileTableSize, size_t PathTableSize);

  // The tables
  sHASHTABLE fileNameTable;
  sHASHTABLE pathNameTable;
};


#endif
