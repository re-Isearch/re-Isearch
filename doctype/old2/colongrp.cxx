/*-@@@
File:		colongrp.cxx
Version:	1.00   
Description:	Class COLONGRP - COLONDOC-like Text w/ groups
Author:		Archie Warnock, warnock@clark.net
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "common.hxx"
#include "doc_conf.hxx"

#include "strstack.hxx"
#include "gstack.hxx"
#include "colongrp.hxx"
#include "lang-codes.hxx"

#define AW_DEBUG 0

#define MaxCOLONGRPSize 10240


COLONGRP::COLONGRP (PIDBOBJ DbParent, const STRING& Name):
	COLONDOC (DbParent, Name)
{
}

const char *COLONGRP::Description(PSTRLIST List) const
{
  List->AddEntry ("COLONGRP");
  COLONDOC::Description(List);
  return "COLONDOC-like text with DIF inspired groups.";
}


void COLONGRP::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-COLONGRP";
}



void COLONGRP::AfterIndexing()
{

  recBuffer.Free(Doctype, "recBuffer");
  tagBuffer.Free(Doctype, "tagBuffer");
  startsBuffer.Free(Doctype, "startsBuffer");
  endsBuffer.Free(Doctype, "endsBuffer");
  COLONDOC::AfterIndexing();
}

void COLONGRP::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
  if (!fp)
    {
      return;			// ERROR
    }

  INT4 RecStart = NewRecord->GetRecordStart ();
  INT4 RecEnd = NewRecord->GetRecordEnd ();
  if ((RecEnd == 0) || (RecEnd == -1))
    {
      fseek (fp, 0, 2);
      RecStart = 0;
      RecEnd = ftell (fp) - 1;
    }
  fseek (fp, RecStart, 0);

  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = (PCHR)recBuffer.Want (RecLength + 1);
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  fclose (fp);

  RecBuffer[ActualLength] = '\0';

  PCHR *groups = parse_Groups (RecBuffer, ActualLength);
  PCHR *tags = parse_tags (RecBuffer, ActualLength);

  if (tags == NULL || tags[0] == NULL)
    {
      STRING doctype;
      NewRecord->GetDocumentType (&doctype);
      if (tags)
	{
	  logf (LOG_WARN, "Found no `%s' fields/tags in \"%s\"[%ld-%ld].",
		(const char *)doctype, (const char *)fn,
		(long)RecStart, (long)RecEnd);
	}
      else
	{
	  logf (LOG_WARN, "%s parser was unable to process \"%s\"[%ld-%ld].",
		(const char *)doctype, (const char *)fn,
		(long)RecStart, (long)RecEnd);
	}
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName;

  GDT_BOOLEAN first_tag = (KeyField.GetLength() == 0);
  // Walk though tags, skipping the groups
  STRLIST FieldNames;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      PCHR p = tags_ptr[1];	// end of field
      if (p == NULL)		// If no end of field
	p = &RecBuffer[RecLength];	// use end of buffer

      // eg "Author:"
      if (strcasecmp(*tags_ptr, "Group") != 0)
	{
	  size_t off = strlen (*tags_ptr) + 1;
	  size_t val_start = (*tags_ptr + off) - RecBuffer;
	  // Skip while space after the ':'
	  while (isspace (RecBuffer[val_start]))
	    val_start++, off++;
	  // Also leave off the \n
	  int val_len = (p - *tags_ptr) - off - 1;
	  // Strip potential trailing while space
	  while (val_len >= 0 && isspace (RecBuffer[val_len + val_start]))
	    val_len--;
	  if (val_len < 0)
	    {
	      logf (LOG_WARN, "Record in \"%s\" had an 'empty' field '%s'",
                  (const char *)fn, *tags_ptr);
	      continue;           // Don't bother with empty fields
	    }

	  if (first_tag || (KeyField ^= *tags_ptr))
	    {
	      PCHR entry_id = new CHR[val_len + 2];
	      strncpy (entry_id, (*tags_ptr + off), val_len + 1);
	      entry_id[val_len + 1] = '\0';

	      if (Db->KeyLookup (entry_id))
		logf (LOG_ERROR, "Record in \"%s\" uses a non-unique %s '%s'",
		  (const char *)fn, *tags_ptr, entry_id);
	      else
		NewRecord->SetKey (entry_id);
	      delete[]entry_id;
	      first_tag = GDT_FALSE;
	    }
#if BSN_EXTENSIONS
	  else if (DateField ^= *tags_ptr)
	    NewRecord->SetDate( &RecBuffer[val_start] );
	  else if (DateModifiedField ^= *tags_ptr)
	    NewRecord->SetDateModified( &RecBuffer[val_start] );
	  else if (DateCreatedField ^= *tags_ptr)
	    NewRecord->SetDateCreated( &RecBuffer[val_start] );
	  else
#endif
	  if (LanguageField ^= *tags_ptr)
	    {
	      // Only if valid do we over-ride
	      SHORT code = Lang2Id ( &RecBuffer[val_start] );
	      if (code != 0)
		NewRecord->SetLanguage (code);
	    }
	  // Add the coordinates for all the field names..
	  const INT Total = UnifiedNames(*tags_ptr, &FieldNames);
	  if (Total)
	    {
	      FC Fc (val_start, val_start + val_len);
	      df.SetFct(Fc);
	      // Now Walk through list (backwards)..
	      for (const STRLIST *p = FieldNames.Prev(); p != &FieldNames; p=p->Prev())
		{
		  dfd.SetFieldType( Db->GetFieldType( p->Value() ) ); // Get the type added 30 Sep 2003
		  dfd.SetFieldName (p->Value());
		  Db->DfdtAddEntry (dfd);
		  df.SetFieldName (p->Value());
		  pdft->AddEntry (df);
		}
	    }
      } // Not a "Group"
  }

  // Now, deal with the groups
  GSTACK pGroups;
  STRSTACK pFieldNames;

  for (PCHR * tags_ptr1 = groups; *tags_ptr1; tags_ptr1++)
    {
      PCHR p = tags_ptr1[1];	// end of field
      if (p == NULL)		// If no end of field
	p = &RecBuffer[RecLength];	// use end of buffer

      INT val_start, val_len;
      FieldName = *tags_ptr1;
      if (FieldName.CaseEquals ("Group"))
	{
	  size_t off = strlen (*tags_ptr1) + 1;
	  val_start = (*tags_ptr1 + off) - RecBuffer;

	  // Skip while space after the ':'
	  while (isspace (RecBuffer[val_start]))
	    val_start++, off++;

	  if (val_start == RecLength)
	    break; // Premature EOF

	  // Now, RecBuffer[val_start] should point to the first word after
	  // "GROUP", so it's the new fieldname
	  FieldName.Clear();
	  for (val_len = 0;
	       !isspace(RecBuffer[val_start + val_len]) && ((val_start+val_len) < RecLength);
	       val_len++)
	    {
	      FieldName.Cat(RecBuffer[val_start + val_len]);
	    }

	  INT *Val = new INT;

	  *Val = val_start;
	  pGroups.Push (Val);
	  pFieldNames.Push (FieldName);
	}
      else
	{
	  INT  *pStart;
	  INT val_end;

	  val_end = *tags_ptr1 - RecBuffer;
	  pFieldNames.Pop (&FieldName);
	  pStart = (INT *) pGroups.Top ();
	  pGroups.Pop ();
	  val_start = *pStart /* Added by edz -> */ + FieldName.GetLength() + 1;
	  delete pStart;

	  // Now store the fields
	  STRLIST FieldNamesList;
	  const INT Total = UnifiedNames(FieldName, &FieldNamesList);
	  if (Total)
	    {
	      fc.SetFieldStart (val_start);
	      fc.SetFieldEnd (val_start + val_len);
	      df.SetFct (fc);
	      // Forward
	      for (const STRLIST *p = FieldNamesList.Next(); p != &FieldNamesList; p=p->Next())
		{
		  dfd.SetFieldType( Db->GetFieldType(p->Value()) ); // Get the type added 30 Sep 2003
		  dfd.SetFieldName (p->Value());
		  Db->DfdtAddEntry (dfd);
		  df.SetFieldName (p->Value());
		  pdft->AddEntry (df);
		}
	   }
	}
    }

  NewRecord->SetDft (*pdft);
  delete pdft;
}

