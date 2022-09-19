%module IB


%{
#pragma ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

//static const long timeout = 979891200; // Expires Fri Jan 19 09:00:00 2001

static const int DebugFlag = 0;

#include "../src/common.hxx"
#include "../src/string.hxx"
#include "../src/vidb.hxx"
#include "../src/rset.hxx"
#include "../src/irset.hxx"
#include "../src/log.hxx"
#include "../src/infix2rpn.hxx"
#include "../src/operator.hxx"

//// Use cached IRSETs..
//#undef IRSET
//#undef PIRSET
//#define IRSET  _IRSET
//#define PIRSET P_IRSET
///

#ifdef SWIGPYTHON
#include "callbacks.hxx"
#endif

#ifdef WINDOWS 
 #define JNICALL
 #include "win32.i"
#endif

// #ifdef SWIGPYTHON
// #include "pyglue.cxx"
// #endif

static const ArraySTRING NulArraySTRING;

static const char rFileErrorMsg[] = "file type is required for read parameter";
static const char wFileErrorMsg[] = "file type is required for write parameter";
static const char keyErrorMsg[]   = "key not available";

#ifdef SWIGPYTHON

#define myWrite(f) { \
  FILE *fp = NULL; \
  if (PyString_Check(f)) { \
    if ((fp = fopen( PyString_AsString(f), "a+")) != NULL) \
      f = NULL; \
  } else if (f == NULL || f == Py_None || !PyFile_Check(f)) { \
    PyErr_SetString(PyExc_TypeError, wFileErrorMsg); \
    return NULL; \
  } else fp = PyFile_AsFile(f); \
  bool result = GDT_TRUE; \
  if (fp) self->Write(fp); \
  else result = GDT_FALSE; \
  if (f == NULL) fclose(fp); \
  return Py_BuildValue("i", (int)result); \
}

#define myRead(f) { \
  FILE *fp = NULL; \
  if (PyString_Check(f)) { \
    if ((fp = fopen( PyString_AsString(f), "rb")) != NULL) { \
      f = NULL; \
    } \
  } else if (f == NULL || f == Py_None || !PyFile_Check(f)) { \
    PyErr_SetString(PyExc_TypeError, rFileErrorMsg); \
    return NULL; \
  } else fp = PyFile_AsFile(f); \
  bool result = fp ? self->Read(fp) : GDT_FALSE; \
  if (f == NULL) fclose(fp); \
  return Py_BuildValue("i", (int)result); \
}
#endif

#ifdef SWIGPYTHON

PyObject* PyIB_dict;

#if 0

PyObje* PyIBConstructObject(void* ptr, char* className)
{
    char            buff[64];  // should always be big enough...
    char            swigptr[64];
    _swig_type_info ty;

    ty.name = buff;

    sprintf(buff, "_%s_p", className);
    SWIG_MakePtr(swigptr, ptr, &ty);

    sprintf(buff, "%sPtr", className);
    PyObject* classobj = PyDict_GetItemString(PyIB_dict, buff);
    if (! classobj) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    PyObject* arg = Py_BuildValue("(s)", swigptr);
    PyObject* obj = PyInstance_New(classobj, arg, NULL);
    Py_DECREF(arg);

    return obj;
}
#endif /* SWIGPYTHON */

PyIBCallbackHelper::PyIBCallbackHelper() {
    m_self = NULL;
    m_lastFound = NULL;
}


PyIBCallbackHelper::~PyIBCallbackHelper() {
#ifdef WXP_WITH_THREAD
    PyEval_RestoreThread(PyIBEventThreadState);
#endif

    Py_XDECREF(m_self);

#ifdef WXP_WITH_THREAD
    PyEval_SaveThread();
#endif
}

void PyIBCallbackHelper::setSelf(PyObject* self) {
    m_self = self;
    Py_INCREF(m_self);
}



bool PyIBCallbackHelper::findCallback(const char * name) {
    m_lastFound = NULL;
    if (m_self && PyObject_HasAttrString(m_self, (char *)name))
        m_lastFound = PyObject_GetAttrString(m_self, (char *)name);
    return m_lastFound != NULL;
}


int PyIBCallbackHelper::callCallback(PyObject* argTuple) {
    PyObject*   result;
    int         retval = GDT_FALSE;

    result = callCallbackObj(argTuple);
    if (result) {                       // Assumes an integer return type...
        retval = PyInt_AsLong(result);
        Py_DECREF(result);
        PyErr_Clear();                  // forget about it if it's not...
    }
#ifdef WXP_WITH_THREAD
    PyEval_SaveThread();
#endif
    return retval;
}

// Invoke the Python callable object, returning the raw PyObject return
// value.  Caller should DECREF the return value and also call PyEval_SaveThread.
PyObject* PyIBCallbackHelper::callCallbackObj(PyObject* argTuple) {
#ifdef WXP_WITH_THREAD
    PyEval_RestoreThread(PyIBEventThreadState);
#endif
    PyObject*   result;

    result = PyEval_CallObject(m_lastFound, argTuple);
    Py_DECREF(argTuple);
    if (!result) {
        PyErr_Print();
    }
    return result;
}
#endif

//----------------------------------------------------------------------

%}

%wrapper %{
#ident "%Z%%Y%IB  %I% %G% %U% BSN"
%}

//----------------------------------------------------------------------
// This is where we include the other wrapper definition files for SWIG
//----------------------------------------------------------------------

%include my_typedefs.i

typedef enum SortBy {
  Unsorted, ByDate, ByReverseDate, ByScore, ByAdjScore, ByAuxCount,
  ByHits, ByReverseHits, ByKey, ByIndex, ByCategory, ByNewsrank, ByFunction,
  ByPrivate, ByPrivateLocal1, ByPrivateLocal2, ByPrivateLocal3, ByExtIndex=64,
  ByExtIndex1,  ByExtIndex2, ByExtIndex3, ByExtIndex4, ByExtIndex5, ByExtIndex6, ByExtIndex7,
  ByExtIndex8, ByExtIndex9, ByExtIndex10, ByExtIndex11, ByExtIndex12,
  ByExtIndexLast = 127 /* External sorts are 0 - 63 */
} SortBy;


// Operator Types
typedef enum {
// Error Operator
  OperatorERR = -1,
// 0-ary Operator (Bad Operator)
  OperatorNoop = 0,
// Unary Operators
  OperatorNOT, 
  OperatorWithin,
  OperatorXWithin,
  OperatorInside,
  OperatorInclusive,
  OperatorSibling,
  OperatorNotWithin,  // Like NOT but to field
  OperatorReduce,     // Trim set
  OperatorHitCount,   // Trim set by hits
  OperatorTrim,       // Trim set to max.
  OperatorWithinFile,
  OperatorWithinFileExtension,
  OperatorWithinDoctype,
  OperatorWithKey,
  OperatorSortBy,
  OperatorBoostScore,
// Binary Operators 
  OperatorOr, OperatorAnd, OperatorAndNot, OperatorXor, OperatorXnor,
  OperatorNotAnd, OperatorNor, OperatorNand,
  OperatorLT, OperatorLTE, OperatorGT, OperatorGTE,
  OperatorJoin, OperatorJoinL, OperatorJoinR,
  OperatorProximity = 40, // In same level- adj, near, same, with
  OperatorBefore,
  OperatorAfter,
  OperatorAdj,
  OperatorFollows,
  OperatorPrecedes,
  OperatorNear,
  OperatorFar,
  OperatorNeighbor,
  OperatorAndWithin,
  OperatorOrWithin,
  OperatorBeforeWithin,
  OperatorAfterWithin,
  OperatorPeer,
  OperatorBeforePeer,
  OperatorAfterPeer,
  OperatorXPeer,
// Special Unary operator (that mimic a term search)
  OperatorKey,
  OperatorFile,
} t_Operator;





enum NormalizationMethods {
  Unnormalized = 0, NoNormalization, CosineNormalization, MaxNormalization, LogNormalization,
  BytesNormalization, preCosineMetricNormalization, CosineMetricNormalization, UndefinedNormalization
};

%constant const char * ELEMENT_Full     = "F";
%constant const char * ELEMENT_Brief    = "B";

%constant const char * ELEMENT_Short    = "S";
%constant const char * ELEMENT_G        = "G";

%constant const char * ELEMENT_Raw      = "R";
%constant const char * ELEMENT_Location = "L";
%constant const char * ELEMENT_Metadata = "M";

%constant const char *  RECORDSYNTAX_Sutrs   ="1.2.840.10003.5.101";
%constant const char *  RECORDSYNTAX_Usmarc  ="1.2.840.10003.5.10";
%constant const char *  RECORDSYNTAX_Html    ="1.2.840.10003.5.108";
%constant const char *  RECORDSYNTAX_Sgml    ="1.2.840.10003.5.1000.81.1";
%constant const char *  RECORDSYNTAX_Xml     ="1.2.840.10003.5.1000.81.2";
%constant const char *  RECORDSYNTAX_Raw     ="1.2.840.10003.5.1000"; // Private
%constant const char *  RECORDSYNTAX_DVBHtml ="1.2.840.10003.5.1000.34.2"; // Private for DVB

%constant const char *  copyright = "\
Copyright 1999-2000 Edward C. Zimmermann and Basis Systeme netzwerk, Munich Germany.";
%constant const char * version = "%I%";



#ifndef SWIGPYTHON

class STRLIST {
public:
  STRLIST();
//  STRLIST(const char * String); // See STRING() below
//  STRLIST(const char * String1, const char * String2);
//  STRLIST(const char * String, char Chr); // above is Chr = '\0'
//  STRLIST(const STRLIST OtherVList);
  STRLIST(const ArraySTRING Array);
  STRLIST(const char * const * CStrList);

  STRLIST& Cat(const STRLIST& OtherVlist);
  STRLIST& Cat(const ArraySTRING Array);
  STRLIST& Cat(const char * const * CStrList);

  STRLIST *AddEntry(const char * StringEntry);
  STRLIST *AddEntry(const STRLIST& Strlist);

  STRLIST *SetEntry(const unsigned int Index, const char *StringEntry);
  STRING   GetEntry(const unsigned int Index);
  bool     DeleteEntry(const unsigned int pos);

  STRLIST       *Next();
  STRLIST       *Prev();
  STRING        Value();

  unsigned      GetTotalEntries() const;

  unsigned int Sort ();
  unsigned int UniqueSort();

