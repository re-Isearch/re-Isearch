/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)strlist.cxx"

/*-@@@
File:		strlist.cxx
Description:	Class STRLIST - String List
@@@-*/

#include "common.hxx"
#include "strlist.hxx"
#include "magic.hxx"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#if  LISTOBJ_NO_INLINED
// LISTOBJ 
LISTOBJ::LISTOBJ () { message_log(LOG_DEBUG, "LISTOBJ Create");}
LISTOBJ::LISTOBJ (const STRING&) {;}
LISTOBJ::~LISTOBJ() { message_log(LOG_DEBUG, "LISTOBJ Destroy");}
size_t      LISTOBJ::GetTotalEntries () const { return 0; }
bool LISTOBJ::Load(const STRING& arg) {
  message_log(LOG_DEBUG, "LISTOBJ::Load(%s)", arg.c_str());
  return false;
}
bool LISTOBJ::InList(const STRING& Word) const {
  return InList((const UCHR*)(Word.c_str()), Word.GetLength());
}
bool LISTOBJ::InList (const UCHR*, const STRINGINDEX) const {
  return false;
}
#endif


const STRLIST NulStrlist;

STRLIST::STRLIST() : VLIST()
{
}

STRLIST::STRLIST(const STRING& SIS) : VLIST()
{
  // See STRING() below
  Split ((CHR)'\000', SIS);
}

STRLIST::STRLIST(const STRING& String1, const STRING& String2) : VLIST()
{
  AddEntry(String1);
  AddEntry(String2);
}


STRLIST::STRLIST(const STRING& SIS, CHR Chr) : VLIST()
{
  Split (Chr, SIS);
}


STRLIST::STRLIST(const STRLIST& List) : VLIST()
{ 
  Cat(List);
}

STRLIST::STRLIST(const ArraySTRING& Array) : VLIST()
{
  Cat(Array);
}

STRLIST::STRLIST(const char * const * CStrList) : VLIST()
{
  Cat(CStrList);
}


STRLIST& STRLIST::Cat(const ArraySTRING& Array)
{
  const UINT total = Array.Count();
  for (UINT i=0; i < total; i++)
    AddEntry( Array[i] );
  return *this;
}


STRLIST& STRLIST::Cat(const STRLIST& OtherVlist)
{
  for (const STRLIST *p = OtherVlist.Next(); p != &OtherVlist; p = p->Next())
    AddEntry (p->String);
  return *this;
}

STRLIST& STRLIST::Cat(const char * const * CStrList)
{
  if (CStrList)
    {
      for (size_t i = 0; CStrList[i]; i++) 
	AddEntry (CStrList[i]);
    }
  return *this;
}

STRLIST& STRLIST::operator =(const STRLIST& OtherVlist)
{
  Clear ();
  return Cat(OtherVlist);
}

STRLIST& STRLIST::operator =(const ArraySTRING& Array)
{
  Clear ();
  return Cat(Array);
}


STRLIST& STRLIST::operator =(const char * const * CStrList)
{
  Clear ();
  return Cat (CStrList);
}

STRLIST& STRLIST::operator +=(const STRLIST& OtherVlist)
{
  return Cat(OtherVlist);
}

STRLIST& STRLIST::operator +=(const char * const * CStrList)
{
  return Cat(CStrList);
}


STRLIST& STRLIST::operator +(const STRLIST& OtherVlist)
{
  static STRLIST add;
  add  = *this;
  add += OtherVlist;
  return add;
}

STRLIST *STRLIST::AddEntry(const char *Entry)
{
  if (Entry && *Entry)
    {
      STRING  StringEntry (Entry);
      return AddEntry(StringEntry);
    }
  return this;
}

STRLIST *STRLIST::AddEntry(const STRING& StringEntry)
{
  // Don't add entry strings

  if (StringEntry.GetLength())
    {
      STRLIST* NodePtr;
      try {
        NodePtr = new STRLIST();
      } catch (...) {
        NodePtr = NULL;
      }
      if (NodePtr != NULL)
	{
	  NodePtr->String = StringEntry;
	  VLIST::AddNode(NodePtr);
	} 
      else
	message_log (LOG_PANIC, "Memory allocation failed in STRLIST::AddEntry(\"%s\")", StringEntry.c_str());
    }
  return this;
}

