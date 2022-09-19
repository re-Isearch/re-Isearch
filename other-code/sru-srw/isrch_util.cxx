// $Id: isrch_util.cxx 162 2006-05-24 18:25:08Z warnock $
/***********************************************************************
Copyright (c) A/WWW Enterprises, 2001-2005

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/

/*@@@
File:          	isrch_util.cxx
Version:        1.0
$Revision$
Description:    Generic utilities for Isearch-cgi
Author:         Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include "isrch_sru.hxx"
#include "mxml.h"

#include "result.hxx"
#include "vidb.hxx"
#include "infix2rpn.hxx"

INT ParseQuery(SRU* srudata, VIDB* pdb, CHR* query,STRING& TheQuery);
INT GetIsearchTerm(SRU* srudata, VIDB* pdb,
		   mxml_node_t *nodeSearchClause, 
		   mxml_node_t *top, 
		   STRING& term);
INT MapIndexToDbField(SRU* srudata,STRING& ContextField,STRING& NewField);

/**  Converts the query into a reasonable RPN form for Isearch
     - called only by doSearchRetrieve
*/
INT
ParseQuery(SRU* srudata, VIDB* pdb, CHR* query,STRING& TheQuery) {
  STRING  Query;
  CHR    *xcql;
  INT     len, ret;
  mxml_node_t *tree;
  mxml_node_t *nodeSearchClause;

  TheQuery = "";
  xcql = new CHR[XMLMAXLEN];

  // Convert the CQL query into XCQL so we can use the XML parser on it
  ret = cql2xcql(query,xcql);

  if (ret != ISRU_OK) {
    delete [] xcql;
    return ret;
  }

  // The XML parser requires a real XML header on it, so we will
  // prepend one to the converted XCQL query using our STRING
  // objects, then extract that to a char array for the XML parser
  // to use
  Query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  Query.Cat(xcql);

#ifdef DEBUG
  //  cerr << "Query (XML)=" << endl;
  //cerr << Query << endl;
#endif

  delete [] xcql; // done with the converted CQL query

  // The XML parser wants real XML in a char array, so we may as well
  // reuse the char pointer we already defined
  xcql = Query.NewCString();  

  // Load the XCQL query into the tree
  tree = mxmlLoadString(NULL, xcql, MXML_TEXT_CALLBACK);
  if (!tree) {
    cerr << "No parent node!" << endl;
    return ISRU_BADQSYNTAX;
  }
    
  if (tree->type != MXML_ELEMENT) {
    cerr << "ERROR: Parent not MXML_ELEMENT!" << endl;
    mxmlDelete(tree);
    return ISRU_BADQSYNTAX;
  }


  // Find the searchClause node (eventually, this will be in a loop)
  nodeSearchClause = mxmlFindElement(tree, tree, "searchClause", 
				     NULL, NULL, MXML_DESCEND);
  if (nodeSearchClause->type != MXML_ELEMENT) {
    cerr << "ERROR: searchClause node not MXML_ELEMENT!" << endl;
    mxmlDelete(tree);
    return ISRU_BADQSYNTAX;
  }

  ret = GetIsearchTerm(srudata,pdb,nodeSearchClause,tree,Query);
  if (ret == ISRU_OK)
    TheQuery=Query;

  if (tree)
    mxmlDelete(tree);

  delete [] xcql;
  return ret;
}

