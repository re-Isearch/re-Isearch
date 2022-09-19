#pragma ident  "@(#)dvbline.cxx	1.37 05/08/01 21:49:16 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		dvbline.cxx
Version:	1.00
Description:	Class DVBLINE -  DVB Filmline 1.x Variant Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

#include <ctype.h>
#include "common.hxx"
#include "dvbline.hxx"
#include "doc_conf.hxx"

/* Filmline (Only Present is different from MEDLINE) */

DVBLINE::DVBLINE (PIDBOBJ DbParent, const STRING& Name):
	MEDLINE (DbParent, Name)
{
  if (DateField.IsEmpty()) DateField = "ZR";
  if (KeyField.IsEmpty())  KeyField  = "SI";
}

const char *DVBLINE::Description(PSTRLIST List) const
{
  List->AddEntry ("DVBLINE");
  MEDLINE::Description(List);
  return "DVB Filmline 1.x Variant Document format.";
}


void DVBLINE::SourceMIMEContent (PSTRING StringPtr) const
{
  // MIME/HTTP Content type for DVBline Sources 
  *StringPtr = "Application/X-DVBline";
}


// Hooks into the Field parser from Medline
INT DVBLINE::UnifiedNames (const STRING& Tag, PSTRLIST Value) const
{
 const TagTable_t Table[] = {
  /* Sorted List! */
/* NOTE: No white space and only Alphanumeric ASCII characters plus
the characers - and . are allowed. */
  /* Sorted List! */
{"AB", "Kurzinhalt"},		/* abstract */
{"AD", "Adressaten"},		/* audience */
{"AU", "Drehbuch"},		/* author */
{"AV", "+Verfuegbarkeitszeitraum"},		/* availability */
{"AW", "+Praedikate"},		/* awards */
{"CA", "+Titelbild"},
{"CG", "Kamera"},		/* cinematographer */
{"CL", "+Verleiherschluessel"},		/* clue - not official DVB */
{"CO", "Produktionsland"},		/* country */
{"CR", "+Critics"},		/* not official DVB */
{"CS", "+Character_set"},		/* not official DVB */
{"DA", "Datum_Erfassung"},		/* date_collected */
{"DE", "+Kurzbeschreibung_Bildliste"},		/* description */
{"DI", "Regie"},		/* director */
{"DK", "Begleitmaterial"},		/* documentation */
{"DT", "Kurzform_des_Vertriebs"},		/* distribution */
{"ED", "Schnitt"},		/* editor */
{"EF", "+Gestaltungselemente"},		/* effects */
{"ES", "+Datum_Verleihende"},		/* best_before */
{"FM", "+Laenge_Format_Durchmesser"},		/* format */
{"FP", "+Preis"},		/* fee_purchase */
{"FR", "+Gebuehr"},		/* fee_rent, not official DVB */
{"GE", "+Genre"},		/* genre, not official DVB */
{"GM", "+Rechte"},		/* gema */
{"I0", "+Status"},		/* LMZ-Status */
{"I1", "+Erschliesser"},		/* record_author */
{"I2", "+Dezentrale_Signatur"},		/* local_signature */
{"I4", "+Zustaendigkeit"},		/* record_technical_author */
{"I5", "Paralleltitel"},		/* parallel_title */
{"I6", "Produzent_Auftraggeber"},		/* agency */
{"I8", "FWU_Signatur"},		/* FWU-call-id */
{"IA", "+Verweis_FWU_Signatur"},		/* linkto_FWU_call-id */
{"IB", "Bemerkungen"},		/* remarks */
{"IC", "Anzahl"},		/* size */
{"IE", "Hintergrunddokument"},		/* external_links_public */
{"IF", "+Hintergrunddokument_geheim"},		/* external_links_secret */
{"IG", "Weitere_Urheber"},		/* writers */
{"IH", "+Originaltitel_des_2_Einzeltitels"},/* original_alternativ_title */
{"IJ", "+Untertitel_zum_Serientitel"},	/* subtitle_series */
{"IK", "+Jahr_der_Endabnahme"},		/* year_of_baba_yaga */
{"IL", "+Behandelte_Personen_und_Institutionen"},		/* themes */
{"IM", "+Topographische_Begriffe"},		/* geographical_issues */
{"IN", "+Lernziele"},		/* paed_issues */
{"IO", "+Vorkenntnisse"},		/* propaedeuticum */
{"IU", "+Sortiertitel_des_2_Einzeltitels"},	/* paranoia */
{"IV", "+Beigaben_Lieferumfang"},		/* documentation */
{"KW", "Schlagwoerter"},		/* keywords */
{"LA", "Sprache"},		/* code-language */
{"LC", "+Oeffentliche_Vorfuehrrechte"},		/* license */
{"LN", "Laufzeit"},		/* length */
{"LO", "Standort"},		/* location */
{"LT", "Literarische_Vorlage"},		/* lit_author */
{"MA", "+Sammelfeld"},		/* media_attributes */
{"MD", "Farbe"},		/* color */
{"MM", "Datentraeger"},		/* media */
{"MO", "Multimediaobjekte"},
{"MU", "+Musik"},		/* music, not official DVB */
{"NA", "+Thesaurus_MOTBIS"},		/* thesaurus-motbis, not official DVB */
{"NB", "+Thesaurus_TEE"},		/* thesaurus-tee, not official DVB */
{"OM", "+Systemvoraussetzungen"},		/* operating_system */
{"OW", "+Owner_DT_Subsatz"},		/* owner_media_subrecord */
{"PO", "Verweis_Sammelmedien"},		/* parent */
{"PR", "Produzent_Realisator"},		/* producer */
{"PS", "+Beurteilung"},		/* commentary */
{"PU", "Herausgeber"},		/* publisher */
{"RA", "FSK_Freigabevermerk"},		/* ratings */
{"SA", "+Sammlung"},		/* collections, not official DVB */
{"SD", "Ton"},		/* sound */
{"SE", "Serientitel"},		/* series */
{"SH", "+Sortierhaupttitel"},		/* sort_title */
{"SI", "ID_Nummer"},		/* local-call-id */
{"SL", "Verweis_Sprachfassungen"},	/* see also languages */
{"SM", "Verweis_Einzelmedien"},	/* see also media */
{"SO", "Verweis_Kontextmedien"},	/* see also */
{"SR", "+Originaltitel_der_Serie"},	/* series_original */
{"SS", "+Sortierserientitel"},		/* series_sort_title */
{"ST", "+Mitwirkende"},		/* other-credits, not official DVB */
{"SU", "+Sortieruntertitel"},	/* sort_subtitel, not official DVB */
{"SY", "Systematik"},		/* systematic */
{"TA", "+2_Einzeltitel_bei_Mehrfachtiteln"},		/* second_title */
{"TD", "Sonstige_Angaben"},		/* technical_data */
{"TE", "Haupttitel"},		/* title-uniform */
{"TI", "+Wahrer-Titel"},		/* title-cover, not official DVB */
{"TO", "Originaltitel"},		/* title-parallel */
{"TU", "Untertitel"},		/* title-caption */
{"UA", "+Klassifik-UDC"},		/* not official DVB */
{"UB", "+Klassifik-Dewey"},		/* not official DVB */
{"UL", "Verweis_Webseite"},
{"VL", "+Volume"},		/* volume, not official DVB*/
{"VT", "+Synchronisation"},		/* sync, not official DVB */
{"YR", "Produktionsjahr"},		/* date-of-publication */
{"ZC", "+ILL"},		/* not official DVB */
{"ZL", "+USMarc.lang"},		/* not official DVB */
{"ZR", "Datum_letzte_Aenderung"},		/* date-last-modified */
{"ZU", "+URL"},		/* URL, not official DVB */
{"ZZ", "+Verschiedenes"}		/* misc, not official DVB */
  /* NO NULL please! */
  };
  return MEDLINE::UnifiedNames (Table, sizeof(Table)/sizeof(TagTable_t), Tag, Value);
}

