/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
// $Id: thesaurus.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $

/*@@@
File:		thesaurus.hxx
Description:	Class THESAURUS - Thesaurus and synonyms
@@@*/

#include "thesaurus.hxx"
#include "date.hxx"
#include "pathname.hxx"

/*
# Rows of the form:
# parent phrase = child1+child2+multiword child+ ... +childN
#
# White space is ignored at the start and end of child terms
# Comments start with #
spatial=geospatial+geographic+terrestrial # Here are more comments
land use=land cover + land characterization + land surface + ownership property
wetlands=wet land+NWI+hydric soil+inundated
hydrography=stream+river+spring+lake+pond+aqueduct+siphon+well
hypsography=elevation + relief + topgraphy + contour
*/


/*
extern INT ParentSortCmp(const void* x, const void* y);
extern INT ParentSearchCmp(const void* x, const void* y);
extern INT EntrySortCmp(const void* x, const void* y);
extern INT EntrySearchCmp(const void* x, const void* y);
*/

/////////////////////////////////////////////////////////////////
// Class: TH_PARENT
/////////////////////////////////////////////////////////////////
/*
void TH_PARENT::Copy(const TH_PARENT& OtherValue)
{
}
*/


/////////////////////////////////////////////////////////////////
// Class: TH_PARENT_LIST
/////////////////////////////////////////////////////////////////

static INT ParentSortCmp(const void* x, const void* y) 
{
  return ((TH_PARENT*)x)->GetString().Cmp ( ((TH_PARENT*)y)->GetString()  );
}


static INT ParentSearchCmp(const void* x, const void* y) 
{
  return -((TH_PARENT*)y)->GetString().Cmp((const char *)x);
}


//  Here are the methods for handling lists of parent terms
TH_PARENT_LIST::TH_PARENT_LIST()
{
  MaxEntries = 100;
#ifdef __EXCEPTIONS
  try {
    table = new TH_PARENT[MaxEntries];
  } catch (...) {
    MaxEntries = 0;
  }
#else
   table = new TH_PARENT[MaxEntries];
   if (table == NULL) MaxEntries = 0;
#endif

  Count = 0;
}


void TH_PARENT_LIST::AddEntry(const TH_PARENT& NewParent)
{
  if (Count >= MaxEntries)
    {
      // Expand
      TH_PARENT *oldTable = table;
      TH_PARENT *newTable;
      size_t     want     = MaxEntries*2 + 50;

      try {
	newTable = new TH_PARENT[want];
      } catch (...) {
	message_log (LOG_ERRNO, "Can't allocate TH_PARENT_LIST with %ld elements", (long)want);
	return; // can't expand things;
      }
      for (size_t i=0; i<Count; i++)
	newTable[i] = oldTable[i];
      table = newTable;
      MaxEntries = want;
      delete[] oldTable;
    }
  table[Count++] = NewParent;
}


void TH_PARENT_LIST::GetEntry(const size_t index, TH_PARENT* TheParent)
{
  if (index <= Count)
    *TheParent = table[index];
}


TH_PARENT*TH_PARENT_LIST::GetEntry(const size_t index) {
  if (index <= Count)
    return(&table[index]);
  return((TH_PARENT*)NULL);
}


size_t TH_PARENT_LIST::GetCount() const
{
  return(Count);
}


void TH_PARENT_LIST::Dump(PFILE fp)
{
  if (fp) {
    for (size_t i=0;i<Count;i++) {
      fprintf(fp,"[%d]\t%s\n", table[i].GetGlobalStart(),
	table[i].GetString().c_str());
    }
  }
}


void TH_PARENT_LIST::WriteTable(PFILE fp) {
  if (fp) {
    ::Write((INT4)Count, fp);
    for (size_t i=0;i<Count;i++)
      ::Write(table[i].GetGlobalStart(), fp);
  }
}


void TH_PARENT_LIST::LoadTable(PFILE fp)
{
  if (fp) {
    INT4 x;
    ::Read(&x, fp);

    Count = (x > 0 ? (size_t)x : 0);
    if (Count == 0)
      return;

    TH_OFF_T ptr;
    for (size_t i=0;i<Count;i++) {
      ::Read(&ptr, fp);
      if (ptr > 0) {
	table[i].SetGlobalStart(ptr);
	table[i].SetString(NulString);
      } else {
	table[i].SetGlobalStart(0);
      }
    }
  } else {
    Count = 0;
  }
}


