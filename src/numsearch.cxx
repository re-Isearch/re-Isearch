/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/* $Id: numsearch.cxx,v 1.2 2007/06/19 06:24:03 edz Exp $ */

// TODO: The INT4 references need to be replaced with a GPTYPE
//

/*@@@
File:		numsearch.cxx
Version:	$Revision: 1.2 $
Description:	Class INDEX - numeric search methods
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "defs.hxx"
#include "string.hxx"
#include "vlist.hxx"
#include "strlist.hxx"
#include "common.hxx"
//#include "sw.hxx"
#include "soundex.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "record.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "result.hxx"
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
#include "index.hxx"
#include "fprec.hxx"
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#ifdef DICTIONARY
#include "dictionary.hxx"
#endif

#define DEBUG 0
#define NEW_CODE 1


#ifndef FIELD_WILD_MATCH
#define FIELD_WILD_MATCH(_x, _y)  (_x).FieldMatch(_y)
#endif

void INDEX::SortNumericFieldData()
{
  DFD DfdRecord;
  STRING Fn;
  FIELDTYPE FieldType;
  // INT4 Count;

  size_t total = Parent->DfdtGetTotalEntries();

  for (size_t x=1; x<=total; x++) {
    Parent->DfdtGetEntry(x,&DfdRecord);
    FieldType = DfdRecord.GetFieldType();
    if (FieldType.IsText())
      continue;
    if (Parent->DfdtGetFileName(DfdRecord,&Fn) == GDT_FALSE)
      {
        message_log (LOG_ERROR, "Could not find %s [%s] in DFD", DfdRecord.GetFieldName().c_str(), FieldType.c_str());
        continue; // ERROR
      }
    if (!FileExists(Fn))
      {
        message_log (LOG_DEBUG, "Field data for %s of type %s not found [%s].",
		DfdRecord.GetFieldName().c_str(), FieldType.c_str(), Fn.c_str());
	continue;
      }

    message_log (LOG_INFO, "Creating %s [%s]", DfdRecord.GetFieldName().c_str(), FieldType.c_str());

//  GDT_BOOLEAN    IsNumeric() const { return Type == numerical || Type == computed || Type == currency || Type == dotnumber; }
//  GDT_BOOLEAN    IsNumerical() const{ return Type == numerical || Type == ttl; }

    if (FieldType.IsNumerical() || FieldType.IsComputed() || FieldType.IsPhonetic() ||
		FieldType.IsHash() || FieldType.IsCaseHash() || FieldType.IsPrivHash() ||
		FieldType.IsCurrency() || FieldType.IsLexiHash() ) {
      NUMERICLIST().WriteIndex(Fn);
    } else if (FieldType.IsNumericalRange()) {
      INTERVALLIST().WriteIndex(Fn);
    } else if (FieldType.IsDate()) {
      DATELIST().WriteIndex(Fn);
    } else if (FieldType.IsDateRange()) {
      INTERVALLIST IntList;
#if 1
      IntList.WriteIndex(Fn);
#else
      IntList.SetFileName(Fn);
      IntList.LoadTable(0,-1);
      if ((Count = IntList.GetCount()) > 0) { // was > 1
	::remove(Fn);

	IntList.SortByStart();
	IntList.WriteTable(0);
	IntList.SortByEnd();
	IntList.WriteTable(Count);
	IntList.SortByGP();
	IntList.WriteTable(2*Count);
      }
#endif
    } else if (FieldType.IsBox()) {
      BBOXLIST().WriteIndex(Fn);
    } else if (FieldType.IsGPoly()) {
      GPOLYLIST().WriteIndex(Fn);
    } else {
      message_log (LOG_PANIC, "INDEX: No indexing method defined for field type '%s'???",  FieldType.c_str());
      continue;
    }
  }
  return;
}

PIRSET INDEX::HashSearch(const STRING& Contents, const STRING& FieldName, INT4 Relation, GDT_BOOLEAN useCase)
{
  const NUMERICOBJ  fKey = useCase ? encodeCaseHash(Contents) : encodeHash(Contents);
  message_log (LOG_DEBUG, "Numeric search Hash(\"%s\")->%F", Contents.c_str(), (double)fKey);

#ifdef FIELD_WILD_MATCH
  STRING          pattern(FieldName);
  const DFDT     *dfdtp = Parent->GetMainDfdt();

  if (!pattern.IsPlain() && dfdtp)
    {
      const size_t total = dfdtp->GetTotalEntries();
      STRING       field;
      FIELDTYPE    fType;
      IRSET       *irset0 = NULL;
      IRSET       *irset  = NULL;

      //pattern.ToUpper();
      //pattern.Replace("\\","/");
      for (size_t i=0; i<total; i++)
        {
          field = dfdtp->GetFieldName(i+1);
          fType = Parent->GetFieldType(field);
	  if ( (useCase && !fType.IsCaseHash()) || (!useCase && !fType.IsHash()))
	    continue;

          // Does it match?
          //field.Replace("\\", "/");
          if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, field))
            {
              if ((irset0 = NumericSearch(fKey, field, Relation)) != NULL)
                {
                  if (irset)
                    {
                      irset->Or(*irset0);
                      delete irset0;
                    }
                  else irset = irset0;
                }
            }
        }
      if (irset) return irset;
    }
#endif
  return NumericSearch( fKey, FieldName, Relation);
}

PIRSET INDEX::LexiHashSearch(const STRING& Term, const STRING& FieldName, INT4 Relation)
{
  if (Parent == NULL) return NULL;

//cerr << "LexiHashSearch: " << Term << endl;
 
  const NUMBER   fKey = encodeLexiHash(Term);

#ifdef FIELD_WILD_MATCH
  STRING          pattern(FieldName);
  const DFDT     *dfdtp = Parent->GetMainDfdt();

  if (!pattern.IsPlain() && dfdtp)
    {
      const size_t total = dfdtp->GetTotalEntries();
      STRING       field;
      FIELDTYPE    fType;
      IRSET       *irset0 = NULL;
      IRSET       *irset  = NULL;

      //pattern.ToUpper();
      //pattern.Replace("\\","/");
      for (size_t i=0; i<total; i++)
        {
          field = dfdtp->GetFieldName(i+1);
          fType = Parent->GetFieldType(field);
          // Is this field lexi?
          if (! fType.IsLexiHash())
            continue;

          // Does it match?
          //field.Replace("\\", "/");
          if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, field))
            {
              if ((irset0 = NumericSearch(fKey, field, Relation)) != NULL)
                {
                  if (irset)
                    {
                      irset->Or(*irset0);
                      delete irset0;
                    }
                  else irset = irset0;
                }
            }
        }
      if (irset) return irset;
    }
#endif
  return NumericSearch(fKey, FieldName, Relation);
}



PIRSET INDEX::MonetarySearch(const MONETARYOBJ& Price, const STRING& FieldName, INT4 Relation)
{
  if (Parent == NULL) return NULL;
  
  const NUMBER   fKey = Price ;

#ifdef FIELD_WILD_MATCH
  STRING          pattern(FieldName);
  const DFDT     *dfdtp = Parent->GetMainDfdt();
 
  if (!pattern.IsPlain() && dfdtp)
    {
      const size_t total = dfdtp->GetTotalEntries();
      STRING       field;
      FIELDTYPE    fType;
      IRSET       *irset0 = NULL;
      IRSET       *irset  = NULL;

      //pattern.ToUpper();
      //pattern.Replace("\\","/");
      for (size_t i=0; i<total; i++)
        {
          field = dfdtp->GetFieldName(i+1);
          fType = Parent->GetFieldType(field);
          // Is this field currency? 
          if (! fType.IsCurrency())
            continue;

          // Does it match?
          //field.Replace("\\", "/");
          if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, field))
            {
              if ((irset0 = NumericSearch(fKey, field, Relation)) != NULL)
                {
                  if (irset)
                    {
                      irset->Or(*irset0);
                      delete irset0;
                    }
                  else irset = irset0;
                }
            }
        }
      if (irset) return irset;
    }
#endif
  return NumericSearch(fKey, FieldName, Relation);
}

//

PIRSET INDEX::NumericSearch(const NUMBER fKey, const STRING& FieldName, INT4 Relation) 
{
  STRING      FieldType;
  PIRSET      pirset = new IRSET( Parent );
  SearchState Status;
  INT4        Start=-1, End=-1, ListCount;
  size_t      w, old_w = 0;
  IRESULT     iresult;
  STRING      Fn, TextFn;

  if (Parent == NULL) return NULL;

#ifdef FIELD_WILD_MATCH
  STRING          pattern(FieldName);
  const DFDT     *dfdtp = Parent->GetMainDfdt();
 
  if (!pattern.IsPlain() && dfdtp)
    {
      const size_t total = dfdtp->GetTotalEntries();
      STRING       field;
      FIELDTYPE    fType;
      IRSET       *irset0 = NULL;
      IRSET       *irset  = NULL;

      // pattern.ToUpper();
      // pattern.Replace("\\","/");
      for (size_t i=0; i<total; i++)
	{
	  field = dfdtp->GetFieldName(i+1);
	  fType = Parent->GetFieldType(field);
	  // Is this field numerical enough?
	  if (! fType.IsNumeric())
	    continue;

	  // Does it match?
          // field.Replace("\\", "/");
          if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, field))
	    {
	      if ((irset0 = NumericSearch(fKey, field, Relation)) != NULL)
		{
		  if (irset)
		    {
		      irset->Or(*irset0);
		      delete irset0;
		    }
		  else irset = irset0;
		}
            }
	}
      if (irset) return irset;
    }
#endif

#if DEBUG
  cerr << "Numeric Search for " << (long)fKey << " in " << FieldName << " with Rel " << Relation << endl;
#endif

  FIELDTYPE ft = Parent->GetFieldType(FieldName);
  if (ft.IsText())
    {
      Parent->SetErrorCode(113); // "Unsupported attribute type"
      message_log(LOG_DEBUG, "Can't search numeric in a mundane text field '%s'", FieldName.c_str());
      return pirset;
    }
  if (ft.IsDate())
    return DateSearch(SRCH_DATE((DOUBLE)fKey), FieldName, Relation);
  if (ft.IsDateRange())
    return DoDateSearch(SRCH_DATE((DOUBLE)fKey).ISOdate(), FieldName, Relation, ZStructDate);


  if (!Parent->DfdtGetFileName(FieldName, ft, &Fn))
    {
      Parent->SetErrorCode(1); // Permanent System Error 
      message_log (LOG_PANIC, "Could not create a table name for field '%s'", FieldName.c_str());
      return pirset;
    }

  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(113); // "Unsupported attribute type"
      return pirset;
    }

  if (!Parent->DfdtGetFileName(FieldName, &TextFn))
    TextFn.Clear();

/*
  if (NumFieldCache == NULL)
    NumFieldCache = new Dictionary(31); 
*/
  NUMERICLIST List;