#if 0
const CHR *DVBLINE::UnifiedField (const CHR *Tag) const
{
/*
ID-Nummer			Local number (12)
Status				Record-status (1046)
                                Date-record-status (1055)
Erschlie=DFer			Code--institution (56)
Datum Erfassung			Date/time added to database (1011)
Datum letzte =C4nderung		Date/time last modified (1012)
Zustaendigkeit			Code--institution (56)
Haupttitel			Title (4)
Untertitel			Title (4), Title other variant (41)
Serientitel			Title (4), Title series (5)
Paralleltitel			Title (4), Title parallel (35)
Originaltitel			Title (4), Title other variant (41)
Sortierhaupttitel		Title expanded (44)
Sortierserientitel		Title expanded (44)
Sprache				Code--language (54)
Regie				Name (1002)
Drehbuch			Name (1002)
Kamera				Name (1002)
Literarische Vorlage		Name (1002)
Schnitt				Name (1002)
Ton				Name (1002)
Weitere Urheber			Name (1002)
Produktionsland			-
Produktionsjahr			Date (30)
Produzent/Auftraggeber		Name (1002)
Produzent/Realisator		Name (1002)
FSK-Freigabevermerk		-
Adressaten			-
Schlagwoerter			Local subject index (29)
Systematik			Local classification (20)
Kurzinhalt			Abstract (62)
Bemerkungen			Note (63)
Verweis Einzelmedien		Control-number-linking (1049)
Verweis Sammelmedien		Control-number-linking (1049)
Verweis Sprachfassungen		Control-number-linking (1049)
Verweis Kontextmedien		Control-number-linking (1049)
Datentr=E4ger			Material type (1031)
Anzahl				Authority/format identifier (1013)
Laufzeit			Authority/format identifier (1013)
Farbe				Authority/format identifier (1013)
Sonstige Angaben		Authority/format identifier (1013)
Begleitmaterial			Authority/format identifier (1013)
Herausgeber			Publisher (1018)
Kurzform des Vertriebs		Authority/format identifier (1013)
FWU-Signatur			Authority/format identifier (1013)
Standort			Number local call (53)
*/
  const CHR *Title = "Titel";
  const CHR *Creator = "Urheber";

  if (Tag == NULL)
    return (PCHR)Title;

  // Title Fields
  static struct {
    const CHR *key;
    const CHR *name;
  } Table[] = {
    /* Sorted List! */
    {"AU", Creator},		/* author */
    {"CG", Creator},		/* camera */
    {"DI", Creator},		/* director */
    {"ED", Creator},		/* editor */
    {"I5", Title},		/* parallel_title */
    {"I6", Creator},		/* agency */
    {"IG", Creator},		/* writers */
    {"IH", Title},		/* original_alternativ_title */
    {"IJ", Title},		/* subtitle_series */
    {"LT", Creator},		/* lit_author */
    {"MU", Creator},		/* music */
    {"PR", Creator},		/* producer */
    {"PU", Creator},		/* publisher */
    {"SD", Creator},		/* sound */
    {"SE", Title},		/* series */
    {"SH", Title},		/* sort_title */
    {"SS", Title},		/* series_sort_title */
    {"ST", Creator},		/* other-credits */
    {"SU", Title},		/* sort_subtitel */
    {"TA", Title},		/* second_title */
    {"TE", Title},		/* title-uniform */
    {"TI", Title},		/* title-cover */
    {"TO", Title},		/* title-parallel */
    {"TU", Title},		/* title-caption */
    {"VL", Title} 		/* volume */
    /* NO NULL please! */
  };
  int n=0;
  for (size_t i=0; i < sizeof(Table)/sizeof(Table[0]); i++)
    {
      if (StrCaseCmp(Tag, Table[i].name) == 0)
	return Tag;
      if ((n = strcmp(Tag, Table[i].key)) == 0)
        return (PCHR)Table[i].name; // Return "our" unified name
    }
  return NULL; // Not in list
}
#endif

