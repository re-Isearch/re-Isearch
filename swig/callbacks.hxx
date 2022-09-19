#ifndef __IB_CALLBACKS_HXX
#define __IB_CALLBACKS_HXX

extern PyObject* PyIB_dict;

PyObject* PyIBConstructObject(void* ptr, char* className);

//----------------------------------------------------------------------

class PyIBCallback {
public:
  PyIBCallback(PyObject* func) {
    m_func = func;
    Py_INCREF(m_func);
  }
  ~PyIBCallback() {
#ifdef IB_WITH_THREAD
    yEval_RestoreThread(wxPyEventThreadState);
#endif

    Py_DECREF(m_func);
#ifdef IB_WITH_THREAD
    PyEval_SaveThread();
#endif
  }
  PyObject*   m_func;
};



//---------------------------------------------------------------------------
// This class holds an instance of a Python Shadow Class object and assists
// with looking up and invoking Python callback methods from C++ virtual
// method redirections.  For all classes which have virtuals which should be
// overridable in Python, a new subclass is created that contains a
// PyIBCallbackHelper.
//---------------------------------------------------------------------------

class PyIBCallbackHelper {
public:
    PyIBCallbackHelper();
    ~PyIBCallbackHelper();

    void        setSelf(PyObject* self);

    bool findCallback(const char *name);
    int         callCallback(PyObject* argTuple);
    PyObject*   callCallbackObj(PyObject* argTuple);

private:
    PyObject*   m_self;
    PyObject*   m_lastFound;
};



//---------------------------------------------------------------------------
// These macros are used to implement the virtual methods that should
// redirect to a Python method if one exists.  The names designate the
// return type, if any as well as any parameter types.
//---------------------------------------------------------------------------

#define PYCALLBACK__(PCLASS, CBNAME)                                    \
    void CBNAME() {                                                     \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("()"));                 \
        else                                                            \
            PCLASS::CBNAME();                                           \
    }                                                                   \
    void base_##CBNAME() {                                              \
        PCLASS::CBNAME();                                               \
    }

//---------------------------------------------------------------------------

#define PYPRIVATE                               \
    void _setSelf(PyObject* self) {             \
        m_myInst.setSelf(self);                 \
    }                                           \
    private: PyIBCallbackHelper m_myInst;

//---------------------------------------------------------------------------

