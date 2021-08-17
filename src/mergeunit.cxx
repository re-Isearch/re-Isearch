/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)mergeunit.cxx  1.15 02/05/01 00:33:15 BSN"

/* $Id: mergeunit.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $ */
/*-@@@
File:		mergeunit.cxx
Version:	1.00
$Revision: 1.1 $
Description:	Class MERGEUNIT
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "common.hxx"
#include "index.hxx"
#include "filemap.hxx"
#include "mergeunit.hxx"

MERGEUNIT::MERGEUNIT() 
{
  fp = NULL;
// sistring="";
  CachePosition=LoadLim=LIM;

  list = new GPTYPE[1];
  Start = new GPTYPE[1];
  sistrings = new STRING[1];
  Tag = new CHR[1];
  CacheWritten=FlushWritten=CacheFlush=0;
  ItemsToMerge=TotalLoaded=0;
}

static void LocalGetSistring(STRING *s, size_t Gp, const UCHR *buf)
{
  s->SetTermLower(&buf[Gp], StringCompLength);
}

GDT_BOOLEAN MERGEUNIT::CacheLoad()
{
//  GPTYPE fe;
//  int namecount=0;

  CachePosition=0;

  message_log(LOG_DEBUG, "Load Unit #%d", ID);

  size_t found = 0;
  if (fp && !feof(fp)){
    for(size_t i=0; i<LoadLim; i++)
      found += Parent->GpFread(&list[i], fp);
  }
  if(found==0){
    LoadLim=0;
    return(GDT_FALSE);
  }
  TotalLoaded +=found;

  if(found<LoadLim)
    LoadLim=found;
  message_log(LOG_DEBUG, "Build Starting Keys..(got %d)", LoadLim);
  for(size_t j=0; j<LoadLim; j++){
    Tag[j]=0;
    Start[j]=Map->GetKeyByGlobal(list[j]);
  }
  message_log(LOG_DEBUG, "Key Build Complete - Commence Merge Load... %ld of %ld Complete",
	TotalLoaded-found, ItemsToMerge);

//  STRING x;
  size_t q=LoadLim,count=0,ncount=0;

  STRING Fn;
  DOCTYPE_ID Doctype;

  UCHR *p = NULL;
  GPTYPE p_size = 0;
  FILE *fq;

  for(size_t i=0; i<LoadLim; i++){
    GPTYPE size, LocalStart;

    if(Tag[i]==0){
      // gp is Global Position of start of record (*not* file)
      const GPTYPE gp=Map->GetNameByGlobal(list[i],&Fn,&size,&LocalStart,&Doctype);

      if (p_size < size)
	{
	  if (p) delete[] p;
	  p_size = size+1;
	  p = new UCHR[p_size];
	}
      size_t got = Parent->ReadIndirect(Fn, p, LocalStart, size, Doctype);
      if (got != size)
	{
	  message_log(LOG_ERRNO, "Read error on '%s'(%ld). Wanted %ld bytes but got %ld",
			(const char *)Fn, LocalStart, size, got);
	  size = got;
	}
      p[size]='\0';

      LocalGetSistring(&sistrings[i],list[i]-gp, (const UCHR *)p);
      Tag[i]=1;
      count++;
      for(size_t j=i; j<LoadLim; j++){ // inside loop - find all later entries 
	// from same record
	if(Tag[j]==0){ // not visited already
	  GPTYPE tmpgp=Start[j];
	  if(tmpgp==gp){		// same record
	    LocalGetSistring(&sistrings[j],list[j]-gp, (const  UCHR *)p);
	    Tag[j]=1;
	    count++;
	    ncount++;
	  }
	}
      }
      if(ncount>(LoadLim/5)){
        message_log(LOG_DEBUG, "Finished %d of %d", count, q);
	ncount=0;
      }
    }
  }		/* for() */
  if (p) delete[] p;
  return (count > 0);
}

// flush entire unit to file
GDT_BOOLEAN MERGEUNIT::Flush(FILE *fout, FILE *sout)
{ 
  message_log(LOG_DEBUG, "Merge Flush..");

//  Parent->GpFwrite(Gp, fout);
//  ++FlushWritten;
  // flush cache here
  for(size_t i=CachePosition; i<LoadLim; i++){
    if (sout) Parent->SisWrite(sistrings[i], sout);
    Parent->GpFwrite(list[i], fout);
    ++CacheFlush;
  }

  while(Parent->GpFread(&Gp, fp)){
    //  cout << Gp <<endl;
    ++FlushWritten;
    Parent->GpFwrite(Gp,fout);
  }
  CachePosition=LoadLim;
  return GDT_TRUE;
}

