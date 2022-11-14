/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)dfdt.cxx"
/*-@@@
File:		dfdt.cxx
Description:	Class DFDT - Data Field Definitions Table
@@@*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "common.hxx"
#include "idbobj.hxx"
#include "dfdt.hxx"
#include "magic.hxx"

#define CHUNK 50
#define GROWTH(_X) (((3*(_X)/CHUNK) + 1)*CHUNK)

#ifndef FIELD_WILD_MATCH
#define FIELD_WILD_MATCH(_x, _y)  (_x).FieldMatch(_y)
#endif


DFDT::DFDT ()
{
  MaxEntries = 0;
  Table = NULL;
  TotalEntries = 0;
  Sorted = true;
  Changed = false;
}

DFDT::DFDT (size_t InitialSize)
{
  MaxEntries = InitialSize;
  Table = MaxEntries ? new DFD[MaxEntries] : NULL;
  TotalEntries = 0;
  Sorted = true;
  Changed = false;
}


// Create from a file..
DFDT::DFDT (const STRING& FileName)
{
  MaxEntries = 0;
  Table = NULL;
  Sorted = true;
  Changed = false;

  LoadTable (FileName);
}

// Create from a file..
DFDT::DFDT (PFILE Fp)
{
  MaxEntries = 0;
  Table = NULL;
  Sorted = true;
  Changed = false;

  Read (Fp);
}


DFDT::DFDT (const DFDT& OtherDfdt)
{
  MaxEntries = 0;
  Table = NULL;
  *this = OtherDfdt;
}

bool DFDT::IsSystemFile (const STRING&) const
{
  return false;
}


DFDT& DFDT::operator =(const DFDT& OtherDfdt)
{
  const size_t OtherTotal = OtherDfdt.GetTotalEntries ();
  TotalEntries = 0;
  if (OtherTotal > MaxEntries)
    Resize (OtherTotal);

  Changed = false;
  for (TotalEntries = 0; TotalEntries < OtherTotal; TotalEntries++)
    Table[TotalEntries] = OtherDfdt.Table[TotalEntries];
  Sorted = OtherDfdt.Sorted; // Take the sort
  return *this;
}


DFDT& DFDT::Cat (const DFDT& OtherDfdt)
{
  const size_t OtherTotal = OtherDfdt.GetTotalEntries ();
  const size_t NewTotal = TotalEntries + OtherTotal;

  if (NewTotal >= MaxEntries)
    Resize(GROWTH(NewTotal));
  for (size_t i=0; i < OtherTotal; i++)
    Table[TotalEntries++] = OtherDfdt.Table[i];
  Sorted = false;
  return *this;
}

DFDT& DFDT::operator +=(const DFDT& OtherDfdt)
{
  return Cat(OtherDfdt); 
}

void DFDT::LoadTable (const STRING& FileName)
{
  TotalEntries = 0;
  PFILE fp = FileName.Fopen ("rb");
  if (fp)
    {
      Read (fp);
      fclose (fp);
    }
}

bool DFDT::Read (PFILE fp)
{
  TotalEntries = 0;
  obj_t obj = getObjID(fp);
  if (obj != objDFDT)
    {
      PushBackObjID (obj, fp);
      return false;
    }

  UINT2 DfdCount;
  ::Read(&DfdCount, fp);

  if (DfdCount > MaxEntries)
    {
      if (Resize (DfdCount) == false) // Expand
	return false;
    }
  DFD dfd;
  ATTRLIST AttrList;
  for (size_t x = 0; x < DfdCount; x++)
    {
      INT2 FileNumber;
      ::Read(&FileNumber, fp);
      dfd.SetFileNumber(FileNumber);
      AttrList.Read(fp);
      dfd.SetAttributes (AttrList);
      Table[TotalEntries++] = dfd; // Instead of AddEntry
    }
  BYTE x;
  ::Read(&x, fp);
  Sorted = ((x == 1) ? true : false);
  if (!Sorted) Sort(); // Sort
  Changed = false;
  return true;
}

void DFDT::Flush(const STRING& FileName)
{
  if (Changed)
    {
      SaveTable (FileName);
    }
}

void DFDT::SaveTable (const STRING& FileName)
{
  if (TotalEntries)
    {
      PFILE fp = FileName.Fopen ("wb");
      if (fp)
	{
	  if (!Sorted) Sort();
	  Write (fp);
	  fclose (fp);
	  Changed = false;
	}
    }
  else if (-1 != FileName.Unlink ())
    {
      Changed = false;
    }
}


void DFDT::Write (PFILE fp) const
{
  putObjID(objDFDT, fp);
  ::Write((UINT2)TotalEntries, fp);
  for (size_t x = 0; x < TotalEntries; x++)
    {
      ::Write((INT2)(Table[x].GetFileNumber ()), fp);
      Table[x].GetAttributesPtr()->Write(fp);

    }
  ::Write(Sorted, fp); 
}

INT DFDT::GetNewFileNumber () const
{
#if 1
  return TotalEntries+1;
#else
  // We start at 001 so look untill we wrap around..
  size_t x;
  for (x = TotalEntries+1; x > 0; x = ((x+1)%1000))
    {
      // Make sure its not already being used..
      bool Found = false;
      for (size_t y = 0; y < TotalEntries; y++)
	{
	  if (Table[y].GetFileNumber () == (INT)x)
	    {
	      Found = true;
	      break;
	    }
	}
      if (!Found) break;
    }
  return (INT)x;
#endif
}

void DFDT::FastAddEntry(const DFD& DfdRecord)
{
  STRING Fieldname ( DfdRecord.GetFieldName ());

  if (checkFieldName(Fieldname) == false)
    {
      return;
    }

  if (Sorted && TotalEntries > 1)
    {
      int res = Fieldname.FieldCmp( Table[TotalEntries].GetFieldName() );
      if (res == 0)
	return;
      else if (res < 0)
        Sorted = false;
    }
  // Need more room?
  if (TotalEntries == MaxEntries)
    Expand ();
  Table[TotalEntries++] = DfdRecord;
}

void DFDT::AddEntry(const DFD& DfdRecord)
{
  STRING Fieldname ( DfdRecord.GetFieldName ());

  if (checkFieldName(Fieldname) == false)
    {
      return;
    }


  // Need more room?
  if (TotalEntries == MaxEntries) Expand ();
  // Need to sort?
  if (!Sorted) Sort();

  // Insert into "the right place"
  size_t left = TotalEntries;
  if (left == 0) {
    const int newNum = GetNewFileNumber ();
    (Table[TotalEntries++] = DfdRecord).SetFileNumber (newNum);
    Changed = true;
    return;
  } else if (left == 1) {
    const int res = Fieldname.FieldCmp( Table[0].GetFieldName() );
    if (res != 0) {
      // Not already in list...
      const int newNum = GetNewFileNumber ();
      if (res < 0) {
	Table[TotalEntries++] = Table[0];
	(Table[0] = DfdRecord).SetFileNumber (newNum);
      } else {
	(Table[TotalEntries++] = DfdRecord).SetFileNumber (newNum);
      }
      Changed = true;
    } else {
       Table[0].SetFieldType (DfdRecord.GetFieldType()); // added 9 Oct 2003 
    }
    return;
  } else {
    for (size_t i = (left - 1) / 2, oip, low = 0, high = left - 1;;) {
      int             res;
      /* binary search */
      oip = i;
      if ((res = Fieldname.FieldCmp( Table[i].GetFieldName()  )) == 0) {
	Table[i].SetFieldType (DfdRecord.GetFieldType()); // added 9 Oct 2003
	return; // Already available
      } else if (res < 0)
	high = i;
      else
	low = i;
      if (high - low <= 0 || (i = (high + low) / 2) == oip) {
	left = high;
	if ((res = Fieldname.FieldCmp( Table[left].GetFieldName())) > 0)
	  left++;
	else if (res == 0)
	  {
	    Table[left].SetFieldType (DfdRecord.GetFieldType()); // added 9 Oct 2003
	    return; // Already in list
	  }
	break;
      }
    }
  }
  if (TotalEntries > left)
    {
      for (size_t i = TotalEntries; i > left; i--)
	Table[i] = Table[i-1];
      (Table[left] = DfdRecord).SetFileNumber (GetNewFileNumber ());
    }
  else
    (Table[TotalEntries] = DfdRecord).SetFileNumber (GetNewFileNumber ());
  TotalEntries++;

