/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*--@@@
File:		dtreg.cxx
Version:	3.00
Description:	Class DTREG - Document Type Registry
Author:		Edward C. Zimmermann
@@@*/

#include "common.hxx"
#include "doctype.hxx"
#include "dtreg.hxx"

#ifndef _WIN32
#include <iostream>
#include <iomanip>
#include "dirent.hxx"
#include <sys/stat.h>
#include <dlfcn.h>

#define HINSTANCE void *
#else
///////////// WIN32 ////////////////////////////////////
# ifndef _MSDOS
#  define _MSDOS
# endif
# define dlsym(_h,_name) ::GetProcAddress(_h,_name) 
# define dlopen(_f, _o)  ::LoadLibrary(_f)
# define dlclose(_h)     ::FreeLibrary(_h)
# define dlerror()	"Can't load"
#endif
#include "dtreg_list.hxx" /* List of "builtin" document types....  */

#pragma ident  "@(#)dtreg.cxx  1.80 03/01/01 00:33:05 BSN"

static const char DocDefine = ':';

#ifndef HOST_MACHINE_64
# error "Should have included platform.h"
#endif

static const char *PluginSymbolPrefix =  "__plugin_";


#if HOST_MACHINE_64

static const STRING PlugInsEntry ("PlugIns");
static const char *PluginExtension = ".sob";

#define HAVE_ALT_PLUGINS 1
static const STRING PlugInsEntryAlt ("PlugIns64");
static const char *PluginExtensionAlt = ".sob64";

#elif defined(O_BUILD_IB64) /* 64-bit IB on 32-bit platform */

static const STRING PlugInsEntry ("PlugIns64");
static const char *PluginExtension = ".sob64";

#else /* Plain old vanilla 32-bit IB */

static const STRING PlugInsEntry ("PlugIns");
static const char *PluginExtension = ".sob";

#define HAVE_ALT_PLUGINS 1
static const STRING PlugInsEntryAlt ("PlugIns32");
static const char *PluginExtensionAlt = ".sob32";


#endif


//
// To add a new doctype
// 1) Add an ID to the list below
// 2) Add its name and the id to the structure below
// 3) Add the creation routine to DTREG::GetDocTypePtr()
//
//
// List of document handlers
enum Doctypes {
  _UNDEFINED = 0, _NULL = 1,
  _AUTODETECT, _AOLLIST,
  _BIBCOLON,   _BIBTEX,
  _BINARY,     _COLONDOC,
  _COLONGRP,   _DIALOGB,
  _DIF,        _DIGESTTOC,
  _DOCTYPE,    _DVBLINE,
  _ENDNOTE,    _EUROMEDIA,
  _FILMLINE,   _FILTER2HTML,
  _FILTER2MEMO, _FILTER2TEXT,
  _FILTER2XML, _FIRSTLINE,
  /* 21 */
  _FTP,        _GILS,
  _GILSXML,    _HARVEST,
  _HTML,       _ANTIHTML,
  _HTMLCACHE,  _HTMLHEAD,
  _HTMLMETA,   _HTMLREMOTE,
  _IAFADOC,    _IKNOWDOC,
  _IRLIST,     _LISTDIGEST,
  _MAILDIGEST, _MAILFOLDER,
  _MAILMAN,    _MEDLINE,
  _MEMODOC,    _METADOC,
  _MISMEDIA,   _NEWSFOLDER,
  _NIL, // _NIL is not used
  /* 44 */
  _TBINARY,    _ONELINE,
  _OZSEARCH,   _PANDOC,
  _PAPYRUS,
  _PARA,       _PDFDOC,
  _PLAINTEXT,  _PSDOC,
  _PTEXT,      _RDF,
  _REFERBIB,
  /* 54 */
  _RESOURCEDOC,_RIS,
  _ROADSDOC,   _RSS091,
  _RSS1,       _RSS2,
  _SGML,       _SGMLNORM,
  _SGMLTAG,    _SIMPLE,
  _SOIF,       _TSLDOC,
  _TSVDOC,     _XBINARY,
  _FILTER,     _XFILTER,
  _XML,        _XMLBASE,
  /* 72 */
  _YAHOOLIST,	_ISOTEIA,
  _CAPRSS,	_RSSCORE,
  _HTMLZERO,	_RSSCOREARCHIVE,
  _ATOM,	_NEWSML,
  _XMLREC,      _CSVDOC,
  /* 80 */
  _IMAGEGIF,
  _IMAGEPNG,
  _IMAGETIFF,
  _IMAGEJPEG,
  _MAX_ID, // This is the "last real" doctype
  _PLUGIN = 126
};

// NOTE: For performance the list of the bulitin doctypes NEEDS to
// start off aligned with the enumerators
//

