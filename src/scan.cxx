#pragma ident  "@(#)scan.cxx"
/************************************************************************
************************************************************************/

/*-@@@
File:		scan.cxx
Version:	1.00
Description:	Scan
Author:		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
//#include <sys/file.h>
#include <sys/stat.h>
#include "common.hxx"
#include "magic.hxx"
#include "index.hxx"
#include "soundex.hxx"
#include "metaphone.hxx"
#include "mmap.hxx"

#define _USE_STOPWORDS 0

void SCANOBJ::Dump(ostream& os) const
{
  os << "(\"" << String << "\", " <<  Count << ")";
}
void SCANOBJ::Write(FILE *fp) const
{
  if (Count > 0)
    {
      String.Write(fp);
      ::Write(Count, fp);
    }
}
GDT_BOOLEAN SCANOBJ::Read(FILE *fp)
{
  if (String.Read(fp))
    {
      ::Read(&Count, fp);
      if (Count > 0) return GDT_TRUE;
    }
  return GDT_FALSE;
}


void atomicSCANLIST::Dump(ostream& os) const
{
  int count = 0;
  os << "{";
  for (register const atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      if (count++) os << ", ";
      p->Scan.Dump(os);
    }
  os << "} " << endl;
}

void atomicSCANLIST::Write(FILE *fp) const
{
  UINT4 count = 0;
  putObjID(objSCANLIST, fp);
  off_t  count_pos = ftell(fp);
  ::Write(count, fp); // Write Zero
  for (register const atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      p->Scan.Write(fp);
      count++;
    }
  // Go back to the position reserved
  if (count && -1 != fseek(fp, count_pos, SEEK_SET))
    {
      ::Write(count, fp);
      fseek(fp, 0L, SEEK_END); // Go to end
    }
}

GDT_BOOLEAN atomicSCANLIST::Read(FILE *fp)
{
  obj_t obj = getObjID(fp); // It it really an Atomic Scanlist?
  if (obj != objSCANLIST)
    {
      PushBackObjID(obj, fp);
      return GDT_FALSE;
    }
  UINT4 count, rcount=0;

  ::Read(&count, fp);
  // We have a list
  SCANOBJ Scan;
  while ((rcount < count || count == 0) && Scan.Read(fp))
    {
      FastAddEntry(Scan);
      rcount++;
    }
  if ((rcount > 0 && count == 0) || rcount == count)
    return GDT_TRUE; // Everthing went OK
  logf (LOG_ERROR, "Scanlist read failure. Expected %lu elements, got %lu",
	(unsigned long) count, (unsigned long)rcount);
  return GDT_FALSE;
}


void atomicSCANLIST::Save (const STRING& FileName) const
{
  if (IsEmpty())
    {
      FileName.Unlink();
      return;
    }
  PFILE fp = FileName.Fopen ("wb");
  if (fp != NULL)
    {
      Write(fp);
      fclose(fp);
    }
}

void atomicSCANLIST::Load (const STRING& FileName)
{
  FILE  *fp;

  Clear();
  if ((fp = FileName.Fopen ("rb")) != NULL)
    {
      Read(fp);
      fclose(fp);
    }
}




atomicSCANLIST::atomicSCANLIST() : VLIST()
{
}

atomicSCANLIST *atomicSCANLIST::Find (const STRING &Term) const
{
  for (register const atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      if (p->Scan.String == Term)
	{
	  return (atomicSCANLIST *)p;
	}
    }
  return NULL;
}

void atomicSCANLIST::Add (const atomicSCANLIST *OtherList)
{
  if (OtherList == NULL || OtherList->IsEmpty())
    return;
  for (const atomicSCANLIST *p=OtherList->Next(); p!=OtherList ; p=p->Next())
    AddEntry (*p);
//UniqueSort ();
}

void atomicSCANLIST::FastAddEntry(const atomicSCANLIST& Other)
{
  if (Other.Scan.Count == 0) return; // XXXX

  if (Other.Scan.String != Scan.String)
    {
      atomicSCANLIST *NodePtr = new atomicSCANLIST();
      if (NodePtr)
	{
	  NodePtr->Scan = Other.Scan;
	  VLIST::AddNode(NodePtr);
	}
    }
  else
    Scan.Count++;
}

void atomicSCANLIST::FastAddEntry(const SCANOBJ& Other) 
{
  if (Other.Count == 0) return; /// XXXXX

  if (Other.String != Scan.String)
    {
      atomicSCANLIST *NodePtr = new atomicSCANLIST();
      if (NodePtr)
	{
	  NodePtr->Scan = Other; 
	  VLIST::AddNode(NodePtr);
	}
    }
  else
   Scan.Count++;
}

void atomicSCANLIST::FastAddEntry(const STRING& StringEntry, size_t Frequency)
{
//  if (StringEntry.GetLength() && Frequency > 0)
    FastAddEntry( SCANOBJ(StringEntry, Frequency) );
}


atomicSCANLIST *atomicSCANLIST::AddEntry(const atomicSCANLIST& Other)
{
  // Insert in alpha order!
  for (register atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      INT diff = p->Scan.String.Compare(Other.Scan.String);
      if (diff == 0)
        {
	  p->Scan.Count += Other.Scan.Count;
	  return this;
        }
      else if (diff > 0)
	{
	  // Insert Node
	  p->FastAddEntry(Other);
	  return this;
	}
    }
  // Not added yet? Stick on the end..
  FastAddEntry(Other);
  return this;
}


atomicSCANLIST *atomicSCANLIST::AddEntry(const STRING& StringEntry, size_t Frequency)
{
#if 1
  // Insert in alpha order!
  for (register atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      INT diff = p->Scan.String.Compare(StringEntry);
      if (diff == 0)
        {
          p->Scan.Count += Frequency;
          return this;
        }
      else if (diff > 0)
        {
          // Insert Node
          p->FastAddEntry(StringEntry, Frequency);
          return this;
        }
    }
  // Not added yet? Stick on the end..
  FastAddEntry(StringEntry, Frequency);
  return this;
#else
  atomicSCANLIST* NodePtr = Find(StringEntry);

  if (NodePtr)
    NodePtr->Scan.Freq += Frequency;
  else
    FastAddEntry(StringEntry, Frequency);
  return this;
#endif
}

atomicSCANLIST *atomicSCANLIST::AddEntry(const STRING& Words, const LISTOBJ *StopWords)
{
  STRLIST p;
  return AddEntry(p.SplitWords(Words, StopWords));
}


/*
  Add the contents of the string list to the ScanList keeping track
  of word counts
*/
atomicSCANLIST *atomicSCANLIST::AddEntry(const STRLIST& StrList)
{
  STRLIST newStrList (StrList);

  newStrList.Sort();

  register atomicSCANLIST *p = Next();
  register int             diff;
  register const STRLIST  *ptr = newStrList.Next();
  STRING                   term (ptr->Value());

  while ((ptr != &newStrList) && (p != this))
    {
      // We only wants terms longer than single characters!
      if (term.GetLength() <= 1)
	{
	  term = (ptr = ptr->Next())->Value(); // Point to next in list
	}
      else if ((diff = p->Scan.String.Compare( term )) > 0)
	{
	  // NEED TO INSERT IT BEFORE! 
	  int count = 1;
	  STRING old_term = term;
	  while (((ptr = ptr->Next()) != &newStrList)  && ((diff = (term =  ptr->Value()).Compare(old_term)) == 0))
	    count++;
          p->FastAddEntry(old_term, count);
	}
      else if (diff == 0)
	{
	  /* word matches position */
          p->Scan.Count++;
	  term = (ptr = ptr->Next())->Value();
	}	
      else if (diff < 0)
	{
	  p = p->Next();
	}
    }
  /* Add the rest */
  int count = 0; // Start off with 0
  while (ptr != &newStrList)
    {
      STRING this_term;

      if ((diff = (this_term = ptr->Value()).Compare(term)) != 0)
	{
	  if (count) FastAddEntry(term, count);
	  term = this_term;
	  count = 1;
	}
      else
	count++;
      ptr = ptr->Next();
    }
  if (count > 1)
    FastAddEntry(term, count);

  return this;
}