#if 0
  { cerr << "Dump after Add..:" << endl;
    for (size_t i=0; i < TotalEntries; i++) cerr << "\"" << Table[i].GetFieldName() << "\" ";
    cerr << endl; }
#endif

  Changed = true;
}

STRING DFDT::GetFieldName(size_t Index) const
{
  if (Index > TotalEntries || Index <= 0) return NulString;
  return Table[Index-1].GetFieldName();
}

// edz: speed-up search
size_t DFDT::Lookup (const STRING& FieldName) const
{
  static int lastIndex = 0;

  if (FieldName.IsEmpty()) return 0;
  
  STRING Field (FieldName);
  Field.ToUpper ();

#if 1
  if (!Field.IsPlain())
    {
      STRING nFieldName;
      if (FieldExists(Field, &nFieldName) == 1) {
	if (Field != nFieldName)
	  return Lookup(nFieldName);
      }
      return 0;
    }
#endif

  // Sanity check
  if ( checkFieldName(Field) == false) return 0;


  // Only bother when 3 or more..
  if (Sorted && TotalEntries > 3)
    {
      // Binary Search
      int high = TotalEntries, low = 0, ip = (high+low)/2;
      int oip;
      do {
	oip = ip;
	const INT x = Field.FieldCmp( Table[ip].GetFieldName () );
	if (x < 0 )
	  high = ip-1;
	else if (x > 0)
	  low = ip+1;
	else
	  return ip + 1; // Found it
	if ((size_t)((ip = (high+low) / 2)) >= TotalEntries)
	  ip = TotalEntries - 1;
	else if (ip < 0)
	  ip = 0;
      } while (oip != ip);
    }
  else
    {
      // Linear search
      for (size_t i = 0; i < TotalEntries; i++)
	{
	  size_t j = (lastIndex + i) % TotalEntries;
	  if (Table[j].GetFieldName () == Field)
	    return (lastIndex = j) + 1; // Found it
	}
    }
  // Not found
#if 0
  { cerr << "\"" << Field << "\" not found. Dump: ";
    for (size_t i=0; i < TotalEntries; i++) cerr << "\"" << Table[i].GetFieldName() << "\" ";
    cerr << endl; }
#endif
  return 0;
}


