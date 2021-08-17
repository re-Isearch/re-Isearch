/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:		bboxlist.cxx
Version:	1.00
$Revision: 1.2 $
Description:	Class BBOXLIST
Author:		Edward C. Zimmermann
                Derived from class NLIST by J. Fullton
@@@*/

#include <stdlib.h>
#include <time.h>

#include "common.hxx"
#include "bboxlist.hxx"
#include "magic.hxx"

static const size_t BASIC_CHUNK=64;

typedef UINT4  _Count_t;

#define SIZEOF_HEADER  sizeof(_Count_t)+1 /* size of magic */


void BBOXLIST::Expand()
{
  Resize(2*Count + BASIC_CHUNK);
}

void BBOXLIST::Clear()
{
  Count      = 0;
  FileName.Clear();
  Sorted     = Unsorted;
}

void BBOXLIST::Empty()
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


BBOXLIST::BBOXLIST()
{
  MaxEntries = 0;
  table      = NULL; 
  Count      = 0;
}


BBOXLIST::BBOXLIST(INT n)
{
  MaxEntries = n > 0 ? n : 0;
  table      = MaxEntries ? new BBOXFLD[MaxEntries] : NULL;
  Count      = 0;
  Sorted     = Unsorted;
}


void BBOXLIST::Write(FILE *fp)
{
  if (fp)
    {
      for (INT i=0; i<Count; i++)
	table[i].Write(fp);
    }
}


void BBOXLIST::WriteTable()
{
  FILE  *fp;
  
  if (FileName.IsEmpty())
    message_log (LOG_ERROR, "BBOXLIST::WriteTable: FileName not set");
  else if ((fp = fopen(FileName,"wb")) != NULL)
    {
      SortByGp();  // Make sure its sorted
      Write(fp);
      fclose(fp);
    }
  else
    message_log (LOG_ERRNO, "BBOXLIST: Could not write table to '%s'", FileName.c_str());
}

// Write the combined table
//
void BBOXLIST::WriteTable(INT Offset)
{
  FILE  *fp;
  
  if ( FileName.IsEmpty())
    return;

  // Offset = 0 is start of file
  if ((fp = fopen (FileName, (Offset == 0) ? "wb" : "a+b")) != NULL)
    {
      // First, write out the count
      if (Offset == 0)
        {
          putObjID(objBBOXLIST, fp);
          ::Write((_Count_t)Count, fp);
        }

      // Now, go to the specified offset and write the table
      off_t  MoveTo = (off_t)(Offset* sizeof(BBOXFLD)) + SIZEOF_HEADER;

      // cout << "Offsetting " << MoveTo << " bytes into the file." << endl;

      if (fseek(fp, MoveTo, SEEK_SET) == 0)
        {
          for (_Count_t x=0; x<Count; x++)
            table[x].Write(fp);
        }
      else message_log (LOG_ERRNO, "Seek error in BBOXLIST");
      fclose(fp);
   }
}



INT4 BBOXLIST::LoadRawTable()
  // This is only called when the table has been written out during the
  // index creation phase, before the file has been sorted (with both
  // components dumped)
  //
  // Return the number of items read
{
  size_t n = 0;
  if (FileName.IsEmpty()) {
    message_log(LOG_ERROR, "BBOXLIST::LoadRawTable: FileName not set");
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


INT4 BBOXLIST::Read(FILE *fp)
{
  BBOXFLD   bbox;
  INT4      nCount = Count;

  if (fp == NULL || ferror(fp)) return 0; // Nothing to read

  while (!feof(fp))
    {
      if (bbox.Read(fp) < 0)
	break; // Not a bbox

      if (Count >= MaxEntries)
      	Expand();
      table[Count++] = bbox;
    }
  if (Count != nCount) Count--; 
  return Count - nCount;
}


INT BBOXLIST::LoadTable(STRING& Fn)
{
  FILE    *fp;
  INT      result = -1;

  // Load the entire list of polygons into the list object
  if ((fp = fopen(Fn,"rb")) != NULL)
    result = Read(fp);
  return result;
}


static int GpSearchCmp(const void* x, const void* y) 
{
  const GPTYPE gp1 = ((BBOXFLD *)x)->GetGlobalStart();
  const GPTYPE gp2 = ((BBOXFLD *)y)->GetGlobalStart();

  if      (gp1<gp2)  return -1;
  else if (gp1>gp2)  return 1;
  return 0;
}

//  DOUBLE     getNorth() const { return _getVal(Vertices[north]);}
//  DOUBLE     getWest() const  { return _getVal(Vertices[west]); }
//  DOUBLE     getSouth() const { return _getVal(Vertices[south]);}
//  DOUBLE     getEast() const  { return _getVal(Vertices[east]); }

static int SortCmpNorth(const void* x, const void* y)
{
  NUMBER a=((*((BBOXFLD *)x)).getNorth()) - ((*((BBOXFLD *)y)).getNorth()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}

static int SortCmpEast(const void* x, const void* y)
{
  NUMBER a=((*((BBOXFLD *)x)).getEast()) - ((*((BBOXFLD *)y)).getEast()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}
     
static int SortCmpWest(const void* x, const void* y)
{
  NUMBER a=((*((BBOXFLD *)x)).getWest()) - ((*((BBOXFLD *)y)).getWest()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}
     
static int SortCmpSouth(const void* x, const void* y)
{
  NUMBER a=((*((BBOXFLD *)x)).getSouth()) - ((*((BBOXFLD *)y)).getSouth()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}

static int SortCmpExtent(const void* x, const void* y)
{
  NUMBER a=((*((BBOXFLD *)x)).getExtent()) - ((*((BBOXFLD *)y)).getExtent()) ;
  if (a<0)
    return(-1);
  if (a>0)
    return(1);
  return(0);
}


void BBOXLIST::SortByGp()
{
  if (Sorted != ByGp)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  GpSearchCmp);
      Sorted = ByGp;
    }
}

void BBOXLIST::SortByNorth()
{
  if (Sorted != ByNorth)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  SortCmpNorth);
      Sorted = ByNorth;
    }
}
void BBOXLIST::SortByEast()
{
  if (Sorted != ByEast)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  SortCmpEast);
      Sorted = ByEast;
    }
}
void BBOXLIST::SortByWest()
{
  if (Sorted != ByWest)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  SortCmpWest);
      Sorted = ByWest;
    }
}
void BBOXLIST::SortBySouth()
{
  if (Sorted != BySouth)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  SortCmpSouth);
      Sorted = BySouth;
    }
}