atomicSCANLIST *atomicSCANLIST::MergeEntry(const STRING& StringEntry, size_t Frequency)
{
  // Insert in alpha order!
  if (Frequency == 0)
    return this; // Nothing to do

  for (register atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
    {
      INT diff = p->Scan.String.Compare(StringEntry);
      if (diff == 0)
        {
          if (Frequency > p->Scan.Count)
            p->Scan.Count = Frequency;
          return this;
        }
      else if (diff > 0)
        {
          // Insert Node
          p->FastAddEntry(StringEntry, Frequency);
          return this;
        }
    }
  // Not added yet? Stick on the end..
  FastAddEntry(StringEntry, Frequency);
  return this;
}


atomicSCANLIST *atomicSCANLIST::Entry (const size_t Index) const
{
  return (atomicSCANLIST *) (VLIST::GetNodePtr (Index));
}


GDT_BOOLEAN atomicSCANLIST::GetEntry (const size_t Index, STRING *StringEntry, size_t *freq) const
{
  atomicSCANLIST *NodePtr = (atomicSCANLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    {
      *StringEntry = NodePtr->Scan.String;
      *freq = NodePtr->Scan.Count;
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

SCANOBJ atomicSCANLIST::GetEntry (const size_t Index) const
{
  atomicSCANLIST *NodePtr = (atomicSCANLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    return NodePtr->Scan;
  return SCANOBJ();
}


typedef struct {
  STRING String;
  size_t Freq;
} table_t;

static int ScanListCompare (const void *x, const void *y)
{
  const table_t *ptr1 = (table_t *)x;
  const table_t *ptr2 = (table_t *)y;
  return (ptr1->String).Compare ( ptr2->String );
}

size_t atomicSCANLIST::UniqueSort ()
{
  return UniqueSort(0, 0, GDT_TRUE);
}


size_t atomicSCANLIST::UniqueSort (const size_t from, const size_t max, GDT_BOOLEAN Add)
{
  const size_t Total = GetTotalEntries();
  size_t newTotal = Total;

  // Do we have anything to do?
  if (Total > 1)
    {
      // Build a linear copy of current list
      table_t *TablePtr = new table_t[Total];
      size_t i = 0;
      // Flatten into a linear list
      for (register atomicSCANLIST *p = Next() ; p != this; p = p->Next() )
        {
	  if (p->Scan.Count>0 &&  p->Scan.String.GetLength())
	    {
	      TablePtr[i].Freq = p->Scan.Count;
	      TablePtr[i++].String = p->Scan.String;
	    }
        }
      Clear(); // Don't need the old list anymore

      // Sort
      QSORT (TablePtr, i /* Total */, sizeof (table_t), ScanListCompare);

      newTotal = 0; // Reset count
      // Put unique strings into list
      int diff = 0;
      size_t incr = 0;
      for (i= (from > 0 ? from-1 : 0); i<Total; i++)
        {
	  if (i)
	    {
	      if ((diff = TablePtr[i].String.Compare(TablePtr[i-1].String)) == 0)
		if (Add) incr += TablePtr[i].Freq;
	    }
	  if (diff)
	    {
	      FastAddEntry (TablePtr[i].String, TablePtr[i].Freq+incr);
	      if (++newTotal > max && max > 0)
		break;
	      incr = 0;
	    }
        }
      delete[] TablePtr; // Clean-up
    }
  return newTotal;
}

size_t atomicSCANLIST:: Search (const STRING& SearchTerm, size_t *freq) const
{
  size_t x = 0;
  for (register const atomicSCANLIST *p = Next(); p != this; p = p->Next() ) 
    {
      x++;
      if (p->Scan.String == SearchTerm)
	{
	  if (freq)
	    *freq = p->Scan.Count;
	  return x;
	}
    }
  return 0;
}

size_t atomicSCANLIST:: GetFrequency (const STRING& SearchTerm) const
{
  for (register const atomicSCANLIST *p = Next();  p != this; p = p->Next() ) 
    {
      if (p->Scan.String == SearchTerm)
	return p->Scan.Count;
    }
  return 0;
}

size_t atomicSCANLIST:: GetFrequency (const size_t Index) const
{
  const atomicSCANLIST *NodePtr = (const atomicSCANLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    return NodePtr->Scan.Count;
  return 0;
}

atomicSCANLIST::~atomicSCANLIST ()
{
}


/*
Scan APDUs
  ScanRequest ::= SEQUENCE{
   referenceId                    ReferenceId OPTIONAL,
     databaseNames            [3]   IMPLICIT SEQUENCE OF DatabaseName,
     attributeSet                  AttributeSetId OPTIONAL,
     termListAndStartPoint            AttributesPlusTerm,
     stepSize                [5]    IMPLICIT INTEGER OPTIONAL,
     numberOfTermsRequested            [6]    IMPLICIT INTEGER,
     preferredPositionInResponse        [7]    IMPLICIT INTEGER OPTIONAL,
     otherInfo                  OtherInformation OPTIONAL}

  ScanResponse ::= SEQUENCE{
    referenceId                    ReferenceId OPTIONAL,
    stepSize                [3]   IMPLICIT INTEGER OPTIONAL,
    scanStatus                [4]   IMPLICIT INTEGER {
                                      success    (0),
                                      partial-1    (1),
                                      partial-2    (2),
                                      partial-3    (3),
                                      partial-4    (4),
                                      partial-5    (5),
                                      failure   (6) },
    numberOfEntriesReturned          [5]   IMPLICIT INTEGER,
    positionOfTerm             [6]   IMPLICIT INTEGER OPTIONAL,
    entries                [7]   IMPLICIT ListEntries  OPTIONAL,  
    attributeSet               [8]   IMPLICIT AttributeSetId OPTIONAL,
    otherInfo                  OtherInformation OPTIONAL}

*/

// inline
static inline int gpcomp(const void* x, const void* y)
{
  return(*((GPTYPE *)x)-*((GPTYPE *)y));
}

#if _USE_STOPWORDS
// Private Version to exploit that its a sorted list and we have a
// sorted list (and that we are doing this to LARGE numbers of words)
//
static GDT_BOOLEAN __IsStopWord(PUCHR term, STOPLIST *StopWords, size_t *position)
{
  const size_t  TotalEntries = StopWords ? StopWords->GetTotalEntries() : 0;
  size_t        pos          = position ? *position : 0;
  
  if (pos <= TotalEntries && TotalEntries > 0)
    {
      PUCHR stopword;
      int   diff;

      if (pos <=0) pos = 1;

      stopword = StopWords->Nth(pos);
      while (stopword && (diff = StrCaseCmp(term, stopword)) < 0 && pos <= TotalEntries)
	stopword = StopWords->Nth(++pos);
      if (position) *position = pos;

      if (diff == 0) return GDT_TRUE;
    }
  return GDT_FALSE;
}
#endif


// TODO: Instead of -4 I think we need -sizeof(GPTYPE)
#define GP_OFFSET -(int)sizeof(GPTYPE) /* was -4 */

// VERY Private method
size_t INDEX::_scanAddEntry(SCANLIST *ListPtr,
  BUFFER *GpBuffer, size_t compLength, GDT_BOOLEAN have_field,
  FILE *fpi, const char *tp, const char *Buffer, const size_t Skip, GDT_BOOLEAN Merge) const
{
  if (Buffer == NULL)
    {
      logf (LOG_PANIC, "INDEX::_scanAddEntry() passed Nil Buffer");
      return 0;
    }

  size_t          count = 0;
  const GPTYPE    end = GP(tp, compLength + 1);
  const GPTYPE    start = ((long) tp - (long) Buffer) ?  GP(tp, GP_OFFSET) + 1 : 0;
  int             nhits = end - start + 1;

  if (nhits <= 0)
    return count;		// Paranoia!

  // Length of term...
  const size_t    termLength = (unsigned char) tp[0];
  if (termLength == 1) return count;  // Don't want singletons!

  const GDT_BOOLEAN truncated = (termLength >= compLength);
  size_t          found = nhits;// Total found
  GPTYPE         *gplist = NULL;
  size_t          pos = 0;

  if (have_field || truncated) {
    // Validate term in field..
    gplist = (GPTYPE *)GpBuffer->Want(nhits, sizeof(GPTYPE));
    // Read the GPs..
    nhits = GpFread(gplist, nhits, start, fpi);	/* @@ */
    // Now sort..
    if (nhits > 1 && !truncated)
      QSORT (gplist, nhits, sizeof(GPTYPE), gpcomp);	// Speed up looking

    if (have_field) {
      found = 0;
      for (INT j = 0; j < nhits; j++) {
	if (FieldCache->ValidateInField(gplist[j]))
	  found++;		// Want to know how many hits..
	else
	  gplist[j] = (GPTYPE)( -1 );
      }				/* for */
      if (found == 0)
	return count;
    }
  }
  if (truncated) {
#define Siz(_x,_y) (size_t)((((_x)*8 + _y)*(_y))/(_y))
    UCHR            term[Siz(StringCompLength, 64)]; // Longest possible string?
    const size_t    maxlen = sizeof(term)/sizeof(UCHR) - 2;
    STRING          oldTerm;
    found = 0;

    for (INT j = 0; j < nhits; j++) {
      if (gplist[j] != ((GPTYPE)-1) && GetIndirectTerm(gplist[j], term, maxlen)) {

//cerr << "TERM=" << term << endl;

	size_t termLength = strlen((const char *)term);
	if (termLength == maxlen)
	  {
	    term[maxlen] = '*';
	    term[maxlen+1] = '\0';
	  }

	if (!oldTerm.Equals(term)) {
	  if (found) {
	    if (++pos > Skip)
	      {
#if _USE_STOPWORDS
		if (!IsStopWord( oldTerm)) // Added by edz May 2006.. Don't add if Stopword!
#endif
		  {
		    if (Merge) ListPtr->MergeEntry(oldTerm, found);
		    else ListPtr->AddEntry(oldTerm, found);
		    count++;
		  }
	      }
	  }
	  oldTerm = term;
	  found = 1;
	} else
	  found++;
      }
    }
    if (found) {
      if (++pos > Skip)
	{
#if _USE_STOPWORDS
	  if (!IsStopWord( oldTerm)) // Added edz May 2006
#endif
	    {
	      if (Merge) ListPtr->MergeEntry(oldTerm, found);
	      else ListPtr->AddEntry(oldTerm, found);
	      count++;
	    }
	}
    }
  } else if (++pos > Skip) {
    //
    // We have full word length
    //
    unsigned char   tmp[StringCompLength + 1];
    // Term Matches..
    memcpy(tmp, &tp[1], termLength);
    tmp[termLength] = '\0';
#if _USE_STOPWORDS
    if (!IsStopWord( tmp )) // added edz May 2006
#endif
      {
	if (Merge) ListPtr->MergeEntry(tmp, found);
	else ListPtr->AddEntry(tmp, found);
	count++;
      }
  }
  return count;
}

// Start with the first element that matches Term (right truncated)..
// if TotalTermsRequested == -1 then scan to end of match..

//
// TODO: Need to extend to scan only a specific .SIS etc.
//

size_t INDEX::Scan (SCANLIST *ListPtr, const STRING& Fieldname,
        const STRING& Expression, const INT TotalTermsRequested, GDT_BOOLEAN Cat,
	size_t Sis_id) const
{
//cerr << "@@@@ Scan(ListPtr, " << Fieldname << ", " << Expression << " ..)" << endl;

  Parent->SetErrorCode(0); // OK

  STRING Term  = Expression.Strip( STRING::both );

  if (TotalTermsRequested == 0)
    return 0;

#if 0
  /* Lets first check if its a  query */
  SQUERY query;
  query.SetQueryTerm(Expression);
  if (query.GetTotalTerms() > 1)
    {
      logf (LOG_DEBUG, "Expression '%s' = '%s', use ScanSearch", Expression.c_str(), query.GetRpnTerm().c_str());
      if (!Cat) ListPtr->Clear();
      return ScanSearch(ListPtr, query, Fieldname); 
    }
#endif

  size_t pos = Term.Search('|');
  if (pos) {
     char *ctxt; // strtok_r context
     char *token = Term. NewCString() ;
     char *term = strtok_r (token, "|", &ctxt);
     size_t total_count = 0;
     if (!Cat) ListPtr->Clear();
     do {
	total_count += Scan (ListPtr, Fieldname, term, TotalTermsRequested, GDT_TRUE, Sis_id);
     } while ((term = strtok_r (NULL, "|", &ctxt)) != NULL);
    delete[] token;
#if 0
    if (total_count)
      {
	if (TotalTermsRequested > 0 && (INT)total_count > TotalTermsRequested)
	  total_count = TotalTermsRequested;
	ListPtr->UniqueSort(0, total_count, GDT_FALSE); // Sort the list
      }
#endif
    return total_count;
  }


#if 1
  /* Added August 2005 */
  if (Term.IsEmpty()) return 0; /* Don't scan for empty terms */
#endif

  if (Term.GetChr(1) == '*')
    {
      STRING NewTerm = Term(1);
      if ( (pos = NewTerm.IsWild()) >= NewTerm.GetLength() && NewTerm.GetChr(pos) == '*')
	{
	  NewTerm.EraseAfter(pos-1);
	  return ScanLR(ListPtr, Fieldname, NewTerm, 0, TotalTermsRequested, Cat);
	}
      return ScanGlob(ListPtr, Fieldname, Term, 0, TotalTermsRequested, Cat);
    }
  else
    {
      if ((pos = Term.IsWild()) == Term.GetLength() && Term.GetChr(pos) == '*')
	{
	  STRING NewTerm;
	  NewTerm = Term;
	  NewTerm.EraseAfter(pos-1);
	  return Scan(ListPtr, Fieldname, NewTerm, TotalTermsRequested, Cat);
	}
      else if (pos != 0)
	return ScanGlob(ListPtr, Fieldname, Term, 0, TotalTermsRequested, Cat);

      /* Added August 2005 **** EXPERIMENTAL **** */
     if ((pos = Term.GetLength()) > 1 && Term.GetChr(pos) == '~')
	{
	  Term.EraseAfter(pos-1);
	  if (useSoundex)
	    return ScanSoundex (ListPtr, Fieldname, Term, 6, 0, TotalTermsRequested, Cat);
	  return ScanMetaphone (ListPtr, Fieldname, Term, 0, TotalTermsRequested, Cat);
	}
    }

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryTerm = Term;

  QueryTerm.ToLower(); // Make lowercase
  CHR n[StringCompLength+3];
  n[0] = '\0'; // Truncated Search..
  n[1] = QueryTerm.GetLength();
  QueryTerm.GetCString(&n[2], StringCompLength);

  if (!Cat) ListPtr->Clear();
  // Loop through all sub-indexes

  BUFFER   GpBuffer;
  // GPTYPE  *gplist = NULL;

  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else have_field = GDT_FALSE;

  size_t total_count =0, count;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
	// Use the specified Sis_id
	if (jj==1) jj = Sis_id; else break; // Done
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }
    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    CHARSET sisCharset (off ? (BYTE)Map[1] : GetGlobalCharset()) ;

    const char *t = n[1] ? (const char *) bsearch(&n, Buffer, size/dsiz, dsiz, sisCharset.SisKeys()) : Buffer;
    if (t) {
        // We now have ANY Match...
	// Scan back to find the first match..
	do {
	  t -= dsiz;
	} while ( t >= Buffer && memcmp(&n[2], &t[1], (unsigned char)n[1]) == 0);
	t += dsiz; // Next should be it..
	// t is now pointing to the first match..
	for(char *tp = (char *)t; tp < &Buffer[size - 5]; tp += dsiz) {
          // DO we just want Matches?
          if (TotalTermsRequested == -1)
            {
              if (memcmp(&n[2], &tp[1], (unsigned char)n[1]))
		{
                  break;
		}
            }
	  count += _scanAddEntry(ListPtr, &GpBuffer, compLength, have_field, fpi, tp, Buffer, 0, Cat);
	  if (TotalTermsRequested != -1 && count >= (size_t)TotalTermsRequested)
	    break;
        }
    }
    total_count += count;
  } // for()

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
	count = TotalTermsRequested;
      if (!Cat)
	ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
  return total_count;
}


// Start with the first element that matches Term (inside)
// Left Truncated Search
size_t INDEX::ScanLR(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Term,
	const size_t Position, const INT TotalTermsRequested, GDT_BOOLEAN Cat,
	size_t Sis_id) const
{
//cerr << "@@@@/2  Scan(ListPtr, " << Fieldname << ", " << Term << " ..)" << endl;

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryTerm = Term;
  size_t QueryLength = QueryTerm.GetLength();
  size_t Skip = Position;

  QueryTerm.ToLower(); // Make lowercase
  CHR first_char = QueryTerm.GetChr(1);

  if (!Cat) ListPtr->Clear();

  if (QueryLength == 0 || TotalTermsRequested == 0)
    return 0;

  // Loop through all sub-indexes
  BUFFER   GpBuffer;
  //PGPTYPE  gplist = NULL;
  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0;
  // size_t pos = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
        // Use the specified Sis_id
        if (jj==1) jj = Sis_id; else break; // Done 
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }
    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    // Look for matches...
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz) {
      const size_t len = (unsigned char)tp[0];
      if (len >= QueryLength)
	{
	  // Look to see if the string is inside..
	  GDT_BOOLEAN OK = GDT_FALSE;
	  for (size_t i = 1; i <= (len - QueryLength + 1); i++)
	    {
	      if (tp[i] != first_char)
		continue;
	      if (memcmp(tp+i, QueryTerm.c_str(), QueryLength) == 0)
		{
		  OK = GDT_TRUE;
		  break;
		}
	    }
	  // Did we find a match??
	  if (OK == GDT_FALSE)
	    continue; // Nope

	  size_t n = _scanAddEntry(ListPtr, &GpBuffer, compLength, have_field, fpi, tp, Buffer, Skip, Cat);
	  if (n >= Skip)
	    Skip = 0;
	  else
	    Skip -= n;
	  count += n;
	  if (TotalTermsRequested != -1 && count >= (size_t)TotalTermsRequested)
	    break;
	}
    } /* for() */
    total_count += count;
  } // for()
  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
        total_count = TotalTermsRequested;
      ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
  return total_count;
}

// Start with the first element that matches Term (inside)
size_t INDEX::ScanGlob(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const size_t Position, const INT TotalTermsRequested, GDT_BOOLEAN Cat,
	size_t Sis_id) const
{
//cerr << "@@@@/3 Scan(ListPtr, " << Fieldname << ", " << Pattern << " ..)" << endl ;

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryPattern = Pattern;

  QueryPattern.ToLower(); // Make lowercase

  if (!Cat) ListPtr->Clear();

  if (QueryPattern.GetLength() == 0 || TotalTermsRequested == 0)
    return 0;

  // Loop through all sub-indexes
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;

  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
        // Use the specified Sis_id
        if (jj==1) jj = Sis_id; else break; // Done 
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    char tmp[StringCompLength+1];
    int nhits;
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz)
      {
	// Term Matches..
        memcpy(tmp, &tp[1], (unsigned char)tp[0]);
        tmp[(unsigned char)tp[0]] = '\0';

#if _USE_STOPWORDS
	if (IsStopWord(tmp)) // Added edz May 2006. Don't add stopwords
	  continue; 
#endif

	if (! QueryPattern.MatchWild(tmp) )
	  continue; // No match

	const GPTYPE end = GP(tp, compLength+1);
	const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

	nhits = end - start + 1;
	if (nhits <= 0)
	  continue; // Paranoia!

	if (have_field)
	  {
	    // Validate term in field.. (TO DO)
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            nhits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (nhits > 1)
              QSORT (gplist, nhits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    int found = 0;
	    for (INT j=0; j<nhits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
		found++;
	      }
	    } /* for */
	    nhits = found;
	  }

	if (nhits <= 0)
	  continue;
	if (++pos >= Position)
	  {
	    ListPtr->AddEntry(tmp, nhits);
	    count++;
	    if (TotalTermsRequested>0 && count >= (size_t)TotalTermsRequested)
	      break;
	  }
      }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
        total_count = TotalTermsRequested;
      ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
  return total_count;
}

