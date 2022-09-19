#pragma ident  "@(#)para.cxx	1.7 05/08/01 21:49:11 BSN"
/*-
File:        ptext.cxx
Version:     1
Description: class PTEXT - index documents by paragraphs
Author:      Edward C. Zimmermann, edz@bsn.com
*/

#include <ctype.h>
#include "common.hxx"
#include "ptext.hxx"
#include "doc_conf.hxx"

#define DEBUG 0 
#define DEBUG_PARAGRAPH 0
#define DEBUG_SENTENCE  0
#define USE_HEADLINE_FIELD 1

#define CTL_PB 12

#define  MIN_PAGE_LENGTH(pos, PageStart)  (allowZeroLengthPages || ((pos - 4) > PageStart))

PTEXT::PTEXT(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
  ParseBody = GDT_TRUE;
  allowZeroLengthPages = GDT_FALSE;
  if (tagRegistry)
    {
      STRING S;
      tagRegistry->ProfileGetString("General", "ParseBody", "Y", &S);
      ParseBody = S.GetBool();
      tagRegistry->ProfileGetString("General", "ZeroLengthPages", "N", &S);
      allowZeroLengthPages = S.GetBool();
    }
  if (Db)
    {
      STRING S;
      Db->ProfileGetString(Doctype, "ParseBody", ParseBody ? "Y" : "N", &S);
      ParseBody = S.GetBool();
      Db->ProfileGetString(Doctype, "ZeroLengthPages", allowZeroLengthPages ? "Y" : "N", &S);
      allowZeroLengthPages = S.GetBool();

    }

  if (ParseBody == GDT_FALSE)
    {
      logf (LOG_INFO, "ParseBody of %s types is disabled", Doctype.c_str());
      return;
    }
  if (allowZeroLengthPages && Doctype.Search("PDF") == 0)
    logf (LOG_INFO, "Allow Zero Length Pages for %s", Doctype.c_str());
  initFields = GDT_FALSE;
  initAutoFields = GDT_FALSE;
}

GDT_BOOLEAN PTEXT::IsIgnoreMetaField(const STRING& FieldName) const
{
  STRING Name (FieldName);
  if (Name.ToUpper() == satzFieldName ||
     Name == paraFieldName || Name == lineFieldName || Name == pageFieldName ||
     Name == firstsatzFieldName || Name == firstparaFieldName  || Name == firstlineFieldName ||
     Name == paraPath || Name == satzPath ||  Name == linePath )
    return GDT_TRUE;
  return GDT_FALSE;
}

const char *PTEXT::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("PTEXT");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "\
Plaintext with \"Page\", \"Paragraph\", \"Sentence\" and \"Line\" fields as well\n\
as \"firstParagraph\", \"firstSentence\" and \"firstLine\" fields for the resp.\n\
first incidences of these fields.\n\
A \"Headline\" field is, by default, set to the longer of \"firstLine\" or\n\
\"firstSentence\". If \"Headline\" is set to empty (<ignore>) then no \"Headline\"\n\
field will be created during indexing.\n\
These options are defined in the Doctype.ini  [Fields] section.\n\n\
Additional DB.ini Options:\n\
[General]\n\
ParseBody=Y|N       # To parse the body for the above \"fields\"\n\
ZeroLengthPages=Y|N # Allow for empty pages to be stored (for page count)\n\
Headline=<Fieldname>\n";
}

void PTEXT::BeforeIndexing()
{
  initFields = GDT_FALSE;
  DOCTYPE::BeforeIndexing();
}

