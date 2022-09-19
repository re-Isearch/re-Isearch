// "%Z%%Y%%M%  %I% %G% %U% BSN"
%include exception.i
%include typemaps.i


#define GDT_FALSE 0
//#define false 0
#define GDT_TRUE 1
//#define true 1

/*#ifdef O_BUILD_IB64
*/

//typedef unsigned long long GPTYPE ;
//#define GPTYPE unsigned long long
typedef unsigned long long GPTYPE;

/*#else

//typedef unsigned GPTYPE ;
#define GPTYPE unsigned

#endif
*/


#ifdef SWIGPYTHON
%{
//--------------- Python Helper Functions --------------------------------------

extern int*    int_LIST_helper(PyObject* source);
extern long*   long_LIST_helper(PyObject* source);
extern char**  string_LIST_helper(PyObject* source);
extern STRING* STRING_LIST_helper(PyObject* source);


extern ArraySTRING* ArraySTRING_FromPyList(PyObject *source);
extern STRINGS*  STRINGS_FromPyList(PyObject* source);
extern STRLIST*  STRLIST_FromPyList(PyObject* source);

extern PyObject* PyList_FromArraySTRING(const ArraySTRING& List);
extern PyObject* PyList_FromArraySTRING(const ArraySTRING *List);
extern PyObject* PyList_FromSTRLIST(const STRLIST& List);
extern PyObject* PyList_FromSTRLIST(const STRLIST& List, const int entries);
extern PyObject* PyList_FromSTRLIST(const STRLIST* ListPtr);
extern PyObject* PyList_FromSTRLIST(const STRLIST* ListPtr, const int entries);
extern PyObject* PyList_FromFCT(const FCT list);
extern PyObject* PyList_FromFCT(const FCT *Fct);
%}
#endif /* SWIGPYTHON */


//---------------------------------------------------------------------------
// Tell SWIG to wrap all the wrappers with Python's thread macros

#ifdef SWIGPYTHON
%exception {
    Py_BEGIN_ALLOW_THREADS;
    $function
    Py_END_ALLOW_THREADS;
}
#endif

//---------------------------------------------------------------------------

//%typemap(python,ignore) float          *T_OUTPUT_TOINT(float temp),
//                        double         *T_OUTPUT_TOINT(double temp)
//{
//  $target = &temp;
//}
//
//
//%typemap(python,argout) float          *T_OUTPUT_TOINT,
//                        double         *T_OUTPUT_TOINT
//{
//    PyObject *o;
//    o = PyInt_FromLong((long) (*$source));
//    $target = t_output_helper($target, o);
//}

#ifdef SWIGPYTHON

%typemap(out) FCT {
    $target = PyList_FromFCT($source);
}

%typemap(out) FCT* {
    $target = PyList_FromFCT($source);
    if ($target == NULL) return NULL;
}


%typemap(freearg) FCT* {
   delete $source; // freearg
}


// FILE pointers to internal

%typemap(in) FILE * {
   $target = PyFile_AsFile($source);
}

%typemap(out) FILE * {
   $target = PyFile_FromFile($source, "unknown", "?", fclose);
}



//------------------------------------------------------------------------------

// Here are some to map (int LCOUNT, int* LIST), etc. from a python list

%typemap(build) int LCOUNT {
    if (_in_LIST) {
        $target = PyList_Size(_in_LIST);
    }
    else {
        $target = 0;
    }
}

%typemap(in) int* LIST  {
    $target = int_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) int* LIST {
    delete [] $source;
}

%typemap(in) unsigned* LIST  {
    $target = (unsigned *)int_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) unsigned* LIST {
    delete [] $source;
}


%typemap(in) long* LIST {
    $target = long_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) long* LIST {
    delete [] $source;
}


%typemap(in) unsigned long* LIST {
    $target = (unsigned long*)long_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) unsigned long* LIST {
    delete [] $source;
}

%typemap(in) char** LIST {
    $target = string_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) char** LIST {
    delete [] $source;
}
#endif