// Start with the first element that matches Term (inside)
size_t INDEX::ScanSoundex(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
        const size_t HashLen, const size_t Position, const INT TotalTermsRequested,
	GDT_BOOLEAN Cat, size_t Sis_id) const
{
  if (!Cat) ListPtr->Clear();

  if (Pattern.IsEmpty() || TotalTermsRequested == 0)
    return 0;

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING Hash;

  ::SoundexEncode(Pattern, &Hash, HashLen);

  // Loop through all sub-indexes
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;

  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
        // Use the specified Sis_id
        if (jj==1) jj = Sis_id; else break; // Done 
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    char tmp[StringCompLength+1];
    int nhits;
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz)
      {
	// Term Matches..
        memcpy(tmp, &tp[1], (unsigned char)tp[0]);
        tmp[(unsigned char)tp[0]] = '\0';

#if _USE_STOPWORDS
	if (IsStopWord(tmp)) // Added edz May 2006. Don't add stopwords
	  continue;
#endif

	if (! SoundexMatch(tmp, Hash) )
	  continue; // No match

	const GPTYPE end = GP(tp, compLength+1);
	const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

	nhits = end - start + 1;
	if (nhits <= 0)
	  continue; // Paranoia!

	if (have_field)
	  {
	    // Validate term in field.. (TO DO)
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            nhits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (nhits > 1)
              QSORT (gplist, nhits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    int found = 0;
	    for (INT j=0; j<nhits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
		found++;
	      }
	    } /* for */
	    nhits = found;
	  }

	if (nhits <= 0)
	  continue;
	if (++pos >= Position)
	  {
	    ListPtr->AddEntry(tmp, nhits);
	    count++;
	    if (TotalTermsRequested>0 && count >= (size_t)TotalTermsRequested)
	      break;
	  }
      }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
        total_count = TotalTermsRequested;
      ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
  return total_count;
}