  STRLIST& SplitWords (const char * TheString);
  STRLIST& SplitTerms (const char * TheString);
  STRLIST& SplitPaths (const char * TheString);


  STRLIST& Split(const char Separator, const STRING TheString);
  STRLIST& Split(const char* Separator, const STRING TheString);
  STRLIST& SplitWords (const STRING TheString);
  STRLIST& SplitTerms (const STRING TheString);
  STRLIST& SplitPaths (const STRING TheString);

  // Cat them together
  STRING Join(const char Separator) const;
  STRING Join(const char* Separator) const;

  unsigned Search(const STRING SearchTerm) const;
  unsigned SearchCase(const STRING SearchTerm) const;

  STRING GetValue(const STRING Title) const;


  ~STRLIST();
};

#endif /* !SWIGPYTHON */


class ArraySTRING {
public:
  ArraySTRING();
  ArraySTRING(int size);
  ArraySTRING(const ArraySTRING OtherArraySTRING);
  ArraySTRING(const STRLIST& list);

 ~ArraySTRING();

  void Empty();
  void Clear();

  unsigned  Count() const;
  bool  IsEmpty();

  STRING Item(unsigned nIndex) const;
  STRING GetEntry(const unsigned Index) const;
%extend {
  STRING  __getitem__(int i) {
    return self->GetEntry(i+1);
  }
}
  void   SetEntry(const unsigned Index, const char * src);
  void   Add (const char * str);
  void   Insert(const char * str, unsigned uiIndex);
  void   Replace(const char * src, unsigned nIndex);
  void   Remove(unsigned nIndex);

%extend {
    ArraySTRING& getself() { return *self; }
#ifdef SWIGPYTHON
    PyObject* asTuple() {
    unsigned  count = self->Count();
        PyObject* rv = PyTuple_New(count);
    for (int i=0; i< count; i++)
      PyTuple_SetItem(rv, i,  PyString_FromString(self->Item(i)));
        return rv;
    }
#endif
   STRING Join(const char *seperator) {
      STRING result;
      char quotes = '"';
      if (seperator == NULL) seperator = ",";
      else if (strchr(seperator, '"')) quotes = '\'';
      else if (strchr(seperator, '\'')) quotes = '"';
      // Make sure its OK..
      if (strchr(seperator, quotes)) quotes = '\0'; // Nope

      const unsigned  count = self->Count();
      for (unsigned i=0; i< count; i++)
    {
      // Add seperator
      if (i) result.Cat(seperator);
      bool quote = self->Item(i).Search(seperator);
      // Open quote
      if (quote && quotes) result.Cat(quotes);
      // Item
      result.Cat (self->Item(i));
      // Close quote
      if (quote && quotes) result.Cat(quotes);
    }
      return result;
    }
}

};


// Search Statistics
class IDB_STATS {
public:
  IDB_STATS();
  ~IDB_STATS();
  void SetHits(size_t nHits);
  void SetTotal(size_t nTotal);
  size_t GetTotal() const;
  size_t GetHits() const;
  void   Clear();
  void   SetName(const STRING newName);
  STRING GetName() const;
};


class VIDB_STATS {
public:
  VIDB_STATS();
  ~VIDB_STATS();
  void Clear();
  IDB_STATS operator[](size_t n) const;
  void SetTotal(size_t i, size_t total);
  void SetHits(size_t i, size_t total);
  void SetName(size_t i, const STRING Name);
};


class IDBOBJ {
public:
  IDBOBJ();
  ~IDBOBJ();

  bool getUseRelativePaths() const;
  bool setUseRelativePaths(bool val=1);

  STRING RelativizePathname(const STRING Path) const;
  STRING ResolvePathname(const STRING Path) const;
};


class INDEX_ID {
public:
  INDEX_ID();
  INDEX_ID(const INDEX_ID Other);
  ~INDEX_ID();

  long        GetIndex();
  %extend { void        SetIndex(long index) { *self = index; } }

  void        SetMdtIndex(unsigned NewMdtIndex);
  unsigned    GetMdtIndex() const;
  void        SetVirtualIndex(int NewvIndex);
  int         GetVirtualIndex() const;

  bool        Equals(INDEX_ID Val) const;
  int         Compare(INDEX_ID Val) const;
};

// Fill in more methods later!
class DOCTYPE {
public:
  DOCTYPE();
  DOCTYPE(IDBOBJ* DbParent, const char *NewDoctype="");

  SRCH_DATE  ParseDate(const char * Buffer, const char * FieldName) const;
  DATERANGE  ParseDateRange(const char * Buffer, const char * FieldName) const;
};



class           DTREG {
 public:
  DTREG(IDBOBJ *DbParent);
  DTREG(IDBOBJ *DbParent, const STRING PluginsPath);

  bool           PluginExists(const STRING Name) const;

  void           AddPluginPath(const STRING Path);

  DOCTYPE        *GetDocTypePtr(const STRING Doctype);
  DOCTYPE        *GetDocTypePtr(const DOCTYPE_ID& Id);
  DOCTYPE        *GetDocTypePtr(const char *DocType, const char * DocTypeID);

  bool            ValidateDocType(const STRING DocType);
  bool            ValidateDocType(const DOCTYPE_ID& Id);

  const           STRLIST& GetDocTypeList();
  const int       Version() const;
  ~DTREG();
};


class MDTREC {
public:
  MDTREC(MDT *MdtPtr = NULL);
//  MDTREC(const MDTREC OtherMdtrec);

  void      SetCategory(const unsigned newCategory);
  unsigned  GetCategory() const;

  void    SetPriority(const short newPriority);
  short   GetPriority() const;

  void    SetKey(const char * NewKey);
  STRING  GetKey() const;

  void    SetDocumentType(const DOCTYPE_ID NewDoctypeId);
  DOCTYPE_ID GetDocumentType() const;

  void    SetPath(const STRING NewPathName);
  STRING  GetPath() const;

  void    SetFileName(const STRING NewFileName);
  STRING  GetFileName() const;

  void    SetFullFileName(const STRING NewFullPath);
  STRING  GetFullFileName() const;

  void   SetGlobalFileStart(GPTYPE NewStart);
  GPTYPE GetGlobalFileStart() const;

  void   SetLocalRecordStart(GPTYPE NewStart);
  GPTYPE GetLocalRecordStart() const;

  void   SetLocalRecordEnd(GPTYPE NewEnd);
  GPTYPE GetLocalRecordEnd() const;

  void    SetLocale(LOCALE NewLocale);
  LOCALE  GetLocale() const;

  // We store the dates as a pair of 4-byte ints
  // Generic Date
  void SetDate(SRCH_DATE NewDate);
  SRCH_DATE GetDate() const;
  // Modified Date
  void SetDateModified(SRCH_DATE NewDate);
  SRCH_DATE GetDateModified() const;
  // Date Created
  void SetDateCreated(SRCH_DATE NewDate);
  SRCH_DATE GetDateCreated() const;

  void           SetDateExpires(SRCH_DATE NewDate);
  SRCH_DATE      GetDateExpires() const;

  int            TTL() const;
  int            TTL(SRCH_DATE Now) const;

  void SetDeleted(bool Flag);
  bool GetDeleted() const;

  void FlipBytes();

  STRING Dump() const;

  ~MDTREC();
};


class MDT {

public:
  MDT(STRING DbFileStem, bool WrongEndian); // was with defaults

  STRING GetFileStem() const;
  void SetEntry (const unsigned Index, const MDTREC MdtRecord);
  unsigned LookupByKey (const char * Key);
  MDTREC  *GetEntry (const unsigned Index);
  // Mark the record by index as deleted
  bool Delete(const unsigned Index);
  // Mark the record by index as not deleted
  bool UnDelete(const unsigned Index);
  // Is the record Marked deleted?
  bool IsDeleted(const unsigned Index);
  unsigned GetTotalEntries();
  unsigned GetTotalDeleted();
  bool GetChanged () const;
  unsigned RemoveDeleted ();
  bool Ok() const;

  ~MDT ();
};

class FC {
public:
  FC(FC Fc);
  FC(GPTYPE *GpPair);
  FC(GPTYPE Start, const GPTYPE End);
  FC(unsigned Start = 0, const unsigned End = 0);

%extend {
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f);}
    PyObject *Read (PyObject *f)  { myRead(f); }
#endif
}

  void     SetFieldStart(const unsigned NewFieldStart);
  void     SetFieldEnd(const unsigned NewFieldEnd);
  unsigned GetFieldStart();
  unsigned GetFieldEnd();

  unsigned GetLength ();

  // Comparison
  int Compare (const FC Fc);
  int Contains (const FC Fc);

%extend {
#ifdef SWIGPYTHON
    PyObject* asTuple() {
    PyObject* rv = PyTuple_New(2);
    PyTuple_SetItem(rv, 0, PyInt_FromLong(self->GetFieldStart()));
    PyTuple_SetItem(rv, 1, PyInt_FromLong(self->GetFieldEnd()));
    return rv;
    }
#endif
  }
  void FlipBytes();
  ~FC();
};

#ifndef SWIGPYTHON
class FCT {
public:
  FCT();
  void      AddEntry(const FC FcRecord);
  FC        GetEntry(const unsigned Index);
  bool      IsSorted();
  bool      IsEmpty();
  unsigned  GetTotalEntries();
  void      Reverse();
  void      SortByFc();
  int       Refcount_();
#ifdef SWIGPERL


#endif
} ;
#endif /* SWIGPYTHON */

class TREENODE {
public:
 TREENODE() { }
 TREENODE(const FC fc, const char * name);
 ~TREENODE();

 STRING  Name();
 FC      Fc();
};

class NODETREE {
public:
  NODETREE();
  NODETREE(const NODETREE table);

#if !defined(SWIGPERL) && !defined(SWIGJAVA)
  NODETREE& operator= (const NODETREE table);
#endif

  TREENODELIST   *AddEntry(const TREENODE Node);
  TREENODELIST   *AddEntry(const NODETREE table);

  void Clear();
  void Reverse();
  void Sort();
  bool IsSorted();
  bool IsEmpty();

  TREENODE GetEntry(const int Index);

  STRING  XMLNodeTree(const char *value="");

  ~NODETREE();
};


class RESULT {
public:
  RESULT();
  RESULT(const MDTREC& Mdtrec);
  RESULT(const RESULT& OtherResult);

  %extend {
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
#endif
  }