// If the descriptive name starts with  " " don't publish
STRING& DVBLINE::DescriptiveName(const STRING& FieldName, PSTRING Value) const
{
  const TagTable_t Table[] = {
  /* Sorted List! */
{"AB", "Kurzinhalt"},		/* abstract */
{"AD", "Adressaten"},		/* audience */
{"AU", "Drehbuch"},		/* author */
{"AV", "Datum des Verfügbarkeitszeitraumes"},		/* availability */
{"AW", "Prädikate"},		/* awards */
{"CA", " Titelbild"},
{"CG", "Kamera"},		/* cinematographer */
{"CL", "Verl. Schlüssel"},		/* clue, not official DVB */
{"CO", "Produktionsland"},		/* country */
{"CR", "Critics"},		/* criticts, not official DVB */
{"CS", "Character Set"},		/* charset, not official DVB */
{"DA", "Datum der Erfassung"},		/* date_collected */
{"DE", "Kurzbeschreibung/Bildliste"},		/* description */
{"DI", "Regie"},		/* director */
{"DK", "Begleitmaterial"},		/* documentation */
{"DT", "Vertrieb"},		/* distribution */
{"ED", "Schnitt"},		/* editor */
{"EF", "Gestaltungselemente"},		/* effects */
{"ES", "Datum des empfohlenen Verleihendes"},		/* best_before */
{"FM", "Länge, Format, Durchmesser"},		/* format */
{"FP", "Preis"},		/* fee_purchase */
{"FR", "Gebühr"},		/* fee_rent, not official DVB */
{"GE", "Genre"},		/* genre, not official DVB */
{"GM", "Rechte"},		/* gema */
{"I0", " Status"},		/* LMZ-Status */
{"I1", " Erschliesser"},		/* record_author */
{"I2", " Dezentrale Signatur"},		/* local_signature */
{"I4", " Zuständigkeit"},		/* record_technical_author */
{"I5", "Paralleltitel"},		/* parallel_title */
{"I6", "Produzent/Auftraggeber"},		/* agency */
{"I8", "FWU-Signatur"},		/* FWU-call-id */
{"IA", "Verweis FWU-Signatur"},		/* linkto_FWU_call-id */
{"IB", "Bemerkung"},		/* remarks */
{"IC", "Anzahl"},		/* size */
{"IE", "Hintergrunddokumente öffentlich"},		/* external_links_public */
{"IF", " Hintergrunddokumente geheim"},		/* external_links_secret */
{"IG", "Weitere Urheber"},		/* writers */
{"IH", "Originaltitel des 2. Einzeltitels"},		/* original_alternativ_title */
{"IJ", "Untertitel zum Serientitel"},		/* subtitle_series */
{"IK", " Jahr der Endabnahme"},		/* year_of_baba_yaga */
{"IL", "Behandelte Personen und Institutionen"},		/* themes */
{"IM", "Topographische Begriffe"},		/* geographical_issues */
{"IN", "Lernziele"},		/* paed_issues */
{"IO", "Vorkenntnisse"},		/* propaedeuticum */
{"IU", " Sortiertitel des 2. Einzeltitels"},		/* paranoia */
{"IV", "Beigaben, Lieferumfang"},		/* documentation */
{"KW", "Schlagwörter"},		/* keywords */
{"LA", "Sprache"},		/* code-language */
{"LC", "Öffentliche Vorführrechte"},		/* license */
{"LN", "Laufzeit"},		/* length */
{"LO", "Standort"},		/* location */
{"LT", "Literarische-Vorlage"},		/* lit_author */
{"MA", "Anzahl, Laufzeit, Farbe"},		/* media_attributes */
{"MD", "Farbe"},		/* color */
{"MM", "Datenträger"},		/* media */
{"MO", "Multimediaobjekte"},
{"MU", "Musik"},		/* music, not official DVB */
{"NA", "Thesaurus MOTBIS"},		/* thesaurus-motbis, not official DVB */
{"NB", "Thesaurus TEE"},		/* thesaurus-tee, not official DVB */
{"OM", "Systemvoraussetzungen"},		/* operating_system */
{"OW", " Owner DT-Subsatz"},		/* owner_media_subrecord */
{"PO", "Verweis-Sammelmedien"},		/* parent */
{"PR", "Produzent/Realisator"},		/* producer */
{"PS", "Beurteilung"},		/* commentary */
{"PU", "Herausgeber"},		/* publisher */
{"RA", "FSK-Freigabevermerk"},		/* ratings */
{"SA", "Sammlung"},		/* collections, not official DVB */
{"SD", "Ton"},		/* sound */
{"SE", "Serientitel"},		/* series */
{"SH", " Sortierhaupttitel"},		/* sort_title */
{"SI", "ID-Nummer"},		/* local-call-id */
{"SL", "Verweis-Sprachfassungen"},		/* see also languages */
{"SM", "Verweis-Einzelmedien"},		/* see also media */
{"SO", "Verweis-Kontextmedien"},		/* see also */
{"SR", "Originaltitel der Serie"},		/* series_original */
{"SS", " Sortierserientitel"},		/* series_sort_title */
{"ST", "Mitwirkende"},		/* other-credits, not official DVB */
{"SU", " Sortieruntertitel"},		/* sort_subtitel, not official DVB */
{"SY", "Systematik"},		/* systematic */
{"TA", "2. Einzeltitel bei Mehrfachtiteln"},		/* second_title */
{"TD", "Sonstige Angaben"},		/* technical_data */
{"TE", "Haupttitel"},		/* title-uniform */
{"TI", "Wahrer-Titel"},		/* title-cover, not official DVB */
{"TO", "Originaltitel"},		/* title-parallel */
{"TU", "Untertitel"},		/* title-caption */
{"UA", "UDC Klassifikation"},		/* not official DVB */
{"UB", "Klassifik-Dewey"},		/* not official DVB */
{"UL", "Verweis Webseite"},
{"VL", "Volume"},		/* volume, not official DVB*/
{"VT", "Synchronisation"},		/* sync, not official DVB */
{"YR", "Produktionsjahr"},		/* date-of-publication */
{"ZC", "``Internal-Distribution/Loan'' Code for ILL"},		/* not official DVB */
{"ZL", "Sprache (USMarc)"},		/* not official DVB */
{"ZR", " Datum letzte Änderung"},		/* date-last-modified */
{"ZU", "URL"},		/* URL, not official DVB */
{"ZZ", "Verschiedenes"}		/* misc, not official DVB */
  /* NO NULL please! */
  };

 return MEDLINE::DescriptiveName(Table, sizeof(Table)/sizeof(TagTable_t),
        FieldName, Value);
}

