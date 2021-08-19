#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include "platform.h"

/////////////////////// Search Resource Configuration /////////////////////////////
#ifndef DEFAULT_MAX_TERM_SEARCH_TIME  
# define DEFAULT_MAX_TERM_SEARCH_TIME 6
#endif
#ifndef DEFAULT_MAX_CPU_TICKS
# define DEFAULT_MAX_CPU_TICKS (DEFAULT_MAX_TERM_SEARCH_TIME*CLOCKS_PER_SEC)
#endif
#ifndef DEFAULT_QUERY_TERM_TIME_FACTOR
# define DEFAULT_QUERY_TERM_TIME_FACTOR 3
#endif
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Indexing Configuration //////////////////////////////////////
#ifndef VERY_COMMON_TERM
# define VERY_COMMON_TERM                   100000
#endif
#ifndef DEFAULT_TOO_MANY_RECORDS_THRESHOLD
# define DEFAULT_TOO_MANY_RECORDS_THRESHOLD  10000
#endif
/////////////////////////////////////////////////////////////////////////////////////


#if defined(_WIN32) //|| defined(LINUX)
# ifndef fseeko
    static int fseeko(FILE *stream, off_t offset, int whence)
    { return fseek(stream, (long)offset, whence); }
# endif
# ifndef ftello
    static off_t ftello(FILE *stream)
    { return (off_t)ftell(stream); }
# endif
#endif

// Search
int _ib_defaultMaxCPU_ticks             = DEFAULT_MAX_CPU_TICKS;
int _ib_defaultMaxQueryCPU_ticks        = DEFAULT_QUERY_TERM_TIME_FACTOR * DEFAULT_MAX_CPU_TICKS;
// Index
int _ib_defaultCommonWordsThreshold     = VERY_COMMON_TERM;
int _ib_defaultTooManyRecordsThreshold  = DEFAULT_TOO_MANY_RECORDS_THRESHOLD;


// Computed Numerical Callback
// Args:
//    Doctype:   The name of the doctype
//    fieldname: The name of the field
//    buffer:    The contents of the field
//    len:       The length of the contexts
//
// When used?    During indexing. Search is numerical using the value provided
//               in the query.
// NOTE: The default is to view the contents as a number and use the number
long double (*_IB_parse_computed)(const char *doctype, const char *fieldname, const char *buffer, size_t len) = 0;


// Hash Datatype
// Args:
//  fieldname:  the name of field 
//  buffer:     the contents of the field
//  len:        the length of the buffer contents 
//
// When used?   During BOTH indexing and search.
//              During indexing: to create a numerical value for the index
//              During search: to derive a numerical value for the search term to search
//              in the numerical index
//
long double (*_IB_private_hash)(const char *fieldname, const char *buffer, size_t len) = 0;
// Description string for the function of the hash
const char   *_IB_private_hash_descr = NULL;

// Install as function, description
typedef long double (*__private_hash_t)(const char *, const char *, size_t );
struct _IB_private_hash_def {
   __private_hash_t  function;
  const char       *description;
} _IB_private_hash_array[10] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} };


/* FILE stream I/O */
FILE* (*_IB_Extern_fopen) (const char *, const char*) = fopen;
FILE* (*_IB_Extern_freopen) (const char *, const char*, FILE *) = freopen;
int   (*_IB_Extern_fclose) (FILE *) = fclose;

int   (*_IB_Extern_fseek)(FILE *, long, int) = fseek;
long  (*_IB_Extern_ftell)(FILE *) = ftell;

int   (*_IB_Extern_fseeko)(FILE *, off_t, int) = fseeko;
off_t (*_IB_Extern_ftello)(FILE *) = ftello;

void  (*_IB_Extern_rewind)(FILE *) = rewind;
int   (*_IB_Extern_fgetpos)(FILE *, fpos_t *) = fgetpos;
int   (*_IB_Extern_fsetpos)(FILE *, const fpos_t *) = fsetpos;

char* (*_IB_Extern_fgets)(char *, int, FILE *) = fgets;

size_t (*_IB_Extern_fread)(void *, size_t, size_t, FILE *) = fread;
size_t (*_IB_Extern_fwrite)(const void *, size_t, size_t, FILE *) = fwrite;



/* Low level I/O */
/* Need to do these as function due to vargs */
int _IB_Extern_open(const char *path, int flags, mode_t mode)
{
  return open(path, flags, mode);
}

int _IB_Extern_close(int fd)
{
  return close(fd);
}


// Private IB Error Messages to extend the standard Z codes
// Extend as needed.
static const char *IB_ErrorMessage(int ErrorCode)
{
  switch (ErrorCode)
    {
      case -64: return "32-bit indexes are not compatible with 64-bit libs.";
      case -32: return "64-bit indexes are not compatible with 32-bit libs. ";
      default:  return "Unknown Error";
    }
}


const char *(*__Private_IB_ErrorMessage)(int) = IB_ErrorMessage;


/*
  _ib_ResolveBinPath

  args:
        Filename  := name of the binary (exe), e.g. pdftomemo
        buffer    := target to copy the resolved full path to the binary
        length    := size of the buffer
  returns
        the length of the resolved path
        return  0 if the path could not be resolved in this callback function
        return -1 if the buffer was not large enough

*/
int (*_ib_ResolveBinPath)(const char *Filename, char *buffer, int length);

/* Same as above but looking for configuration files, e.g. ini files */

int (*_ib_ResolveConfigPath)(const char *Filename, char *buffer, int length);


// Where it might be installed (default places)
const char * const __IB_StandardBaseDirs[]= {
  "/opt/nonmonotonic/ib",
  "/opt/re-isearch/",
  "/var/opt/nonmonotonic/ib",
  "/var/opt/re-isearch",
  "/usr/local/nonmonotonic",
  NULL
};