//---------------------------------------------------------------------------

%{
static char* StringErrorMsg = (char *)"string type is required for parameter";
%}

typedef const STRING& constStringRef ;
typedef STRING&       StringRef ;
typedef const STRING  constString ;
typedef STRING        String ;


//
//%typemap(python,argout) STRING   *OUTPUT
//{
//  $target = STRING_output_helper($target,  PyString_AsString((*$source)) );
//}

#ifdef SWIGJAVA

%naturalvar STRING;

class STRING;

// STRING
%typemap(jni) STRING "jstring"
%typemap(jtype) STRING "String"
%typemap(jstype) STRING "String"
%typemap(javadirectorin) STRING "$jniinput"
%typemap(javadirectorout) STRING "$javacall"

%typemap(in) STRING
%{ if(!$input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null STRING");
    return $null;
   }
#if defined(_MSDOS) || defined(_WIN32) || defined(WIN32)
   {
        int length = jenv->GetStringLength($input/*jstr*/);
        const jchar* jcstr = jenv->GetStringChars($input/*jstr*/, 0 );
        int size   = 0;
        int maxLen = length*3 + 1;

#ifdef __GNUG__
    char rtn[ maxLen];
#elif defined(NO_ALLOCA)
        char* rtn = (char*)malloc( maxLen );
#else
    char *rtn = (char *)alloca( maxLen );
#endif
    memset(rtn, '\0', maxLen);

/* Microsoft says:
"Security Alert   Using the WideCharToMultiByte function incorrectly can compromise the
security of your application. Calling this function can easily cause a buffer overrun
because the size of the input buffer indicated by lpWideCharStr equals the number of
WCHAR values in the string, while the size of the output buffer indicated by lpMultiByteStr
equals the number of bytes. To avoid a buffer overrun, your application must specify a
buffer size appropriate for the data type the buffer receives.  "
*/
    // WIN32API: WideCharToMultiByte
    // UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr,
    // int cbMultiByte, LPCSTR lpDefaultChar,    LPBOOL lpUsedDefaultChar
    // CP_ACP := The current system Windows ANSI code page. This value can be different on
    // different computers, even on the same network. It can be changed on the same computer,
        // leading to stored data becoming irrecoverably corrupted.
        if ((size = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)jcstr, length, rtn, maxLen, NULL, NULL )) > 0)
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        rtn[size] = 0;
        $1 = rtn;
      }
      else
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        size = 0;
        rtn[size] = 0;
        $1 = rtn;
      }
#if !defined( __GNUG__) && defined(NO_ALLOCA)
    free(rtn);
#endif
    if (rtn[0] == 0) return $null; // Empty string
   }
#else
   {
       const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0);
       if (!$1_pstr) return $null;
       $1 =  STRING($1_pstr);
       jenv->ReleaseStringUTFChars($input, $1_pstr);

   }
#endif
%}



%typemap(directorout) STRING
%{ if(!$input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null STRING");
    return $null;
   }
#if defined(_MSDOS) || defined(_WIN32) || defined(WIN32)
   {
        int length = jenv->GetStringLength($input/*jstr*/);
        const jchar* jcstr = jenv->GetStringChars($input/*jstr*/, 0 );
        int size   = 0;
        int maxLen = length*3 + 1;

#ifdef __GNUG__
    char rtn[ maxLen];
#elif defined(NO_ALLOCA)
        char* rtn = (char*)malloc( maxLen );
#else
    char *rtn = (char *)alloca( maxLen );
#endif
    memset(rtn, '\0', maxLen);
    // WIN32API: WideCharToMultiByte
    // UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr,
    // int cbMultiByte, LPCSTR lpDefaultChar,    LPBOOL lpUsedDefaultChar
    // CP_ACP := The current system Windows ANSI code page. This value can be different on
    // different computers, even on the same network. It can be changed on the same computer,
        // leading to stored data becoming irrecoverably corrupted.
        if ((size = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)jcstr, length, rtn, maxLen, NULL, NULL )) > 0)
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        rtn[size] = 0;
        $1 = rtn;
      }
      else
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        size = 0;
        rtn[size] = 0;
        $1 = rtn;
      }