size_t INDEX::Scan (SCANLIST *ListPtr, const STRING& Fieldname,
        const size_t Position, const INT TotalTermsRequested,
	const size_t Start, GDT_BOOLEAN Cat, size_t Sis_id) const
{
//cerr << "@@@@ Scan <-- " << Fieldname << " Pos=" << Position << endl;
  if (!Cat) ListPtr->Clear();
  if (TotalTermsRequested <= 0)
    return 0; // Nothing to do

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;

  // Loop through all sub-indexes

  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  size_t Offset = Start;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
        // Use the specified Sis_id
        if (jj==1) jj = Sis_id; else break; // Done 
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)

    const char *t = NULL;
    if ((Offset*dsiz) < (size - off))
      {
        t =  &Buffer[Offset*dsiz];
	Offset = 0;
      }
    else
      {
	Offset -= (size - off)/dsiz;
      }

    if (t) {
	// t is now pointing to the position
	char tmp[StringCompLength+1];
	int nhits;
	for(char *tp = (char *)t; tp < &Buffer[size - 5]; tp += dsiz) {

	  const GPTYPE end = GP(tp, compLength+1);
	  const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

          nhits = end - start + 1;
	  if (nhits <= 0)
	    continue; // Paranoia

	  if (nhits >0 && have_field) {
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            nhits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (nhits > 1)
              QSORT (gplist, nhits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    int found = 0;
	    for (INT j=0; j<nhits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
		found++;
	      }
	    } /* for */
	    nhits = found;
	  }
	  // Term Matches..
	  if (tp[0] == 1)
	    continue; // Don't want singletons

	  if (nhits && ++pos > Position)
	    {
	      size_t termLength = (unsigned char)tp[0];
	      memcpy(tmp, &tp[1], termLength);
	      if (termLength >= compLength)
		tmp[termLength++] = '*';
	      tmp[termLength] = '\0';
#if _USE_STOPWORDS
	      if (!IsStopWord(tmp)) // Added by edz May 2006.. Don't add if Stopword!
#endif
		{
		  ListPtr->AddEntry(tmp, nhits);
		  count++; // Added Nov 2005
		  //cerr << "Adding " << tmp << " with hits = " << nhits << endl;
		}
	    }
        } // for()
    }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
        total_count = TotalTermsRequested;
      ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