void PTEXT::AddFieldDefs()
{
  // DEFAULTS
  const char Page[]      = "Page";
  const char Line[]      = "Line";
  const char Paragraph[] = "Paragraph";
  const char Sentence[]  = "Sentence";

//cerr << "PTEXT::AddFieldDefs()" << endl;
  if (initFields) return;

  firstsatzFieldName  = UnifiedName("firstSentence").ToUpper();
  firstparaFieldName  = UnifiedName("firstParagraph").ToUpper();
  firstlineFieldName  = UnifiedName("firstLine").ToUpper();

  satzFieldName       = UnifiedName(Sentence).ToUpper();
  paraFieldName       = UnifiedName(Paragraph).ToUpper();
  lineFieldName       = UnifiedName(Line).ToUpper();
  pageFieldName       = UnifiedName(Page).ToUpper();

//cerr << "Sentence = " << satzFieldName << endl;
//cerr << "Line     = " << lineFieldName << endl;
//cerr << "Para     = " << paraFieldName << endl;
//cerr << "page     = " << pageFieldName << endl;

  if (paraFieldName == satzFieldName && !paraFieldName.IsEmpty())
    {
      logf (LOG_ERROR, "\
Should not map sentence and paragraph to the same name. Paragraph field disabled!");
      paraFieldName.Clear();
    }

  if (lineFieldName == paraFieldName && !paraFieldName.IsEmpty())
    {
      logf (LOG_ERROR, "\
Should not map line and paragraph to the same name. Paragraph field disabled!");
      paraFieldName.Clear();
    }
  if (lineFieldName == satzFieldName && !lineFieldName.IsEmpty())
    {
      logf (LOG_ERROR, "\
Should not map line and sentence to the same name, Sentence field disabled!");
      satzFieldName.Clear();
    }
  // Get the headlines
  STRING  headline      = UnifiedName("HEADLINE");
  headlineFieldName     = Getoption("Headline", headline);
  // Only firstXXX are allowed
  if (!headlineFieldName.IsEmpty() &&
        !(headlineFieldName.CaseEquals(headline) ||
        headlineFieldName.CaseEquals(firstsatzFieldName) ||
        headlineFieldName.CaseEquals(firstparaFieldName)))
    headlineFieldName = headline;

  // Define the field names for the paths
  STRING  page (pageFieldName);
  STRING  para (paraFieldName);

  if (page.IsEmpty()) page = Page;
  if (para.IsEmpty()) para = Paragraph;

  paraPath = UnifiedName(page + "\\" + para);

  // Define satzPath ONLY if we have a sentence field
  if (satzFieldName.IsEmpty())
    satzPath.Clear();
  else
    satzPath = UnifiedName(paraPath + "\\" + satzFieldName);

  // Define linePath ONLY if we have a line field
  if (lineFieldName.IsEmpty())
    linePath.Clear();
  else
   linePath = UnifiedName(paraPath + "\\" + lineFieldName);

  // Para path ONLY if paragraph defined
  if (paraFieldName.IsEmpty()) paraPath.Clear(); 

//cerr << "Para = " << paraPath << endl;
//cerr << "Satz = " << satzPath << endl;
//cerr << "Line = " << linePath << endl;

  // Done

  initFields = GDT_TRUE;
  initAutoFields = GDT_TRUE;
}

void PTEXT::InitFields()
{
//cerr << "PTEXT::InitFields()" << endl;
  PTEXT::AddFieldDefs();

//cerr << "ParseBody = " << (int)ParseBody << endl;
  if (Db && initFields && initAutoFields  && ParseBody)
    {
      DFD         dfd;
      int         count = 0;
      int         maxD = 0;

      dfd.SetFieldType (FIELDTYPE::text);

#define _f_init(_x) { if (!_x.IsEmpty()) { \
          dfd.SetFieldName (_x); SetMetaIgnore(_x); Db->DfdtAddEntry (dfd); count++; } ; maxD++; }

      _f_init(paraPath);
      _f_init(satzPath);
      _f_init(linePath);

      _f_init(satzFieldName);
      _f_init(paraFieldName);
      _f_init(lineFieldName);
      _f_init(pageFieldName);

      _f_init(firstsatzFieldName);
      _f_init(firstparaFieldName);
      _f_init(firstlineFieldName);

      _f_init(headlineFieldName);

//cerr << "satz = " << satzFieldName << " = " << satzPath << endl;
//cerr << "para = " << paraFieldName << " = " << paraPath << endl;
//cerr << "line = " << lineFieldName << " = " << linePath << endl;
//cerr << "page = " << pageFieldName << endl;

#undef _f_init

     initAutoFields = GDT_FALSE;
     if (count == 0) {
       logf (LOG_INFO, "%s: All %d textual body autotag fields disabled.",
		Doctype.c_str(), 6-count);
       ParseBody = GDT_FALSE;
     } else if (count == maxD)
       logf (LOG_INFO, "%s: All %d textual body autotag fields enabled", Doctype.c_str(), count);
     else 
       logf (LOG_INFO, "%s: %d textual body autotag fields enabled, %d disabled.",
		Doctype.c_str(), count, maxD-count);
    }
}