#if NEW_CODE

  // Find the starting and ending indexes of the matching values in the table
  if (Relation == ZRelNE)
    Status = List.FindIndexes(Fn,fKey, ZRelEQ,&Start, &End);
  else
    Status = List.FindIndexes(Fn,fKey, (ZRelation_t)Relation,&Start, &End);

#if DEBUG
  switch (Status) {
    case TOO_LOW:  cerr << "too low"; break;
    case TOO_HIGH: cerr << "too high"; break;
    case NO_MATCH: cerr << "no match"; break;
    default:       cerr << "match " << (int)Status;
  }
  cerr << "Start = " << Start << "   End=" << End << endl;
#endif


#else
  switch (Relation) {

  case ZRelEQ:			// equals
  case ZRelNE:

#if DEBUG
  cerr << "Equals or Not-Equals.. Start with the smallest" << endl; 
#endif

    // Start is the smallest index in the table 
    // for which fKey is <= to the table value
    Status = List.Find(Fn, fKey, ZRelLE, &End); // was ZRelGT

#if DEBUG
  cerr << "End = " << End << endl;
#endif

    if (Status == TOO_LOW)    // We ran off the bottom end without a match
     Status = NO_MATCH;

#if DEBUG
  cerr << "<= Status = ";
  switch (Status) {
    case TOO_LOW:  cerr << "too low"; break;
    case TOO_HIGH: cerr << "too high"; break;
    case NO_MATCH: cerr << "no match"; break;
    default:       cerr << "match " << (int)Status;
  }
  cerr << endl;
#endif

    if (Status == NO_MATCH)   // No matching values - bail out
      break;

    // End is the largest index in the table for which
    // fKey is >= to the table value;
    Status = List.Find(Fn, fKey, ZRelGE, &Start); // was ZRelLT
#if DEBUG
  cerr << "Start = " << Start << endl;
  cerr << ">= Status = "; 
  switch (Status) {
    case TOO_LOW:  cerr << "too low"; break;
    case TOO_HIGH: cerr << "too high"; break;
    case NO_MATCH: cerr << "no match"; break;
    default:       cerr << "match " << (int)Status;
  }
  cerr << endl;
#endif

    if (Status == TOO_HIGH)   // We ran off the top
      Status = NO_MATCH;
    
    break;

  case ZRelLT:			// less than
  case ZRelLE:			// less than or equal to

    // Start at the beginning of the table
    Start=0;
    // End is the largest index in the table for which 
    // fKey is <= the table value
    Status = List.Find(Fn, fKey, Relation, &End);
    if (Status == TOO_LOW)    // We ran off the low end without a match
      Status = NO_MATCH;
    break;

  case ZRelGE:			// greater than or equal to
#if DEBUG
   cerr << "ZRelGE" << endl;
#endif
  case ZRelGT:			// greater than
#if DEBUG
  cerr << ">=" << endl;
#endif

    // Go to the end of the table
    End = -1;
    // Find the smallest index for which fKey is >= the table value
    Status = List.Find(Fn, fKey, Relation, &Start);
    if (Status == TOO_HIGH)   // We ran off the top end without a match
      Status = NO_MATCH;

#if DEBUG
  switch (Status) {
    case TOO_LOW:  cerr << "too low"; break;
    case TOO_HIGH: cerr << "too high"; break;
    case NO_MATCH: cerr << "no match"; break;
    default:       cerr << "match " << (int)Status;
  }
  cerr << endl;
#endif


    break;

#if DEBUG
   default:
 cerr << "Unhandled relation!" << endl;
#endif
  }

