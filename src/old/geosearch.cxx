/*@@@
File:		geosearch.cxx
Version:	1.0
$Revision: 1.1 $
Description:	Class INDEX - spatial search methods
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include "common.hxx"
#include "index.hxx"
#include "irset.hxx"


static void
FractionLineOverlap(DOUBLE t_min, DOUBLE t_max,
                    DOUBLE q_min, DOUBLE q_max,
                    DOUBLE* f_target, DOUBLE* f_query) ;


/////////////////////////////////////////////////////////////////////
// Determines the fractional overlap of two rectangles.  We already
// know the rectangles have some overlap since the targets are in the
// list of hits in the spatial search.  Required to compute the 
// spatial score.
// Adapted from K. Lanfear's perl implementation
/////////////////////////////////////////////////////////////////////
static void
FractionRectOverlap(DOUBLE TargetSouth,
		    DOUBLE TargetWest,
		    DOUBLE TargetNorth,
		    DOUBLE TargetEast,
		    DOUBLE QuerySouth,
		    DOUBLE QueryWest,
		    DOUBLE QueryNorth,
		    DOUBLE QueryEast,
		    DOUBLE *f_target,
		    DOUBLE *f_query)
{
  DOUBLE ftlat, fqlat, ftlon, fqlon;

  // Initialize the return values
  *f_target = 0.0;
  *f_query = 0.0;

  // Initialize the intermediate values
  ftlat = 0.0; // fractional overlap in latitude, relative to the target
  fqlat = 0.0; // fractional overlap in latitude, relative to the query
  ftlon = 0.0; // fractional overlap in longitude, relative to the target
  fqlon = 0.0; // fractional overlap in longitude, relative to the query

  // Calculate the longitute fractional overlap relative to the
  // query and the target
  FractionLineOverlap(TargetWest,TargetEast,QueryWest,QueryEast,
		      &ftlon,&fqlon);
  if (ftlon <= 0.0) {
    return;
  }
  // Calculate the latitude fractional overlap relative to the
  // query and the target
  FractionLineOverlap(TargetSouth,TargetNorth,QuerySouth,QueryNorth,
		      &ftlat,&fqlat);

  // For the target, the total fractional overlap is the product of the
  // latitude overlap and the longitude overlap
  *f_target = ftlat * ftlon;

  // Similarly for the query
  *f_query = fqlat * fqlon;
  return;
}


/////////////////////////////////////////////////////////////////////
// Examines two parallel lines (target and query) and determines the
// fractional overlap of each.  Required to compute the spatial score.
// Adapted from K. Lanfear's perl implementation
/////////////////////////////////////////////////////////////////////
static void
FractionLineOverlap(DOUBLE t_min, DOUBLE t_max,
		    DOUBLE q_min, DOUBLE q_max,
		    DOUBLE* f_target, DOUBLE* f_query)
{
  DOUBLE overlap;

  // Initialize the return values
  *f_target = 0.0;
  *f_query = 0.0;

  // Bail if the intervals are disjoint
  if ( (q_min >= t_max) ||(t_min >= q_max)) {
    return;
  }
  overlap = t_max-q_min;

  // Adjust the overlap for any extra overhang
  if (q_max<t_max)
    overlap -= (t_max-q_max);

  if (q_min<t_min)
    overlap -= (t_min-q_min);

  // Now, if they do overlap, scale to the full range for both
  // query and target and send the values back
  if (overlap>0) {
    *f_target = overlap / (t_max-t_min);
    *f_query = overlap / (q_max-q_min);
  }
}

/////////////////////////////////////////////////////////////////////
// Compute the fractional overlap between the query box and all of the 
// hits.  This only calculates the overlaps - the actual score gets 
// computed (and combined) along with the other scores later.
//
// Because we compute these only one per hit, we are not following
// Lanfear's original implementation and scaling the scores to the
// sum of the hit areas.  And because of this, we never actually need
// to calculate the areas of the query nor target boxes - it all just
// factors out to 1 anyway.
/////////////////////////////////////////////////////////////////////
void INDEX::SetSpatialScores(IRSET* pirset,
			STRING FieldName,
			NUMBER NorthBC,
			NUMBER SouthBC,
			NUMBER WestBC,
			NUMBER EastBC)
{
  IRESULT ResultRecord;
  MDTREC mdtrec;
  DOUBLE target_area, hit_area;
  STRING ResultKey, Fn;
  GPOLYLIST *list;
  GPOLYFLD *target;
  DOUBLE TargetWest,TargetNorth,TargetEast,TargetSouth;
  DOUBLE AreaQ, AreaT;
  DOUBLE f_target, f_query;

  DOUBLE QueryWest = WestBC;
  DOUBLE QueryNorth = NorthBC;
  DOUBLE QueryEast = EastBC;
  DOUBLE QuerySouth = SouthBC;

  if (QueryEast < QueryWest)
    QueryEast += 360.0;

  list = new GPOLYLIST;

  Parent->DfdtGetFileName(FieldName, FIELDTYPE::gpoly, &Fn);
  list->LoadTable(Fn);

  MDT *ThisMdt = Parent->GetMainMdt();
  const INT n_hits = pirset->GetTotalEntries();
  // Then walk through the IRSET to get the RESULT objects
  for (INT i=0; i<n_hits; i++) {
    pirset->GetEntry(i+1,&ResultRecord);
    INT mdt_index = ResultRecord.GetMdtIndex();
    ThisMdt->GetEntry(mdt_index, &mdtrec);

    const GPTYPE GpStart = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();
    const GPTYPE GpEnd = GpStart + mdtrec.GetLocalRecordEnd();

    // Retrieve the coordinates
    target=list->GetGpolyByGp(GpStart,GpEnd);

    if (target == NULL)
      {
	logf (LOG_DEBUG, "INDEX::SetSpatialScores: Empty target");
	continue;
      }

    // West, North, East, South
    if ((target->GetVertexN(0,&TargetWest))
	&& (target->GetVertexN(1,&TargetNorth))
	&& (target->GetVertexN(2,&TargetEast))
	&& (target->GetVertexN(3,&TargetSouth))) {
      if (TargetEast < TargetWest)
	TargetEast += 360.0;

      FractionRectOverlap(TargetSouth,TargetWest,TargetNorth,TargetEast,
			  QuerySouth,QueryWest,QueryNorth,QueryEast,
			  &f_target,&f_query);
    } else {
      f_target = 0.0;
      f_query = 0.0;
    }

    // Store the fractions in the result record for later use
    ResultRecord.SetGscore(GEOSCORE(f_target, f_query));
    pirset->AddEntry(ResultRecord,0);
  }

  return;
}


void INDEX::LoadBoundingBoxFieldNames()
{
  if (NorthFieldName.IsEmpty())
    {
      const STRLIST *opts = Parent ? Parent->GetDocTypeOptionsPtr() : NULL;
      STRING   field;
      STRING   NorthBC, EastBC, WestBC, SouthBC;
      const char message[] = "[%s] %s=%s";
      STRING   suffix("BC");
      STRING   section("BOUNDING-BOX");

      field = "NORTH";
      if (opts == NULL || opts->GetValue (field, &NorthBC) == 0)
	NorthBC = field + suffix;
      Parent->ProfileGetString(section, field, NorthBC, &NorthFieldName);
      logf (LOG_DEBUG, message, section.c_str(), field.c_str(), NorthFieldName.c_str());


      field = "EAST";
      if (opts == NULL || opts->GetValue (field, &EastBC) == 0)
        EastBC = field + suffix;
      Parent->ProfileGetString(section, field, EastBC, &EastFieldName);
      logf (LOG_DEBUG, message, section.c_str(), field.c_str(), EastFieldName.c_str());


      field = "SOUTH";
      if (opts == NULL || opts->GetValue (field, &SouthBC) == 0)
        SouthBC = field + suffix;
      Parent->ProfileGetString(section, field, SouthBC, &SouthFieldName);
       logf (LOG_DEBUG, message, section.c_str(), field.c_str(), SouthFieldName.c_str());


      field = "WEST";
      if (opts == NULL || opts->GetValue (field, &WestBC) == 0)
        WestBC = field + suffix;
      Parent->ProfileGetString(section, field, WestBC, &WestFieldName);
      logf (LOG_DEBUG, message, section.c_str(), field.c_str(), WestFieldName.c_str());
   }
//cerr << "BoundingBoxFieldNames Loaded" << endl;

}


// CANONICAL ORDER(!)
IRSET* INDEX::BoundingRectangle(const char *tp,
	NUMBER *N, NUMBER *S, NUMBER *W, NUMBER *E)
{
  double No=0,So=0,West=0,East=0;
  char tmp[1024];
  char endCh = '\0';

//cerr << "BoundingRectangle Called: " << tp << endl;

  while (isspace(*tp)) tp++;

  // Note: May contain RECT{  (Observation of Nov 2005)
  // so lets skip alpha 
  while (isalpha(*tp)) tp++;


  if      (*tp == '[') endCh = ']';
  else if (*tp == '(') endCh = ')';
  else if (*tp == '<') endCh = '>'; 
  else if (*tp == '{') endCh = '}';

  if (endCh)
    {
      tp++; // Now skip the character
      while (isspace(*tp)) tp++;
    }

  for (int i=0; i<sizeof(tmp) && *tp; tp++)
    {
      if (*tp == ',' || *tp == ';' || iscntrl(*tp))
	{
	  tmp[i++] = ' ';
	}
      else if (*tp == endCh)
	{
	  tmp[i] = '\0';
	  break;
	}
      else
	tmp[i++] = *tp;
    }

  // Now expect canonical term order
  if (sscanf(tmp,"%lf %lf %lf %lf",&No,&West, &So, &East) == 4)
    {
      if (N) *N = No;
      if (S) *S = So;
      if (W) *W = West;
      if (E) *E = East;
//cerr << "North = " << No << " West=" << West << " South=" << So << " East=" << East <<endl;
      return BoundingRectangle(No,So,West,East);
    }

  Parent->SetErrorCode(124); // "Unsupported coded value for term"
  return NULL;
}



IRSET* INDEX::BoundingRectangle(NUMBER NorthBC, NUMBER SouthBC, NUMBER WestBC, NUMBER EastBC)
{
  
  // rationale: if any of the 4 intervals constructed from this set of
  // points intersects a target interval in the database, our
  // BoundingRectangle overlaps the target rectangle that contains the
  // interval.
  
  // There are certain special cases we need to worry about:
  //
  // One: a target that overlaps the International Date Line - I don't
  // know how to deal with that except in the indexer, so I won't
  // worry with it right now.  It's fixable, though.
  //
  // Two: a query that overlaps the International Data line.  For that
  // case, we break the query into two parts (8 intervals) Search
  // separately, and OR the results 

  // We have to adjust the boundaries to make sure the "direction" is
  // correct.  Don't be confused by terminology - the North
  // *longitudinal* boundary is the East/West longitudinal line
  // defined by the coordinate points that is furthest North.
  
  // we must add code to find all that are entirely enclosed by the
  // query.
  
  
  GDT_BOOLEAN DateLineIntersection=GDT_FALSE;
  
  // Unified Bounding box field names
  LoadBoundingBoxFieldNames();

  // first, build a result set of all the totally enclosed
  // items in the database.
  // all < NorthQuery
  //  AND
  // all> SouthQuery
  //  AND
  // all < EastQuery
  //  AND
  // all > WestQuery

  IRSET *LessThanNorth; // We carry this one over

//cerr << "NorthBC = " << NorthBC << "   NorthField = " << NorthFieldName << endl;

  if ((LessThanNorth=NumericSearch(NorthBC,NorthFieldName, ZRelLT)) == NULL)
    return (IRSET*)NULL;


  if (LessThanNorth->GetTotalEntries() > 0)
    {
      // Now North AND East
      IRSET* LessThanEast=NumericSearch(EastBC, EastFieldName, ZRelLT);

      if (LessThanEast == NULL)
        {
	  // Less than East was not defined so box is not defined
	  delete LessThanNorth;
	  LessThanNorth = new IRSET(Parent);
	}
      else
        {
	  LessThanNorth->And(*LessThanEast);
	  delete LessThanEast;
        }
    }



  if (LessThanNorth->GetTotalEntries() > 0)
    {
      IRSET *MoreThanSouth=NumericSearch(SouthBC, SouthFieldName, ZRelGE /* was 4 */);
  
      IRSET *MoreThanWest=NumericSearch(WestBC, WestFieldName, ZRelGE);
  
      if (MoreThanWest && MoreThanSouth)
	{
	  MoreThanSouth->And(*MoreThanWest);
	  delete MoreThanWest;
	}
  
      if (MoreThanSouth)
	{
	  LessThanNorth->And(*MoreThanSouth);
	  delete MoreThanSouth;
	}
    }
  
  // Northernmost longitudinal boundary:
  
  if (WestBC > EastBC)	// we cross the DateLine
    DateLineIntersection=GDT_TRUE;
  else
    DateLineIntersection=GDT_FALSE;
 

  IRSET *NorthLongitude; 
 
  if (DateLineIntersection == GDT_TRUE) {

    IRSET *WestDateLineNL =Interval(WestBC, 180.0, NorthBC, NorthBC);
    IRSET *EastDateLineNL =Interval(-180.0, EastBC, NorthBC, NorthBC);

    if (WestDateLineNL && EastDateLineNL)
      {
	WestDateLineNL->Or(*EastDateLineNL);
	NorthLongitude=WestDateLineNL;
	delete EastDateLineNL;
      }
    else
      NorthLongitude = EastDateLineNL ? EastDateLineNL : WestDateLineNL;

  } else
    NorthLongitude=Interval(WestBC, EastBC, NorthBC, NorthBC);
  
  // we now have a north longitude result set.  
  // Southernmost longitudinal boundary:
  
  if (WestBC > EastBC)	// we cross the DateLine
    DateLineIntersection=GDT_TRUE;
  else
    DateLineIntersection=GDT_FALSE;
 

  IRSET *SouthLongitude;
 
  if (DateLineIntersection == GDT_TRUE) {
    IRSET *WestDateLineSL =Interval(WestBC, 180.0, SouthBC, SouthBC);
    IRSET *EastDateLineSL =Interval(-180.0, EastBC, SouthBC, SouthBC);

    if (WestDateLineSL && EastDateLineSL)
      {
	WestDateLineSL->Or(*EastDateLineSL);
	SouthLongitude=WestDateLineSL;
	delete EastDateLineSL;
      }
    else
      SouthLongitude = EastDateLineSL ? EastDateLineSL : WestDateLineSL;
    
  } else
    SouthLongitude=Interval(WestBC, EastBC, SouthBC, SouthBC);
  
  // our north and south longitudinal boundary intersections
  // have been computed.
  //
  // now compute intersections for eastern and western latitudinal 
  // latitudinal boundaries.  I don't deal with the Poles.  Queries
  // can't overlap the Polar regions.  I can fix that...
  
  IRSET *EastLatitude=Interval(EastBC, EastBC, SouthBC, NorthBC);
  IRSET *WestLatitude=Interval(WestBC, WestBC, SouthBC, NorthBC);
  
  // OR these buzzards together for a set of hits...
 
  if (WestLatitude)
    { 
      if (EastLatitude == NULL)
	{
	  EastLatitude = WestLatitude;
	}
      else
	{
	  EastLatitude->Or(*WestLatitude);
	  delete WestLatitude;
	}
    }
  if (NorthLongitude)
    {
      if (EastLatitude == NULL)
	{
	  EastLatitude = NorthLongitude;
	}
      else
	{
	  EastLatitude->Or(*NorthLongitude);
	  delete NorthLongitude;
	}
    }
  if (SouthLongitude)
    {
      if (EastLatitude == NULL)
	{
	  EastLatitude = SouthLongitude;
	}
      else
	{
	  EastLatitude->Or(*SouthLongitude);
	  delete SouthLongitude;
	}
    }

  if (EastLatitude)
    {
      logf (LOG_DEBUG, "%i Hits from Rect", (int) EastLatitude->GetTotalEntries());
      EastLatitude->Or(*LessThanNorth);
      delete LessThanNorth;
      return(EastLatitude);	// our combined set of hits...
    }

  logf (LOG_DEBUG, "EastLatitude not defined, returning North case");
  return LessThanNorth;
}