void TH_PARENT_LIST::Sort()
{
  QSORT((void *)table, Count, sizeof(TH_PARENT),ParentSortCmp);
}


void* TH_PARENT_LIST::Search(const void* term)
{
  return bsearch(term, (void *)table, Count, sizeof(TH_PARENT),ParentSearchCmp);
}


TH_PARENT_LIST::~TH_PARENT_LIST() {
  delete [] table;
}


/////////////////////////////////////////////////////////////////
// Class: TH_ENTRY_LIST
/////////////////////////////////////////////////////////////////

static INT EntrySortCmp(const void* x, const void* y) 
{
  return ((TH_ENTRY*)x)->GetString().Cmp(((TH_ENTRY*)y)->GetString());
}


static INT EntrySearchCmp(const void* x, const void* y) 
{
  return -((TH_ENTRY*)y)->GetString().Cmp( (const char *)x);
}


//  Here are the methods for handling lists of parent terms
TH_ENTRY_LIST::TH_ENTRY_LIST()
{
  MaxEntries = 25;
  try {
    table = new TH_ENTRY[MaxEntries];
  } catch (...) {
    MaxEntries = 0;
  }
  Count = 0;
}


void TH_ENTRY_LIST::AddEntry(const TH_ENTRY& NewChild)
{
  if (Count >= MaxEntries)
    {
      // Expand
      TH_ENTRY *oldTable = table;
      TH_ENTRY *newTable;
      size_t    want     = MaxEntries*3 + 10;

      try {
	newTable = new TH_ENTRY[want];
      } catch (...) {
	message_log (LOG_ERRNO, "Can't allocate TH_ENTRY_LIST with %ld elements", (long)want);
	return;
      }
      for (size_t i=0; i<Count; i++)
        newTable[i] = oldTable[i];
      table = newTable;
      MaxEntries = want;
      delete[] oldTable;
    }
  table[Count++] = NewChild;
}


void TH_ENTRY_LIST::GetEntry(const size_t index, TH_ENTRY* TheChild)
{
  if (index <= Count) {
    *TheChild = table[index];
  }
}


TH_ENTRY* TH_ENTRY_LIST::GetEntry(const size_t index) {
  if (index <= Count) {
    return(&table[index]);
  }
  return((TH_ENTRY*)NULL);
}


size_t TH_ENTRY_LIST::GetCount() const
{
  return(Count);
}


void TH_ENTRY_LIST::Dump(PFILE fp)
{
  if (fp) {
    for (size_t i=0;i<Count;i++) {
      const TH_OFF_T ptr1 = table[i].GetGlobalStart();
      const TH_OFF_T ptr2 = table[i].GetParentPtr();
      fprintf(fp,"[%d]\t%s, child of %d\n",ptr1, table[i].GetString().c_str(),ptr2);
    }
  }
}


void TH_ENTRY_LIST::WriteTable(PFILE fp) {
  STRING str;

  if (fp) {
    ::Write((INT4)Count, fp);
    for (size_t i=0;i<Count;i++) {
      const TH_OFF_T ptr1 = table[i].GetGlobalStart();
      const TH_OFF_T ptr2 = table[i].GetParentPtr();
      ::Write(ptr1, fp);
      ::Write(ptr2, fp);
    }
  }
}


void TH_ENTRY_LIST::LoadTable(PFILE fp) {
  if (fp) {
    INT4  x;
    ::Read(&x, fp);
    Count = (x > 0 ? (size_t)x : 0);
    if (Count == 0)
      return;

    TH_OFF_T ptr;
    for (size_t i=0;i<Count;i++) {
      ptr = 0;
      ::Read(&ptr, fp);
      if (ptr > 0) {
	table[i].SetGlobalStart(ptr);
	table[i].SetString(NulString);
      } else {
	table[i].SetGlobalStart(0);
      }
      ptr = 0;
      ::Read(&ptr, fp);
      if (ptr > 0) {
	table[i].SetParentPtr(ptr);
      } else {
	table[i].SetParentPtr(0);
      }
    }
  } else {
    Count = 0;
  }
}


