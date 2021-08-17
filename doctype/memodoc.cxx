#pragma ident  "@(#)memodoc.cxx	1.10 02/24/01 17:45:22 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		memodoc.cxx
Version:	$Revision: 1.1 $
Description:	Class MEMODOC - Colon-like Memo Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "memodoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"
#include <alloca.h>


MEMODOC::MEMODOC (PIDBOBJ DbParent, const STRING& Name) :
	PTEXT (DbParent, Name)
{
  const char PARSE_MESSAGE_STRUCTURE[] = "ParseMessageStructure";
  const char AUTODETECT_FIELDTYPES[]   = "AutodetectFieldtypes";

  ParseMessageStructure = Getoption(PARSE_MESSAGE_STRUCTURE, "Y").GetBool();
  message_log (LOG_DEBUG, "%s: %s=%d", Doctype.c_str(), PARSE_MESSAGE_STRUCTURE, (int)ParseMessageStructure);

  autoFieldTypes  =  Getoption(AUTODETECT_FIELDTYPES, "Y").GetBool();
  recordsAdded = 0;

  help_text = "Colon-like Memo document type.\n\
Format COLONDOC type lines with a blank line to separate to the body\n\n\
Example:\n\
Field_name: Content of the field\n\
Field_name2: Content of the 2nd field\n\
\nHere is the content of the \"";
  help_text << UnifiedName(Doctype+"-Body");
  help_text  << "\" field.\n\nOptions:\n" << 
  "[General]\n" << 
  PARSE_MESSAGE_STRUCTURE << "=Yes|No // specifies if the message line/sentence/paragraph\n\
                             // structure should be parsed (Default: " <<
	(ParseMessageStructure ? "YES" : "NO")  << ")\n" <<
 AUTODETECT_FIELDTYPES << "=Yes|No  // specified if we should guess fieldtypes (default:" <<
	(autoFieldTypes ? "YES" : "NO") << ").\n\n";
}

const char *MEMODOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("MEMODOC");
  if (Doctype.Search("MEMO") == 0 && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  PTEXT::Description(List);
  return help_text.c_str();
}

void MEMODOC::BeforeIndexing()
{
  PTEXT::BeforeIndexing();
}

void MEMODOC::AfterIndexing()
{
  tmpBuffer.Free();
  tagBuffer.Free();
  PTEXT::AfterIndexing();
}


FIELDTYPE MEMODOC::GuessFieldType(const STRING& FieldName, const STRING& Contents)
{
  if (Db && Contents.GetLength())
    {
      FIELDTYPE ft ( Db->GetFieldType(FieldName) );

      // We already have a type?
      if (ft.Defined())
	return ft;

      if (autoFieldTypes)
	{
	  if (Contents.IsGeoBoundedBox())      ft = FIELDTYPE::box;
	  else if (Contents.IsNumberRange())   ft = FIELDTYPE::numericalrange;
	  else if (Contents.IsNumber())
	    {
	      // Number can also be date?
	      // CCYYMMDD
	      if (Contents.GetLong() > 19000101)
		{
	          SRCH_DATE date (Contents);
	          ft = (date.Ok() && date.IsDayDate()) ? FIELDTYPE::date : FIELDTYPE::numerical;
		}
	      else ft = FIELDTYPE::numerical;
	    }
	  else if (Contents.IsDateRange())     ft = FIELDTYPE::daterange;
	  else if (Contents.IsDate())          ft = FIELDTYPE::date;
	  else if (Contents.IsCurrency())      ft = FIELDTYPE::currency;
	  else if (Contents.IsDotNumber())     ft = FIELDTYPE::dotnumber;
#if 1
	  else if ((Contents.GetLength() > DocumentKeySize) && ( FieldName ^= "ipnode"))
	    ft = FIELDTYPE::hash;
#endif
	  else ft = FIELDTYPE::text;
	  message_log (LOG_INFO, "%s: Field '%s' autotyped as '%s'", Doctype.c_str(), FieldName.c_str(), ft.c_str());

	}
      else ft = FIELDTYPE::text;
      Db->AddFieldType(FieldName, ft);
      return ft;
    }
  return FIELDTYPE::text;
}


