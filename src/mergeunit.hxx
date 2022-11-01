/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/


#ifndef MERGEUNIT_HXX
#define MERGEUNIT_HXX

#include "defs.hxx"
#include "index.hxx"

#define LIM 10000

class MERGEUNIT{
	
 public:
  MERGEUNIT();
  bool Smallest(PSTRING small);
  bool Initialize(STRING& FileName,const PINDEX iParent,
			 FILEMAP *map, size_t IDValue); // true = success
  bool Empty();				     // true = no more entries
  GPTYPE GetGp(); // returns the GP
  void GetSistring (PSTRING a);
  bool Flush(FILE *fout, FILE *sout = NULL);
  bool Load();		// get more items
  void Write(FILE *fout, FILE *sout = NULL);
  bool CacheLoad();	// Load Cache From Disk
  void SetLoadLimit(INT lim);
  
  ~MERGEUNIT();
  
private:
  bool CacheEmpty();
  bool GetIndirectBuffer(const GPTYPE Gp, PSTRING Buffer); 
  PINDEX Parent;
  FILE *fp;			// file of GP's to read from
  STRING sistring;
  GPTYPE Gp;
  FILEMAP *Map;
  size_t CachePosition;
  size_t LoadLim,ID;
  off_t ItemsToMerge,TotalLoaded;
  off_t CacheWritten,FlushWritten,CacheFlush;
  
  GPTYPE *list;
  GPTYPE *Start;
  
  STRING *sistrings;
 // STRING *names;
  CHR *Tag;
};

typedef MERGEUNIT *PMERGEUNIT;

#endif