void PTEXT::AfterIndexing()
{
  initFields = GDT_FALSE;
  initAutoFields = GDT_FALSE;
  DOCTYPE::AfterIndexing();
}

void PTEXT::ParseRecords(const RECORD& FileRecord)
{
  DOCTYPE::ParseRecords (FileRecord);
}

#if DEBUG
static FILE *Debug_Fp = NULL;
#endif

//
//	Method name : ParseFields
//
//	Description : Parse to get the coordinates of the first sentence.
//	Input : Record Pointer
//	Output : None
///

//  A sentence is recognized as ending in ". ", "? ", "! ", ".\n" or "\n\n".
//
//
void PTEXT::ParseFields (PRECORD NewRecord)
{
  if (Db == NULL)
    {
      logf (LOG_PANIC, "Can't add to a NIL database");
      return;
    }
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      logf (LOG_ERRNO, "Can't open '%s'", fn.c_str());
      return;		// ERROR
    }

#if DEBUG
  Debug_Fp = fopen(fn, "rb");
#endif

  off_t start     = NewRecord->GetRecordStart();
  off_t end       = NewRecord->GetRecordEnd();

  DFT *pdft = ParseStructure(fp, fseek(fp, start, SEEK_SET), end > 0 ? end-start+1 : 0);
  if (pdft)
    {
      NewRecord->SetDft (*pdft);
      delete pdft;
    }

  Db->ffclose (fp);

#if DEBUG 
  fclose(Debug_Fp);
  Debug_Fp = NULL;
#endif
}


PDFT PTEXT::ParseStructure(FILE *fp, const off_t Position, const off_t Length)
{
  PDFT        pdft = new DFT();

  PTEXT::InitFields();

  if (!ParseBody) {
//cerr << "ParseBody off" << endl;
    return pdft; // Nothing to parse
  }
//cerr << "ParseBody" << endl;

  off_t PageStart = Position < 0 ? ftell(fp) : Position;
  off_t ParaStart = PageStart;
  off_t LineStart = ParaStart;
  off_t SatzStart = LineStart;

  unsigned lineCount = 0;
  unsigned satzCount = 0;
  unsigned paraCount = 0;
  unsigned pageCount = 0;
  const GDT_BOOLEAN have_headline_field = !(headlineFieldName.IsEmpty());

  int Ch;
  off_t pos = 0;
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan = 0, Start, Newline, Punct, LineBreak, ParaBreak, PageBreak} State = Scan;
#if DEBUG
  const char * const StateNames[] = {"Scan", "Start", "Newline", "Punct", "LineBreak", "ParaBreak", "PageBreak" };
#endif


while (pos < Length && Length != 0) {
  FC fc;
  DF df;

#if DEBUG
# define STATE_ANNOUNCE(_x) {\
  cerr << "****** POS=" << pos << "->" <<  StateNames[(int)(_x)] << "  Previous state was: " << StateNames[(int)State] << endl;} 

  STATE_ANNOUNCE(Scan);
