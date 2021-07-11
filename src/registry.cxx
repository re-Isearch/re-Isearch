#pragma ident  "@(#)registry.cxx  1.43 04/26/01 14:29:54 BSN"

/************************************************************************
************************************************************************/

/*-@@@
File:		registry.cxx
Version:	2.00
Description:	Class REGISTRY - Structured Profile Registry
@@@-*/

#include <fstream>
#include "common.hxx"
#include "registry.hxx"
#include "buffer.hxx"
#include <ctype.h>



static const CHR MagicSep[] = "\001";

#ifndef BUFSIZ
# define BUFSIZ  1024
#endif

static const size_t Maxline = BUFSIZ;
static const size_t Stdline = 256;

REGISTRY::REGISTRY (const STRING& Title)
{
  Data = Title;
  Next = 0;
  Child = 0;
}

REGISTRY::REGISTRY (const CHR* Title)
{
  Data = Title;
  Next = 0;
  Child = 0;
}

GDT_BOOLEAN REGISTRY::Empty() const
{
  return Next == 0 || Child == 0;
}

REGISTRY& REGISTRY::operator =(const REGISTRY& OtherRegistry)
{
  // Delete what I've got..
  if (Child) delete Child;
  if (Next)  delete Next;

  // Set new..
  Data = OtherRegistry.Data;
  Next = OtherRegistry.Next;
  Child = OtherRegistry.Child;
  return *this;
}

REGISTRY* REGISTRY::clone() const
{
  REGISTRY* r;
  try {
    r  = new REGISTRY(Data);
  } catch (...) {
    r = NULL;
  }
  if (r)
    {
      r->Next  = Next  ? Next->clone()  : 0;
      r->Child = Child ? Child->clone() : 0;
    }
  return r;
}

REGISTRY* REGISTRY::Add(REGISTRY *Reg)
{
  if (Reg)
    {
      REGISTRY *p  = Next;
      while (p->Next && p != this) p = p->Next;
      // Glue the end on
      p->Next = Reg->clone();
    }
  return this;
}

REGISTRY* REGISTRY::Add (REGISTRY *Reg1, REGISTRY *Reg2) const
{
  if (Reg2 && Reg1)
    {
      REGISTRY *r = Reg1->clone();
      REGISTRY *p  = r->Next;

      // Go to the end
      while (p->Next && p != r) p = p->Next;
      // Glue the end on
      p->Next = Reg2->clone();
      return r;
    }
  if (Reg1 == NULL && Reg2 == NULL)
    return NULL;
  if (Reg2) return Reg2->clone();
  return Reg1->clone();
}

#if 0

STRING MetaString(enum METAFORMATS mdType) const
{
  switch (mdType)
    {
      case mdHTML:  return HtmlMeta();
      case mdXML:  return   Sgml();
      case mdHTML_F: return Html();
      case mdTEXT:  return 
    }
  

}

STRING Xml() const
{
  STRING String;
  String Charset;

  GetGlobalCharset(&Charset);

  String.Cat ("<?XML VERSION=\"1.0\" ENCODING=\"");
  String.Cat (Charset);
  String.Cat ("\" ?>\n\<!DOCTYPE ");
  String.Cat (Data);
  String.Cat (" SYSTEM \"");
  String.Cat (Data);
  String.Cat (".dtd\" >\n");
  String.Cat (Sgml());
  return String;
}
#endif

void REGISTRY::PrintSgml(FILE* fp, const STRLIST& Position) const
{
  const REGISTRY* node = FindNode(Position);
  if (node)
    {
      for (REGISTRY* p = node->Child; p != NULL; p=p->Next)
	{
	  p->PrintSgml(fp);
        }
    }
}

static STRING EntityFix(const STRING& Input)
{
  STRING String;
  size_t Length = Input.GetLength();
  char tmp[32];
  UCHR ch;

  for (size_t i=1; i<= Length; i++)
    {
      switch (ch = Input.GetUChr(i))
	{
	  case '<': String.Cat("&lt;"); break;
	  case '>': String.Cat("&gt;"); break;
	  case '&': String.Cat("&amp;"); break;
	  default:
	    if (ch< 32 && !isspace(ch))
	      {
		sprintf(tmp, "&#%d;", ch);
		String.Cat(tmp);
	      }
	    else
	      String.Cat(ch);
	}
    }
  return String;
}

static STRING EntityDeFix(const char *Input)
{
  STRING String;
  for (const char *tp = Input; *tp; tp++)
    {
      if (*tp == '&')
	{
	  if (tp[1] == '#' && isdigit(tp[2]) && isdigit(tp[3]) && tp[4] == ';')
	    {
	      String.Cat ( (char)( tp[2]*10 + tp[3] ));
	      tp += 4;
	    }
	  else if (tp[1] == '#' && isdigit(tp[2]) && tp[3] == ';')
	    {
	      String.Cat ( tp[2] );
	      tp += 3;
	    }
	  else if (tp[1] == 'l' && tp[2] == 't' && tp[3] == ';')
	    {
	      String.Cat ('<');
	      tp += 3;
	    }
	  else if (tp[1] == 'g' && tp[2] == 't' && tp[3] == ';')
	   {
	      String.Cat ('>');
	      tp += 3;
	   }
	  else if (tp[1] == 'a' && tp[2] == 'm' && tp[3] == 'p' && tp[4] == ';')
	   {
	      String.Cat ('&');
	      tp += 4;
	   }
	  else
	   String.Cat ('&');
	}
      else
	String.Cat (*tp);
    }
  return String;
}