#if !defined( __GNUG__) && defined(NO_ALLOCA)
    free(rtn);
#endif
    if (rtn[0] == 0) return $null; // Empty string
   }
#else
   {
       const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0);
       if (!$1_pstr) return $null;
       $1 =  STRING($1_pstr);
       jenv->ReleaseStringUTFChars($input, $1_pstr);

   }
#endif
%}

%typemap(directorin,descriptor="Ljava/lang/String;") STRING
%{ $input = jenv->NewStringUTF($1.c_str()); %}

%typemap(out) STRING
%{ $result = jenv->NewStringUTF($1.c_str()); %}

%typemap(javain) STRING "$javainput"

%typemap(javaout) STRING {
    return $jnicall;
  }

%typemap(typecheck) STRING = char *;

%typemap(throws) STRING
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

// const STRING &
%typemap(jni) const STRING& "jstring"
%typemap(jtype) const STRING& "String"
%typemap(jstype) const STRING& "String"
%typemap(javadirectorin) const STRING& "$jniinput"
%typemap(javadirectorout) const STRING& "$javacall"

%typemap(in) const STRING& (STRING temp)
%{ if(!$input) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null STRING");
    return $null;
   }
#if defined(_MSDOS) || defined(_WIN32) || defined(WIN32)
   {
        int length = jenv->GetStringLength($input/*jstr*/);
        const jchar* jcstr = jenv->GetStringChars($input/*jstr*/, 0 );
        int size   = 0;
        int maxLen = length*3 + 1;

#ifdef __GNUG__
    char rtn[ maxLen];
#elif defined(NO_ALLOCA)
        char* rtn = (char*)malloc( maxLen );
#else
    char *rtn = (char *)alloca( maxLen );
#endif
    memset(rtn, '\0', maxLen);
    // WIN32API: WideCharToMultiByte
    // UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr,
    // int cbMultiByte, LPCSTR lpDefaultChar,    LPBOOL lpUsedDefaultChar
    // CP_ACP := The current system Windows ANSI code page. This value can be different on
    // different computers, even on the same network. It can be changed on the same computer,
        // leading to stored data becoming irrecoverably corrupted.
        if ((size = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)jcstr, length, rtn, maxLen, NULL, NULL )) > 0)
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        rtn[size] = 0;
        temp = rtn;
        $1 = &temp;
      }
      else
      {
        jenv->ReleaseStringChars($input/*jstr*/, jcstr);
        size = 0;
        rtn[size] = 0;
        temp = rtn;
        $1 = &temp;
      }
#if !defined( __GNUG__) && defined(NO_ALLOCA)
    free(rtn);
#endif
    if (rtn[0] == 0) return $null; // Empty string
   }
#else
   {
       const char *$1_pstr = (const char *)jenv->GetStringUTFChars($input, 0);
       if (!$1_pstr) return $null;
       temp = $1_pstr;
       $1 = &temp;
       jenv->ReleaseStringUTFChars($input, $1_pstr);
   }
#endif
%}

%typemap(directorin,descriptor="Ljava/lang/String;") const STRING&
%{ $input = jenv->NewStringUTF($1.c_str()); %}

%typemap(out) const STRING&
%{ $result = jenv->NewStringUTF($1->c_str()); %}

%typemap(javain) const STRING& "$javainput"

%typemap(javaout) const STRING& {
    return $jnicall;
  }

%typemap(typecheck) const STRING& = char *;

%typemap(throws) const STRING&
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, $1.c_str());
   return $null; %}

#endif

#ifdef SWIGPERL
%typemap(argout) STRING  &argout {
  $target = sv_newmortal();
  sv_setpvn($target, ($source)->c_str(), ($source)->length());
  argvi++;
}

