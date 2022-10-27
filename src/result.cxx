/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)result.cxx"

/************************************************************************
************************************************************************/

/*@@@
File:		result.cxx
Description:	Class RESULT - Search Result
@@@*/

#include <ctype.h>
#include "common.hxx"
#include "result.hxx"
#include "magic.hxx"
#include "lang-codes.hxx"
#include "dtreg.hxx"


#ifdef DEBUG_MEMORY
long __IB_RESULT_allocated_count = 0; // Used to track stray RESULTs
#endif

static RESULT _nulresult;

const RESULT& NulResult = _nulresult;

RESULT::RESULT()
{
  Index       = 0;
  ExtIndex    = 0;
  Score       = 0;
  AuxCount    = 0;
  Category    = 0;
  RecordStart = 0;
  RecordEnd   = 0;
#ifdef DEBUG_MEMORY
  __IB_RESULT_allocated_count++;
#endif
}

RESULT::RESULT(const MDTREC& mdtrec)
{
  Index       = 0;
  ExtIndex    = 0;
  Score       = 0;
  AuxCount    = 0;
  Key         = mdtrec.GetKey() ;
  DocumentType= mdtrec.GetDocumentType();
  Locale      = mdtrec.GetLocale();
  Pathname    = mdtrec.GetPathname();
  origPathname= mdtrec.GetOrigPathname();
  RecordStart = mdtrec.GetLocalRecordStart();
  RecordEnd   = mdtrec.GetLocalRecordEnd();
  Date        = mdtrec.GetDate();
  DateModified= mdtrec.GetDateModified();
  DateCreated = mdtrec.GetDateCreated();
  DateExpires = mdtrec.GetDateExpires();
  Category    = mdtrec.GetCategory();
#ifdef DEBUG_MEMORY
  __IB_RESULT_allocated_count++;
#endif
}

RESULT::RESULT(const RESULT& OtherResult)
{
#ifdef DEBUG_MEMORY
  __IB_RESULT_allocated_count++;
#endif
  *this = OtherResult;
}

RESULT& RESULT::operator=(const RESULT& OtherResult)
{
  Index        = OtherResult.Index;
  ExtIndex     = OtherResult.ExtIndex;
  Key          = OtherResult.Key;
  DocumentType = OtherResult.DocumentType;
  Locale       = OtherResult.Locale;
  Pathname     = OtherResult.Pathname;
  origPathname = OtherResult.origPathname;
  RecordStart  = OtherResult.RecordStart;
  RecordEnd    = OtherResult.RecordEnd;
  Date         = OtherResult.Date;
  DateModified = OtherResult.DateModified;
  DateCreated  = OtherResult.DateCreated;
  DateExpires  = OtherResult.DateExpires;
  Score        = OtherResult.Score;
  AuxCount     = OtherResult.AuxCount;
  Category     = OtherResult.Category;
  HitTable     = OtherResult.HitTable;
// HitTable->SortByFc();
  return *this;
}

STRING RESULT::GetGlobalKey(char Ch) const
{
  INT VirtualIndex = GetVirtualIndex();
  if (VirtualIndex)
    return STRING().form("%d%c%s", VirtualIndex, Ch, Key.c_str());    
  return Key;
}

const FCT RESULT::GetHitTable(FCLIST *HitTableBuffer) const
{
  if (HitTableBuffer)
    {
      *HitTableBuffer = HitTable.GetFCLIST();
    }
  return HitTable;
}

#if 0
off_t RESULT::GetRecordSize() const
{
  return (RecordEnd - RecordStart + 1);
}
#endif

void RESULT::GetRecordData(STRING *StringBuffer, DOCTYPE *DoctypePtr) const
{
  STRING       fn (GetFullFileName());

  StringBuffer->Clear();
  if (RecordEnd <= RecordStart)
    message_log (LOG_ERROR, "Element Error (%ld-%ld) in '%s'!", RecordStart, RecordEnd, fn.c_str());
  else if (::GetRecordData(fn, StringBuffer, RecordStart, (size_t)(RecordEnd - RecordStart + 1), DoctypePtr) == 0)
    message_log (LOG_ERRNO, "Element %ld-%ld read error in '%s'!", RecordStart, RecordEnd, fn.c_str()); // ERROR
}