void COLONGRP::
DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    const STRING& RecordSyntax, PSTRING StringBufferPtr) const
{
  if (ElementSet == SOURCE_MAGIC)
    DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
  else
    COLONDOC::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}


void COLONGRP::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 const STRING& RecordSyntax, PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "";
  if (ElementSet == BRIEF_MAGIC)
    {
      STRING Headline;
      DOCTYPE::Present (ResultRecord, BRIEF_MAGIC, &Headline);
      if (Headline.GetLength() == 0)
	Headline = "<Untitled Record>";
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  HtmlCat(Headline, StringBufferPtr);
	}
      else
	{
	  *StringBufferPtr = Headline;
	}
    }
  else if ((ElementSet != FULLTEXT_MAGIC) && (RecordSyntax == HtmlRecordSyntax) )
    {
      STRING Value;
      COLONDOC::Present (ResultRecord, ElementSet, RecordSyntax, &Value);

      const size_t len = Value.GetLength();
      size_t pos = len;
      while ( isspace( Value.GetChr(pos) ) )
	pos--;
      if (pos != len) Value.EraseAfter(pos);

      if (Value.Search("\n ") || Value.Search("\n\t"))
	{
	  *StringBufferPtr << "<PRE>" << Value << "</PRE>";
	}
      else
	*StringBufferPtr = Value;
    }
  else
    COLONDOC::Present (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}