#else
# define STATE_ANNOUNCE(_x)
#endif

  State = Scan;
  for (;;) {
    if (++pos > Length && Length != 0) break;
    Ch = getc(fp);

#if DEBUG
    cerr << "POS [" << pos << "] = " << (char)Ch << endl;
#endif

    // Kludge to handle MSDOS's \r\n for line characters
    if (Ch == '\r')
      {
	// \r is the MAC end-of-line so we need to be carefull!
        if ((Ch = getc(fp)) != '\n')
	  {
	    ungetc(Ch, fp); // pushback character
	    Ch = '\r';
	  }
	else
	  if (++pos > Length && Length != 0) break;; // increment count
      }

    if (State == Scan)
      {
#if DEBUG
        cerr << "State = Scan, pos=" << pos << "  Ch=" << (int)Ch << endl;
#endif
	if (isalnum(Ch))
	  {
	    STATE_ANNOUNCE(Start);
	    State = Start;
	  }
	SatzStart = pos-1;

	if (Ch == CTL_PB &&  MIN_PAGE_LENGTH(pos, PageStart))
	  {
#if DEBUG
	    cerr << "Write PAGE: " << PageStart << "-" << pos-1 << endl;
#endif
	    fc.SetFieldStart(PageStart);
	    fc.SetFieldEnd (pos-1);
	    PageStart = pos; // was + 1
	    df.SetFct (fc);
	    df.SetFieldName (pageFieldName);
	    pdft->AddEntry (df);
	    ++pageCount;
	  }
      }
    else if (Ch == '\r' || Ch == '\n' || Ch == EOF || Ch == CTL_PB)
      {
#if DEBUG
        cerr << "Parser found linebreak ch=" << (int)Ch <<  endl;
#endif
	// At least 2 characters wide
	if (pos-2 > LineStart)
	  {
	    if (State != Newline)
	      {
#if DEBUG
		cerr << "Write Line: " << LineStart << "-" << pos-1 << endl;
#endif
	        // We now have a line
	        fc.SetFieldStart(LineStart);
	        fc.SetFieldEnd (pos-1);
	        df.SetFct (fc);
	        df.SetFieldName (lineFieldName);
	        pdft->AddEntry (df);
		if (linePath.GetLength())
		  {
		    df.SetFieldName (linePath);
		    pdft->AddEntry (df);
		  }
	        if (++lineCount == 1)
		  {
		    df.SetFieldName(firstlineFieldName);
		    pdft->AddEntry(df);
#if USE_HEADLINE_FIELD 
		    if (satzCount > 0 && have_headline_field)
		      {
			df.SetFieldName(headlineFieldName);
			pdft->AddEntry(df);
		      }
#endif
		  }
	     }
	    LineStart = pos;
	  }
	if (Ch == EOF  || Ch == CTL_PB )
	  {
//cerr << "ParaBreak via PageBreak" << endl;
	    STATE_ANNOUNCE(PageBreak);
	    State = PageBreak;
	    break;
	  }
	else if (State == Newline || State == Punct)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    LineStart = pos;
	    State = ParaBreak;
	    break; // Done
	  }
	else
	  {
	    STATE_ANNOUNCE(Newline);
	    State = Newline;
	  }
      }
    else if (State == LineBreak)
      {
#if DEBUG
	cerr << "Parse was in linebreak modus" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	cerr << "This is a space so its a pagrapraph break" << endl;
#endif
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
      } 
    else if (State == Newline)
      {
	if (isalnum(Ch))
	  {
	    // See a character after the newline
#if DEBUG
	     cerr << "We were in newline mode and we now see an alpha-num character: " << (char)Ch << endl;
#endif
	     STATE_ANNOUNCE(State);
	     State = Start;
	  }
	else if (Ch == '\t' || Ch == '\v')
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
	else if (Ch == EOF  || Ch == CTL_PB )
	  {
	    STATE_ANNOUNCE(PageBreak);
	    State = PageBreak;
	    break;
	  }
	else if (Ch == '\f' || Ch == ' ')
	  {
#if DEBUG
	    cerr << "Newline modus sees: " << (int)Ch << endl;
#endif
	    State = LineBreak; // We might be going into a paragraph
	  }
	else if (isspace(Ch))
	  break;
      }
    else if (State == Punct)
      {
#if DEBUG
	cerr << "State is punctuation" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	    cerr << "And we have a space so break out of loop" << endl;
#endif
	    break; // Done
	  }
      }
#if 1
    else if (Ch == CTL_PB)
      {
	State = PageBreak;
	break;
      }