void RESULT::Write(FILE *fp) const
{
  putObjID (objRESULT, fp);
  ::Write(Index, fp);
  ::Write(ExtIndex, fp);
  ::Write(Key, fp);
  ::Write(DocumentType, fp);
  ::Write(Locale, fp);
  ::Write(Pathname, fp);
  ::Write(origPathname, fp);
  ::Write(RecordStart, fp);
  ::Write(RecordEnd, fp);
  ::Write(Date, fp);
  ::Write(DateModified, fp);
  ::Write(DateCreated, fp);
  ::Write(DateExpires, fp);
  ::Write(Score, fp);
  ::Write(AuxCount, fp);
  ::Write(Category, fp);
  HitTable.Write(fp);
}

GDT_BOOLEAN RESULT::Read(FILE *fp)
{
  obj_t obj = getObjID(fp);
  if (obj != objRESULT)
    {
      PushBackObjID(obj, fp);
    }
  else
    {
      ::Read(&Index, fp);
      ::Read(&ExtIndex, fp);
      ::Read(&Key, fp);
      ::Read(&DocumentType, fp);
      ::Read(&Locale, fp);
      ::Read(&Pathname, fp);
      ::Read(&origPathname, fp);
      ::Read(&RecordStart, fp);
      ::Read(&RecordEnd, fp);
      ::Read(&Date, fp);
      ::Read(&DateModified, fp);
      ::Read(&DateCreated, fp);
      ::Read(&DateExpires, fp);
      ::Read(&Score, fp);
      ::Read(&AuxCount, fp);
      ::Read(&Category, fp);
      HitTable.Read(fp);
    }
  return obj == objRESULT;
}

STRING RESULT::XMLHitTable() const
{
  STRING XML;
  if (!HitTable.IsEmpty())
    {
      size_t z = 0;
      for (const FCLIST *ptr = HitTable, *itor = ptr->Next(); itor != ptr; itor = itor->Next())
        {
	  XML << "  <LOC POS=\"" << itor->Value().GetFieldStart()
		<< "\" LEN=\"" << itor->Value().GetLength() << "\"/>\n";
	  z++;
        }
      STRING prefix ("<HITS UNITS=\"characters\" NUMBER=\"");
      prefix << z << "\">\n";
      XML.Insert(1, prefix);
      XML << "</HITS>\n";
    }
  return XML;
}

STRING RESULT::GetXMLHighlightRecordFormat(int pageno, off_t offset) const
{
  STRING XML ("<XML>\n<Body Units=Characters color=#FF00FF Mode=Active version=2>\n <Highlight>\n");
  if (!HitTable.IsEmpty())
    {
      pageno = Key.SearchReverse(':');
      if (pageno) pageno = atoi(((const char *)Key) + pageno);
      if (pageno > 1) pageno--;
      for (const FCLIST *ptr = HitTable, *p = ptr->Next(); p != ptr ; p = p->Next())
        {
	  const off_t Start = p->Value().GetFieldStart() - offset;
	  const off_t End   = p->Value().GetFieldEnd() - offset;
	  XML << "\t<Loc Pg=" << pageno << " pos=" << Start << " len=" << End-Start+1 << " />\n";
        }
    }
  else
    XML << "\t<loc Pg=" << pageno << " pos=0 len=0 />\n";
  XML << "</Highlight></Body></XML>\n";
  return XML;
}

FC RESULT::GetBestContextHit() const
{
  int metric = 200; 
  const FCLIST *listPtr = HitTable.GetPtrFCLIST();
  const FCLIST *Nth = listPtr->Next();
  for (const FCLIST *p = Nth; p != listPtr; p = p->Next())
    {
      int distance = p->Next()->Value().GetFieldEnd()-p->Value().GetFieldStart();
      if (distance > 0 && distance < metric)
	{
	  Nth = p;
	  metric = distance;
	}
    }
  return Nth->Value();
}

GDT_BOOLEAN RESULT::PresentBestContextHit(STRING *StringBuffer, STRING *Term,
  const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr, STRING *TagPtr) const
{
  return PresentHit(GetBestContextHit(), StringBuffer, Term, BeforeTerm, AfterTerm, DoctypePtr, TagPtr);
}

GDT_BOOLEAN RESULT::PresentNthHit(size_t N, STRING *StringBuffer, STRING *Term,
        const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr, STRING *TagPtr) const
{
  FC Fc;
  if (StringBuffer)
    StringBuffer->Clear();
  if (Term)
    Term->Clear();
  if (TagPtr)
    TagPtr->Clear();

  if (HitTable.GetEntry(N, &Fc))
    {
      return PresentHit(Fc, StringBuffer, Term, BeforeTerm, AfterTerm, DoctypePtr, TagPtr);
    }
  return GDT_FALSE;
}