  void     SetIndex(const INDEX_ID newIndex);
  INDEX_ID GetIndex() const;

  void     SetMdtIndex(const unsigned NewMdtIndex);
  unsigned GetMdtIndex() const;
  void     SetVirtualIndex(const unsigned newIndex);
  unsigned GetVirtualIndex() const;

  long     GetCategory() const;
  void     SetCategory(long newCategory);

  void     SetKey(const char * NewKey);
  STRING   GetKey() const;

  STRING   GetGlobalKey() const;

  const char     *GetLanguageCode () const;
  const char     *GetCharsetCode () const ;
  const char     *GetLanguageName () const;
  const char     *GetCharsetName () const;

  STRING    GetFullFileName() const;
  STRING    GetPath() const;
  STRING    GetFileName() const;

  STRING    GetDoctype() const;
  long      GetRecordStart() const;
  long      GetRecordEnd() const;
  long      GetLength () const;
  long      GetRecordSize() const;
  double    GetScore() const;
  unsigned  GetAuxCount() const;
  unsigned  GetHitTotal() const;
  SRCH_DATE GetDate() const;
  SRCH_DATE GetDateModified() const;
  SRCH_DATE GetDateCreated() const;

  FCT       GetHitTable() const;
  int       GetRefcount_() const;

  // Dump a XML hit table
  STRING XMLHitTable() const;

  ~RESULT();
};

class IRSET {
public:
  IRSET(IDBOBJ *IdbPtr, unsigned Reserve=0); // Create with a VIDB or IDB pointer

  %extend {
    IRSET &getself() { return *self; }
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
#else
    void    Write(const char * Path) {
      FILE *Fp = fopen(Path, "wb");
      if (Fp)
    self->Write(Fp);
    }
    void    Read(const char * Path) {
      FILE *Fp = fopen(Path, "rb");
      if (Fp) self->Read(Fp);
    }
#endif
  }

  void LoadTable (const STRING FileName);
  void SaveTable (const STRING FileName) const;

  %extend {
    bool SaveSearch (const STRING& FileName, const QUERY& Query) {
      FILE *fp = FileName.Fopen("wb");
      if (fp)
        {
      STRING("#IB_SEARCH#").Write(fp); // Write Magic
      Query.Write(fp);
          self->Write(fp);
      SRCH_DATE("Now").Write(fp);
          fclose(fp);
      return 1;
        }
      return 0;
    }
%newobject IRSET::LoadSearch ;
    QUERY * LoadSearch (const STRING& FileName) {
      FILE *fp = FileName.Fopen("rb");
      if (fp)
    {
      STRING m;
      m.Read(fp);
      if (m == "#IB_SEARCH#")
        {
          QUERY *QueryPtr = new QUERY();
          QueryPtr->Read(fp);
          self->Read(fp);
          // Could now read a timestamp that we'll ignore
          fclose(fp);
          return QueryPtr;
        }
      fclose(fp);
    }
      return NULL; // No available IRSET
    }
  }

%newobject GetRset;
%newobject Fill;
  RSET *GetRset (unsigned Total= 0) const;
  RSET *Fill(unsigned Start, unsigned End = 0) const;


  unsigned GetTotalEntries () const;
  unsigned GetHitTotal ();

///// NEED TO UPDATE THESE //////

  // Binary Functions
  IRSET *Or (const IRSET& OtherIrset);
  IRSET *Nor (const IRSET& OtherIrset);
  IRSET *And (const IRSET& OtherIrset);
  IRSET *Nand (const IRSET& OtherIrset);
  IRSET *AndNot (const IRSET& OtherIrset);
  IRSET *Xor (const IRSET& OtherIrset);
  IRSET *Near (const IRSET& OtherIrset);
  IRSET *Far (const IRSET& OtherIrset);
  IRSET *After (const IRSET& OtherIrset);
  IRSET *Before (const IRSET& OtherIrset);
  IRSET *Adj (const IRSET& OtherIrset);
  IRSET *Follows (const IRSET& OtherIrset);
  IRSET *Precedes (const IRSET& OtherIrset);
  IRSET *Neighbor (const IRSET& OtherIrset);
  IRSET *Peer (const IRSET& OtherIrset);
  IRSET *BeforePeer (const IRSET& Irset);
  IRSET *AfterPeer (const IRSET& Irset);
  IRSET *XPeer (const IRSET& OtherIrset);

  IRSET *Within (const char * FieldName);
  IRSET *Within (IRSET& OtherIrset, const char *FieldName);
  IRSET *BeforeWithin (IRSET& OtherIrset, const char *FieldName);
  IRSET *AfterWithin (IRSET& OtherIrset, const char *FieldName);

  IRSET *XWithin (const char *FieldName);
  IRSET *Inside (const char *FieldName);

  IRSET *WithinXChars (const IRSET& OtherIrset, float Metric = 50.0);
  IRSET *WithinXChars_Before (const IRSET& OtherIrset, float Metric = 50.0);
  IRSET *WithinXChars_After (const IRSET& OtherIrset, float Metric = 50.0);

  IRSET *WithinXPercent (const IRSET& OtherIrset, float Percent);
  IRSET *WithinXPercent_Before (const IRSET& OtherIrset, float Percent);
  IRSET *WithinXPercent_After (const IRSET& OtherIrset, float Percent);

  // Unary Functions
  IRSET *Not ( );

  IRSET *Reduce (int Value=0);

  void ComputeScores (int TermWeight);

  void SortBy(enum SortBy SortBy);

  double GetMaxScore() const;
  double GetMinScore() const;

  ~IRSET();
};

class RSET {
public:
  RSET(unsigned Reserve = 0);

  %extend {
    RSET &getself() { return *self; }
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
#else
    void    Write(const char * Path) {
      FILE *Fp = fopen(Path, "wb");
      if (Fp) self->Write(Fp);
    }
    void    Read(const char * Path) {
      FILE *Fp = fopen(Path, "rb");
      if (Fp) self->Read(Fp);
    }
#endif
  }

  RSET& Cat(const RSET OtherSet);

  void LoadTable(const STRING FileName);
  void SaveTable(const STRING FileName) const;

  double GetMaxScore () const;
  double GetMinScore () const;

  bool FilterDateRange(const DATERANGE Range);

  RESULT GetEntry(const unsigned Index) const;
  int GetScaledScore(const double UnscaledScore, const int ScaleFactor);

  unsigned GetHitTotal () const;

  unsigned Find(const char * Key) const;

  unsigned GetTotalEntries();
  void SortBy(enum SortBy SortBy);
  void SortByCategoryMagnetism(double Factor);

  unsigned Reduce(int TermCount);
  unsigned DropByTerms(size_t TermCount);
  unsigned DropByScore(double Score);

%extend {
   RESULT __getitem__(int i) { return self->GetEntry(i+1); }
  }
};


class INFIX2RPN {
public:
  INFIX2RPN ();
  STRING  Parse (const STRING InfixSentence);
  bool    InputParsedOK () const;
  STRING  GetErrorMessage () const;

  ~INFIX2RPN();
};


class THESAURUS {
public:
  THESAURUS();
  THESAURUS(const STRING Path);
  THESAURUS(const STRING SourceFileName, const STRING TargetPath, bool Force);

  bool Compile(const STRING Source, const STRING Target, bool Force=1);

  void   SetFileName(const STRING Fn);
  ~THESAURUS();
};


class OPERATOR{
public:
  OPERATOR();
  t_OpType   GetOpType() const;
#ifndef SWIGJAVA
  OPERATOR& operator=(const OPERATOR& OtherOp);
#endif
  void       SetOperatorType(const t_Operator OperatorType);
  t_Operator GetOperatorType() const;
  void       SetOperatorMetric(const float Metric);
  float      GetOperatorMetric() const;
  void       SetOperatorString(const const char *Arg);
  STRING     GetOperatorString() const;
  ~OPERATOR();
};


enum QueryTypeMethods { QueryAutodetect = 0, QueryRPN, QueryInfix, QueryRelevantId};
class SQUERY {
public:
  SQUERY();
  SQUERY(SQUERY OtherSquery);
  SQUERY(const STRING Sentence, enum QueryTypeMethods Typ=0, THESAURUS *Ptr = NULL);

  %extend {
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
    STRING __repr__ () { return self->GetRpnTerm(); }
#endif
  }

  unsigned GetTotalTerms() const;

  // Covert the Ors in plain query to use Operator
  // example AND:field etc.
  bool SetOperator(const OPERATOR& Operator);
  // Convienience of above, set to AND:<FieldName> or Peer
  bool SetOperatorAndWithin(const char *FieldName);
  bool SetOperatorNear(); // Near or Peer with Fielded
  bool SetOperatorPeer();

  // Replace means don't add to existing attributes
  unsigned SetAttributes(const ATTRLIST& Attrlist, bool Replace=0);

  // True is plain query (not fields, no operators other than Or)
  bool     isPlainQuery() const;

  // Are all the Ops the same? Example isOpQUery(OperatorAnd) for all AND
  bool isOpQuery (const t_Operator Operator) const;

  // Returns number of terms set, 0 ==> Error
  unsigned SetRelevantTerm (const STRING RelId);
  unsigned SetInfixTerm(const STRING NewTerm);
  unsigned SetRpnTerm(const STRING NewTerm);
  unsigned SetQueryTerm(const STRING NewTerm); // Try to guess Infix, RPN or ORed words

  unsigned SetQueryTermUTF(const STRING& UtfTerm); // ONLY Latin1 set!

  unsigned SetWords (const STRING NewTerm, int Weight=1);
  %extend {
    unsigned SetWordsAnd (const STRING NewTerm, int Weight=1) {
    return self->SetWords(NewTerm, Weight,  OperatorAnd); }
  }
  size_t SetWords(const STRING Words, const OPERATOR& BinaryOperator, int Weight = 1);
  size_t SetFreeFormWords(const STRING Sentence, int Weight=1);
  size_t SetFreeFormWordsPhonetic(const STRING Sentence, int Weight);

#if 0
  size_t SetWords(const STRING Sentence, ATTRLIST *Attrlist);
#endif

  STRING  LastErrorMessage() const;

  STRING  GetRpnTerm() const;

  SQUERY& Cat(const SQUERY OtherSquery);
  SQUERY& Cat(const SQUERY OtherSquery, const char * Opname);

  void OpenThesaurus(const STRING FullPath);
  void CloseThesaurus();
  void ExpandQuery();