/** Takes the pointer to a searchClause node, extracts the field and
    term, figures out about phrases and returns the appropriate term
    to store in an SQUERY query.
*/
INT
GetIsearchTerm(SRU* srudata, VIDB* pdb,
	       mxml_node_t *nodeSearchClause, 
	       mxml_node_t *top, 
	       STRING& term) {
  mxml_node_t *nodeRelation;
  mxml_node_t *nodeTerm;
  mxml_node_t *nodeIndex;
  mxml_node_t *nodeModifiers;
  mxml_node_t *node;
  INT          ret=ISRU_OK;
  STRING       ContextField;
  STRING       field;

  // The child node of <index> is the field name, including the
  // context set (ie, dc.title)
  node = mxmlFindElement(nodeSearchClause, top, "index", 
			 NULL, NULL, MXML_DESCEND);
  nodeIndex = mxmlWalkNext(node,top,MXML_DESCEND);
#ifdef DEBUG
  //  cerr << "Found field: " << nodeIndex->value.text.string << endl;
#endif

  // Convert the context+field name into an Isearch field via mapping files
  ContextField = nodeIndex->value.text.string;
  ret = MapIndexToDbField(srudata,ContextField,field);
  if (ret != ISRU_OK)
    return ret;

  // For now, we will explicitly test for some of the non-text fields we
  // know about.  Ultimately, we will call a method in VIDB to determine
  // the datatype of the field and branch here accordingly.
  if (field.CaseEquals("METADATA_IDINFO_SPDOM_BOUNDING")) {
    // It is a spatial search
  } else if ((field.CaseEquals("METADATA_IDINFO_CITATION_CITEINFO_PUBDATE"))
	     || (field.CaseEquals("METADATA_METAINFO_METD"))) {
    // It is a date search
  } else if ((field.CaseEquals("METADATA_IDINFO_SPDOM_BOUNDING_WESTBC")) 
	     || (field.CaseEquals("METADATA_IDINFO_SPDOM_BOUNDING_EASTBC")) 
	     || (field.CaseEquals("METADATA_IDINFO_SPDOM_BOUNDING_NORTHBC")) 
	     || (field.CaseEquals("METADATA_IDINFO_SPDOM_BOUNDING_SOUTHBC"))) {
    // It is a numeric search
  } else {  // It is a text search

    // At this point, we have to pass the field name to the VIDB object
    // and ask what the field type is, since the form of the XCQL query
    // that we have to parse will depend on things like modifiers and 
    // relations

    // The child node of <relation> is <value>, so we have to step down
    // one level to find the value element within relation, then extract
    // the actual relation
    node = mxmlFindElement(nodeSearchClause, top, "relation", 
			   NULL, NULL, MXML_DESCEND);
    node = mxmlFindElement(node, top, "value", NULL, NULL, MXML_DESCEND);
    nodeRelation = mxmlWalkNext(node,top,MXML_DESCEND);
#ifdef DEBUG
    //    cerr << "Found relation: " << nodeRelation->value.text.string << endl;
#endif

    // The child node of <term> is the search term
    node = mxmlFindElement(nodeSearchClause, top, "term", 
			   NULL, NULL, MXML_DESCEND);
    nodeTerm = mxmlWalkNext(node,top,MXML_DESCEND);
#ifdef DEBUG
    //    cerr << "Found term: " << nodeTerm->value.text.string << endl;
#endif

    // Build the Isearch search term
    term = field;
    term.Cat("/");
    term.Cat(nodeTerm->value.text.string);
  }
  return ret;
  
}

/** Maps from the SRU <context>.<index> string to the appropriate
    Isearch field name
*/
INT
MapIndexToDbField(SRU* srudata,STRING& ContextField,STRING& NewField) {
  STRINGINDEX n;
  STRING      Context,Field;
  STRING      DefaultContext;
  INT         ret=ISRU_OK;

  DefaultContext = "dc";
  n = ContextField.Search('.');
  Context = ContextField;
  Context.EraseAfter(n-1);

  // First, see if this context set is supported
  ret = srudata->ValidateContextSet(Context);
  if (ret != ISRU_OK) {
    srudata->SetDiagnostic(ret,Context);
    return ret;
  }
   
  // If we support this context set, we need to find out what the
  // corresponding map file is for this index
  ret = srudata->LoadDbMapFile(Context);
  if (ret != ISRU_OK) {
    srudata->SetDiagnostic(ret,Context);
    return ret;
  }

  // Convert the requested context set field into a supported Isearch field
  ret = srudata->GetMappedField(ContextField,&Field);
  if (ret != ISRU_OK) {
    srudata->SetDiagnostic(ret,ContextField);
    return ret;
  }

  NewField=Field;
  return ret;
}


