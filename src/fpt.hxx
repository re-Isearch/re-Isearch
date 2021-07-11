/*@@@
File:		fpt.hxx
Version:	1.00
Description:	Class FPT - File Pointer Table
Author:		Nassib Nassar, nrn@cnidr.org
		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#ifndef FPT_HXX
#define FPT_HXX

#include "fprec.hxx"

typedef PFILE* PPFILE;

class FPT {
public:
  FPT();
  FPT(size_t TableSize);

  FILE *ffopen(const STRING& FileName, const CHR* Type);
  FILE *ffreopen(const STRING& Filename, const CHR* mode, FILE *stream);
  INT   ffclose(FILE *FilePointer);

  INT   ffdispose(FILE *FilePointer); // Hard Close
  INT   ffdispose(const STRING& Filename); // Remove Entry

  INT   ffdelete(const STRING& Filename); // remove/unlink 
  INT   ffdelete(FILE *FilePointer); // Hard close and delete file

  INT   fflush(FILE *FilePointer) { return ::fflush(FilePointer); };
  void  Sync();
  void  CloseAll();

  INT   ffclose(const STRING& Filename);
  GDT_BOOLEAN hasOpenHandle(const STRING& Filename) const;

  void   Dump(ostream& os = cout) const;
  STRING Dump(const STRING& FileName) const {
    size_t z = Lookup(FileName);
    return (z) ? (STRING)Table[z-1] : NulString;
  }

  ~FPT();
private:
  void   Init(size_t TableSize);
  size_t Lookup(const STRING& FileName) const;
  size_t Lookup(const FILE *FilePointer) const;
  INT    FreeSlot();
  void   HighPriority(const size_t Index);
  void   LowPriority(const size_t Index);

  PFPREC          Table;
  size_t          TotalEntries;
  size_t          MaximumEntries;
#ifdef  USE_pThreadLocker  
  pthread_mutex_t mutex;
#endif
};

typedef FPT* PFPT;

#endif