// Structure
static const struct {
    const char    *name;
    enum Doctypes  id;
    GDT_BOOLEAN    pub;
} builtin_doctypes[] = {
  { "<NIL>",      _UNDEFINED,  GDT_FALSE},{ "0",          _NULL,       GDT_FALSE},
  { "AUTODETECT", _AUTODETECT, GDT_TRUE}, { "AOLLIST",    _AOLLIST,    GDT_TRUE},
  { "BIBCOLON",   _BIBCOLON,   GDT_TRUE}, { "BIBTEX",     _BIBTEX,     GDT_TRUE},
  { "BINARY",     _BINARY,     GDT_TRUE}, { "COLONDOC",   _COLONDOC,   GDT_TRUE},
  { "COLONGRP",   _COLONGRP,   GDT_TRUE}, { "DIALOG-B",   _DIALOGB,    GDT_TRUE},

  { "DIF",        _DIF,        GDT_TRUE}, { "DIGESTTOC",  _DIGESTTOC,  GDT_FALSE},
  { "DOCTYPE",    _DOCTYPE,    GDT_FALSE},{ "DVBLINE",    _DVBLINE,    GDT_TRUE},
  { "ENDNOTE",    _ENDNOTE,    GDT_TRUE}, { "EUROMEDIA",  _EUROMEDIA,  GDT_TRUE},
  { "FILMLINE",   _FILMLINE,   GDT_TRUE}, { "FILTER2HTML",_FILTER2HTML,GDT_TRUE}, 
  { "FILTER2MEMO",_FILTER2MEMO,GDT_TRUE}, { "FILTER2TEXT",_FILTER2TEXT,GDT_TRUE},

  { "FILTER2XML", _FILTER2XML, GDT_TRUE}, { "FIRSTLINE",  _FIRSTLINE,  GDT_TRUE},
  { "FTP",        _FTP,        GDT_TRUE}, { "GILS",       _GILS,       GDT_TRUE},
  { "GILSXML",    _GILSXML,    GDT_TRUE}, { "HARVEST",    _HARVEST,    GDT_TRUE},
  { "HTML",       _HTML,       GDT_TRUE}, { "HTML--",     _ANTIHTML,   GDT_TRUE},
  { "HTMLCACHE",  _HTMLCACHE,  GDT_TRUE}, { "HTMLHEAD",   _HTMLHEAD,   GDT_TRUE},

  { "HTMLMETA",   _HTMLMETA,   GDT_TRUE}, { "HTMLREMOTE", _HTMLREMOTE, GDT_TRUE},
  { "IAFADOC",    _IAFADOC,    GDT_TRUE}, { "IKNOWDOC",   _IKNOWDOC,   GDT_TRUE},
  { "IRLIST",     _IRLIST,     GDT_TRUE}, { "LISTDIGEST", _LISTDIGEST, GDT_TRUE},
  { "MAILDIGEST", _MAILDIGEST, GDT_TRUE}, { "MAILFOLDER", _MAILFOLDER, GDT_TRUE},
  { "MAILMAN",    _MAILMAN,    GDT_FALSE},{ "MEDLINE",    _MEDLINE,    GDT_TRUE},

  { "MEMO",       _MEMODOC,    GDT_TRUE}, { "METADOC",    _METADOC,    GDT_TRUE},
  { "MISMEDIA",   _MISMEDIA,   GDT_TRUE}, { "NEWSFOLDER", _NEWSFOLDER, GDT_TRUE},
  { "NULL",       _NULL,       GDT_FALSE},
  { "OCR",        _TBINARY,    GDT_TRUE}, { "ONELINE",    _ONELINE,    GDT_TRUE},
  { "OZSEARCH",   _OZSEARCH,   GDT_TRUE}, { "PANDOC",     _PANDOC,     GDT_TRUE},

  { "PAPYRUS",    _PAPYRUS,    GDT_TRUE},
  { "PARA",       _PARA,       GDT_TRUE}, { "PDF",        _PDFDOC,     GDT_TRUE},
  { "PLAINTEXT",  _PLAINTEXT,  GDT_TRUE}, { "PS",         _PSDOC,      GDT_TRUE},
  { "PTEXT",      _PTEXT,      GDT_TRUE}, { "RDF",        _RDF,        GDT_TRUE},
  { "REFERBIB",   _REFERBIB,   GDT_TRUE},

  { "RESOURCE",   _RESOURCEDOC,GDT_FALSE},{ "RIS",        _RIS,        GDT_TRUE},
  { "ROADS++",    _ROADSDOC,   GDT_TRUE}, { "RSS.9x",     _RSS091,     GDT_TRUE},
  { "RSS1",       _RSS1,       GDT_TRUE}, { "RSS2",       _RSS2,       GDT_TRUE},
  { "SGML",       _SGML,       GDT_TRUE}, { "SGMLNORM",   _SGMLNORM,   GDT_TRUE},
  { "SGMLTAG",    _SGMLTAG,    GDT_TRUE}, { "SIMPLE",     _SIMPLE,     GDT_TRUE},

  { "SOIF",       _SOIF,       GDT_TRUE}, { "TSVDOC",     _TSLDOC,     GDT_TRUE},
  { "TSV",        _TSVDOC,     GDT_TRUE}, { "XBINARY",    _XBINARY,    GDT_TRUE},
  { "FILTER",     _FILTER,     GDT_TRUE}, { "XFILTER",    _XFILTER,    GDT_TRUE},
  { "XML",         _XML,       GDT_TRUE}, { "XMLBASE",    _XMLBASE,    GDT_TRUE},
  { "YAHOOLIST",  _YAHOOLIST,  GDT_TRUE}, { "ISOTEIA",    _ISOTEIA,    GDT_TRUE},

  { "CAP",        _CAPRSS,     GDT_TRUE}, { "RSSCORE",    _RSSCORE,    GDT_TRUE},
  { "HTMLZERO",   _HTMLZERO,   GDT_TRUE}, { "RSSARCHIVE", _RSSCOREARCHIVE, GDT_TRUE},
  { "ATOM",       _ATOM,       GDT_TRUE}, { "NEWSML",     _NEWSML,     GDT_TRUE},
  { "XMLREC",     _XMLREC,     GDT_TRUE}, { "CSVDOC",     _CSVDOC,     GDT_TRUE},

  /* Image Formats */
  { "GIF",        _IMAGEGIF,   GDT_FALSE}, { "PNG",        _IMAGEPNG,   GDT_FALSE},
  { "TIFF",       _IMAGETIFF,  GDT_FALSE}, { "JPEG",       _IMAGEJPEG,  GDT_FALSE},

  /* Aliases */
  { "TBINARY",    _TBINARY,     GDT_FALSE},
  { "MEMODOC",    _MEMODOC,     GDT_FALSE},
  { "TEXT",       _PLAINTEXT,   GDT_FALSE},
  { "IMAGE",      _RESOURCEDOC, GDT_FALSE},
  { "MOVIE",      _RESOURCEDOC, GDT_FALSE},
  { "OBJ",        _RESOURCEDOC, GDT_FALSE},
  { "SOUND",      _RESOURCEDOC, GDT_FALSE},
  { "XMLGILS",    _GILSXML,     GDT_FALSE},
  { "RSS",        _RSS2,        GDT_FALSE},
  { "RSS091",     _RSS091,      GDT_FALSE},
  { "RSS.9",      _RSS091,      GDT_FALSE},
  { "CORERSS",    _RSSCORE,     GDT_FALSE},
  { "ALERT",      _CAPRSS,      GDT_FALSE},
  { "ROADSDOC",   _ROADSDOC,    GDT_FALSE},

  { "JPG",        _IMAGEJPEG,   GDT_FALSE},
  { "TIF",        _IMAGETIFF,   GDT_FALSE},

  { "ODT",        _PANDOC,      GDT_TRUE},
  { "DOCX",       _PANDOC,      GDT_TRUE},
  { "LATEX",      _PANDOC,      GDT_TRUE},
  { "MARKDOWN",   _PANDOC,      GDT_TRUE},
  { "JIRA",      _PANDOC,      GDT_TRUE},
  { "JSON",       _PANDOC,      GDT_FALSE}, // Supports ONLY Pandoc created JSON



  { "CSLDOC",     _CSVDOC,      GDT_FALSE}, // Old name
  { "TSLDOC",     _TSLDOC,      GDT_FALSE}, // Old name


  { "PLUGIN",     _PLUGIN,      GDT_FALSE}
};