%typemap(ignore) STRING &argout ($basetype string_argout) {
  $target = &string_argout;
}

%typemap(in) const STRING&, STRING& {
  $target = SvPV($source, PL_na);
  // const char* str = SvPV($source, PL_na);
  // $target =  new STRING(str, strlen(str)); */
}


%typemap(out) const STRING&, STRING&, const STRING, STRING  {
  ST(argvi) = sv_newmortal();
  sv_setpv(ST(argvi++), (char *) (result).c_str());
}


%typemap(ret) const STRING&, STRING&, STRING  {
 //   if ($source) delete $source; // NOT A POINTER ANYMORE
}

#endif

#ifdef SWIGPYTHON

/* Overloading check */

%typemap(typecheck) STRING = char *;
%typemap(typecheck) const STRING& = char *;

%typemap(in) STRING {
  if (PyString_Check($input))
    $1 = STRING(PyString_AsString($input), PyString_Size($input));
  else
    SWIG_exception(SWIG_TypeError,  StringErrorMsg);
}

%typemap(in) const STRING& (STRING temp) {
  if (PyString_Check($input)) {
    temp = STRING(PyString_AsString($input), PyString_Size($input));
    $1 = &temp;
  } else {
    SWIG_exception(SWIG_TypeError,  StringErrorMsg);
  }
}

%typemap(out) STRING {
        $result = PyString_FromStringAndSize($1.c_str(),$1.size());
}

%typemap(out) const STRING& {
        $result = PyString_FromStringAndSize($1->c_str(),$1->size());
}

%typemap(directorin, parse="s") STRING, const STRING&, STRING& "$1_name.c_str()";

%typemap(directorin, parse="s") STRING *, const STRING * "$1_name->c_str()";

%typemap(directorout) STRING {
  if (PyString_Check($input))
    $result = STRING(PyString_AsString($input), PyString_Size($input));
  else
    throw Swig::DirectorTypeMismatchException( StringErrorMsg);
}

%typemap(directorout) const STRING& (STRING temp) {
  if (PyString_Check($input)) {
    temp = STRING(PyString_AsString($input), PyString_Size($input));
    $result = &temp;
  } else {
    throw Swig::DirectorTypeMismatchException(  StringErrorMsg );
  }
}

#endif

#ifdef SWIGTCL8

%typemap(in) const STRING&, STRING&  {
  const char *str = Tcl_GetStringFromObj($source,NULL);
  $target = new STRING( str, strlen(str) );
}
%typemap(out) const STRING&, STRING&, const STRING, STRING {
    Tcl_SetStringObj( Tcl_GetObjResult(interp), (char *)($source).c_str(),  ($source).length());
}
%typemap(tcl8, ret) const STRING&, STRING&, STRING  {
    // if ($source) delete $source;
}

#endif


#ifdef SWIGPHP4

%typemap(in) STRING {
  convert_to_string_ex($input);
  $1 = STRING(Z_STRVAL_PP($input));
}

%typemap(in) const STRING& (STRING temp) {
  convert_to_string_ex($input);
  temp = STRING(Z_STRVAL_PP($input));
  $1 = &temp;
}

%typemap(out) STRING {
  ZVAL_STRINGL($result,const_cast<char*>($1.c_str()),$1.length(),1);
}

%typemap(out) const STRING& {
  ZVAL_STRINGL($result,const_cast<char*>($1->c_str()),$1->length(),1);
}

#endif



//%typemap(freearg) const STRING&, STRING&, STRING {
//    if ($target) delete $source;
//}

#ifdef SWIGPYTHON

%typemap(in) STRING *{
   STRING tmp_string(PyString_AsString($source), PyString_Size($source));
   $target=&tmp_string;
}

%typemap(out) STRING * {
    $target = PyString_FromString(($source)->c_str());
    if ($target == NULL) return NULL;
}
#endif
//
// Annoying: match when returning a ref or a pointer.
// If it's a ref, the _result is really a pointer to
// a local variable that cannot be free'd. If it's a
// pointer, the _result contains the pointer and we
// probably want to free it (check other typemap facilities
// to solve this ?)
//