INT DFDT::GetFileNumber (const STRING& FieldName) const
{
  size_t x = Lookup(FieldName);
  return x ? Table[x-1].GetFileNumber () : 0;
}

bool DFDT::IsNumericalField (const STRING& FieldName) const
{
  size_t x = Lookup (FieldName);
  if (x == 0)
    return false;
  return Table[x-1].GetAttributesPtr()->AttrGetFieldNumerical ();
}

bool DFDT::GetAttributes (const size_t Index, PATTRLIST AttributesBuffer) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *AttributesBuffer = *(Table[Index-1].GetAttributesPtr ());
      return true;
    }
  return false;
}

bool DFDT::GetAttributes (const STRING& FieldName, PATTRLIST AttributesBuffer) const
{
  size_t x = Lookup(FieldName);
  if (x)
    {
      // Found it
      *AttributesBuffer = *(Table[x-1].GetAttributesPtr ());
      return true;
    }
  // Not found
  return false;
}


static int DfdtCompare (const void *p1, const void *p2)
{
  return ((DFD *) p1)->GetFieldName().FieldCmp (((DFD *) p2)->GetFieldName());
}

void DFDT::Sort ()
{
  if (!Sorted)
    {
      QSORT (Table, TotalEntries, sizeof (Table[0]), DfdtCompare);
      Sorted = true;
    }
}

bool DFDT::GetEntry (const size_t Index, PDFD DfdRecord) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *DfdRecord = Table[Index - 1];
      return true;
    }
  return false;
}

bool DFDT::GetDfdRecord (const STRING& FieldName, PDFD DfdRecord) const
{
  const size_t x = Lookup (FieldName);
  if (x)
    {
      *DfdRecord = Table[x - 1];
      return true;
    }
  // ER
  return false;
}