#endif

  // We've now searched...

#if DEBUG
  cerr << "Search Done" << endl;
#endif

  // Bail out if we didn't find the value we were looking for
  if (Status == NO_MATCH)
    {
      if (Relation == ZRelNE)
	{
	  if (ClippingThreshold > 0 ||
		(Parent->GetTotalRecords() <= TooManyRecordsThreshold) )
	    goto complement;
	  Parent->SetErrorCode(12); //  "Too many records retrieved";
	}
#if DEBUG
      cerr << "NOTHING FOUND" << endl;
#endif
      return pirset;
    }


#if !NEW_CODE

  List.SetFileName(Fn);

  if (Relation != ZRelEQ && Relation != ZRelNE) {
#if DEBUG
   cerr << "(0) Load Table (" << Start << ", " << End << ")" << endl;
#endif
    List.LoadTable(Start+1, End, VAL_BLOCK); // Jan 2008 // Was LoadTable(Start, End, VAL_BLOCK);
  } else if ( (End == -1) || (Start == -1) ) {
#if 0
    if (Start != -1) Start++;
    if (End != -1)   End--;
#else
    if (Start != -1) Start++;
    if (End   > 0)   End--;
#endif
#if DEBUG
    cerr << "(1) LoadTable(" << Start << ", " << End << ")" << endl;
#endif
    List.LoadTable(Start, End, VAL_BLOCK);
    
  } else {
    // OK - it was equal and we found the value.  But Start points to the
    // first value less than the hit, and End points to the first value
    // greater than the hit, so we need to bump them to get to the right 
    // values
#if DEBUG
    cerr << "(2) LoadTable(" << (Start+1) << ", " << (End-1) << ")" << endl;
#endif
    List.LoadTable(Start+1, End, VAL_BLOCK); // Jan 2008 // Was LoadTable(Start+1, End-1, VAL_BLOCK); //
  }