void MEMODOC::SourceMIMEContent(PSTRING StringPtr) const
{ 
  // MIME/HTTP Content type for MEMO Sources 
  *StringPtr = "Application/X-Memo";  
} 

void MEMODOC::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}

void MEMODOC::ParseRecords (const RECORD& FileRecord)
{
  // For records use the default parsing mechanism
  DOCTYPE::ParseRecords (FileRecord);
}

GDT_BOOLEAN MEMODOC::IsIgnoreMetaField(const STRING& Tag) 
{
  if (PTEXT::IsIgnoreMetaField(Tag))
    return GDT_TRUE;
  if (Tag ^=  UnifiedName(Doctype+"-Body"))
    return GDT_TRUE;
  return GDT_FALSE;
}


void MEMODOC::ParseFields (RECORD *NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName () );
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      message_log (LOG_ERRNO, "Can't read '%s'. Skipping", fn.c_str());
      NewRecord->SetBadRecord();
      return;		// ERROR
    }

  STRING Key;
  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp); 
  if (RecEnd <= RecStart)
    {
      Db->ffclose(fp);
      NewRecord->SetBadRecord();
      return;
    }
  fseek (fp, RecStart, SEEK_SET);
  off_t RecLength = RecEnd - RecStart + 1;

  PCHR RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1, sizeof(CHR));

  off_t ActualLength = fread (RecBuffer, 1, RecLength, fp);
  Db->ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	message_log (LOG_WARN, "No `%s' fields/tags in %s [%ld-%ld]", Doctype.c_str(), fn.c_str(),
		(long)RecStart, (long)RecEnd);
       else
	message_log (LOG_ERROR, "Unable to parse `%s' record in %s", Doctype.c_str(), fn.c_str());
      NewRecord->SetBadRecord();
      return;
    }
  if ((tags[0] - RecBuffer) > 127 || strlen(tags[0]) > 512)
    {
      message_log (LOG_WARN,
#ifdef _WIN32
	"%s[%I64d,%I64d] does not seem to be in '%s' format (%lu)." 
#else
	"%s[%lld,%lld] does not seem to be in '%s' format (%lu)."
#endif
                , fn.c_str(), (long long)RecStart, (long long)RecEnd, Doctype.c_str(),
                (unsigned long)(tags[0]-RecBuffer));
      NewRecord->SetBadRecord();
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName;
  // Walk though tags
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      PCHR p = tags_ptr[1];
      if (p == NULL)
	p = &RecBuffer[ActualLength]; // End of buffer
      // eg "Author:"
      int off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ':'
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;

      // Strip potential trailing while space
      while (val_len >= 0 && isspace (RecBuffer[val_len + val_start+1])) // ???? 2008
	val_len--;
      if (val_len < 0) continue; // Don't bother with empty fields

      if ((*tags_ptr)[0] == '\0')
	{
	  UnifiedName(Doctype+"-Body", &FieldName);
	  while (strchr("_-+=", RecBuffer[val_start]))
	    val_start++, val_len--;
	  while (RecBuffer[val_start] == '\n' || RecBuffer[val_start] == '\r')
	    val_start++, val_len--;
	  // BUGFIX Fri Jun 28 12:41:04 MET DST 1996
	  // edz: Add trailing white space 'till first \n 
	  while ((val_len + val_start) < ActualLength)
	    {
	      if (RecBuffer[val_len+val_start] == '\n')
		break;
	      val_len++;
	    }

	  // Now we want to also parse the contents
	  if (ParseMessageStructure)
	    {
	      ParseStructure(pdft, &RecBuffer[val_start], val_start, val_len);
	    }
	}
      else
	{
	  UnifiedName(*tags_ptr, &FieldName);
	  // Wed Nov 19 02:27:06 MET 1997 added date parsing! 
	  if (FieldName ^= TTLField)
	    {
	      int ttl =  atol( &RecBuffer[val_start] );
	      if (ttl)
		{
		  SRCH_DATE expires;

		  expires.SetNow();
		  expires.PlusNminutes(ttl);
		  NewRecord->SetDateExpires( expires );
		}

	    }

#if 0
	  if (IsSpecialField(FieldName))
	    {
	      char entry_id[ val_len + 1];
	      strncpy (entry_id, &RecBuffer[val_start], val_len);
	      entry_id[val_len] = '\0';
	      HandleSpecialFields(NewRecord, FieldName, entry_id);
	    }
#else
	  else if ((FieldName ^= DateField) ||
		(FieldName ^= DateModifiedField) || (FieldName ^= DateCreatedField) ||
		(FieldName ^= DateExpiresField) )
	    {
	      SRCH_DATE Datum (  &RecBuffer[val_start] );
	      if (Datum.Ok())
		{
		  if (FieldName ^= DateField) NewRecord->SetDate( Datum );
		  if (FieldName ^= DateModifiedField) NewRecord->SetDateModified( Datum );
		  if (FieldName ^= DateCreatedField) NewRecord->SetDateCreated( Datum );
		  if (FieldName ^= DateExpiresField) NewRecord->SetDateExpires( Datum );
		}
	      else
		message_log (LOG_ERROR, "Unparseable Date in %s", FieldName.c_str());
	    }
	  else if (FieldName ^= LanguageField)
	    NewRecord->SetLanguage( &RecBuffer[val_start] );
	  else if (FieldName ^=  CategoryField)
	    NewRecord->SetCategory ( &RecBuffer[val_start] );
	  else if (FieldName ^= KeyField)
	    {
	      if (!Key.IsEmpty())
		message_log(LOG_WARN, "Duplicate Keys defined: overwriting %s with %s", Key.c_str(), &RecBuffer[val_start]);
	      Key = &RecBuffer[val_start];
	    }
#endif
	}

      if (FieldName.GetLength())
	{
//***************************************************
	  FIELDTYPE ft ( Db->GetFieldType(FieldName) );
	  if (recordsAdded < 100 && !ft.Defined())
	    {
	      char *entry_id = (char *)alloca( val_len + 1);

              strncpy (entry_id, &RecBuffer[val_start], val_len);
              entry_id[val_len] = '\0';
	      ft = GuessFieldType(FieldName, entry_id);
	    }
	  dfd.SetFieldType(ft); // Set the type
//*****************************************************
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);
	  df.SetFct ( FC(val_start, val_start + val_len) );
	  df.SetFieldName (FieldName);
	  pdft->AddEntry (df);
	}
    }

  NewRecord->SetDft (*pdft);
  delete pdft;

  if (!Key.IsEmpty())
    {
      if (Db->KeyLookup (Key))
        {
          message_log (LOG_WARN, "%s Record in \"%s\" used a non-unique %s '%s'",
                Doctype.c_str(), fn.c_str(), KeyField.c_str(), Key.c_str());
          Db->MdtSetUniqueKey(NewRecord, Key);
        }
      else
        NewRecord->SetKey (Key);
    }
  recordsAdded++;
}