GDT_BOOLEAN REGISTRY::PrintSgml(const STRING& Filename) const
{
  FILE *fp = fopen(Filename, "wb");
  if (fp)
    {
       fprintf(fp, "<?xml version=\"1.0\"?>\n<!DOCTYPE INI SYSTEM \"ini.dtd\">\n");
       PrintSgml(fp);
       fclose(fp);
       return GDT_TRUE;
    }
  return GDT_FALSE;
}


void REGISTRY::PrintSgml(FILE* fp) const
{
  if (Child)
    {
      if (!Data.IsEmpty())
	{
	  fprintf(fp, "<");
	  Data.Print(fp);
	  fprintf(fp, ">");
	}
      Child->PrintSgml(fp);
      if (!Data.IsEmpty())
	{
	  fprintf(fp, "</");
	  Data.Print(fp);
	  fprintf(fp, ">\n");
	}
    }
  else
    {
      EntityFix(Data).Print(fp);
//    fprintf(fp, "\n");
    }
  if (Next)
    {
      Next->PrintSgml(fp);
    }
}

STRING REGISTRY::Sgml(const STRLIST& Position) const
{
  STRING String;
  const REGISTRY* node = FindNode(Position);
  if (node)
    {
      for (REGISTRY* p = node->Child; p != NULL; p=p->Next)
        {
          String.Cat ( p->Sgml() );
        }
    }
  return String;
}

STRING REGISTRY::Sgml() const
{
  STRING String;
  if (Child)
    {
      if (!Data.IsEmpty())
	String << "<" << Data << ">";
      String.Cat (Child->Sgml());
      if (!Data.IsEmpty())
	{
	  if (Data.Search(' '))
	    String << "</" << Data.Before(' ') << ">\n";
	  else
	    String << "</" << Data << ">\n";
	}
    }
  else
    {
      String.Cat( EntityFix(Data) );
    }
  if (Next)
    {
      String.Cat ( Next->Sgml() );
    }
  return String;
}

STRING REGISTRY::Html(const STRLIST& Position) const
{
  STRING String;
  const REGISTRY* node = FindNode(Position);
  if (node)
    {
      for (REGISTRY* p = node->Child; p != NULL; p=p->Next)
        {
          String.Cat ( p->Html() );
        }
    }
  return String;
}

STRING REGISTRY::Html() const
{
  STRING String;
  if (Child)
    {
      if (!Data.IsEmpty())
        String << "<DL><DT>" << Data << ":</DT>\n\t<DD>";
      String.Cat (Child->Html());
      if (!Data.IsEmpty())
        String << "</DD></DL>\n";
    }
  else
    {
      GlobalLocale.Charset().HtmlCat (Data, &String);
//      String.Cat("</DD>\n");
    }
  if (Next)
    {
      String.Cat ( Next->Html() );
    }
  return String;
}

// Print the HTML Meta Stuff
STRING REGISTRY::HtmlMeta(const STRLIST& Position) const
{
  STRING String;
  const REGISTRY* node = FindNode(Position);
  if (node)
    {
      String = node->HtmlMeta ();
    }
  return String;
}

STRING REGISTRY::HtmlMeta() const
{
  STRING String;
  for (REGISTRY* p = Child; p != NULL; p=p->Next)
    {
      String.Cat ( p->HtmlMeta(0, 0, "") );
    }
  return String;
}

#if 0

STRING REGISTRY::HtmlMeta(size_t level, size_t depth, const STRING& Tag) const
{
  STRING String;
  STRING tag (Tag);

//cerr << "Level = " << level << " depth=" << depth <<  " Tag = '" << Tag << "'" << endl;

  if (level == 0)
    String.Cat ("<META NAME=\"");
  if (Child)
    {
      if (!Data.IsEmpty())
	{
	  if (Child->Child)
	    {
	      // Extend the tag
	      if (tag.GetLength() && tag.GetChr(tag.GetLength()) != '.')
		tag.Cat (".");
	      tag.Cat (Data);
	    }
	  if (tag.GetLength() && depth)
	    String.Cat (tag);
	  if (Child->Child == 0)
	    String.Cat (Data);
	}
      String.Cat (Child->HtmlMeta(level+1, depth+1, tag));
    }
  else
    {
      char quote = Data.Search('"') == 0 ? '"' : '\'';
      STRING value = Data;

      if (quote == '\'')
	{
	  if (value.Search(quote))
	    {
	      quote = '"';
	      value.Replace("\"", "&quot;", GDT_TRUE);
	    }
	}
      String.Cat ("\" VALUE="); String.Cat (quote); String.Cat (value); String.Cat (quote); String.Cat (">\n");
    }
  if (Next)
    {
//cerr << "Now via next... tag=" << tag;
#if 1 /* EXPERIMENTAL 2008 */
      if (Data.GetLength())
	{
	   size_t pos = tag.SearchReverse(".");
	  if (pos) tag.EraseAfter(pos-1);
	}
//cerr << " --> tag=" << tag << endl;
#endif
      String.Cat (Next->HtmlMeta(0, depth+1, tag));
    }
//cerr << "Return " << String << endl;
  return String;
}
#else