#endif /* NEW_CODE */

  ListCount = List.GetCount();

#if DEBUG
  cerr << "ListCount = " << ListCount << endl;
#endif

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);


  {GDT_BOOLEAN isDeleted = GDT_FALSE;
  SRCH_DATE   rec_date;
  FILE *fp = ffopen(TextFn, "rb");

#if NEW_CODE
  for(INT4 Pointer=Start; Pointer<=End; Pointer++) 
#else
  for(INT4 Pointer=0; Pointer<ListCount; Pointer++)
#endif
   {
    GPTYPE Value=List.GetGlobalStart(Pointer);
    if ((w = Parent->GetMainMdt()->LookupByGp(Value)) == 0)
       continue; // Not found

   // Check Date Range (don't do it if != was the op)
    if ( DateRange.Defined () && Relation != ZRelNE ) {
      if ( w != old_w ) {
        MDTREC          mdtrec;
        if ( Parent->GetMainMdt()->GetEntry ( w, &mdtrec ) ) {
          rec_date = mdtrec.GetDate ();
          isDeleted = mdtrec.GetDeleted ();
        } else {
          isDeleted = GDT_TRUE;
        }
        old_w = w;
      }
      if ( isDeleted )
        continue;               // Don't bother since its marked deleted

      if ( rec_date.Ok () ) {
        // Check date range
        if ( !DateRange.Contains ( rec_date ) ) {
          continue;             // Out of range range
        }
      }
    }
    // Yes..
    iresult.SetMdtIndex ( w );
    if (Relation != ZRelNE)
      iresult.SetHitTable ( FieldCache->FcInField(Value, fp) );
    pirset->FastAddEntry(iresult);
  }
  if (fp) ffclose(fp);
  }