void DVBLINE::
DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  GDT_BOOLEAN ShowAll = (RecordSyntax == DVBHtmlRecordSyntax);
  GDT_BOOLEAN UseHtml = (ShowAll || (RecordSyntax == HtmlRecordSyntax));

  if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      STRING Language ( ResultRecord.GetLanguageCode() );
      STRING Value, Tmp;

      if (UseHtml)
	{
	  HtmlHead (ResultRecord, ElementSet, StringBuffer);
	  STRING title;
	  Present (ResultRecord,
		UnifiedName ("TE", &Tmp), HtmlRecordSyntax, &title);
	  if (title.GetLength())
	    {
	      STRING year;
	      Present (ResultRecord,
		UnifiedName("YR", &Tmp), SutrsRecordSyntax, &year);
	      *StringBuffer << "<H1><CITE>\"" << title << "\"</CITE>";
	      if (year.GetLength())
		*StringBuffer << ", " << year;
	      *StringBuffer << "</H1>\n";
	    }
	  *StringBuffer << "<DL TITLE=\"" << title << "\">\n";
	}

      // DVBline class documents are 1-deep so just collect the fields
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey (&Key);
      GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();
      DFD Dfd;
      STRING FieldName, LongName;
      // Walk through the doc's DFD 
      for (INT i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);
	  MEDLINE::DescriptiveName(Language, FieldName, &LongName);
	  if ((LongName.SubString(1, 3) ^= "Any") && !isalnum(LongName.GetChr(4)))
	    continue;
	  // Is it a published field?
	  if (LongName.GetChr(1) != '+' &&
		(LongName.GetChr(1) != ' ' || ShowAll)) {
	    // Get Value of the field, use parent
	    Present (ResultRecord, FieldName,
		UseHtml ? (STRING)HtmlRecordSyntax : RecordSyntax, &Value); // @@@
	    if (Value != "")
	      {
	        if (UseHtml) StringBuffer->Cat ("<DT><B>");
		if (UseHtml) HtmlCat(LongName, StringBuffer);
		else         StringBuffer->Cat(LongName);
	        *StringBuffer << ": ";
	        if (UseHtml) StringBuffer->Cat ("</B><DD>");
#ifdef CGI_FETCH
		if ((FieldName ^= UnifiedName("IE", &Tmp)) ||
			(FieldName ^= UnifiedName("IF", &Tmp)))
		  {
		    if (UseHtml)
		      {
			STRING RecordKey;
			STRING furl;
			Db->URLfrag(&furl);

			ResultRecord.GetKey(&RecordKey);
			const INT Length = Value.GetLength();
			// Make anchor
			Value = "(<A HREF=\"";
			Value << CGI_FETCH << furl <<
				RecordKey << "+" << FieldName <<
				"\">zur Dokumentation (";
			if (Length > 1024)
			  Value << Length/1024 << " Kb";
			else
			  Value << Length << " bytes";
			Value << ")</A>";
		      }
		    else
		      Value = "Vorhanden";
		  } else
#endif
		if (UseHtml)
		  {
		    // Clean-up 
		    INT e_count, s_count;
		    Value.Replace("{}", " ");
		    if ((e_count = Value.Replace("}", "</CITE>}")) > 0)
		      {
			s_count = Value.Replace("{", "{<CITE>");
			while (s_count - e_count > 0)
			  e_count += Value.Replace("}", "</CITE>}");
		      }
		    if ((e_count = Value.Replace("]", "</DFN>]")) > 0)
		      {
			s_count = Value.Replace("[", "[<DFN>");
			while (s_count - e_count > 0)
			  e_count += Value.Replace("]", "</DFN>]");
		      }
		  }
	        *StringBuffer << Value << "\n";
	      }
	  }
	} /* for */
      if (UseHtml)
	{
	  StringBuffer->Cat ("</DL>"); // End of definition list
	  HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail of document
	}
    }
  else 
    MEDLINE::DocPresent(ResultRecord, ElementSet,
	UseHtml ? (STRING)HtmlRecordSyntax : RecordSyntax, StringBuffer);
}