// this functions takes a pair of points forming an interval
// and match them to intervals in the database.
// option - pass the field names with the values.
// We assume that the West Longitude is <= East Longitude
// and South Latitude <=North Latitude

IRSET* INDEX::Interval(NUMBER WestLongitude, NUMBER EastLongitude,
		NUMBER SouthLatitude, NUMBER NorthLatitude)
{
  // goal - find intervals in the database that intersect this interval.
  
  
//  CHR TempBuffer[256];
  IRSET* ResultA;
  IRSET* ResultB;
  IRSET* ResultC;
  IRSET* ResultD;
  STRING Query,FieldName;

  LoadBoundingBoxFieldNames();
  
  // Put a cacheing structure here to avoid search duplication

  ResultA=NumericSearch(EastLongitude, WestFieldName, ZRelLE);
  logf(LOG_DEBUG, "Got %i entries <= %f in %s", (int)ResultA->GetTotalEntries(),
	 (double)EastLongitude, WestFieldName.c_str());
  
  if (ResultA->GetTotalEntries() == 0) {
    // no hits rules out entire search
  if (DebugMode)
    logf(LOG_DEBUG, "No hits in last search, so bailing out.");

    return(ResultA);
  }  
  

  ResultB=NumericSearch(WestLongitude, EastFieldName, ZRelGE);
  if (DebugMode)
    logf(LOG_DEBUG, "Got %i entries >= %f in %s", ResultB->GetTotalEntries(),
	 (double)WestLongitude, EastFieldName.c_str());
  
  if (ResultB->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
  if (DebugMode)
    logf(LOG_DEBUG, "No hits in last search, so bailing out");
    return(ResultB);
  }
  
  // two valid result sets.  AND them
  
  ResultA->And(*ResultB);
  delete ResultB;
  if (ResultA->GetTotalEntries() == 0) {
    // no hits rules out entire search
    if (DebugMode)
      logf(LOG_DEBUG, "No hits in last search, so bailing out");
    return(ResultA);
  }
  if (DebugMode)
    logf(LOG_DEBUG, "Now have %i entries in range.", ResultA->GetTotalEntries());
  
  // ResultA Now contains a valid interval set
  
  ResultC=NumericSearch(SouthLatitude, NorthFieldName, ZRelGE);
  logf(LOG_DEBUG, "Got %i entries >= %f in %s", ResultC->GetTotalEntries(),
	(double)SouthLatitude, NorthFieldName.c_str());
  
  if (ResultC->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
    if (DebugMode)
      logf(LOG_DEBUG, "No hits in last search, so bailing out");
    return(ResultC);
  }
  
  ResultD=NumericSearch(NorthLatitude, SouthFieldName, ZRelLE);
  logf(LOG_DEBUG, "Got %i entries <= %f in %s", ResultD->GetTotalEntries(),
	 (double)NorthLatitude, SouthFieldName.c_str());
  
  if (ResultD->GetTotalEntries() == 0) {
    // no hits rules out entire search
    delete ResultA;
    delete ResultC;
    if (DebugMode) logf(LOG_DEBUG, "No hits in last search, so bailing out");
    return(ResultD);
  }
  ResultC->And(*ResultD);
  if (DebugMode) logf(LOG_DEBUG, "Now have %i entries in range.", ResultC->GetTotalEntries());
  delete ResultD;
  
  if (ResultC->GetTotalEntries() == 0) {
    // no hits rules out entire search
    if (DebugMode) logf(LOG_DEBUG, "No hits in last search, so bailing out");
    return(ResultC);
  }
  
  // ResultA and ResultC contain our Lat/Long intrsections.  AND them
  ResultA->And(*ResultC);
  delete ResultC;
  if (DebugMode) logf(LOG_DEBUG, "Now have %i entries in range, heading back",
	 ResultA->GetTotalEntries());
  return(ResultA);		// full 'o hits (maybe)
  
}