#ifdef SWIGPERL
// Perl5

%typemap(in) STRING * ($basetype string_temp) {
  if (!SvPOK($source)) croak("Argument $argnum is not a string");
  STRLEN len;
  char *ptr = SvPV($source, len);
  string_temp.assign(ptr, len);
  $target = &string_temp;
}

%typemap(out) STRING * {
  /* STRING * */
  ST(argvi) = sv_newmortal();
  sv_setpvn(ST(argvi++), (char *)(result)->c_str(), result->length());
}

#endif

#ifdef SWIGTCL

// Tcl8
%typemap(out) STRING* {
   Tcl_SetStringObj( Tcl_GetObjResult(interp), (char *)($source)->c_str(),  ($source)->length());
}

// This tells SWIG to treat char ** as a special case
%typemap(in) const char ** {
  int tempc;
  if (Tcl_SplitList(interp, (char *)($source),&tempc,&$target) == TCL_ERROR)
    return TCL_ERROR;
}

// This gives SWIG some cleanup code that will get called after the function call
%typemap(freearg) char ** { free((char *) $source);
}

// Return a char ** as a Tcl list
%typemap(out) const char ** {
  int i = 0;
  while ($source[i]) {
    Tcl_AppendElement(interp,$source[i]);
    i++;
  }
}
#endif

#ifdef SWIGPYTHON

%typemap(in) STRING* LIST  {
    $target = STRING_LIST_helper($source);
    if ($target == NULL) {
        return NULL;
    }
}
%typemap(freearg) STRING* LIST {
    delete [] $source;
}


// STRLIST
%typemap(in) STRLIST&, const STRLIST&, STRLIST, const STRLIST {
  STRLIST *l =  STRLIST_FromPyList($source);
  if (l == NULL) $target = (STRLIST *)&NulStrlist;
  else $target =  l;

}

%typemap(typecheck) STRLIST List {
   $1 = PyList_Check($input) ? 1 : 0;
}

%typemap(freearg) STRLIST List{
//    if ($target) delete $source;
}


%typemap(out) const STRLIST& List {
    $target = PyList_FromSTRLIST($source);
}

%typemap(out) STRLIST List{
    $target = PyList_FromSTRLIST($source);
}


%typemap(ret) STRLIST
{
  delete $source;
}

%typemap(out) STRLIST* {
    $target = PyList_FromSTRLIST(*$source);
}


%typemap(out) STRINGS {
    $target = PyList_FromSTRLIST($source.GetSTRLIST());
}


%typemap(out) STRINGS* {
    $target = PyList_FromSTRLIST($source->GetSTRLIST());
}

%typemap(in) PyObject * {
    $target = $source;
}

%typemap(out) PyObject * {
    return $source;
}

#endif


%{
#ifndef bool
# define bool GDT_BOOLEAN
#endif
%}

#ifdef SWIGPERL
%{
# undef croak
static void croak(const char *x) { message_log(LOG_ERROR, "%s", x); }
static void croak(const char *x, char *y) { message_log(LOG_ERROR, x, y); }

%}
#endif

#ifdef SWIGJAVA
#if 0
%{
bool CurrentLanguageEncodingToCString( const char* pszSrc, char* pszDest, int lDestLen)
{
   bool fDone = 0;
   CFStringRef cfsUTF8 ,cfsSystemEncoded;
   unsigned int len = strlen(pszSrc);
   memset( pszDest, 0, lDestLen);
   cfsUTF8 = CFStringCreateWithBytes ( NULL,pszSrc,len, kCFStringEncodingUTF8, 0);
   CFStringGetCString( cfsUTF8,pszDest, lDestLen,CFStringGetSystemEncoding());
   cfsSystemEncoded = CFStringCreateWithBytes( NULL,pszDest,len, CFStringGetSystemEncoding(), 0 );
   fDone = CFStringGetCString( cfsSystemEncoded, pszDest, lDestLen,kCFStringEncodingUTF8);
   return fDone;
}
%}
#endif
#endif


