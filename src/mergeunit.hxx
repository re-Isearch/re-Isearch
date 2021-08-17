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
  GDT_BOOLEAN Smallest(PSTRING small);
  GDT_BOOLEAN Initialize(STRING& FileName,const PINDEX iParent,
			 FILEMAP *map, size_t IDValue); // true = success
  GDT_BOOLEAN Empty();				     // true = no more entries
  GPTYPE GetGp(); // returns the GP
  void GetSistring (PSTRING a);
  GDT_BOOLEAN Flush(FILE *fout, FILE *sout = NULL);
  GDT_BOOLEAN Load();		// get more items
  void Write(FILE *fout, FILE *sout = NULL);
  GDT_BOOLEAN CacheLoad();	// Load Cache From Disk
  void SetLoadLimit(INT lim);
  
  ~MERGEUNIT();
  
private:
  GDT_BOOLEAN CacheEmpty();
  GDT_BOOLEAN GetIndirectBuffer(const GPTYPE Gp, PSTRING Buffer); 
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
