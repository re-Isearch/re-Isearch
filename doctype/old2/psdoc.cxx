#include "psdoc.hxx"
#include "doc_conf.hxx"


static const char default_filter[] = "pstotext";

static const char *def_filter = default_filter; 

static const STRING mime ("application/postscript");

PSDOC::PSDOC(PIDBOBJ DbParent, const STRING& Name) : FILTER2TEXTDOC(DbParent, Name)
{
  SetMIME_Type(mime);
  if (!IsAbsoluteFilePath(ResolveBinPath(def_filter)))
    def_filter = "ps2ascii";
}

const char *PSDOC::Description(PSTRLIST List)  const
{
  static const char message[] = "Postscript Indexing via (external PS to Text) filter\n\
Option:\n\
   Filter\tSpecifies the program to use (default \"%s\")\n";
  static char       tmp[sizeof(default_filter)+sizeof(message) - 2 /* the %s */];

  const STRING ThisDoctype("PS");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  FILTER2TEXTDOC::Description(List);

  if (tmp[1] == '\0') sprintf(tmp, message, def_filter);
  return tmp;
}

void PSDOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr = mime;
}

const char *PSDOC::GetDefaultFilter() const
{
  return def_filter;
}

PSDOC::~PSDOC() { }


#if 0

static const char myDescription[] = "Adobe PS Plugin";

// Stubs for dynamic loading
extern "C" {
  PSDOC *  __plugin_adobe_pdf_create (IDBOBJ * parent, const STRING& Name)
  {
    return new PSDOC (parent, Name);
  }  
  int          __plugin_ps_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_ps_query (void) { return myDescription; }
}



#endif
