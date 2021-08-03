#pragma ident  "@(#)vidb.cxx"

/*-@@@
File:           vidb.cxx
Version:        1.00
Description:    Class VIDB, Virtual IDB
Author:         Kevin Gamiel, kgamiel@cnidr.org
		Nassib Nassar, nrn@cnidr.org
		Edward Zimmermann, edz@nonmonotonic.com
@@@*/

//////////////////////////////////////////////////////
///// TODO:
/////
///// Make Multithreaded in calling Search for each
///// virtual subindex...
/////
//////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "idbobj.hxx"
#endif

#include "common.hxx"
#include "vidb.hxx"
#include "stoplist.hxx"
#ifdef HAVE_LOCALE
#include <locale.h>
#endif


#ifdef MULTITHREADED
# include <pthread.h>
# include <semaphore.h>
#endif


static const STRING DbInfoSection ("DbInfo");
static const STRING CollectionsEntry ("Collections");
static const STRING DatabasesEntry ("Databases");
static const STRING DatabaseListFileEntry ("vdb-file");

//
//  Calls Initialize()
//

#define Init() { MainDfdt = NULL; c_dbcount = 0; c_dblist = NULL; c_irsetlist = NULL; c_rsetlist = NULL;\
	 MainRegistry = NULL; Opened = GDT_FALSE; c_inconsistent_doctypes = GDT_FALSE; }


VIDB::VIDB()
{
  Init();
// Open (NulString, NulStrlist, GDT_TRUE);
}

VIDB::VIDB(const STRING& DBName)
{
  Init();
  Open (DBName, NulStrlist, GDT_TRUE);
}


VIDB::VIDB(const STRING& DBName, REGISTRY *Registry)
{
  Init();
  Open (DBName, NulStrlist, GDT_TRUE);
}


VIDB::VIDB(const STRING& DBName, GDT_BOOLEAN Searching)
{
  Init();
  Open (DBName, NulStrlist, Searching);
}


VIDB::VIDB(const STRING& DBName, const STRLIST& NewDocTypeOptions)
{
  Init();
  Open (DBName, NewDocTypeOptions, GDT_TRUE);
}


VIDB::VIDB(const STRING& DBName, const STRLIST& NewDocTypeOptions, const GDT_BOOLEAN Searching)
{
  Init();
  Open (DBName, NewDocTypeOptions, Searching);
}

VIDB::VIDB (const STRING& NewPathName, const STRING& NewFileName,
      const STRLIST& NewDocTypeOptions)
{
  Init();
  Open (NewPathName, NewFileName, NewDocTypeOptions, GDT_TRUE);
}


VIDB::VIDB (const STRING& NewPathName, const STRING& NewFileName,
      const STRLIST& NewDocTypeOptions, const GDT_BOOLEAN Searching)
{
  Init();
  Open (NewPathName, NewFileName, NewDocTypeOptions, Searching);
}

//
//  Calls Init/Open()
//
VIDB::VIDB (const STRING& NewPathName, const STRING& NewFileName)
{
  Init();
  Open (NewPathName, NewFileName, NulStrlist, GDT_TRUE);
}

VIDB::VIDB (const STRING& NewPathName, const STRING& NewFileName,
	GDT_BOOLEAN Searching)
{
  Init();
  Open (NewPathName, NewFileName, NulStrlist, Searching);
}

//
// Reads ".ini" and, if it exists, gets a few fields and a file list.
// If the file list is empty or the ".ini" file did not exist it 
// attempts to locate a database file with extension ".vdb".  If that
// file does not exist, it attempts to open the database normally.  If
// it does exist, it opens that file and assumes there to be a list of
// database names separated by newline characters.  It loads each
// database listed in the ".vdb" file and subsequent search and
// present operations are performed on the entire list of databases.
//
//
// Each database MUST be indexed with the same doctype or this will
// fail! -- @@@ edz@nonmonotonic.com: Not sure of that!
//
GDT_BOOLEAN VIDB::Open (const STRING& DBname, const STRLIST& NewDocTypeOptions,
	const GDT_BOOLEAN SearchOnly)
{
  logf (LOG_DEBUG, "VIDB::Open('%s',..,%d)", DBname.c_str(), SearchOnly);
  size_t t = DBname.Search("<");
  if (t > 0 && t < 5 && DBname.Search(">")>t)
    {
      // DBname is probably an XML buffer
      return Open (NulString, NulString, NewDocTypeOptions, SearchOnly, DBname);
    }
  else if ((t = DBname.Search("[")) > 0 && t < 5 && DBname.Search("]") > 4)
    {
      // DBname is perhaps an .ini buffer
      return Open (NulString, NulString, NewDocTypeOptions, SearchOnly, DBname);
    }
  else
    {
      STRING  fullpath ( DBname.IsEmpty() ? ExpandFileSpec(__IB_DefaultDbName) : ExpandFileSpec(DBname) );
      //logf (LOG_DEBUG, "VIDB::Open '%s'", fullpath.c_str());
      return Open (RemoveFileName(fullpath),  RemovePath (fullpath), NewDocTypeOptions, SearchOnly);
    }
}

size_t VIDB::GetDbSearchCutoff(size_t Idx) const
{
  size_t cut = 0;
  if (Idx == 0)
    {
      size_t val;
      // Return Min.
      for (size_t i=0; i<c_dbcount; i++)
	{
	  if ((val = c_dblist[i]->GetDbSearchCutoff()) > 0 && val < cut)
	    cut = val;
	}
    } 
  else if (Idx > 0 && Idx <= c_dbcount)
    cut = c_dblist[Idx-1]->GetDbSearchCutoff();
  return cut;
}

static STRLIST& CollectDBs(STRLIST *FilenameList, const STRING& Path)
{
  if (!FilenameList->IsEmpty()) {
    // Collection of virtual databases....
    STRLIST         newList;
    FILE           *Fp;
    for (const STRLIST * p = FilenameList->Next(); p != FilenameList; p = p->Next()) {
      const STRING    db((IsAbsoluteFilePath(p->Value()) ? p->Value() : Path + p->Value()));
      const STRING    ini = db + (STRING) DbExtDbInfo;
      if (newList.Search(db) != 0) {
	logf(LOG_ERROR, "Circular collection around %s", db.c_str());
	continue;
      }
      logf(LOG_DEBUG, "Opening '%s'", ini.c_str());
      if ((Fp = fopen(ini, "r")) != NULL) {
	REGISTRY        Registry("VirtualIsearch");
	GDT_BOOLEAN     loaded = Registry.Read(Fp);
	fclose(Fp);
	if (!loaded)
	  continue;
	STRLIST         tmpList;
	Registry.ProfileGetString(DbInfoSection, CollectionsEntry, &tmpList);
	if (!tmpList.IsEmpty()) {
	  newList += CollectDBs(&tmpList, Path);
	  continue;
	} else {
	  Registry.ProfileGetString(DbInfoSection, DatabasesEntry, &tmpList);
	  if (!tmpList.IsEmpty()) {
	    newList += tmpList;
	  } else {
	    logf(LOG_WARN, "'%s' did not have a '%s' specified in [%s]",
		 ini.c_str(), DatabasesEntry.c_str(), DbInfoSection.c_str());
	    newList.Cat(db);
	  }
	}
      } else
	logf(LOG_ERRNO, "Could not open '%s'", ini.c_str());
    }
    logf(LOG_DEBUG, "Sorting list of indexes..");
    newList.UniqueSort();	// Sort
    *FilenameList = newList;
  }
  return *FilenameList;
}


// Read from a .vdb.. Supports includes when it sees one..
static size_t ReadFromFile(const STRING& vdb, STRLIST *FilenameListPtr, int depth = 0)
{
  //
  // Try to read the ".vdb" file if it exists
  //
  if (!Exists(vdb))
    return 0;
  if (depth > 20)
    {
      logf(LOG_ERROR, "Virtual db include too deep (%d). Circular includes in '%s'?", depth,  vdb.c_str());
      return 0;
    }
  if (depth) logf (LOG_DEBUG, "Virtual db include (depth=%d) of '%s'", depth, vdb.c_str());

  STRING  RawFilenameList, Filename;
  STRLIST AddList;
  size_t  DatabaseCount = 0;

  RawFilenameList.ReadFile (vdb);
  if (!RawFilenameList.IsEmpty())
    {
      AddList.Split ('\n', RawFilenameList);
      // Add Entries;
      for (const STRLIST *s = AddList.Next(); s != &AddList; s = s->Next())
	{
	  Filename = s->Value();
	  // ; is a comment
	  STRINGINDEX pos = Filename.Search(';');
	  if (pos) Filename.EraseAfter(pos-1);
	  Filename.Trim();

	  if (Filename.GetLength())
	    {
	      if (Filename.GetLength() > 5 && Filename.Search(DbExtVdb) == (Filename.GetLength()-strlen(DbExtVdb) + 1))
		{
		  DatabaseCount += ReadFromFile(Filename, FilenameListPtr, ++depth);
		}
	      else
		{
		  // Add it to the list
		  DatabaseCount++;
		  FilenameListPtr->AddEntry(Filename);
		}
	    }
	}
    }
  return DatabaseCount;
}

GDT_BOOLEAN VIDB::Open (const STRING& NewPathName, const STRING& NewFileName,
	const STRLIST& NewDocTypeOptions, const GDT_BOOLEAN SearchOnly)
{
  return Open(NewPathName, NewFileName, NewDocTypeOptions, SearchOnly, NulString);
}