void BBOXLIST::SortByExtent()
{
  if (Sorted != ByExtent)
    {
      if (table) qsort((void *)table, Count, sizeof(BBOXFLD),  SortCmpExtent);
      Sorted = ByExtent;
    }
}



BBOXFLD* BBOXLIST::GetByGp(const FC& Fc) const
{
  return GetByGp(Fc.GetFieldStart(), Fc.GetFieldEnd());
}


BBOXFLD* BBOXLIST::GetByGp(GPTYPE GpStart, GPTYPE GpEnd) const
{
  return (BBOXFLD *)bsearch((const void*)&GpStart, (void *)table, Count, sizeof(BBOXFLD), GpSearchCmp);
}


BBOXFLD* BBOXLIST::GetByIndex(size_t x) const
{
  if (x < Count && x>= 0)
    return &table[x];
  return NULL; // Out of range
}


void BBOXLIST::Resize(size_t Entries)
{
  BBOXFLD* temp=new BBOXFLD[Entries];
  UINT4    CopyCount;
  UINT4    x;

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


void BBOXLIST::Dump(ostream& os) const
{
  os << "Count=" << Count << ", MaxEntries=" << MaxEntries << endl;
  for(INT x=0; x<Count; x++)
    table[x].Dump(os);
  os << endl;
}


BBOXLIST::~BBOXLIST()
{
  if (table)
   {
     delete [] table;
     MaxEntries = 0;
     Count = 0;
   }
}


//
//
// Order: North, West, South, East
//

void BBOXLIST::WriteIndex(const STRING& Fn)
{
  SetFileName(Fn);
  LoadRawTable();
  if (Count > 0) { // was > 1
    ::remove(Fn);

    // Write Gp table
    SortByGp(); WriteTable(0);

    // North
    SortByNorth(); WriteTable(Count);

    // West
    SortByWest(); WriteTable(Count*2);

    // South
    SortBySouth(); WriteTable(Count*3);

    // East
    SortByEast(); WriteTable(Count*4);

    // Extent
    SortByExtent(); WriteTable(Count*5);
  }
}

FILE * BBOXLIST::OpenForAppend(const STRING& Fn)
{
  // New file?
  if (GetFileSize(Fn) == 0)
    return fopen(Fn, "wb");

  FILE *fp = fopen(Fn, "rb");

  if (fp == NULL)
   {
      message_log (LOG_ERRNO, "BBOXLIST:: Can't open '%s' for reading.", Fn.c_str());
      return NULL;
   }
  if (getObjID(fp)!= objBBOXLIST)
    {
      fclose(fp);
      return fopen(Fn, "a+b");
    }

  _Count_t Total;
  ::Read(&Total, fp);
  if (Total == 0) 
    {
      // Nothing in there so
      fclose(fp);
      return fopen(Fn, "wb"); // Can start from scratch
    }

  STRING TmpName = Fn + "~";

  for (size_t i =0; FileExists(TmpName); i++)
    {
      TmpName.form ("%s.%d", Fn.c_str(), (int)i);
    }
  FILE *ofp = fopen(TmpName, "wb");
  if (ofp == NULL)
    {
      // Fall into scatch (we might not have writing permission
      char     scratch[ L_tmpnam+1];
      char    *TempName = tmpnam( scratch ); 

      message_log (LOG_WARN, "Could not create '%s', trying tmp '%s'", TmpName.c_str(),
	TempName);
      if ((ofp = fopen(TempName, "wb")) == NULL)
	{
	  message_log (LOG_ERRNO, "Can't create a temporary numlist '%s'", Fn.c_str());
	  fclose(fp);
	  return NULL;
	}
      TmpName = TempName; // Set it
    }

  // Copy over
  BBOXFLD fld;
  for (UINT4 i=0; i< Total; i++)
    {
      fld.Read(fp);
      fld.Write(ofp);
    }
  fclose(fp);
  fclose(ofp);
  if (::remove(Fn) == -1)
    message_log (LOG_ERRNO, "Can't remove '%s'", Fn.c_str());
  if (RenameFile(TmpName, Fn) == -1)
    message_log (LOG_ERRNO, "Can't rename '%s' to '%s'", TmpName.c_str(), Fn.c_str());

  // Now open for append
  if ((fp = fopen(Fn, "a+b")) == NULL)
    message_log (LOG_ERRNO, "Could not open '%s' for bounding-box append", Fn.c_str());
  else
    message_log (LOG_DEBUG, "Opening '%s' for bboxlist append", Fn.c_str());
  return fp;
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