STRING REGISTRY::HtmlMeta(size_t level, size_t depth, const STRING& Tag) const
{
  STRING String;
  STRING tag (Tag);

  if (level == 0)
    String.Cat ("<META NAME=\"");
  if (Child)
    {
      if (!Data.IsEmpty())
	{
	  if (Child->Child)
	    {
	      if (tag.GetLength() && tag.GetChr(tag.GetLength()) != '.')
		tag.Cat (".");
	      tag.Cat (Data);
	    }
	  if (tag.GetLength() && depth)
	    String.Cat (tag);
	  if (Child->Child == 0)
	    String.Cat (Data);
	}
      String.Cat (Child->HtmlMeta(level+1, depth+1, tag));
    }
  else
    {
      char quote = Data.Search('"') == 0 ? '"' : '\'';
      STRING value = Data;

      if (quote == '\'')
	{
	  if (value.Search(quote))
	    {
	      quote = '"';
	      value.Replace("\"", "&quot;", GDT_TRUE);
	    }
	}
      String.Cat ("\" VALUE="); String.Cat (quote); String.Cat (value); String.Cat (quote); String.Cat (">\n");
    }
  if (Next)
    {
      String.Cat (Next->HtmlMeta(0, depth+1, tag));
    }
  return String;
}
#endif

// SGML style
GDT_BOOLEAN REGISTRY::ReadFromSgml(const STRING& Filename)
{
  FILE *Fp = fopen(Filename, "r");
  size_t count = 0;
  if (Fp)
    {
      count = ReadFromSgml (Fp);
      fclose(Fp);
    }
  return count != 0;
}


GDT_BOOLEAN REGISTRY::ReadFromSgml(FILE *fp)
{
  if (fp)
    {
      DeleteChildren ();
      return AddFromSgml (fp) != 0;
    }
  return GDT_FALSE;
}

static char *trimWhitespace(char* s)
{
  // trim off end space
  char* p = s + strlen(s) - 1;
  while ( (p >= s) && (isspace(*p)) )
    p--;
  *(p+1) = '\0';
  // trim off beginning space
  p = s;
  while ( (*p != '\0') && (isspace(*p)) )
    p++;
// memmove(s, p, strlen(p) + 1);
  return p;
}

size_t REGISTRY::AddFromSgml(FILE *fp)
{
  STRLIST Position;
  return AddFromSgml(Position, fp);
}

size_t REGISTRY::AddFromSgml(const STRING& Root, FILE *fp)
{
  STRLIST Position;

  Position.AddEntry(Root);
  return AddFromSgml(Position, fp);
}

size_t REGISTRY::AddFromSgml(const STRLIST& Position, FILE *fp)
{
  size_t          count = 0;
  int             eof = 0;
  int             tokenReady = 0;
  int             tagging = 0;
  char            token[BUFSIZ];
  char           *tokenEnd;
  STRLIST         path = Position;
  STRLIST        *data = NULL;
  if (fp) {
    char            c;
    while (!eof) {
      // get token
      tagging = 0;
      tokenEnd = token;
      tokenReady = 0;
      while (!tokenReady) {
	c = fgetc(fp);
	if (c == EOF) {
	  tokenReady = 1;
	  eof = 1;
	  break;
	}
	if (tagging) {
	  if (c == '>') {
	    tokenReady = 1;
	  } else {
	    *(tokenEnd++) = c;
	  }
	} else {
	  if (c == '<') {
	    if (tokenEnd == token) {
	      tagging = 1;
	      *(tokenEnd++) = c;
	    } else {
	      ungetc(c, fp);
	      tokenReady = 1;
	    }
	  } else {
	    *(tokenEnd++) = c;
	  }
	}
      }
      *tokenEnd = '\0';
      if (token[0] == '<') {
	switch (token[1]) {
	case '/':
	  path.EraseAfter(path.GetTotalEntries() - 1);
	  break;
	case '?':
	case '!':
	  break;
	default:
	  if (strncasecmp(token+1, "xi:include", 10) == 0 ||
	      strncasecmp(token+1, "ib:include", 11) == 0 )
	    {
	      char *val = token+11;
	      char *tp = strchr(val, '=');
	      if (tp)
		{
		  char quote = '\"';
		  do { tp++; } while (isspace(*tp));
		  if (*tp == '\'')
		    {
		      quote = *tp;
		      tp++;
		    }
		  else if (*tp == '\"')
		    tp++;
		  else quote = '\0';
		  char *include = tp;
		  while (*tp && *tp != quote) tp++;
		  *tp = '\0';
		  STRING fn (include);
		  if (FileExists(fn)
#ifndef STANDALONE
			|| ResolveConfigPath(&fn)
#endif
				)
		    {
		      logf (LOG_DEBUG, "Registry: include \"%s\"", fn.c_str());
		      PREGISTRY Node = GetNode (path);
		      if (Node)
			return Node->ProfileAddFromFile (fn);
		    }
                  else
                    logf (LOG_WARN, "Registry: Included file '%s' not available.", include);
		}
	     else
		logf (LOG_WARN, "Registry: Incorrect include format: %s", token);
	    }
	  else
	    path.AddEntry(token + 1);
	}
      } else {
	if (path.GetTotalEntries() > 0) {
	  if (data == NULL) data = new STRLIST();
	  else data->Clear();
	  data->AddEntry( trimWhitespace(token) );
	  AddData(path, *data);
	  count++;
	}
      }
    } // while()
    if (data) delete data;
  }
  return count;
}