GDT_BOOLEAN VIDB::Open(const STRING& NewPathName, const STRING& NewFileName,
            const STRLIST& NewDocTypeOptions, const GDT_BOOLEAN SearchOnly, const STRING& XMLBuffer)
{
  if (Opened)
    {
      if (Close() == GDT_FALSE)
	return GDT_FALSE;
    }

  Opened = GDT_TRUE;

  logf (LOG_DEBUG, "Initializing virtual DB \"%s%s\" ",  NewPathName.c_str(), NewFileName.c_str());

  // Clear
#ifdef BSD
  bzero(c_index, sizeof(c_index)/sizeof(c_index[0])); 
#else
  memset(c_index, '\0', sizeof(c_index)/sizeof(c_index[0]));
#endif

  MainRegistry = new REGISTRY ("VirtualIsearch");

  //
  // Build various filenames
  //
  DbPathName = ExpandFileSpec( AddTrailingSlash(NewPathName));
  if (NewFileName.Search(PathSepChar())) DbPathName.Cat (RemoveFileName(NewFileName));
  AddTrailingSlash(&DbPathName);

  DbFileName = RemovePath(NewFileName);
  if (DbFileName.IsEmpty())  DbFileName = __IB_DefaultDbName;

  STRLIST FilenameList;
  STRLIST GlobalDocTypeList;

  PFILE Fp=fopen(GetDbFileStem()+ (STRING)DbExtDbInfo, "r");
  if (Fp)
    {
      MainRegistry->Read (Fp);
      fclose(Fp);

      MainRegistry->ProfileGetString(DbInfoSection, "Maintainer.Name", NulString, &DbMaintainerName);
      MainRegistry->ProfileGetString(DbInfoSection, "Maintainer.Email", NulString, &DbMaintainerMail);

      STRLIST Remarks;
      MainRegistry->ProfileGetString(DbInfoSection, "Comments", &Remarks);
      Remarks.Join(' ', &DbComments);

      MainRegistry->ProfileGetString(DbInfoSection, "Title", NulString, &DbTitle);
      // We support multiple doctypes
      MainRegistry->ProfileGetString(DbInfoSection, "DocTypes", NulString, &GlobalDocTypeList);

      // Stop list
      MainRegistry->ProfileGetString(DbInfoSection, "Stoplist", NulString, &StoplistFileName); 
      if (StoplistFileName.Equals("C") || StoplistFileName.Equals("<NULL>"))
	StoplistFileName.Clear();

      // This is the special domain of the VIDB code...
      MainRegistry->ProfileGetString(DbInfoSection, CollectionsEntry, &FilenameList);
      if (!FilenameList.IsEmpty())
	CollectDBs(&FilenameList, DbPathName);
      else
	MainRegistry->ProfileGetString(DbInfoSection, DatabasesEntry, &FilenameList);
  }

  // We have some augmented metadata encoded in XML
  if (XMLBuffer.GetLength())
    MainRegistry->AddFromSgmlBuffer(XMLBuffer);

  size_t DatabaseCount = FilenameList.GetTotalEntries();
  if (DatabaseCount == 0)
    {
      // Fetch the name of the list (if defined)
      STRLIST VirtualDbFnList;
      MainRegistry->ProfileGetString(DbInfoSection, DatabaseListFileEntry, 
	GetDbFileStem() + (STRING)DbExtVdb, // Default list
	&VirtualDbFnList);
      //
      // Build the "Raw" file list
      //
      for (const STRLIST *p = VirtualDbFnList.Next(); p != &VirtualDbFnList; p = p->Next())
	{
	  // Try to read the ".vdb" file if it exists
	  DatabaseCount += ReadFromFile(p->Value(), &FilenameList);
	}
      if (DatabaseCount)
	{
	  FilenameList.UniqueSort();
	  DatabaseCount = FilenameList.GetTotalEntries();
	}
      if (DatabaseCount == 0)
	{
	  //
	  // Boring old single database
	  //
	  DatabaseCount = 1;
	  FilenameList.Clear ();
	  FilenameList.AddEntry (NewFileName);
	}
    }

  logf (LOG_DEBUG, "Virtual list of %d indexes...", DatabaseCount);

  if (DatabaseCount > VolIndexCapacity)
    {
      logf (LOG_FATAL, "Virtual specified with %d databases: \
A virtual database may contain a MAX. of %u databases!", DatabaseCount, VolIndexCapacity);
      exit(0);
    }

  //
  // Build the list of pointers for IDBs and RSETs
  //
#ifdef _WIN32
  c_dblist = new IDB *[DatabaseCount+1]; // WIN32 WORKAROUND
#else
  c_dblist = new IDB *[DatabaseCount];
#endif
  for (size_t i = 0; i< DatabaseCount; i++) c_dblist[i] = NULL; // Zero

  //
  // Load each database
  //
  c_inconsistent_doctypes = GDT_FALSE;
  c_dbcount = 0;
  MainDfdt = NULL;

  for (const STRLIST *p=FilenameList.Next(); p != &FilenameList; p = p->Next())
    {
      if (p->Value().IsEmpty())
	continue;
      const STRING Fn = IsAbsoluteFilePath(p->Value()) ? p->Value() : DbPathName + p->Value();

      try {
	c_dblist[c_dbcount] = new IDB (this, Fn, NewDocTypeOptions, SearchOnly);
      } catch (...) {
	c_dblist[c_dbcount] = NULL;
      }
      if (c_dblist[c_dbcount] == NULL)
	{
	  logf (LOG_WARN, "Could not open database \"%s\"", Fn.c_str());
	  continue; // Not inited.
	}
      else if (!c_dblist[c_dbcount]->IsDbCompatible())
	{
	  logf (LOG_WARN, "Can't open database \"%s\": %s.",
		Fn.c_str(), c_dblist[c_dbcount]->ErrorMessage(), Fn.c_str());
	  delete c_dblist[c_dbcount];
	  c_dblist[c_dbcount] = NULL;
	  continue;
	}
      else if (c_dblist[c_dbcount]->IsEmpty())
	{
	  if (p->Value() != __IB_DefaultDbName)
	    {
	      logf (LOG_WARN, "Database \"%s\" is \"Empty\" or \"Undefined\"! Ignoring.", Fn.c_str());
	      delete c_dblist[c_dbcount];
	      c_dblist[c_dbcount] = NULL;
	      continue;
	    }
	}
      else
	{
	  logf (LOG_DEBUG, "Adding Index '%s' to virtual database.", Fn.c_str());
	  c_dblist[c_dbcount]->SetParent(this, c_dbcount);
#if USE_STD_MAP
	  Segments[c_dblist[c_dbcount]->GetSegmentName()] = c_dbcount;
#endif
	}
      //
      // It was claimed that each database MUST have the same doctype!
      // But this is does not seem to be the case. The result of mixing
      // different databases with differing global doctypes seems less
      // of a problem than mixing different doctypes in a database--- and
      // this only effects the field list in the CGI interface.
      //
      const DOCTYPE_ID TmpDoctype = c_dblist[c_dbcount]->GetGlobalDoctype ();
      if (TmpDoctype.IsDefined())
	{
	  const STRING doctype = TmpDoctype.DocumentType();
	  if (!doctype.IsEmpty() && !GlobalDocTypeList.SearchCase(doctype))
	    {
	      // Doctype not in list so add..
	      GlobalDocTypeList.AddEntry( doctype );
	    }
	}
      c_dbcount++;
    }

#if 1 /* DEBUG CODE?? */
  // I think we always want at least one DB? (11 March 2004) edz@nonmonotonic.com
  if (c_dbcount == 0 && SearchOnly == GDT_FALSE)
    {
      c_dblist[c_dbcount++] = new IDB (DbPathName+DbFileName, NewDocTypeOptions, SearchOnly);
    }
#endif

  if (c_dbcount > 1)
    {
      // Set Volume
      for (size_t i=0; i< c_dbcount; i++)
          c_dblist[i]->SetVolume(DbFileName, i+1);
    }

  INT Clip = 0;
  MainRegistry->ProfileGetString(DbInfoSection, "SearchCutoff", 0, &Clip);
  if (Clip) SetDbSearchCutoff(Clip);

  if (!GlobalDocType.IsEmpty())
    {
      // We support Multiple Global Doctypes..
      GlobalDocTypeList.Join(',', &GlobalDocType); 
    }
  if (c_dbcount == 1)
    {
      if (DbTitle.IsEmpty())
	DbTitle = c_dblist[0]->GetTitle();
      if (DbMaintainerName.IsEmpty() && DbMaintainerMail.IsEmpty())
	c_dblist[0]->GetMaintainer(&DbMaintainerName, &DbMaintainerMail);
      if (DbComments.IsEmpty())
	DbComments = c_dblist[0]->GetComments();
    }

  return GDT_TRUE;
}

// 1st Queue
GDT_BOOLEAN  VIDB::AddRecord(const RECORD& newRecord)
{
  int Segment = newRecord.GetSegment();
  if ((size_t)Segment > c_dbcount || Segment < 0)
   Segment = 0;
  if (Segment) c_index[Segment] = 1;
  return c_dblist[Segment]->AddRecord(newRecord);
}

// 2nd Queue
void  VIDB::DocTypeAddRecord(const RECORD& newRecord)
{
  const int Segment = newRecord.GetSegment();
  if ((size_t)Segment < c_dbcount && Segment >= 0)
    {
      c_index[Segment] = 1;
      c_dblist[Segment]->DocTypeAddRecord(newRecord);
    }
}

GDT_BOOLEAN  VIDB::Index(GDT_BOOLEAN newIndex)
{
  GDT_BOOLEAN result = GDT_TRUE;

  for (size_t i=0; i< c_dbcount; i++)
    if (c_index[i]) result &= c_dblist[i]->Index(newIndex);

  return result;
}


void VIDB::SetFindConcatWords(GDT_BOOLEAN Set)
{
  for (size_t i=0; i<c_dbcount; i++)
    c_dblist[i]->SetFindConcatWords(Set);
}
GDT_BOOLEAN VIDB::GetFindConcatWords() const
{
  GDT_BOOLEAN set = GDT_FALSE;
  for (size_t i=0; i<c_dbcount && !set; i++)
    set = (set || c_dblist[i]->GetFindConcatWords()); 
  return set;
}


const STRLIST& VIDB::GetDocTypeOptions() const
{
  return c_dbcount ? c_dblist[0]->GetDocTypeOptions() : NulStrlist;
}

const STRLIST *VIDB::GetDocTypeOptionsPtr() const
{
  return c_dbcount ? c_dblist[0]->GetDocTypeOptionsPtr() : NULL;
}

enum DbState VIDB::GetDbState() const
{
  enum DbState DbState = DbStateReady;
  for (size_t i = 0; i < c_dbcount; i++)
    {
      if ((DbState = c_dblist[i]->GetDbState()) != DbStateReady)
	break;
    }
  return DbState;
}

GDT_BOOLEAN VIDB::SetDateRange(const DATERANGE& DateRange)
{
  GDT_BOOLEAN res = GDT_TRUE;
  for (size_t i = 0; i < c_dbcount; i++)
    res =  res && c_dblist[i]->SetDateRange(DateRange);
  return res;
}

GDT_BOOLEAN VIDB::SetDateRange(const SRCH_DATE& From, const SRCH_DATE& To)
{
  GDT_BOOLEAN res = GDT_TRUE;
  for (size_t i = 0; i < c_dbcount; i++)
    res = res && c_dblist[i]->SetDateRange(From, To);
  return res;
}