  ~SQUERY();
};


class QUERY {
public:

  QUERY();
#ifndef SWIGJAVA
  QUERY(const QUERY& Query);
  // QUERY(enum SortBy sort=Unsorted, enum NormalizationMethods method=defaultNormalization);
  QUERY(enum SortBy sort, enum NormalizationMethods method=defaultNormalization);
  QUERY(const SQUERY& query, enum SortBy sort=Unsorted, enum NormalizationMethods method=defaultNormalization);
#endif

#ifndef SWIGJAVA
  QUERY& operator =(const QUERY& Other);
  QUERY& operator =(const SQUERY& Other);
#endif

  void          SetSQUERY(const SQUERY& newQuery);
  const SQUERY& GetSQUERY() const;

  void SetNormalizationMethod(enum NormalizationMethods newMethod);
  enum NormalizationMethods GetNormalizationMethod() const;

  void SetSortBy(enum SortBy newSort);
  enum SortBy GetSortBy() const;

  unsigned GetMaximumResults() const;
  void     SetMaximumResults(unsigned newMax); // 0 is unlimited

#ifdef SWIGPYTHON
%extend {
  PyObject *Write (PyObject *f) { myWrite(f); }
  PyObject *Read (PyObject *f)  { myRead(f);  }
  PyObject *asTuple() {
    PyObject* rv = PyTuple_New(4);
        PyTuple_SetItem(rv, 0,  PyString_FromString( self->GetSQUERY().GetRpnTerm()) );
        PyTuple_SetItem(rv, 1,  PyInt_FromLong(self->GetSortBy()) );
        PyTuple_SetItem(rv, 1,  PyInt_FromLong(self->GetNormalizationMethod()) );
        PyTuple_SetItem(rv, 2,  PyInt_FromLong(self->GetMaximumResults()) );
        return rv;
  }
}
#endif
};

class SRCH_DATE {
public:
  SRCH_DATE(const char * Value=NULL);
  SRCH_DATE(const double FloatVal);
  SRCH_DATE(const long LongVal);


  %extend {
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
    STRING __repr__ () { return self->RFCdate(); }
#endif
  }

  // Add/Subract dates..
  SRCH_DATE Plus(const SRCH_DATE OtherDate);
  SRCH_DATE Minus(const SRCH_DATE OtherDate);
  // Add N seconds, minutes, hours, days, weeks, months or years..
  SRCH_DATE PlusNseconds(int seconds);
  SRCH_DATE PlusNminutes(int minutes);
  SRCH_DATE PlusNhours(int hours);
  SRCH_DATE PlusNdays(int days);
  SRCH_DATE PlusNweeks(int weeks);
  SRCH_DATE PlusNmonths(int months);
  SRCH_DATE PlusNyears(int years);
  // Substract..
  SRCH_DATE MinusNseconds(int seconds);
  SRCH_DATE MinusNminutes(int minutes);
  SRCH_DATE MinusNhours(int hours);
  SRCH_DATE MinusNdays(int days);
  SRCH_DATE MinusNweeks(int weeks);
  SRCH_DATE MinusNmonths(int months);
  SRCH_DATE MinusNyears(int years);
  // Some cases
  SRCH_DATE Tommorrow();
  SRCH_DATE Yesterday();
  SRCH_DATE NextWeek();
  SRCH_DATE LastWeek();
  SRCH_DATE NextMonth();
  SRCH_DATE LastMonth();
  SRCH_DATE NextYear();
  SRCH_DATE LastYear();

  double         GetValue() ;

  bool IsYearDate();
  bool IsMonthDate();
  bool IsDayDate();
  bool IsBogusDate();
  bool IsValidDate();
  bool IsLeapYear();

  bool Ok();

  bool TrimToMonth();
  bool TrimToYear();

  bool SetToYearStart();    // To 1 Jan
  bool SetToYearEnd();  // To 31 Dec
  bool SetToMonthStart();// To 1 XXX
  bool SetToMonthEnd(); // To ? XXX (depends on month)
  bool SetToDayStart();  // To 00:00:00
  bool SetToDayEnd();    // To 23:59:59

  bool PromoteToMonthStart();
  bool PromoteToMonthEnd();
  bool PromoteToDayStart();
  bool PromoteToDayEnd();

  void        GetTodaysDate();
  void        SetNow();

  // Set Year, Month, Day
  bool SetYear(int nYear);   // YYYY
  bool SetMonth(int nMonth); // MM
  bool SetDay(int nDay);     // DD

  // Get Year, Month, Day
  int         Year() ;  // Year of date YYYY
  int         Month() ; // Month of date MM (1-12)
  int         Day() ;   // Day of date YY (1-31)

  int         DayOfWeek() ; // 1 = Sunday, ... 7 = Saturday
  int         DayOfYear() ;
  int         GetFirstDayOfMonth() ; // 1 = Sunday, ... 7 = Saturday

  int         GetWeekOfMonth() ;
  int         WeekOfYear() ;

  int         GetDaysInMonth() ;
  long        GetJulianDate() ; // days since 1/1/4713 B.C.
  long        GetTimeSeconds() ; // Seconds of Time since Midnight

  bool SetTimeOfFile(const STRING Pathname);
  bool SetTimeOfFileCreation(const STRING Pathname);

  SRCH_DATE GetTimeOfFile(const STRING Pathname);
  SRCH_DATE GetTimeOfFileCreation(const STRING Pathname);

  bool IsBefore(SRCH_DATE OtherDate) ;
  bool Equals(SRCH_DATE OtherDate) ;
  bool IsDuring(SRCH_DATE OtherDate) ;
  bool IsAfter(SRCH_DATE OtherDate) ;

  // Human Readable Copy
  STRING  ISOdate() ;
  STRING  RFCdate() ;
  STRING  ANSIdate() ;
  STRING  LCdate() ;

  %extend {
    STRING Strftime(const char *format) {
      STRING result;
      if (!self->Strftime(format, &result))
        result.form("*** Error, bad Strftime format: %s", format);
      return result;
    }
#ifdef SWIGPYTHON
    int __cmp__(const SRCH_DATE Other) {
    return Compare(*self, Other);
    }
#endif
  }
 ~SRCH_DATE();
};

class DATERANGE
{
public:
  DATERANGE(const SRCH_DATE Start, const SRCH_DATE End);

  SRCH_DATE  GetStart() ;
  SRCH_DATE  GetEnd()   ;

  void  SetStart(const SRCH_DATE NewStart);
  void  SetEnd(const SRCH_DATE NewEnd);

  bool Ok() ;
  bool Defined() ;
  bool Contains(const SRCH_DATE TestDate) ;

  // Formating...
  %extend {
    STRING ISO() { return (STRING)(*self); }
    STRING RFC() {
      STRING from, to;
      if (self->RFC(&from, &to))
    return STRING().form("%s To %s", from.c_str(), to.c_str());
      return NulString;
    }
    STRING Strftime(const char *fmt, const char *sep = NULL) {
      STRING from, to;
      if (sep == NULL || *sep == '\0') sep = "-";
      if (fmt && self->Strftime(fmt, &from, &to))
        return STRING().form("%s%s%s", from.c_str(), sep, to.c_str());
      return NulString;
    }
  }

  // I/O
  %extend {
#ifdef SWIGPYTHON
    PyObject *Write (PyObject *f) { myWrite(f); }
    PyObject *Read (PyObject *f)  { myRead(f);  }
#endif
  }

  ~DATERANGE();

  %extend {
#ifdef SWIGPYTHON
    PyObject* asTuple() {
        PyObject* rv = PyTuple_New(2);
        PyTuple_SetItem(rv, 0,  PyString_FromString(self->GetStart().ISOdate()) );
        PyTuple_SetItem(rv, 1,  PyString_FromString(self->GetEnd().ISOdate()) );
        return rv;
    }
#endif
    double Duration() {
    return self->Ok() ? self->GetEnd().GetValue() - self->GetStart().GetValue() : 0.0;
    }
  }
};

class SCANOBJ {
public:
  SCANOBJ(const STRING Term, unsigned Freq);
  unsigned Frequency() const;
  STRING   Term () const;
  ~SCANOBJ();

  %extend {
#ifdef SWIGPYTHON
    PyObject* asTuple() {
        PyObject* rv = PyTuple_New(2);
        PyTuple_SetItem(rv, 0,  PyString_FromString(self->Term()) );
        PyTuple_SetItem(rv, 1,  PyInt_FromLong(self->Frequency()) );
        return rv;
    }
#endif
  }
};


class SCANLIST {
public:
  SCANLIST(const SCANLIST Table);
  void     Reverse();
  bool     IsEmpty();
  unsigned GetTotalEntries();
  SCANOBJ  GetEntry(const unsigned Index);
  ~SCANLIST();
  %extend {
#ifdef SWIGPYTHON
    PyObject* Get() {
        PyObject *listPtr = PyList_New ( self->GetTotalEntries() );
        int i = 0;
        const atomicSCANLIST *List = self->GetPtratomicSCANLIST() ;
        for (const atomicSCANLIST *p = List->Next(); p != List; p = p->Next())
	  {
	    register SCANOBJ t = p->Value();
            PyList_SetItem(listPtr, i++,  SCANOBJ_asTuple( &t ));
	  }
        return listPtr;
    }
    PyObject *__getitem__(int i) {
       register SCANOBJ t = self->GetEntry(i+1);
       return SCANOBJ_asTuple( &t );
    }
#endif
  }
};

class          DOCTYPE_ID {
 public:
  DOCTYPE_ID(const char * DocType);

  STRING DocumentType() const;
  void   Set(const STRING Value);
  STRING Get() const;
  char   *c_str() const;
  bool   IsDefined() const;
  bool   Equals(const DOCTYPE_ID Other);

  ~DOCTYPE_ID();
};


class DOC_ID {
 public:
  DOC_ID(const char *GlobalKey);
  bool Equals(const DOC_ID& OtherDoc);
  int Compare(const DOC_ID& OtherDoc);
  STRING GlobalKey();
  ~DOC_ID();
};




class RECORD {
public:
  RECORD();
  RECORD(const STRING Fullpath);
  RECORD(const RECORD& OtherRecord);

#if !defined(SWIGPERL) && !defined(SWIGJAVA)
  RECORD& operator= (const RECORD& OtherRecord);
#endif

  void    SetKey(const STRING NewKey);

