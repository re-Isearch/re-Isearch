/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#ifndef IO_HXX
#define IO_HXX

class IO {
  enum IO_type { LFILE,URL };  // LFILE = Local File, URL = Uniform Resource Locator
  IO_type  type;
  STRING   id;         // contains filename or URL
  STRING   mode;       // read/write mode of file
  union {
    FILE  *fp;         // file ptr for open file
    int    p;          // for other types - may be an enum in future
  } ptr;
  bool             debug;      // 1 = show debugging, 0=hide
  unsigned char   *Buf;
  size_t           BufLength;
  off_t            cachePtr; 

public:
  IO();
  IO(const STRING& Fname, const STRING& mmode = "rb");


  IO& operator =(FILE *fp) {
    close();
    ptr.fp = fp;
    return *this;
  };

  void          debug_on() { debug = true; }
  void          debug_off(){ debug = false;}

  bool          open(const STRING& Fname, const STRING& mmode);
  void          close();

  int           remove(); 

  size_t        read(char *buffer, size_t length, size_t size);
  size_t        write(const char *buffer, size_t length, size_t size);

  off_t         iseek(off_t pos, int whence);
  off_t         iseek(off_t pos);
  off_t         itell();
  char         *igets(char *, int);
  void          top();
  ~IO();

  // Operator
  operator FILE *()  { return ptr.fp; }
  operator STRING () { return id;     }
};

// Like the stdio functions
//
//

inline char *fgets(char *s, int size, IO& i) {
  return i.igets(s, size); 
}

inline void fclose(IO i) { i.close(); }
inline size_t fread(void *ptr, size_t size, size_t nitems, IO& i) {
  return i.write((char *)ptr, nitems, size);
}
inline size_t fwrite(const void *ptr, size_t size, size_t nitems, IO& i) {
  return i.write((char *)ptr, nitems, size);
}
inline int fseek(IO i, off_t offset, int whence) {
  return i.iseek(offset, whence);
}
inline long ftell(IO& i) {
  return i.itell();
}
inline void rewind(IO& i) {
  i.top();
}


#endif