void TH_ENTRY_LIST::Sort()
{
  QSORT((void *)table, Count, sizeof(TH_ENTRY),EntrySortCmp);
}


void* TH_ENTRY_LIST::Search(const void* term)
{
  return bsearch(term, (void *)table, Count, sizeof(TH_ENTRY),EntrySearchCmp);
}


TH_ENTRY_LIST::~TH_ENTRY_LIST() {
  delete [] table;
}




/////////////////////////////////////////////////////////////////
// Class: THESAURUS
/////////////////////////////////////////////////////////////////

THESAURUS::THESAURUS()
{
}

// This is the search-time constructor
THESAURUS::THESAURUS(const STRING& Path)
{
  Init(Path);
}

void THESAURUS::Init(const STRING& Path)
{
  SetFileName(Path);
  LoadParents();
  LoadChildren();
#if 0
  cout << "Parents-----------" << endl;
  Parents.Dump(stdout);
  cout << "Children----------" << endl;
  Children.Dump(stdout);
#endif
}



// This is the index-time constructor.  It parses the input file and
// creates the synonym table and the indexes for parents and children.
THESAURUS::THESAURUS(const STRING& Source, const STRING& Target, bool Force)
{
  Compile(Source, Target, Force);
}

bool THESAURUS::Compile(const STRING& SourceFileName, const STRING& Target, bool Force)
{
  const STRING SynFile ( Target + DbExtDbSynonyms);

  SetFileName(Target);

  if (!Force) {
    STRING SynChildFileName;
    const STRING SynParentFileName ( Target + DbExtDbSynParents);
    if (FileExists(SynFile) && FileExists(SourceFileName) + FileExists(SynParentFileName))
      {
        // if SynFile is newer than we don't need to compile
        SRCH_DATE SynFileDate, SrcFileDate, SynParFileDate;
 
        SynFileDate.SetTimeOfFile(SynFile);
        SrcFileDate.SetTimeOfFile(SourceFileName);
        SynParFileDate.SetTimeOfFile(SynParentFileName);
        if ((SynFileDate > SrcFileDate) && (SynParFileDate>=SynFileDate))
          return true; // Don't need to compile
      }
   }

  if (SynFile == SourceFileName)
     FileLink(SourceFileName, SourceFileName + ".BAK");
  
  // Create the file names for the thesaurus files
  // -- dbname.syn holds the actual text synonyms
  // -- dbname.spx holds the index of parent terms
  // -- dbname.scx holds the index of child terms

  STRING sBuf;
  // Read in the user-specified file
  sBuf.ReadFile(SourceFileName);

  // Dump it into a character buffer and parse it on newlines
  const char IFS[] = "\r\n";

  CHR *b    = sBuf.NewCString();
  CHR *ctxt;  // strtok_r context
  CHR *pBuf = strtok_r (b, IFS, &ctxt);

  // Now, pBuf points to one synonym definition
  size_t ParentGP = 0;

  FILE *Fp = OpenSynonymFile("wb");
  if (!Fp)
    return false;

  TH_PARENT TheParent;
  TH_ENTRY  TheChild;

  STRING ParentString, ChildString, ThisChild;

  do {
    // Skip leading blanks
    while (*pBuf == ' ')
      pBuf++;

    // Skip comments
    if (*pBuf != '\0' && *pBuf != '#') {
//  cerr << "Input string ->" << pBuf << "<-\n";

      // Now split the line into parent and children 
      // Make the parent first, trimmed and lower case
      ParentString = pBuf;
      STRINGINDEX eq_sign = ParentString.Search('=');
      ParentString.EraseAfter(eq_sign-1);
      ParentString.ToLower();
      ParentString.Pack(); // Pack it!

      // Store the information into a TH_PARENT object
      TheParent.SetString(ParentString);
      TheParent.SetGlobalStart(ParentGP);

      // Put the object into the list of Parent objects
      Parents.AddEntry(TheParent);

      // Make the child string - we have to clean up the terms
      // individually, so it will be kinda slow
      ChildString = pBuf;
      ChildString.EraseBefore(eq_sign+1); // Get rid of the parent
      STRINGINDEX num_sign = ChildString.Search('#');
      ChildString.EraseAfter(num_sign-1); // Get rid of trailing comments
//    ChildString.ToLower(); // Lower

      // Clean the children by loading into a STRLIST, then looping over
      // each child in the list, trimming off leading and trailing junk
      //
      // Once we have each child, we need to make a TH_ENTRY object out
      // of it so we can store it into the children index

      size_t ChildOffset = ParentGP + ParentString.GetLength() + 1;
      STRLIST ChildrenList;

      ChildrenList.Split('+',ChildString);
      for (STRLIST *ptr = ChildrenList.Next(); ptr != &ChildrenList; ptr=ptr->Next())
	{	
	  (ThisChild = ptr->Value()).Pack();
	  TheChild.SetString(ThisChild);
	  TheChild.SetParentPtr(ParentGP);
	  TheChild.SetGlobalStart(ChildOffset);
	  Children.AddEntry(TheChild);
	  ChildOffset += ThisChild.GetLength()+1;
	}
      // Make a new string for the synonym file
      ParentString += "=";
      ParentString += ChildrenList.Join("+");
//    cerr << "Output string->" << ParentString << "<- [" << ParentGP << "]" << endl;
      ParentString += "\n";

      // Write the entry out to the synonym file
      ParentString.Print(Fp);

      // Update to point to the start of the next line in the file
      ParentGP += ParentString.GetLength();
    }
  } while ( (pBuf = strtok_r ((CHR*)NULL, IFS, &ctxt)) );

  delete [] b;
  fclose(Fp);

  // Write out the index of the parents
  Parents.Sort();

  Fp = OpenParentsFile("wb");
  if (Fp) {
    Parents.WriteTable(Fp);
    fclose(Fp);
  }

  // Write out the index of the children
       //  Children.Dump(stdout);
  Children.Sort();
  //  cout << "------------" << endl;
  //  Children.Dump(stdout);
  Fp = OpenChildrenFile("wb");
  if (Fp) {
    Children.WriteTable(Fp);
    fclose(Fp);
  }
  return true;
}