// TODO:
// DT --> DVBADR
// [ Only when Record Syntax is DVBRecordSyntax
//

// DVBLINE Handler
void DVBLINE::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  GDT_BOOLEAN UseHtml = (RecordSyntax == HtmlRecordSyntax);
  *StringBuffer = "";
  STRING Tmp;
  if (ElementSet.Equals (BRIEF_MAGIC))
    {
      // Brief Headline is "Call-number:Title"
      STRING CallNumber;
      MEDLINE::Present (ResultRecord,
	UnifiedName ("SI", &Tmp), RecordSyntax, &CallNumber);
      if (CallNumber.GetLength () != 0)
	{
	  *StringBuffer << CallNumber << ":";
	}

      // Get a title
      STRING Title;
      MEDLINE::Present (ResultRecord,
	UnifiedName ("TE", &Tmp), RecordSyntax, &Title);
      if (Title.GetLength () == 0)
	{
	  MEDLINE::Present (ResultRecord,
		UnifiedName ("TI", &Tmp), RecordSyntax, &Title);
	  if (Title.GetLength () == 0)
	    {
	      // Should not really happen, heck use Original title
	      MEDLINE::Present (ResultRecord,
		UnifiedName ("TO", &Tmp), RecordSyntax, &Title);
	    }
	}
      if (Title.GetLength () != 0)
	*StringBuffer << Title;

      MEDLINE::Present (ResultRecord,
	UnifiedName ("YR", &Tmp), RecordSyntax, &Title);
      if (Title.GetLength () != 0)
	*StringBuffer << "; " << Title;

      else if (StringBuffer->GetLength () == 0)
	*StringBuffer = "** FEHLER: Korrupt Rekord/Falsches Format **";
    }
  else if ((ElementSet ^= UnifiedName ("IF", &Tmp)) ||
	(ElementSet ^= UnifiedName ("IE", &Tmp)))
    {
      // Background Documentation
      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      // Change Indentation
      if (StringBuffer->GetLength ())
	{
	  // Remove leading space to get presentation right
	  StringBuffer->Replace ("\n      ", "\n");
	  if (UseHtml)
	    {
	      StringBuffer->Insert (1, "<pre>\n");
	      StringBuffer->Cat ("</pre>");
	    }
	}
    }
  else if ((ElementSet ^= UnifiedName ("MO", &Tmp)) ||
	   (ElementSet ^= UnifiedName ("UL", &Tmp)))
    {
      // Raw HTML
      DOCTYPE::Present (ResultRecord, ElementSet, "", &Tmp);
      if (Tmp.GetLength() && Tmp.Search('<') == 0)
	HtmlCat(Tmp, StringBuffer);
      else
	*StringBuffer = Tmp;
    }
  else if (ElementSet ^= UnifiedName ("CA", &Tmp))
    {
      // Cover Art, IMAGE
      STRLIST Images;
      Db->GetFieldData (ResultRecord, ElementSet, &Images);
      const size_t Total = Images.GetTotalEntries();
      for (size_t i=1; i<= Total; i++)
	{
	  Images.GetEntry(i, &Tmp);
	  if (i > 1) *StringBuffer << ", ";
	  if (UseHtml && Tmp.GetLength() &&
		Tmp.Search('<') == 0 && Tmp.Search(' ') == 0)
	    {
	      *StringBuffer << "<IMG ALT=\""
		<< ElementSet << "\" SRC=\"" << Tmp << "\">";	   
	    }
	  else
	    *StringBuffer << Tmp;
	}
    }
