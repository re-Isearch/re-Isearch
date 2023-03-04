/*@@@
File:		zsearch.cxx
Version:	2.00
$Revision$
Description:	Command-line search utility
@@@*/

#define RETURN_ZERO -1

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <sys/stat.h>
#ifdef UNIX
#include <unistd.h>
#endif

//#include "isearch.hxx"

#include "defs.hxx"
#include "common.hxx"
#include "infix2rpn.hxx"
#include "vidb.hxx"

/*
#include "dtreg.hxx"
#include "rcache.hxx"
#include "index.hxx"
#include "fprec.hxx"       
#include "fpt.hxx"
#include "registry.hxx"
#include "idb.hxx"
#include "thesaurus.hxx"
*/

#define MAX_QUERY_LINE 10240

//#ifdef REMOTE_INDEXING
//RLDCACHE* Cache;
//#endif

int 
main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr,"zsearch v%s\n",  __IB_Version);
    fprintf(stderr,"-d (X)        # Search database with root name (X).\n");
    fprintf(stderr,"-V            # Print the version number.\n");
    // fprintf(stderr,"-syn          # Do synonym expansion.\n");
    fprintf(stderr,"-f (X)        # Present results in format (X).\n");
    fprintf(stderr,"-q (X)        # Read the query from file (X)\n");
    fprintf(stderr,"-o (X)        # Document type specific option.\n");
    // fprintf(stderr,"-prefix (X)   # Add prefix (X) to matched terms in document.\n");
    // fprintf(stderr,"-suffix (X)   # Add suffix (X) to matched terms in document.\n");
    // fprintf(stderr,"-byterange    # Print the byte range of each document within\n");
    // fprintf(stderr,"              # the file that contains it.\n");
    // fprintf(stderr,"              # in the list.\n");
    fprintf(stderr,"-RECT{North South West East}  # Find targets that overlap\n");
    fprintf(stderr,"                              # this geographic rectangle.\n");
    fprintf(stderr,"(X) (Y) (...) # Search for words (X), (Y), etc.\n");
    fprintf(stderr,"              # [fieldname/]searchterm[*][:n]\n");
    fprintf(stderr,"              # Prefix with fieldname/ for fielded searching.\n");
    fprintf(stderr,"              # Append * for right truncation.\n");
    //    cout << "                        // Append ~ for soundex search." << endl;
    fprintf(stderr,"              # Append :n for term weighting (default=1).\n");
    fprintf(stderr,"              # (Use negative values to lower rank.)\n");
    fprintf(stderr,"Examples: zsearch -d POETRY truth \"beaut*\" urn:2\n");
    fprintf(stderr,"          zsearch -d WEBPAGES title/library\n");

    fprintf(stderr,"\n");
    fprintf(stderr,"zsearch is currently experimental and should be used ");
    fprintf(stderr,"cautiously.  Suggestions\nand improvements are welcomed.\n");
    fprintf(stderr,"\n");
    fflush(stdout); fflush(stderr); exit (0);
  }

  printf("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
  printf("<!DOCTYPE zsearch SYSTEM \"zsearch.dtd\">\n");
  printf("<zsearch xmlns:isearch=\"http://www.exodao.net/dtd\">\n");

  STRLIST DocTypeOptions;
  STRING Flag;
  STRING DBName;
  STRING ElementSet;
  STRING RecordSyntax;
  STRING TermPrefix, TermSuffix;
  STRING StartDoc="", EndDoc="";
  STRING QueryFile;
  STRING error_message="";
  INT DebugFlag = 0;
  INT QuitFlag = 0;
  INT ByteRangeFlag = 0;
  INT BooleanAnd = 0;
  INT RpnQuery = 0;
  INT InfixQuery = 0;
  //  INT SpatialRectFlag=0;
  INT x = 0;
  INT LastUsed = 0;
  GDT_BOOLEAN TerseFlag=GDT_FALSE;
  GDT_BOOLEAN Synonyms=GDT_FALSE;
  GDT_BOOLEAN Error=GDT_FALSE;

  ElementSet = "B";
  while (x < argc) {
    if (argv[x][0] == '-') {
      Flag = argv[x];
      if (Flag.Equals("-o")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No option specified after -o.</isearch:error_text>\n");
	}
	STRING S;
	S = argv[x];
	DocTypeOptions.AddEntry(S);
	LastUsed = x;
      }
      if (Flag.Equals("-d")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No database name specified after -d.</isearch:error_text>\n");
	}
	DBName = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-p")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No element set specified after -p.</isearch:error_text\n");
	}
	ElementSet = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-q")) {
	if (++x >= argc) {
	  fprintf(stderr,"ERROR: No file name specified after -f.\n\n");
	  RETURN_ZERO;
	}
	QueryFile = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-f")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No format specified after -f.</isearch:error_text>\n");
	}
	RecordSyntax = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-prefix")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No prefix specified after -prefix.</isearch:error_text>\n");
	}
	TermPrefix = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-suffix")) {
	if (++x >= argc) {
	  Error=GDT_TRUE;
	  error_message.Cat("\t\t\t<isearch:error_text>No suffix specified after -suffix.</isearch:error_text>\n");
	}
	TermSuffix = argv[x];
	LastUsed = x;
      }
      if (Flag.Equals("-syn")) {
	Synonyms = GDT_TRUE;
	LastUsed = x;
      }
      if (Flag.Equals("-t")) {
	TerseFlag = GDT_TRUE;
	QuitFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-byterange")) {
	ByteRangeFlag = 1;
	LastUsed = x;
      }
      if (Flag.Equals("-V")) {
	fflush(stdout); fflush(stderr); exit (0);
      }
      if (Flag.Equals("-debug")) {
	DebugFlag = 1;
	LastUsed = x;
      }
    }
    x++;
  }
	
  if (DBName.Equals("")) {
    DBName =  __IB_DefaultDbName;
  }
	
  /*
  if (!setlocale(LC_CTYPE,"")) {
    fprintf(stderr,"Warning: Failed to set the locale!\n");
  }
  */

  // Check that we did not read too much from the command line
  if (x > argc) {
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>Too many arguments</isearch:error_text>\n");
  }

  INT NumWords;
  INT z;
  STRING QueryString;
 
  if (QueryFile.GetLength() > 0) {
    // We have something in QueryFile, so we ignore the remainder of
    // the command line and just read the query from the file
    CHR s[MAX_QUERY_LINE + 1];
    QueryString = "";
    // The file might be stdin
    if (QueryFile.Equals("-")) {
      while (fgets(s, MAX_QUERY_LINE, stdin) != NULL) {
	if (s[strlen(s)-1] == '\n') {
	  s[strlen(s)-1] = '\0';
	  QueryString.Cat(s);
	}
      }

    } else {
      // Or it might be an ordinary file, in which case we just suck it in
      if (!(QueryString.ReadFile(QueryFile))) {
	Error=GDT_TRUE;
	error_message.Cat("\t\t\t<isearch:error_text>Cannot find query file (-f)</isearch:error_text>\n");
      }
    }
    // When we get here, QueryString contains the query, although it
    // might not be quite usable.
    STRLIST QueryList;
    QueryString.Replace("\n"," ");

  } else {
    x = LastUsed + 1;
    NumWords = argc - x;
    for (z=0; z<NumWords; z++) {
      if (z != 0) {
	QueryString.Cat(' ');
      }
      QueryString.Cat(argv[z+x]);
    }
  }
  
  STRING DBPathName, DBFileName;
  STRING DBCheckName;
  STRING XmlBuffer;
  STRING PathName, FileName;
  SQUERY squery;
  RSET  *prset=(RSET*)NULL;
  IRSET *pirset=(IRSET*)NULL;
  RESULT result;
  INT t, n;
	
  DBPathName = DBName;
  DBFileName = DBName;
 
  // See if we have a legitimate file
  if (!DBExists(DBName)) {
    // The file does not exist
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>The specified database was not found: ");
    XmlBuffer = DBName;
    // XmlBuffer.XmlCleanup();
    error_message.Cat(XmlBuffer);
    error_message.Cat("</isearch:error_text>\n");
  }

  if (Error) {
    XmlBuffer = DBName;
    // XmlBuffer.XmlCleanup();
    cout << "\t<isearch:search status=\"Error\" dbname=\"" << XmlBuffer
	 << "\">" << endl;

    XmlBuffer = error_message;
    cout << "\t\t<isearch:error_block>" << endl;
    cout << XmlBuffer;
    cout << "\t\t</isearch:error_block>" << endl;
    cout << "\t</search>" << endl;
    cout << "</zsearch>" << endl;

    fflush(stdout);
    fflush(stderr);
    exit (0);
  }

  RemovePath(&DBFileName);
  RemoveFileName(&DBPathName);

  VIDB *pdb;
  pdb = new VIDB(DBPathName, DBFileName, DocTypeOptions);

  if (DebugFlag) {
    pdb->DebugModeOn();
  }
  
  if (!pdb->IsDbCompatible()) {
    Error=GDT_TRUE;
    error_message.Cat("\t\t\t<isearch:error_text>The specified database is not compatible with this version of zsearch.</isearch:error_text>\n");
    delete pdb;
  }
 
  squery.SetQueryTerm(QueryString);
  QUERY query = QUERY(squery);
  query.SetNormalizationMethod(EuclideanNormalization);
  query.SetSortBy(ByScore);
  pirset = pdb->SearchSmart(&query);

  squery = query.GetSQUERY();
  QueryString = squery.GetRpnTerm();
  // QueryString.XmlCleanup();
  cout << "\t<isearch:query>" << QueryString << "</isearch:query>" << endl;

  if (pirset) {
    printf("\t<isearch:search status=\"OK\" dbname=\"");
    XmlBuffer = DBName;
    // XmlBuffer.XmlCleanup();
    XmlBuffer.Print();
    printf("\">\n");

    n = pirset->GetTotalEntries();
    printf("\t\t<isearch:results count=\"%i\">\n",n);

    if (RecordSyntax.IsEmpty()) RecordSyntax = SutrsRecordSyntax;
    pdb->BeginRsetPresent(RecordSyntax);
	
    // display all of them
    prset=pirset->GetRset(0,n);
    // pirset->Fill(0,n,prset);
    prset->SetScoreRange(pirset->GetMaxScore(), pirset->GetMinScore());
  
    n  = prset->GetTotalEntries(); // Should be same as previous n value

  
    //    CHR Selection[80];
    //    CHR s[256];
    //    INT FileNum;
    STRING BriefString;
    STRING Element, TempElementSet;
    //    GDT_BOOLEAN FirstRun = GDT_TRUE;
    STRLIST BriefList;
    STRING TotalBrief;
    STRING ResultKey;
    STRING Delim;

    INT MajorCount=0;

    for (t=1; t<=n; t++) {
      prset->GetEntry(t, &result);
      ++MajorCount;
      
      // result.GetPathName(&PathName);
      // result.GetFileName(&FileName);
      
      result.GetVKey(&ResultKey);
      printf("\t\t\t<isearch:result rank=\"%i\"",t);
      cout << " docid=\"" << ResultKey << "\" ";
      printf(" score=\"%i\"/>\n", prset->GetScaledScore(result.GetScore(), 100));

    }
    printf("\t\t</isearch:results>\n");
    pdb->EndRsetPresent(RecordSyntax);
    delete pirset;
    
  }

  printf("\t</isearch:search>\n");
  printf("</zsearch>\n");

  if (prset)
    delete prset;
  if (pdb)
    delete pdb;

  fflush(stdout); 
  fflush(stderr); 
  exit (0);
}