void MEMODOC::
DocPresent (const RESULT& ResultRecord,
	    const STRING& ElementSet, const STRING& RecordSyntax,
	    PSTRING StringBuffer) const
{
  if (ElementSet.Equals (SOURCE_MAGIC))
    {
      DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      return;
    }
  STRING Value;
  GDT_BOOLEAN UseHtml = (RecordSyntax == HtmlRecordSyntax);
  if (UseHtml)
    {
      HtmlHead (ResultRecord, ElementSet, StringBuffer);
      *StringBuffer << "<DL>\n";
    }

  if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey (&Key);
      Db->GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();
      STRING FieldName, S;
      DFD Dfd;
      // MEMO class documents are 1-deep so just collect the fields
      for (size_t i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);
	  if (FieldName ^= UnifiedName(Doctype+"-Body", &S))
	    continue; // Skip Body
	  if (DOCTYPE::IsMetaIgnoreField(FieldName))
	    continue; // Skip this one too

	  DescriptiveName(FieldName, &S);
	  if (S.GetLength())
	    {
	      // Get Value of the field, use parent
	      DOCTYPE::Present (ResultRecord, FieldName, RecordSyntax, &Value);
	      Value.Pack ();
	      if (!Value.IsEmpty())
		{
		  if (UseHtml) *StringBuffer << "<DT>";
		  *StringBuffer << S << ": ";
		  if (UseHtml) *StringBuffer << "<DD>";
		  *StringBuffer << Value << "\n";
		}
	    }
	}			/* for */
    }
  else
    {
      if (UseHtml)
	*StringBuffer << "<DT>";
      *StringBuffer << ElementSet << ": ";
      if (UseHtml)
	*StringBuffer << "<DD>";
      Present (ResultRecord, ElementSet, RecordSyntax, &Value);
      *StringBuffer << Value << "\n";
    }

  if (UseHtml)
    *StringBuffer << "</DL>";

  if (ElementSet.Equals(FULLTEXT_MAGIC))
    {
      static const char rule[] = "\n\
_________________________________________________________________\n";
      STRING S;

      if (UseHtml)
	*StringBuffer << "<HR><PRE>";
      else
	*StringBuffer << rule;
      Present (ResultRecord, UnifiedName(Doctype+"-Body", &S), RecordSyntax, &Value);
      *StringBuffer << Value;
      if (UseHtml)
 	*StringBuffer << "</PRE>";
    }

  if (UseHtml)
    {
      HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
    }
}