GDT_BOOLEAN VIDB::GetDateRange(DATERANGE *DateRange) const
{
  if (c_dbcount) return c_dblist[0]->GetDateRange(DateRange);
  return GDT_FALSE;
}


STRING VIDB::Description() const
{
  STRING  result;

  result << "Virtual level <database>.ini Options:\n";
  result << "[" << DbInfoSection << "]\n";
  result << CollectionsEntry << "=<List of virtual databases>\n";
  result << DatabasesEntry << "=<List of physical databases>\n";
  result << DatabaseListFileEntry << "=<Path to file list> (default: <database>"+ (STRING)DbExtVdb;
  result << ") # File has 1 entry per line\n\n";
#if 1
  if (c_dbcount > 0)
    {
       result <<  c_dblist[0]->Description();
    }
  else
    {
       IDB *pdb = new IDB();
       result << pdb->Description();
       delete pdb;
    }
#else
  result << (c_dbcount > 0 ? c_dblist[0]->Description() : IDB(GDT_TRUE).Description());
#endif
  result << "\n\n";

  return result;
}


void VIDB::ProfileGetString(const STRING& Section, const STRING& Entry,
        const STRING& Default, PSTRING StringBuffer) const
{
  if (MainRegistry) MainRegistry->ProfileGetString(Section, Entry, Default, StringBuffer);
  else if (StringBuffer) StringBuffer->Clear();
}

int VIDB::GetErrorCode() const
{
  int errcode = 0;
  for (size_t i = 0; i < c_dbcount && errcode == 0; i++)
    {
      errcode = c_dblist[i]->GetErrorCode();
    }
  return errcode;
}


int VIDB::GetErrorCode(const INT Idx) const
{
  if (Idx > 0 && Idx <= (int)c_dbcount)
    return c_dblist[Idx-1]->GetErrorCode();
  return GetErrorCode();
}


const char * VIDB::ErrorMessage() const
{
  return c_dbcount ? c_dblist[0]->ErrorMessage( GetErrorCode()) : "No DB defined";
}

const char *VIDB::ErrorMessage(const INT Idx) const
{
  if (Idx > 0 && Idx <= (int)c_dbcount)
   return c_dblist[Idx-1]->ErrorMessage();
  return "DB Index out of range";
}

GDT_BOOLEAN VIDB::Close()
{
  if (MainDfdt == NULL && MainRegistry == NULL && c_dbcount > 0)
    {
      return Opened == GDT_FALSE;
    }

  if (MainDfdt)
   {
     delete MainDfdt;
     MainDfdt = NULL;
   }
  while (c_dbcount-- > 0)
   {
     if (c_dblist[c_dbcount])
      {
	delete c_dblist[c_dbcount];
	c_dblist[c_dbcount] = NULL;
      }
   }
  if (c_dblist)
    {
      delete [] c_dblist; // NEEDED???
      c_dblist = NULL;
    }
  if (c_rsetlist)
    {
      delete [] c_rsetlist; // NEEDED???
      c_irsetlist = NULL;
    }
  if (c_irsetlist)
    {
      delete [] c_irsetlist; // NEEDED????
      c_irsetlist = NULL;
    }
  if (MainRegistry)
    {
      delete MainRegistry;
      MainRegistry = NULL;
    }
  Opened = GDT_FALSE;
  return GDT_TRUE;
}


//
// Delete all databases
//
VIDB::~VIDB ()
{
  Close();
}


size_t VIDB::GetTotalDatabases () const
{
  return  (c_dbcount);
}

void VIDB::GetGlobalDocType (PSTRING StringBuffer) const
{
  *StringBuffer = GlobalDocType;
}

//
// Set debugging mode for all databases
//
void VIDB::SetDebugMode (GDT_BOOLEAN OnOff)
{
  for (size_t i = 0; i < c_dbcount; i++)
    c_dblist[i]->SetDebugMode (OnOff);
}

// Get an IDB pointer..
IDBOBJ *VIDB::GetIDB(size_t idx) const
{
  if (idx == 0 && c_dbcount <= 1)
     return c_dblist[0];
  if (idx > 0 && idx <= c_dbcount)
    return c_dblist[idx-1];
  return NULL; // Not found
}


//
// Check each database for compatibility problems
//
GDT_BOOLEAN VIDB::IsDbCompatible () const
{
  //
  // See the Open() method for info on this
  //
#if 0
  if (c_inconsistent_doctypes)
    return GDT_FALSE;
#endif

  for (size_t i = 0; i < c_dbcount; i++)
    {
      if (c_dblist[i]->IsDbCompatible () == GDT_FALSE)
	{
	  return GDT_FALSE;
	}
    }
  return GDT_TRUE;
}


GDT_BOOLEAN VIDB::IsEmpty() const
{
  for (size_t i = 0; i < c_dbcount; i++)
    {
      if (!c_dblist[i]->IsEmpty ())
	return GDT_FALSE;
    }
  return GDT_TRUE;
}


int VIDB::BitVersion() const
{
  int bits = sizeof(GPTYPE)*8;
  if (c_dbcount)
    bits = c_dblist[0]->BitVersion();
  for (size_t i = 1; i < c_dbcount; i++)
    {
      if (c_dblist[i]->BitVersion() != bits)
        return 0;
    }
  return bits;
}



PMDT VIDB::GetMainMdt(INT Idx) const
{
  if (c_dbcount)
    {
      if (Idx == 0 && c_dbcount <= 1)
	return c_dblist[0]->GetMainMdt(); // Only one
      if (Idx > 0 && (size_t)Idx <= c_dbcount)
	return c_dblist[Idx-1]->GetMainMdt();
    }
  return NULL; // Error
}

INDEX *VIDB::GetMainIndex(INT Idx) const
{
  if (c_dbcount)
    {
      if (Idx == 0 && c_dbcount <= 1)
	return c_dblist[0]->GetMainIndex(); // Only one
      if (Idx > 0 && (size_t)Idx <= c_dbcount)
	return c_dblist[Idx-1]->GetMainIndex();
    }
  return NULL; // Error
}


FCACHE *VIDB::GetFieldCache(INT Idx)
{
  if (c_dbcount)
    {
      if (Idx == 0 && c_dbcount <= 1)
        return c_dblist[0]->GetFieldCache(); // Only one
      if (Idx > 0 && (size_t)Idx <= c_dbcount)
        return c_dblist[Idx-1]->GetFieldCache();
    }
  return NULL; // Error
}




//
//
GDT_BOOLEAN VIDB::GetDocumentInfo (const INT Idx, const INT Index, PRECORD RecordBuffer) const
{
  if (Idx == 0) 
    return GetDocumentInfo (Index, RecordBuffer); 
  else if (Idx > 0 && (size_t)Idx <= c_dbcount)
    return c_dblist[Idx-1]->GetDocumentInfo (Index, RecordBuffer);
  return GDT_FALSE;
}

PDOCTYPE VIDB::GetDocTypePtr(const DOCTYPE_ID& DocType) const
{
  return c_dbcount ? c_dblist[0]->GetDocTypePtr(DocType) : NULL;
}


//
//
GDT_BOOLEAN VIDB::GetDocumentInfo (const INT Index, PRECORD RecordBuffer) const
{
  GDT_BOOLEAN res = GDT_FALSE;
  INT RealIndex = Index;

  for (size_t i = 0; i < c_dbcount; i++)
    {
      size_t Total = c_dblist[i]->GetTotalRecords ();
      if ((size_t)RealIndex <= Total)
	{
	  res = c_dblist[i]->GetDocumentInfo (RealIndex, RecordBuffer);
	  break;
	}
      RealIndex -= Total; // Next block
    }
  return res;
}


// 
// Total (including "deleted" records)
//
// Total searchable = GetTotalRecords() - GetTotalDocumentsDeleted()
//
size_t VIDB::GetTotalRecords (const INT Idx) const
{
  if (Idx > 0 && (size_t)Idx <= c_dbcount && c_dbcount)
    return c_dblist[Idx-1]->GetTotalRecords ();
  return GetTotalRecords();
}

size_t VIDB::GetTotalRecords () const
{
  size_t TotalRecords = 0;
  for (size_t i = 0; i < c_dbcount; i++)
    {
      TotalRecords += c_dblist[i]->GetTotalRecords ();
    }
  return TotalRecords;
}

off_t VIDB::GetTotalWords (const INT Idx) const
{
  if (Idx <= 0)    
    return GetTotalWords();
  if ((int)Idx <= (int)c_dbcount)
    return c_dblist[Idx-1]->GetTotalWords ();
  return 0;
}


off_t VIDB::GetTotalWords() const
{
  off_t Total = 0;
  for (size_t i = 0; i < c_dbcount; i++)
    {
      Total += c_dblist[i]->GetTotalWords ();
    }
  return Total;
}

off_t VIDB::GetTotalUniqueWords (const INT Idx) const
{
  if (Idx <= 0)
    return GetTotalUniqueWords();
  if ((int)Idx <= (int)c_dbcount)
    return c_dblist[Idx-1]->GetTotalUniqueWords ();
  return 0;
}

off_t VIDB::GetTotalUniqueWords() const
{
  off_t Total = 0;
  for (size_t i = 0; i < c_dbcount; i++)
    Total += c_dblist[i]->GetTotalUniqueWords ();
  return Total;
}


// Total "deleted" records
size_t VIDB::GetTotalDocumentsDeleted(const INT Idx) const
{
  if (Idx > 0 && (size_t)Idx <= c_dbcount)
    return c_dblist[Idx-1]->GetTotalDocumentsDeleted ();
  return GetTotalDocumentsDeleted();
} 

size_t VIDB::GetTotalDocumentsDeleted() const 
{
  size_t TotalRecords = 0;
  for (size_t i = 0; i < c_dbcount; i++)
    {
      TotalRecords += c_dblist[i]->GetTotalDocumentsDeleted ();
    }
  return TotalRecords;
}