#endif
    else if (Ch == '.' || Ch == '!' || Ch == '?')
      {
	STATE_ANNOUNCE(Punct);
	State = Punct;
      }
  }		// for(;;)

  off_t p = pos;
  if (State == Newline) p--; // Leave off NL
  if (isspace(Ch)) p--; // Leave off trailing white space

  if (Length && SatzStart >= Length) break; // Nothing

 if (State == PageBreak && (p-2 > PageStart))
    {
      fc.SetFieldStart(PageStart);
      fc.SetFieldEnd (p-1);
      PageStart = pos;
      df.SetFct (fc);
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
      State = ParaBreak; // Its also a paragraph break
    }
  if ((State == ParaBreak || State == Newline) && (p-2 > ParaStart))
    {
#if DEBUG_PARAGRAPH
      STRING S;
      S.Fread(Debug_Fp, p-ParaStart, ParaStart);
      cerr << "Paragraph goes from: " << ParaStart << " - " << p -1 << endl;
      cerr << "<paragraph>" << S << "</paragraph>" << endl;
#endif
      fc.SetFieldStart(ParaStart);
      fc.SetFieldEnd (p-1);
      ParaStart = pos;
      df.SetFct (fc);
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);

      if (paraPath.GetLength())
	{
	  df.SetFieldName(paraPath);
	  pdft->AddEntry(df);
	}
      if (++paraCount == 1)
	{
	  df.SetFieldName (firstparaFieldName);
	  pdft->AddEntry (df);
	}
    }

  if (p-2 > SatzStart)
    {
#if DEBUG_SENTENCE
      STRING S;
      S.Fread(Debug_Fp, p-SatzStart, SatzStart);
      cerr << "Sentence goes from: " << SatzStart << " - " << p -1 << endl;
      cerr << "<sentence ID=\"" << satzCount+1 << "\">" << S << "</sentence>" << endl; 
#endif
      fc.SetFieldStart (SatzStart);
      fc.SetFieldEnd (p-1);
      df.SetFct (fc);
      df.SetFieldName (satzFieldName);
      pdft->AddEntry (df);
      if (satzPath.GetLength())
	{
	  df.SetFieldName (satzPath);
	  pdft->AddEntry (df);
	}
      if (++satzCount == 1)
        {
          df.SetFieldName (firstsatzFieldName);
          pdft->AddEntry (df);
#if USE_HEADLINE_FIELD 
	  if (lineCount > 0 && have_headline_field)
	    {
	      df.SetFieldName(headlineFieldName);
	      pdft->AddEntry(df);
	    }
#endif
        }
    }

  if (Ch == EOF)
    break; // Done
}

  if ((pos-2) > ParaStart)
    {
#if DEBUG
      cerr << "FINAL PARAGRAPH" << endl;
#endif
#if DEBUG_PARAGRAPH
      STRING S;
      S.Fread(Debug_Fp, pos-ParaStart, ParaStart);
      cerr << "Paragraph goes from: " << ParaStart << " - " << pos -1 << endl;
      cerr << "<paragraph>" << S << "</paragraph>" << endl;
#endif
      DF df;
      df.SetFct ( FC(ParaStart, pos));
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);
      if (paraPath.GetLength())
	{
	  df.SetFieldName(paraPath);
	  pdft->AddEntry (df);
	}
      paraCount++;
    }

  if ((pos - 2) > PageStart)
    {
#if DEBUG
     cerr << "Write FINAL page: " << PageStart << "-" << pos << endl;
#endif
      // Final page
      DF df;
      df.SetFct (FC(PageStart, pos));
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
    }


  logf (LOG_INFO, "%s: %u lines/%u sentences/%u paragraphs/%u pages.",
	Doctype.c_str(), lineCount, satzCount, paraCount, pageCount);

  return pdft;
}

void PTEXT::ParseStructure(DFT *pdft, const char * const mem, const off_t Position, const off_t Length)
{
  PTEXT::InitFields();

  if (!ParseBody)
    return; // Nothing to do

  off_t pos       = 0;

  off_t PageStart = 0;
  off_t ParaStart = 0;
  off_t LineStart = 0;
  off_t SatzStart = 0;

  unsigned  lineCount = 0;
  unsigned  satzCount = 0;
  unsigned  paraCount = 0;
  unsigned  pageCount = 0;

  char Ch = '\0';
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan = 0, Start, Newline, Punct, LineBreak, ParaBreak, PageBreak} State = Scan;
#if DEBUG
  const char * const StateNames[] = {"Scan", "Start", "Newline", "Punct", "LineBreak", "ParaBreak", "PageBreak" };
#endif

while (pos < Length && Length != 0) {
  FC fc;
  DF df;

#if DEBUG
# define STATE_ANNOUNCE(_x) {\
  cerr << "****** POS=" << pos << "->" <<  StateNames[(int)(_x)] << "  Previous state was: " << StateNames[(int)State] << endl;} 

  STATE_ANNOUNCE(Scan);