#if 1 /* EXPERIMENTAL */
  else if (UseHtml && (
	(ElementSet ^= UnifiedName ("SE", &Tmp)) ||
	(ElementSet ^= UnifiedName ("SR", &Tmp)) ))
    {
      STRING Value;
      MEDLINE::Present (ResultRecord, ElementSet, "", &Value);
      if (Value.GetLength())
	{
	  STRING DBname, Anchor;
	  Db->DbName(&DBname);
	  RemovePath(&DBname);
	  *StringBuffer << "<A TARGET=\"" << ElementSet << "\" \
onMouseOver=\"self.status='Suche " << ElementSet << "'; return true\" \
HREF=\"i.search?DATABASE%3D" << DBname << "/TERM%3D%22" << ElementSet
		<< "/" << URLencode(Value, &Tmp) << "%22\">";
	  HtmlCat(Value, StringBuffer, GDT_FALSE);
	  *StringBuffer << "</A>";
	}
    }
  else if (UseHtml && (ElementSet ^= UnifiedName ("SY", &Tmp)))
    {
      MEDLINE::Present (ResultRecord, ElementSet, "", &Tmp);
      if (Tmp.GetLength())
	{
	  STRING DBname, Anchor;
	  Db->DbName(&DBname);
	  RemovePath(&DBname);
	  Anchor << "<A TARGET=\"" << ElementSet << "\" \
onMouseOver=\"self.status='Suche " << ElementSet << "'; return true\" \
HREF=\"i.search?DATABASE%3D" << DBname
		<< "/TERM%3D%22" << ElementSet << "/";
	  // 080 06 03 [Biologie - Vkologie - Vkosysteme]
	  // Search to ']'
	  UCHR *buf = Tmp.NewUCString();
	  UCHR *tp2, *tp3, *tp = buf;
	  while ((tp2 = (UCHR *)strchr((char *)tp, ']')) != NULL)
	    {
	      if (tp2[1]) *++tp2='\0';
	      tp2++;
	      while (isspace(*tp)) tp++;
	      if ((tp3 = (UCHR *)strchr((char *)tp, '[')) != NULL)
		{
		  if (tp3[-1] == ' ') tp3--;
		  UCHR ch = *tp3;
		  *tp3 = '\0';
		  URLencode(tp, &Tmp);
		  *tp3 = ch;
		}
	      else URLencode(tp, &Tmp);
	      *StringBuffer << Anchor << Tmp << "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>; ";
	      tp = tp2;
            }
	  if (*tp)
	    {
	      HtmlCat(tp, StringBuffer);
	    }
	  delete[]buf;
	}
    }
  else if (UseHtml && (
	(ElementSet ^= UnifiedName ("KW", &Tmp)) ||
	(ElementSet ^= UnifiedName ("DI", &Tmp)) ||
	(ElementSet ^= UnifiedName ("PR", &Tmp)) ||
	(ElementSet ^= UnifiedName ("DT", &Tmp))  ))
    {
      MEDLINE::Present (ResultRecord, ElementSet, "", &Tmp);
      if (Tmp.GetLength())
	{
	  STRING DBname, Anchor;
	  Db->DbName(&DBname);
	  RemovePath(&DBname);
	  Anchor << "<A TARGET=\"" << ElementSet << "\" \
onMouseOver=\"self.status='Suche " << ElementSet << "'; return true\" \
HREF=\"i.search?DATABASE%3D" << DBname
	<< "/TERM%3D%22" << ElementSet << "/";
	  // These are seperated by ';'
	  UCHR *buf = Tmp.NewUCString();
	  UCHR *tp2, *tp = buf;
	  while ((tp2 = (UCHR *)strchr((char *)tp, ';')) != NULL)
	    {
	      *tp2++='\0';
	      while (isspace(*tp)) tp++;
	      *StringBuffer << Anchor << URLencode(tp, &Tmp) << "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>; ";
	      tp = tp2;
	    }
	  if (*tp)
	    {
	      while (isspace(*tp)) tp++;
	      *StringBuffer << Anchor
		<< URLencode(tp, &Tmp)
		<< "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>";
	    }
	  delete[]buf;
	}
    }