//cerr << "Total = " << total_count << endl;
  return total_count;
}

///************************************************************************************************


#if 0
size_t INDEX:TermFrequency(const STRING& Term, const STRING& Fieldname)
{
  return Scan(NULL, Fieldname, Term, -1);
}
#endif


// Start with the first element that matches Term (right truncated)..
// if TotalTermsRequested == -1 then scan to end of match..

size_t INDEX::Scan (PSTRLIST ListPtr, const STRING& Fieldname,
        const STRING& Term, const INT TotalTermsRequested) const
{
  if (TotalTermsRequested == 0)
    return 0;

  if (Term.GetChr(1) == '*')
    {
      STRING NewTerm = Term(1);
      size_t pos = NewTerm.IsWild(); 
      if (pos >= NewTerm.GetLength() && NewTerm.GetChr(pos) == '*' && NewTerm.Search('|') == 0)
	{
	  NewTerm.EraseAfter(pos-1);
	  return ScanLR(ListPtr, Fieldname, NewTerm, 0, TotalTermsRequested);
	}
      return ScanGlob(ListPtr, Fieldname, Term, 0, TotalTermsRequested);
    }
  else
    {
      size_t pos = Term.IsWild();
      if (pos == Term.GetLength() && Term.GetChr(pos) == '*')
	{
	  STRING NewTerm;
	  NewTerm = Term;
	  NewTerm.EraseAfter(pos-1);
	  return Scan(ListPtr, Fieldname, NewTerm, TotalTermsRequested);
	}
      else if (pos != 0 || Term.Search('|'))
	return ScanGlob(ListPtr, Fieldname, Term, 0, TotalTermsRequested);
    }

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryTerm = Term;

  QueryTerm.ToLower(); // Make lowercase
  CHR n[StringCompLength+3];
  n[0] = '\0'; // Truncated Search..
  n[1] = QueryTerm.GetLength();
  QueryTerm.GetCString(&n[2], StringCompLength);

  if (ListPtr) ListPtr->Clear();
  // Loop through all sub-indexes

  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else have_field = GDT_FALSE;

  size_t count, total_count = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    CHARSET sisCharset (off ? (BYTE)Map[1] : GetGlobalCharset()) ;
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    const char *t = n[1] ? (const char *) bsearch(&n, Buffer, size/dsiz, dsiz, sisCharset.SisKeys()) : Buffer;
    if (t) {
        // We now have ANY Match...
	// Scan back to find the first match..
	do {
	  t -= dsiz;
	} while ( t >= Buffer && memcmp(&n[2], &t[1], (unsigned char)n[1]) == 0);
	t += dsiz; // Next should be it..
	// t is now pointing to the first match..
	char tmp[StringCompLength+1];
	for(char *tp = (char *)t; tp < &Buffer[size - 5]; tp += dsiz) {

          // DO we just want Matches?
          if (TotalTermsRequested == -1)
            {
              if (memcmp(&n[2], &tp[1], (unsigned char)n[1]))
                break;
            }

	  GDT_BOOLEAN OK = GDT_TRUE;
	  if (have_field) {
	    OK = GDT_FALSE;
	    // Validate term in field.. (TO DO)
	    const GPTYPE end = GP(tp, compLength+1);
	    const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1LL : 0;

            int nhits = end - start + 1;
	    if (nhits <= 0)
	      continue; // Paranoia!
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            int num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (num_hits > 1)
              QSORT (gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    for (INT j=0; j<num_hits; j++) {
	     if (FieldCache->ValidateInField(gplist[j])) {
	        OK = GDT_TRUE;
                break; // in field..
	      }
	    } /* for */
	  }
	  if (OK == GDT_FALSE)
	    {
	      continue;
	    }
	  if (tp[0] == 1)
	    continue; // Don't want singletons

	  // Term Matches..
	  size_t termLength = (unsigned char)tp[0];
	  memcpy(tmp, &tp[1], termLength);
	  if (termLength >= compLength)
	    tmp[termLength++] = '*';
	  tmp[termLength] = '\0';
#if _USE_STOPWORDS
	  if (!IsStopWord(tmp)) // added edz May 2006
#endif
	    {
	      if (ListPtr) ListPtr->AddEntry(tmp);
	      count++;
	      if (TotalTermsRequested > 0 && count >= (size_t)TotalTermsRequested)
		break;
	    }
        }
    }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (ListPtr) ListPtr->UniqueSort(); // Sort the list
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
	{
	  if (ListPtr) ListPtr->EraseAfter(TotalTermsRequested);
	  return TotalTermsRequested;
	}
    }
  return total_count;
}