void MEMODOC::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      // Brief Headline is "From: Subject"
      STRING From;
      DOCTYPE::Present (ResultRecord, "FROM", RecordSyntax, &From);
      STRING Subject;
      DOCTYPE::Present (ResultRecord, "SUBJECT", RecordSyntax, &Subject);
      *StringBuffer = "Memo ";
      if (From.GetLength() || Subject.GetLength())
	{
	  if (!From.IsEmpty())
	    *StringBuffer << "from " << From;
	  if (!Subject.IsEmpty())
	    *StringBuffer << ": " << Subject;
	}
      else
	{
	  // Use default method
	  STRING Headline;
	  DOCTYPE::Present (ResultRecord, BRIEF_MAGIC, RecordSyntax, &Headline);
	  *StringBuffer << Headline;
	}
    }
  else
    DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

MEMODOC::~MEMODOC ()
{
  if (recordsAdded)
    message_log(LOG_INFO, "%s: Added %u records", Doctype.c_str(), recordsAdded);
}

/*-
   What:        Given a buffer of MEMO data:
   returns a list of char* to all characters pointing to the TAG

   MEMO Records:
TAG1: ...
.....
TAG2: ...
TAG3: ...
...
....
___________________________

Memo main body



Fields are continued when the line has no tag
For the ___ break we shall require at least four (4) conseq.
_,-,+ or = or a line starting with ':'.
___

-*/
PCHR * MEMODOC::parse_tags (PCHR b, off_t len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING, DONE } State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = (PCHR *)tagBuffer.Want (max_num_tags, sizeof(PCHR));

  for (off_t i = 0; i < len - 4; i++)
    {
      if (b[i] == '\r' || b[i] == '\v')
 	continue; // Skip over
      if (State == DONE)
	break;
      else if (State == HUNTING && (
	 (b[i] == '\n') || (
	 (b[i]   == '_' || b[i]   == '-' || b[i]   == '+' || b[i]   == '=') &&
	 (b[i+1] == '_' || b[i+1] == '-' || b[i+1] == '+' || b[i+1] == '=') &&
	 (b[i+2] == '_' || b[i+2] == '-' || b[i+2] == '+' || b[i+2] == '=') &&
	 (b[i+3] == '_' || b[i+2] == '-' || b[i+3] == '+' || b[i+3] == '='))))
	{
	  b[i] = '\0'; // Empty tag name
	  t[tc++] = &b[i];
	  State = DONE;
	}
      else if (State == HUNTING && !isspace(b[i]))
	{
	  t[tc] = &b[i];
	  State = STARTED;
	}
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '\t'))
	{
	  State = CONTINUING;
	}
      else if ((State == STARTED) && (b[i] == ':'))
	{
	  b[i] = '\0';
	  if ( *(t[tc]) == '\0')
	    State = DONE;
	  else
	    State = CONTINUING;
	  // Expand memory if needed
	  t = (PCHR *)tagBuffer.Expand(++tc, sizeof(PCHR));
	}
      else if ((State == CONTINUING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
    }
  t[tc] = (PCHR) NULL;
  return t;
}