#endif		/* EXPERIMENTAL */
  else if (ElementSet ^= UnifiedName ("MM", &Tmp))
    {
      STRING Language ( ResultRecord.GetLanguageCode() );
/*
<TABLE>
<TR><TD>TAG_NAME
</TD><TD>TAG_VALUE
</TD></TR><TR><TD>TAG_NAME
</TD><TD>TAG_VALUE
</TD></TR></TABLE>
*/
#define USE_TABLE 1
      // Special Case for Subtag presentation
      STRLIST Strlist;
      Db->GetFieldData (ResultRecord, ElementSet, &Strlist);
      size_t entries = Strlist.GetTotalEntries ();
      STRING Temp;
      *StringBuffer = "";
      if (entries > 1 && UseHtml)
	StringBuffer->Cat ("<OL>");

      for (size_t i = 1; i <= entries; i++)
	{
	  Strlist.GetEntry (i, &Temp);
	  if (UseHtml && entries > 1)
	    StringBuffer->Cat ("<LI>");

	  STRING LongName;
	  UCHR *buf = Temp.NewUCString ();	// Make Copy

	  GDT_BOOLEAN SawItem = GDT_FALSE;
	  GDT_BOOLEAN SawSpace = GDT_FALSE;
#if USE_TABLE
	  GDT_BOOLEAN first_time = GDT_TRUE;
	  STRING Caption;
#endif
	  for (UCHR * tcp = buf; *tcp; tcp++)
	    {
	      if (*(tcp + 2) == ':')
		{
		  *(tcp + 2) = '\0';
		  MEDLINE::DescriptiveName (Language, tcp, &LongName);
		  tcp += 3;
		  while (*tcp == ' ' || *tcp == '\t')
		    tcp++;
		  if (*tcp && *tcp != '\n')
		    {
		      if (!SawItem)
			{
			  if (UseHtml)
			    {
#if USE_TABLE
			      *StringBuffer << "\n\
<TABLE BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\"><CAPTION>\
<font face=\"Arial,Helvetica\" size=\"+1\">" << Caption << "</font></CAPTION>" ;
#else
			      *StringBuffer << "\n<DL COMPACT=\"COMPACT\">";
#endif
			    }
			  SawItem = GDT_TRUE;
			}
		      if (UseHtml)
			{
#if USE_TABLE
			  if (!first_time)
			    {
			      *StringBuffer << "</TD></TR>" ;
#if 1	/* KLUDGE FOR MULTIPLE MMs in a single MM */
			      if (LongName == "Datenträger")
				{
				  *StringBuffer << "\
<TR><TD COLSPAN=2>&nbsp;</TD></TR>";
				}
#endif
			    }
			  else first_time = GDT_FALSE;
			  *StringBuffer << "<TR>\
<TH VALIGN=\"Top\" ALIGN=\"Left\">";
			  HtmlCat (LongName, StringBuffer);
			  *StringBuffer << ":&nbsp;</TH><TD>";
#else
			  *StringBuffer << "<DT><I>";
			  HtmlCat (LongName, StringBuffer);
			  *StringBuffer << "</I>:<DD> ";
#endif
			}
		      else
			*StringBuffer << "\n\t" << LongName << ": ";
		      tcp--;
		    }
		}
	      else
		{
#if USE_TABLE
		  if (!SawItem && UseHtml)
		    {
		      HtmlCat (*tcp, &Caption);
		    }
		  else
#endif
		  if (isspace (*tcp))
		    {
		      if (!SawSpace)
			*StringBuffer << " ";
		      SawSpace = GDT_TRUE;
		    }
		  else
		    {
		      if (UseHtml)
			HtmlCat (*tcp, StringBuffer);
		      else
			*StringBuffer << (UCHR) (*tcp);
		      SawSpace = GDT_FALSE;
		    }
		}
	    }
	  if (SawItem && UseHtml)
	    {
#if USE_TABLE
	      *StringBuffer << "</TD></TR></TABLE>";
#else
	      *StringBuffer << "</DL>";
#endif
	    }
#if USE_TABLE
	  else if (UseHtml)
	    {
	      *StringBuffer << Caption;
	    }
#endif
	  else
	    *StringBuffer << "\n\t";
	  delete[]buf;
	}
      if (entries > 1 && UseHtml)
	StringBuffer->Cat ("</OL>\n");
    }
  else
    {
      MEDLINE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}

DVBLINE::~DVBLINE ()
{
}