// Start with the first element that matches Term (inside)
size_t INDEX::ScanLR(PSTRLIST ListPtr, const STRING& Fieldname, const STRING& Term,
	const size_t Position, const INT TotalTermsRequested) const
{
  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryTerm = Term;
  size_t QueryLength = QueryTerm.GetLength();

  QueryTerm.ToLower(); // Make lowercase
  CHR first_char = QueryTerm.GetChr(1);

  ListPtr->Clear();

  if (QueryLength == 0 || TotalTermsRequested == 0)
    return 0;

  // Loop through all sub-indexes
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    char tmp[StringCompLength+1];
    // Look for matches...
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz) {
      const size_t len = (unsigned char)tp[0];
      if (len >= QueryLength)
	{
	  // Look to see if the string is inside..
	  GDT_BOOLEAN OK = GDT_FALSE;
	  for (size_t i = 1; i <= (len - QueryLength + 1); i++)
	    {
	      if (tp[i] != first_char)
		continue;
	      if (memcmp(tp+i, QueryTerm.c_str(), QueryLength) == 0)
		{
		  OK = GDT_TRUE;
		  break;
		}
	    }
	  // Did we find a match??
	  if (OK && have_field) {
	    OK = GDT_FALSE;
	    // Validate term in field.. (TO DO)
	    const GPTYPE end = GP(tp, compLength+1);
	    const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

            int nhits = end - start + 1;
	    if (nhits <= 0)
	      continue; // Paranoia!
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            int num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (num_hits > 1)
              QSORT (gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    for (INT j=0; j<num_hits; j++) {
	     if ((OK = FieldCache->ValidateInField(gplist[j])) == GDT_TRUE) {
                break; // in field..
	      }
	    } /* for */
	  }
	  if (OK == GDT_FALSE)
	    {
	      continue;
	    }
	  if (tp[0] == 1)
	    continue; // Don't want singletons

	  // Term Matches..
	  if (++pos > Position)
	    {
	      size_t termLength = (unsigned char)tp[0];
	      memcpy(tmp, &tp[1], termLength);
	      if (termLength >= compLength)
		tmp[termLength++] = '*';
	      tmp[termLength] = '\0';
#if _USE_STOPWORDS
	      if (!IsStopWord(tmp)) // added 2006 May edz
#endif
		{
		  ListPtr->AddEntry(tmp);
		  count++;
		  if (TotalTermsRequested > 0 && count >= (size_t)TotalTermsRequested)
		    break;
		}
	    }
	}
    } /* for() */
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      ListPtr->UniqueSort(); // Sort the list
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
	{
	  ListPtr->EraseAfter(TotalTermsRequested);
	  return TotalTermsRequested;
	}
    }
  return total_count;
}