size_t MEMODOC::CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn) const
{
  STRING metaFile (Fn + ".meta");
  if (FileExists(metaFile))
    return CatMetaInfoIntoFile(outFp, metaFile, 0);
  return 0;
}

size_t MEMODOC::CatMetaInfoIntoFile(FILE *outFp, const STRING& Fn, int level) const
{ 
  size_t lines = 0;

  if (outFp && FileExists(Fn))
    {
	FILE *inp = fopen(Fn, "r");
	if (inp)
	  {
	    STRING S;
	    while (S.FGet (inp))
	      {
		S.Pack (); // Remove Duplicate and trailing spaces
		const char *fld = S.c_str();
		if (*fld == '#')
		  {
		    if (strncmp(fld, "#include", 8) == 0)
		      {
			if (level > 50)
			  {
			    message_log (LOG_WARN, "Include depth too deep (%d): '%s' ignored.",
				level, fld);
			    continue;
			  }
			// include another file...
		        fld += 8;
			while (isspace(*fld)) fld++;
			char quote = *fld;
			if (quote == '<' )
			  {
			    quote = '>';
			    fld++;
			  }
			else if (quote == '"' || quote == '\'')
			  {
			    fld++;
			  }
		        char buf[1024];
			int i =  0;
			for (const char *tcp = fld; *tcp && *tcp != quote && i < sizeof(buf)-1; tcp++)
			  {
			    buf[i++] = *tcp;
			  }
			buf[i] = '\0';
			if (i)
			  {
			    STRING include (buf);
			    if (quote == '>')
			      {
				if (!IsAbsoluteFilePath(include) && !ResolveConfigPath(&include))
				  message_log (LOG_ERROR, "%s: Can't resolve '%s'", Doctype.c_str(), include.c_str());
			      }
			    else if (!IsAbsoluteFilePath(include))
			      {
				include.Prepend(AddTrailingSlash(RemoveFileName(Fn)));
			      }
			    lines += CatMetaInfoIntoFile(outFp, include, level++);
			  }
		      }
		  }
		char *tp = (char *)strchr(fld, ':');
		if (tp)
		  {
		   *tp++ = '\0';
		    // Should have at most one space but..
		    while (*tp && isspace(*tp)) tp++; 
		    if (*tp)
		      {
			// Have a line so spit it out
			fprintf(outFp, "%s: %s\n", fld, tp);
			lines++;
		      }
		  }
	      }
	    fclose(inp);
	  }
    }
  else
    message_log (LOG_ERROR, "%s: Can't read '%s'", Doctype.c_str(), Fn.c_str());
  return lines;
}


