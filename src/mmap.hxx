/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _MMAP_HXX
#define _MMAP_HXX 1

enum mapping_access { MapNormal, MapSequential, MapRandom };

class MMAP {
public:
  MMAP();
  MMAP(const STRING& name);
  MMAP(const STRING& name, enum mapping_access flag);
  MMAP(const STRING& name, off_t from, off_t to, enum mapping_access flag=MapNormal);
  MMAP(const STRING& name, int permit, off_t from =0, off_t to=0,
        enum mapping_access flag=MapNormal);
  MMAP(const char *name);
  MMAP(const char *name, off_t from, off_t to, enum mapping_access flag=MapNormal);
  MMAP(const char *name, int permit, off_t from =0, off_t to=0,
	enum mapping_access flag=MapNormal);
  MMAP(int fd, off_t from =0, off_t to=0, enum mapping_access flag=MapNormal);
  MMAP(FILE *fp, off_t from =0, off_t to=0, enum mapping_access flag=MapNormal);

  bool Unmap();
  bool CreateMap(const STRING& fileName);
  bool CreateMap(const STRING& fileName, enum mapping_access flag);
  bool CreateMap(int fd, enum mapping_access flag);
  bool CreateMap(int fd, off_t from = 0, enum mapping_access flag = MapNormal);
  bool CreateMap(int fd, off_t from = 0, off_t to = 0, enum mapping_access flag = MapNormal);
  bool Ok() const;
  UCHR       *Ptr() const { return ptr; }
  UCHR       *Ptr(off_t offset) {
    if (offset > (off_t)(window+chunk))
      slide(window+chunk);
    else if (offset < (off_t)window)
      slide (offset-chunk);
    return &ptr[offset % window];
  }
  size_t      Size() const;
  bool Advise(int flag = MapNormal, size_t from=0, size_t to = 0);

  operator const char*() const { return (const char *)Ptr(); }
  operator const unsigned char*() const { return (unsigned char *)Ptr(); }

 ~MMAP();
private:
  void        slide(off_t starting) { /* for now */; }
  UCHR       *map(const char *, int, off_t, off_t, enum mapping_access);
  UCHR       *map(int fd, int permit, off_t from, off_t to, enum mapping_access flag);
  UCHR       *ptr;
  void       *addr;
  size_t      size;
  ino_t       inode;
  off_t       window;
  size_t      chunk; 
  int         fhandle;
  off_t       len;
};

// Simple little table of MMAPs with a pre-defined size
class MMAP_TABLE {
public:
  MMAP_TABLE(size_t Elements);
  size_t      TotalElements() const { return MaxElements; };
  MMAP       *Map(size_t Element) const;
  bool CreateMap(size_t Element, const STRING& FileName);
  bool Ok(size_t Element) const;
  PUCHR       Ptr(size_t Element) const;
  size_t      Size(size_t Element) const;
  bool Advise(size_t Element, int flag = MapNormal, size_t from=0, size_t to = 0);
  ~MMAP_TABLE();
private:
  size_t MaxElements;
  MMAP *Table;
};
#endif