// write item in Gp to disk
void MERGEUNIT::Write(FILE *fout, FILE *sout)
{
// cerr <<"|"<<sistrings[CachePosition]<<"|"<<endl;
  if (sout) Parent->SisWrite(sistrings[CachePosition], sout);
  ++CacheWritten;
  Parent->GpFwrite(list[CachePosition++],fout);
  Load();
}


GDT_BOOLEAN MERGEUNIT::Smallest(PSTRING Current)
{
  const char *a = (const char *)(Current[0]); 
  const char *b = (const char *)(sistrings[CachePosition]);
  if(a && b && strcmp(a,b)<=0)
    return(GDT_TRUE); 
  if (b && *b) *Current=b;
  return(GDT_FALSE);
}


// signify whether entire unit is empty

GDT_BOOLEAN MERGEUNIT::Empty()
{
  return ((fp==NULL)||(feof(fp)&&(CacheEmpty()==GDT_TRUE)));
}

GPTYPE MERGEUNIT::GetGp()
{
  return(list[CachePosition]);
}

void MERGEUNIT::GetSistring(PSTRING a)
{
  *a=sistrings[CachePosition];
}

GDT_BOOLEAN MERGEUNIT::CacheEmpty()
{
  // note - we have a good value in Gp!!
  return (CachePosition==LoadLim);
}

void MERGEUNIT::SetLoadLimit(INT v)
{
  CachePosition=LoadLim= (v > 1024 ? v : 1024);

  delete [] list;
  delete [] Start;
  delete [] sistrings;
  delete [] Tag;

  list = new GPTYPE[LoadLim+1];
  Start = new GPTYPE[LoadLim+1];
  sistrings = new STRING[LoadLim+1];
  Tag = new CHR[LoadLim+1];
}


// get top item in cache and put in Gp/sistring
// reload cache if necessary
// returns TRUE if cache is empty (nothing left)

GDT_BOOLEAN MERGEUNIT::Load()
{
  if( CacheEmpty()==GDT_TRUE)
    return CacheLoad() ? GDT_FALSE : GDT_TRUE;
  return(GDT_FALSE);
}


GDT_BOOLEAN MERGEUNIT::Initialize(STRING& IndexFileName,const PINDEX iParent, FILEMAP *m,
	size_t value)
{
  GDT_BOOLEAN val = GDT_FALSE;

  const off_t Size = GetFileSize(IndexFileName);
  const INT Off = Size % sizeof (GPTYPE);
  ItemsToMerge = ( (Size - Off) / sizeof (GPTYPE)); // Number of Entries in Index

  Parent = iParent;
  ID=value;
  Map=m;

  if (fp) Parent->ffclose(fp);

  if ((fp=Parent->ffopen(IndexFileName,"rb")) != NULL) {
    if (Off) {
      UINT2 magic = getINT2 (fp);
      if ((INT)((magic & 0xFF) + 6) != Parent->Version()) {
	message_log (LOG_ERROR, "MERGEUNIT: Bad index magic in %s. Expected %d but got %d!",
		(const char *)IndexFileName, (int)(magic&0xFF), (int)Parent->Version()-6);
	Parent->ffclose(fp);
	fp = NULL;
	return GDT_FALSE;
      }
      // Move past magic
      if (fseek(fp, Off, SEEK_SET) == -1)
	{
	  message_log (LOG_ERRNO, "Could not seek on '%s' to %ld", (const char *)IndexFileName, Off);
	}
    }
    val=Load();
    // Gp=list[CachePosition];
    //  sistring=sistrings[CachePosition++];
  }
  return(val);
  
}



MERGEUNIT::~MERGEUNIT()
{
  
  if(fp) Parent->ffclose(fp);
  //  delete names;
  delete list;
  delete [] sistrings;
  delete Tag;
  delete Start;

  message_log(LOG_DEBUG, "Unit #%d Written From Cache: %ld", ID, CacheWritten);
  message_log(LOG_DEBUG, "Unit #%d Remaining Items Flushed From Cache: %ld", ID, CacheFlush);
  message_log(LOG_DEBUG, "Unit #%d Remaining Items Flushed From Disk: %ld", ID, FlushWritten);
}

