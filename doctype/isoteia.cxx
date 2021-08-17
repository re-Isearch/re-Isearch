/*
ISOTEIA Project Metadata
*/


#include <stdlib.h>
#include <ctype.h>
#include "platform.h"
#include "common.hxx"
#include "gilsxml.hxx"
#include "doc_conf.hxx"
#include "gpolyfield.hxx"
#include "bboxfield.hxx"

static const STRING  cdate ("Date-of-Last-Review");
static const STRING  mdate ("Date-of-Last-Modification");

const char *date_fields[] = {
 "Beginning-Date",
 "Ending-Date",
 "Time-Textual",
 "Date-of-Last-Modification",
 "Date-of-Last-Review",
 NULL
};

const char *daterange_fields[] = {
 "Time-Structured",
 "Hours-of-Service",
  NULL
};

const char *numerical_fields[] = {
 "Number-of-Records",
 "Record-Size",
 "Message-Size",
 "West", 
 "East",
 "North",
 "South",
 NULL
};

const char *bbox_fields[] = {
  "Bounding-Coordinates",
  NULL
};

static const char myDescription[] = "ISOTEIA project (GILS Metadata) XML format locator records";

static const char myOptions[] = "\
Note: the document types can be extexted to include new fields via their markup as:\n\
<tag type=\"<field_type>\">...</tag> where <field_type> is one of the supported internal field\n\
types such as: numerical, computed, range, date, date-range, box, gpoly etc. (by default all fields\n\
as also treated as \"text\").\n\n\
Example:\n\
 <Date-of-Publication type=\"date\">.....</Date-of-Publication>\n\n\
In the case of \"box\", however, the ISOTEIA format will expect West[...], North[...],\n\
East[...] and South[...] subelements. For date range too it will expect S[tart...] and\n\
E[nd...] subelements.\n\n";


class IBDOC_ISOTEIA : public GILSXML {
public:
  IBDOC_ISOTEIA (PIDBOBJ DbParent, const STRING& Name) : GILSXML (DbParent, Name) {

    message_log (LOG_DEBUG, "IBDOC_ISOTEIA Creation for %s", Name.c_str());

    DateCreatedField  = cdate;
    DateModifiedField = mdate;
    LanguageField     = "language";
    if (KeyField.IsEmpty())
      KeyField = "Control-Identifier";

    if (DbParent)
      {
	STRING   section("BOUNDING-BOX");
	STRING   field, fieldname;
	const char * const directions[] = {"NORTH", "EAST", "WEST", "SOUTH"};

	for (size_t i = 0; i< sizeof(directions)/sizeof(directions[0]); i++)
	  {
	    field = directions[i];
            fieldname = DOCTYPE::Getoption(field);
	    if (fieldname.IsEmpty())
	      fieldname = DbParent->ProfileGetString(section, field);
	    if (fieldname.IsEmpty())
	      DbParent->ProfileWriteString(section, field, field);
	  }
      }	

    if (help.IsEmpty())
      help << myDescription << "\n\n" << myOptions << "\n";
    message_log (LOG_DEBUG, "ISOTEIA: plugin '%s' created", Name.c_str());
  }

  virtual void LoadFieldTable() {
    message_log (LOG_DEBUG, "ISOTEIA: LoadFieldTable()");
    if (Db)
      {
	size_t i;
	for (i=0; date_fields[i]; i++)
	  Db->AddFieldType(date_fields[i], FIELDTYPE::date);
	for (i=0; numerical_fields[i]; i++)
	  Db->AddFieldType(numerical_fields[i], FIELDTYPE::numerical);
	for (i=0; daterange_fields[i]; i++)
	  Db->AddFieldType(daterange_fields[i], FIELDTYPE::daterange);
	for (i=0; bbox_fields[i]; i++)
	  Db->AddFieldType(bbox_fields[i], FIELDTYPE::box);
      }
    DOCTYPE::LoadFieldTable();
  }

  virtual void ParseFields (PRECORD NewRecord) {
     GILSXML::ParseFields(NewRecord);
  }

  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const {
    if (ElementSet.Equals(BRIEF_MAGIC))
      {
        if (Db && Db->GetFieldData (ResultRecord, "title", StringBuffer, this))
  	return;
      }
    return XMLBASE::Present(ResultRecord, ElementSet, StringBuffer);
   }

  ~IBDOC_ISOTEIA() { }

  virtual const char *Description(PSTRLIST List) const {
    List->AddEntry ("ISOTEIA");

    GILSXML::Description(List);
    return help.c_str();
  }

  DATERANGE  ParseDateRange(const STRING& Buffer) const;
  int        ParseGPoly(const STRING&, GPOLYFLD*) const;
  int        ParseBBox(const STRING& Buffer, BBOXFLD *fld) const;
private:
  STRING     help;
};

/*
 <Beginning-Date><!-- Starting Date/Time --></Beginning-Date>
    <Ending-Date><!-- Ending Date/Time --></Ending-Date>
*/