/*
  Pass FC for hit
  BeforeTerm, AfterTerm to insert before and after the hit
  DoctypePtr as the pointer to the DOCTYPE class to handle reading (the class in the RESULT)

   DoctypePtr from a RESULT  ResultRecord is (IDB class):
      GetDocTypePtr( ResultRecord.GetDocumentType() )

   Class IDB:
         DOCTYPE *GetDocTypePtr(const DOCTYPE_ID& DocType) const;

  Returns:

   StringBuffer -> Content
   Term         -> The term that the FC hits
   TagPtr       -> The tag/path of the hit

   TRUE/FALSE   -> If OK

*/
static const int PeerMinimumFieldLength = 5;

GDT_BOOLEAN RESULT::PresentHit(const FC& Fc, STRING *StringBuffer, STRING *Term,
        const STRING& BeforeTerm, const STRING& AfterTerm, DOCTYPE *DoctypePtr,
 	STRING *TagPtr) const
{
  if (StringBuffer)
    StringBuffer->Clear();
  if (Term)
    Term->Clear();
  if (TagPtr)
    TagPtr->Clear();

  if (!Fc.IsEmpty())
    {
      const GPTYPE start      = Fc.GetFieldStart();
      const GPTYPE end        = Fc.GetFieldEnd();
      GPTYPE       localStart = (start > 44 ? start - 40 : 0);
      GPTYPE       localEnd   = end + 60 + (end-start)/2;
      GPTYPE       peer_end   = RecordEnd;
 

      GDT_BOOLEAN  skipToFirstWord = GDT_TRUE;

#if 1
      if (DoctypePtr && DoctypePtr->Db) {
	IDBOBJ *idb = DoctypePtr->Db;
	MDTREC  mdtrec;
	STRING  Tag;
        if ( idb->GetMainMdt()->GetEntry (GetMdtIndex(), &mdtrec))
	  {
	     off_t offset = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();
             FC     PeerFC = idb->GetPeerFc(FC(Fc)+=offset,&Tag);

	     if (TagPtr) *TagPtr = Tag;

	     GPTYPE peer_start = PeerFC.GetFieldStart() - offset;
	     peer_end   = PeerFC.GetFieldEnd() - offset + 1  /***** Added + 1 // 2022 ***/;

	     if (peer_end - peer_start < 200) {
		// Min
		if ((peer_end - peer_start) > (end - start + PeerMinimumFieldLength)) {
		  localEnd = peer_end;
		  localStart = peer_start;
		  skipToFirstWord = GDT_FALSE;
		}
	     } else {
		if (peer_end < localEnd || ((peer_end - localEnd) < 200))
		  localEnd = peer_end;
		if (peer_start > localStart || (localStart - peer_start < 200)) {
		  localStart = peer_start;
		  skipToFirstWord = GDT_FALSE;
		}
	     }
	  }
      }
#endif


      if (RecordEnd < RecordStart)
	{
	  return GDT_FALSE;
	}

      if ((localEnd + RecordStart) > RecordEnd)
      {
	localEnd = RecordEnd - RecordStart; // Don't cross boundaries
      }

      if (localEnd <= localStart)
	{
	  return GDT_FALSE;
	}

      const size_t Length = localEnd - localStart + 1;
//    if (Length > BUFSIZ) Length = BUFSIZ;
      STRING strPtr;


      if (::GetRecordData(GetFullFileName(), &strPtr, localStart + RecordStart, Length, DoctypePtr) == 0)
	return GDT_FALSE;
      register unsigned char *ptr = (unsigned char *)(strPtr.c_str());
      if (StringBuffer)
	{
	  register unsigned char *tcp = ptr;
	  if (localStart && skipToFirstWord)
	    {
	      while (!isspace(*tcp))
		tcp++; // Skip to first word..
	    }
	  int i = tcp - ptr;
	  if (i > (int)(start - localStart))
	    {
	      i = 0;
	      tcp = ptr;
	    }
	  else if (localStart && i)
	    {
	      StringBuffer->Cat ("... ");
	      i -= 4; // Offset from "... "
	    }
	  for (; *tcp; tcp++)
	    {
	      if (*tcp == '<' || *tcp == '>' || *tcp == '&')
		{
		  StringBuffer->Cat ('.');
		}
	      else
		{
		  StringBuffer->Cat (*tcp);
		}
	    }

#if 0
cerr << "Build copy" << endl;
        char tmp[1024];
        strncpy(tmp, StringBuffer->c_str() + start - localStart - i, end - start +1);
        tmp[end-start+1] = 0;
cerr << "Term= \"" << tmp << "\"" << endl;
#endif

	  if (AfterTerm.GetLength())
	    StringBuffer->Insert(end - localStart - i + 2, AfterTerm);
	  if (BeforeTerm.GetLength())
	    StringBuffer->Insert(start - localStart - i + 1, BeforeTerm);
	  // Remove multiple empty spaces...
	  StringBuffer->Pack();
	  if (localEnd != peer_end)
	    StringBuffer->Cat ("...");
	}
      if (Term)
	{
	  unsigned char *term = ptr + (start - localStart);
	  term[end - start + 1] = '\0';
	  *Term = term;
	}
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

GDT_BOOLEAN RESULT::XMLPresentNthHit(size_t N, STRING *StringBuffer, const STRING& Tag,
   STRING *Term, DOCTYPE *DoctypePtr) const
{
  FC      Fc;
  STRING  hitTag;

  if (StringBuffer)
    StringBuffer->Clear();
  if (Term)
    Term->Clear();
  if (HitTable.GetEntry(N, &Fc))
    {
      const GPTYPE start = Fc.GetFieldStart();
      const GPTYPE end   = Fc.GetFieldEnd();
      GPTYPE       localStart = (start > 44 ? start - 40 : 0);
      GPTYPE       localEnd   = end + 60 + (end-start)/2;
      GPTYPE       peer_end = RecordEnd;
      GDT_BOOLEAN  skipToFirstWord = GDT_TRUE;

      if (DoctypePtr && DoctypePtr->Db) {
        IDBOBJ *idb = DoctypePtr->Db;
        MDTREC  mdtrec;
        if ( idb->GetMainMdt()->GetEntry (GetMdtIndex(), &mdtrec))
          {
             off_t offset = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();
             FC     PeerFC = idb->GetPeerFc(FC(Fc)+=offset,&hitTag);

             GPTYPE peer_start = PeerFC.GetFieldStart() - offset;

             peer_end   = PeerFC.GetFieldEnd() - offset + 1 /* 2022 add +1 */;

             if (peer_end - peer_start < 200) {
		// Min
		if ((peer_end - peer_start) > (end - start + PeerMinimumFieldLength)) {
                  localEnd = peer_end;
                  localStart = peer_start;
                  skipToFirstWord = GDT_FALSE;
		}
             } else {
                if (peer_end < localEnd || ((peer_end - localEnd) < 200))
                  localEnd = peer_end;
                if (peer_start > localStart || (localStart - peer_start < 200)) {
                  localStart = peer_start;
                  skipToFirstWord = GDT_FALSE;
                }
             }
          }
      }

      if (localEnd > RecordEnd) localEnd = RecordEnd - RecordStart;

      if (localStart > localEnd)
	{
	  message_log (LOG_PANIC, "Start after End in RESULT::XMLPresentNthHit()");
StringBuffer->form("ERROR (%ld,%ld) not inside Record (%ld,%ld)",  start, end, RecordStart, RecordEnd);
	  return GDT_FALSE;
	}

      const size_t Length = localEnd - localStart + 1;

//    if (Length > BUFSIZ) Length = BUFSIZ;
      STRING strPtr;
      if (::GetRecordData(GetFullFileName(), &strPtr, localStart + RecordStart, Length, DoctypePtr) == 0)
	return GDT_FALSE;
      register unsigned char *ptr = (unsigned char *)(strPtr.c_str());
      if (StringBuffer)
	{
	  STRING Context;
	  register unsigned char *tcp = ptr;

	  if (localStart && skipToFirstWord)
	    {
	      while (!isspace(*tcp) && *tcp)
		tcp++; // Skip to first word..
	    }
	  int i = tcp - ptr;

          if (i > (int)(start - localStart))
	    {
	      Context.Cat ("...");
	      i = -3;
	      tcp = ptr;
	    }
	  else if (localStart && i)
	    {
	      Context.Cat ("... ");
	      i -= 4; // Offset from "... "
	    }
	  Context.Cat(tcp);
	  Context.Insert(end - localStart - i + 2, "\002");
	  Context.Insert(start - localStart - i + 1, "\001");
	  // Remove multiple empty spaces...
	  Context.Pack();
#if 0
	  if (localEnd != RecordEnd) Context.Cat ("...");
#else
	  if (localEnd != peer_end) Context.Cat ("...");
#endif
	  const int Context_len = (int) Context.Length();
	  const BYTE   charsetId =  Locale.GetCharsetId();
	  CHARSET charset (charsetId);
	  for (i=1; i <= Context_len; i++)
	    {
	      char buf[11]; // max 4294967295 although most is only short (UCS-2)
	      unsigned int wchar;
	      unsigned char ch;

	      if ((ch = Context.GetUChr(i)) == '\001')
		{
		  *StringBuffer << "<" << Tag;
		  if (hitTag.GetLength())
		    *StringBuffer << " CONTAINER_NAME=\"" << hitTag << "\"";
		  *StringBuffer << ">";
		}
	      else if (ch == '\002')
		{
		  *StringBuffer << "</" << Tag << ">";
		}
	      else if ((wchar = (unsigned)charset.UCS(ch)) < 0x7f && wchar > 0x1f)
		{
		  if (wchar == '<')
		    StringBuffer->Cat("&lt;");
		  else if (wchar == '>')
		    StringBuffer->Cat("&gt;");
		  else if (wchar == '&')
		    StringBuffer->Cat("&amp;");
		  else
		    StringBuffer->Cat(ch);
		}
	      else if (wchar == 160)
		{
		  StringBuffer->Cat("&nbsp;");
		}
	      else if (wchar == 173)
		{
		  StringBuffer->Cat("&shy;");
		}
	      else
		{
		  // Non-ASCII character -- Map to UCS
		  sprintf(buf, "&#%u;", wchar);
		  StringBuffer-> Cat (buf);
		}
	    }
	}
      if (Term)
	{
	  unsigned char *term = ptr + (start - localStart);
	  term[end - start + 1] = '\0';
	  *Term = term;
	}
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

void RESULT::GetHighlightedRecord(const STRING& BeforeTerm, const STRING& AfterTerm,
	STRING *StringBuffer, DOCTYPE *DoctypePtr) const
{
  GetRecordData(StringBuffer, DoctypePtr);

  GPTYPE End = StringBuffer->GetLength() + 1;
  GPTYPE Start = End;

  // process terms backwards
  for (const FCLIST *ptr = HitTable, *p = ptr->Prev(); p != ptr; p = p->Prev())
    {
      const FC Fc ( p->Value() );
      // @@@ Highlight Bugfix workaround (edz@nonmonotonic.com)
      // see also fct.cxx
      if ( Fc.GetFieldEnd() < End && Fc.GetFieldStart() < Start)
	{
	  End = Fc.GetFieldEnd();
	  Start = Fc.GetFieldStart();
	  StringBuffer->Insert(End + 2, AfterTerm);
	  StringBuffer->Insert(Start + 1, BeforeTerm);
	}
    }
}


// Get the Range of the record with bits highlighted..
void RESULT::GetHighlighted(const STRING& BeforeTerm, const STRING& AfterTerm, FC Range,
	STRING *StringBuffer, DOCTYPE *DoctypePtr) const
{
  StringBuffer->Clear();
  const GPTYPE rs = RecordStart + Range.GetFieldStart();
  const GPTYPE re = Range.GetFieldEnd();

  // Make sure that the range is consistant..
  if (rs > re || re > RecordEnd || rs < RecordStart)
    return; // Error

  const size_t size = re - rs + 1;

  if (::GetRecordData( GetFullFileName(), StringBuffer, rs, size, DoctypePtr) == 0)
    return; // Error

  // Now have the data...
  GPTYPE End = size + 1;
  GPTYPE Start = End;
  // process terms backwards
  for (const FCLIST *ptr = HitTable, *p = ptr->Prev(); p != ptr; p = p->Prev())
    {
      const FC Fc ( p->Value() );
      const GPTYPE end = Fc.GetFieldEnd();
      const GPTYPE start = Fc.GetFieldStart();
      // @@@ Highlight Bugfix workaround (edz@nonmonotonic.com)
      // see also fct.cxx
      if ( end < End && start < Start)
	{
	  End = end;
	  Start = start;
	  StringBuffer->Insert(End + 2, AfterTerm);
	  StringBuffer->Insert(Start + 1, BeforeTerm);
        }
    }
}



RESULT::~RESULT()
{
#ifdef DEBUG_MEMORY
  if (--__IB_RESULT_allocated_count < 0)
    message_log (LOG_PANIC, "RESULT global allocated count %ld < 0!", (long)__IB_RESULT_allocated_count);
#endif
}

