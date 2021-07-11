/************************************************************************
************************************************************************/

/*@@@
File:		metadata.cxx
Version:	1.00
Description:	Class METADATA - Structured Metadata Registry
Author:		Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

#include "common.hxx"
#include "metadata.hxx"

#define MetaDataVersion "1.0"

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif


/*
<Locator>
<Title> </Title>
<Originator></Originator>
<Language-of-Resource></Language-of-Resource>
<Abstract>\n</Abstract>
<Availability>
<Available-Linkage>
<Linkage-Type>\ntext/HTML\n</Linkage-Type>\n\n");
   buffer->Cat("<Linkage>\n");
   // can we come up with the linkage to put here?
   buffer->Cat("</Linkage>\n");
   buffer->Cat("</Available-Linkage>\n");
   buffer->Cat("</Availability>\n\n");
   buffer->Cat("<Point-of-Contact>\n\n");
   buffer->Cat("<Name>\n");
   // can we get this from the author?
   buffer->Cat("</Name>\n\n");
   buffer->Cat("<Country>\n</Country>\n\n");
   buffer->Cat("<Network-Address>\n</Network-Address>\n");
   buffer->Cat("</Point-of-Contact>\n\n");
   buffer->Cat("<Control-Identifier>\n");
   // file name?
   buffer->Cat("</Control-Identifier>\n\n");
   buffer->Cat("<Record-Source>\n</Record-Source>\n\n");

   buffer->Cat("<Language-of-Record>\n</Language-of-Record>\n\n");
   buffer->Cat("<Date-of-Last-Modification>\n");
   // file date
   buffer->Cat("</Date-of-Last-Modification>\n");
   buffer->Cat("</Locator>\n");
*/

METADATA::METADATA()
{
  mdRegistry = NULL;
  Charset = GlobalLocale.Charset();
}

METADATA::METADATA(const STRING& MdType)
{
  mdRegistry = new REGISTRY(mdType);
  mdType = MdType;
  Charset = GlobalLocale.Charset();
  Load (MdType);
}

METADATA::METADATA(const STRING& MdType, const STRING& DefaultsPath)
{
  mdRegistry = new REGISTRY(MdType);
  mdType = MdType;
  Charset = GlobalLocale.Charset();
  Load (MdType, DefaultsPath);
}

METADATA& METADATA::operator =(const METADATA& Other)
{
  if (mdRegistry)
    delete mdRegistry;
  mdRegistry = Other.mdRegistry;
  mdType     = Other.mdType;
  Charset    = Other.Charset;
  return *this;
}

