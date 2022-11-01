#include "harvest.hxx"
#include "doc_conf.hxx"

HARVEST::HARVEST (PIDBOBJ DbParent, const STRING& Name) :
	SOIF (DbParent, Name)
{ }


const char *HARVEST::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("HARVEST");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  SOIF::Description(List);
  return "SOIF format files and points to resource.";
}

bool HARVEST::GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  Present(ResultRecord, UnifiedName("URL"), "", StringBuffer);
  return StringBuffer->GetLength() != 0;
}

bool HARVEST::URL(const RESULT& ResultRecord, PSTRING StringBuffer,
        bool OnlyRemote) const
{
  if (GetResourcePath(ResultRecord, StringBuffer))
    return true;
  if (OnlyRemote)
    return false;
  return DOCTYPE::URL(ResultRecord, StringBuffer, OnlyRemote);
}



void HARVEST::DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet.Equals (FULLTEXT_MAGIC) && (RecordSyntax == HtmlRecordSyntax))
    {
      STRING Url, tmp;

      Present(ResultRecord, UnifiedName ("URL", &tmp), SutrsRecordSyntax, &Url); 
      if (Url.GetLength())
	{
	  // Redirect
	  StringBuffer->form ("Status-Code: 302\nServer: NONMONOTONIC public re-Isearch 3.0\nLocation: %s\r\n\r\n",
		(const char *)Url);
	  return;
	}
    }
  SOIF::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


HARVEST::~HARVEST () { }