COLONGRP::~COLONGRP ()
{
}


/* Handle the Group stuff */
#define TAG_GROW_SIZE 20
PCHR * COLONGRP::parse_Groups (PCHR b, size_t len)
{
  PCHR *t, *starts, *ends;	// array of pointers to first char of tags

  PCHR where, found;
  size_t tc = 0;		// tag count

  size_t tc_start = 0;		// tag count

  size_t tc_end = 0;		// tag count

  size_t max_num_tags;		// max num tags for which space is allocated

  // You should allocate these as you need them, but for now...
  max_num_tags = TAG_GROW_SIZE;

  t = (PCHR *)tagBuffer.Want (max_num_tags, sizeof(PCHR));
  starts = (PCHR *)startsBuffer.Want(max_num_tags, sizeof(PCHR));
  ends = (PCHR *)endsBuffer.Want(max_num_tags, sizeof(PCHR));

  where = b;
  while ((where-b <= len) && (found = strstr (where, "Group:")))
    {

      starts[tc_start] = found;
#if AW_DEBUG
      cout << "Group offset: " << found - b << endl;
#endif /* AW_DEBUG */
      where = found + strlen ("Group:");

      // Expand memory if needed
      starts = (PCHR *)startsBuffer.Expand(++tc_start, sizeof(PCHR));
    }

  where = b;
  while ((where-b <= len) && (found = strstr (where, "End_Group")))
    {
      ends[tc_end] = found - 1;	// BUGFIX edz:  -1
#if AW_DEBUG
      cout << "End_Group offset: " << found - b << endl;
#endif /* AW_DEBUG */
      where = found + strlen ("End_Group") - 1;		// BUGFIX edz: -1 !!

      // Expand memory if needed
      ends = (PCHR *)endsBuffer.Expand(++tc_end, sizeof(PCHR));
    }

  // Bail out if we don't have the same number of starting and ending tags
  if (tc_start != tc_end)
    {
      logf (LOG_WARN, "Warning: unable to parse Groups. Mismatched starting and ending tags.");
      t[0] = (PCHR) NULL;
      return t;
    }


  // Now, sort the list on the addresses
  size_t n_Group = 0, n_End = 0;
  INT start, end;
  do
    {
      start = starts[n_Group] - b;
      end = ends[n_End] - b;
#if AW_DEBUG
      cout << "Comparing start: " << start << " end: " << end << endl;
#endif /* AW_DEBUG */
      if (start < end)
	{
	  t[tc] = starts[n_Group];
#if AW_DEBUG
	  cout << "Saving starting offset: " << start << endl;
#endif /* AW_DEBUG */
	  n_Group++;
	}
      else
	{
	  t[tc] = ends[n_End];
#if AW_DEBUG
	  cout << "Saving ending offset: " << end << endl;
#endif /* AW_DEBUG */
	  n_End++;
	}
      // Expand memory if needed
      t = (PCHR *)tagBuffer.Expand(++tc, sizeof(PCHR));
    }
  while ((n_Group < tc_start) && (n_End < tc_end));

  // All the Group tags should be done, but there might still be some
  // remaining End_Group tags, so we need to put them in, too

  while (n_End < tc_end)
    {
      end = ends[n_End] - b;
#if AW_DEBUG
      cout << "Leftover End_Group offset: " << end << endl;
#endif /* AW_DEBUG */
      t[tc] = ends[n_End];
      n_End++;

      // Expand memory if needed
      t = (PCHR *)tagBuffer.Expand(++tc, sizeof(PCHR));
    }

  t[tc] = (PCHR) NULL;
  return t;
}