GDT_BOOLEAN VIDB::SetLocale (const CHR *LocaleName) const
{
  if (LocaleName && *LocaleName)
    {
#ifdef HAVE_LOCALE
      PCHR oldLocale = setlocale (LC_ALL, NULL);
      if (StrCaseCmp (oldLocale, LocaleName) != 0)
	{
	  if (setlocale (LC_ALL, LocaleName) == NULL)
	    {
	      setlocale (LC_CTYPE, oldLocale);
	    }
	  else
	    {
	      static STRING x;
	      x = "LC_ALL=";
	      x += LocaleName;
	      putenv ((char *)x.c_str());
	    }
	}
#endif
      LOCALE myLocale (LocaleName);

      if (!SetGlobalCharset(myLocale.GetCharsetId()))
	logf(LOG_ERROR, "Could set set character set '%s'", myLocale.GetCharsetName());
      else
	return GDT_TRUE;
    }
  return GDT_FALSE;
}

size_t VIDB::Scan(SCANLIST *ListPtr, const STRING& Field, const size_t Position, const INT TotalTermsRequested) const
{
  ListPtr->Clear();
  if (c_dbcount)
    c_dblist[0]->Scan (ListPtr, Field, Position, TotalTermsRequested);
  else
    return 0; // No DB so no entries!
  if (c_dbcount > 1)
    {
      SCANLIST Scanlist;
      const size_t fudge = (Position > 50 ? 50 : Position);
      for (size_t i = 1; i < c_dbcount; i++)
        {
          if (TotalTermsRequested > 0)
	    c_dblist[i]->Scan (&Scanlist, Field, Position, TotalTermsRequested);
          else
	    c_dblist[i]->Scan (&Scanlist, Field, Position - fudge, TotalTermsRequested + fudge);
          ListPtr->Add (&Scanlist);
        }
    }
  return ListPtr->GetTotalEntries();
}

size_t VIDB::Scan(PSTRLIST ListPtr, const STRING& Field,
	const size_t Position, const INT TotalTermsRequested) const
{
  ListPtr->Clear();
  if (c_dbcount == 1)
    {
      c_dblist[0]->Scan (ListPtr, Field, Position, TotalTermsRequested);
    }
  else if (c_dbcount)
    {
      STRLIST Strlist;
      const size_t fudge = (Position > 50 ? 50 : Position);
      for (size_t i = 0; i < c_dbcount; i++)
        {
	  if (TotalTermsRequested > 0)
	   c_dblist[i]->Scan (&Strlist, Field, Position, TotalTermsRequested);
	  else
            c_dblist[i]->Scan (&Strlist, Field, Position - fudge, TotalTermsRequested + fudge);
          ListPtr->Cat (Strlist);
        }
      // Want only the unique terms
      if (TotalTermsRequested > 0)
	ListPtr->UniqueSort(fudge, TotalTermsRequested);
      else
	ListPtr->UniqueSort();
   }
  return ListPtr->GetTotalEntries();
}

size_t VIDB::Scan(SCANLIST *ListPtr, const STRING& Field,
	const STRING& Term, const INT TotalTermsRequested) const
{
  ListPtr->Clear();
  if (c_dbcount)
    {
      c_dblist[0]->Scan (ListPtr, Field, Term, TotalTermsRequested);
    }
  else
    return 0; // No DB so no entries!
  if (c_dbcount > 1)
    {
      SCANLIST ScanList;
      for (size_t i = 1; i < c_dbcount; i++)
        {
          c_dblist[i]->Scan (&ScanList, Field, Term, TotalTermsRequested);
          ListPtr->Add (&ScanList);
        }
    }
  return ListPtr->GetTotalEntries();
}

size_t VIDB::Scan(PSTRLIST ListPtr, const STRING& Field,
        const STRING& Term, const INT TotalTermsRequested) const
{
  ListPtr->Clear();
  if (c_dbcount == 1)
    {
      c_dblist[0]->Scan (ListPtr, Field, Term, TotalTermsRequested);
    }
  else if (c_dbcount)
    {
      STRLIST Strlist;
      for (size_t i = 0; i < c_dbcount; i++)
        {
          c_dblist[i]->Scan (&Strlist, Field, Term, TotalTermsRequested);
          ListPtr->Cat (Strlist);
        }
      // Want only the unique terms
      ListPtr->UniqueSort(0, TotalTermsRequested);
    }
  return ListPtr->GetTotalEntries();
}


size_t VIDB::ScanGlob(PSTRLIST ListPtr, const STRING& Field, const STRING& Term,
   const INT TotalTermsRequested) const
{
  ListPtr->Clear();
  if (c_dbcount == 1)
    {
      c_dblist[0]->ScanGlob (ListPtr, Field, Term, TotalTermsRequested);
    }
  else if (c_dbcount)
    {
      STRLIST Strlist;
      for (size_t i = 0; i < c_dbcount; i++)
        {
          c_dblist[i]->Scan (&Strlist, Field, Term, TotalTermsRequested);
          ListPtr->Cat (Strlist);
        }
      ListPtr->UniqueSort(); // Want only the unique terms
    }
  return ListPtr->GetTotalEntries();
}

// Scan for field contents according to a search
size_t VIDB::ScanSearch(SCANLIST *ListPtr, const QUERY& SearchQuery, const STRING& Fieldname,
         size_t MaxRecordsThreshold, GDT_BOOLEAN Cat)
{
  if (ListPtr)
    {
      SCANLIST Scanlist = ScanSearch (SearchQuery, Fieldname, MaxRecordsThreshold);
      if (Cat)
	ListPtr->AddEntry (Scanlist);
      else
	*ListPtr = Scanlist;
      return Scanlist.GetTotalEntries();
    }
  logf (LOG_PANIC, "ScanSearch passed a NULL pointer as list?");
  return 0; // Can't
}


SCANLIST VIDB::ScanSearch(const QUERY& SearchQuery, const STRING& Fieldname, size_t MaxRecordsThreshold)
{
  SCANLIST Scanlist;
  if (c_dbcount == 1)
    {
      Scanlist = c_dblist[0]->ScanSearch (SearchQuery, Fieldname, MaxRecordsThreshold);
    }
  else if (c_dbcount)
    {
      for (size_t i = 0; i < c_dbcount; i++)
        Scanlist.AddEntry (c_dblist[i]->ScanSearch (SearchQuery, Fieldname, MaxRecordsThreshold));
    }
  return Scanlist;
}



PRSET  VIDB::VSearchRpn(const STRING& QueryString, enum SortBy Sort, size_t Total,
	size_t *TotalFound, enum NormalizationMethods Method)
{
  SQUERY squery;
  if (!squery.SetRpnTerm (QueryString))
   {
     logf (LOG_NOTICE|LOG_ERROR, "Bad RPN Query: %s", squery.LastErrorMessage().c_str());
     if (TotalFound) *TotalFound = 0;
     return NULL;
   }
  return VSearch(squery, Sort, Total, TotalFound, Method);
}

PRSET  VIDB::VSearchInfix(const STRING& QueryString, enum SortBy Sort, size_t Total,
	size_t *TotalFound, enum NormalizationMethods Method)
{
  SQUERY squery;
  if (!squery.SetInfixTerm (QueryString))
   {
     logf (LOG_NOTICE|LOG_ERROR, "Bad Infix Query: %s", squery.LastErrorMessage().c_str());
     if (TotalFound) *TotalFound = 0;
     return NULL;
   }
  return VSearch(squery, Sort, Total, TotalFound, Method);
}


PRSET  VIDB::VSearchWords(const STRING& QueryString, enum SortBy Sort, size_t Total,
	size_t *TotalFound, enum NormalizationMethods Method)
{
  SQUERY squery;
  squery.SetWords (QueryString);
  return VSearch(squery, Sort, Total, TotalFound, Method);
}


PRSET VIDB::VSearchSmart(const STRING& Sentence, const STRING& DefaultField,
        enum SortBy Sort, size_t Total, size_t *TotalFound,
        enum NormalizationMethods Method, SQUERY* Query)
{
  IRSET *pIrset = SearchSmart(Sentence, DefaultField, Sort, Method, Query);
  RSET  *pRset  = NULL;
  if (pIrset)
    {
      if (TotalFound) *TotalFound = pIrset->GetTotalEntries();
      pRset = pIrset->GetRset(Total);
      delete pIrset;
    }
  return pRset;
}

PRSET VIDB::VSearchSmart(QUERY *Query, const STRING& DefaultField)
{
  return Query ? VSearchSmart(*Query, DefaultField, &(Query->Squery)) : NULL;
}


PRSET VIDB::VSearchSmart(const QUERY& Query, const STRING& DefaultField, SQUERY *SqueryPtr)
{
  IRSET *pIrset = SearchSmart(Query, DefaultField, SqueryPtr);
  RSET  *pRset  = NULL;
  if (pIrset)
    {
      pRset = pIrset->GetRset(Query.MaxResults);
      delete pIrset;
    }
  return pRset;
}

PRSET VIDB::VSearchSmart(const SQUERY& Query, const STRING& DefaultField,
        enum SortBy Sort, size_t Total, size_t *TotalFound,
        enum NormalizationMethods Method, SQUERY* QueryPtr)
{
  IRSET *pIrset = SearchSmart(Query, DefaultField, Sort, Method, QueryPtr);
  RSET  *pRset  = NULL;
  if (pIrset)
    {
      if (TotalFound) *TotalFound = pIrset->GetTotalEntries();
      pRset = pIrset->GetRset(Total);
      delete pIrset;
    }
  return pRset;
}