// This is a dummy
class _DummyDoc : public DOCTYPE {
public:
   _DummyDoc(PIDBOBJ DbParent, const STRING& Name) : DOCTYPE(DbParent) {
	Doctype = Name; }
   const char *Description(PSTRLIST List) const {
        List->AddEntry(Doctype);
        DOCTYPE::Description(List);
	return "Internal NO-OP Document class";
   }
   void ParseRecords (const RECORD&) {}
   GPTYPE ParseWords(UCHR*, GPTYPE, GPTYPE, GPTYPE*, GPTYPE) { return 0; }
   ~_DummyDoc() {}
};



class DoctypesClassRegistry : public Object
{
public:
  DoctypesClassRegistry() {
    trans = new Dictionary(101, 1.0); // was 31  edz  9 Jun 2003
  } 
  ~DoctypesClassRegistry() {
    Object *dt_obj;
    trans->Start_Get();
    while ((dt_obj = trans->Get_NextObject()) != NULL)
      delete (DOCTYPE *)dt_obj;
    trans->Release();
    delete trans;
  }
  DOCTYPE *Find (const STRING& name) const {
    return (DOCTYPE *)trans->Find(name.c_str());
  }
  void      Remove(const STRING& name) const {
    trans->Remove(name);
  }
  DOCTYPE  *Add (const STRING& Name,  DOCTYPE* dt_obj) {
    trans->Add(Name, (Object *) dt_obj);
    return dt_obj;
  }
private:
  Dictionary    *trans;
};

static DoctypesClassRegistry *globalDoctypesRegistry = NULL;


DOCTYPE *DTREG::RegisterDocType (const STRING& name, DOCTYPE* Ptr)
{
  if (Ptr && !name.IsEmpty())
    {
//cerr << "Registering " << name << " with ptr " << (long long)Ptr << endl;
      if (DoctypesRegistry)
	DoctypesRegistry->Add(name, Ptr);
//cerr << "Load field table" << endl;
      Ptr->LoadFieldTable();
//cerr << "Done" << endl;
      return Ptr;
    }
  return NULL;
}


