/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)rcache.cxx"

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "rcache.hxx"

#define DEFCACHE 20



static const BYTE RCACHE_VERSION = 2;

#define DEBUG 0

RCACHE::RCACHE(const PIDBOBJ DbParent)
{
  Init(DbParent, DEFCACHE);
}


RCACHE::RCACHE(const PIDBOBJ DbParent, size_t MaxCache)
{
  Init(DbParent, MaxCache);
}

void RCACHE::Init(const PIDBOBJ DbParent, size_t MaxCache)
{
#if DEBUG
 cerr << "RCACHE::Init..";
#endif
  Changed    = false;
  Sorted     = false;
  Count      = 0;
  Parent     = DbParent;
  MaxEntries = (MaxCache > 65500 || MaxCache <= 0) ? DEFCACHE : MaxCache;
  Table      = new RCacheObj[MaxEntries];
  for (size_t i=0; i < MaxEntries; i++)
    Table[i] = RCacheObj(DbParent);
#if DEBUG
  cerr << "Done" << endl;
#endif
}


PIRSET RCACHE::Fetch(INT w)
{
  PIRSET Temp = NULL;

#if DEBUG
  cerr << "Want element " << w << endl;
#endif
  if (w >=0 && (size_t)w < Count)
    {
      Temp=(IRSET *)(Table[w].ResultSet->Duplicate());
#if DEBUG
      cerr << "Got entry from cache pos" << w << endl;
#endif
    }
  return Temp;
}

INT RCACHE::Add(const RCacheObj& RCacheObject)
{
  DOUBLE Minimum=100*RCacheObject.ResultSet->GetTotalEntries() * RCacheObject.Data.Cost; // was 999999999;
  INT MinPos=-1;

#if DEBUG
  cout<< "Adding " << RCacheObject << " to cache" << endl;
#endif

  if(Count == MaxEntries){
    // Find the smallest (lightest) result set
    for(size_t i=0; i<Count; i++){
      DOUBLE weight = (Table[i].ResultSet->GetTotalEntries())*Table[i].Data.Cost;
      if(weight <Minimum){
	Minimum= weight;
	MinPos= (INT)i;
      }
    }
    if (MinPos == -1)
      return MinPos; // Error
  } else
    MinPos=Count++;
  Table[MinPos] = RCacheObject;
  Changed = true;
#if DEBUG
  cout<< "Added to cache at "<< MinPos << endl;
#endif
  return(MinPos);
}


INT RCACHE::Check(const SearchObj& SearchObject)
{
  for(size_t i=0; i<Count; i++)
    {
      if(Table[i].Data == SearchObject)
	{
	  message_log (LOG_DEBUG, "Found %s/\"%s\" (rel=%d) in slot %d",
		SearchObject.FieldName.c_str(),
		SearchObject.Term.c_str(),
		SearchObject.Relation,
		i);
	  return(i);
	}
    }
#if DEBUG
    cerr << "RCACHE: Nothing found" << endl;
#endif
  return(-1); 
}

RCACHE::~RCACHE()
{
  if (Table)
    {
      message_log (LOG_DEBUG, "Deleting RCACHE: %d objects", Count);
      delete[] Table;
      Count=0;
   }
}

void RCACHE::Write (PFILE Fp) const
{
  putObjID(objRCACHE, Fp);
  ::Write(RCACHE_VERSION, Fp);
#if 0
  Parent->GetDbFileStem().Write(Fp);
#endif
  ::Write((UINT2)Count, Fp); // Write Count
  for (size_t i=0; i<Count; i++)
    Table[i].Write(Fp);
  ::Write((BYTE)Sorted, Fp);
}


bool RCACHE::Read (PFILE Fp)
{
  obj_t obj = getObjID(Fp);
  if (obj != objRCACHE)
    {
      // Not a RCACHE Object!
       PushBackObjID(obj, Fp);
    }
  else
    {
      BYTE version = 0;
      ::Read(&version, Fp);
      if (version != RCACHE_VERSION) {
	message_log (LOG_DEBUG, "Stale RCACHE, version %d!=%d", version, RCACHE_VERSION); 
      } else {
      UINT2 TotalEntries;
      ::Read(&TotalEntries, Fp); // Fetch Count
#if 0
      STRING Fn;
      Fn.Read(Fp);
      if (Fn != Parent->GetDbFileStem())
	message_log (LOG_WARN, "Cache Clash in %s, check your %s.ini and %s.ini files",
		Parent->PersistantCacheName().c_str(),
		Fn.c_str(),
		Parent->GetDbFileStem().c_str());
      message_log (LOG_DEBUG, "Reading %d Cache %s IRSET Objects", TotalEntries, Fn.c_str());
#else
      message_log (LOG_DEBUG, "Reading %d Cache IRSET Objects", TotalEntries);
#endif
      for (Count = 0; Count < TotalEntries; Count++)
	Table[Count].Read(Fp);
      BYTE x = 0;
      ::Read(&x, Fp);
      Sorted = (x ? true : false);
      return Count != 0;
      }
    }
  return false;
}

void Write(const RCACHE& Cache, PFILE Fp)
{
  Cache.Write(Fp);
}

bool Read(RCACHE * Ptr, PFILE Fp)
{
  return Ptr->Read(Fp);
}