//
// Search on all databases and merge all results into a single
// result set.  We then call a sort method.
//
// Combine IRSETs, Sort, break top 300 into IRSETs, Get RSETs
// and combine...
PRSET VIDB::VSearch (const QUERY& Query)
{
  QUERY SearchQuery(Query);
  const enum SortBy Sort = Query.Sort;
  size_t            TotalFound = 0;
  size_t            Total = Query.GetMaximumResults();

  if (c_dbcount > 1)
     SearchQuery.Sort = Unsorted; // Unsorted for now

  if (c_irsetlist == NULL) c_irsetlist = new PIRSET[c_dbcount];
  if (c_rsetlist  == NULL) c_rsetlist  = new PRSET[c_dbcount];

  //
  // Search each database
  //
  register size_t i;
  GDT_BOOLEAN QueryError = GDT_TRUE;

  GDT_BOOLEAN Not_Seen = GDT_TRUE;
  int         hit_set  = 0;

  for (i = 0; i < c_dbcount; i++)
    {
      if ((c_irsetlist[i] = c_dblist[i]->Search (SearchQuery)) != NULL)
	{
	  QueryError = GDT_FALSE;
	  if (Not_Seen)
	    {
	      if (c_irsetlist[i]->GetTotalEntries () > 0)
		{
		  hit_set = i + 1;
		  Not_Seen = GDT_FALSE;
		}
	      else // No hits
		{
		  delete c_irsetlist[i];
		  c_irsetlist[i] = NULL;
		}
	    }
	  else if (hit_set > 0 && c_irsetlist[i]->GetTotalEntries ())
	    hit_set = -1;

	}
    }
  if (i == 0)
    logf (LOG_ERROR, "No database is opened for search!");
  if (QueryError)
    {
      logf (LOG_DEBUG, "VIDB::VSearch() returning undefined result set (NULL)");
      return NULL; // Undefined result set
    }

  // If only 1 database or result save some effort...
  if (hit_set > 0)
    {
      const size_t y = c_irsetlist[ hit_set-1 ]->GetTotalEntries ();
      TotalFound = y;

      c_irsetlist[ hit_set-1 ]->SortBy (Sort); // Make sure its sorted
      const PRSET Prset = c_irsetlist[ hit_set-1 ]->GetRset ( Total );

      for (i = 0; i < c_dbcount; i++)
	{
	  if (c_irsetlist[i])
	    {
	      delete c_irsetlist[i]; // Now dispose..
	      c_irsetlist[i] = NULL;
	    }
	}
      return Prset;
    }
  else if (c_dbcount <= 1)
    {
      if ( c_dblist[i-1]->GetErrorCode() > 0)
	logf (LOG_INFO, "Search Failure: %s",  c_dblist[i-1]->ErrorMessage() );
      return new RSET(); // Nothing found
    }

  // Combine several sets into 1 and clip the rest ...  
  size_t HitCount = 0;
  // Combine
  PIRSET NewIrset = new IRSET (c_dblist[i]);


  if ( Sort == Unsorted)
    {
      size_t got = 0;
      for (i = 0; i < c_dbcount; i++)
	{
	  const size_t y = c_irsetlist[i]->GetTotalEntries ();
	  IRESULT iresult;
	  TotalFound += y;
	  for (size_t x = 1; x <= y && got <= Total; x++)
	    {
	      c_irsetlist[i]->GetEntry (x, &iresult);
	      NewIrset->FastAddEntry (iresult);
	    }
	  c_irsetlist[i]->SetTotalEntries ( 0 ); // Now clear it
	}
    }
  else for (i = 0; i < c_dbcount; i++)
    {
      if (c_irsetlist[i])
	{
	  const size_t y = c_irsetlist[i]->GetTotalEntries ();
	  IRESULT iresult;
	  TotalFound += y;
	  HitCount += c_irsetlist[i]->GetHitTotal ();
	  // Only need to copy at most Total entries
	  for (size_t x = 1; x <= y && x <= Total; x++)
	    {
	      c_irsetlist[i]->GetEntry (x, &iresult);
	      NewIrset->FastAddEntry (iresult);
	    }
	  c_irsetlist[i]->SetTotalEntries ( 0 ); // Now clear it
	}
    }
  // Sort
  const size_t TotalResults = NewIrset->GetTotalEntries();
  NewIrset->SortBy(Sort);

  IRESULT iresult;
  for (i=1; (i <= TotalResults) && (i <= Total); i++)
    {
      NewIrset->GetEntry (i, &iresult);
      c_irsetlist[ iresult.GetVirtualIndex() - 1 ]->FastAddEntry (iresult);
    }

  // Destruct
  for (i = 0; i < c_dbcount; i++)
    {
      c_rsetlist[i] = c_irsetlist[i]-> GetTotalEntries ()  ? c_irsetlist[i]->GetRset() : NULL;
      delete c_irsetlist[i];
      c_irsetlist[i] = NULL;
    }

  // search could be truncated here.  Add method to RSET that shows how many 
  // term occurances were found for the query (NOT documents or entries!)
  // this would prevent really huge, meaningless result sets for virtual
  // databases (JMF)
  //
  // Combine the results into a single result set
  //
  PRSET rset = new RSET ( TotalResults >=Total ? Total : TotalResults );

  RESULT ResultRecord;
  for (i = 0; i < c_dbcount; i++)
    {
      if (c_rsetlist[i] == NULL)
	{
	  continue;
	}
      const size_t entry_count = c_rsetlist[i]->GetTotalEntries ();
      for (size_t j = 1; j <= entry_count; j++)
	{
	  if (c_rsetlist[i]->GetEntry (j, &ResultRecord))
	    {
	      rset->AddEntry (ResultRecord);
	    }
	}		/* for () */
//    HitCount += c_rsetlist[i]->GetHitTotal ();
    }		/* for() */

  rset->SetHitTotal (HitCount);
  rset->SortBy(Sort);

  rset->SetTotalFound(TotalFound);

  //
  // Delete the old, uncombined result sets
  //      
  for (i = 0; i < c_dbcount; i++)
    {
      if (c_rsetlist[i])
	delete c_rsetlist[i];
    }
  return rset;
}


PIRSET VIDB::Search(const SQUERY& SearchQuery, enum SortBy Sort, enum NormalizationMethods Method,
  VIDB_STATS *Stats)
{
  QUERY Query;

  Query.Squery = SearchQuery;
  Query.Sort   = Sort;
  Query.Method = Method;
  return Search(Query, Stats);
}


//
// Search on all databases and merge all results into a single
// result set.  We then call a sort method.
//
// Combine IRSETs, Sort, break top 300 into IRSETs, Get RSETs
// and combine...
PIRSET VIDB::Search(const QUERY& Query, VIDB_STATS *Stats)
{
  if (!Opened || c_dbcount == 0)
    return NULL;

  // If we have more than 1 database, don't sort untill we've finished since we'll
  // need to sort the combined sets anyway (and sorting a set of unsorted sets is
  // typically faster than trying to sort each set and then sorting the sets together).
  QUERY myQuery(Query);
  if (c_dbcount > 1) myQuery.Sort = Unsorted;

  if (c_irsetlist == NULL) c_irsetlist = new PIRSET[c_dbcount];
  if (c_rsetlist  == NULL) c_rsetlist  = new PRSET[c_dbcount];

  //
  // Search each database
  //
  register size_t i;
  GDT_BOOLEAN QueryError = GDT_TRUE;
  size_t TotalEntries = 0; 

  GDT_BOOLEAN Not_Seen = GDT_TRUE;
  int         hit_set  = 0;
  int         smallest_hit_set_idx = -1;
  size_t      smallest_hit_set_hits = 0;

  if (Stats) Stats->Clear();

#ifdef MULTITHREADED
  sem_t      semaphores[c_dbcount]; /* semaphores */
  pthread_t  idthreads[c_dbcount]; /* ids of threads */
#endif

  for (i = 0; i < c_dbcount; i++)
    {
      // TODO: Run in a thread
#ifdef MULTITHREADED
      // NOT YET!
      if ((sem_init(& semaphores[i], 0, 0) < 0 ) ||
	 (pthread_create(& idthreads[i], NULL, c_dblist[i]->Search, (void *)&myQuery) != 0))
	{
	  logf (LOG_ERRNO, "VIDB:: Could not run threaded!");
	  c_irsetlist[i] = c_dblist[i]->Search (myQuery);
	}
#else
      c_irsetlist[i] = c_dblist[i]->Search (myQuery);
#endif
      if (c_irsetlist[i] != NULL)
	{
	  size_t hits = c_irsetlist[i]->GetTotalEntries ();
	  if (Stats)
	    {
	      Stats->SetTotal(i, hits);
	      Stats->SetName(i, c_dblist[i]->GetSegmentName());
	    }

	   // Set smallest set
	  if ((smallest_hit_set_idx < 0) || (hits < smallest_hit_set_hits))
	    {
	      smallest_hit_set_hits = hits;
	      smallest_hit_set_idx  = (int)i;
	    }

	  QueryError = GDT_FALSE;
	  if (Not_Seen)
	    {
	      if (hits > 0)
		{
		  hit_set = i + 1;
		  Not_Seen = GDT_FALSE;
		}
	    }
	  else if (hit_set > 0 && hits > 0)
	    hit_set = -1;
	  TotalEntries += hits;
	}
    }
  // TODO: Wait untill the threads are finished

  if (i == 0)
    logf (LOG_ERROR, "No database is opened for search!");

  if (QueryError)
    {
      logf (LOG_DEBUG, "VIDB::Search() returning undefined result set (NULL)");
      return NULL; // Undefined result set
    }
  // If only 1 database or result save some effort...
  if (hit_set > 0)
    {
      logf(LOG_DEBUG, "Only one hit set (%d)", hit_set);
      // delete what we don't need...
      for (i=0; i < c_dbcount; i++)
	{
	  if ((int)i != hit_set-1 && c_irsetlist[i])
	    {
	      delete c_irsetlist[i];
	      c_irsetlist[i] = NULL;
	    }
	}
      PIRSET newIrset = c_irsetlist[ hit_set-1 ];
      newIrset->SortBy(Query.Sort);

      if (Stats) {
	 Stats->SetHits(i, newIrset->GetHitTotal());
	 // Want also here to set the Max Aux..
      }

      return newIrset; 
    }
  else if (c_dbcount <= 1)
    {
      if ( c_dblist[i-1]->GetErrorCode() > 0)
        logf (LOG_INFO, "Search Failure: %s",  c_dblist[i-1]->ErrorMessage() );
      // delete what we are done with
      for (i=0; i < c_dbcount; i++)
	{
	  if (c_irsetlist[i])
	    {
	      delete c_irsetlist[i];
	      c_irsetlist[i] = NULL;
	    }
	}
      // return an empty set...
      return new IRSET(c_dblist[0]); // Nothing found
    }

  // Combine multiple sets

  // Use the smallest set to start to minimize copying
  IRSET *NewIrset = c_irsetlist[smallest_hit_set_idx];
  NewIrset->Reserve(TotalEntries+1); // Resize for everything

  for (i = 0; i < c_dbcount; i++)
    {
      if (Stats) Stats->SetHits(i, c_irsetlist[i]->GetHitTotal());
      if ((int)i != smallest_hit_set_idx && c_irsetlist[i])
	{
	  logf(LOG_DEBUG, "Adding %d [Set #%d]", c_irsetlist[i]->GetTotalEntries(), i);
	  NewIrset->Concat (*(c_irsetlist[i]));
	  delete c_irsetlist[i];; // Done with it
	}
    }
  NewIrset->SortBy(Query.Sort);
  return NewIrset;
}

