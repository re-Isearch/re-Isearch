#pragma ident  "@(#)dfdt.cxx  1.29 05/08/01 21:37:02 BSN"
/*-@@@
File:		dfdt.cxx
Version:	1.01
Description:	Class DFDT - Data Field Definitions Table
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@nonmonotonic.com
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
  Sorted = GDT_TRUE;
  Changed = GDT_FALSE;
}

DFDT::DFDT (size_t InitialSize)
{
  MaxEntries = InitialSize;
  Table = MaxEntries ? new DFD[MaxEntries] : NULL;
  TotalEntries = 0;
  Sorted = GDT_TRUE;
  Changed = GDT_FALSE;
}


// Create from a file..
DFDT::DFDT (const STRING& FileName)
{
  MaxEntries = 0;
  Table = NULL;
  Sorted = GDT_TRUE;
  Changed = GDT_FALSE;

  LoadTable (FileName);
}

// Create from a file..
DFDT::DFDT (PFILE Fp)
{
  MaxEntries = 0;
  Table = NULL;
  Sorted = GDT_TRUE;
  Changed = GDT_FALSE;

  Read (Fp);
}


DFDT::DFDT (const DFDT& OtherDfdt)
{
  MaxEntries = 0;
  Table = NULL;
  *this = OtherDfdt;
}

GDT_BOOLEAN DFDT::IsSystemFile (const STRING&) const
{
  return GDT_FALSE;
}


DFDT& DFDT::operator =(const DFDT& OtherDfdt)
{
  const size_t OtherTotal = OtherDfdt.GetTotalEntries ();
  TotalEntries = 0;
  if (OtherTotal > MaxEntries)
    Resize (OtherTotal);

  Changed = GDT_FALSE;
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
  Sorted = GDT_FALSE;
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

GDT_BOOLEAN DFDT::Read (PFILE fp)
{
  TotalEntries = 0;
  obj_t obj = getObjID(fp);
  if (obj != objDFDT)
    {
      PushBackObjID (obj, fp);
      return GDT_FALSE;
    }

  UINT2 DfdCount;
  ::Read(&DfdCount, fp);

  if (DfdCount > MaxEntries)
    {
      if (Resize (DfdCount) == GDT_FALSE) // Expand
	return GDT_FALSE;
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
  Sorted = ((x == 1) ? GDT_TRUE : GDT_FALSE);
  if (!Sorted) Sort(); // Sort
  Changed = GDT_FALSE;
  return GDT_TRUE;
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
	  Changed = GDT_FALSE;
	}
    }
  else if (-1 != FileName.Unlink ())
    {
      Changed = GDT_FALSE;
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
      GDT_BOOLEAN Found = GDT_FALSE;
      for (size_t y = 0; y < TotalEntries; y++)
	{
	  if (Table[y].GetFileNumber () == (INT)x)
	    {
	      Found = GDT_TRUE;
	      break;
	    }
	}
      if (!Found) break;
    }
  return (INT)x;
#endif
}

static GDT_BOOLEAN  _checkFieldName(const STRING& Fieldname)
{
  static const char ReservedViolationError[] =
        "Fieldname '%s' is a reserved name. Record presentations MAY hang!!";
  static const char ReservedViolationWarning[] =
        "Fieldname '%s' %s create problems. Single letter fields are reserved.";
  static const char ReservedViolationFatal[] = 
	"Fieldname '%s' contains a reserved character '%s'. Skipping.";

 if (Fieldname.IsEmpty())
    {
      return GDT_FALSE;
    }
  if (Fieldname == FULLTEXT_MAGIC)
    {
      logf (LOG_ERROR, ReservedViolationError, Fieldname.c_str());
    }
  else if (Fieldname.GetLength() == 1)
    {
      // Install the ones we don't want to complain about
      switch (Fieldname[0]) {
	case 'A': case 'a': case 'P': case 'D': case '.': case '@': break;
	case 'S': case 'L': case 'M': case 'H':
	  logf (LOG_ERROR, ReservedViolationWarning, Fieldname.c_str(), "can");
	  break;
	default:
	  logf (LOG_WARN, ReservedViolationWarning, Fieldname.c_str(), "may");
      }
    }
  else if (Fieldname.Search(  __AncestorDescendantSeperator ))
    {
      logf (LOG_ERROR, ReservedViolationFatal, Fieldname.c_str(),  __AncestorDescendantSeperator );
      return GDT_FALSE;
    }
  return GDT_TRUE;
}

void DFDT::FastAddEntry(const DFD& DfdRecord)
{
  STRING Fieldname ( DfdRecord.GetFieldName ());

  if (_checkFieldName(Fieldname) == GDT_FALSE)
    {
      return;
    }

  if (Sorted && TotalEntries > 1)
    {
      int res = Fieldname.FieldCmp( Table[TotalEntries].GetFieldName() );
      if (res == 0)
	return;
      else if (res < 0)
        Sorted = GDT_FALSE;
    }
  // Need more room?
  if (TotalEntries == MaxEntries)
    Expand ();
  Table[TotalEntries++] = DfdRecord;
}

void DFDT::AddEntry(const DFD& DfdRecord)
{
  STRING Fieldname ( DfdRecord.GetFieldName ());

  if (_checkFieldName(Fieldname) == GDT_FALSE)
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
    Changed = GDT_TRUE;
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
      Changed = GDT_TRUE;
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

  Changed = GDT_TRUE;
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

GDT_BOOLEAN DFDT::IsNumericalField (const STRING& FieldName) const
{
  size_t x = Lookup (FieldName);
  if (x == 0)
    return GDT_FALSE;
  return Table[x-1].GetAttributesPtr()->AttrGetFieldNumerical ();
}

GDT_BOOLEAN DFDT::GetAttributes (const size_t Index, PATTRLIST AttributesBuffer) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *AttributesBuffer = *(Table[Index-1].GetAttributesPtr ());
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

GDT_BOOLEAN DFDT::GetAttributes (const STRING& FieldName, PATTRLIST AttributesBuffer) const
{
  size_t x = Lookup(FieldName);
  if (x)
    {
      // Found it
      *AttributesBuffer = *(Table[x-1].GetAttributesPtr ());
      return GDT_TRUE;
    }
  // Not found
  return GDT_FALSE;
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
      Sorted = GDT_TRUE;
    }
}

GDT_BOOLEAN DFDT::GetEntry (const size_t Index, PDFD DfdRecord) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      *DfdRecord = Table[Index - 1];
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

GDT_BOOLEAN DFDT::GetDfdRecord (const STRING& FieldName, PDFD DfdRecord) const
{
  const size_t x = Lookup (FieldName);
  if (x)
    {
      *DfdRecord = Table[x - 1];
      return GDT_TRUE;
    }
  // ER
  return GDT_FALSE;
}

GDT_BOOLEAN DFDT::Expand ()
{
  return Resize (GROWTH(TotalEntries));
}

void DFDT::CleanUp ()
{
  Resize (TotalEntries);
}

void DFDT::Clear()
{
  Sorted = GDT_TRUE;
  if (TotalEntries)
    Changed = GDT_TRUE;
  TotalEntries = 0;
}

void DFDT::Empty()
{
  Sorted = GDT_TRUE;
  Changed = GDT_FALSE;
  TotalEntries = 0;
  MaxEntries = 0;
  if (Table) delete[]Table;
}


GDT_BOOLEAN DFDT::Resize (const size_t Entries)
{
  if (Entries != MaxEntries)
    {
      DFD *OldTable = Table;
      try {
	Table = new DFD[Entries];
      } catch (...) {
	logf (LOG_ERRNO, "DFDT:Resize from %u to %u elements failed!",
		(unsigned)MaxEntries, (unsigned)Entries);
	Table = OldTable;
	return GDT_FALSE;
      }
      MaxEntries = Entries;
      TotalEntries = (Entries >= TotalEntries) ? TotalEntries : Entries;
      for (size_t i = 0; i < TotalEntries; i++)
	Table[i] = OldTable[i];
      if (OldTable) delete[]OldTable;
    }
  return GDT_TRUE;
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


GDT_BOOLEAN DFDT::KillAll(IDBOBJ* DbParent)
{
  STRING      s;
  GDT_BOOLEAN result = GDT_TRUE;
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
		  logf (LOG_FATAL|LOG_ERRNO, "Can't remove/erase '%s' (%s index) contents for re-index!",
			s.c_str(), Table[i].GetFieldName().c_str());
		  result = GDT_FALSE;
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
			  logf (LOG_FATAL|LOG_ERRNO, "Can't remove/erase '%s' (%s %s index) contents for re-index!",
				sx.c_str(), Table[i].GetFieldName().c_str(), ft.c_str());
			  result = GDT_FALSE;
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