/** Pulls about the database not in the INI file from the IDB object
    and stores it into the SRU object for use in generating the
    explain record
*/
INT
doExplain(SRU* srudata) {
  INT      status;
  VIDB    *pdb;
  STRLIST  DocTypeOptions; // dummy for searching

  status=ISRU_OK;
  pdb = new VIDB(srudata->GetDatabasePath(), 
		 srudata->GetIndexName(), 
		 DocTypeOptions);
  if (!pdb) {
    return ISRU_BADPPVAL;
  }

  srudata->SetDbNumDocs(pdb->GetTotalRecords());
  return status;
}


///  Creates the complete search response from the IRSET
INT
doPresent(SRU* srudata, VIDB* pdb, IRSET* pirset) {
  STRING  response,
    Count,
    RecordSyntax,
    RecordSchema,
    SchemaResponse;
  CHR    *temp;
  INT     startDoc, 
    nHits,
    nDocs,
    status;

  status = ISRU_OK;
  nHits = pirset->GetTotalEntries(); // cast to STRING
  Count = nHits; // cast to STRING
  response="<zs:numberOfRecords>";
  response.Cat(Count);
  response.Cat("</zs:numberOfRecords>\n");
  srudata->SetResponse(response);

  if ((temp = srudata->GetValueByName("startRecord")) == NULL) {
    startDoc=1;
  } else {
    startDoc=atoi(temp);
  }

  if ((temp = srudata->GetValueByName("maximumRecords")) == NULL) {
    nDocs=0;
  } else {
    nDocs=atoi(temp);
  }

  if ((temp = srudata->GetValueByName("recordSchema")) == NULL) {
    RecordSchema = "geo";
  } else {
    RecordSchema=temp;
  }

  if (RecordSchema.CaseEquals("geo")) {
    RecordSyntax=XmlRecordSyntax; // the attribute set in Isearch terms
    SchemaResponse="http://www.blueangeltech.com/Standards/GeoProfile/";

  } else if (RecordSchema.CaseEquals("xml")) {
    RecordSyntax=XmlRecordSyntax; // the attribute set in Isearch terms

  } else {
    status = ISRU_BADSCHEMA;
    srudata->SetDiagnostic(status,RecordSchema);
    return status;
  }

  if (nDocs > 0) {
    RSET *prset;
    RESULT RsRecord;
    STRING ESName;
    STRING Record;
    STRINGINDEX head;

    ESName="F";
    pirset->SortByScore();
    pdb->BeginRsetPresent(RecordSyntax);
    prset=pirset->GetRset(0,nDocs);
    pirset->Fill(0,nDocs,prset);
    prset->SetScoreRange(pirset->GetMaxScore(),
			 pirset->GetMinScore());
    srudata->AddToResponse("<zs:records>\n");
    while ((nDocs > 0) && (nHits > 0)) {
      // Fetch the hits
      prset->GetEntry(startDoc, &RsRecord);
      pdb->Present(RsRecord, ESName, RecordSyntax, &Record);

      // Now that we have the record, we had best strip off the XML header
      head = Record.Search("<metadata>");
      if (head > 0) {
          Record.EraseBefore(head);
      }

      nDocs--;
      startDoc++;
      nHits--;
      srudata->AddToResponse("<zs:record>\n");
      //      srudata->AddToResponse("<zs:recordSchema>info:srw/schema/1/marcxml-v1.1</zs:recordSchema>\n");
      //      srudata->AddToResponse("<zs:recordSchema>http://www.blueangeltech.com/Standards/GeoProfile/</zs:recordSchema>\n");
      srudata->AddToResponse("<zs:recordSchema>");
      srudata->AddToResponse(SchemaResponse);
      srudata->AddToResponse("</zs:recordSchema>\n");
      srudata->AddToResponse("<zs:recordPacking>");
      srudata->AddToResponse(RecordSyntax);
      srudata->AddToResponse("</zs:recordPacking>\n");
      srudata->AddToResponse("<zs:recordData>\n");
      srudata->AddToResponse("<record>\n");
      srudata->AddToResponse(Record);
      srudata->AddToResponse("</record>\n");
      srudata->AddToResponse("</zs:recordData>\n");
      srudata->AddToResponse("</zs:record>\n");
    }
    srudata->AddToResponse("</zs:records>\n");
  }

  return status;
}