GDT_BOOLEAN  DTREG::UnregisterDocType (const STRING& name)
{
  DOCTYPE *Ptr = DoctypesRegistry ? DoctypesRegistry->Find(name) : NULL;
  if (Ptr)
    {
      DoctypesRegistry->Remove(name);
      delete Ptr;
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


#define SIZEOF(X) (sizeof(X)/sizeof(X[0]))

class DocumentTypes : public Object
{
public:
  DocumentTypes() {
    trans = new Dictionary(SIZEOF(builtin_doctypes)+1, 1.0);
    if (trans) init();
  }

  ~DocumentTypes() {
    if (trans)
      {
	trans->Release();
	delete trans;
      }
  }

  Object *Find (const char *name) const {
    return trans ? trans->Find(name) : NULL;
  }
private:
  void           init();
  Dictionary    *trans;
};

static DocumentTypes Handlers;


void DocumentTypes::init()
{
  for (size_t i = 0; i < SIZEOF(builtin_doctypes); i++)
    {
      // if (builtin_doctypes[i].id != i && builtin_doctypes[i].pub) cerr << "TRIPPING: " << builtin_doctypes[i].name << endl;
      trans->Add(builtin_doctypes[i].name, (Object *) builtin_doctypes[i].id);
    }
}


// Map Document type name (class) to its enumeration (Id)
static long Doctype2Id(const STRING& DocType)
{
  STRING          Doctype(DocType);
  STRINGINDEX     pos =  Doctype.SearchReverse( DocDefine );

  if (pos > 2 && pos == Doctype.GetLength())
    {
      return _PLUGIN;
    }
  else if (pos > 1)
    {
      Doctype.EraseBefore(pos + 1);
    }
  if (Doctype.IsEmpty())
    return  _NULL;

  const Object *r = Handlers.Find(Doctype.ToUpper());
  return  builtin_doctypes[(long)(r)].id;
}

DOCTYPE_ID::DOCTYPE_ID()
{
  Id = _UNDEFINED;
// Name = NulString;
}

static void String2DOCTYPE_ID(DOCTYPE_ID *Ptr, const STRING& DocType)
{
  static STRING LastDoctype;
  static STRING LastChild;
  static int    LastId;

  if (DocType == LastDoctype)
    {
      Ptr->Name = LastChild;
      Ptr->Id   = LastId;
    }
  else
    {
      STRING        Child (DocType);

      STRINGINDEX   pos = Child.SearchReverse( DocDefine );
      if (pos > 1 && pos != Child.GetLength())
	{
	  // Remove multiple : 
	  while (Child.GetChr(pos-1) == DocDefine) pos-- ;
          Child.EraseAfter(pos-1);
	}
/*
      else
        Child = NulString;
*/
      LastChild   = Ptr->Name = Child;
      LastId      = Ptr->Id   = Doctype2Id(DocType);

      LastDoctype = DocType;
    }
}

STRING DOCTYPE_ID::DocumentType() const
{
  STRING myName = Name;
  if (myName.IsEmpty())
    {
      if (Id < _MAX_ID && Id >= _NULL)
	myName = (STRING)builtin_doctypes[Id].name;
    }
  else
    {
      STRINGINDEX pos    = myName.Search( DocDefine );
      if (pos) myName.EraseAfter(pos-1);
    }
  return myName; 

}

void  DOCTYPE_ID::Set(const STRING& Value)
{
  if (Value.IsEmpty())
    {
      Name.Clear();
      Id = _UNDEFINED;
    }
  else
    {
      const char *ptr = strchr(Value, '/');
      if (ptr)
	{
	  if ((Id = atoi(Value.c_str())) > 0)
	    Name = ptr+1;
	  else
	    Id = _UNDEFINED;
	}
      else *this = Value;
    }
}


STRING  DOCTYPE_ID::Get() const
{
  STRING String;
  const STRING name ( Name.IsEmpty() ?
        (Id != _PLUGIN ? (STRING)builtin_doctypes[Id].name : NulString ) : Name );
  if (IsDefined() && Id != _PLUGIN)
    String.form("%d/%s", Id, name.c_str());
  else
    String = name;
  return String;
}


STRING DOCTYPE_ID::ClassName(GDT_BOOLEAN Base) const
{
  STRING className (Name);
  if (Id < _MAX_ID && Id >= _NULL)
    {
      const char *myName = builtin_doctypes[Id].name;
      if (Base == GDT_TRUE)
	className = myName;
      else if (className != myName)
	className << DocDefine << myName;
    }
  return className;
}


DOCTYPE_ID::DOCTYPE_ID(const char *DocType)
{
   String2DOCTYPE_ID(this, DocType);
}

DOCTYPE_ID::DOCTYPE_ID(const STRING& DocType)
{
  String2DOCTYPE_ID(this, DocType);
}

DOCTYPE_ID& DOCTYPE_ID:: operator =(const DOCTYPE_ID& NewId)
{
  Name = NewId.Name;
  Id   = NewId.Id;
  return *this;
}

void DOCTYPE_ID::Write(FILE *fp) const
{
  ::Write(Name, fp);
  ::Write(Id, fp);
}

GDT_BOOLEAN DOCTYPE_ID::Read(FILE *fp)
{
  ::Read(&Name, fp);
  ::Read(&Id, fp);
  return GDT_TRUE;
}


DOCTYPE_ID::~DOCTYPE_ID() { }


DTREG::DTREG(PIDBOBJ DbParent)
{
#ifndef _WIN32
  STRING SearchPath("~/.ib/plugins:/var/opt/nonmonotonic/ib/lib/plugins:\
~asfadmin/lib/plugins:~ibadmin/lib/plugins:/opt/nonmonotonic/ib/lib/plugins:.");
  PluginsSearchPath.SplitPaths(SearchPath);
#endif
  Db = DbParent;
  DoctypesRegistry = globalDoctypesRegistry ? globalDoctypesRegistry : new DoctypesClassRegistry();
  pluginsLoaded = 0;
}

DTREG::DTREG(PIDBOBJ DbParent, const STRING& PluginsPath)
{
  PluginsSearchPath.SplitPaths(PluginsPath);
  Db = DbParent;
  DoctypesRegistry = globalDoctypesRegistry ? globalDoctypesRegistry : new DoctypesClassRegistry();
  pluginsLoaded = 0;
}


GDT_BOOLEAN DTREG::PluginExists(const STRING& Doctype) const
{
  GDT_BOOLEAN not_found = GDT_TRUE;
  STRING name;

 if (Doctype.SearchReverse( DocDefine ) != Doctype.GetLength())
    return PluginExists(Doctype + ":");

  Db->ProfileGetString(PlugInsEntry, Doctype, Doctype, &name);
  name.ToLower();

  if (name.SearchReverse( DocDefine ) == name.GetLength())
    name.EraseAfter(name.GetLength()-1);

  STRING dt_fname = FindSharedLibrary(name + PluginExtension);

#if HAVE_ALT_PLUGINS
  if (dt_fname.IsEmpty() || !FileExists(dt_fname))
    dt_fname =  FindSharedLibrary(name + PluginExtensionAlt);
#endif
  for (const STRLIST *p = PluginsSearchPath.Next();
	(not_found = (!FileExists(dt_fname))) &&  p != &PluginsSearchPath;
	p = p->Next())
    {
      dt_fname = AddTrailingSlash(p->Value()) + name + PluginExtension;
#if HAVE_ALT_PLUGINS
      if (!FileExists(dt_fname))
	dt_fname = AddTrailingSlash(p->Value()) + name + PluginExtensionAlt;
#endif
    }
  return !not_found;
}

int    DTREG::Version() const
{
  char tmp[12];
 
  sprintf(tmp, "2%d",  _MAX_ID-1);
  return (int) (100.0 * atof(tmp));
}

GDT_BOOLEAN     DTREG::ValidateDocType(const STRING& DocType)
{
  int id = DoctypeId(DocType);

  if (id == _PLUGIN)
   {
      DOCTYPE_ID d;
      d.Id = id;
      d.Name = DocType;
      return NULL != GetDocTypePtr(d);
   }
  return id > 0;
}

GDT_BOOLEAN     DTREG::ValidateDocType(const DOCTYPE_ID& Id)
{
  const int id = Id.Id;
  if (id == _PLUGIN)
    return NULL != GetDocTypePtr(Id);
  return id > 0;
}

int		DTREG::DoctypeId(const STRING& DocType)
{
  // Check cache (speeds things up)
  if (DocType == LastDoctype)
    {
      return LastId;
    }
  LastDoctype = DocType;
  return LastId = Doctype2Id(DocType); 
}

PDOCTYPE        DTREG::GetDocTypePtr(UINT2 Id)
{
  if (Id > (sizeof(builtin_doctypes)/sizeof(builtin_doctypes[0])))
    return 0;
  // This depends upon the design of the bultin list where
  // builtin_doctypes[Id].id == Id 
  return GetDocTypePtr((STRING)builtin_doctypes[Id].name);
}

PDOCTYPE        DTREG::GetDocTypePtr(const STRING& Doctype)
{
  return GetDocTypePtr((DOCTYPE_ID)Doctype);
}

PDOCTYPE        DTREG::GetDocTypePtr(const STRING& DocType, const STRING& DoctypeID)
{
  DOCTYPE_ID   newDoctype;
  newDoctype.Id   = DoctypeId(DocType);
  newDoctype.Name = DoctypeID.IsEmpty() ? DocType : DoctypeID;
  return GetDocTypePtr(newDoctype);
}

PDOCTYPE        DTREG::GetDocTypePtr(const DOCTYPE_ID& DoctypeId)
{
  const INT    Id        = DoctypeId.Id;
  if (Id != _PLUGIN && (Id < 0 || Id > (INT)(sizeof(builtin_doctypes)/sizeof(builtin_doctypes[0]))))
    return 0;

  const STRING Name = DoctypeId.Name.IsEmpty() ?
        (Id != _PLUGIN ? (STRING)builtin_doctypes[Id].name : NulString ) :
	DoctypeId.Name;
  const STRING Ident ( DoctypeId.Get() );

  DOCTYPE* dt_obj = DoctypesRegistry ? DoctypesRegistry->Find ( Ident ) : NULL;
  if (dt_obj)
    return dt_obj;

  switch (Id) {
    case _DOCTYPE:
      return RegisterDocType (Ident, new DOCTYPE(Db, Name));
    case _NULL:
      return RegisterDocType (Ident, Db ? new _DummyDoc(Db, Name) : NULL);
    case _AUTODETECT:
      return RegisterDocType (Ident, new AUTODETECT(Db, Name));
    case _PLAINTEXT:
      return RegisterDocType (Ident, new PLAINTEXT(Db, Name));
    case _ONELINE:
      return RegisterDocType (Ident, new ONELINE(Db, Name));
    case _SIMPLE:
      return RegisterDocType (Ident, new SIMPLE(Db, Name));
    case _PTEXT:
      return RegisterDocType (Ident, new PTEXT(Db, Name));
    case _PARA:
      return RegisterDocType (Ident, new PARA(Db, Name));
    case _SGMLTAG:
      return RegisterDocType (Ident, new SGMLTAG(Db, Name));
    case _GILS:
      return RegisterDocType (Ident, new GILS(Db, Name));
    case _FIRSTLINE:
      return RegisterDocType (Ident, new FIRSTLINE(Db, Name));
    case _BINARY:
      return RegisterDocType (Ident, new BINARY(Db, Name));
    case _TBINARY:
      return RegisterDocType (Ident, new TBINARY(Db, Name));
    case _FILTER:
      return RegisterDocType (Ident, new FILTERDOC(Db, Name));
    case _FTP:
      return RegisterDocType (Ident, new FTP(Db, Name));
    case _COLONDOC:
      return RegisterDocType (Ident, new COLONDOC(Db, Name));
    case _METADOC:
      return RegisterDocType (Ident, new METADOC(Db, Name));
    case _DIALOGB:
      return RegisterDocType (Ident, new DIALOGB(Db, Name));
    case _TSLDOC:
      return RegisterDocType (Ident, new TSLDOC(Db, Name));
    case _CSVDOC:
      return RegisterDocType (Ident, new CSVDOC(Db, Name));
    case _TSVDOC:
      return RegisterDocType (Ident, new TSVDOC(Db, Name));
    case _OZSEARCH:
      return RegisterDocType (Ident, new OZSEARCH(Db, Name));
    case _IAFADOC:
      return RegisterDocType (Ident, new IAFADOC(Db, Name));
    case _MAILFOLDER:
      return RegisterDocType (Ident, new MAILFOLDER(Db, Name));
    case _MAILMAN:
       return RegisterDocType (Ident, new MAILMAN(Db, Name));
    case _NEWSFOLDER:
      return RegisterDocType (Ident, new NEWSFOLDER(Db, Name));
    case _REFERBIB:
      return RegisterDocType (Ident, new REFERBIB(Db, Name));
    case _ENDNOTE:
      return RegisterDocType (Ident, new REFERBIB_ENDNOTE(Db, Name));
    case _PAPYRUS:
      return RegisterDocType (Ident, new REFERBIB_PAPYRUS(Db, Name));
    case _IRLIST:
      return RegisterDocType (Ident, new IRLIST(Db, Name));
    case _LISTDIGEST:
      return RegisterDocType (Ident, new LISTDIGEST(Db, Name));
    case _YAHOOLIST:
      return RegisterDocType (Ident, new YAHOOLIST(Db, Name));
    case _AOLLIST:
      return RegisterDocType (Ident, new AOLLIST(Db, Name));
    case _MAILDIGEST:
      return RegisterDocType (Ident, new MAILDIGEST(Db, Name));
    case _DIGESTTOC:
      return RegisterDocType (Ident, new DIGESTTOC(Db, Name));
    case _MEDLINE:
      return RegisterDocType (Ident, new MEDLINE(Db, Name));
    case _RIS:
      return RegisterDocType (Ident, new MEDLINE_RIS(Db, Name));
    case _FILMLINE:
      return RegisterDocType (Ident, new FILMLINE(Db, Name));
    case _DVBLINE:
      return RegisterDocType (Ident, new DVBLINE(Db, Name));
    case _MEMODOC:
      return RegisterDocType (Ident, new MEMODOC(Db, Name));
    case _SGMLNORM:
      return RegisterDocType (Ident, new SGMLNORM(Db, Name));
    case _SGML:
      return RegisterDocType (Ident, new SGML(Db, Name));
    case _XBINARY:
      return RegisterDocType (Ident, new XBINARY(Db, Name));
    case _XML:
      return RegisterDocType (Ident, new XML(Db, Name));
    case _XMLBASE:
      return RegisterDocType (Ident, new XMLBASE(Db, Name));
    case _XFILTER:
      return RegisterDocType (Ident, new XFILTER(Db, Name));
    case _XMLREC:
      return RegisterDocType (Ident, new XMLREC(Db, Name));
    case _RDF:
      return RegisterDocType (Ident, new RDFREC(Db, Name));
    case _GILSXML:
      return RegisterDocType (Ident, new GILSXML(Db, Name));
    case _RSS091:
    case _RSS2:
      return RegisterDocType (Ident, new RSS2 (Db, Name));
    case _RSSCORE:
      return RegisterDocType (Ident, new RSSCORE (Db, Name));
    case _RSSCOREARCHIVE:
      return RegisterDocType (Ident, new RSSCOREARCHIVE (Db, Name));
    case _CAPRSS:
      return RegisterDocType (Ident, new CAP_RSS (Db, Name));
    case _ATOM:
      return RegisterDocType (Ident, new IETF_ATOM (Db, Name));
    case _NEWSML:
      return RegisterDocType (Ident, new NEWSML (Db, Name));
    case _HTML:
      return RegisterDocType (Ident, new HTML(Db, Name));
    case _HTMLMETA:
      return RegisterDocType (Ident, new HTMLMETA(Db, Name));
    case _HTMLHEAD:
      return RegisterDocType (Ident, new HTMLHEAD(Db, Name));
    case _HTMLZERO:
      return RegisterDocType (Ident, new HTMLZERO(Db, Name));
    case _HTMLREMOTE:
      return RegisterDocType (Ident, new HTMLREMOTE(Db, Name));
    case _FILTER2HTML:
      return RegisterDocType (Ident, new FILTER2HTMLDOC(Db, Name));
    case _FILTER2MEMO:
      return RegisterDocType (Ident, new FILTER2MEMODOC(Db, Name));
    case _FILTER2TEXT:
      return RegisterDocType (Ident, new FILTER2TEXTDOC(Db, Name));
    case _FILTER2XML:
      return RegisterDocType (Ident, new FILTER2XMLDOC(Db, Name));
    case _PANDOC:
      return RegisterDocType (Ident, new PANDOC(Db, Name));
    case _ANTIHTML:
      return RegisterDocType (Ident, new ANTIHTML(Db, Name));
    case _HTMLCACHE:
      return RegisterDocType (Ident, new HTMLCACHE(Db, Name));
    case _BIBTEX:
      return RegisterDocType (Ident, new BIBTEX(Db, Name));
    case _SOIF:
      return RegisterDocType (Ident, new SOIF(Db, Name));
    case _HARVEST:
      return RegisterDocType (Ident, new HARVEST(Db, Name));
    case _IKNOWDOC:
      return RegisterDocType (Ident, new IKNOWDOC(Db, Name));
    case _ROADSDOC:
      return RegisterDocType (Ident, new ROADSDOC(Db, Name));
    case _COLONGRP:
      return RegisterDocType (Ident, new COLONGRP(Db, Name));
    case _BIBCOLON:
      return RegisterDocType (Ident, new BIBCOLON(Db, Name));
    case _EUROMEDIA:
      return RegisterDocType (Ident, new EUROMEDIA(Db, Name));
    case _MISMEDIA:
      return RegisterDocType (Ident, new MISMEDIA(Db, Name));
    case _DIF:
      return RegisterDocType (Ident, new DIF(Db, Name));
    case _PDFDOC:
      return RegisterDocType (Ident, new ADOBE_PDFDOC(Db, Name));
    case _PSDOC:
      return RegisterDocType (Ident, new PSDOC(Db, Name));
    case _RESOURCEDOC:
      return RegisterDocType (Ident, new RESOURCEDOC(Db, Name));
    case _IMAGEGIF:
      return RegisterDocType (Ident, new IMAGEGIF(Db, Name));
    case _IMAGEPNG:
      return RegisterDocType (Ident, new IMAGEPNG(Db, Name));
    case _IMAGETIFF:
      return RegisterDocType (Ident, new IMAGETIFF(Db, Name));
    case _IMAGEJPEG:
      return RegisterDocType (Ident, new IMAGEJPEG(Db, Name));
    case _ISOTEIA:
      return RegisterDocType (Ident, new GILS_ISOTEIA(Db, Name));
    case _PLUGIN:
      // Now look at the plugins
      STRING DocType = DoctypeId.Name;
      STRING myName  = DoctypeId.Name;
      STRINGINDEX pos = DocType.Search( DocDefine );
      if (pos)
	{
	  DocType.EraseBefore(pos + 1);
	  myName.EraseAfter(pos-1);
	  if ((pos = DocType.SearchReverse( DocDefine)) != 0)
	    DocType.EraseAfter(pos-1);
	}
      if (DocType.IsEmpty())
	DocType = myName; // Nothing
      if (DoctypesRegistry && (dt_obj = DoctypesRegistry->Find (myName)) != NULL)
	return dt_obj;
      // Defined in the .ini?
      if (Db)
	{
	  STRING Fn;
	  Db->ProfileGetString(PlugInsEntry, DocType, NulString, &Fn);
	  if (!Fn.IsEmpty())
	    {
	      STRING dt_name (DocType);
	      dt_name.ToLower();
	      // Open Plugin Contructor 
	      dt_constr_t create = open_doctype_constr(Fn, dt_name);
	      if (create != NULL)
		{
		  // Now Register
		  RegisterDocType(Name, dt_obj = create (Db, myName));
		  pluginsLoaded++;
		  return dt_obj;
		}
	    }
	}

      // Look in FindSharedLibrary(DocType +  PluginExtension);
      { STRING Fn (DocType); STRING dt_name (FindSharedLibrary(Fn.ToLower() + PluginExtension));
        if (Fn.GetLength() && FileExists(Fn))
	  {
	    dt_constr_t create = open_doctype_constr(Fn, dt_name);
	    if (create != NULL)
	      {
		RegisterDocType(Name, dt_obj = create (Db, myName));
		pluginsLoaded++;
		return dt_obj;
	      }
	  }
      }
      // Search Path...
      for (const STRLIST *p = PluginsSearchPath.Next(); p != &PluginsSearchPath; p = p->Next())
        {
	  // Just return the first one...
	  dt_constr_t create = get_doctype_constr (p->Value(), DocType);
	  if (create != NULL)
	    {
	      RegisterDocType(Name, dt_obj = create (Db, myName));
	      pluginsLoaded++;
	      return dt_obj;
	    }
	}
      break;
  } // switch()
  return 0;
}

void  DTREG::AddPluginPath(const STRING& Path)
{
  if (PluginsSearchPath.Search(Path))
    return; // Already in path
  if (!DocTypeList.IsEmpty())
    BuildPluginList (Path, DocTypeList);
  PluginsSearchPath.AddEntry(Path);
}

// We don't announce our internal builtin_doctypes, eg. RESOURCE
const STRLIST&     DTREG::GetDocTypeList()
{
  if (DocTypeList.IsEmpty())
    {
      size_t i;
      for (i = 0; i < SIZEOF(builtin_doctypes); i++)
	{
	  if (builtin_doctypes[i].pub)
	    DocTypeList.AddEntry(builtin_doctypes[i].name);
	}
      DocTypeList.Sort();
      for (const STRLIST *p = PluginsSearchPath.Next(); p != &PluginsSearchPath; p = p->Next())
	BuildPluginList (p->Value(), DocTypeList);
    }
  return DocTypeList;
}

void              DTREG::PrintDoctypeList(ostream& os) const
{
  // A little bit of effort to print the list nicely
  os << "Available Built-in Document Base Classes (v" <<  (double)(Version()/1000.0) << "):";
  size_t i=0;
  STRLIST list;

  for (i = 0; i < SIZEOF(builtin_doctypes); i++)
    {
      if (builtin_doctypes[i].pub)
	list.AddEntry(builtin_doctypes[i].name);
    }
  list.Sort();

  int j=0;
  for (STRLIST *ptr = list.Next(); ptr != &list; ptr=ptr->Next())
    {
      size_t z;
      STRING name = ptr->Value();

      if (j++ % ((80/(DocumentTypeSize))-1) == 0)
	os << endl;
#ifndef _WIN32
      if ((z = name.GetLength() ) < DocumentTypeSize)
	os << setw(DocumentTypeSize-z) << " ";
      else
#endif
	os << " ";
      os << name;
    }

#ifndef _MSDOS
  STRLIST dtlist, qlist;
  for (const STRLIST *p = PluginsSearchPath.Next(); p != &PluginsSearchPath; p = p->Next())
    BuildPluginList (p->Value(), dtlist, &qlist);
  if (!dtlist.IsEmpty())
    {
      os << endl << "External Base Classes (\"Plugin Doctypes\"):" << endl;
      for (const STRLIST *dt = dtlist.Next(), *q = qlist.Next(); dt != &dtlist; dt = dt->Next(), q = q->Next())
	{
	  os << "  " << dt->Value() << ":"
		<< setw(3+DocumentTypeSize-dt->Value().GetLength())
		<< " " << "// " << q->Value() << endl;
	}
    }
#endif
  os << endl;
}

void           DTREG::PrintDoctypeHelp(const STRING& DoctypeName, ostream& os)
{
  STRING Help ("NO HELP AVAILABLE");
  STRING Doctype (DoctypeName);

  if (Doctype.Pack().IsEmpty())
   Doctype = "DOCTYPE";
  PDOCTYPE DoctypePtr = GetDocTypePtr (Doctype);

  if (DoctypePtr == NULL)
    {
      // Plugin?
      if ((DoctypePtr = GetDocTypePtr (Doctype + DocDefine)) != NULL)
	{
	  os << "Warning: " << Doctype << " is available ONLY as plugin" << endl;
	  Doctype.Cat( DocDefine );
	}
    }
  size_t  x = Doctype.SearchReverse(DocDefine);
  if (x) 
    {
      os << (x == (Doctype.GetLength()) ? "Plugin " : "Derived ");
    }
  os << '"' << Doctype.ToUpper() << "\": ";
  if (DoctypePtr)
    DoctypePtr->Help(&Help);
  os << Help << endl;
}

static inline int PluginVersion(HINSTANCE handle, const STRING& module)
{
  int version = -1;
  if (handle)
    {
      STRING symbol (PluginSymbolPrefix);
      symbol += module;
      symbol += "_id";
      message_log (LOG_DEBUG, "Symbol '%s' load", symbol.c_str());
      int (*func)(void) = (int(*)())dlsym (handle, symbol.c_str());
      if (func)
        version = func();
    }
  return version;
}

static inline dt_constr_t Creator(HINSTANCE handle, const STRING &module)
{
  dt_constr_t create = NULL;
  if (handle)
    {
      STRING symbol (PluginSymbolPrefix);
      symbol += module;
      symbol += "_create";
      message_log (LOG_DEBUG, "Symbol '%s' load", symbol.c_str());
      create = (dt_constr_t) dlsym (handle, symbol.c_str());
    }
  return create;
}

static inline STRING Description(HINSTANCE handle, const STRING &module)
{
  const char *description = NULL;
  if (handle)
    {
      STRING symbol (PluginSymbolPrefix);
      symbol += module;
      symbol += "_query";
      dt_query_t func = (dt_query_t) dlsym (handle, symbol.c_str());
      if (func)
	description = func();
    }
  return description ? (STRING)description : NulString;
}


dt_constr_t DTREG::get_doctype_constr (const STRING& dir, const STRING& doctype) const
{
  dt_constr_t create;

  // The name of the file to open
  STRING dt_name (doctype);
  dt_name.ToLower();

#ifdef RTLD_DEFAULT 
  if ((create = Creator(RTLD_DEFAULT, dt_name)) != NULL)
    return create;
#endif

  STRING dt_fname = AddTrailingSlash(dir) + dt_name + PluginExtension;

  if ((create = open_doctype_constr(dt_fname, dt_name)) != NULL)
    if (Db) Db->ProfileWriteString(PlugInsEntry, doctype, dt_fname);
  return create;
}


dt_constr_t DTREG::open_doctype_constr(const STRING& dt_fname, const STRING& doctype) const
{
  dt_constr_t create = 0;
  struct stat buf;

  if (stat (dt_fname, &buf) != -1 && S_ISREG(buf.st_mode))
    {
      // Yes, it's a file...
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
      // In Windows this is Openlibrary( );
      HINSTANCE handle = dlopen (dt_fname, RTLD_GLOBAL | RTLD_NOW);
      if (handle)
	{
	  if (PluginVersion(handle, doctype) == DoctypeDefVersion)
	    {
	      // We're almost there.  Try to find a constructor.
	      create = Creator(handle, doctype);
	    }
	  if (create == NULL)
	    {
	      message_log (LOG_ERROR, "'%s' (%s) is not a valid doctype v.%d plugin.",
		doctype.c_str(), dt_fname.c_str(), (0xFFF & DoctypeDefVersion));
	      dlclose(handle);
	    }
	}
      else
	message_log (LOG_ERROR, "Error loading plugin %s: %s", dt_fname.c_str(), dlerror());
    }
  return create;
}

void DTREG::BuildPluginList (const STRING& dir, STRLIST& dtlist, STRLIST *Query) const
{
  if (!DirectoryExists(dir))
    return; // Nothing to do
#ifndef _MSDOS
  DIR *plugindir = opendir (dir.c_str());
  if (!plugindir) {
    message_log (LOG_ERRNO, "Can't open plug-in directory '%s'", dir.c_str());
    return;
  }
  struct dirent *ent;
  STRING dt_name, dt_fname, module, symbol;
  STRINGINDEX pos;
  while ((ent = readdir (plugindir)) != NULL) {
    struct stat buf;

    dt_name  = ent->d_name;
    if ((pos = dt_name.Search (PluginExtension)) == 0)
      continue;
    dt_name.EraseAfter (pos - 1).ToUpper();
    if (dtlist.Search(dt_name))
      continue; // Already in list
    (module = dt_name).ToLower(); // Modules must be named lowercase...
    // The name of the file to open
    dt_fname = AddTrailingSlash(dir) + module + PluginExtension;
    if (stat (dt_fname.c_str(), &buf) != -1 && S_ISREG(buf.st_mode)) {
      // Okay, it exists, and it's a file.  So try to dlopen() it.
#ifndef RTLD_DEFAULT
      HINSTANCE handle = dlopen (dt_fname.c_str(), RTLD_LAZY);
#else
      HINSTANCE handle = RTLD_DEFAULT;
      if (Creator(handle, module) == NULL)
	handle = dlopen (dt_fname.c_str(), RTLD_LAZY);
#endif
      if (handle)
	{
	  int plugin_version = PluginVersion(handle, module); 
	  if (plugin_version != DoctypeDefVersion)
	    {
	      if (plugin_version > 0)
		message_log (LOG_NOTICE, "File %s not comform to the current plug-in specification (%d!=%d).",
			dt_fname.c_str(), plugin_version, DoctypeDefVersion);
	      else
		message_log (LOG_DEBUG, "File %s does not comform to the current plug-in specification!",
			dt_fname.c_str());
	    }
	  else if (Creator(handle, module))
	    {
	      dtlist.AddEntry(dt_name);
	      if (Query)
		Query->AddEntry( Description(handle, module) );
	      message_log (LOG_DEBUG, "Adding DOCTYPE plugin '%s' (%s)", dt_name.c_str(), dt_fname.c_str()); 
	    }
	  else
	    message_log (LOG_DEBUG, "Can't load DOCTYPE plugin '%s' (%s): %s",
		dt_name.c_str(), dt_fname.c_str(), dlerror());
#ifdef RTLD_DEFAULT
	  if (handle != RTLD_DEFAULT)
#endif
	    dlclose(handle);
	}
      else
	message_log (LOG_ERROR, "Plugin %s failed: %s", dt_fname.c_str(), dlerror());
    }
  }
  closedir (plugindir);
#endif
}


DTREG::~DTREG()
{
#if 1
  if (DoctypesRegistry)
    delete DoctypesRegistry;

#else
  if (pluginsLoaded == 0 && DoctypesRegistry)
    delete DoctypesRegistry;
  else
    globalDoctypesRegistry = DoctypesRegistry;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Reading routines
///////////////////////////////////////////////////////////////////////////////////////////////////

size_t ReadIndirect(FILE *Fp, char *Buffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
 size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->ReadFile(Fp, Buffer, Start, Length);
    }
  else if (Length && Fp)
    {
      if (fseek(Fp, Start, SEEK_SET) != -1)
	n = fread(Buffer, sizeof(char), Length, Fp);
    }
  Buffer[n] = '\0';
  return n;
}

size_t ReadIndirect(const STRING& Filename, char *Buffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->ReadFile(Filename, Buffer, Start, Length);
    }
  else if (Length)
    {
      FILE *fp = fopen(Filename, "rb");
      if (fp)
        {
          if (fseek(fp, Start, SEEK_SET) == -1)
            message_log (LOG_ERRNO, "Seek Error in '%s' (%ld)!", Filename.c_str(), Start);
          else
            n = fread(Buffer, sizeof(char), Length, fp);
          fclose(fp);
        }
    }
  Buffer[n] = '\0';
  return n;
}

size_t ReadIndirect(FILE *Fp, STRING *StringBuffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->ReadFile(Fp, StringBuffer, Start, Length);
    }
  else if (Length && Fp)
    {
      n = StringBuffer->Fread(Fp, Length, Start);
    }
  return n;
}