  STRING  GetKey() const;

  void    SetPath(const STRING NewPathName);
  STRING  GetPath() const;

  void    SetFileName(const STRING NewName);
  STRING  GetFileName() const;

  void    SetFullFileName (const STRING FullName);
  STRING  GetFullFileName() const;

  void    SetRecordStart(const unsigned NewStart);
  unsigned  GetRecordStart() const;

  void    SetRecordEnd(const unsigned NewEnd);
  unsigned  GetRecordEnd() const;

  void    SetDocumentType(const DOCTYPE_ID NewType);
  DOCTYPE_ID  GetDocumentType() const;

  LOCALE   GetLocale () const;
  void     SetLocale (const LOCALE NewLocale);

  void    SetLanguage (const char * Language);
  void    SetCharset  (const char * Charset);

  SRCH_DATE GetDate() const;
  void      SetDate (const SRCH_DATE NewDate);
  // Modified Date
  void SetDateModified(SRCH_DATE NewDate);
  SRCH_DATE GetDateModified() const;
  // Date Created
  void SetDateCreated(SRCH_DATE NewDate);
  SRCH_DATE GetDateCreated() const;

  int     GetPriority() const;
  void    SetPriority(int);

  int     GetCategory() const;
  void    SetCategory(int newCategory);

  %extend {
    int __len__() {
       return self->GetRecordEnd() - self->GetRecordStart();
    }
  }

  void           SetDateExpires(SRCH_DATE NewDate);
  SRCH_DATE      GetDateExpires() const;

  int            TTL() const;
  int            TTL(SRCH_DATE Now) const;

  ~RECORD();
};


class FCACHE {
public:
   FCACHE(IDBOBJ *parent);

   bool Ok() const;

   // Normal entry points...
   bool ValidateInField(const GPTYPE HitGp);
   bool ValidateInField(const FC HitGp);

#if 0
   bool ValidateInField(const GPTYPE HitGp, STRSTACK Stack, bool Disk);
   bool ValidateInField(const FC HitFc, STRSTACK Stack, bool Disk);
#endif

   bool ValidateInField (const GPTYPE HitGp, const char * FieldName) const;
   bool ValidateInField (const FC HitFc, const char * FieldName) const;

   size_t GetTotal() const;

   bool SetFieldName(const char * fieldName, bool Disk=0);
   STRING      GetFieldName() const;

   ~FCACHE();
};


const int DbStateInvalid = -1;  // DB files have been corrupted because  some operation was interrupted
const int DbStateReady   =  0;  // DB ready for searching
const int DbStateBusy    =  2;  // DB files are being modified

enum MergeStatus { iNothing = 0, iOptimize, iMerge, iCollapse, iIncremental };


%newobject IDB::Search ;
%newobject IDB::SearchSmart ;
%newobject IDB::VSearch ;
%newobject IDB::KeyLookup ;

class IDB:public IDBOBJ {
public:
#ifndef SWIGJAVA
  IDB();
  IDB(bool SearchOnly);
#endif
  IDB(const STRING DBFullPath, bool SearchOnly=0);

#ifndef SWIGJAVA
#ifdef SWIGPYTHON
  IDB(const STRING DBFullPath, const STRLIST& DocTypeOptions); // Create
#else
  IDB(const STRING DBFullPath, const ArraySTRING DocTypeOptions); // Create
#endif
#endif

  // Open a DB (close it first)
  bool Open (const STRING DBName, bool SearchOnly = 0);
#ifdef SWIGPYTHON
  bool Open (const STRING DBName, const STRLIST& NewDocTypeOptions, bool SearchOnly = 0);
#endif
  // Close a DB
  bool Close(); // Don't need to call unless one wan't to call Open

  void SetDebugMode(bool OnOff);

//  const char *Description();

  STRING FirstKey() ;
  STRING LastKey() ;
  STRING NextKey(const STRING Key) ;
  STRING PrevKey(const STRING Key) ;

  void SetVolume(const STRING Name, int Vol);
  INT  GetVolume() const;

  void SetFindConcatWords(bool Set=1);
  GDT_BOOLEAN GetFindConcatWords() const;

  // Stuff for virtual databases
  // Segment is the slot number in an array of IDB instances
  void   SetSegment(const STRING newName, int newNumber = -1);
  void   SetSegment(int newNumber);

  STRING      GetSegmentName() const;

  int         Segment(const char * Name);

  bool setUseRelativePaths(bool val);

  void        SetWorkingDirectory(const STRING Dir);
  void        ClearWorkingDirectoryEntry();

  int         SetErrorCode(int Error);
  int         GetErrorCode() const;
  const char *ErrorMessage() const;
  const char *ErrorMessage(int ErrorCode) const;


  bool        FieldExists(const STRING FieldName) const;

#ifdef SWIGPYTHON
%extend {
 PyObject *GetFieldDefinitionList() const {
    STRLIST list;
    self->GetFieldDefinitionList(&list);
    return PyList_FromSTRLIST( list );
  }
}
#else
%newobject GetFieldDefinitionList;
%extend {
  STRLIST *GetFieldDefinitionList() const {
    STRLIST *list = new STRLIST();
    self->GetFieldDefinitionList(list);
    return list;
  }
}
#endif

%extend {
%newobject  GetDocumentInfo ;
  RECORD* GetDocumentInfo (const int Index) {
     RECORD *pRecord = new RECORD();
     self->GetDocumentInfo(Index, pRecord);
     return pRecord;
  }
}

#ifdef SWIGPYTHON
%extend {
 PyObject *GetAllDocTypes() {
    return PyList_FromSTRLIST( self->GetAllDocTypes() );
  }
}
#else
  STRLIST GetAllDocTypes ();
#endif

  MDT   *GetMainMdt() const;

  SRCH_DATE DateCreated() ;
  SRCH_DATE DateLastModified() ;

  void SetCommonWordsThreshold(long x);

  bool CreateCentroid();

  bool SetLocale(const char *LocaleName = NULL);

  bool IsDbCompatible();
  bool IsEmpty() const;
  bool Ok();

  void ffGC();

  void SetMergeStatus(enum MergeStatus MergeStatus);

%extend {
  void SetDbState(int DbState) const {
    enum DbState State = (enum DbState)DbState;
    self->SetDbState(State) ;
  }
}
  int  GetDbState() const;

  // Index and freshness boost factors
  void   SetIndexBoostFactor(double x);
  double GetIndexBoostFactor() const;
  void   SetFreshnessBoostFactor(double x);
  double GetFreshnessBoostFactor() const;
  void   SetLongevityBoostFattor(double x);
  double GetLongevityBoostFactor() const;
  // Freshness dateline
  void      SetFreshnessBaseDateLine(const SRCH_DATE d);
  SRCH_DATE GetFreshnessBaseDateLine() const;

  void     SetDefaultDbSearchCutoff(unsigned x);
  void     SetDbSearchCutoff(unsigned m);
  unsigned GetDbSearchCutoff() const;
  void     SetDbSearchFuel(unsigned Percent);
  void     SetDbSearchCacheSize(unsigned NewCacheSize);

  void   SetDefaultPriorityFactor(double x);
  void   SetPriorityFactor(double x);
  double GetPriorityFactor() const;

  void   SetDbSisLimit(unsigned m);

  void   SetTitle(const STRING NewTitle);
  STRING GetTitle() const;
  void   SetComments(const STRING NewComments);
  STRING GetComments() const;
  void   SetCopyright(const STRING NewCopyright);
  STRING GetCopyright() const;
  void   SetMaintainer(const char * NewName, const char * NewAddress);
  STRING GetMaintainer() const; // Html mailto:
  void   SetGlobalDoctype(const DOCTYPE_ID NewGlobalDoctype);


  void  SetIndexingMemory(const long MemorySize, bool Force=0);
  long  GetIndexingMemory() const;

  void   SetStoplist(const STRING Filename);
  void   SetGlobalStoplist (const STRING NewStoplist);
  STRING GetGlobalStoplist () const;

  long GetTotalWords() const;
  long GetTotalUniqueWords() const;
  unsigned GetTotalRecords() const;
  unsigned GetTotalDocumentsDeleted() const;

  FCACHE *GetFieldCache();

  FC GetPeerFc (GPTYPE HitGp, STRING *INOUT = NULL);
  FC GetPeerFc (FC HitFc,     STRING *INOUT = NULL);

%extend {
  STRING GetFieldName(GPTYPE HitGp) {
     STRING Value;
     self->GetPeerFc(HitGp, &Value);
     return Value;
  }
  STRING GetFieldName (FC HitFc) {
    STRING Value;
    self->GetPeerFc (HitFc, &Value);
    return Value;
  }
}

  TREENODE GetPeerNode (const GPTYPE& HitGp);
  TREENODE GetPeerNode (const FC& HitFc);

  STRING GetPeerContent(FC HitFc);
  STRING GetPeerContentXMLFragement(FC HitFc);

  NODETREE GetNodeTree(GPTYPE HitGp);
  NODETREE GetNodeTree(FC HitFc);

  bool KillCache() const; // Zap the Headline/Present Cache!

  bool FillHeadlineCache();
  bool FillHeadlineCache(const char * RecordSyntax);

  bool IsSystemFile(const char * FileName);

  void   SetServerName(const char * ServerName);
  STRING GetServerName() const;

  bool MergeIndexFiles();
  bool CollapseIndexFiles();

  size_t DeleteExpired();
  size_t DeleteExpired(const SRCH_DATE Now);

  bool KillAll();

  STRING GetVersionID() const;

  void ParseRecords(const RECORD NewRecord);

  bool AddRecord(const RECORD NewRecord);
  bool Index(bool newIndex = 0);
  bool Index1();
  bool Index2();
  %extend {
/*#ifndef SWIGJAVA */
    bool AddRecord(const STRING Filename) {
      static DOCTYPE_ID DefaultDoctype ("AUTODETECT");
      RECORD Record (Filename);
      Record.SetDocumentType(DefaultDoctype);
      return self->AddRecord(Record);
    }
/*#endif */
    bool AppendToIndex(const RECORD Record) {
      self->AddRecord(Record);
      return self->Index(GDT_FALSE);
    }
    bool AppendFileToIndex(const STRING Filename) {
      static DOCTYPE_ID DefaultDoctype ("AUTODETECT");
      RECORD Record (Filename);
      Record.SetDocumentType(DefaultDoctype);
      return self->AddRecord(Record) && self->Index(GDT_FALSE);
    }
  }