FILE* THESAURUS::OpenSynonymFile(const char *mode)
{
// cerr << "BaseFileName = " << BaseFileName << endl;
  return fopen(BaseFileName + DbExtDbSynonyms, mode);
}


FILE* THESAURUS::OpenParentsFile(const char *mode)
{
  return fopen(BaseFileName + DbExtDbSynParents, mode);
}


FILE* THESAURUS::OpenChildrenFile(const char *mode)
{
  return fopen(BaseFileName + DbExtDbSynChildren, mode);
}


void THESAURUS::GetIndirectString(FILE *fp, const TH_OFF_T ptr, STRING* term)
{
  term->Clear();
  // Offset into the synonym table and read the row
  if (fseek(fp,ptr,SEEK_SET) != -1)
    {
      int ch;
      while ((ch = getc(fp)) != EOF && ch != '=' && ch != '+' && ch != '\r' && ch != '\n')
	{
	  term->Cat ((CHR)ch);
	}
    }
}


void THESAURUS::LoadParents()
{
  FILE *fp;

  // Load up the starting pointers into the parent list
  if ((fp = OpenParentsFile("rb")) == NULL)
    return;
  Parents.LoadTable(fp);
  fclose(fp);

  if ((fp = OpenSynonymFile("rb")) == NULL) {
    return;
  }

  STRING b;
  const size_t TheCount = Parents.GetCount();
  for (size_t i=0;i<TheCount;i++) {
    TH_PARENT *TheParent = Parents.GetEntry(i); // Get ptr to the entry
    if (TheParent)
      {
	GetIndirectString(fp, TheParent->GetGlobalStart(), &b);
	// Save the string
	TheParent->SetString(b);
      }
  } // Done loading parents
  fclose(fp);
}