_VMEMODOC::_VMEMODOC (PIDBOBJ DbParent, const STRING& Name) : MEMODOC (DbParent, Name)
{
  out_ext = "";
  SOURCE_PATH_ELEMENT = "$Path";

  if (DateModifiedField.IsEmpty()) DateModifiedField = SOURCE_PATH_ELEMENT + "-LastModifiedDate";
  if (DateCreatedField.IsEmpty())  DateCreatedField  = SOURCE_PATH_ELEMENT+"-CreationDate";

   if (DbParent)
    {
      DbParent->AddFieldType(SOURCE_PATH_ELEMENT, FIELDTYPE::text);
      DbParent->AddFieldType(SOURCE_PATH_ELEMENT+"-Original", FIELDTYPE::text);
      DbParent->AddFieldType(SOURCE_PATH_ELEMENT+"-WorkingDirectory", FIELDTYPE::text);
      DbParent->AddFieldType(DateCreatedField, FIELDTYPE::date);
      DbParent->AddFieldType(DateModifiedField, FIELDTYPE::date);
    }
}

GDT_BOOLEAN _VMEMODOC::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  if (SOURCE_PATH_ELEMENT.IsEmpty())
    return PTEXT::GetResourcePath(ResultRecord, StringBuffer);

  STRING uri;

  Present(ResultRecord, SOURCE_PATH_ELEMENT, NulString, &uri);

  if (uri.GetLength())
    {
      uri.Trim(GDT_TRUE);
      uri.Trim(GDT_FALSE);
    }
  else if (Db && Db->getUseRelativePaths() )
    {
      // Error, probably moved strangely
      STRING key = ResultRecord.GetKey();

      STRING path;
      Db->ComposeDbFn (&path, DbExtCat);
      // <db_ext>.cat/<Hash>/<Key>.memo
      AddTrailingSlash(&path);
      path.Cat (((long)key.CRC16()) % 1000);
      AddTrailingSlash(&path); 

      if (path != AddTrailingSlash(ResultRecord.GetPath()))
	{
	  // Read the first line
	  FILE *fp = fopen( path + key + out_ext, "rb");
	  if (fp)
	    {
	      // The value is just after the field (+2 for ':' and ' ')
	      if (fseek(fp, SOURCE_PATH_ELEMENT.GetLength() + 2, SEEK_SET) == 0)
		{
		  uri.FGet(fp);
		}
	      fclose(fp);
	    }
	}
    }
  if (uri.GetLength() && !IsAbsoluteFilePath(uri))
    uri.Prepend( AddTrailingSlash(Db->GetWorkingDirectory()) );

  if (StringBuffer) *StringBuffer = uri;
  return uri.GetLength() != 0;
}


off_t _VMEMODOC::RunPipe(FILE *fp, const STRING& Source)
{
  if (Filter.GetLength())
    message_log (LOG_ERROR, "%s: Undefined RunPipe Method: Can't process '%s'", Doctype.c_str(),
	Source.c_str());
  return 0;
}