// Dump the dates in an index
size_t INDEX::GPolyScan(STRLIST *Strlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn;
  GPOLYLIST    List;
  size_t       total = 0;

  if (!FieldType.IsGPoly())
    return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.LoadTable(Fn);

  size_t  count = List.GetCount();
  GPOLYFLD old_box;

  for (size_t i=0; i<count; i++)
   {
     GPOLYFLD box = List[i];
     if (box != old_box)
       {
	  STRING value (box);
	  if (!value.IsEmpty())
	    {
	      Strlist->AddEntry((STRING)box);
	      if (TotalRequested == ++total)
		break;

	    }
	  old_box = box;
       }
   }
  return total;
}


// Dump the dates in an index
size_t INDEX::GPolyScan(SCANLIST *Scanlist, const STRING& FieldName, size_t Start, INT TotalRequested)
{
  FIELDTYPE    FieldType = Parent->GetFieldType(FieldName);
  STRING       Fn;
  GPOLYLIST    List;
  size_t       total = 0;

  if (!FieldType.IsGPoly())
    return 0;

  Parent->DfdtGetFileName(FieldName, FieldType, &Fn);
  if (!FileExists(Fn))
    {
      Parent->SetErrorCode(123); // "Unsupported attribute combination";
      return 0;
    }

  List.LoadTable(Fn);

  size_t  count = List.GetCount();
  GPOLYFLD old_box = List[0l];
  size_t   freq    = 1;

  for (size_t i=1; i<count; i++)
   {
     GPOLYFLD box = List[i];
     if (box != old_box)
       {   
          STRING value (old_box);
          if (!value.IsEmpty())
            {
              Scanlist->AddEntry((STRING)old_box, freq);
              if (TotalRequested == ++total)
                break;

            }
          old_box = box;
       }
     else freq++;
   }
  if (freq > 1 && (total < TotalRequested || TotalRequested < 0))
    {
      Scanlist->AddEntry((STRING)old_box, freq);
      total++;
    }
  return total;
}

