/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)filemap.cxx  1.7 02/05/01 00:33:19 BSN"

#include "common.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "idbobj.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "filemap.hxx"

FILEMAP::FILEMAP(const PIDBOBJ DbParent)
{
   Parent = DbParent;
   MdtCount = 0;

   mdt = Parent->GetMainMdt();
   const size_t Total = mdt->GetTotalEntries();
   Items = new struct _table[Total];

   MDTREC          Mdtrec;
   for (MdtCount = 1; MdtCount <= Total; MdtCount++) {
      if (mdt->GetEntry(MdtCount, &Mdtrec)) {
	 Items[MdtCount - 1].Path       = Mdtrec.GetFullFileName();
	 Items[MdtCount - 1].Doctype    = Mdtrec.GetDocumentType();
         Items[MdtCount - 1].GpStart    = Mdtrec.GetGlobalFileStart();
         Items[MdtCount - 1].LocalStart = Mdtrec.GetLocalRecordStart();
         Items[MdtCount - 1].LocalEnd   = Mdtrec.GetLocalRecordEnd();
      }
   }
}

static int TableCompareKeys(const void *GpPtr, const void *GpRecPtr)
{
   const GPTYPE    LocalValue = ((struct FILEMAP::_table *) GpPtr)->GpStart;
   const GPTYPE    Start = ((struct FILEMAP::_table *) GpRecPtr)->GpStart +
			((struct FILEMAP::_table *) GpRecPtr)->LocalStart;
   const GPTYPE    End = ((struct FILEMAP::_table *) GpRecPtr)->GpStart +
			((struct FILEMAP::_table *) GpRecPtr)->LocalEnd;

  return (((LocalValue >= Start) && (LocalValue <= End)) ? 0 :
	( (LocalValue < Start) ? -1 : 1));
}

GPTYPE FILEMAP::GetKeyByGlobal(GPTYPE gp) const
{
   GPTYPE          Start = 0;

   struct _table   key;
   key.GpStart = gp;
   // key.GpEnd=0;
   struct _table  *t = (struct _table *) bsearch(&key, Items, MdtCount,
        sizeof(_table), TableCompareKeys);
   if (t)
      Start = t->GpStart + t->LocalStart;
   else
     message_log(LOG_ERROR, "FILEMAP Lookup failed for %ld", (long) gp);
   return (Start);
}


GPTYPE FILEMAP::GetNameByGlobal(GPTYPE gp, PSTRING s, GPTYPE *size, GPTYPE *LS, DOCTYPE_ID *Doctype) const
{
   GPTYPE          Start = 0;

   struct _table   key;
   key.GpStart = gp;
   // key.GpEnd=0;
   struct _table  *t = (struct _table *) bsearch(&key, Items, MdtCount,
				 sizeof(_table), TableCompareKeys);
   if (t) {
      if (s)       *s = t->Path;
      if (LS)      *LS = t->LocalStart;
      if (Doctype) *Doctype = t->Doctype;
      if (size)    *size = t->LocalEnd - t->LocalStart + 1;
      Start = t->GpStart + t->LocalStart;
   } else
      message_log(LOG_ERROR, "FILEMAP Lookup failed for %ld", (long) gp);

   return (Start);
}


FILEMAP::~FILEMAP()
{
   delete[] Items;
}