// Start with the first element that matches Term (inside)
size_t INDEX::ScanGlob(PSTRLIST ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const size_t Position, const INT TotalTermsRequested) const
{
  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;
  STRING QueryPattern = Pattern;

  QueryPattern.ToLower(); // Make lowercase

  ListPtr->Clear();

  if (QueryPattern.GetLength() == 0 || TotalTermsRequested == 0)
    return 0;

  // Loop through all sub-indexes
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;

  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    char tmp[StringCompLength+1];
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz)
      {
	  // Term Matches..
          memcpy(tmp, &tp[1], (unsigned char)tp[0]);
          tmp[(unsigned char)tp[0]] = '\0';
	  GDT_BOOLEAN OK = QueryPattern.MatchWild(tmp);
	  if (OK && have_field) {
	    OK = GDT_FALSE;
	    // Validate term in field.. (TO DO)
	    const GPTYPE end = GP(tp, compLength+1);
	    const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

            int nhits = end - start + 1;
	    if (nhits <= 0)
	      continue; // Paranoia!
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            int num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (num_hits > 1)
              QSORT (gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    for (INT j=0; j<num_hits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
	        OK = GDT_TRUE;
                break; // in field..
	      }
	    } /* for */
	  }
	  if (OK == GDT_FALSE)
	    {
	      continue;
	    }
	  if (++pos >= Position)
	    {
	      ListPtr->AddEntry(tmp);
	      count++;
	      if (TotalTermsRequested > 0 && count >= (size_t)TotalTermsRequested)
		break;
	    }
      }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      ListPtr->UniqueSort(); // Sort the list
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
	{
	  ListPtr->EraseAfter(TotalTermsRequested);
	  return (size_t)TotalTermsRequested;
	}
    }
  return total_count;
}

size_t INDEX::Scan (PSTRLIST ListPtr, const STRING& Fieldname,
        const size_t Position, const INT TotalTermsRequested,
	const size_t Start) const
{
  ListPtr->Clear();
  if (TotalTermsRequested <= 0)
    return 0; // Nothing to do

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;

  // Loop through all sub-indexes

  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  size_t Offset = Start;
  MMAP MemoryMap;

  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle
      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)

    const char *t = NULL;

    if ((Offset*dsiz) < (size - off))
      {
        t =  &Buffer[Offset*dsiz];
	Offset = 0;
      }
    else
      {
	Offset -= (size - off)/dsiz;
      }
    if (t) {
	// t is now pointing to the position
	char tmp[StringCompLength+1];
	for(char *tp = (char *)t; tp < &Buffer[size - 5]; tp += dsiz) {

	  GDT_BOOLEAN OK = GDT_TRUE;
	  if (have_field) {
	    OK = GDT_FALSE;
	    // Validate term in field.. (TO DO)
	    const GPTYPE end = GP(tp, compLength+1);
	    const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

            int nhits = end - start + 1;
	    if (nhits <= 0)
	      continue; // Paranoia!
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            int num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (num_hits > 1)
              QSORT (gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    for (INT j=0; j<num_hits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
	        OK = GDT_TRUE;
                break; // in field..
	      }
	    } /* for */
	  }
	  if (OK == GDT_FALSE)
	    {
	      continue;
	    }
	  if (tp[0] == 1)
	    continue; // Don't want singletons

	  // Term Matches..
	  if (++pos > Position)
	    {
	      size_t termLength = (unsigned char)tp[0];
	      memcpy(tmp, &tp[1], termLength);
	      if (termLength >= compLength)
		tmp[termLength++] = '*';
	      tmp[termLength] = '\0';
#if _USE_STOPWORDS
	      if (!IsStopWord(tmp)) // Added 2006 edz
#endif
		{
		  ListPtr->AddEntry(tmp);
		  count++;
		}
	    }
        } // for()
    }
    total_count += count;
  } // for()

  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      ListPtr->UniqueSort(); // Sort the list
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
	{
	  ListPtr->EraseAfter(TotalTermsRequested);
	  return (size_t)TotalTermsRequested;
	}
    }
  return total_count;
}