  bool IsStopWord (const STRING Word) const;

  unsigned MdtLookupKey (const STRING Key) const;

  bool GetDocumentDeleted(const int Index) const;
  bool DeleteByIndex (const int Index);
  bool DeleteByKey(const STRING Key);
  bool UndeleteByIndex (const int Index);
  bool UndeleteByKey(const STRING Key);

  void SetOverride(bool Flag);
  bool GetOverride() const;

  int CleanupDb();

  int GetLocks() const;

//  SCANLIST Scan(STRING Term) const;
  SCANLIST Scan(STRING Term, int TotalTermsRequested) const;
//  SCANLIST Scan(STRING Term, STRING Field);
  SCANLIST Scan(STRING Term, STRING Field = "", int TotalTermsRequested = 0) const;

  SCANLIST ScanGlob(const STRING Pattern);
  SCANLIST ScanGlob(const STRING Pattern, int TotalTermsRequested);

  SCANLIST ScanGlob(const STRING Pattern, const char *Field);
  SCANLIST ScanGlob(const STRING Pattern, const char *Field, int TotalTermsRequested);

  SCANLIST ScanSearch(const QUERY& Query, const char * Fieldname,  size_t MaxRecordsThreshold = 0);

  void BeginRsetPresent(const char * RecordSyntax);
  void EndRsetPresent(const char * RecordSyntax);

  void   BeforeSearching (QUERY*);
  IRSET *AfterSearching (IRSET* ResultSetPtr);
  void BeforeIndexing ();
  void AfterIndexing ();

  // Standard Search Interface function
  IRSET *Search(const QUERY SearchQuery);

  // Obsolete
  IRSET *Search(const SQUERY SearchQuery, enum SortBy SortBy = Unsorted,
    enum NormalizationMethods = defaultNormalization);

  // Smart Search
#if 1
IRSET *SearchSmart(QUERY *INOUT, const char *DefaultField="");
#else
IRSET *SearchSmart(const QUERY& Query, const char * DefaultField="");
#endif

  RSET *VSearch(const QUERY& SearchQuery);
  RSET *VSearchSmart(QUERY *INOUT, const char *DefaultFIeld="");

  // Show Headline ("B")..
#ifdef SWIGPHP
  STRING Headline(const RESULT ResultRecord, const char * RecordSyntax = (const char *)HtmlRecordSyntax );
#else
  STRING Headline(const RESULT ResultRecord);
  STRING Headline(const RESULT ResultRecord, const char * RecordSyntax );
#endif

  // Summary Field (if available)
#ifdef SWIGPHP
  STRING Summary(const RESULT ResultRecord, const char * RecordSyntax = (const char *)HtmlRecordSyntax );
#else
  STRING Summary(const RESULT ResultRecord);
  STRING Summary(const RESULT ResultRecord, const char * RecordSyntax);
#endif

  // Show first hit...
  STRING Context(const RESULT ResultRecord, const char * Before = NULL, const char * After = NULL);

  // Nth Hit..
  STRING NthContext(unsigned N, const RESULT ResultRecord, const char * Before = NULL,
    const char * After = NULL);

  // Return URL to Source Document
  STRING  URL (const RESULT ResultRecord, bool OnlyRemote = 0);
  // Record Highlighting
  STRING HighlightedRecord(const RESULT ResultRecord, const char * BeforeTerm, const char * AfterTerm);

#ifdef SWIGPHP
  STRING DocHighlight (const RESULT ResultRecord, const char * RecordSyntax = (const char *)HtmlRecordSyntax );
#else
  STRING DocHighlight (const RESULT ResultRecord);
  STRING DocHighlight (const RESULT ResultRecord, const char * RecordSyntax);
#endif

  %extend {
    // Get List of field data elements
#ifdef SWIGPYTHON
    PyObject *GetFieldData(const RESULT *ResultPtr, const char * ESet, DOCTYPE_ID Doctype=0) {
      STRLIST Strlist;
      if (Doctype == 0 && ResultPtr) Doctype =  ResultPtr->GetDocumentType ();
      if (ResultPtr &&
        self->GetFieldData (*ResultPtr, ESet, &Strlist, self->GetDocTypePtr ( Doctype ) ) )
        {
          return PyList_FromSTRLIST(Strlist);
        }
      return  PyList_New (0); // Zero list
    }
#else
%newobject IDB::GetFieldData ;
  STRLIST *GetFieldData(const RESULT *ResultPtr, const char * ESet, DOCTYPE_ID Doctype=0)
    {
      if (ResultPtr)
    {
      STRLIST *StrlistPtr = new STRLIST();
      if (Doctype == 0) Doctype =  ResultPtr->GetDocumentType ();
          self->GetFieldData (*ResultPtr, ESet, StrlistPtr, self->GetDocTypePtr ( Doctype ));
      return StrlistPtr;
    }
      return NULL;
    }
#endif
#ifdef SWIGPYTHON
    // Get List of RAW field data elements
    PyObject *GetFieldContents(const RESULT *ResultPtr, const char * ESet) {
      STRLIST Strlist;
      if (ResultPtr && self->GetFieldData (*ResultPtr, ESet, &Strlist))
        {
          return PyList_FromSTRLIST(Strlist);
        }
      return  PyList_New (0); // Zero list
    }
#else
  ArraySTRING GetFieldContents(const RESULT *ResultPtr, const char * ESet)
    {
      STRLIST Strlist;
      if (ResultPtr)
        self->GetFieldData (*ResultPtr, ESet, &Strlist);
      return ArraySTRING(Strlist);
    }
#endif
  }


  // Present Element Data
#ifdef SWIGPHP
  STRING Present(const RESULT ResultRecord, const char * ElementSet, const char * RecordSyntax = "");
#else
  STRING Present(const RESULT ResultRecord, const char * ElementSet);
  STRING Present(const RESULT ResultRecord, const char * ElementSet, const char * RecordSyntax);
#endif
  // Present Document (containing the element, "F" for fulltext
#ifdef SWIGPHP
  STRING DocPresent(const RESULT ResultRecord, const char * ElementSet, const char * RecordSyntax = "") ;
#else
  STRING DocPresent(const RESULT ResultRecord, const char * ElementSet);
  STRING DocPresent(const RESULT ResultRecord, const char * ElementSet, const char * RecordSyntax) ;
#endif

  // Default is PageField = "Page" and TagElement = "Pg" for <loc Pg=...>
  STRING GetXMLHighlightRecordFormat(const RESULT& Result, const char * PageField = "", const char *TagElement = "");

  int    GetNodeOffsetCount (const GPTYPE HitGp, const char * NodeName = "", FC *ContentFC = NULL,
        FC *ParentFC = NULL);

  FCT    GetDescendentsFCT (const FC& HitFc, const char * NodeName);
  FC     GetAncestorFc (const FC& HitFc, const char * NodeName);

#ifdef SWIGPYTHON
  %extend {
    PyObject*  GetAncestorContent (RESULT& Result, const char *NodeName) {
    STRLIST  Strlist;
    size_t   count;
    if ((count = self->GetAncestorContent (Result, NodeName, &Strlist)) > 0)
      return PyList_FromSTRLIST(Strlist, count);
    return PyList_New (0);
    }
   PyObject *GetDescendentsContent (const FC HitFc, const char * NodeName) {
        STRLIST  Strlist;
        size_t   count;
        if ((count = self->GetDescendentsContent (HitFc, NodeName, &Strlist)) > 0)
      return PyList_FromSTRLIST(Strlist, count);
        return PyList_New (0);
   };
  }
#else
  // TODO: This needs to return a STRLIST
  size_t GetDescendentsContent (const FC HitFc, const char *NodeName, STRLIST *StrlistPtr);
  size_t GetAncestorContent (RESULT& Result, const char *NodeName, STRLIST *StrlistPtr);
#endif

  %extend {
   RESULT* KeyLookup(const STRING Key) {
    RESULT *result = new RESULT();
    if (self->KeyLookup(Key, result) == GDT_FALSE) {
      delete result;
#ifdef SWIGPYTHON
//      PyErr_SetString(PyExc_RuntimeError,  keyErrorMsg);
#endif
      return NULL;
    }
    return result;
   }

   bool KeyExists(const STRING Key) { return self->KeyLookup(Key, (RESULT *)NULL); }

#if defined(SWIGPYTHON)
   PyObject *GetFields(const RESULT* result = NULL) {
    DFDT            Dfdt;
    DFD             Dfd;

    if (result) {
       self->GetRecordDfdt (*result, &Dfdt);
    } else {
      self->GetDfdt(&Dfdt);
    }
    const int       total = Dfdt.GetTotalEntries();
    PyObject *listPtr = PyList_New ( total );
    for (int i=0; i < total; i++) {
      Dfdt.GetEntry(i+1, &Dfd);
      PyList_SetItem(listPtr, i, PyString_FromString(STRINGCAST(Dfd.GetFieldName())));
    }
    return listPtr;
  }
#else
  STRLIST GetFields(const RESULT* result = NULL) {
    DFDT            Dfdt;
    DFD             Dfd;

    if (result) {
       self->GetRecordDfdt (*result, &Dfdt);
    } else {
      self->GetDfdt(&Dfdt);
    }
    const size_t    total = Dfdt.GetTotalEntries();
    STRLIST list;
    for (int i=0; i < total; i++) {
      Dfdt.GetEntry(i+1, &Dfd);
      list.AddEntry ( Dfd.GetFieldName());
    }
    return list;
  }
#endif
 }

  ~IDB();
};

#ifdef SHI

%{
#ifdef IDBC_DEFINED
#include "../../libshi_ib/idbc.hxx"
#endif
%}

#ifdef IDBC_DEFINED
%include idbc.i
#endif

#endif


// Virtual IDB class

%newobject VIDB::Search ;
%newobject VIDB::SearchSmart ;
%newobject VIDB::VSearch ;
%newobject VIDB::VSearchSmart ;

class VIDB:public IDBOBJ {
public:
  VIDB();
#ifndef SWIGJAVA
#ifdef SWIGPYTHON
  VIDB(const STRING  DbFullPrefix);
#else
  VIDB(const STRING DbFullPrefix, const ArraySTRING Options=NulArraySTRING);
#endif
#endif
  VIDB(const STRING DbFullPrefix, bool Searching);
  ~VIDB();

//  const char *Description();