GDT_BOOLEAN REGISTRY::ReadFromSgmlBuffer(const STRING& Buffer)
{
  DeleteChildren ();
  return AddFromSgmlBuffer (Buffer) != 0;
}


size_t REGISTRY::AddFromSgmlBuffer(const STRING& Buffer)
{
  STRLIST Position;
  return AddFromSgmlBuffer(Position, Buffer);
}

size_t REGISTRY::AddFromSgmlBuffer(const STRING& Root, const STRING& Buffer)
{
  STRLIST Position;
 
  Position.AddEntry(Root);
  return AddFromSgmlBuffer(Position, Buffer);
}

size_t REGISTRY::AddFromSgmlBuffer(const STRLIST& Position, const STRING& Buffer)
{
  size_t          count = 0;
  int             eof = 0;
  int             tokenReady = 0;
  int             tagging = 0;
  char            token[BUFSIZ];
  char           *tokenEnd;
  STRLIST         path = Position;
  STRLIST        *data = NULL;
  size_t          position = 0;
  size_t          length = Buffer.GetLength();
  
  if (length)
    {
      char            c;
      while (position < length)
        {
          // get token
          tagging = 0;
          tokenEnd = token;
          tokenReady = 0;
          while (!tokenReady)
            {
              c = Buffer.GetChr(++position);
              if (position == length)
                {
                  tokenReady = 1;
                  eof = 1;
                  break;
                }
              if (tagging)
                {
                  if (c == '>')
                    {
                      tokenReady = 1;
                    }
                  else
                    {
                      *(tokenEnd++) = c;
                    }
                }
              else
                {
                  if (c == '<')
                    {
                      if (tokenEnd == token)
                        {
                          tagging = 1;
                          *(tokenEnd++) = c;
                        }
                      else
                        {
                          position--;
                          tokenReady = 1;
                        }
                    }
                  else
                    {
                      *(tokenEnd++) = c;
                    }
                }
            }
          *tokenEnd = '\0';
          if (token[0] == '<')
            {
              switch (token[1])
                {
                case '/':
                  path.EraseAfter(path.GetTotalEntries() - 1);
                  break;
                case '?':
                case '!':
                  break;
                default:
                  path.AddEntry(token + 1);
                }
            }
          else
            {
              if (path.GetTotalEntries() > 0)
                {
                  if (data == NULL) data = new STRLIST();
                  else data->Clear();
                  data->AddEntry( trimWhitespace(token) );
                  AddData(path, *data);
                  count++;
                }
            }
        } // while()
      if (data) delete data;
    }
  return count;
}


REGISTRY *parseMetaDefaults(const STRING& filename)
{
  REGISTRY       *metadef = new REGISTRY("meta");
  metadef->ReadFromSgml(filename);
  return metadef;
}

void REGISTRY::GetEntryList(const STRING& Section, STRLIST *Ptr, GDT_BOOLEAN Cat) const
{
  if (Ptr)
   {
      if (!Cat) Ptr->Clear();
      const REGISTRY *p = FindNode(Section);
      if (p)
	{
	  for (REGISTRY *pp = p->Child; pp; pp = pp->Next)
	    Ptr->AddEntry(pp->Data);
	}
   }
}


const REGISTRY *REGISTRY::FindNode (const STRING& Position) const
{
  // Search for a matching node (linear search):
  for (PREGISTRY p = Child; p; p = p->Next)
    {
      if (p->Data ^= Position)
	return p;
    }
  return NULL; // Not found
}

const REGISTRY *REGISTRY::FindNode (const STRLIST& Position) const
{
  const REGISTRY *TempNode = this;
  for (const STRLIST *p = Position.Next(); p != &Position; p = p->Next())
    {
      if ((TempNode = TempNode->FindNode ( p->Value() )) == NULL)
	return NULL;
    }
  return TempNode;
}

REGISTRY *REGISTRY::GetNode (const STRING& Position)
{
  PREGISTRY p = Child;
  if (p)
    {
      PREGISTRY op = 0;
      // Search for a matching node:
      while (p)
	{
	  if (p->Data ^= Position)
	    {
	      return p;
	    }
	  op = p;
	  p = p->Next;
	}
      // End up with op pointing to the last node.
      // Now create new node:
      p = new REGISTRY (Position);
      op->Next = p;
      return p;
    }
  else
    {
      // Create single node:
      p = new REGISTRY (Position);
      Child = p;
      return p;
    }
}

REGISTRY *REGISTRY::GetNode (const STRLIST& Position)
{
  PREGISTRY TempNode = this;
  for (const STRLIST *p = Position.Next(); p != &Position; p = p->Next() )
    {
      if ((TempNode = TempNode->GetNode (p->Value())) == NULL)
	break;
    }
  return TempNode;
}

void REGISTRY::SetData (const STRLIST& Value)
{
  DeleteChildren ();
  AddData (Value);
}

void REGISTRY::AddData (const STRLIST& Value)
{
  if (Value.IsEmpty())
    return;

  const STRLIST *ptr = Value.Next();
  PREGISTRY NewNode = new REGISTRY (ptr->Value());
  PREGISTRY p;
  if (Child)
    {
      for (p = Child; p->Next; p = p->Next)
	/* loop */;
      p->Next = NewNode;
    }
  else
    {
      Child = NewNode;
    }
  p = NewNode;
  // Now add the rest
  while ( (ptr = ptr->Next()) != &Value)
    {
      NewNode = new REGISTRY ( ptr->Value() );
      p->Next = NewNode;
      p = NewNode;
    }
}

