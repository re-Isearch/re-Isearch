/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
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
  sHASHOBJ(const STRING& p, off_t off = 0, sHASHOBJ *ptr = NULL) {
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

  void         Clear() { str.Clear(); offset = 0;}

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

  void         MMAP_reads(GDT_BOOLEAN Map) { mmap_reads = Map; }

  void         Init(const STRING& Fn, int size = 0);  // intialize the hashtable

  GDT_BOOLEAN  KillAll();

  off_t        Add(const STRING& Str);            // add a string to the table, return file offset

  STRING       Get(off_t i);        // given offset, return string
  STRING       operator[](off_t n) { return Get(n); }
  off_t        operator[](const STRING& Str) { return Add(Str); }
  STRING       Filename() const { return filename; }

 ~sHASHTABLE();

private:
  void         AllocTable();
  GDT_BOOLEAN  LoadOnDiskHashTable();  // load on disk hash (into memory)
  void         DeleteTable();
  size_t       Hash(const STRING& Str) const;   // hash function for in memory hashtable
  MMAP         DiskTable;
  GDT_BOOLEAN  loaded;

  sHASHOBJ     lastCache;   // Cache last string/offset pair

  STRING       filename;    // filename of table values
  int          fd;          // file handle
  off_t        offset;      // current offset into on-disk file
  off_t        file_length; // length of file
  GDT_BOOLEAN  mmap_reads;  // MMAP or not?

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

  GDT_BOOLEAN  KillAll();

  GPTYPE    Add(const STRING& Name) {
    if (Name == oldFileName) return oldFileOffset;
    if (Name == oldPath) return oldPathOffset;
    return (GPTYPE)StringTable.Add(Name);
  }

  STRING   Get(GPTYPE i) {
    if (i == oldFileOffset) return oldFileName;
    if (i == oldPathOffset) return oldPath;
    return StringTable.Get((off_t)i);
  }

  GPTYPE   AddFileName(const STRING& newFileName) {
    if (newFileName != oldFileName)
      return oldFileOffset = (GPTYPE)StringTable.Add(oldFileName = newFileName);
    return oldFileOffset;
  }
  STRING    GetFileName(GPTYPE i) {
    if (i != oldFileOffset)
      return oldFileName = StringTable.Get((off_t)(oldFileOffset = i));
    return oldFileName;
  }
  GPTYPE    AddPath(const STRING& newPath) {
    if (newPath != oldPath)
      return oldPathOffset = (GPTYPE)StringTable.Add(oldPath = newPath);
    return oldPathOffset;
  }
  STRING    GetPath(GPTYPE i) {
    if (i != oldPathOffset)
      return oldPath = StringTable.Get((off_t)(oldPathOffset = i));
    return oldPath;
  }

  STRING       Filename() const { return StringTable.Filename(); }

 ~MDTHASHTABLE();

private:
  void       Init(const STRING& Stem);
  void       Init(const STRING& FileStem, size_t TableSize);

  // Some Cached values
  STRING     oldFileName, oldPath;
  GPTYPE     oldFileOffset, oldPathOffset;

  // The table
  sHASHTABLE StringTable;
};


#endif