STRLIST *STRLIST::AddEntry(const STRLIST& Strlist)
{
  Cat (Strlist);
  return this;
}


static int StrListCompare (const void *x, const void *y)
{
  return ((STRING *) x)->Compare ( *(STRING *) y );
}

static int StrListCaseCompare (const void *x, const void *y)
{     
  return ((STRING *) x)->CaseCompare ( *(STRING *) y );
}


size_t STRLIST::UniqueSort ()
{
  return UniqueSort(0, 0);
}

size_t STRLIST::UniqueSort (const size_t from, const size_t max)
{
  const size_t Total = GetTotalEntries();
  size_t newTotal = Total;

  // Do we have anything to do?
  if (Total > 1)
    {
      // Build a linear copy of current list
      STRING *TablePtr;
      try {
        TablePtr = new STRING[Total];
      } catch (...) {
	message_log (LOG_PANIC|LOG_ERRNO,
		"Can't allocate %u string slots in STRLIST::UniqueSort().", (unsigned)Total);
        return Total;
      }

      size_t i = 0;
      // Flatten into a linear list
      for (register const STRLIST *p = Next(); p != this; p=p->Next())
        TablePtr[i++] = p->String;
      Clear(); // Don't need the old list anymore
      // Sort
      QSORT (TablePtr, Total, sizeof (STRING), StrListCompare);

      newTotal = 0; // Reset count
      // Put unique strings into list
      STRING s;
      for (i= (from > 0 ? from-1 : 0); i<Total; i++)
        {
	  if (i == 0 || (TablePtr[i].Compare(TablePtr[i-1])))
	    {
	      s =  TablePtr[i];
	      const STRINGINDEX Position =  s.Search ('=');

	      if (Position ==  s.GetLength())
		continue; // trailing =

	      AddEntry (s);
	      if (++newTotal > max)
		{
		  if (max>0)
		    break;
		}
	    }
        }
      delete[] TablePtr; // Clean-up
    }
  return newTotal;
}

bool STRLIST::DeleteEntry(const size_t pos)
{
  size_t i = 0;
  for (register STRLIST *p = Next(); p != this; p = p->Next() )
    {
      if (++i == pos)
	{
	  delete (STRLIST *)p->Disattach();
	  return true;
	}
    }
  return false;
}

 
size_t STRLIST::Sort ()
{
  const size_t Total = GetTotalEntries();  

  // Do we havfe anything to do?
  if (Total == 2)
    {
      if (Next()->String.Compare(Next()->Next()->String) > 0) // August 2005 changed < to > 
	Reverse();
    }
  else if (Total > 1)
    {
      // Build a linear copy of current list
      STRING           *TablePtr;
      try {
        TablePtr = new STRING[Total];
      } catch (...) {
        message_log (LOG_PANIC|LOG_ERRNO,
		"Can't allocate %u string slots in STRLIST::Sort().", (unsigned)Total);
        return Total;
      }
      register STRING  *ptr      = TablePtr;
      register STRLIST *p;

      // Flatten into a linear list
      for (p = Next();  p != this; p = p->Next())
	*ptr++ = p->String;

      // Sort
      QSORT (TablePtr, Total, sizeof (STRING), StrListCompare);

      // Put back into circular linked list
      ptr = TablePtr;
      for (p = Next(); p != this; p = p->Next())
	p->String = *ptr++;

      delete[] TablePtr; // Clean-up
    }
  return Total;
}


size_t STRLIST::CaseSort ()
{
  const size_t Total = GetTotalEntries();  

  // Do we havfe anything to do?
  if (Total == 2)
    {
      if (Next()->String.CaseCompare(Next()->Next()->String) > 0) // August 2005 changed < to > 
	Reverse();
    }
  else if (Total > 1)
    {
      // Build a linear copy of current list
      STRING           *TablePtr;
      try {
        TablePtr = new STRING[Total];
      } catch (...) {
        message_log (LOG_PANIC|LOG_ERRNO,
		"Can't allocate %u string slots in STRLIST::CaseSort().", (unsigned)Total);
        return Total;
      }
      register STRING  *ptr      = TablePtr;
      register STRLIST *p;

      // Flatten into a linear list
      for (p = Next();  p != this; p = p->Next())
	*ptr++ = p->String;

      // Sort
      QSORT (TablePtr, Total, sizeof (STRING), StrListCaseCompare);

      // Put back into circular linked list
      ptr = TablePtr;
      for (p = Next(); p != this; p = p->Next())
	p->String = *ptr++;

      delete[] TablePtr; // Clean-up
    }
  return Total;
}