void REGISTRY::SetData (const STRLIST& Position, const STRLIST& Value)
{
  PREGISTRY Node = GetNode (Position);
  Node->SetData (Value);
}

void REGISTRY::AddData (const STRLIST& Position, const STRLIST& Value)
{
  PREGISTRY Node = GetNode (Position);
  Node->AddData (Value);
}

size_t REGISTRY::GetData (PSTRLIST StrlistBuffer) const
{
  size_t i=0;
  StrlistBuffer->Clear ();
  for (PREGISTRY p = Child; p; p = p->Next)
    {
      StrlistBuffer->AddEntry (p->Data);
      i++;
    }
  return i;
}

size_t REGISTRY::GetData (const STRLIST& Position, PSTRLIST StrlistBuffer) const
{
  const REGISTRY *Node = FindNode (Position);
  size_t i = 0;
  if (Node)
    {
      i += Node->GetData (StrlistBuffer);
    }
  else
    {
      StrlistBuffer->Clear ();
    }
  return 0;
}

// Get String (main routine)
void REGISTRY::ProfileGetString (const STRING& Section, const STRING& Entry, PSTRLIST StrlistBuffer)
{
  STRLIST o (Section, Entry);
  const REGISTRY *Node = FindNode (o);
  if (Node)
    Node->GetData (StrlistBuffer);
  else
   StrlistBuffer->Clear();
}

// Get String
void REGISTRY::ProfileGetString (const STRING& Section, const STRING& Entry,
		  const STRING& Default, PSTRING StringBuffer)
{
  STRLIST Strlist;
  ProfileGetString (Section, Entry, &Strlist);

  if (Strlist.IsEmpty())
    *StringBuffer = Default;
  else
    Strlist.Join (MagicSep, StringBuffer);
}

// Get String list
void REGISTRY::ProfileGetString (const STRING& Section, const STRING& Entry,
	const STRING& Default, PSTRLIST StrlistBuffer)
{
  ProfileGetString (Section, Entry, StrlistBuffer);
  if (StrlistBuffer->IsEmpty() && Default.GetLength())
    StrlistBuffer->AddEntry(Default);
}


// Get Boolean Value
#ifdef G_BOOL
void REGISTRY::
ProfileGetString (const STRING& Section, const STRING& Entry,
		  const GDT_BOOLEAN Default, GDT_BOOLEAN *BoolBuffer)
{
  STRING StringBuffer;
  STRING DefaultString = Default;
  ProfileGetString (Section, Entry, DefaultString, &StringBuffer);
  *BoolBuffer = (StringBuffer == GDT_TRUE) ? GDT_TRUE : GDT_FALSE;
}
#endif

GDT_BOOLEAN REGISTRY::ProfileGetBoolean(const STRING& Section, const STRING& Entry)
{
  STRING StringBuffer;
  ProfileGetString (Section, Entry, NulString, &StringBuffer);
  return StringBuffer.GetBool();
}
int REGISTRY::ProfileGetInteger(const STRING& Section, const STRING& Entry)
{
  STRING StringBuffer;
  ProfileGetString (Section, Entry, NulString, &StringBuffer);
  return StringBuffer.GetInt();
}

NUMBER REGISTRY::ProfileGetNumber(const STRING& Section, const STRING& Entry)
{
  STRING StringBuffer;
  ProfileGetString (Section, Entry, NulString, &StringBuffer);
  return (NUMBER)StringBuffer;
}

STRING REGISTRY::ProfileGetString(const STRING& Section, const STRING& Entry)
{
  STRING StringBuffer;
  ProfileGetString (Section, Entry, NulString, &StringBuffer);
  return StringBuffer;
}


// Get "C" String
void REGISTRY::
ProfileGetString (const STRING& Section, const STRING& Entry,
		  const STRING& Default, PPCHR CStringBuffer)
{
  STRING StringBuffer;
  ProfileGetString (Section, Entry, Default, &StringBuffer);
  *CStringBuffer = StringBuffer.NewCString ();
}

// Get Integer
void REGISTRY::
ProfileGetString (const STRING& Section, const STRING& Entry,
		  const INT Default, INT *IntBuffer)
{
  STRING StringBuffer, s (Default);
  ProfileGetString (Section, Entry, s, &StringBuffer);
  if (IntBuffer) *IntBuffer = StringBuffer.GetInt();
}

void REGISTRY::ProfileGetString(const STRING& Section, const STRING& Entry,
	const UINT Default, UINT *IntBuffer)
{
  STRING StringBuffer, s (Default);
  ProfileGetString (Section, Entry, s, &StringBuffer);
  if (IntBuffer) *IntBuffer = StringBuffer.GetInt();
}

void REGISTRY::ProfileGetString(const STRING& Section, const STRING& Entry,
        const FLOAT Default, FLOAT *DoubleBuffer)
{
  STRING StringBuffer, s (Default);
  ProfileGetString (Section, Entry, s, &StringBuffer);
  if (DoubleBuffer) *DoubleBuffer = StringBuffer.GetDouble();
}

void REGISTRY::ProfileGetString(const STRING& Section, const STRING& Entry,
	const DOUBLE Default, DOUBLE *DoubleBuffer)
{
  STRING StringBuffer, s (Default);
  ProfileGetString (Section, Entry, s, &StringBuffer);
  if (DoubleBuffer) *DoubleBuffer = StringBuffer.GetDouble();
}