  %extend {
    VIDB &getself() { return *self; }
    IDB   *GetIDB(unsigned idx = 1) { return (IDB *)self->GetIDB(idx); }
  }
  unsigned GetIDBCount() const;
  bool     IsDbVirtual() const;

  MDT   *GetMainMdt() const;
  MDT   *GetMainMdt(unsigned Idx) const; // default = 1

  FCACHE *GetFieldCache();
  FCACHE *GetFieldCache(unsigned Idx); // default = 1;

  %extend {
#ifdef SWIGPYTHON
   PyObject *GetDocTypeOptions() {
    return PyList_FromSTRLIST( self->GetDocTypeOptions() );
   }
#else
    ArraySTRING GetDocTypeOptions() const {
      return ArraySTRING(self->GetDocTypeOptions());
    }
#endif
  }

  STRING   GetDbFileStem(int Idx = 0) const; // was STRING


  STRING   XMLHitTable(const RESULT& Result);

  STRING   XMLNodeTree (const RESULT ResultRecord, FC Fc);

  void     SetPriorityFactor(double x, unsigned idx = 0);
  void     SetDbSearchCutoff(unsigned m, unsigned idx = 0);
  unsigned GetDbSearchCutoff();
  void     SetDbSearchFuel(unsigned Percent, unsigned idx = 0);

  void     SetDbSearchCacheSize(unsigned NewCacheSize, unsigned idx = 0);

  void BeforeSearching (QUERY *SearchQuery);

  void SetDebugMode(bool OnOff);

  int         GetErrorCode(const int Idx = 0);
  const char *ErrorMessage(const int Idx = 0) const;

  long GetTotalWords(const int Idx = 0);
  long GetTotalUniqueWords(const int Idx = 0);

  unsigned GetTotalRecords(const int Idx = 0);
  unsigned GetTotalDocumentsDeleted(const int Idx = 0);
  unsigned GetTotalDatabases();

  bool IsDbCompatible();
  bool IsEmpty() const;
  bool Ok();

  void SetCommonWordsThreshold(long x);

  void SetStoplist(const STRING Filename);
  bool IsStopWord (const STRING Word) ;

// Virtual Database Information
  STRING GetTitle(const int Idx=0) ;
  STRING GetComments(const int Idx=0) ;
  STRING GetMaintainer(const int Idx=0) ;


#if 1
  IRSET *SearchSmart(QUERY *INOUT, const char * DefaultField="");
#else
  IRSET *SearchSmart(const QUERY& Query, const char * DefaultField="");
#endif

  IRSET *Search(const QUERY& Query);
  IRSET *Search(const QUERY& Query, VIDB_STATS *INOUT);

  // Obsolete stuff
#ifndef SWIGJAVA
  IRSET *Search(const SQUERY SearchQuery, enum SortBy Sort = Unsorted,
        enum NormalizationMethods Method = defaultNormalization);
#endif
#if 0 /* OBSOLETE */
  // Convienience methods (mainly for use by scripts)
  IRSET *SearchWords(const char * Words, enum SortBy SortBy = Unsorted,
    enum NormalizationMethods Method = defaultNormalization); // All Words are taken as OR'd
  IRSET *SearchRpn(const char * RpnQuery, enum SortBy SortBy = Unsorted,
    enum NormalizationMethods Method = defaultNormalization); // String is considered an RPN expression
  IRSET *SearchInfix(const char * InfixQuery, enum SortBy SortBy = Unsorted,
    enum NormalizationMethods Method = defaultNormalization); // String is considered an Infix expression
#endif

  // Combined
 RSET *VSearch(const QUERY& Query);
 RSET *VSearchSmart(QUERY *INOUT, const char *DefaultFIeld="");

#if 0 /* OBSOLETE */
  RSET *VSearchWords(const char *Words, enum SortBy SortBy = Unsorted, unsigned Total = 300,
    size_t * TotalHits = NULL, enum NormalizationMethods Method = defaultNormalization);
  RSET *VSearchRpn(const char *RpnQuery, enum SortBy SortBy = Unsorted, unsigned Total = 300,
    size_t * TotalHits = NULL, enum NormalizationMethods Method = defaultNormalization);
  RSET *VSearchInfix(const char *InfixQuery, enum SortBy SortBy = Unsorted, unsigned Total = 300,
    size_t * TotalHits = NULL, enum NormalizationMethods Method = defaultNormalization);
  RSET  *VSearchSmart(const char *Query, const STRING DefaultField = "",
        enum SortBy Sort = Unsorted, unsigned Total = 300,
    size_t * TotalHits = NULL, enum NormalizationMethods Method = defaultNormalization);
#endif

  SCANLIST Scan(const STRING Term) const;
  SCANLIST Scan(const STRING Term, const int TotalTermsRequested);
  SCANLIST Scan(const STRING Term, const char * Field);
  SCANLIST Scan(const STRING Term, const char * Field, const int TotalTermsRequested );

  SCANLIST ScanGlob(const STRING Pattern);
  SCANLIST ScanGlob(const STRING Pattern, const INT TotalTermsRequested);

  SCANLIST ScanGlob(const STRING Pattern, const char * Field);
  SCANLIST ScanGlob(const STRING Pattern, const char * Field, const INT TotalTermsRequested);

  SCANLIST ScanSearch(const QUERY& Query, const char * Fieldname);
  SCANLIST ScanSearch(const QUERY& Query, const char * Fieldname,  size_t MaxRecordsThreshold);

  void BeginRsetPresent(const char * RecordSyntax);

#ifdef SWIGPYTHON
  %extend {
    PyObject*  GetAncestorContent (RESULT& Result, const char *NodeName) {
        STRLIST  Strlist;
        size_t   count;
        if ((count = self->GetAncestorContent (Result, NodeName, &Strlist)) > 0)
      return PyList_FromSTRLIST(Strlist, count);
        return PyList_New (0);
    }
  }
#else
 size_t  GetAncestorContent (RESULT& Result, const char *NodeName, STRLIST *StrlistPtr);
#endif


  // Show Headline ("B")..
#ifdef SWIGPHP
  STRING Headline(const RESULT ResultRecord, const char * RecordSyntax = (const char *)HtmlRecordSyntax);
#else
  STRING Headline(const RESULT ResultRecord);
  STRING Headline(const RESULT ResultRecord, const char * RecordSyntax);
#endif

  // Summary Field (if available)
#ifdef SWIGPHP
  STRING Summary(const RESULT ResultRecord, const char * RecordSyntax = (const char *)HtmlRecordSyntax);
#else
  STRING Summary(const RESULT ResultRecord);
  STRING Summary(const RESULT ResultRecord, const char * RecordSyntax);
#endif

  // Show first hit...
  STRING Context(const RESULT ResultRecord, const STRING Before, const STRING After);
  STRING Context(const RESULT ResultRecord);

  // Nth Hit..
  STRING NthContext(unsigned N, const RESULT ResultRecord);
  STRING NthContext(unsigned N, const RESULT ResultRecord, const STRING Before, const STRING After);

  // Return URL to Source Document
  STRING  URL (const RESULT ResultRecord, bool OnlyRemote = GDT_FALSE);

  // Record Highlighting
  STRING HighlightedRecord(const RESULT ResultRecord, const STRING BeforeTerm, const STRING AfterTerm) ;

  STRING DocHighlight (const RESULT ResultRecord);
  STRING DocHighlight (const RESULT ResultRecord, const char * RecordSyntax);


  %extend {
    // Get List of field data elements
#ifdef SWIGPYTHON
    PyObject *GetFieldData(const RESULT *ResultPtr, const char * ESet, DOCTYPE_ID Doctype=0) {
      STRLIST Strlist;
      if (Doctype == 0 && ResultPtr) Doctype =  ResultPtr->GetDocumentType ();
      if (ResultPtr &&
        self->GetFieldData (*ResultPtr, ESet, &Strlist, self->GetDocTypePtr ( Doctype ) ))
        {
          return PyList_FromSTRLIST(Strlist);
        }
      return  PyList_New (0); // Zero list
    }
    // Get List of RAW field data elements
    PyObject *GetFieldContents(const RESULT *ResultPtr, const char * ESet) {
      STRLIST Strlist;
      if (ResultPtr && self->GetFieldData (*ResultPtr, ESet, &Strlist))
    {
      return PyList_FromSTRLIST(Strlist);
    }
      return  PyList_New (0); // Zero list
    }
#else
%newobject VIDB::GetFieldData ;
    STRLIST *GetFieldData(const RESULT *ResultPtr, const char * ESet, DOCTYPE_ID Doctype=0) {
      if (ResultPtr)
    {
      STRLIST *StrlistPtr = new STRLIST();
      if (Doctype == 0) Doctype =  ResultPtr->GetDocumentType ();
      if (self->GetFieldData (*ResultPtr, ESet, StrlistPtr, self->GetDocTypePtr ( Doctype )) )
        return StrlistPtr;
      delete[] StrlistPtr;
    }
      return NULL; // NULL
    }
    // Get List of RAW field data elements
    ArraySTRING GetFieldContents(const RESULT *ResultPtr, const char * ESet) {
      STRLIST Strlist;
      if (ResultPtr && self->GetFieldData (*ResultPtr, ESet, &Strlist))
        {
          return ArraySTRING(Strlist);
        }
      return ArraySTRING (0); // Zero list
    }
#endif
  }


  // Present Element Data
  STRING Present(const RESULT ResultRecord, const char * ElementSet);
  STRING Present(const RESULT ResultRecord, const char * ElementSet, const char * RecordSyntax) ;

  // Present Document (containing the element, "F" for fulltext

  STRING DocPresent(const RESULT ResultRecord, const char * ElementSet) ;
  STRING DocPresent(const RESULT ResultRecord, const char * ElementSet,
    const char * RecordSyntax) ;

  void EndRsetPresent(const char * RecordSyntax);

  STRING GetGlobalDocType() ;

  %extend {
    RESULT* KeyLookup(const STRING Key) {
      RESULT *result = new RESULT();
      if (self->KeyLookup(Key, result) == GDT_FALSE)
    {
      delete result;
#ifdef SWIGPYTHON
//    PyErr_SetString(PyExc_RuntimeError,  keyErrorMsg);
#endif
      return NULL;
    }
      return result;
    }

    bool KeyExists(const STRING Key) { return self->KeyLookup(Key, NULL); }
  }