bool DFDT::Expand ()
{
  return Resize (GROWTH(TotalEntries));
}

void DFDT::CleanUp ()
{
  Resize (TotalEntries);
}

void DFDT::Clear()
{
  Sorted = true;
  if (TotalEntries)
    Changed = true;
  TotalEntries = 0;
}

void DFDT::Empty()
{
  Sorted = true;
  Changed = false;
  TotalEntries = 0;
  MaxEntries = 0;
  if (Table) delete[]Table;
}


bool DFDT::Resize (const size_t Entries)
{
  if (Entries != MaxEntries)
    {
      DFD *OldTable = Table;
      try {
	Table = new DFD[Entries];
      } catch (...) {
	message_log (LOG_ERRNO, "DFDT:Resize from %u to %u elements failed!",
		(unsigned)MaxEntries, (unsigned)Entries);
	Table = OldTable;
	return false;
      }
      MaxEntries = Entries;
      TotalEntries = (Entries >= TotalEntries) ? TotalEntries : Entries;
      for (size_t i = 0; i < TotalEntries; i++)
	Table[i] = OldTable[i];
      if (OldTable) delete[]OldTable;
    }
  return true;
}

#if 0
HASH *GetFieldTypes()
{
    {
      ATTRLIST Attrlist;
for (size_t i=0;i<TotalEntries;i++)
    {
      Table[i].GetAttributes(&Attrlist);
      FieldTypes->AddEntry( AttrList.AttrGetFieldName(),
        AttrList.AttrGetFieldType() );
    }
}
#endif


STRING DFDT::GetFileName(IDBOBJ* DbParent, const STRING& FieldName)
{
  if (DbParent)
    {
      size_t i = Lookup(FieldName);
      if (i)
	return DbParent->ComposeDbFn (Table[i-1].GetFileNumber());
    }
  return NulString;
}


bool DFDT::KillAll(IDBOBJ* DbParent)
{
  STRING      s;
  bool result = true;
  for (size_t i = 0; i < TotalEntries; i++)
    {
      s = DbParent->ComposeDbFn (Table[i].GetFileNumber());
      if (FileExists(s))
	{
	  if (UnlinkFile (s) == -1)
	    {
	      STRING bak(s + "~");
	      if (RenameFile(s, bak) == 0)
		{
		  AddtoGarbageFileList(bak);
		}
	      else if (EraseFileContents(s) != 0) // Zap contents
		{
		  message_log (LOG_FATAL|LOG_ERRNO, "Can't remove/erase '%s' (%s index) contents for re-index!",
			s.c_str(), Table[i].GetFieldName().c_str());
		  result = false;
		}
	    }
	  // Zap also the object tables
	  for (int j= FIELDTYPE::text; j< FIELDTYPE::__last; j++)
	    {
	      const FIELDTYPE ft ((BYTE)j);
	      if (!ft.Ok())
	        break;
	      const char *dt = ft.datatype();
	      if (dt && *dt)
		{
		  const STRING sx ( s + ft.datatype() );
		  if (FileExists(sx) && UnlinkFile(sx) == -1)
		    {
		      STRING bak (sx + "~");
		      if (RenameFile(sx, bak) == 0)
			{
			  AddtoGarbageFileList(bak);
			}
		      else if (EraseFileContents(sx) != 0) // Zap contents
			{
			  message_log (LOG_FATAL|LOG_ERRNO, "Can't remove/erase '%s' (%s %s index) contents for re-index!",
				sx.c_str(), Table[i].GetFieldName().c_str(), ft.c_str());
			  result = false;
			}
		    }
		}
	    }
	}
    }
  TotalEntries = 0;

  // Added 26 Feb 2004. Need to get rid of the old table
  if (Table)
    {
      delete[]Table;
      Table = MaxEntries ? new DFD[MaxEntries] : NULL;
    }
  return result;
}

int DFDT::Roots(STRLIST *ListPtr) const
{
  size_t     count = 0;
  for (size_t i=0; i<TotalEntries; i++)
    {
      const char Seps[] = "\\/:";
      STRING     fn;
      size_t     len;

      len = (fn = Table[i].GetFieldName()).GetLength();
      for (size_t j = 0; Seps[j]; j++)
	{
	  if (fn.Search(Seps[j]) == len)
	    {
	      if (ListPtr) ListPtr->AddEntry(fn);
	      count++;
	      break;
	    }
	}
    }
  return count;
}

STRING DFDT::FirstRoot() const
{
  STRLIST List;
  if (Roots(&List) == 0)
    return NulString;
  return List.Next()->Value(); // Head
}

size_t DFDT::FieldExists(const STRING& FieldName) const
{
  return FieldExists(FieldName, NULL);
}

size_t DFDT::FieldExists(const STRING& FieldName, STRING *Ptr) const
{
  STRING       pattern(FieldName);
  size_t       i;

//cerr << "Looking up to see if the field exists..." << endl;

  if (!pattern.IsPlain())
    {
      STRING       fld;
      size_t       count = 0;

      // Field names are uppercase
      // pattern.ToUpper();
      // pattern.Replace("\\", "/");
      for (i=0; i<TotalEntries; i++)
	{
	  if ((fld = GetFieldName(i+1)).GetLength())
	    {
	      //fld.Replace("\\", "/");
	      if (FIELD_WILD_MATCH(pattern, fld))
		{
		  if (Ptr) *Ptr = fld;
		  count++;
		}
	    }
	}
//cerr << "MATCH " << count << endl;

       // Do we want to let not plain characters pass through as
       // field names???
       //
       // if (count)
	       return count;
     }

//cerr << "Lookup " << FieldName << endl;

  if ((i = Lookup(FieldName)) > 0)
    {
      if (Ptr) *Ptr = Table[i-1].GetFieldName();
      return 1;
    }
  return 0;
}


size_t DFDT::NumericFieldExists(const STRING& FieldName, STRING *Ptr) const
{
  STRING       pattern(FieldName);
  size_t       i;

  if (!pattern.IsPlain())
    {
      STRING       fld;
      size_t       count = 0;
          
      // Field names are uppercase
      //pattern.ToUpper();
      //pattern.Replace("\\", "/");
      for (i=0; i<TotalEntries; i++)
        {
          if ((fld = Table[i].GetFieldName()).GetLength() && Table[i].GetFieldType().IsNumeric())
            {
              //fld.Replace("\\", "/");
              if (FIELD_WILD_MATCH(pattern, fld))
		{
		  if (Ptr) *Ptr = fld;
		  count++;
                }
            }
        }
       return count;
     }
  if ((i = Lookup(FieldName)) > 0 && Table[i-1].GetFieldType().IsNumeric())
    {
      if (Ptr) *Ptr = Table[i-1].GetFieldName();
      return 1;
    }
  return 0;
}

size_t DFDT::DateFieldExists(const STRING& FieldName, STRING *Ptr) const
{
  return TypeFieldExists(FIELDTYPE::date, FieldName, Ptr);
}


size_t DFDT::TypeFieldExists(const FIELDTYPE& Ft, const STRING& FieldName, STRING *Ptr) const
{
  STRING       pattern(FieldName);
  size_t       i;

  if (!pattern.IsPlain())
    {
      STRING       fld;
      size_t       count = 0;
  
      // Field names are uppercase
      //pattern.ToUpper();
      //pattern.Replace("\\", "/");
      for (i=0; i<TotalEntries; i++)
        {
          if ((fld = Table[i].GetFieldName()).GetLength() && Table[i].GetFieldType() == Ft)
            {
	      //fld.Replace("\\", "/");
              if (FIELD_WILD_MATCH(pattern, fld))
                {
                  if (Ptr) *Ptr = fld;
                  count++;
                }
            }
        }
       return count;
     }
  if ((i = Lookup(FieldName)) > 0 && Table[i-1].GetFieldType() == Ft)
    {
      if (Ptr) *Ptr = Table[i-1].GetFieldName();
      return 1;
    }
  return 0;
}




DFDT::~DFDT ()
{
  if (Table)
    delete[]Table;
}

