/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#define __RECORDSYNTAX
#include <stdlib.h>
#include "common.hxx"
#include "string.hxx"
#include "dictionary.hxx"
#include "recordsyntax.hxx"

#pragma ident  "%Z%%Y%%M%  %I% %G% %U% BSN"


using namespace Syntax;
static const struct {
  RecordSyntax_t     syntax;
  const char        *oid;
} RecordSyntaxes[] = {
  {UnimarcRecordSyntax,                        "1.2.840.10003.5.1"},
  {IntermarcRecordSyntax,                      "1.2.840.10003.5.2"},
  {CCFRecordSyntax,                            "1.2.840.10003.5.3"},
  {USmarcRecordSyntax,                         "1.2.840.10003.5.10"},
  {UKmarcRecordSyntax,                         "1.2.840.10003.5.11"},
  {NormarcRecordSyntax,                        "1.2.840.10003.5.12"},
  {LibrismarcRecordSyntax,                     "1.2.840.10003.5.13"},
  {DanmarcRecordSyntax,                        "1.2.840.10003.5.14"},
  {FinmarcRecordSyntax,                        "1.2.840.10003.5.15"},
  {MABRecordSyntax,                            "1.2.840.10003.5.16"},
  {CanmarcRecordSyntax,                        "1.2.840.10003.5.17"},
  {SBNRecordSyntax,                            "1.2.840.10003.5.18"},
  {PicamarcRecordSyntax,                       "1.2.840.10003.5.19"},
  {AusmarcRecordSyntax,                        "1.2.840.10003.5.20"},
  {IbermarcRecordSyntax,                       "1.2.840.10003.5.21"},
  {CatmarcRecordSyntax,                        "1.2.840.10003.5.22"},
  {MalmarcRecordSyntax,                        "1.2.840.10003.5.23"},
  {ExplainRecordSyntax,                        "1.2.840.10003.5.100"},
  {SUTRSRecordSyntax,                          "1.2.840.10003.5.101"},
  {OPACRecordSyntax,                           "1.2.840.10003.5.102"},
  {SummaryRecordSyntax,                        "1.2.840.10003.5.103"},
  {GRS0RecordSyntax,                           "1.2.840.10003.5.104"},
  {GRS1RecordSyntax,                           "1.2.840.10003.5.105"},
  {ESTaskPackageRecordSyntax,                  "1.2.840.10003.5.106"},
  {fragmentRecordSyntax,                       "1.2.840.10003.5.107"},
  {PDFRecordSyntax,                            "1.2.840.10003.5.109.1"},
  {postscriptRecordSyntax,                     "1.2.840.10003.5.109.2"},
  {HtmlRecordSyntax,                           "1.2.840.10003.5.109.3"},
  {TiffRecordSyntax,                           "1.2.840.10003.5.109.4"},
  {jpegTDRecordSyntax,                         "1.2.840.10003.5.109.6"},
  {pngRecordSyntax,                            "1.2.840.10003.5.109.7"},
  {mpegRecordSyntax,                           "1.2.840.10003.5.109.8"},
  {sgmlRecordSyntax,                           "1.2.840.10003.5.109.9"},
  {XmlRecordSyntax,                            "1.2.840.10003.5.109.10"},
  {applicationXMLRecordSyntax,                 "1.2.840.10003.5.109.11"},
  {tiffBRecordSyntax,                          "1.2.840.10003.5.110.1"},
  {wavRecordSyntax,                            "1.2.840.10003.5.110.2"},
  {SQLRSRecordSyntax,                          "1.2.840.10003.5.111"},
  {Latin1RecordSyntax,                         "1.2.840.10003.5.1000.3.1"},
  {PostscriptRecordSyntax,                     "1.2.840.10003.5.1000.3.3"},
  {CXFRecordSyntax,                            "1.2.840.10003.5.1000.6.2"},
  {SgmlRecordSyntax,                           "1.2.840.10003.5.1000.34.2"},
  {ADFRecordSyntax,                            "1.2.840.10003.5.1000.147.1"},

  // Private Exensions
  {DVBHtmlRecordSyntax,                        "1.2.840.10003.5.1000.34.3"}, // Private for DVB
  {TextHighlightRecordSyntax,                  "1.2.840.10003.5.1000.34.4.1"}, // Private for IB
  {HTMLHighlightRecordSyntax,                  "1.2.840.10003.5.1000.34.4.2"}, // Private for IB
  {XMLHighlightRecordSyntax,                   "1.2.840.10003.5.1000.34.4.3"}, // Private for IB

  // Dups
  {HTMLHighlightRecordSyntax,                  "1.2.840.10003.5.1000.34.4"}, // Private for IB
  {applicationXMLRecordSyntax,                 "1.2.840.10003.5.1000.81.2"},
  {HtmlRecordSyntax,                           "1.2.840.10003.5.108"}, // OLD
  {PDFRecordSyntax,                            "1.2.840.10003.5.109.5"},
  {TiffRecordSyntax,                           "1.2.840.10003.5.1000.3.2"},
  {GRS0RecordSyntax,                           "1.2.840.10003.5.1000.6.1"},
  {SgmlRecordSyntax,                           "1.2.840.10003.5.1000.81.1"},


  // Add Extras here
  {DVBHtmlRecordSyntax,                        "DVB"}, // Private for DVB
  {TextHighlightRecordSyntax,                  "TEXT-HIGHLIGHT"}, // Private for IB 
  {HTMLHighlightRecordSyntax,                  "HTML-HIGHLIGHT"}, // Private for IB 
  {XMLHighlightRecordSyntax,                   "XML-HIGHLIGHT"}, // Private for IB 
  {HtmlRecordSyntax,                           "HTML"}, // Private
  {HtmlRecordSyntax,                           "HTMLRECORDSYNTAX"}, // Private
  {SUTRSRecordSyntax,                          "TEXT"}, // Private
  {SUTRSRecordSyntax,                          "SUTRS"}, // Private
  {SUTRSRecordSyntax,                          "SUTRSRECORDSYNTAX"}, // Private
  {XmlRecordSyntax,                            "XML"},
  {XmlRecordSyntax,                            "XMLRECORDSYNTAX"}
};

