%{
#if PY_MAJOR_VERSION >= 3

FILE* PyFile_AsFile(PyObject *pyfile) {
    FILE* fp;
    int fd;
    const char *mode_str = NULL;
    PyObject *mode_obj;

    if ((fd = PyObject_AsFileDescriptor(pyfile)) == -1) {
        PyErr_SetString(PyExc_BlockingIOError,
                        "Cannot find file handler for the Python file!");
        return NULL;
    }

    if ((mode_obj = PyObject_GetAttrString(pyfile, "mode")) == NULL) {
        mode_str = "rb";
        PyErr_Clear();
    }
    else {
        /* convert to plain string
         * note that error checking is embedded in the function
         */
        mode_str = PyUnicode_AsUTF8AndSize(mode_obj, NULL);
    }

    if((fp = fdopen(fd, mode_str)) == NULL) {
         PyErr_SetFromErrno(PyExc_IOError);
    }

    Py_XDECREF(mode_obj);
    return fp;
}


int PyFile_Check(PyObject *pyfile) {
   return PyObject_AsFileDescriptor(pyfile) != -1;
}

PyObject* PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE*))
{
  if (fp) {
    int fd = fileno(fp);
    if (fd > 0) 
      PyFile_FromFd(fd, name, mode, -1, NULL, NULL,NULL,1);
  }
  PyErr_SetFromErrno(PyExc_IOError);
  return NULL;
}

#define PyClass_Check(obj) PyObject_IsInstance(obj, (PyObject *)&PyType_Type)
#define PyInt_Check(x) PyLong_Check(x)
#define PyInt_AsLong(x) PyLong_AsLong(x)
#define PyInt_FromLong(x) PyLong_FromLong(x)
#define PyInt_FromSize_t(x) PyLong_FromSize_t(x)
#define PyString_Check(name) PyBytes_Check(name)
#define PyString_FromString(x) PyUnicode_FromString(x)
#define PyString_Format(fmt, args)  PyUnicode_Format(fmt, args)
#define PyString_AsString(str) PyBytes_AsString(str)
#define PyString_Size(str) PyBytes_Size(str)
#define PyString_InternFromString(key) PyUnicode_InternFromString(key)
#define Py_TPFLAGS_HAVE_CLASS Py_TPFLAGS_BASETYPE
#define PyString_AS_STRING(x) PyUnicode_AS_STRING(x)
#define _PyLong_FromSsize_t(x) PyLong_FromSsize_t(x)

#else /* PY2K */

#define PyLong_FromLong(x) PyInt_FromLong(x)
#define PyUnicode_AsUTF8(x) PyString_AsString(x)
#define PyUnicode_FromString(x) PyString_FromString(x)
#define PyUnicode_Format(x, y) PyString_Format(x, y)

#endif /* PY_MAJOR_VERSION */
%}