size_t ReadIndirect(const STRING& Filename, STRING *StringBuffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  StringBuffer->Clear();
  if (Length)
    {
      FILE *fp = fopen(Filename, "rb");
      if (fp)
        {
	  n = ReadIndirect(fp, StringBuffer, Start, Length,DoctypePtr);
	  fclose(fp);
        }
    }
  return n;
}

size_t GetRecordData(FILE *Fp, char *Buffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
 size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->GetRecordData(Fp, Buffer, Start, Length);
    }
  else if (Length && Fp)
    {
      if (fseek(Fp, Start, SEEK_SET) != -1)
	n = fread(Buffer, sizeof(char), Length, Fp);
    }
  Buffer[n] = '\0';
  return n;
}

size_t GetRecordData(const STRING& Filename, char *Buffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->GetRecordData(Filename, Buffer, Start, Length);
    }
  else if (Length)
    {
      FILE *fp = fopen(Filename, "rb");
      if (fp)
        {
          if (fseek(fp, Start, SEEK_SET) == -1)
            message_log (LOG_ERRNO, "Seek Error in '%s' (%ld)!", Filename.c_str(), Start);
          else
            n = fread(Buffer, sizeof(char), Length, fp);
          fclose(fp);
        }
    }
  Buffer[n] = '\0';
  return n;
}

size_t GetRecordData(FILE *Fp, STRING *StringBuffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  if ( DoctypePtr )
    {
      n = DoctypePtr->GetRecordData(Fp, StringBuffer, Start, Length);
    }
  else if (Length && Fp)
    {
      n = StringBuffer->Fread(Fp, Length, Start);
    }
  return n;
}

size_t GetRecordData(const STRING& Filename, STRING *StringBuffer, off_t Start, size_t Length, const DOCTYPE *DoctypePtr)
{
  size_t n = 0;

  StringBuffer->Clear();
  if (Length)
    {
      FILE *fp = fopen(Filename, "rb");
      if (fp)
        {
	  n = GetRecordData(fp, StringBuffer, Start, Length,DoctypePtr);
	  fclose(fp);
        }
    }
  return n;
}




void PrintDoctypeList(ostream& os)
{
  DTREG dtreg ( (IDBOBJ *)NULL);
  dtreg.PrintDoctypeList(os);
  os << endl << endl;
}

void PrintDoctypeHelp(const STRING& Doctype, ostream& os)
{
  DTREG dtreg ( (IDBOBJ *)NULL);
  dtreg.PrintDoctypeHelp(Doctype, os);
  os << endl << endl;
}