// Write String (main routine)
void REGISTRY::
ProfileWriteString(const STRING& Section, const STRING& Entry, 
		    const STRLIST& StringlistData)
{
  SetData (STRLIST(Section, Entry), StringlistData);
}

// Write String (main routine)
void REGISTRY::
ProfileWriteString (const STRING& Section, const STRING& Entry,
		    const STRING& StringData)
{
  STRLIST Value;
  if (!StringData.IsEmpty())
    Value.Split (MagicSep, StringData);
  ProfileWriteString(Section, Entry, Value);
}

// Write Boolean
#ifdef G_BOOL
void REGISTRY::
ProfileWriteString (const STRING& Section, const STRING& Entry,
		    const GDT_BOOLEAN BooleanData)
{
  ProfileWriteString (Section, Entry, STRING(BooleanData));
}
#endif

// Write "C" string
void REGISTRY::
ProfileWriteString (const STRING& Section, const STRING& Entry, const CHR *pData)
{
  ProfileWriteString (Section, Entry, STRING(pData));
}

// Write Integer
void REGISTRY::ProfileWriteString (const STRING& Section, const STRING& Entry,
  const INT iData)
{
  ProfileWriteString (Section, Entry, STRING(iData));
}

// Write Integer
void REGISTRY::ProfileWriteString (const STRING& Section, const STRING& Entry,
  const UINT iData)
{
  ProfileWriteString (Section, Entry, STRING(iData));
}

// Write Long Integer
void REGISTRY::ProfileWriteString (const STRING& Section, const STRING& Entry,
  const long iData)
{
  ProfileWriteString (Section, Entry, STRING(iData));
}

// Write Long Integer
void REGISTRY::ProfileWriteString (const STRING& Section, const STRING& Entry,
  const unsigned long iData)
{
  ProfileWriteString (Section, Entry, STRING(iData));
}


// Write Double 
void REGISTRY::ProfileWriteString (const STRING& Section, const STRING& Entry,
  const DOUBLE fData)
{
  ProfileWriteString (Section, Entry, STRING(fData));
}


// Recursive
void REGISTRY::Print (ostream& os, int Level) const
{
  for (int x = 0; x < Level; x++)
    {
      os << ' ';
    }
  { STRING NewData (Data);
    NewData.cEncode();
    os << '+' << NewData << endl;
  }
  for(PREGISTRY p = Child; p; p=p->Next)
    p->Print (os, Level + 1);
}

void REGISTRY::SaveToFile (const STRING& FileName, const STRLIST& Position)
{
#ifdef VMS
  ofstream Ofs;
  PCHR fileBuffer = new char[VMS_BUFSIZ]; 
  Ofs.setbuf(fileBuffer,VMS_BUFSIZ);
  Ofs.open((const CHR *)FileName);
#else
  ofstream Ofs ((const CHR *)FileName);
#endif
  if (Ofs.good())
    {
      const REGISTRY *Node = FindNode (Position);
      if (Node) Ofs << *Node;
    }
#ifdef VMS
  delete [] fileBuffer;
#endif
}


size_t REGISTRY::ProfileLoadFromFile (const STRING& FileName)
{
  DeleteChildren ();
  return ProfileAddFromFile (FileName);
}


size_t REGISTRY::ProfileLoadFromFile (const STRING& FileName, const STRLIST& Position)
{
  PREGISTRY Node = GetNode (Position);
  if (Node)
     {
	Node->DeleteChildren ();
	return Node->ProfileAddFromFile (FileName);
     }
  return 0;
}

size_t REGISTRY::ProfileAddFromFile (const STRING& FileName, const STRLIST& Position, int depth)
{
  PREGISTRY Node = GetNode (Position);
  return Node->ProfileAddFromFile (FileName, depth);
}

// Read Windows type profiles (.ini files)
size_t REGISTRY::ProfileAddFromFile (const STRING& FileName, int depth)
{
  size_t res = 0;
  FILE  *Fp = fopen(FileName, "r");
  if (Fp)
    {
//    char buf[Maxline];
//    setvbuf(Fp, buf, _IOFBF, sizeof(buf)/sizeof(char));
      res = ProfileAddFromFile(Fp, depth);
      fclose(Fp);
    }
  return res;
}

// Internal procedure
/*
static void RegSplit (PSTRLIST List, const STRING& TheString)
{
  CHR *buf = TheString.NewCString ();
  RegSplit (List, buf);
  delete[]buf;
}
*/

static void RegSplit (PSTRLIST List, char *tp)
{
  STRLIST NewList;
  char *tp2, *tp1;
  int quoted = 0;

  while(*tp)
    {
      while (isspace(*tp)) tp++;
      for (tp2 = tp; *tp2 && (*tp2 != ',' || quoted); tp2++)
	{
	  if (*tp2 == '\\') tp2++;
	  else if (*tp2 == '"') quoted = !quoted;
	}
      tp1 = tp2 - 1;
      if (*tp2) *tp2++ = '\0';
      while(tp1 > tp && isspace(*tp1))
	*tp1-- = '\0';
      if ((*tp == '"' || *tp == '\'') && *tp1 == *tp)
	{
	  tp++;
	  *tp1 = '\0';
	}
      if (*tp) NewList.AddEntry (tp);
      tp = tp2;
    }
  *List = NewList;
}