#ifdef SWIGPYTHON
%{
//--------------- Helper Functions --------------------------------------

static const char err_nomem[] = "Unable to allocate temporary array";
static const char err_not_a_list[] = "Expected a list object.";

//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Some helper functions for typemaps in my_typemaps.i
//

static inline PyObject *FCasTuple(const FC& Fc)
{
  PyObject* rv = PyTuple_New(2);
  PyTuple_SetItem(rv, 0, PyInt_FromLong(Fc.GetFieldStart()));
  PyTuple_SetItem(rv, 1, PyInt_FromLong(Fc.GetFieldEnd()));
  return rv;
}


PyObject* PyList_FromFCT(const FCT Fct)
{
  PyObject     *listPtr = PyList_New ( Fct.GetTotalEntries() );
  const FCLIST *fclist  = Fct.GetPtrFCLIST();
  int i = 0;
  for (const FCLIST *p = fclist->Next(); p != fclist; p = p->Next())
    PyList_SetItem(listPtr, i++,  FCasTuple(p->Value()) );
  return listPtr;
}

PyObject* PyList_FromFCT(const FCT *Fct)
{
  if (Fct)
    {
      PyObject     *listPtr = PyList_New ( Fct->GetTotalEntries() );
      const FCLIST *fclist  = Fct->GetPtrFCLIST();
      int i = 0;
      for (const FCLIST *p = fclist->Next(); p != fclist; p = p->Next())
    PyList_SetItem(listPtr, i++,  FCasTuple(p->Value()) );
      return listPtr;
    }
  PyErr_SetString(PyExc_TypeError, "Null pointer. Expected an FCT pointer.");
  return NULL;
}

PyObject * PyList_FromArraySTRING(const ArraySTRING& array)
{
  const size_t  count = array.Count();
  PyObject *listPtr = PyList_New ( count );
  for (size_t i=0; i < count; i++)
    PyList_SetItem(listPtr, i, PyString_FromString(array[i].c_str()) );
  return listPtr;
}

PyObject * PyList_FromArraySTRING(const ArraySTRING *array)
{
  if (array)
    {
      const size_t  count = array->Count();
      PyObject *listPtr = PyList_New ( count );
      for (size_t i=0; i < count; i++)
    PyList_SetItem(listPtr, i, PyString_FromString((*array)[i].c_str()) );
      return listPtr;
    }
  PyErr_SetString(PyExc_TypeError, "Null pointer. Expected an ArraySTRING pointer.");
  return NULL;
}


PyObject * PyList_FromSTRLIST(const STRLIST& List)
{
  return PyList_FromSTRLIST(&List);
}

PyObject * PyList_FromSTRLIST(const STRLIST& List, int entries)
{
  return PyList_FromSTRLIST(&List, entries);
}

PyObject * PyList_FromSTRLIST(const STRLIST *List)
{
  const size_t  count = List ? List->GetTotalEntries() : 0;
  PyObject *listPtr = PyList_New ( count );
  int i = 0;
  for (const STRLIST *p = List->Next(); p != List; p = p->Next())
    {
       PyList_SetItem(listPtr, i++, PyString_FromString(STRINGCAST(p->Value()).c_str()) );
    }
  return listPtr;
}

PyObject * PyList_FromSTRLIST(const STRLIST *List, int entries)
{
  const size_t  count = ((List && entries == 0) ? List->GetTotalEntries() : (List ? entries : 0));
  PyObject *listPtr = PyList_New ( count );
  int i = 0;
  if (List)
    {
      for (const STRLIST *p = List->Next(); p != List && i < count; p = p->Next())
    PyList_SetItem(listPtr, i++, PyString_FromString(STRINGCAST(p->Value()).c_str()) );
    }
  return listPtr;
}


int* int_LIST_helper(PyObject* source) {
    if (!PyList_Check(source)) {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
        return NULL;
    }
    int count = PyList_Size(source);
    int* temp = new int[count];
    if (! temp) {
        PyErr_SetString(PyExc_MemoryError, err_nomem);
        return NULL;
    }
    for (int x=0; x<count; x++) {
        PyObject* o = PyList_GetItem(source, x);
        if (! PyInt_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of integers.");
            return NULL;
        }
        temp[x] = PyInt_AsLong(o);
    }
    return temp;
}


long* long_LIST_helper(PyObject* source) {
    if (!PyList_Check(source)) {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
        return NULL;
    }
    int count = PyList_Size(source);
    long* temp = new long[count];
    if (! temp) {
        PyErr_SetString(PyExc_MemoryError, err_nomem);
        return NULL;
    }
    for (int x=0; x<count; x++) {
        PyObject* o = PyList_GetItem(source, x);
        if (! PyInt_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of long integers.");
            return NULL;
        }
        temp[x] = PyInt_AsLong(o);
    }
    return temp;
}


char** string_LIST_helper(PyObject* source) {
    if (!PyList_Check(source)) {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
        return NULL;
    }
    int count = PyList_Size(source);
    char** temp = new char*[count+1];
    if (! temp) {
        PyErr_SetString(PyExc_MemoryError, err_nomem);
        return NULL;
    }
    for (int x=0; x<count; x++) {
        PyObject* o = PyList_GetItem(source, x);
        if (! PyString_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of C-strings.");
            return NULL;
        }
        temp[x] = PyString_AsString(o);
    }
    temp[count] = (char *)NULL;
    return temp;
}


STRING* STRING_LIST_helper(PyObject* source) {
    if (!PyList_Check(source)) {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
        return NULL;
    }
    int count = PyList_Size(source);
    STRING* temp = new STRING[count+1];
    if (! temp) {
        PyErr_SetString(PyExc_MemoryError, err_nomem);
        return NULL;
    }
    int j = 0;
    for (int x=0; x<count; x++) {
    char *ptr;
        PyObject* o = PyList_GetItem(source, x);
        if (! PyString_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of C-strings.");
            return NULL;
        }
    if ((ptr = PyString_AsString(o)) != NULL && *ptr)
      temp[j++] = ptr;
    }
    temp[j] = NulString;
    return temp;
}




STRINGS *STRINGS_FromPyList(PyObject* source) {
    if (!PyList_Check(source)) {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
    } else {
    STRINGS *temp = new STRINGS();
    const int count = PyList_Size(source);
    for (int x=0; x<count; x++) {
      PyObject* o = PyList_GetItem(source, x);
      if (! PyString_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of strings.");
        delete temp;
            return NULL;
          }
          temp->AddEntry(  PyString_AsString(o) );
        }
    return temp;
    }
    return NULL;
}


ArraySTRING *ArraySTRING_FromPyList(PyObject* source) {
    if (!PyList_Check(source))
      {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
      }
    else
      {
    const int count = PyList_Size(source);
        ArraySTRING *temp = new ArraySTRING(count);
        for (int x=0; x<count; x++) {
          PyObject* o = PyList_GetItem(source, x);
          if (! PyString_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of strings.");
            delete temp;
            return NULL;
          }
          temp->Add(  PyString_AsString(o) );
        }
        return temp;
      }
    return NULL;
}


STRLIST *STRLIST_FromPyList(PyObject* source) {
    if (!PyList_Check(source))
      {
        PyErr_SetString(PyExc_TypeError, err_not_a_list);
      }
    else
      {
    STRLIST *temp = new STRLIST();
        const int count = PyList_Size(source);
        for (int x=0; x<count; x++) {
          PyObject* o = PyList_GetItem(source, x);
          if (! PyString_Check(o)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of strings.");
            delete temp;
        return NULL;
          }
          temp->AddEntry(  PyString_AsString(o) );
        }
    return temp;
      }
    return NULL;
}

%}


#endif