GDT_BOOLEAN METADATA::SetCharset(const CHARSET& NewCharset)
{
  if (NewCharset.Ok())
    {
      Charset = NewCharset;
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


GDT_BOOLEAN METADATA::Load(const STRING& Type)
{
  STRING temp (Type);

  temp.ToLower(); // Make lowercase
  temp.Cat(".xml");

  const STRING path = ResolveConfigPath(temp);
  if (FileExists(path))
    return Load(Type, path);
  return GDT_FALSE;
}

GDT_BOOLEAN METADATA::Load (const STRING& MdType, const STRING& DefaultsPath)
{
  if (mdRegistry && (MdType != MdType))
    {
      delete mdRegistry;
      mdRegistry = NULL;
    }
  if (mdRegistry == NULL)
    {
      mdRegistry = new REGISTRY(MdType);
      mdType = MdType;
    }
  if (Exists(DefaultsPath))
    {
      logf (LOG_DEBUG, "Loading Metadata from '%s'", (const char *)DefaultsPath);
      return mdRegistry->ReadFromSgml(DefaultsPath);
    }
  return GDT_FALSE;
}

/*
 <!DOCTYPE mydoc PUBLIC "-//MyCompany//My product//EN" 
                        "http://my.company.com/myproduct.dtd">

Your application would recognise the id "-//MyCompany//My product//EN"
and supply a built-in DTD instead of fetching the URL.
*/

static const char fmt[] = "\
<!-- (*-XML-*) IB Metadata Record v.%s -->\n\
<?XML VERSION=\"1.0\" ENCODING=\"%s\" standalone='yes'?>\n<!DOCTYPE %s SYSTEM \"%s.dtd\" >\n";

#define HEADER	fmt, MetaDataVersion, \
        strncmp((const char *)Charset, "us", 2) == 0 ? "utf-8" : (const char *)Charset, \
	(const char *)(mdRegistry->Child->Data), (const char *)mdType

void METADATA::Print(const STRING& Filename) const
{
  FILE *fp = fopen(Filename, "w");
  if (fp)
    {
      Print(fp);
      fclose(fp);
    }
}

void METADATA::Print(const STRING& Filename, const STRLIST& Position) const
{
  FILE *fp = fopen(Filename, "w");
  if (fp)
    {
      Print(fp, Position);
      fclose(fp);
    }
}

void METADATA::Append(const STRING& Filename) const
{
  GDT_BOOLEAN header = FileExists(Filename);
  if (header == GDT_FALSE)
    {
      Print(Filename);
    }
  else
    {
      FILE *fp = fopen(Filename, "a");
      if (fp)
	{
	  mdRegistry->PrintSgml(fp);
	  fclose(fp);
	}
    }
}

void METADATA::Append(const STRING& Filename, const STRLIST& Position) const
{
  GDT_BOOLEAN header = FileExists(Filename);
  if (header == GDT_FALSE)
    {
      Print(Filename);
    }
  else
    {
      FILE *fp = fopen(Filename, "a");
      if (fp)
        {
          mdRegistry->PrintSgml(fp, Position);
          fclose(fp);
        }
    }
}


void METADATA::Append(FILE *Fp) const
{
  mdRegistry->PrintSgml(Fp);
}

void METADATA::Append(FILE *Fp, const STRLIST& Position) const
{
  mdRegistry->PrintSgml(Fp, Position);
}

void METADATA::Print(FILE *Fp) const
{
  fprintf(Fp, HEADER);
  mdRegistry->PrintSgml(Fp);
}

void METADATA::Print(FILE *Fp, const STRLIST& Position) const
{
  fprintf(Fp, HEADER);
  mdRegistry->PrintSgml(Fp, Position);
}


METADATA:: operator STRING() const
{
  STRING String;

  String.form(HEADER);
  String.Cat ( mdRegistry->Sgml() );
  return String;
}

#undef HEADER


METADATA *METADATA::clone() const
{
  METADATA *m = new METADATA();
  m->mdRegistry = mdRegistry->clone();
  m->mdType = mdType;
  return m;
}

// Get a plain text (SUTRS) version of the metadata
STRING METADATA::Text() const
{
  STRING String;
  for (const REGISTRY* p = mdRegistry->Child; p != NULL; p=p->Next)
    {
      String.Cat ( Text(p) );
    }
  return String;
}

STRING METADATA::Text(const STRLIST& Position) const
{
  STRING String;
  const REGISTRY *node = mdRegistry->FindNode(Position);

  if (node && node->Child)
    {
      String.Cat ( Text(node->Child) );
    }
  return String;
}


// Private routine
STRING METADATA::Text(const REGISTRY* r, STRSTACK *Stack) const
{
  STRING String, Value;
  GDT_BOOLEAN Alloc = GDT_FALSE;
  if (Stack == NULL)
    {
      Stack = new STRSTACK();
      Alloc = GDT_TRUE;
    }
  if (r->Child)
    {
      if (!r->Data.IsEmpty())
	Stack->Push(r->Data);
      String.Cat (Text(r->Child, Stack));
      if (!r->Data.IsEmpty())
        Stack->Pop(&Value);
    }
  else
    {
      String.Cat (Stack->Join(":"));
      String.Cat("=\t");
      String.Cat( r->Data );
      String.Cat("\n");

    }
  if (r->Next)
    {
      String.Cat ( Text(r->Next, Stack) );
    }
  if (Alloc) delete Stack;
  return String;
}

// Get a SGML/XML version of the metadata
STRING METADATA::Xml() const
{
  return mdRegistry->Sgml();
}


STRING METADATA::Xml(const STRLIST& Position) const
{
  return mdRegistry->Sgml(Position);
}



// Get a plain text HTML version of the metadata
STRING METADATA::Html() const
{
  return mdRegistry->Html();
}

STRING METADATA::Html(const STRLIST& Position) const
{
  return mdRegistry->Html(Position);
}


// Print the HTML Meta Stuff
STRING METADATA::HtmlMeta() const
{
  return HtmlMeta(mdRegistry);
}

STRING METADATA::HtmlMeta(const STRLIST& Position) const
{
  STRING String;
  const REGISTRY* node = mdRegistry->FindNode(Position);
  if (node)
    {
      String = HtmlMeta (node);
    }
  return String;
}

STRING METADATA::HtmlMeta(const REGISTRY *r) const
{
  STRING      String;
  const char *classnam = (const char *)(mdRegistry->Data);

  if (classnam && *classnam) String.form("<META CLASS=\"%s\">\n", classnam);
  for (REGISTRY* p = r->Child; p != NULL; p=p->Next)
    {
      String.Cat ( HtmlMeta(p, 0, 0, NulString) );
    }
  if (classnam && *classnam) String.Cat("</META>\n");
  return String;
}

// Private
STRING METADATA::HtmlMeta(REGISTRY *r, size_t level, size_t depth, const STRING& Tag) const
{
  STRING String;
  STRING tag (Tag);

  if (level == 0)
    String.Cat ("<META NAME=\"");
  if (r->Child)
    {
      if (tag.GetLength() && tag.GetChr(tag.GetLength()) != '.')
	{
	  tag.Cat (".");
	}
      if (r->Child->Child)
	{
	  tag.Cat (r->Data);
	}
      if (tag.GetLength() && depth)
	{
	  String.Cat (tag);
	}
      if (r->Child->Child == 0)
	{
	  String.Cat (r->Data);
	}
      String.Cat (HtmlMeta(r->Child, level+1, depth+1, tag));
    }
  else
    {
      STRING value = r->Data;
      char quote = value.Search('"') == 0 ? '"' : '\'';

      if (quote == '\'')
	{
	  if (value.Search(quote))
	    {
	      quote = '"';
	      value.Replace("\"", "&quot;", GDT_TRUE);
	    }
	}
      String.Cat ("\" VALUE="); String.Cat (quote); String.Cat (value); String.Cat (quote); String.Cat (" />\n");
    }
  if (r->Next)
    {
      String.Cat (HtmlMeta(r->Next, 0, depth+1, tag));
    }
  return String;
}

void METADATA::SetData(const STRLIST& Position, const STRLIST& Value)
{
  mdRegistry->SetData(Position, Value);
}


void METADATA::AddData(const STRLIST& Position, const STRLIST& Value)
{
  mdRegistry->AddData(Position, Value);
}

size_t METADATA::GetData(const STRLIST& Position, PSTRLIST StrlistBuffer) const
{
  return mdRegistry->GetData(Position, StrlistBuffer);
}

void METADATA::Clear()
{
  mdRegistry->DeleteChildren ();
}

void METADATA::Clear(const STRLIST& Position)
{
  REGISTRY* node = (REGISTRY *)mdRegistry->FindNode(Position);
  node->DeleteChildren();
}

void METADATA::Write(PFILE Fp) const
{
  Print(Fp);
}

GDT_BOOLEAN METADATA::Read(FILE *Fp)
{
  return mdRegistry->ReadFromSgml(Fp);
}

GDT_BOOLEAN METADATA::Add(FILE *Fp)
{
  return mdRegistry->AddFromSgml(Fp);
}




METADATA::~METADATA()
{
  delete mdRegistry;
}


void Write(const METADATA& Registry, PFILE Fp)
{
  Registry.Write(Fp);
}

GDT_BOOLEAN Read(PMETADATA RegistryPtr, PFILE Fp)
{
  return RegistryPtr->Read(Fp);
}

LOCATOR::LOCATOR()
{
  Metadata = new METADATA ("GILS");
}

LOCATOR::LOCATOR(const STRING& Path)
{
  STRING path (Path);
  struct stat sb;
  if ((stat ((const char *)Path, &sb) >= 0) && ((sb.st_mode & S_IFMT) == S_IFDIR))
    {
      path.Cat("about.xml");
    }
  logf (LOG_DEBUG, "Create METADATA(\"GILS\", \"%s\")", (const char *)path);
  Metadata = new METADATA("GILS", path);
}

LOCATOR::LOCATOR(const METADATA& DefaultMetadata)
{
  Metadata = DefaultMetadata.clone();
}

LOCATOR::LOCATOR(const LOCATOR& OtherLocator)
{
  Metadata = OtherLocator.Metadata->clone();
}


void LOCATOR::Set(const STRING& Name, const STRING& Value)
{
  STRLIST position, value;

  position.AddEntry("locator");
  position.AddEntry(Name);
  value.AddEntry(Value);
  Metadata->SetData(position, value);
}

void LOCATOR::Set(const STRLIST& Position, const STRING& Value)
{
  STRLIST value;

  value.AddEntry(Value);
  Metadata->SetData(Position, value);
}

void LOCATOR::Set(const STRLIST& Position, const STRLIST& Value)
{
  Metadata->SetData(Position, Value);
}

void LOCATOR::Add(const STRING& Name, const STRING& Value)
{
  STRLIST position, value;
  position.AddEntry("locator");
  position.AddEntry(Name);
  value.AddEntry(Value);
  Metadata->AddData(position, value);
}

void LOCATOR::Add(const STRLIST& Position, const STRING& Value)
{
  STRLIST value;

  value.AddEntry(Value);
  Metadata->AddData(Position, value);
}

void LOCATOR::Add(const STRLIST& Position, const STRLIST& Value)
{
  Metadata->AddData(Position, Value);
}


GDT_BOOLEAN LOCATOR::IsEmpty(const STRING& Name) const
{
  STRLIST Position;
  Position.AddEntry ("locator");
  Position.AddEntry (Name);
  return Metadata->Metadata()->FindNode (Position) == NULL;
}

STRING LOCATOR::Get(const STRING& Name) const
{
  STRING result;
  STRLIST list;

  if (Get (Name, &list))
    {
      list.GetEntry(1, &result);
    }
  return result;
}

STRING LOCATOR::Get(const STRLIST& Position) const
{
  STRING result;
  STRLIST list;

  if (Get (Position, &list))
    {
      list.GetEntry(1, &result);
    }
  return result;
}




size_t LOCATOR::Get(const STRING& Name, PSTRLIST StrlistPtr) const
{
  STRLIST Position;
  Position.AddEntry ("locator");
  Position.AddEntry (Name);

  return Metadata->Metadata()->GetData (Position, StrlistPtr);
}


size_t LOCATOR::Get(const STRLIST& Position, PSTRLIST StrlistPtr) const
{
  return Metadata->Metadata()->GetData (Position, StrlistPtr);
}


LOCATOR::~LOCATOR()
{
  if (Metadata)
    delete Metadata;
}


static const CHR *RegistrationTitle      = "Isearch";
// Profile main section
static const CHR DbInfoSection[]         = "DbInfo";
// Indexer control Entries
static const CHR VersionEntry[]          = "VersionNumber";
// DB specified Entries
static const CHR DocTypeEntry[]          = "DocType";
static const CHR DocTypesEntry[]         = "DocTypes";
static const CHR TitleEntry[]            = "Title";
static const CHR CharsetEntry[]          = "Charset";
static const CHR StoplistEntry[]         = "Stoplist";
static const CHR DateEntry[]             = "DateCreated";
static const CHR CommentsEntry[]         = "Comments";
static const CHR CopyrightEntry[]        = "Copyright";
static const CHR DateModifiedEntry[]     = "DateLastModified";
static const CHR MaintainerNameEntry[]   = "Maintainer.Name";
static const CHR MaintainerMailEntry[]   = "Maintainer.Email";
static const CHR DatabasesEntry[]        = "Databases";


DBINFO::DBINFO(const PIDBOBJ DbParent)
{
  init(DbParent, NULL);
}

DBINFO::DBINFO(const PIDBOBJ DbParent, PSTRLIST DocTypeOptions)
{
  init(DbParent, DocTypeOptions);
}

void DBINFO::init(const PIDBOBJ DbParent, PSTRLIST DocTypeOptions)
{
  Parent = DbParent;
  DbInfoChanged = GDT_FALSE;
  STRING DbInfoFn = Parent->ComposeDbFn (DbExtDbInfo);
  MainRegistry = new METADATA (RegistrationTitle);
  // Add .ini options
  MainRegistry->Metadata()->ProfileAddFromFile(DbInfoFn);
  // Add .ini options
  if (DocTypeOptions)
    GetOptions(DocTypeOptions);

#ifndef _WIN32
  STRING Name, Address;
  if ( _IB_GetmyMail (&Name, &Address) )
    {
      if (GetMaintainerName().IsEmpty())
	SetMaintainerName(Name);
      if (GetMaintainerEmail().IsEmpty())
	SetMaintainerEmail(Address);
    }
#endif
  GlobalDoctype.Set( GetValue(DocTypeEntry) );
  SetValue("Methodology", "Automatically Generated by IB/re-iSearch");
}

void DBINFO::GetOptions(PSTRLIST DocTypeOptions) const
{
  STRING Entry, Value;
  for (int i=1; i<256; i++)
    {
      Entry.form("Option[%d]", i);
      MainRegistry->Metadata()->ProfileGetString(DbInfoSection, Entry, NulString, &Value);
      if (Value.GetLength() == 0)
	break;
      DocTypeOptions->AddEntry (Value);
  }
}

void DBINFO::SetValue(const STRING& Entry, const STRING& Value)
{
  MainRegistry->Metadata()->ProfileWriteString(DbInfoSection, Entry, Value);
}

void DBINFO::SetValue(const STRING& Entry, const STRLIST& Value)
{
  MainRegistry->Metadata()->ProfileWriteString(DbInfoSection, Entry, Value);
}


STRING DBINFO::GetValue(const STRING& Entry) const
{
  STRING Value;
  MainRegistry->Metadata()->ProfileGetString(DbInfoSection, Entry, NulString, &Value);
  return Value;
}


STRING DBINFO::GetValue(const STRING& Entry, const STRING& Default) const
{
  STRING Value;
  MainRegistry->Metadata()->ProfileGetString(DbInfoSection, Entry, Default, &Value);
  return Value;
}

#if 0
STRING DBINFO:GetHTDocumentRoot() const
{
  STRING Value;
  char *tp = getenv("WWW_ROOT");
  if (tp == NULL) tp = getenv("HTTP_PATH");
  if (tp == NULL) tp = getenv("HTDOCS");
  if (tp) Value = tp;
  // Now look in .ini
  else MainRegistry->Metadata()->ProfileGetString("HTTP", "Pages", NulString, &Value);
  return Value;
}

void DBINFO::SetHTDocumentRoot(const STRING& Value)
{
  MainRegistry->Metadata()->ProfileWriteString("HTTP", "Pages", Value);
}

#endif

STRING DBINFO::GetTitle() const
{
  return GetValue(TitleEntry, RemovePath( Parent->GetDbFileStem() ));
}

void DBINFO::SetTitle(const STRING& Value)
{
  SetValue(TitleEntry, Value);
}

void DBINFO::SetGlobalDoctype(const DOCTYPE_ID& Doctype)
{
  GlobalDoctype = Doctype;
  SetValue(DocTypeEntry, GlobalDoctype.Get());
}

DOCTYPE_ID DBINFO::GetGlobalDoctype() const 
{
  return GlobalDoctype;
}

void DBINFO::SetGlobalCharset(const STRING& Value)
{
  SetValue(CharsetEntry, Value);
}

STRING DBINFO::GetGlobalCharset() const
{
  STRING Charset ( GetValue(CharsetEntry) );
  if (Charset.IsEmpty())
    ::GetGlobalCharset (&Charset);
  return Charset;
}

STRING DBINFO::GetGlobalStoplist() const
{
  return GetValue(StoplistEntry);
}

void DBINFO::SetGlobalStoplist(const STRING& Value)
{
  SetValue(StoplistEntry, Value);
}

STRING DBINFO::GetDateCreated() const
{
  STRING Value (GetValue(DateEntry));

  if (Value.IsEmpty())
    return GetDateLastModified();
  return Value;
}

void DBINFO::SetDateCreated(const STRING& Value)
{
  if (Value.IsEmpty())
    SetValue(DateEntry, ISOdate(0));
  else
    SetValue(DateEntry, Value);
}


STRING DBINFO::GetDateLastModified() const
{
  return GetValue(DateModifiedEntry, "Now");
}

void DBINFO::SetDateLastModified(const STRING& Value)
{
  SetValue(DateModifiedEntry, Value);
}

STRING DBINFO::GetComments() const
{
  return GetValue(CommentsEntry);
}

void DBINFO::SetComments(const STRING& Value)
{
  SetValue(CommentsEntry, Value);
}

STRING DBINFO::GetRights() const
{
  return GetValue(CopyrightEntry);
}

void DBINFO::SetRights(const STRING& Value)
{
  SetValue(CopyrightEntry, Value);
}


STRING DBINFO::GetMaintainerName() const
{
  return GetValue(MaintainerNameEntry);
}

void DBINFO::SetMaintainerName(const STRING& Value)
{
  SetValue(MaintainerNameEntry, Value);
}


STRING DBINFO::GetMaintainerEmail() const
{
  return GetValue(MaintainerMailEntry);
}

void DBINFO::SetMaintainerEmail(const STRING& Value)
{
  SetValue(MaintainerMailEntry, Value);
}


GDT_BOOLEAN DBINFO::GetDatabaseList(PSTRLIST FilenameList) const
{
  MainRegistry->Metadata()->ProfileGetString(DbInfoSection, DatabasesEntry, FilenameList);
  return !FilenameList->IsEmpty();
}


DBINFO::~DBINFO()
{
  if (MainRegistry) delete MainRegistry;
}