/*-
   What:        Given a buffer of ColonTag data:
   returns a list of char* to all characters pointing to the TAG

   Colon Records:
TAG1: ...
.....
TAG2: ...
TAG3: ...
...
....

1) Fields are continued when the line has no tag
2) Field names may NOT contain white space
3) The space BEFORE field names MAY contain white space
4) Between the field name and the ':' NO white space is
   allowed.

-*/
PCHR *DVBLINE::parse_subtags (PCHR b, off_t len) const
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING } State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = new PCHR[max_num_tags];
  for (off_t i = 0; i < len; i++)
    {
      if (b[i] == '\r' || b[i] == '\v')
 	continue; // Skip over
      if (State == HUNTING && !isspace(b[i]))
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
	  // Expand memory if needed
	  if (++tc == max_num_tags - 1)
	    {
  	      // allocate more space
  	      max_num_tags += TAG_GROW_SIZE;
	      PCHR *New = new PCHR[max_num_tags];
	      if (New == NULL)
		{
		  delete [] t;
		  return NULL; // NO MORE CORE!
		}
	      memcpy(New, t, tc*sizeof(PCHR));
 	      delete [] t;
	      t = New;
	    }
	  State = CONTINUING;
	}
      // Bugfix Steve Ciesluk <cies@gsosun1.gso.uri.edu>
      // was State == CONTINUING 
      else if ((State != HUNTING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
    }
  t[tc] = (PCHR) NULL;
  return t;
}

///////////////// Code to handle the Heirarchical Structure

struct DTAB {
    DFD Dfd;
    FC Fc;
};

static int DfdtCompare (const void *p1, const void *p2)
{
  int dif = ((struct DTAB *) p1)->Fc .GetFieldStart() - ((struct DTAB *) p2)->Fc .GetFieldStart();
  if (dif == 0)
    dif = ((struct DTAB *) p2)->Fc .GetFieldEnd() - ((struct DTAB *) p1)->Fc .GetFieldEnd();
  return dif;
}


GDT_BOOLEAN DVBLINE::GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const
{
  PMDT MainMdt = Db->GetMainMdt ();
  PDFDT MainDfdt = Db->GetMainDfdt ();

  DfdtBuffer->Clear();

  MDTREC Mdtrec;
  if (!MainMdt->GetMdtRecord (Key, &Mdtrec))
    return GDT_FALSE;

  const off_t MdtS = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordStart ();
  const off_t MdtE = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordEnd ();

  FC MdtFc (MdtS, MdtE);

  DFD dfd;
  STRING FieldName, Fn;

  const size_t TotalEntries = MainDfdt->GetTotalEntries ();

  size_t count = 0;
  struct DTAB *Table = new struct DTAB[TotalEntries];

  for (size_t x = 1; x <= TotalEntries; x++)
    {
      MainDfdt->GetEntry (x, &dfd);
      dfd.GetFieldName (&FieldName);
      Db->DfdtGetFileName (FieldName, &Fn);
      PFILE Fp = Fn.Fopen ("rb");
      if (Fp)
	{
	  // Get Total coordinates in file
	  const size_t Total = GetFileSize (Fp) / sizeof (FC);	// used fseek(Fp)

	  if (Total)
	    {
	      INT Low = 0;
	      INT High = (INT) Total - 1;
	      INT X = High / 2;
	      INT OldX;
	      FC Fc;	// Field Coordinates

	      do
		{
		  OldX = X;
		  if (-1 == fseek (Fp, X * sizeof (FC), SEEK_SET))
		    break;	// Error

		  if (Fc.Read (Fp) == GDT_FALSE)
		    {
		      // Read Error
		      if (++X >= Total) X = Total - 1;
		      continue;
		    }
		  if (MdtFc.Compare(Fc) == 0)
		    {
		      Table[count].Dfd = dfd;
		      Table[count++].Fc = Fc;
		      break;	// Done this loop
		    }
		  if (MdtE < Fc.GetFieldEnd())
		    High = X;
		  else
		    Low = X + 1;
		  // Check range
		  if ((X = (Low + High) / 2) < 0)
		    X = 0;
		  else if (X >= Total)
		    X = Total - 1;
		}
	      while (X != OldX);
	    }
	  fclose (Fp);
	}
    }				// for()


  // qsort Table and put into DfdtBuffer so
  // that it represents the "order" in the record
  QSORT (Table, count, sizeof (Table[0]), DfdtCompare);
  // Now add things NOT inside groups
  off_t lastEnd = 0;
  DfdtBuffer->Resize(count+1);
  for (int i=0; i < count; i++)
    {
      if ((lastEnd < Table[i].Fc.GetFieldEnd()) && (Table[i].Fc.GetFieldStart() >= lastEnd)) 
	{
	  DfdtBuffer->FastAddEntry (Table[i].Dfd);
	  lastEnd = Table[i].Fc.GetFieldEnd(); 
	}
    }
  delete[] Table;

  return GDT_TRUE;
}