STRLIST * STRLIST::SetEntry (const size_t Index, const STRING& StringEntry)
{
  STRLIST *NodePtr = (STRLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    {
      NodePtr->String = StringEntry;
      return NodePtr;
    }
  else if (Index > 0)
    {
      // Add filler nodes
      const int y = Index - GetTotalEntries ();
      for (int x = 1; x < y; x++)
	AddEntry(NulString);
      AddEntry (StringEntry);
      return this->Prev();
    }
  return NULL;
}

STRING STRLIST::GetEntry (const size_t Index) const
{
  STRLIST *NodePtr = (STRLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    return NodePtr->String;
  return NulString;
}

bool STRLIST::GetEntry (const size_t Index, STRING *StringEntry) const
{
  STRLIST *NodePtr = (STRLIST *) (VLIST::GetNodePtr (Index));
  if (NodePtr)
    {
      *StringEntry = NodePtr->String;
      return true;
    }
  StringEntry->Clear();
  return false;
}


//
//	Method name : SplitWords
//
//	Description : Split words from STRING into a list
//	Input : A STRING with words separated by white space
//	Output : A STRLIST of words
//
//
// NOTE: We blast some leading and trailing punctuation
//
// TODO: Use same word split algorithm as indexer
// and preserve hyphenated words
STRLIST& STRLIST::SplitWords (const STRING& TheString, const CHARSET *Charset)
{
  return SplitWords(TheString, Charset, NULL);
}


STRLIST& STRLIST::SplitWords (const STRING& TheString, const CHARSET *Charset,
	const LISTOBJ *Stopwords)
{
#define CTYPE(_s, _x) (Charset ? Charset->ib_##_s(_x) : _ib_##_s(_x))

  STRLIST      NewList;
  UCHR        *scratch = TheString.NewUCString ();
  const size_t TheLength = TheString.Len();

  UCHR *word = scratch;
  while (CTYPE(isspace, *word) || CTYPE(ispunct, *word))
    word++;
  if (word > scratch && word[-1] == '$')
    word--;
  for (UCHR *tp = word; (size_t)(tp - scratch) < TheLength ; tp++)
    {
      if ((CTYPE(ispunct, *tp) || CTYPE(isspace, *tp)) &&
	// Preserve words like $12
	!( tp == word && *tp == '$' && (*(tp+1) == '.' || CTYPE(isdigit, *(tp+1))) ) &&
	// Preserve words like 12%
	!( *tp == '%' && tp > word && CTYPE(isdigit, *(tp-1)) && (*(tp+1) == '\0' || CTYPE(isspace, *(tp+1))) ) &&
	// Preserve words like l'espace and auto-mobile, edz@nonmonotonic.com
	!( strchr("-'_.,@:", *tp) && CTYPE(isalnum, *(tp+1))) &&
	!IsTermChr(tp)  )
	{
	  *tp = '\0';
	  if (*word)
	    {
	      if (Stopwords == NULL || !Stopwords->InList(word))
		{
		  NewList.AddEntry (word);
		}
	    }
#if 1
	  // Skip over NON-Term characters
	  while (!IsTermChr(tp+1) && *(tp+1) != '$')
	    {
	      if (*++tp == '\0') break;
	    }
	  word = tp+1;
#else
	  while (CTYPE(isspace, *(tp+1)) || (CTYPE(ispunct, *(tp+1)) && *(tp+1) != '$'))
	    tp++; // Skip over spaces and punctuation (but not '$')
	  word = tp+1;
#endif
	}
    }
  if ((size_t)(word - scratch) < TheLength)
    {
      if (Stopwords == NULL || !Stopwords->InList(word))
	{
	  NewList.AddEntry(word);
	}
    }

  delete[]scratch;
  NewList.Sort();
  return *this = NewList;
}


//
//	Method name : SplitTerms
//
//	Description : Split words from STRING into a list
//	Input : A STRING with words separated by white space
//	Output : A STRLIST of words
//
//
// NOTE: We blast some leading and trailing punctuation
//
STRLIST& STRLIST::SplitTerms (const STRING& TheString, const CHARSET *Charset)
{
  STRLIST NewList;
  CHR *scratch = TheString.NewCString ();
  CHR *tcp = scratch;
  CHR *tp = NULL;
  static const CHR quote[] = {'"', '\'', '`'};
  STRING temp;

  enum { Normal, Quoted} State = Normal;
  do {
    while (CTYPE(isspace, (UCHR)*tcp))
      tcp++; // Ship leading white space
    if (*tcp == '\\' && *(tcp+1) == quote[0])
      {
	tcp += 2;
      }
    if (*tcp == quote[0])
      {
	if (State == Normal)
	  {
	    State = Quoted;
	    do {
	      tcp++;
	    } while (CTYPE(isspace, (UCHR)*tcp));
	    tp = tcp;
	  }
	else if (State == Quoted)
	  {
	    // Special Characters
	    if (*++tcp == '=') // Can be field=value or = as in case
	      {
		if (*tcp++ == quote[0])
		  {
		    State = Quoted;
		    continue;
		  }
		else if (*tcp == ':') // Weights
		  {
		    while (*tcp && !CTYPE(isspace, (UCHR)*tcp)) tcp++;
		  }
	      }
	    if (*tcp == '*' || *tcp == '~' || *tcp == '>'  )
		{
		  tcp++;
		}
	    // Handle []. () amd {} constructs
	    if (*tcp == '[' || *tcp == '(' || *tcp == '{')
	      {
		char end = (*tcp == '[') ? ']' : (*tcp == '(' ? ')' : '}');
		while (*tcp && *tcp != end) tcp++;
		if (*tcp) tcp++;
	      }
	    // Handle Weights
            if (*tcp == ':')
              {
                while (*tcp && !CTYPE(isspace, (UCHR)*tcp))
                  tcp++;
              }

	    if (*tcp) *tcp++ = '\0'; // Cut
	    while (CTYPE(isspace, (UCHR)*tp))
	      tp++;
	    temp = tp;
	    temp.cDecode(); // Map \" to "
	    NewList.AddEntry( temp );
	    State = Normal;
	    tp = NULL;
	    continue;
	  }
      }
    else if (State == Normal)
      tp = tcp;

    while (*tcp && !CTYPE(isspace, (UCHR)*tcp))
      {
	// Field/"term" or field="term" or field>"term" or field<"term"
	if ((*tcp == '/' || *tcp == '=' || *tcp == '<' || *tcp == '>')
		&& *(tcp+1) == quote[0] && State != Quoted)
	  {
	    State = Quoted;
	    tcp += 2;
	    break;
	  }
	if (*tcp == '\\' && *(tcp+1) == quote[0])
	  {
	    tcp += 2;
	  }
	if (*tcp == quote[0] && (CTYPE(isspace, (UCHR)*(tcp+1)) || CTYPE(ispunct, (UCHR)*(tcp+1))))
	  {
	    if (State == Normal) State = Quoted;
	    break;
	  }
	tcp++; // Move over word
      }
    if (*tcp && State == Normal)
      {
	if (tp > tcp && CTYPE(ispunct, *(tcp-1)))
	  *(tcp-1) = '\0';
	else
	  *tcp = '\0';
	if (*tp && tp)
	  {
	    temp = tp; 
	    temp.cDecode(); // Map \" to "
	    NewList.AddEntry( temp );
	  }
	tp = ++tcp;
      }
  } while (*tcp);
  if (tp && *tp)
    {
      // Clean `word', 'word'
      if (*tp == quote[1] && (*(tcp-1) == quote[1] || *(tcp-1) == quote[2]))
	{
	  *(tcp - 1) = '\0';
	  tp++;
	}
      temp = tp; 
      temp.cDecode(); // Map \" to "
      NewList.AddEntry( temp );
    }

  delete[] scratch;
  return *this = NewList;
}

#undef CTYPE

STRLIST& STRLIST::Split (const CHR *Separator, const STRING& TheString)
{
  size_t Position;
  const size_t SLen = strlen (Separator);

  Clear();
  // parse S and build list of terms
  STRING S (TheString);
  while ((Position = S.Search (Separator)) != 0)
    {
      STRING T(S);
      T.EraseAfter (Position - 1);
      S.EraseBefore (Position + SLen);
      if (!T.IsEmpty())
	AddEntry (T);
    }
  if (!S.IsEmpty())
    AddEntry (S);		// add the remaining entry
  return *this;
}

STRLIST& STRLIST::Split (const CHR Sep, const STRING& TheString)
{
#if 1 /* NEW FASTER CODE */
  const unsigned len = TheString.Len();
  size_t       end = 0;

  Clear();
  for (unsigned i=0; i< len; i++)
    {
      if (TheString[i] == Sep)
	{
	  if (i - end > 0)
	    AddEntry( TheString(end, i-end) );
	  end = i + 1;
	}
    }
  if (end < len)
    AddEntry( TheString(end, len-end) );
  return *this;
#else
  CHR Sep[] = {Separator, 0};
  return Split (Sep, TheString);
#endif
}


// getenv("PATH")
STRLIST& STRLIST::SplitPaths (const STRING& TheString)
{
  const size_t len = TheString.Len();
  size_t       end = 0;
  int          slash = 0;
  int          Ch;
  extern STRING __ExpandPath(const STRING&);


  Clear();
  for (unsigned i=0; i< len; i++)
    {
      // Watch out for \\ which is really '/' in DOS
      // in windows \\drive\ is different than /drive/
      if ((Ch = TheString[i]) == '\\')
	{
	  slash = !slash; // On
	}
      else if (slash == 0)
	{
	  if (Ch == ':' || Ch == ';' 
#ifndef WIN32
		|| _ib_isspace(Ch)
#endif
					)
	    {
	      if (i - end > 0)
		{
		  // Only add those that exist!
		  STRING S ( __ExpandPath(TheString(end, i-end)) );
		  if (Exists(S)) AddEntry( S ); 
		}
	      end = i + 1;
	    }
        }
      else
	slash = 0;
    }
  if (end < len)
    {
      // Only add if file or directory exists (resp. readable)!
      STRING S( __ExpandPath(TheString(end, len-end)) );
      if (Exists(S)) AddEntry(S);
    }
  return *this;
}


STRING STRLIST::Join (const CHR *Separator) const
{
  STRING NewString;
  int    follow = 0;
  for (const STRLIST *p = Next(); p != this; p = p->Next() )
    {
      if (follow)
        NewString.Cat ( Separator );
      else
	follow = 1;
      NewString.Cat ( p->Value() ); 
    }
  return NewString;
}

STRING STRLIST::Join (const CHR Separator) const
{
  CHR Sep[] = {Separator, 0};
  return Join (Sep);
}


size_t STRLIST:: Search (const STRING& SearchTerm) const
{
  size_t x = 0;
  for (const STRLIST *p = Next(); p != this; p = p->Next() )
    {
      x++;
      if (p->String == SearchTerm)
	return x;
    }
  return 0;
}

size_t STRLIST::SearchCase (const STRING& SearchTerm) const
{
  size_t x = 0;
  for (const STRLIST *p = Next(); p != this; p = p->Next() )
    {
      x++;
      if (p->String ^= SearchTerm)
	return x;
    }
  return 0;
}

size_t STRLIST::SetValue (const STRING& Title, const STRING& Value)
{
  const size_t pos = GetValue(Title, (STRING *)NULL);

  if (Value.GetLength() == 0)
    {
      if (pos)
	DeleteEntry(pos);
    }
  else
    {
      STRING s;
      s << Title << "=" << Value;
      if (pos)
	SetEntry(pos, s);
      else
	AddEntry(s);
    }
  return pos;
}

size_t STRLIST::GetValue (const STRING& Title, STRING *StringBuffer) const
{
  if (StringBuffer) StringBuffer->Clear();

  size_t x = 1;
  for (const STRLIST *p = Next(); p != this; p = p->Next(), x++ )
    {
      STRING            S        = p->Value();
      const STRINGINDEX Position =  S.Search ('=');
      if (Position != 0)
	{
	  S.EraseAfter (Position - 1);

	  register int zap = 0;
	  // get rid of leading spaces
	  while (_ib_isspace(S.GetChr (zap+1)))
	    zap++;
	  if (zap)
	    S.EraseBefore (zap+1);

	  // get rid of trailing spaces
	  zap = S.GetLength();
	  const int y = zap;
	  while (_ib_isspace(S.GetChr (zap)))
	    zap--;
	  if (zap != y)
	    S.EraseAfter (zap);

	  if (S ^= Title)
	    {
	      if (StringBuffer)
		{
		  *StringBuffer = p->Value();
		  StringBuffer->EraseBefore (Position + 1);
		}
	      return x;
	    }
	}
    }
  return 0; // NOT FOUND
}

size_t STRLIST::GetValue (const STRING& Title, INT *intBuffer) const
{
  STRING s;
  size_t x;
  if ((x=GetValue(Title, &s) && s.IsNumber()) != 0)
    {
      if (intBuffer) *intBuffer = (INT)s;
      return x;
    }
  return 0;
}

size_t STRLIST::GetValue (const STRING& Title, DOUBLE *numBuffer) const
{
  STRING s;
  size_t x;
  if ((x=GetValue(Title, &s) && s.IsNumber()) != 0)
    {
      if (numBuffer) *numBuffer = (DOUBLE)s;
      return x;
    }
  return 0;
}



ostream& operator << (ostream& os, const STRLIST& str)
{
  size_t i = 0;
  for (const STRLIST *p = str.Next(); p != &str; p = p->Next())
    {
      if (i++) os << ", ";
      os << '"' << p->Value() << '"';
    }
  return os;
}

void STRLIST::Write (PFILE Fp) const
{
  const size_t TotalEntries = GetTotalEntries ();
  putObjID(objSTRLIST, Fp);
  ::Write((UINT4)TotalEntries, Fp); // Write Count
  UINT4 i = 0;
  for (const STRLIST *p = Next(); p != this && i < TotalEntries; p = p->Next(), i++)
    p->Value().Write(Fp);
}

bool STRLIST::Read (PFILE Fp)
{
  STRLIST NewList;
  obj_t obj = getObjID(Fp);
  if (obj != objSTRLIST)
    {
      // Not a STRLIST Object!
       PushBackObjID(obj, Fp);
    }
  else
    {
      STRING S;
      UINT4 TotalEntries;

      ::Read(&TotalEntries, Fp); // Fetch Count
      for (size_t i = 1; i <=TotalEntries; i++)
	{
	  S.Read(Fp);
	  NewList.AddEntry (S);
	}
    }
  *this = NewList;
  return obj == objSTRLIST;
}

UINT8 STRLIST::Hash() const
{
  int   len = 0;
  UINT8 hash = 0;
  for (const STRLIST *p = Next(); p != this; p = p->Next(), len++)
    {
      (hash <<= 8) += p->Value().Hash();
    }
  hash <<= 4;
  hash += len;
  return hash;
}


bool STRLIST::Equals(const STRLIST& OtherList) const
{
  const STRLIST *p = Next();
  const STRLIST *p2 = OtherList.Next();

  while (p != this && p2 != &OtherList)
    { 
      if (p->Value() != p2->Value())
	return false;
      p  = p->Next();
      p2 = p2->Next();
    }
 return (p == this && p2 == &OtherList);
}

bool STRLIST::CaseEquals(const STRLIST& OtherList) const
{
  const STRLIST *p = Next();
  const STRLIST *p2 = OtherList.Next();
 
  while (p != this && p2 != &OtherList)
    {
      if (!(p->Value() ^= p2->Value()))
        return false;
      p  = p->Next();
      p2 = p2->Next();
    }
 return (p == this && p2 == &OtherList);
}


 // Contains
bool     STRLIST::Contains(const STRING& Item) const
{
  for (const STRLIST *p = Next(); p != this; p = p->Next())
    if (p->Value() == Item) return true;
  return false;
}

bool     STRLIST::ContainsCase(const STRING& Item) const
{
   // Case independent
   for (const STRLIST *p = Next(); p != this; p = p->Next())
    if (p->Value() ^= Item) return true;
   return false;
}




STRLIST::~STRLIST ()
{
}

// Common functions

void Write(const STRLIST Strlist, PFILE Fp)
{
  Strlist.Write(Fp);
}

bool Read(PSTRLIST StrlistPtr, PFILE Fp)
{
  return StrlistPtr->Read(Fp);
}



int STRLIST::Do(bool (*Function)(const STRING& What))
{
  int count = 0;
  for (STRLIST *f = Next(); f != this; )
    {
      STRLIST *f0 = f;
      f = f->Next();
      if (Function(f0->Value()) == true)
	delete f0->Disattach();
      else
	count++;
    }
  return count;
}

