/* $Id: dsearch.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $ */

/*@@@
File:		dsearch.cxx
Version:	$Revision: 1.1 $
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
#include "dfield.hxx"
#include "dlist.hxx"
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

#ifndef FIELD_WILD_MATCH
# define FIELD_WILD_MATCH(_x, _y)  (_x).FieldMatch(_y)
#endif

static const char *what ( SearchState Status)
{
  switch (Status) {
    case NO_MATCH: return "NO_MATCH";
    case TOO_LOW:  return "TOO_LOW";
    case TOO_HIGH: return "TOO_HIGH";
    case MATCH:    return "MATCH";
  }
  return "Unknown State";
}

PIRSET INDEX::DateSearch(const STRING& Term, const STRING& FieldName, INT4 Relation)
{
#ifdef FIELD_WILD_MATCH
  STRING          pattern(FieldName);
  const DFDT     *dfdtp = Parent->GetMainDfdt();
 
  pattern.Replace("\\", "/");
  if (pattern.IsWild() && dfdtp)
    {
      const size_t total = dfdtp->GetTotalEntries();
      STRING       field;
      FIELDTYPE    fType;
      IRSET       *irset0 = NULL;
      IRSET       *irset  = NULL;

      pattern.ToUpper();
      for (size_t i=0; i<total; i++)
	{
	  field = dfdtp->GetFieldName(i+1);
	  fType = Parent->GetFieldType(field);
	  // Is this a date field? 
	  if (! fType.IsDate())
	    continue;

	  // Does it match?
          field.Replace("\\", "/");
          if (field.GetLength() &&  FIELD_WILD_MATCH(pattern, field))
	    {
	      if ((irset0 = DateSearch(Term, field, Relation)) != NULL)
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

  FIELDTYPE   ft = Parent->GetFieldType(FieldName);

  if (ft.IsDateRange())
    return DoDateSearch(Term, FieldName, Relation, ZStructDate);

  SRCH_DATE date;

//cerr << "INDEX::DateSearch.." << endl;

  if (Term.IsNumber())
    {
      if (ft.IsNumerical() || ft.IsComputed())
	{
	  // Not a date but a number and we have a number
	  return NumericSearch(Term.GetDouble(), FieldName, Relation);
	}

      long val = Term.GetLong();

      if (Term.Search('.'))
	date.Set(Term.GetDouble());
      else if (Term.GetLength() <= 4)
	{
	  // YYYY
	  date.SetDate(val,0,0);
//	  SetPrecision(YEAR_PREC);
	}
      else if (Term.GetLength() <= 6)
	{
	  // YYYYMM
	  date.SetDate( val/100, val%100, 0 );
//	  SetPrecision(MONTH_PREC);
	}
      else if (Term.GetLength() <= 8)
	{
	  // YYYYMMDD
	  int year = val/10000;
	  int mon  = (val/100) % 100;
	  int day  = val % 100;
	  date.SetDate(year, mon, day);
	}
      else
	date.Set(val);
    } else
      date.Set(Term);

//cerr << "Now return DateSearch(" << date << "," << FieldName << ", " << (int)Relation << ")" << endl;
  return DateSearch(date, FieldName, Relation);
}


PIRSET INDEX::DateSearch(const SRCH_DATE& Key, const STRING& FieldName, INT4 Relation) 
{
  SearchState Status = NO_MATCH;
  INT4        Start=-1, End=-1, Pointer=0, ListCount;
  size_t      w, old_w = 0;
  IRESULT     iresult;
  STRING      Fn, TextFn;
  DATELIST    List;
  PIRSET      pirset = NULL;

  if (Parent == NULL)
    {
      // This should NEVER happen
      Parent->SetErrorCode(1); // "Permanent system error";
      return pirset; // Nothing
    }
  if (!Key.Ok())
    {
      Parent->SetErrorCode(125); // "Malformed search term";
      return pirset;
    }

  FIELDTYPE   ft = Parent->GetFieldType(FieldName);

//cerr << "ft = " << ft << endl;

  if (ft.IsDateRange())
    return DoDateSearch(Key.ISOdate(),FieldName,Relation, ZStructDateRange);

  pirset = new IRSET( Parent );
  if (ft.IsText())
    {
      Parent->SetErrorCode(113); // "Unsupported attribute type"
      logf(LOG_DEBUG, "Can't search numeric in a mundane text field '%s'", FieldName.c_str());
      return pirset;
    }

  if (!Parent->DfdtGetFileName(FieldName, ft, &Fn))
    {
      Parent->SetErrorCode(1); // Permanent System Error 
      logf (LOG_PANIC, "Could not create a table name for field '%s'", FieldName.c_str());
      return pirset;
    }

  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(113); // "Unsupported attribute type"
      return pirset;
    }

  if (!Parent->DfdtGetFileName(FieldName, &TextFn))
    TextFn.Clear();

//cerr << "Relation = " << (int)Relation << endl;

  switch (Relation) {

  case ZRelEQ:			// equals
  case ZRelNE:
//cerr << "ZRelEQ/NE" << endl;
    // Start is the smallest index in the table 
    // for which Key is <= to the table value
    Status = List.Find(Fn, Key, ZRelLE, &End); // Was ZRelGT 25 Dec 2007

#if 0
    if (Status == TOO_HIGH)
      {
         List.LoadTable(0, Key); 
	// Then the first element might be ==?
	if (Compare(Key, List.GetValue(0)) == 0)
	  {
	    Status = (enum SearchState)2; // MATCH;
	    goto found;
	  }
      }
#endif

    if (Status == TOO_LOW)    // We ran off the bottom end without a match
      Status = NO_MATCH;
    if (Status == NO_MATCH)   // No matching values - bail out
      break;

    // End is the largest index in the table for which
    // Key is >= to the table value;
    Status = List.Find(Fn, Key, ZRelGE, &Start); // Was ZRelLT 25 Dec 2007


    if (Status == TOO_HIGH)   // We ran off the top
      Status = NO_MATCH;
    
    break;

  case ZRelLT:			// less than
  case ZRelLE:			// less than or equal to

    // Start at the beginning of the table
    Start=0;
    // End is the largest index in the table for which 
    // Key is <= the table value
    Status = List.Find(Fn, Key, Relation, &End);
    if (Status == TOO_LOW)    // We ran off the low end without a match
      Status = NO_MATCH;
    break;

  case ZRelGE:			// greater than or equal to
  case ZRelGT:			// greater than

    // Go to the end of the table
    End = -1;
    // Find the smallest index for which Key is >= the table value
    Status = List.Find(Fn, Key, Relation, &Start);
    if (Status == TOO_HIGH)   // We ran off the top end without a match
      Status = NO_MATCH;
    break;
  }
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
      return pirset;
    }


  List.SetFileName(Fn);
  if (Relation != ZRelEQ && Relation != ZRelNE) {
    List.LoadTable(Start, End, VAL_BLOCK);
  } else if ( (End == -1) || (Start == -1) ) {
    if (Start != -1)
      Start++;
    if (End != -1)
      End--;
    List.LoadTable(Start, End, VAL_BLOCK);
    
  } else {
    // OK - it was equal and we found the value.  But Start points to the
    // first value less than the hit, and End points to the first value
    // greater than the hit, so we need to bump them to get to the right 
    // values
    List.LoadTable(Start+1, End-1, VAL_BLOCK); 
  }

//found:

  ListCount = List.GetCount();

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  {GDT_BOOLEAN isDeleted = GDT_FALSE;
  SRCH_DATE   rec_date;
  FILE *fp = ffopen(TextFn, "rb");

   for(Pointer=0; Pointer<ListCount; Pointer++){
    GPTYPE Value=List.GetGlobalStart(Pointer);
    if ((w = Parent->GetMainMdt()->LookupByGp(Value)) == 0)
       continue; // Not found

   // Check Date Range
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

    if (Relation != ZRelNE) // If NOT we can save looking for the zones
      iresult.SetHitTable ( FieldCache->FcInField(Value, fp) );
    pirset->FastAddEntry(iresult);
  }
  if (fp) ffclose(fp); }

//  pirset->SortBy(ByIndex);

  pirset->MergeEntries ( GDT_TRUE );

  if (Relation == ZRelNE)
    {
complement:
#if 1
      pirset->Not (FieldName);
#else
      IRSET *pirsetU = (IRSET *)(pirset->Not());
      size_t entries = pirsetU->GetTotalEntries();
      MDTREC mdtrec;

      pirset = new IRSET( Parent, entries );

      for (size_t i = 1; i <= entries; i++)
        {
          if (pirsetU->GetMdtrec(i, &mdtrec))
            {
	      // Check Date Range
	      if ( DateRange.Defined () )
		{
		  if ( !DateRange.Contains ( mdtrec.GetDate () ) )
		    continue; // Not in Range
		  if (mdtrec.GetDeleted ())
		    continue; // Don't bother with deleted records
		}

	      // Now look for the hits
              FCT fct ( GetFieldFCT (mdtrec, FieldName) );

              if (! fct.IsEmpty() )
		{
		  iresult = (*pirsetU)[i-1]; 
		  iresult.SetHitTable ( fct );
		  pirset->FastAddEntry(iresult);
		}
	       
            }
        }
      delete pirsetU; // Am done with it!
#endif
    }

  //  SetCache->Add(T1,Relation,FieldName,DBName,pirset);
#if DEBUG
  logf (LOG_DEBUG, "NumericSearch - %ld hits in %s for term='%f' relation=%d",
	pirset->GetTotalEntries(), FieldName.c_str(), Key, Relation);
#endif
  return(pirset);
}

// Dump the dates in an index
size_t INDEX::DateScan(STRLIST *Strlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn;
  DATELIST     List;
  size_t       total = 0;

//cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;

  if (! (FieldType.IsDate() || FieldType.IsDateRange()) ) return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.SetFileName(Fn);
  List.LoadTable(Start, -1, VAL_BLOCK);

  size_t  count = List.GetCount();

  SRCH_DATE oldDate;
  for (size_t i=0; i<count; i++)
   {
     SRCH_DATE date (List.GetValue(i));
     if (date != oldDate)
       {
	  Strlist->AddEntry(date.ISOdate());
	  oldDate = date;
	  total++;
	  if (TotalRequested == (INT)total)
	    break;
       }
   }
  return total;
}

// Dump the dates in an index
size_t INDEX::DateScan(SCANLIST *Scanlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn; 
  DATELIST     List;   
  size_t       total = 0;

//cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;

  if (! (FieldType.IsDate() || FieldType.IsDateRange()) ) return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.SetFileName(Fn);
  List.LoadTable(Start, -1, VAL_BLOCK);

  const size_t  count = List.GetCount();

  SRCH_DATE oldDate ( List.GetValue(0) ) ;
  SRCH_DATE date;
  size_t    freq = 1;
  for (size_t i=0; i<count; i++)
   {
     if ((date = List.GetValue(i)) != oldDate)
       {
	  Scanlist->AddEntry (oldDate.ISOdate(), freq);
          oldDate = date;
          total++;
          if ((size_t)TotalRequested == total)
            break;
	  freq = 1;
       }
     else
       freq++;
   }
  if (freq > 1 && (TotalRequested < 0 || total < (size_t)TotalRequested))
    {
      Scanlist->AddEntry (oldDate.ISOdate(), freq);
      total++;
    }
  return total;
}