DATERANGE  IBDOC_ISOTEIA::ParseDateRange(const STRING& Buffer) const
{
  SRCH_DATE Start;
  SRCH_DATE End;
  STRING Content = STRING(Buffer).XMLCommentStrip();
  /* Now look for Beginn... */
  if (Content.IsEmpty())
    {
      return DATERANGE();
    } 
  STRING value; 
  const char *ptr = Content.c_str();
  while (*ptr)
   {
      if (*ptr == '<')
	{
	  char    tag[64];
	  size_t  count = 0;

	  while (*++ptr == ' ') /* loop */;

	  // <? Declaration ?> ?
	  if (*ptr == '?') {
	    // Look for ?>
	    while (*ptr && *ptr != '>' && *(ptr-1) != '?')
	      ptr++;
	    continue;
	  }
	  // <!-- comment  --> ?
	  if (*ptr == '!' && *(ptr+1) == '-' && *(ptr+2) == '-') {
	    // comment, look for ->
	    while (*ptr && *ptr != '>' && *(ptr-1) != '-' && *(ptr-2) != '-')
	      ptr++;
	    continue; 
	  }
	  if ((*ptr == '/' ) /* End-tag */ || (*ptr == '!') ) {
	    while (*ptr && *ptr != '>')
	      ptr++;
	    continue;
	  }
	  tag[count++] = *ptr++;
	  while (*ptr && *ptr != '>' && count < sizeof(tag)/sizeof(char) -1)
	    tag[count++] = *ptr++; // Skip to end
	  tag[count] = '\0';
	  if (*ptr) ptr++; // pointing now at first octet after >

	  while (*ptr == ' ') ptr++; // skip white space
	  if (*ptr == '<')
	    {
	      continue; // Empty field
	    }
	  /* Zap < in field */
	  value.Clear();
	  while (*ptr && ptr[1] != '<' && ptr[1] != '>')
	    value.Cat (*ptr++);
	  value.Pack(); // Remove redundant space
	  if (!value.IsEmpty()) // Don't bother with dates on empty fields
	   switch (tag[0]) {
	    case 'B': case 'b':
	      /* Beginning-Date */
	      Start.Set( value );
	      if (!Start.Ok())
		message_log (LOG_WARN, "%s: DateRange Subelement %s contained non-parseable starting date: %s",
			Doctype.c_str(), tag, value.c_str());
	      //message_log (LOG_INFO, "%s = %s", tag, Start.RFCdate().c_str());
	      break;

	    case 'E': case 'e':
	      /* Ending-Date */
	      End.Set (value);
	      if (!End.Ok())
		message_log (LOG_WARN, "%s: DateRange Subelement %s contained non-parseable ending date: %s",
			Doctype.c_str(), tag, value.c_str());
		//message_log (LOG_INFO, "%s = %s", tag, End.RFCdate().c_str());
	      break;

	    default:
	      message_log (LOG_ERROR, "%s: Unknown tag '%s'", Doctype.c_str(), tag);
	      break;
	   }
	}
      ptr++;
   }
 if (End > Start)
   return DATERANGE(Start, End);
  return DATERANGE();
}

// Handy routine to make sure we always get back a numeric value where
// appropriate and where expected.  It correctly handles the (obviously)
// non-numeric value UNKNOWN by sending back a 0.
static GDT_BOOLEAN GetNumericValue(const STRING& Buffer, const STRING& FieldName, DOUBLE *val)
{
  GDT_BOOLEAN result = GDT_FALSE;
  STRING Hold (Buffer);
  STRING sTag;

  // We support right truncated tags.. so <WEST matches <WESTBL
  sTag << "<"  << FieldName; // Start

  STRINGINDEX tag_start = Buffer.SearchAny(sTag); // Case Independent search
  *val = 0;

  if (tag_start > 0)
    {
      STRINGINDEX pos = tag_start;
      CHR Ch;
      STRING eTag ("</");
      // Have something
      pos += sTag.GetLength();
      while ((Ch = Hold.GetChr(pos++)) != '>')
	sTag.Cat(Ch); // Build the whole tag.
      sTag.Cat(Ch);
      eTag << Hold(tag_start,pos-tag_start-1);
      Hold.EraseBefore(pos);
      // End tag is case dependent!
      if ((pos = Hold.Search(eTag)) > 0)
	{
	  Hold.EraseAfter(pos-1);
	  Hold.Pack();
	  if (Hold.IsNumber())
	    {
	      *val = Hold;
	      result = GDT_TRUE;
	    }
	  if (Hold ^= "UNKNOWN")
	    result = GDT_TRUE;
	}
    }
  return(result);
}


/*
 <Bounding-Coordinates>
    <West> </West>
    <East> </East>
    <North></North>
     <South> </South>
</Bounding-Coordinates>
*/

int  IBDOC_ISOTEIA::ParseGPoly(const STRING& Buffer, GPOLYFLD* fld) const 
{
  BBOXFLD   bbox;
  int       result;
  if ((result = ParseBBox(Buffer, &bbox)) > 0)
    {
      if (fld) *fld = GPOLYFLD(bbox);
    }
 return result;
}



int  IBDOC_ISOTEIA::ParseBBox(const STRING& Buffer, BBOXFLD *fld) const
{
  size_t   count = 0;
  DOUBLE   value;
  STRING   Content (Buffer);

  Content.XMLCommentStrip();

 if (GetNumericValue(Content, "West", &value))
   {
     if (fld) fld->setWest(value);
     count++;
   }
 if (GetNumericValue(Content, "North", &value))
   {
     if (fld) fld->setNorth(value);
     count++;
   }
 if (GetNumericValue(Content, "East", &value))
   {
     if (fld) fld->setEast(value);
     count++;
   }
 if (GetNumericValue(Content, "South", &value))
   {
     if (fld) fld->setSouth(value);
     count++;
   }
  return count;
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_ISOTEIA *  __plugin_isoteia_create (IDBOBJ * parent, const STRING& Name) {
    message_log (LOG_INFO, "Creating '%s' DOCTYPE class from plugin", Name.c_str());
    return new IBDOC_ISOTEIA (parent, Name);
  }
  int          __plugin_isoteia_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_isoteia_query (void) { return myDescription; }
}