// Read Windows type profiles (.ini files)
// [Section]
// entry=value
// ; this is a comment line
// #include PATH <-- this add PATH
//
size_t REGISTRY::ProfileAddFromFile (FILE *Fp, int depth)
{
  size_t count = 0;
  if (Fp)
    {
      STRLIST Position, Value;
      char   *tcp;
      BUFFER  buffer;

      // Could use FGetMultiline but we won't.
      char *buf = (char *)buffer.Want(Maxline*4); // Start off with 4xMaxline
      while (fgets (buf, buffer.Size()-1, Fp) != NULL)
	{
	  size_t offset = 0;

	  if (count == 0 && buf[0] == '<')
	    {
	      // Sgml Style .ini
	      fseek(Fp, -strlen(buf), SEEK_CUR);
	      AddFromSgml(Fp);
	    }
	  // Grab lines, a trailing \ is a continuation..
	  for (;;)
	    {
	      size_t len = strlen(buf);
	      if (len >= buffer.Size()-2)
		logf (LOG_ERROR, "Line too long (%ld > %ld) in .ini file, truncated", len - offset, Maxline);
	      else if ((len - offset) >= Maxline-1)
		logf (LOG_WARN, "Line too long (%u > %u) in .ini file", (unsigned)(len - offset), (unsigned)Maxline);
	      else if ((len-offset) > Stdline)
		logf (LOG_INFO, "Long line (%u > %u) in .ini file", (unsigned)len, (unsigned)Stdline);

	      for (tcp = &buf[len-1]; isspace(*tcp) && tcp > buf; tcp--)
		/* loop */;
	      // Looking at last non-white character..
	      // Line continuation??
	      if (*tcp != '\\' || *(tcp+1) == ' ' || *(tcp+1) == '\t')
		{
		  // Nope..
		  *++tcp = '\0'; // Zap trailing white space
		  break;
		}
	      offset = tcp - buf;
	      // Get continuation..
	      buf = (char *)buffer.Expand(offset+Maxline+Stdline); // Expand buffer for the line
	      tcp = offset + buf; // Point to the same place again
	      // Read into..
	      if (fgets(tcp, buffer.Size() - offset -1, Fp) == NULL)
		break;
	    }
	  // Skip leading white space 
	  for (tcp = buf; *tcp == ' ' || *tcp == '\t'; tcp++)
	    /* loop */;
	  if (*tcp == '#')
	    {
	      static int MAX_DEPTH = 20;
	      // Skip any white space after '#'
	      do {
		tcp++;
	      } while (isspace(*tcp));
	      if (*tcp == '!')
		continue; // Shell magic (#!)
	      if (*tcp == '[')
		goto section; // Hack for shell scripts
	      if (strncmp(tcp, "include", 7) == 0)
		{
		  STRING FileName;

		  // We don't want to include too deep
		  if (++depth > MAX_DEPTH)
		    {
		      logf(LOG_ERROR, ".ini includes too deep (%d)", depth);
		      continue;
		    }
		  // Have an include
		  tcp += 7;
		  // Skip space
		  while (isspace(*tcp)) tcp++;
		  if (*tcp == '"' || *tcp == '<')
		    {
		      char end = (*tcp == '<' ? '>' : *tcp); 
		      char *tp = strchr(++tcp, end);
		      if (tp) *tp = '\0';
		      else continue; // Error
#ifdef STANDALONE
		      FileName = tcp;
#else
		      FileName = ((end == '>' && *tcp != '/') ? ResolveConfigPath(tcp) : STRING(tcp) );
#endif

		    } else FileName = tcp;
		  if (FileExists(FileName))
		    count += ProfileAddFromFile(FileName, depth);
		  else
		    logf (LOG_WARN, "Registry include file \"%s\" not found.", FileName.c_str());
		}
	      // else ignore line
	    }
	  else if (*tcp == '[')
	    {
section:
	      // Section
	      char *tp;
	      if ((tp = strchr(++tcp, ']')) != NULL)
		{
		  *tp = '\0';
		  Position.SetEntry (1, tcp); // Set Section name
		}
	    }
	  else if (*tcp != ';')
	    {
	      // Entry
	      char *value;
	      if ((value = strchr(tcp, '=')) != NULL)
		{
//		  const CHR sep = ','; // Use , for list separation
		  *value++ = '\0';
		  while (isspace(*value))
		    value++;
		  if (*value == '\0')
		    continue; // Nothing
		  Position.SetEntry(2, tcp); // Set Variable
		  Value.Clear ();
/*		  T = value; */
//		  T.cDecode (); // Decode C-Strings
//		  if (T.Search (sep)) {
		    RegSplit (&Value, value /*T*/);
//		  } else
//		    Value.AddEntry (T);
		  SetData (Position, Value);
		  count++;
		}
	    }
	}
    }
  return count;
}

void REGISTRY::ProfileWrite(ostream& os, const STRING&, const STRLIST&)
{
  for(REGISTRY *Node = Child; Node; Node=Node->Next)
    Node->ProfilePrint(os,0);
}

static inline GDT_BOOLEAN __iniLineNeedsQuotes(const STRING& Data)
{
  const STRINGINDEX x = Data.GetLength();
  if (x )
    {
      UCHR ch;
      if ((ch = Data.GetChr(x)) == '\\' || isspace(ch))
	return GDT_TRUE;
      if (Data.Search((char)','))
	return GDT_TRUE;
    }
  return GDT_FALSE;
}

