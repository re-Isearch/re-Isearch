// $Id: gpolylist.cxx,v 1.2 2007/06/19 06:24:03 edz Exp $

/*@@@
File:		gpolylist.cxx
Version:	1.00
$Revision: 1.2 $
Description:	Class GPOLYLIST
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
                Derived from class NLIST by J. Fullton
@@@*/

#include <stdlib.h>
#include <time.h>

#include "gpolylist.hxx"
#include "magic.hxx"

static const size_t BASIC_CHUNK=64;


void GPOLYLIST::Expand()
{
  Resize(2*Count + BASIC_CHUNK);
}

void GPOLYLIST::Clear()
{
  Count      = 0;
  FileName.Clear();
  Sorted     = GDT_FALSE;
}

void GPOLYLIST::Empty()
{
  if(table)
    {
      delete [] table;
      table      = NULL;
      Count      = 0;
      MaxEntries = 0;
    }
  FileName.Empty();
}


GPOLYLIST::GPOLYLIST()
{
  MaxEntries = 0;
  table      = NULL; 
  Count      = 0;
}


GPOLYLIST::GPOLYLIST(INT n)
{
  MaxEntries = n > 0 ? n : 0;
  table      = MaxEntries ? new GPOLYFLD[MaxEntries] : NULL;
  Count      = 0;
  Sorted     = GDT_FALSE;
}


void GPOLYLIST::Write(FILE *fp)
{
  if (fp)
    {
      for (INT i=0; i<Count; i++)
	::Write(table[i], fp);
    }
}


void GPOLYLIST::WriteTable()
{
  FILE  *fp;
  
  if (FileName.IsEmpty())
    logf (LOG_ERROR, "GPOLYLIST::WriteTable: FileName not set");
  else if ((fp = fopen(FileName,"wb")) != NULL)
    {
      Sort();  // Make sure its sorted
      putObjID(objGPOLYLIST, fp); // Write Magic
      ::Write(Count, fp); // Write Count
      Write(fp);
      fclose(fp);
    }
  else
    logf (LOG_ERRNO, "GPOLYLIST: Could not write table to '%s'", FileName.c_str());
}


INT4 GPOLYLIST::LoadRawTable()
  // This is only called when the table has been written out during the
  // index creation phase, before the file has been sorted (with both
  // components dumped)
  //
  // Return the number of items read
{
  size_t n = 0;
  if (FileName.IsEmpty()) {
    logf(LOG_ERROR, "GPOLYLIST::LoadRawTable: FileName not set");
  } else {
    FILE *fp = fopen(FileName,"rb");
    if (fp)
      {
	n = Read( fp );
	fclose(fp);
      }
  }
  return n;
}


INT4 GPOLYLIST::Read(FILE *fp)
{
//INT4      nRecs=0, i=0;
  GPOLYFLD  gpoly;
  INT4      nCount = Count;

  if (fp == NULL || ferror(fp)) return 0; // Nothing to read

  // if (fp) ::Read(&nRecs, fp); // NOT YET SINCE WE DON"T WRITE via this

  while (/* i++ < nRecs && */ !feof(fp))
    {
      if (gpoly.Read(fp) < 0)
	break; // Not a gpoly

      if (Count >= MaxEntries)
      	Expand();
      table[Count++] = gpoly;
    }
  if (Count != nCount) Count--; 
  if (Count > nCount)
    {
      Cleanup();
      Sorted = GDT_FALSE;
    }
  return Count - nCount;
}


INT GPOLYLIST::LoadTable(STRING& Fn)
{
  FILE    *fp;
  INT      result = -1;

  // Load the entire list of polygons into the list object
  if ((fp = fopen(Fn,"rb"))) {
     obj_t  obj = getObjID(fp); // It it really a GPOLY Field?

     if (obj == -1 && feof(fp))
	return 0; // End-of-file

    if (obj == objGPOLYLIST)
      {
	INT4     Total;
	GPOLYFLD gfld;

	::Read(&Total, fp);

	if (Total > GetCount())
	  Resize(Total);
	SetCount(Total);

	// Loop over the objects stored in the file
	for (INT i=0;i<Total;i++)
	  {
	    if (gfld.Read(fp) == -1)
	      break;
	    if (result == -1)
	      result = 0;
	    result++;
	    table[i] = gfld;
	   }
	  Sorted = GDT_TRUE;
	}
      else /* NOT MAGIC so not sorted! */
	{
	  PushBackObjID(obj, fp);
	  result = Read(fp);
	}
  }
  return result;
}


