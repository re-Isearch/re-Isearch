#include "defs.hxx"
#include "sru.hxx"

#define XMLMAXLEN 4*1024*1024

/// This is in isrch_utils.cxx
INT  doExplain(SRU* srudata);

/// This is in isrch_utils.cxx
INT  doSearchRetrieve(SRU* srudata, CHR* query);

/// This is in isrch_utils.cxx
void PrintDbListDoc(SRU* srudata);

/// This is in isrch_cql.cxx
INT  cql2xcql(CHR *cqlquery,CHR *xcqlquery);