// Start with the first element that matches Term (inside)
size_t INDEX::ScanMetaphone(SCANLIST *ListPtr, const STRING& Fieldname, const STRING& Pattern,
	const size_t Position, const INT TotalTermsRequested, GDT_BOOLEAN Cat, size_t Sis_id) const
{
  if (!Cat) ListPtr->Clear();

  if (Pattern.IsEmpty() || TotalTermsRequested == 0)
    return 0;

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;

  UINT8  Hash = DoubleMetaphone(Pattern);

  // Loop through all sub-indexes
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;

  GDT_BOOLEAN have_field;
  if (Fieldname.GetLength())
    {
      FieldCache->SetFieldName(Fieldname);
      have_field = GDT_TRUE;
    }
  else
    have_field = GDT_FALSE;

  size_t count, total_count = 0, pos = 0;
  MMAP MemoryMap;

  clock_t _start_clock = clock();

  Parent->SetErrorCode(0); // OK
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++) {

    if ( MaxQueryCPU_ticks != 0 && (clock() -  _start_clock) > MaxQueryCPU_ticks)
      {
	Parent->SetErrorCode(33); // Time exceeded: Valid subset of results available
	break;
      }

    count = 0;
    if (NumberOfIndexes == 0) {
      fpi = ffopen (IndexFileName, "rb");
      if (fpi == NULL) continue; // Next
      NumberOfIndexes = -1;
      MemoryMap.CreateMap(SisFileName, MapRandom);
    } else {
      if (fpi) ffclose(fpi); // Close old handle

      ////// Do we have a Sis_id?
      if (Sis_id > 0) {
        // Use the specified Sis_id
        if (jj==1) jj = Sis_id; else break; // Done 
       }
      ////// End Process Sis_id hook

      STRING s;
      s.form("%s.%d", IndexFileName.c_str(), jj);
      fpi = ffopen(s, "rb");
      if (fpi == NULL) continue; // Next
      s.form("%s.%d", SisFileName.c_str(), jj);
      MemoryMap.CreateMap(s, MapRandom);
    }

    if (!MemoryMap.Ok())
      continue;

    // Determine the size of the index
    // const INT  Off = GetFileSize(fpi) % sizeof(GPTYPE);

    const char *Map = (char *)MemoryMap.Ptr();
    const size_t size = MemoryMap.Size();
    const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2); // was sizeof(int)
    const char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(GPTYPE)+1; // was sizeof(int)
    if (off) SetGlobalCharset( (BYTE)Map[1] );

    char tmp[StringCompLength+1];
    char firstCh[3];

    // TODO:: Speed up by looking only at the first 2 characters first!

    int nhits;

    firstCh[2] = '\0';
    for(char *tp = (char *)Buffer; tp < &Buffer[size - 5]; tp += dsiz)
      {
	if (tp[0] < 3) continue; // Not interested in 1 or 2 letter words

	if (!_ib_isalpha(firstCh[0] = tp[1]) && firstCh[0] != Pattern[0])
	  continue;
	if (!_ib_isalpha(firstCh[1] = tp[2]) && firstCh[1] != Pattern[1])
	  continue;

	// Term Matches..
        memcpy(tmp, &tp[1], (unsigned char)tp[0]);
        tmp[(unsigned char)tp[0]] = '\0';

#if _USE_STOPWORDS
	if (IsStopWord(tmp)) // Added edz May 2006. Don't add stopwords
	  continue;
#endif

	if (! MetaphoneMatch(tmp, Hash) )
	  continue; // No match

#if 1
        cerr << "  = " << tmp << "  hash=" << hex << DoubleMetaphone(tmp) << endl;
	cerr << "  = " << Pattern << "  hash=" << hex << Hash << endl; 
#endif

	const GPTYPE end = GP(tp, compLength+1);
	const GPTYPE start = ((long)tp - (long)Buffer) ? GP(tp, GP_OFFSET) + 1 : 0;

	nhits = end - start + 1;
	if (nhits <= 0)
	  continue; // Paranoia!

	if (have_field)
	  {
	    // Validate term in field.. (TO DO)
            if ((size_t)nhits > gplist_siz) {
              // Reallocate space..
              if (gplist) delete[] gplist;
              gplist_siz += nhits + 100;
              gplist = new GPTYPE[gplist_siz];
            }
            // Read the GPs..
            nhits = GpFread(gplist, nhits, start, fpi); /* @@ */
            // Now sort..
            if (nhits > 1)
              QSORT (gplist, nhits, sizeof(GPTYPE), gpcomp); // Speed up looking
	    int found = 0;
	    for (INT j=0; j<nhits; j++) {
             if (FieldCache->ValidateInField(gplist[j])) {
		found++;
	      }
	    } /* for */
	    nhits = found;
	  }

	if (nhits <= 0)
	  continue;
	if (++pos >= Position)
	  {
	    ListPtr->AddEntry(tmp, nhits);
	    count++;
	    if (TotalTermsRequested>0 && count >= (size_t)TotalTermsRequested)
	      break;
	  }
      }
    total_count += count;
  } // for()


  if (gplist) delete[] gplist; // Clean-up

  if (fpi) ffclose(fpi);
  if (NumberOfIndexes > 1)
    {
      if (TotalTermsRequested > 0 && total_count >= (size_t)TotalTermsRequested)
        total_count = TotalTermsRequested;
      ListPtr->UniqueSort(0, total_count, GDT_TRUE); // Sort the list
    }
  return total_count;
}