void THESAURUS::LoadChildren()
{
  FILE *fp;

  // Load up the starting pointers into the parent list
  if ((fp = OpenChildrenFile("rb")) == NULL)
    return;
  Children.LoadTable(fp);
  fclose(fp);

  if ((fp = OpenSynonymFile("rb")) == NULL)
    return;
  size_t TheCount = Children.GetCount();
  for (size_t i=0;i<TheCount;i++) {
    TH_ENTRY *TheEntry = Children.GetEntry(i); // Get ptr to the entry
    if (TheEntry)
      {
	STRING b;
	GetIndirectString(fp, TheEntry->GetGlobalStart(), &b);
	// Save the string
	TheEntry->SetString(b);
      }
  } // Done loading children
  fclose(fp);
}


bool THESAURUS::MatchParent(const STRING& ParentTerm,  TH_OFF_T *ptr)
{
  STRING TheTerm (ParentTerm);
  TheTerm.ToLower();
  TH_PARENT *p = (TH_PARENT*)Parents.Search( TheTerm );
  if (p) {
    if (ptr) *ptr = p->GetGlobalStart();
    return true;
  }
  return false;
}


// Given the term, go get a list of child terms.  Always return at least
// the original term in the list, so it is always safe to use the 
// returned value
void THESAURUS::GetChildren(const STRING& ParentTerm, STRLIST* Children)
{
  TH_OFF_T ptr;
  if (MatchParent(ParentTerm,&ptr)) {
    FILE *fp = OpenSynonymFile("rb");
    if (!fp)
      return;

    STRING TheEntry;
    fseek(fp,ptr,SEEK_SET);
    if (TheEntry.FGet (fp))
      {
	TheEntry.Replace("=","+");
	Children->Split('+',TheEntry);
      }
    fclose(fp);
  } else {
    Children->Clear();
    Children->AddEntry(ParentTerm);
  }
}


// Tell the caller if there is a match in the list of children terms
bool THESAURUS::MatchChild(const STRING& Term,  TH_OFF_T *ptr)
{
  STRING TheTerm (Term);
  TheTerm.ToLower();

  TH_ENTRY *p = (TH_ENTRY*)Children.Search( TheTerm );
  if (p) {
    if (ptr) *ptr = p->GetParentPtr();
    return true;
  }
  return false;
}


// Given the term, look for the parent term.  If no parent term is found,
// return the original term, so it is always safe to use the returned value
void THESAURUS::GetParent(const STRING& ChildTerm, STRING* TheParent)
{
  TH_OFF_T ptr;

  if (MatchChild(ChildTerm,&ptr)) {
    FILE   *fp;
    if ((fp = OpenSynonymFile("rb")) == NULL)
      return;
    GetIndirectString(fp,ptr, TheParent);
    fclose(fp);
  } else {
    *TheParent=ChildTerm;
  }
}


THESAURUS::~THESAURUS() {
}

#ifdef MAIN
int
main(int argc, char** argv) {
  STRING Flag;
  INT x=0;
  STRING SynonymFileName;
  STRING DbPath,DbName;
  bool HaveSynonyms=false;
  INT LastUsed = 0;
  STRLIST Children;

  STRING ParentTerm,ChildTerm;
  ParentTerm = "spatial";
  ChildTerm = "terrestrial";

  DbPath = "/tmp";
  DbName = "test";

  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-syn")) {
        if (++x >= argc) {
          fprintf(stderr,
                  "ERROR: No synonym file name specified after -syn.\n\n");
          EXIT_ERROR;
        }
        SynonymFileName = argv[x];
        HaveSynonyms = true;
        LastUsed = x;
      }
    }
    x++;
  }

  if (HaveSynonyms) {
    THESAURUS *MyThesaurus;
  
    // Build a new thesaurus
    MyThesaurus = new THESAURUS(SynonymFileName,DbPath,DbName);
    delete MyThesaurus;

    // Load an existing thesaurus
    cout << "-------" << endl;
    MyThesaurus = new THESAURUS(DbPath,DbName);

    // Look for the children matching a parent term
    MyThesaurus->GetChildren(ParentTerm,&Children);
    cout << "Children for " << ParentTerm << ":" << endl;
    Children.Dump(stdout);
    cout << "-------" << endl;
    MyThesaurus->GetParent(ChildTerm,&ParentTerm);
    cout << "Parent of " << ChildTerm << " is " << ParentTerm << endl;
    // Clean up
    delete MyThesaurus;

  }
  EXIT_ZERO;
}

#endif