PIRSET  VIDB::SearchRpn(const STRING& QueryString, enum SortBy Sort, enum NormalizationMethods Method)
{
  SQUERY squery;
  if (!squery.SetRpnTerm (QueryString))
   {
     logf (LOG_NOTICE|LOG_ERROR, "Bad RPN Query: %s", squery.LastErrorMessage().c_str());
     return NULL;
   }
  return Search(squery, Sort, Method);
}

PIRSET  VIDB::SearchInfix(const STRING& QueryString, enum SortBy Sort, enum NormalizationMethods Method)
{
  SQUERY squery;
  if (!squery.SetInfixTerm (QueryString))
   {
     logf (LOG_NOTICE|LOG_ERROR, "Bad Infix Query: %s", squery.LastErrorMessage().c_str());
     return NULL;
   }
  return Search(squery, Sort, Method);
}


PIRSET  VIDB::SearchWords(const STRING& QueryString, enum SortBy Sort, enum NormalizationMethods Method)
{
  SQUERY squery;
  squery.SetWords (QueryString);
  return Search(squery, Sort, Method);
}



//
// The version MUST be the same for the entire collection so we
// only need to call this once.
//
STRING VIDB::GetVersionID () const
{
  return c_dbcount ? c_dblist[0]->GetVersionID () + "/Virtual" : STRING(__IB_Version);
}

// Only need to call this in one idb
const STRLIST& VIDB::GetAllDocTypes ()
{
  return c_dbcount ? c_dblist[0]->GetAllDocTypes () : NulStrlist;
}

// Only need to call this in one idb 
GDT_BOOLEAN VIDB::ValidateDocType(const STRING& DocType) const
{
  return c_dbcount ? c_dblist[0]->ValidateDocType(DocType) : GDT_FALSE;
}


STRING VIDB::GetTitle(const INT Idx) const
{
  if (Idx <= 0)
    return DbTitle;
  else if ((size_t)Idx <= c_dbcount)
    return c_dblist[Idx-1]->GetTitle ();
  return NulString;
}

STRING VIDB::GetComments(const INT Idx) const
{
  if (Idx <= 0)
    return DbComments;
  else if ((size_t)Idx <= c_dbcount)
    return c_dblist[Idx-1]->GetComments ();
  return NulString;
}

STRING VIDB::GetMaintainer(const INT Idx) const
{
  STRING Mailto, Name, Address;
  GetMaintainer(&Name, &Address, Idx);
  if (Name.GetLength() == 0)
    Name = Address;
  else if (Address.GetLength() == 0)
    Address = Name;
  if (Name.GetLength() == 0)
    {
      ProfileGetString(DbInfoSection, "Maintainer", NulString, &Mailto);
    }
  else
    Mailto << "<A HREF=\"mailto:" << Address << "?subject=" << DbFileName << "\">" << Name << "</A>";
  return Mailto;
}


void VIDB::GetMaintainer(PSTRING Name, PSTRING Address, const INT Idx) const
{
  if (Idx <= 0)
   {
      *Name = DbMaintainerName;
      *Address = DbMaintainerMail;
   }
  else if ((size_t)Idx <= c_dbcount)
   c_dblist[Idx-1]->GetMaintainer (Name, Address);
}

INT VIDB::GetLocks() const
{
  INT res = 0;
  for (size_t i = 0; i < c_dbcount; i++)
    res |= c_dblist[i]->GetLocks();
  return res;
}

// Need to find out in which DB the result record is!
size_t VIDB::VirtualSet (const RESULT& ResultRecord, RESULT *Result) const
{
  *Result = ResultRecord;
  size_t i = Result->GetVirtualIndex();
  if (i) return i;

  STRING RealKey;
  STRING Key = Result->GetKey (); // Get current "virtual" key
  if ((i = VirtualSet(Key, &RealKey)) != 0) // Get Virtual set and real key
    {
      Result->SetVirtualIndex(i);
      Result->SetKey (RealKey); // Set "real" key
    }
  else
    {
      // Try to find it?
      STRING DBname;
      while(i < c_dbcount)
	{
	  DBname = c_dblist[i++]-> GetDbFileStem();
	  RemovePath(&DBname); // Strip path
	  if (DBname == DbFileName)
	    {
	      Result->SetVirtualIndex(i);
	      return i; // ALready have the "real" key
	    }
	}
      return 0; // Not found 
    }
  return i;
}

// Need to find out in which DB the result record is!
// -- Encoded in Key (see (KEY) above)
size_t VIDB::VirtualSet (const STRING& Key, STRING *NewKey, char Ch) const
{
  size_t idx = 0;

  *NewKey = Key;
  STRINGINDEX pos = NewKey->Search (Ch);
  if (pos)
    {	
      if ((idx = NewKey->GetInt()) > 0)
	{
	  // Make sure in range
	  if (idx > c_dbcount)
	    idx = 0; // ERROR
	  else
	    NewKey->EraseBefore (pos + 1);
	}
      else if (c_dbcount == 1)
	idx = 1; 
      // else ERROR
   }
  else if (c_dbcount == 1)
    {
      // Only one set so can use it 
      idx = 1;
    }
  return idx;
}

void VIDB::SetStoplist(const STRING& Filename)
{
  StoplistFileName = Filename;
}

GDT_BOOLEAN VIDB::IsStopWord (const STRING& Word) const
{
#if 1
  return c_dbcount ? c_dblist[0]->IsStopWord(Word) : GDT_FALSE;
#else
//cerr << "VIDB::IsStopWord..." << endl;
  if (StoplistFileName.GetLength())
    {
      if (c_dbcount > 1)
	{
	  static STOPLIST StopWords;  // Cache
	  StopWords.Load (StoplistFileName);
	  return StopWords.InList (Word);
	}
       else
	c_dblist[0]->SetStoplist(StoplistFileName );
    }
  return c_dblist[0]->IsStopWord(Word);
#endif
}

void VIDB::SetPriorityFactor(DOUBLE x, size_t idx)
{
  if (idx > 0 && idx <= c_dbcount)
    {
      c_dblist[idx-1]->SetPriorityFactor(x);
    }
  else if (idx == 0)
    {
      for (size_t i=0; i < c_dbcount; i++)
	c_dblist[i]->SetPriorityFactor(x);
    }
}

void VIDB::SetDbSearchCutoff(size_t m)
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->SetDbSearchCutoff(m);
}

void VIDB::SetDbSearchCutoff(size_t m, size_t idx)
{
  if (idx > 0 && idx <= c_dbcount)
    c_dblist[idx-1]->SetDbSearchCutoff(m);
  else if (idx == 0)
    SetDbSearchCutoff(m);
}


void VIDB::SetDbSearchFuel(size_t Percent)
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->SetDbSearchFuel(Percent);
}

void VIDB::SetDbSearchFuel(size_t Percent, size_t idx)
{
  if (idx > 0 && idx <= c_dbcount)
    c_dblist[idx-1]->SetDbSearchFuel(Percent);
  else if (idx == 0)
    SetDbSearchFuel(Percent);
}

void VIDB::SetDbSearchCacheSize(size_t NewCacheSize)
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->SetDbSearchCacheSize(NewCacheSize);
}

void VIDB::SetDbSearchCacheSize(size_t NewCacheSize, size_t idx)
{
  if (idx > 0 && idx <= c_dbcount)
    c_dblist[idx-1]->SetDbSearchCacheSize(NewCacheSize);
  else if (idx == 0)
    SetDbSearchCacheSize(NewCacheSize);
}


void VIDB::SetCommonWordsThreshold(long x)
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->SetCommonWordsThreshold(x);
}

//
//
void VIDB::BeforeSearching (QUERY* QueryPtr)
{
  for (size_t i=0; i < c_dbcount; i++)
    {
      if (StoplistFileName.GetLength())
	c_dblist[i]->SetStoplist(StoplistFileName);
      c_dblist[i]->BeforeSearching(QueryPtr);
    }
}

IRSET *VIDB::AfterSearching  (IRSET* IrsetPtr)
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->AfterSearching(IrsetPtr);
  return IrsetPtr;
}


GDT_BOOLEAN VIDB::KillAll()
{
  GDT_BOOLEAN result = GDT_TRUE;
  for (size_t i=0; i < c_dbcount; i++)
    result &= c_dblist[i]->KillAll();
  return result;
}

GDT_BOOLEAN VIDB::KillAll(size_t idx)
{
  if (idx>=0 && idx < c_dbcount)
    return c_dblist[idx]->KillAll();
  return GDT_FALSE;
}


void VIDB::BeforeIndexing  ()
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->BeforeIndexing();
}

void VIDB::AfterIndexing   ()
{
  for (size_t i=0; i < c_dbcount; i++)
    c_dblist[i]->AfterIndexing();
}

void VIDB::BeginRsetPresent (const STRING& RecordSyntax)
{
  for (size_t i = 0; i < c_dbcount; i++)
    c_dblist[i]->BeginRsetPresent(RecordSyntax);
}

void VIDB::EndRsetPresent (const STRING& RecordSyntax)
{
  for (size_t i = 0; i < c_dbcount; i++)
    c_dblist[i]->EndRsetPresent(RecordSyntax);
}


GDT_BOOLEAN VIDB::GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRING StringBuffer, const DOCTYPE *DoctypePtr)
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->GetFieldData (Result, FieldName, StringBuffer, DoctypePtr);
  StringBuffer->Clear();
  return GDT_FALSE;
}

GDT_BOOLEAN VIDB::GetFieldData(const RESULT& ResultRecord, const STRING& FieldName,
	PSTRLIST StrlistBuffer, const DOCTYPE *DoctypePtr)
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->GetFieldData (Result, FieldName, StrlistBuffer, DoctypePtr);
  StrlistBuffer->Clear();
  return GDT_FALSE;
}


// Show Headline ("B")..
GDT_BOOLEAN VIDB::Headline(const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  if (RecordSyntax.IsEmpty())
    return Headline(ResultRecord, StringBuffer);

  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->Headline (Result, RecordSyntax, StringBuffer);
  StringBuffer->Clear();
  return GDT_FALSE;
}

// Show Headline ("B")..
GDT_BOOLEAN VIDB::Headline(const RESULT& ResultRecord, PSTRING StringBuffer) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->Headline (Result, StringBuffer);
  StringBuffer->Clear();
  return GDT_FALSE;
}

// Context Match
GDT_BOOLEAN VIDB::Context(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
	const STRING& Before, const STRING& After) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->Context (Result, Line, Term, Before, After);
  return GDT_FALSE;
}