void _VMEMODOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.IsEmpty())
    return; // Do nothing

  STRING        key, s, outfile, pdffile;
 
  const STRING  Fn (FileRecord.GetFullFileName ());

  INODE Inode(Fn);
  if (!Inode.Ok())
    {
      if (Inode.isDangling())
        message_log(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
      else
        message_log(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
  if (Inode.st_size == 0)
    {
      message_log(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", Fn.c_str());
      return;
    }
  message_log (LOG_DEBUG, "%s: Input = '%s'", Doctype.c_str(), Fn.c_str());
       
  off_t start = FileRecord.GetRecordStart();
  off_t end   = FileRecord.GetRecordEnd();
       
  if ((end == 0) || (end > Inode.st_size) ) end = Inode.st_size;
       
  if ((key = FileRecord.GetKey()).IsEmpty())
    key = Inode.Key(start, end); // Get Key
       
  for (int i=1; Db->KeyLookup (key); i++)
    key.form("%s.%04x", s.c_str(), i);
  // Now we have a good key

  message_log (LOG_DEBUG, "Key set to '%s'", key.c_str());

  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1)
    {
      message_log (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return;
    }
  // <db_ext>.cat/<Hash>/<Key>.memo
  outfile =  AddTrailingSlash(s);
  outfile.Cat (((long)key.CRC16()) % 1000);
  if (MkDir(outfile, 0, GDT_TRUE) == -1)
    outfile = s; // Can't make it
  AddTrailingSlash(&outfile);
  outfile.Cat (key);

  if (out_ext && out_ext[0]) outfile.Cat (out_ext);

  FILE *fp;

  if ((fp = fopen(outfile, "w")) == NULL)
   {
     message_log (LOG_ERRNO, "%s: Could not create '%s'", Doctype.c_str(), outfile.c_str());
     return;
   }

  const STRING source ( ((Db && Db->getUseRelativePaths()) ? Db->RelativizePathname(Fn) : Fn) );

  fprintf(fp, "%s: %s\n", SOURCE_PATH_ELEMENT.c_str(), source.c_str());
  if (Db && Db->getUseRelativePaths())
    {
      fprintf(fp, "%s-Original: %s\n", SOURCE_PATH_ELEMENT.c_str(), Fn.c_str());
      fprintf(fp, "%s-WorkingDirectory: %s\n", SOURCE_PATH_ELEMENT.c_str(), Db->GetWorkingDirectory().c_str());
    }
  fprintf(fp, "%s: %s\n",  DateModifiedField.c_str(), Inode.mdate.ISOdate().c_str());
  fprintf(fp, "%s: %s\n",   DateCreatedField.c_str(), Inode.cdate.ISOdate().c_str());

  CatMetaInfoIntoFile(fp, Fn); // Add extra metainfo

  off_t  bytes = RunPipe(fp, source);
  off_t  len   = ftell(fp);
  fclose(fp);

  if (bytes <= 5) // Want at least 5 bytes from the pipe
    {
      unlink(outfile);
      if (bytes >= 0)
	message_log (LOG_WARN, "%s: Skipping '%s': Contained %d chars text?", Doctype.c_str(),
		source.c_str(), bytes);
      return;
    }

  RECORD NewRecord(FileRecord);
  // We now have a record in outfile from 0 to len
  NewRecord.SetRecordStart (0);
  NewRecord.SetRecordEnd ( len  - 1 );

  NewRecord.SetFullFileName ( outfile );
  NewRecord.SetKey( key ); // Set the key since we did the effort

  NewRecord.SetOrigPathname ( Fn ); //

  // Set some default dates
  SRCH_DATE mod_filter, mod_input;
  if (mod_filter.SetTimeOfFile(Filter) && mod_input.SetTimeOfFile(Fn) && mod_filter > mod_input)
    NewRecord.SetDateModified ( mod_filter );
  NewRecord.SetDate ( mod_input );

  Db->DocTypeAddRecord(NewRecord);
}


void _VMEMODOC::Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  if (Db && Db->getUseRelativePaths() )
    {
      const STRING Fn ( ResultRecord.GetFullFileName());
      if (!FileExists(Fn))
	{
	  // Error, probably moved strangely
	  STRING key = ResultRecord.GetKey();
	  STRING path;
	  Db->ComposeDbFn (&path, DbExtCat);
	  // <db_ext>.cat/<Hash>/<Key>.memo
	  AddTrailingSlash(&path);
	  path.Cat (((long)key.CRC16()) % 1000);
	  AddTrailingSlash(&path);
	  if (path != AddTrailingSlash(ResultRecord.GetPath()))
	    {
	      RESULT newResult (ResultRecord);
	      newResult.SetPath( path );
	      MEMODOC::Present (newResult, ElementSet, RecordSyntax, StringBuffer);
	      return;
	    }
	}
    }
  MEMODOC::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



void _VMEMODOC:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      STRING source;  
      // Read here the source file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      if (GetResourcePath(ResultRecord, &source) && GetFileSize(source) > 10)
        StringBuffer->CatFile( source );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the converted file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   MEMODOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



