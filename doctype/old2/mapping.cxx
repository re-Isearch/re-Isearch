
MAPPING::MAPPING(PDOCTYPE Parent, const STRING& Doctype)
{
  STRING name;
  name = Doctype;
  name.ToUpper();
  TagRegistry = new REGISTRY (name);
  // Read doctype options
  STRING S;
  STRLIST StrList;
  Parent->Db->GetDocTypeOptions (&StrList);
  if (StrList.GetValue ("CONFIG-FILE", &S) == GDT_FALSE)
    {
      if (name.GetLength())
	{
	  // for HTML look int /opt/BSN/conf/html.ini
	  name.ToLower();
	  S.form("/opt/BSN/conf/%s.ini", (const char *)name);
	}
    }
  if (S.GetLength())
    useBuiltin = (TagRegistry->ProfileLoadFromFile (S) == 0);
  else
    useBuiltin = GDT_TRUE;
}

// Get the field name...
GDT_BOOLEAN MAPPING::UnifiedName (PSTRING StringBuffer, const STRING& name)
{
  return Name(StringBuffer, name, "MAPPINGS");
}

// Get the "Fancy" name for presentation
GDT_BOOLEAN MAPPING::DescriptiveName (PSTRING StringBuffer, const STRING& name)
{
  STRING Temp;
  STRINGINDEX len = Name(&Temp, name, "PRESENTATION");
  STRINGINDEX pos;
#ifndef ATTRIB_SEP
# define ATTRIB_SEP '@'
#endif
  const CHR Sep = ATTRIB_SEP; // Magic Seperator
  // magic sep. from SGML documents types

  if (len && (Temp.Search(' ') == 0)
    {
      if ((pos = Temp.Search(Sep)) != 0)
	{
	  // Have a foo:bar type fieldname
	  // --- map to <foo bar= ...> to
	  // distinguish it from <fop>...</foo>
	  *StringBuffer = "<";
	  *StringBuffer->Cat(Temp);
	  StringBuffer->Replace(Sep, " ");
	  if (pos < len)
	    StringBuffer->Cat("= ");
	  *StringBuffer->Cat("...>");
	}
      else if ((pos = Temp.Search('(')) != 0 && Temp.Search(')') != 0)
	{
	  // Rewrite "XXX(YYY)" to "XXXX (Schema=YYY)"
	  Temp.Replace('(', " (Schema="); // ))
	  *StringBuffer = Temp;
	}
      else
	*StringBuffer = Temp;
    }
  else if (len != 0)
    {
      *StringBuffer = Temp;
    }
  else
    {
      // Use name and build...
      Temp = name;
      len = Temp.GetLength();
      *StringBuffer = ""; // Zero buffer
      GDT_BOOLEAN use_upper = GDT_TRUE;
      for (STRINGINDEX x = 1; x <= len; x++)
	{
	  CHR Ch = Temp.GetChr(x);
	  if (Ch == '-' || Ch == '_')
	    {
	      StringBuffer->Cat (' ');
	      use_upper = GDT_TRUE;
	    }
	  else if (use_upper)
	    {
	      StringBuffer->Cat ((CHR)(toupper (Ch)));
	      use_upper = GDT_FALSE;
	    }
	  else
	    {
	      StringBuffer->Cat ((CHR)(tolower (Ch)));
	    }
	}
    }
  return len != 0;
}


GDT_BOOLEAN MAPPING::Name (PSTRING StringBuffer,
  const STRING& name, const STRING& What)
{
  if (useBuiltin)
    {
      *StringBuffer = name;
    }
  else
    {
      // An entry *=* means use original name
      // If the line is not defined them drop..
      TagRegistry->ProfileGetString (What, name, "*", &StringBuffer);
      if (StringBuffer->Equals("*"))
	{
	  TagRegistry->ProfileGetString (What, "*", "", StringBuffer)
	}
      if (StringBuffer->Equals("*"))
	{
	  *StringBuffer = name;
	}
    }
  return StringBuffer->GetLength() != 0;
}

MAPPING::~MAPPING ()
{
  useBuiltin = GDT_TRUE;
  if (TagRegistry)
    delete TagRegistry;
}