GDT_BOOLEAN VIDB::NthContext(size_t N, const RESULT& ResultRecord, PSTRING Line, STRING *Term,
        const STRING& Before, const STRING& After) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->NthContext (N, Result, Line, Term, Before, After);
  return GDT_FALSE;
}


STRING VIDB::XMLHitTable(const RESULT& ResultRecord)
{
  RESULT Result;   
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->XMLHitTable (Result);
  return STRING("<XML>\n") + ResultRecord.XMLHitTable() + "</XML>\n";
}

GDT_BOOLEAN VIDB::XMLContext(const RESULT& ResultRecord, PSTRING Line, PSTRING Term,
        const STRING& Tag) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->XMLContext (Result, Line, Term, Tag);
  return GDT_FALSE;
}


GDT_BOOLEAN VIDB::Summary(const RESULT& ResultRecord,
	const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->Summary (Result, RecordSyntax, StringBuffer);
  return GDT_FALSE;
}

// Return URL to Source Document
GDT_BOOLEAN VIDB::URL(const RESULT& ResultRecord, PSTRING StringBuffer,
        GDT_BOOLEAN OnlyRemote) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->URL (Result, StringBuffer, OnlyRemote);
  return GDT_FALSE;
}


STRING VIDB::XMLNodeTree (const RESULT& ResultRecord, const FC& Fc) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->GetNodeTree (Fc).XMLNodeTree();
  return STRING("<XML>\n") + ResultRecord.XMLHitTable() + "</XML>\n";
}


STRING VIDB::GetXMLHighlightRecordFormat(const RESULT& ResultRecord,
	const STRING& PageField, const STRING& TagElement)
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->GetXMLHighlightRecordFormat(Result, PageField, TagElement);
  return "<!-- UNTAGED CONTENT -->\n";

  
}

  // Record Highlighting
void VIDB::HighlightedRecord(const RESULT& ResultRecord,
	const STRING& BeforeTerm, const STRING& AfterTerm, PSTRING StringBuffer) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    c_dblist[i-1]->HighlightedRecord (Result, BeforeTerm, AfterTerm, StringBuffer);
  else StringBuffer->Empty();
}

void VIDB::DocHighlight (const RESULT& ResultRecord, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    c_dblist[i-1]->DocHighlight (Result, RecordSyntax, StringBuffer);
  else StringBuffer->Empty();
}


void VIDB::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    c_dblist[i-1]->Present (Result, ElementSet, RecordSyntax, StringBuffer);
  else StringBuffer->Empty();
}

void VIDB::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 PSTRING StringBuffer) const
{
  STRING RecordSyntax;
  Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

void VIDB::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer,
	const QUERY& Query) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    c_dblist[i-1]->DocPresent (Result, ElementSet, RecordSyntax, StringBuffer, Query);
  else
    StringBuffer->Empty();
}


void VIDB::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer, QUERY());
}

void VIDB::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    PSTRING StringBuffer) const
{
  DocPresent (ResultRecord, ElementSet, NulString, StringBuffer, QUERY());
}

// These keys are Vol@Key
GDT_BOOLEAN VIDB::KeyLookup (const STRING& Key, PRESULT ResultBuffer) const
{
  STRING NewKey;
  const size_t i = VirtualSet (Key, &NewKey);
  if (i)
    {
      GDT_BOOLEAN res = c_dblist[i-1]->KeyLookup (NewKey, ResultBuffer);
      if (ResultBuffer) ResultBuffer->SetVirtualIndex(i);
      return res;
    }
  return GDT_FALSE;
}


PDFDT VIDB::GetDfdt (PDFDT DfdtBuffer, const RESULT *ResultPtr)
{
  if (ResultPtr && GetRecordDfdt(*ResultPtr, DfdtBuffer))
    return DfdtBuffer;
  if (MainDfdt == NULL)
    {
      MainDfdt = new DFDT();
      if (c_dbcount == 1)
	{
	  // Since only one can load...
	  c_dblist[0]->GetDfdt (MainDfdt);
	}
      else
        {
	  STRLIST FieldList;
	  STRING Field;
	  DFDT dfdt;
	  DFD dfd;

	  // Build a "merged" Dfdt
	  for (size_t i=0; i< c_dbcount ; i++)
	    {
	      c_dblist[i]->GetDfdt (&dfdt);
	      const size_t FieldCount = dfdt.GetTotalEntries ();
	      for (size_t j = 1; j <= FieldCount; j++)
		{
		  dfdt.GetEntry (j, &dfd);
		  dfd.GetFieldName (&Field);
		  if (i == 0 || FieldList.Search (Field) == 0)
		    {
		      // Add to Global DFDT
		      FieldList.AddEntry (Field);
		      MainDfdt->AddEntry(dfd);
		    }
                }
            }
        }
    }
  if (DfdtBuffer)
    *DfdtBuffer = *MainDfdt;
  return MainDfdt;
}

SRCH_DATE VIDB::DateCreated() const
{
  SRCH_DATE date;
  STRING created;
  ProfileGetString(DbInfoSection, "DateCreated", NulString, &created);
  if (created.IsEmpty())
    {
      SRCH_DATE d;
      // Find the oldest database
      for (size_t i = 0; i < c_dbcount; i++)
	{
	  if ((d = c_dblist[i]-> DateCreated()).Ok())
	    {
	      if (d < date)
		date = d;
	    }
        }
      // Make sure its created BEFORE it was last modified
      if ((d = DateLastModified()).Ok())
	{
	  if (!date.Ok() || d < date)
	    date = d;
	}
    }
  else
    date.Set(created);
  return date;
}


SRCH_DATE VIDB::DateLastModified() const
{
  SRCH_DATE dateMod;

  if (MainRegistry)
    {
      STRING cdat;
      MainRegistry->ProfileGetString(DbInfoSection, "DateLastModified", NulString, &cdat);
      dateMod.Set(cdat);
    }

  for (size_t i = 0; i < c_dbcount; i++)
    {
      SRCH_DATE d ( c_dblist[i]-> DateLastModified() );
      if (d.Ok())
	{
	  if (!dateMod.Ok() || d> dateMod)
	    dateMod = d;
	}
    }
  return dateMod;
}

STRING VIDB::GetDbFileStem() const
{
  return DbPathName + DbFileName;
}

STRING VIDB::GetDbFileStem(const INT Idx) const
{
  if (Idx > 0 && (size_t)Idx <= c_dbcount )
    return c_dblist[Idx-1]->GetDbFileStem();
  return GetDbFileStem();
}

STRING VIDB::GetDbFileStem(const INT Idx, PSTRING StringBuffer) const
{
  return *StringBuffer = GetDbFileStem(Idx);
}

STRING VIDB::GetDbFileStem(const STRING& Key) const
{
  STRING NewKey;
  const size_t i = VirtualSet (Key, &NewKey);
  if (i)
    {
      return c_dblist[i-1]->GetDbFileStem();
    }
  return GetDbFileStem();
}

STRING VIDB::GetDbFileStem(const STRING& Key, PSTRING StringBuffer) const
{
  return *StringBuffer = GetDbFileStem(Key);
}

STRING VIDB::GetDbFileStem(const RESULT& ResultRecord) const
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    {
      return c_dblist[i-1]->GetDbFileStem();
    }
  return GetDbFileStem();
}


STRING VIDB::GetDbFileStem(const RESULT& ResultRecord, PSTRING StringBuffer) const
{
  return *StringBuffer = GetDbFileStem(ResultRecord);
}

GDT_BOOLEAN VIDB::GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer)
{
  STRING NewKey;
  const size_t i = VirtualSet (Key, &NewKey);
  if (i)
    return c_dblist[i-1]->GetRecordDfdt (NewKey, DfdtBuffer);
  return GDT_FALSE;
}

GDT_BOOLEAN VIDB::GetRecordDfdt (const RESULT& Result, PDFDT DfdtBuffer)
{
  if (c_dbcount == 1)
    return c_dblist[0]->GetRecordDfdt (Result, DfdtBuffer);
  const size_t i = Result.GetVirtualIndex();
  if (i)
    return c_dblist[i-1]->GetRecordDfdt (Result, DfdtBuffer);
  return GetRecordDfdt(Result.GetGlobalKey(), DfdtBuffer);
}

STRING VIDB::FirstKey() const
{
  if (c_dbcount)
    {
      STRING S (c_dblist[0]->FirstKey());
      if (!S.IsEmpty())
	{
	  if (c_dbcount > 1)
	    return "1@" + S;
	  else
	    return S;
	}
    }
  return NulString;
}

STRING VIDB::LastKey() const
{
  if (c_dbcount)
    {
      STRING S (c_dblist[c_dbcount-1]->LastKey());
      if (!S.IsEmpty())
	{
	  if (c_dbcount > 1)
	    return STRING((INT)(c_dbcount)) + "@" + S;
	  else
	    return S;
	}
    }
  return NulString;
}


STRING VIDB::NextKey(const STRING& Key) const
{
  STRING NewKey;
  const size_t i = VirtualSet (Key, &NewKey);

  if (i)
    {
      STRING S (c_dblist[i-1]->NextKey(NewKey));
      if (!S.IsEmpty())
	{
	  return STRING((INT)i) + "@" + S;
	}
      else if (i < c_dbcount)
	{
	  // Next volume first record
	  if (!(S = c_dblist[i]->FirstKey()).IsEmpty())
	    return STRING((INT)(i+1)) + "@" + S;
	}
    }
  return NulString;
}

STRING VIDB::PrevKey(const STRING& Key) const
{
  STRING NewKey;
  const size_t i = VirtualSet (Key, &NewKey);

  if (i)
    {
      STRING S (c_dblist[i-1]->PrevKey(NewKey));
      if (!S.IsEmpty())
	{
	  return STRING((INT)i) + "@" + S;
	}
      else if (i >= 2)
	{
	  // Previous Volume last record
	  if (!(S = c_dblist[i-2]->LastKey()).IsEmpty())
	    return STRING((INT)(i-1)) + "@" + S;
	}
    }
  return NulString;
}


size_t VIDB::GetAncestorContent (RESULT& ResultRecord, const STRING& NodeName, STRLIST *StrlistPtr)
{
  RESULT Result;
  const size_t i = VirtualSet (ResultRecord, &Result);
  if (i)
    return c_dblist[i-1]->GetAncestorContent (Result, NodeName, StrlistPtr);
  return 0;  
}


// Saving sessions