#define PYCALLBACK_BOOL_INTINT(PCLASS, CBNAME)                          \
    bool CBNAME(int a, int b) {                                         \
        if (m_myInst.findCallback(#CBNAME))                             \
            return m_myInst.callCallback(Py_BuildValue("(ii)",a,b));    \
        else                                                            \
            return PCLASS::CBNAME(a,b);                                 \
    }                                                                   \
    bool base_##CBNAME(int a, int b) {                                  \
        return PCLASS::CBNAME(a,b);                                     \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK_BOOL_INT(PCLASS, CBNAME)                             \
    bool CBNAME(int a) {                                                \
        if (m_myInst.findCallback(#CBNAME))                             \
            return m_myInst.callCallback(Py_BuildValue("(i)",a));       \
        else                                                            \
            return PCLASS::CBNAME(a);                                   \
    }                                                                   \
    bool base_##CBNAME(int a) {                                         \
        return PCLASS::CBNAME(a);                                       \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK_BOOL_INT_pure(PCLASS, CBNAME)                        \
    bool CBNAME(int a) {                                                \
        if (m_myInst.findCallback(#CBNAME))                             \
            return m_myInst.callCallback(Py_BuildValue("(i)",a));       \
        else return false;                                              \
    }


//---------------------------------------------------------------------------

#define PYCALLBACK__DC(PCLASS, CBNAME)                                  \
    void CBNAME(wxDC& a) {                                              \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(O)",                  \
                            PyIBConstructObject(&a, "wxDC")));           \
        else                                                            \
            PCLASS::CBNAME(a);                                          \
    }                                                                   \
    void base_##CBNAME(wxDC& a) {                                       \
        PCLASS::CBNAME(a);                                              \
    }



//---------------------------------------------------------------------------

#define PYCALLBACK__DCBOOL(PCLASS, CBNAME)                              \
    void CBNAME(wxDC& a, bool b) {                                      \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oi)",                 \
                            PyIBConstructObject(&a, "wxDC"), (int)b));   \
        else                                                            \
            PCLASS::CBNAME(a, b);                                       \
    }                                                                   \
    void base_##CBNAME(wxDC& a, bool b) {                               \
        PCLASS::CBNAME(a, b);                                           \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__DCBOOL(PCLASS, CBNAME)                              \
    void CBNAME(wxDC& a, bool b) {                                      \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oi)",                 \
                            PyIBConstructObject(&a, "wxDC"), (int)b));   \
        else                                                            \
            PCLASS::CBNAME(a, b);                                       \
    }                                                                   \
    void base_##CBNAME(wxDC& a, bool b) {                               \
        PCLASS::CBNAME(a, b);                                           \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__2DBL(PCLASS, CBNAME)                                \
    void CBNAME(double a, double b) {                                   \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(dd)",a,b));           \
        else                                                            \
            PCLASS::CBNAME(a, b);                                       \
    }                                                                   \
    void base_##CBNAME(double a, double b) {                            \
        PCLASS::CBNAME(a, b);                                           \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__2DBL2INT(PCLASS, CBNAME)                            \
    void CBNAME(double a, double b, int c, int d) {                     \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(ddii)",               \
                                                       a,b,c,d));       \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d);                                 \
    }                                                                   \
    void base_##CBNAME(double a, double b, int c, int d) {              \
        PCLASS::CBNAME(a, b, c, d);                                     \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__DC4DBLBOOL(PCLASS, CBNAME)                          \
    void CBNAME(wxDC& a, double b, double c, double d, double e, bool f) {\
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oddddi)",             \
                                   PyIBConstructObject(&a, "wxDC"),      \
                                              b, c, d, e, (int)f));     \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d, e, f);                           \
    }                                                                   \
    void base_##CBNAME(wxDC& a, double b, double c, double d, double e, bool f) {\
        PCLASS::CBNAME(a, b, c, d, e, f);                               \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK_BOOL_DC4DBLBOOL(PCLASS, CBNAME)                      \
    bool CBNAME(wxDC& a, double b, double c, double d, double e, bool f) {\
        if (m_myInst.findCallback(#CBNAME))                             \
            return m_myInst.callCallback(Py_BuildValue("(Oddddi)",      \
                                   PyIBConstructObject(&a, "wxDC"),      \
                                              b, c, d, e, (int)f));     \
        else                                                            \
            return PCLASS::CBNAME(a, b, c, d, e, f);                    \
    }                                                                   \
    bool base_##CBNAME(wxDC& a, double b, double c, double d, double e, bool f) {\
        return PCLASS::CBNAME(a, b, c, d, e, f);                        \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__BOOL2DBL2INT(PCLASS, CBNAME)                        \
    void CBNAME(bool a, double b, double c, int d, int e) {             \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(idii)",               \
                                                (int)a,b,c,d,e));       \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d, e);                              \
    }                                                                   \
    void base_##CBNAME(bool a, double b, double c, int d, int e) {      \
        PCLASS::CBNAME(a, b, c, d, e);                                  \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__DC4DBL(PCLASS, CBNAME)                              \
    void CBNAME(wxDC& a, double b, double c, double d, double e) {     \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Odddd)",              \
                                   PyIBConstructObject(&a, "wxDC"),      \
                                                     b, c, d, e));      \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d, e);                              \
    }                                                                   \
    void base_##CBNAME(wxDC& a, double b, double c, double d, double e) {\
        PCLASS::CBNAME(a, b, c, d, e);                                  \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__DCBOOL(PCLASS, CBNAME)                              \
    void CBNAME(wxDC& a, bool b) {                                      \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oi)",                 \
                                   PyIBConstructObject(&a, "wxDC"),      \
                                                     (int)b));          \
        else                                                            \
            PCLASS::CBNAME(a, b);                                       \
    }                                                                   \
    void base_##CBNAME(wxDC& a, bool b) {                               \
        PCLASS::CBNAME(a, b);                                           \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__WXCPBOOL2DBL2INT(PCLASS, CBNAME)                    \
    void CBNAME(wxControlPoint* a, bool b, double c, double d,          \
                int e, int f) {                                         \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oiddii)",             \
                                 PyIBConstructObject(a, "wxControlPoint"),\
                                 (int)b, c, d, e, f));                  \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d, e, f);                           \
    }                                                                   \
    void base_##CBNAME(wxControlPoint* a, bool b, double c, double d,   \
                       int e, int f) {                                  \
        PCLASS::CBNAME(a, b, c, d, e, f);                               \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__WXCP2DBL2INT(PCLASS, CBNAME)                        \
    void CBNAME(wxControlPoint* a, double b, double c, int d, int e) {  \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(Oddii)",              \
                                 PyIBConstructObject(a, "wxControlPoint"),\
                                 b, c, d, e));                          \
        else                                                            \
            PCLASS::CBNAME(a, b, c, d, e);                              \
    }                                                                   \
    void base_##CBNAME(wxControlPoint* a, double b, double c,           \
                       int d, int e) {                                  \
        PCLASS::CBNAME(a, b, c, d, e);                                  \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__2DBLINT(PCLASS, CBNAME)                             \
    void CBNAME(double a, double b, int c) {                            \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(ddi)", a,b,c));       \
        else                                                            \
            PCLASS::CBNAME(a, b, c);                                    \
    }                                                                   \
    void base_##CBNAME(double a, double b, int c) {                     \
        PCLASS::CBNAME(a, b, c);                                        \
    }

//---------------------------------------------------------------------------

#define PYCALLBACK__BOOL2DBLINT(PCLASS, CBNAME)                         \
    void CBNAME(bool a, double b, double c, int d) {                    \
        if (m_myInst.findCallback(#CBNAME))                             \
            m_myInst.callCallback(Py_BuildValue("(iddi)", (int)a,b,c,d));\
        else                                                            \
            PCLASS::CBNAME(a, b, c, d);                                 \
    }                                                                   \
    void base_##CBNAME(bool a, double b, double c, int d) {             \
        PCLASS::CBNAME(a, b, c, d);                                     \
    }

#endif