INT GpolyfldGpSearchCmp(const void* x, const void* y) 
{
  const GPTYPE gp1 = ((GPOLYFLD *)x)->GetGlobalStart();
  const GPTYPE gp2 = ((GPOLYFLD *)y)->GetGlobalStart();

  if      (gp1<gp2)  return -1;
  else if (gp1>gp2)  return 1;
  return 0;
}

void GPOLYLIST::Sort()
{
#if 0
  // Can't sort since the lengths of each object is variable
  if (Sorted == GDT_FALSE)
    {
      if (table) qsort((void *)table, Count, sizeof(GPOLYFLD),  GpolyfldGpSearchCmp);
      Sorted = GDT_TRUE;
    }
#endif
}

GPOLYFLD* GPOLYLIST::GetGpolyByGp(const FC& Fc) const
{
  return GetGpolyByGp(Fc.GetFieldStart(), Fc.GetFieldEnd());
}


GPOLYFLD* GPOLYLIST::GetGpolyByGp(GPTYPE GpStart, GPTYPE GpEnd) const
{
/*
  // Can't do a bsearch since the length of each object is variable!
  if (Sorted && Count > 3)
    return (GPOLYFLD *)bsearch((const void*)&GpStart, (void *)table, Count, sizeof(GPOLYFLD*),GpolyfldGpSearchCmp);
*/

  // Stupid linear search - for now...
  for (INT i=0;i<Count;i++)
    {
      GPTYPE Gp;
      if ((Gp = table[i].GetGlobalStart())> GpStart && Gp<GpEnd)
	return &table[i];
  }
  return(NULL);
}


GPOLYFLD* GPOLYLIST::GetGpolyByIndex(INT x) const
{
  if (x < Count && x>= 0)
    return &table[x];
  return NULL; // Out of range
}


void GPOLYLIST::Resize(INT4 Entries)
{
  GPOLYFLD* temp=new GPOLYFLD[Entries];
  INT4      CopyCount;
  INT4      x;

  if(Entries>Count)
    CopyCount=Count;
  else {
    CopyCount=Entries;
    Count=Entries;
  }
  for(x=0; x<CopyCount; x++) {
    temp[x]=table[x];
  }
  if(table) delete [] table;
  table=temp;
  MaxEntries=Entries;
  return;
}


void GPOLYLIST::Dump(ostream& os) const
{
  os << "Count=" << Count << ", MaxEntries=" << MaxEntries << endl;
  for(INT x=0; x<Count; x++)
    table[x].Dump(os);
  os << endl;
}


GPOLYLIST::~GPOLYLIST()
{
  if (table)
   {
     delete [] table;
     MaxEntries = 0;
     Count = 0;
   }
}


void GPOLYLIST::WriteIndex(const STRING& Fn)
{
  SetFileName(Fn);
  LoadRawTable();
  if (Count > 0) { // was > 1
    ::remove(Fn);
    Sort();
    WriteTable();
  }
}

FILE * GPOLYLIST::OpenForAppend(const STRING& Fn)
{
  return fopen(Fn, "a+b"); // for now
}

#ifdef NEVER
main()
{
  NUMERICLIST list;
  STRING n;
  INT4 Start,End;
  DOUBLE val;
  n="test.62";
  fprintf(stderr,"DOUBLE is %d bytes\n",sizeof(DOUBLE));
  list.TempLoad();
  End=list.DiskFind(n, (double)88.0,5); // 5 GT
  fprintf(stderr,"\n===\n");
  Start=list.DiskFind(n,(double)88.0,1); // 1 LT
  
  if(End-Start<=1)
    fprintf(stderr,"No Match!\n");
  list.SetFileName(n.NewCString());
  list.LoadTable(Start,End);
    
}
 #endif