GDT_BOOLEAN SessionSave(const STRING& fn,
   PRSET prset, PVIDB pdb, 
   const STRING& formated_query,
   const STRLIST& RelevantURLS,
   const SQUERY& squery, const DATERANGE& range)
{
  PFILE Fp = fopen(fn, "wb");
  if (Fp)
    {
      pdb->GetDbFileStem().Write(Fp);
      prset->Write (Fp);
      formated_query.Write (Fp);
      RelevantURLS.Write (Fp);
      squery.Write (Fp);
      range.Write(Fp);
      fclose (Fp);
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

GDT_BOOLEAN SessionRead(const STRING& fn,
  PRSET prset, PVIDB *pdb,
  PSTRING formated_query, PSTRLIST RelevantURLS,
  PSQUERY squery, DATERANGE *range )
{
  PFILE Fp = fopen(fn, "rb");
  if (Fp)
    {
      STRING fullpath;

      fullpath.Read(Fp);
      prset->Read (Fp);
      formated_query->Read (Fp);
      RelevantURLS->Read (Fp);
      squery->Read (Fp);
      range->Read(Fp);
      fclose (Fp);
      *pdb = new VIDB (fullpath);
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

GDT_BOOLEAN VIDB::SetSortIndexes(int Which, atomicIRSET *Irset)
{
  const size_t TotalEntries = Irset->GetTotalEntries();
  INDEX_ID     index_id;
  int          db_id;
  GDT_BOOLEAN  res = GDT_TRUE;
  for (size_t i=1; i<=TotalEntries; i++)
    {
      index_id = Irset->GetIndex(i);
      db_id    = index_id.GetVirtualIndex();
      if (db_id > (int)c_dbcount)
	logf (LOG_PANIC, "Virtual Index %d exceeds db count %d!", db_id, c_dbcount);
      else
        res = Irset->SetSortIndex(i,  c_dblist[ db_id - 1 ]->GetSortIndex(Which, index_id) ) && res;
    }
  // Need to clean any junk up
  for (size_t i=0; i<c_dbcount; i++)
    c_dblist[i]->AfterSortIndex();
  return res;
}


SRCH_DATE VIDB::GetTimestamp() const
{
  SRCH_DATE Timestamp((time_t *)NULL);
  if (c_dbcount > 0)
    {
      Timestamp = c_dblist[0]->GetTimestamp();
      for (size_t i=1; i<c_dbcount; i++)
	{
	  SRCH_DATE nStamp ( c_dblist[i]->GetTimestamp() );
	  if (nStamp > Timestamp)
	    Timestamp = nStamp;
	}
    }
  return Timestamp;
}



PIRSET VIDB::SearchSmart(QUERY *Query, const STRING& DefaultField)
{
  PIRSET pIrset = NULL;  
  if (Query)
    {
      SQUERY Squery;
      pIrset = SearchSmart(*Query, DefaultField, &Squery);
      Query->Squery = Squery;
    }
  return pIrset;
}

PIRSET VIDB::SearchSmart(const QUERY& Query, SQUERY *SqueryPtr)
{
  return SearchSmart(Query.Squery, Query.Sort, Query.Method, SqueryPtr);

}
PIRSET VIDB::SearchSmart(const QUERY& Query, const STRING& DefaultField, SQUERY *SqueryPtr)
{
  return SearchSmart(Query.Squery, DefaultField, Query.Sort, Query.Method, SqueryPtr);
}


PIRSET VIDB::SearchSmart(const STRING& QueryString, const STRING& DefaultField,
         enum SortBy Sort, enum NormalizationMethods Method, SQUERY *SqueryPtr)
{
  SQUERY squery;
  
  if (squery.SetQueryTerm (QueryString) != 0)
    return SearchSmart(squery, DefaultField, Sort, Method, SqueryPtr);
  return NULL;
}

PIRSET VIDB::SearchSmart(const SQUERY& Squery, const STRING& DefaultField,
	 enum SortBy Sort, enum NormalizationMethods Method, SQUERY *SqueryPtr)
{
  PIRSET pIrset = NULL;

  if (!Opened || c_dbcount == 0)
    return pIrset;

  STRING QueryString;

  if (Squery.isPlainQuery(&QueryString) == GDT_FALSE)
    {
      if (SqueryPtr) *SqueryPtr = Squery; // 17 Dec 2007
      return Search(Squery, Sort, Method);
    }

  SQUERY squery(Squery);
  const size_t terms = squery.GetTotalTerms();

  if (terms == 1)
    return Search(squery, Sort, Method);

   // Search as literal phrase?
   if (terms >= 2)
    {
      SQUERY newQuery; 
      newQuery.SetLiteralPhrase(QueryString);
//cerr << "Search Phrase: " << QueryString << endl;
      if ((pIrset = Search(newQuery, Sort, Method)) != NULL)
	{
	  if (pIrset->GetTotalEntries() == 0)
	    {
	      delete pIrset;
	      pIrset = NULL; // Nothing found
	    }
	  else squery = newQuery;
	}
    }
  if (pIrset == NULL)
    {
      GDT_BOOLEAN res;
      STRING      field (DefaultField);
      // Search as Peer
      if (field.Trim(STRING::both).IsEmpty())
	res = squery.SetOperatorPeer();
      else
	res = squery.SetOperatorAndWithin(field);
      if (res)
	{
//cerr << "Search PEER/FIELD: " << field << endl;
	  if ((pIrset = Search(squery, Sort, Method)) != NULL)
	    {
	      if (pIrset->GetTotalEntries() == 0)
		{
		  delete pIrset;
		  pIrset = NULL; // Nothing found
		}
	    }
	  // Search
	  if (pIrset == NULL)
	    {
//cerr << "Search Or" << endl;
	      squery.SetOperatorOr();
	      if ((pIrset = Search(squery, Sort, Method)) != NULL)
		{
		  size_t total = pIrset->GetTotalEntries();
		  pIrset->Reduce(terms);
		  if (pIrset->GetTotalEntries() != total)
		    squery.PushReduce(terms);
		}
	    
	    }
	}
    }
  if (SqueryPtr) *SqueryPtr = squery;
  return pIrset;
}

//
// Search on all databases and merge all results into a single
// result set. 
PIRSET VIDB::FileSearch(const STRING& FileSpec)
{
  //
  // Search each database
  //
  register size_t i;
  GDT_BOOLEAN QueryError = GDT_TRUE;
  size_t TotalEntries = 0; 

  GDT_BOOLEAN Not_Seen = GDT_TRUE;
  int         hit_set  = 0;
  int         smallest_hit_set_idx = -1;
  size_t      smallest_hit_set_hits = 0;

  for (i = 0; i < c_dbcount; i++)
    {
      c_irsetlist[i] = c_dblist[i]->FileSearch (FileSpec);
      if (c_irsetlist[i] != NULL)
	{
	  size_t hits = c_irsetlist[i]->GetTotalEntries ();
	   // Set smallest set
	  if ((smallest_hit_set_idx < 0) || (hits < smallest_hit_set_hits))
	    {
	      smallest_hit_set_hits = hits;
	      smallest_hit_set_idx  = (int)i;
	    }

	  QueryError = GDT_FALSE;
	  if (Not_Seen)
	    {
	      if (hits > 0)
		{
		  hit_set = i + 1;
		  Not_Seen = GDT_FALSE;
		}
	    }
	  else if (hit_set > 0 && hits > 0)
	    hit_set = -1;
	  TotalEntries += hits;
	}
    }

  if (i == 0)
    logf (LOG_ERROR, "No database is opened for search!");

  if (QueryError)
    {
      logf (LOG_DEBUG, "VIDB::FileSearch() returning undefined result set (NULL)");
      return NULL; // Undefined result set
    }
  // If only 1 database or result save some effort...
  if (hit_set > 0)
    {
      logf(LOG_DEBUG, "Only one hit set (%d)", hit_set);
      // delete what we don't need...
      for (i=0; i < c_dbcount; i++)
	{
	  if ((int)i != hit_set-1 && c_irsetlist[i])
	    {
	      delete c_irsetlist[i];
	      c_irsetlist[i] = NULL;
	    }
	}
      PIRSET newIrset = c_irsetlist[ hit_set-1 ];

      return newIrset; 
    }
  else if (c_dbcount <= 1)
    {
      if ( c_dblist[i-1]->GetErrorCode() > 0)
        logf (LOG_INFO, "Search Failure: %s",  c_dblist[i-1]->ErrorMessage() );
      // delete what we are done with
      for (i=0; i < c_dbcount; i++)
	{
	  if (c_irsetlist[i])
	    {
	      delete c_irsetlist[i];
	      c_irsetlist[i] = NULL;
	    }
	}
      // return an empty set...
      return new IRSET(c_dblist[0]); // Nothing found
    }

  // Combine multiple sets

  // Use the smallest set to start to minimize copying
  IRSET *NewIrset = c_irsetlist[smallest_hit_set_idx];
  NewIrset->Reserve(TotalEntries+1); // Resize for everything

  for (i = 0; i < c_dbcount; i++)
    {
      if ((int)i != smallest_hit_set_idx && c_irsetlist[i])
	{
	  logf(LOG_DEBUG, "Adding %d [Set #%d]", c_irsetlist[i]->GetTotalEntries(), i);
	  NewIrset->Concat (*(c_irsetlist[i]));
	  delete c_irsetlist[i];; // Done with it
	  c_irsetlist[i] = NULL;
	}
    }
  return NewIrset;
}


#ifdef _WIN32
# undef getpid
# define getpid (int)GetCurrentProcessId  /* cast DWORD to int */
#endif

TEMP_VIDB::TEMP_VIDB (const STRLIST DbList, const STRLIST& Options)
{
  STRING TempDir (  AddTrailingSlash( GetTempDir()) );
  STRING Stem;
  int    tries = 0;
  FILE  *Fp;

  do {
     Stem = TempDir + STRING().form("V%xdb%u", ++tries, (unsigned)getpid());
     Fn   =  Stem + DbExtVdb;
  } while (FileExists(Fn));

  if ((Fp = Fn.Fopen("w")) != NULL)
    {
      fprintf(Fp, "; Temp \"%s\"\n", Fn.c_str());
      for (const STRLIST *p = DbList.Next(); p != &DbList; p=p->Next())
	fprintf(Fp, "%s\n", p->Value().c_str());
      fclose (Fp);
      vidbPtr = new VIDB (Stem, Options);
    }
}

TEMP_VIDB::~TEMP_VIDB()
{
  unlink(Fn);
  if (vidbPtr) delete vidbPtr;
}