#define SIZEOF(X) (sizeof(X)/sizeof(X[0]))



class RecordSyntaxDict : public Object
{
public:
  RecordSyntaxDict() {
    trans = new Dictionary();
    for (size_t i = 0; i < SIZEOF(RecordSyntaxes); i++)
      {
        trans->Add(RecordSyntaxes[i].oid, (Object *)(RecordSyntaxes[i].syntax) );
      }
  };
  ~RecordSyntaxDict() {
    trans->Release();
    delete trans;
  };
  Object *Find (const STRING& name) const { return trans->Find(name); }

private:
  Dictionary    *trans;
};

static RecordSyntaxDict InternalOID;


RecordSyntax_t RecordSyntaxID(const STRING& String)
{
  Object *r = 0;

  if (!String.IsEmpty())
    {
      if (String.GetChr(1) != '1')
	r = InternalOID.Find(STRING(String).ToUpper()); 
      else
	r = InternalOID.Find(String);
    }
  return (r == 0 ? DefaultRecordSyntax : (RecordSyntax_t)(long)(r));
}


bool   RegisteredRecordSyntaxID(int Id)
{
  return (Id > 0 || Id < SIZEOF(RecordSyntaxes));
}

const char    *ID2RecordSyntax(RecordSyntax_t Id)
{
  int pos = (int)Id;
  if (RegisteredRecordSyntaxID(pos))
    return RecordSyntaxes[pos].oid;
  return "Unregistered Syntax";
}

const char *RecordSyntaxCannonical(const STRING& String,  RecordSyntax_t Default)
{
  RecordSyntax_t syntax = RecordSyntaxID(String);
  if (syntax == DefaultRecordSyntax) syntax = Default;
  return ID2RecordSyntax (syntax);
}



#ifdef MAIN_STUB
int main(int argc, char **argv)
{
  const char *oid = argv[1] ? argv[1] : "HTML";
  int   id  = (int)RecordSyntaxID(oid);

  printf("%s = %d, %s\n", oid, id, ID2RecordSyntax((RecordSyntax_t)id));
}

#endif