#else
# define STATE_ANNOUNCE(_x)
#endif

  State = Scan;


 for (;;) {
    if (pos > Length && Length != 0) break;

    Ch = mem[pos++];

#if DEBUG
    cerr << "POS [" << (pos-1) << "] = " << Ch << endl;
#endif
    // Kludge to handle MSDOS's \r\n for line characters
    if (Ch == '\r')
      {
        if (pos >= Length) break;
        // \r is the MAC end-of-line so we need to be carefull!
        if ((Ch = mem[pos]) != '\n')
          Ch = '\r';
        else
          if (++pos >= Length) break;
      }

    if (State == Scan)
      {
#if DEBUG
        cerr << "State = Scan, pos=" << pos << "  Ch=" << Ch << endl;
#endif
	if (isalnum(Ch))
	  {
	    STATE_ANNOUNCE(Start);
	    State = Start;
	  }
	SatzStart = pos-1;

	if (Ch == CTL_PB &&  MIN_PAGE_LENGTH(pos, PageStart))
	  {
#if DEBUG
	    cerr << "Write PAGE: " << PageStart << "-" << (pos-1) << endl;
#endif
	    fc.SetFieldStart(PageStart);
	    fc.SetFieldEnd (pos-1);
	    fc += Position;
	    PageStart = pos; // was + 1
	    df.SetFct (fc);
	    df.SetFieldName (pageFieldName);
	    pdft->AddEntry (df);
	    ++pageCount;
	  }
      }
    else if (Ch == '\r' || Ch == '\n' || Ch == CTL_PB)
      {
#if DEBUG
        cerr << "Parser found linebreak ch=" << Ch <<  endl;
#endif
	// At least 2 characters wide
	if (pos-2 > LineStart)
	  {
	    if (State != Newline)
	      {
#if DEBUG
		cerr << "Write Line: " << LineStart << "-" << pos-1 << endl;
#endif
	        // We now have a line
	        fc.SetFieldStart(LineStart);
	        fc.SetFieldEnd (pos-1);
		fc += Position;
	        df.SetFct (fc);
	        df.SetFieldName (lineFieldName);
	        pdft->AddEntry (df);
		if (linePath.GetLength())
		  {
		    df.SetFieldName (linePath);
		    pdft->AddEntry (df);
		  }
	        if (++lineCount == 1)
		  {
		    df.SetFieldName(firstlineFieldName);
		    pdft->AddEntry(df);
		  }
	     }
	    LineStart = pos;
	  }
	if (Ch == CTL_PB)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
	else if (State == Newline || State == Punct)
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    LineStart = pos;
	    State = ParaBreak;
	    break; // Done
	  }
	else
	  {
	    STATE_ANNOUNCE(Newline);
	    State = Newline;
	  }
      }
    else if (State == LineBreak)
      {
#if DEBUG
	cerr << "Parse was in linebreak modus" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	cerr << "This is a space so its a pagrapraph break" << endl;
#endif
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
      } 
    else if (State == Newline)
      {
	if (isalnum(Ch))
	  {
	    // See a character after the newline
#if DEBUG
	     cerr << "We were in newline mode and we now see an alpha-num character: " << (char)Ch << endl;
#endif
	     STATE_ANNOUNCE(State);
	     State = Start;
	  }
	else if (Ch == '\t' || Ch == '\v')
	  {
	    STATE_ANNOUNCE(ParaBreak);
	    State = ParaBreak;
	    break;
	  }
	else if (Ch == CTL_PB)
	  {
	    STATE_ANNOUNCE(PageBreak);
	    State = PageBreak;
	    break;
	  }
	else if (Ch == '\f' || Ch == ' ')
	  {
#if DEBUG
	    cerr << "Newline modus sees: " << (int)Ch << endl;
#endif
	    State = LineBreak; // We might be going into a paragraph
	  }
	else if (isspace(Ch))
	  break;
      }
    else if (State == Punct)
      {
#if DEBUG
	cerr << "State is punctuation" << endl;
#endif
	if (isspace(Ch))
	  {
#if DEBUG
	    cerr << "And we have a space so break out of loop" << endl;
#endif
	    break; // Done
	  }
      }
    else if (Ch == CTL_PB)
      {
	State = PageBreak;
	break;
      }
    else if (Ch == '.' || Ch == '!' || Ch == '?')
      {
	STATE_ANNOUNCE(Punct);
	State = Punct;
      }
  }		// for(;;)

  off_t p = pos;
  if (State == Newline) p--; // Leave off NL
  if (isspace(Ch)) p--; // Leave off trailing white space

  if (Length && SatzStart >= Length) break; // Nothing

 if (State == PageBreak && (p-2 > PageStart))
    {
      fc.SetFieldStart(PageStart);
      fc.SetFieldEnd (p-1);
      fc += Position;
      PageStart = pos;
      df.SetFct (fc);
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
      State = ParaBreak; // Its also a paragraph break
    }
  if ((State == ParaBreak || State == Newline) && (p-2 > ParaStart))
    {
#if DEBUG_PARAGRAPH
      cerr << "Paragraph goes from: " << ParaStart << " - " << p -1 << endl;
#endif
      fc.SetFieldStart(ParaStart);
      fc.SetFieldEnd (p-1);
      fc += Position;
      ParaStart = pos;
      df.SetFct (fc);
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);
      if (paraPath.GetLength())
	{
	  df.SetFieldName (paraPath);
	   pdft->AddEntry (df);
	}
      if (++paraCount == 1)
	{
	  df.SetFieldName (firstparaFieldName);
	  pdft->AddEntry (df);
	}
    }

  if (p-2 > SatzStart)
    {
#if DEBUG_SENTENCE
      STRING S;
      cerr << "Sentence goes from: " << SatzStart << " - " << p -1 << endl;
#endif
      fc.SetFieldStart (SatzStart);
      fc.SetFieldEnd (p-1);
      fc += Position;
      df.SetFct (fc);
      df.SetFieldName (satzFieldName);
      pdft->AddEntry (df);
      if (satzPath.GetLength())
	{
	  df.SetFieldName (satzPath);
	  pdft->AddEntry (df);
	}
      if (++satzCount == 1)
        {
          df.SetFieldName (firstsatzFieldName);
          pdft->AddEntry (df);
        }
    }
}

  if ((pos-2) > ParaStart)
    {
#if DEBUG
      cerr << "FINAL PARAGRAPH" << endl;
#endif
#if DEBUG_PARAGRAPH
      cerr << "Paragraph goes from: " << ParaStart << " - " << pos -1 << endl;
#endif
      DF df;
      df.SetFct ( FC(ParaStart, pos) + Position);
      df.SetFieldName (paraFieldName);
      pdft->AddEntry (df);
      if (paraPath.GetLength())
	{
	  df.SetFieldName (paraPath);
	  pdft->AddEntry (df);
	}
      paraCount++;
    }

  if ((pos - 2) > PageStart)
    {
#if DEBUG
     cerr << "Write FINAL page: " << PageStart << "-" << pos << endl;
#endif
      // Final page
      DF df;
      df.SetFct (FC(PageStart, pos) + Position);
      df.SetFieldName (pageFieldName);
      pdft->AddEntry (df);
      ++pageCount;
    }


  logf (LOG_INFO, "%s: %u lines/%u sentences/%u paragraphs/%u pages.",
	Doctype.c_str(), lineCount, satzCount, paraCount, pageCount);
}

void PTEXT::BeforeSearching (QUERY *Query)
{
  DOCTYPE::BeforeSearching(Query);
  if (headlineFieldName.IsEmpty())
    {
      if (! firstsatzFieldName.IsEmpty())
	headlineFieldName = firstsatzFieldName;
      else if (! firstlineFieldName.IsEmpty())
	headlineFieldName = firstlineFieldName;
      else
	headlineFieldName = BRIEF_MAGIC;
    }
}


void PTEXT::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
         const STRING& RecordSyntax, STRING *StringBufferPtr) const
{
  if (ElementSet == BRIEF_MAGIC)
    {
      DOCTYPE::Present (ResultRecord, headlineFieldName, RecordSyntax, StringBufferPtr);
      if (StringBufferPtr) StringBufferPtr->Trim();
    }
  else
    DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}

PTEXT::~PTEXT()
{
}
