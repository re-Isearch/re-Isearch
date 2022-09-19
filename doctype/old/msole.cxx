/*@@@
File:		text.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include "doctype.hxx"

static const char *MyDescription = "M$ OLE type detector Plugin";

typedef enum MSApps            { Unknown, Word, Excel, Powerpoint } MSApps;
static const char *Plugins[] = { NULL, "MSWORD:", "MSEXCEL:", "MSPPT:"};

static const struct {
   const char   *ext;
   enum MSApps   typ;
} Extensions[] = {
  { "doc", Word},
  { "ppt", Powerpoint},
  { "xls", Excel},
  { "xlt", Excel},
  { "xlm", Excel},
  { "xld", Excel},
  { "xla", Excel},
  { "xlc", Excel},
  { "xlw", Excel},
  { "xll", Excel},
  { "dot", Word},
  { "wiz", Word},
  { "pot", Powerpoint},
  { "ppa", Powerpoint},
  { "pps", Powerpoint},
  { "pwz", Powerpoint},
  { "dot", Word},
  { "wiz", Word},
  { NULL,  Unknown },
};


class IBDOC_MSOLE : public DOCTYPE {
public:
   IBDOC_MSOLE(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
	if (List) {
	  List->AddEntry (Doctype);
	  DOCTYPE::Description(List);
	}
	return MyDescription;
   }
   void ParseRecords(const RECORD& FileRecord) {
	RECORD NewRecord(FileRecord);
	STRING fn (NewRecord.GetFileName());
	STRING ext (fn.Right('.') );
	const char * d = NULL;

	for (size_t i=0; Extensions[i].typ && !d; i++)
	  {
	    if (ext ^= Extensions[i].ext) d = Plugins[Extensions[i].typ];
	  }
	if (!d) // Unknown/Unsupported extensions
	  {
	    logf (LOG_WARN, "%s: File '%s' uses a unsupported M$ Office (OLE) type.",
		Doctype.c_str(), NewRecord.GetFullFileName().c_str() );
	    return;
	  }

	NewRecord.SetDocumentType(d);
	logf (LOG_INFO, "%s: Identified '%s' as %s", Doctype.c_str(), fn.c_str(), d);

	Db->ParseRecords (NewRecord);
   };

   ~IBDOC_MSOLE() {};
};


// Stubs for dynamic loading
extern "C" {
  IBDOC_MSOLE*    __plugin_msole_create (IDBOBJ * parent, const STRING& Name)
	{ return new IBDOC_MSOLE (parent, Name); }
  int          __plugin_msole_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_msole_query (void) { return MyDescription; }
}

IBDOC_MSOLE::IBDOC_MSOLE(PIDBOBJ DbParent, const STRING& Name) : DOCTYPE(DbParent, Name) { }


