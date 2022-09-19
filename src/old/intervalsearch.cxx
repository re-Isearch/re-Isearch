/*@@@
File:		intervalsearch.cxx
Version:	$Revision: 1.1 $
Description:	Class INDEX - range searching methods
Author:	 	Edward C. Zimmermann
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "common.hxx"
#include "index.hxx"
#include "irset.hxx"
#include "nlist.hxx"
#include "intfield.hxx"
#include "intlist.hxx"
#include "numbers.hxx"

#define DEBUG 1

// Flag to select searching on interval start dates or interval end dates
//const GDT_BOOLEAN SEARCH_START = GDT_TRUE;
//const GDT_BOOLEAN SEARCH_END   = GDT_FALSE;
const IntBlock SEARCH_START = START_BLOCK;
const IntBlock SEARCH_END   = END_BLOCK;

// Flag to select whether to require strict date matching (so all dates 
// in the interval must match the query conditions, or loose date matching
// (so we'll get a match if any date in the target interval matches the
// query conditions)
const GDT_BOOLEAN STRICT_MATCH = GDT_TRUE;
const GDT_BOOLEAN LOOSE_MATCH  = GDT_FALSE;

// Flag to select whether to match the query condition endpoint, or to
// use strict inequality
const GDT_BOOLEAN NOT_DURING = GDT_FALSE;
const GDT_BOOLEAN DURING     = GDT_TRUE;


PIRSET INDEX::DoIntervalSearch(const STRING& QueryTerm, const STRING& FieldName, 
			 INT4 Relation, INT4 Structure, GDT_BOOLEAN Strict)
{
  FIELDTYPE FieldType = Parent->GetFieldType(FieldName);

  logf (LOG_DEBUG, "Interval Search in '%s' with: %s (Rel=%d,Struct=%d)",
	FieldName.c_str(), QueryTerm.c_str(), (int)Relation, (int)Structure);

  if (FieldType.IsNumericalRange())
    {
      PIRSET pirset;

      if (Structure == ZLOCALNumericalRange)
	pirset = NumericalRangeSearch(QueryTerm, FieldName, Relation, Strict);
      else
	pirset = SingleNumericalSearch(QueryTerm, FieldName, Relation, Strict);
      return pirset;
    } 
  Parent->SetErrorCode(113); // "Unsupported attribute type"
  logf (LOG_DEBUG, "\"%s\": %s", QueryTerm.c_str(), Parent->ErrorMessage() );
  return(NULL);
}


// 
PIRSET INDEX::NumericalRangeSearch(const STRING& QueryTerm, const STRING& FieldName, 
		       INT4 Relation, GDT_BOOLEAN Strict)
{
  PIRSET    pirset=(PIRSET)NULL;
  PIRSET    other_pirset=(PIRSET)NULL;

  DATERANGE QueryDate=QueryTerm;
  SRCH_DATE QueryStartDate, QueryEndDate;
  
  QueryStartDate = QueryDate.GetStart();
  QueryEndDate   = QueryDate.GetEnd();

  switch(Relation) {

  case ZRelBefore:
  case ZRelLT: // added edz

    // MDStartDate < QueryStartDate
    pirset = SingleDateSearchBefore(QueryStartDate, FieldName, SEARCH_START, 
			     NOT_DURING);
    break;

  case ZRelBefore_Strict:
    // MDEndDate   < QueryStartDate
    pirset = SingleDateSearchBefore(QueryStartDate, FieldName, SEARCH_END, 
			     NOT_DURING);
    break;

  case ZRelBeforeDuring:
    // MDStartDate <= QueryEndDate
    pirset = SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelBeforeDuring_Strict:
  case ZRelLE: // adde edz
    // MDEndDate   <= QueryEndDate
    pirset = SingleDateSearchBefore(QueryEndDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuring:
  case ZRelEQ: // added edz
    // MDStartDate inside QueryDateInterval
    // || MDEndDate inside QueryDateInterval
    pirset = DateRangeSearchContains(QueryDate, FieldName, SEARCH_START, DURING);
    other_pirset = DateRangeSearchContains(QueryDate, FieldName, SEARCH_END, DURING);
    pirset->Or(*other_pirset);
    delete other_pirset;
    break;

  case ZRelDuring_Strict:
    // MDStartDate inside QueryDateInterval
    // && MDEndDate inside QueryDateInterval
    pirset = DateRangeSearchContains(QueryDate, FieldName, SEARCH_START, NOT_DURING);
    other_pirset = DateRangeSearchContains(QueryDate, FieldName, SEARCH_END, NOT_DURING);

    pirset->And(*other_pirset);
    delete other_pirset;
    break;

  case ZRelDuringAfter:
    // MDEndDate   >= QueryStartDate
    pirset = SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_END, 
			    DURING);
    break;

  case ZRelDuringAfter_Strict:
    // MDStartDate >= QueryStartDate
    pirset =
      SingleDateSearchAfter(QueryStartDate, FieldName, SEARCH_START, 
			    DURING);
    break;

  case ZRelAfter:
  case ZRelGT: // added edz

    // MDEndDate   > QueryEndDate
    pirset = SingleDateSearchAfter(QueryEndDate, FieldName, SEARCH_END, 
			    NOT_DURING);
    break;

  case ZRelAfter_Strict:
    // MDStartDate > QueryEndDate
    pirset = SingleDateSearchAfter(QueryEndDate, FieldName, SEARCH_START, 
			    NOT_DURING);
    break;

  default:
    Parent->SetErrorCode(117); // "Unsupported Relation attribute";
    logf (LOG_DEBUG, "DateRangeSearch does not have a handler for relation=%d", Relation);
    break;
  }

  return pirset;
}


// SingleDateSearch searches a field of single dates for values matching
// the date specified by the query term.  Variable precision strings are
// handled - "YYYY", "YYYYMM" or "YYYYMMDD"
PIRSET INDEX::SingleDateSearch(const STRING& QueryTerm, const STRING& FieldName, 
			INT4 Relation, GDT_BOOLEAN Strict)
{
  PIRSET      pirset=(PIRSET)NULL;
  PIRSET      pirset1=(PIRSET)NULL;
  SRCH_DATE   QueryDate (QueryTerm);

//cerr << "QueryDate = " << QueryDate << endl;

  if (!QueryDate.Ok())
    {
      return pirset; 
    }

  switch(Relation) {

  case ZRelBefore:
  case ZRelLT:
    // MDStartDate < QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, 
			     NOT_DURING);
    break;

  case ZRelBefore_Strict:
    // MDEndDate   < QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, 
			     NOT_DURING);
    break;

  case ZRelBeforeDuring:
  case ZRelLE:
    // MDStartDate <= QueryDate
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelBeforeDuring_Strict:
    // MDEndDate   <= QueryDate 
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuring:
  case ZRelEQ:

//cerr << "ZRelEQ" << endl;
    // Match => some date in the target is inside the query
    pirset = SingleDateSearchBefore(QueryDate, FieldName, SEARCH_START, DURING);
    pirset1 = SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, DURING);
//cerr << "Start Set " << pirset->GetTotalEntries() << endl;
//cerr << "End Set " << pirset->GetTotalEntries() << endl;
    pirset->And(*pirset1);
    delete pirset1;
    break;

  case ZRelDuring_Strict:
    // Match => the entire target is inside the query
    pirset =
      SingleDateSearchBefore(QueryDate, FieldName, SEARCH_END, DURING);
    pirset1 =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, DURING);
    pirset->And(*pirset1);
    delete pirset1;
    break;

  case ZRelDuringAfter:
  case ZRelGE:
    // MDEndDate   >= QueryDate
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, DURING);
    break;

  case ZRelDuringAfter_Strict:
    // MDStartDate >= QueryDate 
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, DURING);
    break;

  case ZRelAfter:
  case ZRelGT:
    // MDEndDate   > QueryDate
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_END, 
			    NOT_DURING);
    break;

  case ZRelAfter_Strict:
    // MDStartDate > QueryDate 
    pirset =
      SingleDateSearchAfter(QueryDate, FieldName, SEARCH_START, 
			    NOT_DURING);
    break;

  default:
    Parent->SetErrorCode(117); // "Unsupported Relation attribute";
    logf (LOG_DEBUG, "DateRangeSearch does not have a handler for relation=%d", Relation);

    break;
  }

  return pirset;
}

// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// true, otherwise, the interval end dates are searched.  Dates which equal
// the query date match if fIncludesEndpoint is true, otherwise the
// inequality is strict.
PIRSET INDEX::SingleDateSearchBefore(const SRCH_DATE& QueryDate, 
			      const STRING& FieldName, 
			      IntBlock FindBlock,
			      GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      YYYY=(PIRSET)NULL;
  PIRSET      YYYYMM=(PIRSET)NULL;
  PIRSET      YYYYMMDD=(PIRSET)NULL;
  SRCH_DATE   Y, YM, YMD;
  INT         Precision;
  PIRSET      p_today=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;
  //DOUBLE      fPresent=DATE_PRESENT;

  if (!QueryDate.Ok())
    {
//cerr << "Bad Date" << endl;
      return NULL;
    }

  SRCH_DATE Today ("Now");

  Y   = QueryDate;
  YM  = QueryDate;
  YMD = QueryDate;

  Precision = QueryDate.GetPrecision();
  switch(Precision) {

  case YEAR_PREC:
    if (fIncludeEndpoint) {
      YM.PromoteToMonthEnd();
      YMD.PromoteToDayEnd();
    } else {
      YM.PromoteToMonthStart();
      YMD.PromoteToDayStart();
    }
    break;

  case MONTH_PREC:
    Y.TrimToYear();
    if (fIncludeEndpoint)
      YMD.PromoteToDayEnd();
    else
      YMD.PromoteToDayStart();
    break;

  case DAY_PREC:
    Y.TrimToYear();
    YM.TrimToMonth();
    break;

  default:
    return YYYY;
  }

  // Search the YYYY dates
  YYYY = YSearchBefore(Y, FieldName, FindBlock, fIncludeEndpoint);
#if DEBUG
  YYYY->Dump();
#endif

//cerr << Y << " returned " << endl;


  // Search the YYYYMM dates
  YYYYMM = YMSearchBefore(YM, FieldName, FindBlock, fIncludeEndpoint);
#if DEBUG
  YYYYMM->Dump();
#endif
//cerr << YM << " returned " << endl;


  // Search the YYYYMMDD dates
  YYYYMMDD = YMDSearchBefore(YMD, FieldName, FindBlock, fIncludeEndpoint);
#if DEBUG
  YYYYMMDD->Dump();
#endif

//cerr << YMD << " returned " << endl;

  if (YYYYMM->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMM);
  delete YYYYMM;
  if (YYYYMMDD->GetTotalEntries() > 0)
    YYYY->Or(*YYYYMMDD);
  delete YYYYMMDD;

  // If the query date comes after today, or is equal to today, and if
  // we're searching the interval ending points, then all of the records
  // with ending date = Present match the query and we need to get them 
  // and OR them into the result set.
  if (((Today.Equals(QueryDate)) && (FindBlock == END_BLOCK) 
       && (fIncludeEndpoint))
      || ((Today.IsBefore(QueryDate)) && (FindBlock == END_BLOCK))) {
    p_today =
      DateSearch(DAY_UPPER, FieldName, ZRelGT, SEARCH_END);
    if (p_today->GetTotalEntries() > 0) {
#if DEBUG
      p_today->Dump();
#endif
      YYYY->Or(*p_today);
    }
    delete p_today;
  }

  return YYYY;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYY are matched.
PIRSET INDEX::YSearchBefore(const SRCH_DATE& DateY, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateY.GetValue();

//cerr << "DateY = " << DateY << " val = " << (long)fValue << endl;

  if (fIncludeEndpoint)
    pirset = DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the year dates, and maybe some negative 
    // values, if there were errors stored in the index.  We need to
    // remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYY dates - that is, values > 99.0, then take the Boolean AND
    // (intersection) of the two result sets.  We can use strict inequality 
    // because no YYYY value will be equal to the lower bound value.
    LowerBound =
      DateSearch(YEAR_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYYMM are matched.
PIRSET INDEX::YMSearchBefore(const SRCH_DATE& DateYM, const STRING& FieldName,
		      IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYM.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMM dates, and maybe some YYYY
    // values.  We need to remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYYMM dates - that is, values > 99999.0, then take the Boolean
    // AND (intersection) of the two result sets.  We can use strict 
    // inequality because no YYYYMM value will be equal to the lower bound 
    // value.
    LowerBound =
      DateSearch(MONTH_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come before the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise 
// the inequality is strict.  Only dates of the form YYYYMMDD are matched.
PIRSET INDEX::YMDSearchBefore(const SRCH_DATE& DateYMD, const STRING& FieldName,
		       IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET LowerBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYMD.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelLE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelLT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMMDD dates, and maybe some YYYY
    // and YYYYMM values.  We need to remove those from the result set.  
    //
    // We will search for all values greater than the lower bound value
    // for YYYYMMDD dates - that is, values > 9999999.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYYMMDD value will be equal to the 
    // lower bound value.
    LowerBound =
      DateSearch(DAY_LOWER, FieldName, ZRelGT, FindBlock);

    if (LowerBound->GetTotalEntries() > 0)
      pirset->And(*LowerBound);
    delete LowerBound;
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates 
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.
PIRSET INDEX::SingleDateSearchAfter(const SRCH_DATE& QueryDate, 
			     const STRING& FieldName, 
			     IntBlock FindBlock,
			     GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      YYYY=(PIRSET)NULL;
  PIRSET      YYYYMM=(PIRSET)NULL;
  PIRSET      YYYYMMDD=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;


  if (!QueryDate.Ok())
    {
//cerr << "Bad Date" << endl;
       return NULL;
    }

  SRCH_DATE Y   (QueryDate);
  SRCH_DATE YM  (QueryDate);
  SRCH_DATE YMD (QueryDate);

  INT Precision = QueryDate.GetPrecision();

  switch(Precision) {

  case YEAR_PREC:
    if (fIncludeEndpoint) {
      YM.PromoteToMonthStart();
      YMD.PromoteToDayStart();
    } else {
      YM.PromoteToMonthEnd();
      YMD.PromoteToDayEnd();
    }
    break;

  case MONTH_PREC:
    Y.TrimToYear();
    if (fIncludeEndpoint)
      YMD.PromoteToDayStart();
    else
      YMD.PromoteToDayEnd();
    break;

  case DAY_PREC:
    Y.TrimToYear();
    YM.TrimToMonth();
    break;

  default:
    return YYYY;
  }

//cerr << "Now searching" << endl;

  // Search the YYYY dates
  YYYY = YSearchAfter(Y, FieldName, FindBlock, fIncludeEndpoint);
  //cerr << "Search for " << Y << " returned " << YYYY->GetTotalEntries() << endl;
#if DEBUG
  YYYY->Dump();
#endif

  // Search the YYYYMM dates
  YYYYMM = YMSearchAfter(YM, FieldName, FindBlock, fIncludeEndpoint);
  //cerr << "Search for " << YM << " returned " << YYYYMM->GetTotalEntries() << endl;

#if DEBUG
  YYYYMM->Dump();
#endif

  // Search the YYYYMMDD dates
  YYYYMMDD = YMDSearchAfter(YMD, FieldName, FindBlock, fIncludeEndpoint);
  //cerr << "Search for " << YMD << " returned " << YYYYMMDD->GetTotalEntries() << endl;

#if DEBUG
  YYYYMMDD->Dump();
#endif
  if (YYYYMMDD == NULL)
    logf (LOG_PANIC, "NULL IRSET in date search!!!");

  if (YYYYMM)
    {
      if (YYYYMM->GetTotalEntries() > 0)
	YYYYMMDD->Or(*YYYYMM);
      delete YYYYMM;
    }
  if (YYYY)
    {
      if (YYYY->GetTotalEntries() > 0)
	YYYYMMDD->Or(*YYYY);
      delete YYYY;
    }

  return YYYYMMDD;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYYMMDD are matched.
PIRSET INDEX::YMDSearchAfter(const SRCH_DATE& DateYMD, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET    pirset=(PIRSET)NULL;
  PIRSET    UpperBound=(PIRSET)NULL;
  DOUBLE    fValue;
  //DOUBLE    fPresent=DATE_PRESENT;
  SRCH_DATE Today;

  Today.GetTodaysDate();

  fValue = DateYMD.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // The result set contains hits with ending date=Present (assuming
    // the index contains records with Present dates).  The question is
    // whether they should be left in the result set or not.
    //
    // If we're searching interval starting dates, it doesn't matter what 
    // the ending date is, and only ending dates can have the value Present.
    //
    // If the query date comes before the present date, then those records
    // match the query and we're done.
    //
    // If the query date is after the present date, records with ending 
    // date = Present don't match the query and we have to trim them off.
    // Note we only have to do this if we're searching interval ending 
    // dates.
    //
    if (((Today.IsBefore(DateYMD)) && (FindBlock == END_BLOCK)) 
	|| ((Today.Equals(DateYMD)) && (FindBlock == END_BLOCK))) {
      if (fIncludeEndpoint)
	UpperBound =
	  DateSearch(DAY_UPPER, FieldName, ZRelLT, FindBlock);
      else
	UpperBound =
	  DateSearch(DAY_UPPER, FieldName, ZRelLE, FindBlock);

      //      if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //      delete UpperBound;
    }
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYYMM are matched.
PIRSET INDEX::YMSearchAfter(const SRCH_DATE& DateYM, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET UpperBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateYM.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYYMM dates, and maybe some bigger
    // values. We need to remove those from the result set.  
    //
    // We will search for all values less than the upper bound value
    // for YYYYMM dates - that is, values < 1000000.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYYMM value will be equal to the 
    // upper bound value.
    UpperBound =
      DateSearch(MONTH_UPPER, FieldName, ZRelLT, FindBlock);

    //    if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //    delete UpperBound;
  }

  return pirset;
}


// Searches for stored dates which come after the date specified in the 
// query.  The interval starting dates are searched if FindBlock is
// START_BLOCK, otherwise, the interval end dates are searched.  Dates
// which equal the query date match if fIncludesEndpoint is true, otherwise
// the inequality is strict.  Only dates of the form YYYY are matched.
PIRSET INDEX::YSearchAfter(const SRCH_DATE& DateY, const STRING& FieldName,
		     IntBlock FindBlock, GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET pirset=(PIRSET)NULL;
  PIRSET UpperBound=(PIRSET)NULL;
  DOUBLE fValue;

  fValue = DateY.GetValue();

  if (fIncludeEndpoint)
    pirset = 
      DateSearch(fValue, FieldName, ZRelGE, FindBlock);
  else
    pirset = 
      DateSearch(fValue, FieldName, ZRelGT, FindBlock);

  // If we got no hits, we're done
  if (pirset->GetTotalEntries() > 0) {
    // Now, pirset contains all the YYYY dates, and maybe some bigger
    // values.  We need to remove those from the result set.  
    //
    // We will search for all values less than the upper bound value
    // for YYYY dates - that is, values < 10000.0, then take the 
    // Boolean AND (intersection) of the two result sets.  We can use 
    // strict inequality because no YYYY value will be equal to the 
    // upper bound value.
    UpperBound =
      DateSearch(YEAR_UPPER, FieldName, ZRelLT, FindBlock);

    //    if (UpperBound->GetTotalEntries() > 0)
      pirset->And(*UpperBound);
      //    delete UpperBound;
  }

  return pirset;
}


// This is for searching interval data to match dates inside the
// interval specified by the user's query
PIRSET INDEX::DateRangeSearchContains(const DATERANGE& QueryDate, 
			      const STRING& FieldName,
			      IntBlock FindBlock,
			      GDT_BOOLEAN fIncludeEndpoint)
{
  PIRSET      pirset  = (PIRSET)NULL;
  PIRSET      pirset1 = (PIRSET)NULL;
  SRCH_DATE   QueryStartDate, QueryEndDate;
  DATERANGE   TmpDate = QueryDate;
  //INT         Precision;
  SRCH_DATE   Today;
  PIRSET      p_today=(PIRSET)NULL;
  GDT_BOOLEAN AddPresentResults=GDT_FALSE;
  //DOUBLE      fPresent=DATE_PRESENT;

  Today.GetTodaysDate();

  QueryStartDate = TmpDate.GetStart();
  QueryEndDate   = TmpDate.GetEnd();

  // FindBlock says whether we're looking for indexed start dates or
  // end dates (or even global pointer).
  // 
  // To be in the interval, the target date has to be before the ending
  // date of the query interval
  pirset = SingleDateSearchBefore(QueryEndDate, FieldName, FindBlock,
				 fIncludeEndpoint);

  // And the target date has to be after the starting date of the query
  pirset1 = SingleDateSearchAfter(QueryStartDate, FieldName, FindBlock,
				fIncludeEndpoint);

  // For a match, the target date has to be in both result sets
  pirset->And(*pirset1);
  delete pirset1;

  return pirset;
}


// This is the routine which does all the heavy lifting in the date
// indexes
PIRSET INDEX::DateSearch(const DOUBLE fKey, const STRING& FieldName, 
		  INT4 Relation, IntBlock FindBlock) 
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  PIRSET       pirset;
  SearchState  Status=NO_MATCH;
  INT4         Start=0, End=-1;
  STRING       Fn, TextFn;
  INTERVALLIST List;

  if (((DOUBLE)((INT4)fKey) - fKey) == 0.0)
    List.SetTruncatedSearch();

//cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;

  pirset=new IRSET(Parent);
  if (pirset == NULL)
    {
      Parent->SetErrorCode(31); // "Resources exhausted - no results available";
      return pirset;
    }

  if (! (FieldType.IsDate() || FieldType.IsDateRange()) )
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return pirset; 
    }

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!Exist(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";

      return pirset;
    }

  if (!Parent->DfdtGetFileName(FieldName, &TextFn))
    TextFn.Clear();


  switch(Relation) {
    case ZRelEQ:		// equals
      // Start is the smallest index in the table 
      // for which fKey is <= to the table value
      Status = List.Find(Fn, fKey, ZRelLE, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)   // No matching values - bail out
	break;
      // End is the largest index in the table for which
      // fKey is >= to the table value;
      Status = List.Find(Fn, fKey, ZRelGE, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelLT:		// less than
    case ZRelLE:		// less than or equal to
      // Start at the beginning of the table
      Start=0;
      // End is the largest index in the table for which 
      // fKey is <= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelEnclosedWithin:
      Status = List.Find(Fn, fKey, ZRelGT, FindBlock, &End);
      if (Status == TOO_LOW)   // We ran off the low end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)
	break;
      Status = List.Find(Fn, fKey, ZRelLT, FindBlock, &Start);
      if (Status == TOO_HIGH)    // We ran off the top end without a match
	Status = NO_MATCH;
      break;

    case ZRelGE:		// greater than or equal to
    case ZRelGT:		// greater than
      // Go to the end of the table
      End=-1;
      // Find the smallest index for which fKey is >= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      break;
  }

  // Bail out if we didn't find the value we were looking for
  if (Status == NO_MATCH)
    return pirset;

  List.SetFileName(Fn);

  switch (FindBlock) {
  case START_BLOCK:
    List.LoadTable(Start,End,START_BLOCK);
    break;

  case END_BLOCK:
    List.LoadTable(Start,End,END_BLOCK);
    break;

  case PTR_BLOCK:
    List.LoadTable(Start,End,PTR_BLOCK);
    break;
  }

  SRCH_DATE   rec_date;
  IRESULT     iresult;

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  size_t ListCount = List.GetCount();
  size_t w, old_w = 0;
  FILE  *fp = ffopen(TextFn, "rb");

  for(size_t Pointer=0; Pointer<ListCount; Pointer++){
    GDT_BOOLEAN isDeleted;
    GPTYPE      Gp;

    if ((w = Parent->GetMainMdt()->LookupByGp( Gp = List.GetGlobalStart(Pointer) ) ) != 0)
      {
	// Check Date Range
	if ( DateRange.Defined () ) {
	  if ( w != old_w ) {
	    MDTREC          mdtrec;
	    if ( Parent->GetMainMdt()->GetEntry ( w, &mdtrec ) ) {
	      rec_date = mdtrec.GetDate ();
	      isDeleted = mdtrec.GetDeleted ();
	    } else isDeleted = GDT_TRUE;
            old_w = w;
          }
	 if ( isDeleted )
	  continue;               // Don't bother since its marked deleted
	 if ( rec_date.Ok () ) {
	  if ( !DateRange.Contains ( rec_date ) )
	    continue;             // Out of range range
          }
	} // Date Range defined
	iresult.SetMdtIndex(w);
	iresult.SetHitTable ( FieldCache->FcInField(Gp, fp) );

	pirset->FastAddEntry(iresult);
      }
  }

  if (fp) ffclose(fp);

//pirset->SortBy(ByIndex);
  pirset->MergeEntries(GDT_TRUE);

  //  SetCache->Add(T1,Relation,FieldName,DBName,pirset);

  return(pirset);
}


#if 0


PIRSET INDEX::IntervalSearch(const DOUBLE fKey, const STRING& FieldName, 
		  INT4 Relation, IntBlock FindBlock) 
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  PIRSET       pirset;
  SearchState  Status=NO_MATCH;
  INT4         Start=0, End=-1;
  STRING       Fn, TextFn;
  INTERVALLIST List;

  if (((DOUBLE)((INT4)fKey) - fKey) == 0.0)
    List.SetTruncatedSearch();

//cerr << "FieldName = " << FieldName << "  FieldType = " << FieldType << endl;

  pirset=new IRSET(Parent);
  if (pirset == NULL)
    {
      Parent->SetErrorCode(31); // "Resources exhausted - no results available";
      return pirset;
    }

  if (! (FieldType.IsDate() || FieldType.IsDateRange()) )
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return pirset; 
    }

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!Exist(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";

      return pirset;
    }

  if (!Parent->DfdtGetFileName(FieldName, &TextFn))
    TextFn.Clear();


  switch(Relation) {
    case ZRelEQ:		// equals
      // Start is the smallest index in the table 
      // for which fKey is <= to the table value
      Status = List.Find(Fn, fKey, ZRelLE, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)   // No matching values - bail out
	break;
      // End is the largest index in the table for which
      // fKey is >= to the table value;
      Status = List.Find(Fn, fKey, ZRelGE, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelLT:		// less than
    case ZRelLE:		// less than or equal to
      // Start at the beginning of the table
      Start=0;
      // End is the largest index in the table for which 
      // fKey is <= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &End);
      if (Status == TOO_LOW)    // We ran off the low end without a match
	Status = NO_MATCH;
      break;

    case ZRelEnclosedWithin:
      Status = List.Find(Fn, fKey, ZRelGT, FindBlock, &End);
      if (Status == TOO_LOW)   // We ran off the low end without a match
	Status = NO_MATCH;
      if (Status == NO_MATCH)
	break;
      Status = List.Find(Fn, fKey, ZRelLT, FindBlock, &Start);
      if (Status == TOO_HIGH)    // We ran off the top end without a match
	Status = NO_MATCH;
      break;

    case ZRelGE:		// greater than or equal to
    case ZRelGT:		// greater than
      // Go to the end of the table
      End=-1;
      // Find the smallest index for which fKey is >= the table value
      Status = List.Find(Fn, fKey, Relation, FindBlock, &Start);
      if (Status == TOO_HIGH)   // We ran off the top end without a match
	Status = NO_MATCH;
      break;
  }

  // Bail out if we didn't find the value we were looking for
  if (Status == NO_MATCH)
    return pirset;

  List.SetFileName(Fn);

  switch (FindBlock) {
  case START_BLOCK:
    List.LoadTable(Start,End,START_BLOCK);
    break;

  case END_BLOCK:
    List.LoadTable(Start,End,END_BLOCK);
    break;

  case PTR_BLOCK:
    List.LoadTable(Start,End,PTR_BLOCK);
    break;
  }

  SRCH_DATE   rec_date;
  IRESULT     iresult;

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  size_t ListCount = List.GetCount();
  size_t w, old_w = 0;
  FILE  *fp = ffopen(TextFn, "rb");

  for(size_t Pointer=0; Pointer<ListCount; Pointer++){
    GDT_BOOLEAN isDeleted;
    GPTYPE      Gp;

    if ((w = Parent->GetMainMdt()->LookupByGp( Gp = List.GetGlobalStart(Pointer) ) ) != 0)
      {
	// Check Date Range
	if ( DateRange.Defined () ) {
	  if ( w != old_w ) {
	    MDTREC          mdtrec;
	    if ( Parent->GetMainMdt()->GetEntry ( w, &mdtrec ) ) {
	      rec_date = mdtrec.GetDate ();
	      isDeleted = mdtrec.GetDeleted ();
	    } else isDeleted = GDT_TRUE;
            old_w = w;
          }
	 if ( isDeleted )
	  continue;               // Don't bother since its marked deleted
	 if ( rec_date.Ok () ) {
	  if ( !DateRange.Contains ( rec_date ) )
	    continue;             // Out of range range
          }
	} // Date Range defined
	iresult.SetMdtIndex(w);
	iresult.SetHitTable ( FieldCache->FcInField(Gp, fp) );

	pirset->FastAddEntry(iresult);
      }
  }

  if (fp) ffclose(fp);

//pirset->SortBy(ByIndex);
  pirset->MergeEntries(GDT_TRUE);

  //  SetCache->Add(T1,Relation,FieldName,DBName,pirset);

  return(pirset);
}


#endif