/// Performs the search and generates the complete search result
INT
doSearchRetrieve(SRU* srudata, CHR* query) {
  INT      status;
  IRSET   *pirset=(IRSET*)NULL;
  STRLIST  DocTypeOptions; // dummy for searching
  SQUERY   squery;
  STRING   QueryString,ErrString;
  VIDB    *pdb;
  
  status=ISRU_OK;
  pdb = new VIDB(srudata->GetDatabasePath(), 
		 srudata->GetIndexName(), 
		 DocTypeOptions);
  if (!pdb) {
    status = ISRU_BADPPVAL;
    ErrString = srudata->GetIndexName();
    srudata->SetDiagnostic(status,ErrString);
    return status;
  }

  status = ParseQuery(srudata,pdb,query,QueryString);
  if (status == ISRU_OK) {
    squery.SetRpnTerm(QueryString);
    pirset = pdb->Search(squery);
    status = doPresent(srudata,pdb,pirset);
  } else {
    srudata->SetDiagnostic(status,query);
  }
  delete pdb;
  return status;
}


/** This is the discovery mechanism for Isearch SRU.  If no operation
    or database is specified in the command string, this reads the
    ini file and generates an XML document with a list of the databases
    found there, along with links to their explain records.

    The XML document includes a stylesheet spec to convert it to HTML.
 */
void PrintDbListDoc(SRU* srudata) {
  STRLIST dblist;
  INT     n_databases;
  STRING  temp,dbname,sru,linkage;

  cout << "Content-type: text/xml\n\n";

  cout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>" << endl;
  cout << "<?xml-stylesheet type=\"text/xsl\" href=\""
       << srudata->GetIsruXslUri() << "\"?>" << endl;
  cout << "<!-- A/WWW Enterprises, http://www.awcubed.com/Isite/ -->" << endl;
  cout << "<!DOCTYPE isru SYSTEM \""
       << srudata->GetIsruDtdUri() << "\">" << endl;
  cout << "<isru>" << endl;
  srudata->PrintHost();
  srudata->PrintPort();
  srudata->PrintTitle();
  srudata->PrintDescription();
  srudata->PrintStatus();

  cout << "  <dblist>" << endl;
  dblist = srudata->GetDbList();
  n_databases = dblist.GetTotalEntries();
  for (INT i=1;i<=n_databases;i++) {
    dblist.GetEntry(i,&temp);
    dbname="\"";
    dbname.Cat(temp);
    dbname.Cat("\"");

    sru="\"/";
    sru.Cat(srudata->GetServerSru());
    sru.Cat("/");
    sru.Cat(temp);
    sru.Cat("\"");

    linkage="\"http://";
    linkage.Cat(srudata->GetServerName());
    linkage.Cat(":");
    linkage.Cat(srudata->GetServerPort());
    linkage.Cat("/");
    linkage.Cat(srudata->GetServerSru());
    linkage.Cat("/");
    linkage.Cat(temp);
    linkage.Cat("?operation=explain&amp;version=");
    linkage.Cat(srudata->GetServerSruVersion());
    linkage.Cat("\" />\n");


    cout << "    <db name=";
    cout << dbname;

    cout << " sru=";
    cout << sru;

    cout << " linkage=";
    cout << linkage;

  }
  
  cout << "  </dblist>" << endl;
  cout << "</isru>" << endl;
}