#if 1
void REGISTRY::ProfilePrintBinary(FILE *fp, int Level) const
{
   fputc('B', fp); // Magic
   fputc(Level, fp); // Write Level
   Data.Write(fp);
   for(const REGISTRY *p = Child; p ; p = p->Next)
     p->ProfilePrintBinary(fp, Level + 1);
}

void REGISTRY::DumpBinary(const STRING& Filename) const
{
  // Binary Registry files need to start with !B\0 followed by a string
  FILE *fp = fopen(Filename, "wb");
  if (fp)
    {
      fputc('!', fp);
      for(REGISTRY *Node = Child; Node; Node=Node->Next)
         Node->ProfilePrintBinary(fp,0);
    }
}

#endif

// recursive
void REGISTRY::ProfilePrint(ostream& os, int Level) const
{
  if (Level==0) os << endl << '[';
  if (Level && __iniLineNeedsQuotes(Data))
    os << '"' << Data << '"';
  else
    os << Data;
  if (Level==0) os << ']';

  if (Level==1)                 os << '=';
  else if ((Level>1) && (Next)) os << ',';
  else                          os << endl;

  for(const REGISTRY *p = Child; p ; p = p->Next)
    p->ProfilePrint(os, Level + 1);
  if (Child == NULL)
    os << endl;
}

size_t REGISTRY::LoadFromFile (const STRING& FileName, const STRLIST& Position)
{
  PREGISTRY Node = GetNode (Position);
  return Node->LoadFromFile (FileName);
}

size_t REGISTRY::LoadFromFile (const STRING& FileName)
{
  DeleteChildren ();
  return AddFromFile (FileName);
}

size_t REGISTRY::AddFromFile (const STRING& FileName, const STRLIST& Position)
{
  PREGISTRY Node = GetNode (Position);
  return Node->AddFromFile (FileName);
}

size_t REGISTRY::AddFromFile (FILE *Fp)
{
  size_t count = 0;
  if (Fp)
    {
      STRLIST Position, Value;
      STRING NewData;
      STRINGINDEX x;

      char buf[4096];
      while (fgets (buf, sizeof(buf)/sizeof(char), Fp) != NULL)
	{
	  char *tcp;
	  if ((tcp = strchr(buf, '+')) != NULL)
	    {
	      for (char *p = tcp + strlen(tcp) -1; p>=tcp; p--)
		{
		  if (*p == '\n' || *p == '\r')
		    *p = 0;
		  else
		    break;
		}
	      if ((x = (tcp - buf)) <= Position.GetTotalEntries ())
		{
		  Position.EraseAfter (x);
		  NewData = ++tcp;
		  NewData.cDecode(); // Replace "\\n"-> "\n" etc
		  Value.SetEntry (1, NewData);
		  AddData (Position, Value);
		  Position.AddEntry (NewData);
		  count++;
		}
	    }
	}
    }
  return count;
}

size_t REGISTRY::AddFromFile (const STRING& FileName)
{
  size_t res = 0;
  FILE  *Fp = fopen(FileName, "r");
  if (Fp)
    res = AddFromFile(Fp);
  return res;
}

// recursive
void REGISTRY::ProfilePrint(FILE *Fp, int Level) const
{
  if (Level==0)
    {
      fputc('\n', Fp);
      fputc('[',  Fp);
    }
  if (!Data.IsEmpty())
    {
      if (Level &&  __iniLineNeedsQuotes(Data))
        {
	  fputc('"', Fp);
	  Data.Print(Fp, Stdline);
	  fputc('"', Fp);
	}
      else
	Data.Print (Fp, Stdline);
    }
  if (Level==0) fputc(']', Fp);
  fputc ( Level == 1 ? '=' : ((Level>1) && (Next) ? ',' : '\n'), Fp);

  for(PREGISTRY p = Child; p; p = p->Next)
    p->ProfilePrint(Fp, Level + 1);
  if (Level <=1 && Child == NULL)
    fputc('\n', Fp);
}


// can this be const?
ostream& operator <<(ostream& os, const REGISTRY& Registry)
{
  Registry.Print (os, 0);
  return os;
}


void REGISTRY::DeleteChildren ()
{
  if (Child)
    {
      delete Child;
      Child = 0;
    }
}

// WIN.INI format
GDT_BOOLEAN REGISTRY::Read(FILE *Fp)
{
  if (Fp)
    {
      DeleteChildren ();
      return ProfileAddFromFile (Fp) != 0;
    }
  return GDT_FALSE;
}

// WIN.INI format
void REGISTRY::Write(FILE *Fp) const
{
  extern const char *_IB_INI_MAGIC;

  if (Fp && _IB_INI_MAGIC)
    fputs(_IB_INI_MAGIC, Fp);
  for(REGISTRY *Node = Child; Node; Node=Node->Next)
    Node->ProfilePrint(Fp,0);
}


REGISTRY::~REGISTRY ()
{
  if (Child) delete Child;
  if (Next) delete  Next;
}

void Write(const REGISTRY& Registry, FILE *Fp)
{
  Registry.Write(Fp);
}

GDT_BOOLEAN Read(PREGISTRY RegistryPtr, FILE *Fp)
{
  return RegistryPtr->Read(Fp);
}