//  pirset->SortBy(ByIndex);

  pirset->MergeEntries ( GDT_TRUE );

#if DEBUG
  cerr << "Got " << pirset->GetTotalEntries() << " entries" << endl;
#endif


  if (Relation == ZRelNE)
    {
complement:
      pirset->Not (FieldName);
    }

  //  SetCache->Add(T1,Relation,FieldName,DBName,pirset);
#if DEBUG
  message_log (LOG_DEBUG, "NumericSearch - %ld hits in %s for term='%f' relation=%d",
	pirset->GetTotalEntries(), FieldName.c_str(), fKey, Relation);
#endif
  return(pirset);
}

// Dump the numbers in an index
size_t INDEX::NumericalScan(STRLIST *Strlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn;
  NUMERICLIST  List;
  size_t       total = 0;

#if DEBUG
  cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;
#endif

  if (FieldType.IsDate())
    return DateScan(Strlist, FieldName, Start, TotalRequested);
  if (! (FieldType.IsNumerical() || FieldType.IsComputed()))
    return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.SetFileName(Fn);
  List.LoadTable(Start, -1, VAL_BLOCK);

  size_t  count = List.GetCount();
  DOUBLE  oldVal = 0; // Set to 0 to keep lint quiet
  for (size_t i=0; i<count; i++)
   {
     DOUBLE val = List.GetNumericValue(i);
     if (i==0 || val != oldVal)
       {
          Strlist->AddEntry(val);
          oldVal = val;
          total++;
          if (TotalRequested == (INT)total)
            break;
       }
   }
  return total;
}


size_t INDEX::NumericalScan(SCANLIST *Scanlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn;  
  NUMERICLIST  List;   
  size_t       total = 0;

#if DEBUG
  cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;
#endif

  if (FieldType.IsDate())
    return DateScan(Scanlist, FieldName, Start, TotalRequested);
  if (! (FieldType.IsNumerical() || FieldType.IsComputed()))
    return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.SetFileName(Fn);
  List.LoadTable(Start, -1, VAL_BLOCK);

  const size_t  count = List.GetCount();

  if (count == 0)
    return 0; // Nothing

  DOUBLE  val, oldVal = List.GetNumericValue(0);
  size_t  freq   = 1;

  for (size_t i=1; i<count; i++)
   {
     if ((val = List.GetNumericValue(i)) != oldVal)
       {
          Scanlist->AddEntry(oldVal,freq);
          oldVal = val;
	  freq = 1;
          total++;
          if (TotalRequested == (INT)total)
            break;
       }
     else
       freq++;
   }
  if (freq > 1 && (total < (size_t)TotalRequested || TotalRequested < 0))
   {
      Scanlist->AddEntry(oldVal,freq);
      total++;
   }
  return total;
}