  bool SetDateRange(const DATERANGE DateRange);

  STRING ProfileGetString(const STRING Section, const STRING Entry, const STRING Default="");

  STRING FirstKey() ;
  STRING LastKey() ;
  STRING NextKey(const STRING Key) ;
  STRING PrevKey(const STRING Key) ;

#ifdef SWIGPYTHON
%newobject GetDocumentInfo ;
%extend {
  RECORD *GetDocumentInfo (int Index) {
    RECORD *pRecord = new RECORD();
    self->GetDocumentInfo(Index, pRecord);
    return pRecord;
  }
  RECORD *GetDocumentInfo (int Idx, int Index) {
    RECORD *pRecord = new RECORD();
    self->GetDocumentInfo(Idx, Index, pRecord);
    return pRecord;
  }
}
#else
  bool GetDocumentInfo (const int Idx, const int Index, RECORD *RecordBuffer) ;
#endif

  SRCH_DATE DateCreated() ;
  SRCH_DATE DateLastModified() ;

#ifdef SWIGPYTHON
%extend {
  PyObject *GetAllDocTypes() {
    return PyList_FromSTRLIST( self->GetAllDocTypes() );
  }
}
#else
  STRLIST GetAllDocTypes ();
#endif

  bool ValidateDocType(const char * DocType) ;

  STRING GetVersionID() ;

  int GetLocks() ;
  %extend {
#ifdef SWIGPYTHON
   PyObject *GetFields(const RESULT* result = NULL) {
    DFDT            Dfdt;
    DFD             Dfd;

    if (result) {
       self->GetRecordDfdt (*result, &Dfdt);
    } else
      self->GetDfdt(&Dfdt);
    const int       total = Dfdt.GetTotalEntries();
    PyObject *listPtr = PyList_New ( total );
    for (int i=0; i < total; i++) {
      Dfdt.GetEntry(i+1, &Dfd);
      PyList_SetItem(listPtr, i, PyString_FromString(STRINGCAST(Dfd.GetFieldName())));
    }
    return listPtr;
  }

#else
  ArraySTRING GetFields(const RESULT* result = NULL) {
    DFDT            Dfdt;
    DFD             Dfd;
    ArraySTRING     list;

    if (result) {
       self->GetRecordDfdt (*result, &Dfdt);
    } else {
      self->GetDfdt(&Dfdt);
    }
    const size_t    total = Dfdt.GetTotalEntries();
    ArraySTRING     List(total);
    for (int i=0; i < total; i++) {
      Dfdt.GetEntry(i+1, &Dfd);
      list.Add ( Dfd.GetFieldName());
    }
    return list;
  }
#endif
 }

};

class LANGUAGE {
public:
  LANGUAGE(const char *Name);
  const char * Name() const;
  const char * Code() const;

  ~LANGUAGE();
};

class CHARSET {
public:
  CHARSET(const char * Name);

  bool Ok() const;

  %extend {
    const char * Name() const {
      return (const char *)self;
    }
  }

  STRING HtmlCat (const STRING Input, bool Anchor=GDT_TRUE) const;

  STRING  ToLower(const STRING String) const;
  STRING  ToUpper(const STRING String) const;

  // Character classification functions
  int ib_isalpha(char c) const;
  int ib_isupper(char c) const;
  int ib_islower(char c) const;
  int ib_isdigit(char c) const;
  int ib_isxdigit(char c) const;
  int ib_isalnum(char c) const;
  int ib_isspace(char c) const;
  int ib_ispunct(char c) const;
  int ib_isprint(char c) const;
  int ib_isgraph(char c) const;
  int ib_iscntrl(char c)  const;
  int ib_iswhite(char c) const;
  int ib_isascii(char c) const;
  int ib_islatin1(char c) const;

  int ib_toupper(char c) const;
  int ib_tolower(char c) const;
  int ib_toascii(char c) const;

  int isTermChr(char c) const;
  int isWordSep(char c) const;
  int isTermWhite(char c) const;

  %extend {
    int  UCS(char Ch) { return self->UCS((UCHR)Ch); }
  }

  // Destruction
  ~CHARSET();
};

class LOCALE {
public:
  LOCALE();
  ~LOCALE();

  LOCALE SetLanguage(LANGUAGE Other);
  LOCALE SetCharset(CHARSET Other);

  const char *GetLanguageCode () const; // ISO Language Codes
  const char *GetLanguageName () const; // Long Language Name
  const char *GetCharsetCode () const; // IANA names
  const char *GetCharsetName () const; // Registry Name

  CHARSET  Charset() const;
  LANGUAGE Language() const;

  %extend {
    STRING Name() const {
      return self->LocaleName();
    }
  }
  int      Id() const;
};



// Config
STRING  ResolveConfigPath(const STRING Filename);
STRING  ResolveBinPath (const STRING filename);
//STRING  ResolveHtdocPath(const STRING Filename, bool AsUrl = 1);

// Nice to have procedures
STRING ExpandFileSpec (const STRING FileSpec);


// Log messages
%constant const int LOG_PANIC=  (1 << 0); /* system is unusable */
%constant const int LOG_FATAL=  (1 << 1); /* fatal error conditions */
%constant const int LOG_ERROR=  (1 << 2); /* error conditions */
%constant const int LOG_ERRNO=  (1 << 3); /* error conditions (system) */
%constant const int LOG_WARN=   (1 << 4); /* warning conditions */
%constant const int LOG_NOTICE= (1 << 5); /* normal but signification condition */
%constant const int LOG_INFO=   (1 << 6); /* informational */
%constant const int LOG_DEBUG=  (1 << 7); /* debug info */
%constant const int LOG_ALL  =0xffff;

// log_init for the 3rd arg supports:
//  - filename
//  - <stdout> for stdout
//  - <stderr> for stderr
//  - <stdin>  for stdin
//  - <syslog> for the syslogger
%constant char * DEVICE_STDERR = "<stderr>";
%constant char * DEVICE_STDOUT = "<stdout>";
%constant char * DEVICE_SYSLOG = "<syslog>";

// SYSLOG "devices"
%constant char * DEVICE_LOCAL0 = "<syslog0>";
%constant char * DEVICE_LOCAL1 = "<syslog1>";
%constant char * DEVICE_LOCAL2 = "<syslog2>";
%constant char * DEVICE_LOCAL3 = "<syslog3>";
%constant char * DEVICE_LOCAL4 = "<syslog4>";
%constant char * DEVICE_LOCAL5 = "<syslog5>";
%constant char * DEVICE_LOCAL6 = "<syslog6>";
%constant char * DEVICE_LOCAL7 = "<syslog7>";

bool  set_syslog(const char *device);

bool  log_init (int level, const char *prefix=NULL, const char *name=NULL);
void  log_message (int level, const char *string);

// Misc Functions
%{

class __IB {
 public:

  bool FileGlob(const STRING pattern, const STRING str) const {
    return (bool)::FileGlob(pattern.c_ustr(), str.c_ustr());
  }
  bool Glob(const STRING pattern, const STRING str, bool dot_special=0) const {
    return (bool)::Glob(pattern.c_ustr(), str.c_ustr(), dot_special);
  }

  int FileLink(const STRING Source, const STRING Dest) const {
    return ::FileLink(Source, Dest);
  }
  STRING GetUserHome(const STRING user) const {
    return ::GetUserHome(user);
  }
  bool DirectoryExists(const STRING Path) const {
    return ::DirectoryExists(Path);
  }
  bool FileExists(const STRING Path) const {
    return ::FileExists(Path);
  }
  bool ExeExists(const STRING Path) const {
    return ExeExists(Path);
  }

  long GetFreeMemory() const  { return ::_IB_GetFreeMemory();  }
  long GetTotalMemory() const { return ::_IB_GetTotalMemory(); }
  long Hostid() const         { return ::_IB_Hostid();         }
  long SerialID() const       { return ::_IB_SerialID();       }

  void SendDebugMessage(const char *msg)   { message_log(LOG_DEBUG, msg); }
  void SendInfoMessage(const char *msg)    { message_log(LOG_INFO, msg);  }
  void SendNoticeMessage(const char *msg)  { message_log(LOG_NOTICE, msg);}
  void SendWarningMessage(const char *msg) { message_log(LOG_WARN, msg);  }
  void SendErrorMessage(const char *msg)   { message_log(LOG_ERROR, msg); }
  void SendErrnoMessage(const char *msg)   { message_log(LOG_ERRNO, msg); }
  void SendFatalMessage(const char *msg)   { message_log(LOG_FATAL, msg); }
  void SendPanicMessage(const char *msg)   { message_log(LOG_PANIC, msg); }

};
%}

class INODE {
public:
  INODE();
  INODE(const STRING Path);

  void Clear();

#if !defined(SWIGPERL) && !defined(SWIGJAVA)
  INODE& operator =(const INODE& Other);
#endif

  GDT_BOOLEAN Set(const STRING Path);
  GDT_BOOLEAN Set(FILE *fp);
  GDT_BOOLEAN Set(int fd);

  STRING Key() const;

  bool isLinked() const;

  bool isDangling() const;
  int inode() const;
  int device() const;
} ;


class __IB {
 public:

/*
  long GetFreeMemory();
  long GetTotalMemory();
  long Hostid();
  long SerialID();
*/

  bool FileGlob(const STRING pattern, const STRING str);
  bool Glob(const STRING pattern, const STRING str, bool dot_special=0);

  STRING GetUserHome(const STRING user);

  bool DirectoryExists(const STRING Path);
  bool FileExists(const STRING Path); // Exists and not a directory
  bool ExeExists(const STRING Path); // Is executable (script or bin)

  int FileLink(const STRING Source, const STRING Dest);

  // These are really just convienient aliases for log_message
  void SendDebugMessage(const char *msg);
  void SendInfoMessage(const char *msg);
  void SendNoticeMessage(const char *msg);
  void SendWarningMessage(const char *msg);
  void SendErrorMessage(const char *msg);
  void SendErrnoMessage(const char *msg);
  void SendFatalMessage(const char *msg);
  void SendPanicMessage(const char *msg);

};

%pragma(python) include="ib_overrides.py"
%pragma(python) include="_extras.py"


#ifndef SWIGJAVA

%init %{
  __Register_IB_Application(SWIG_name, stdout, DebugFlag);
  if (DebugFlag) message_log (LOG_DEBUG, "%s completed!", "init");
%}

#endif


