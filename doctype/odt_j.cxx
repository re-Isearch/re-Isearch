/*@@@
File:		odt_j.cxx
Version:	1.01
Description:	Class ODT Using Java and a ODF jar
Author:		Edward Zimmermann
@@@*/


#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "common.hxx"
#include "filter2.hxx"
#include "doc_conf.hxx"

static const char default_jar[] = "odt-search";
static const char default_java[] = "java";
static const char myDescription[] = "\"OASIS Open Document Format Text\" (ODT) Plugin (MEMODOC)";


class IBDOC_ODT_J : public FILTER2MEMODOC {
public:
   IBDOC_ODT_J(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      FILTER2MEMODOC::Description(List);
      return desc.c_str();
   }

  void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/vnd.oasis.opendocument.text";
  }

   virtual const char *GetDefaultFilter() const { return default_filter;}


   // The default headline is from the field HEADLINE
   virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
                   const STRING& RecordSyntax, PSTRING StringBuffer) const {

     if (ElementSet.Equals(BRIEF_MAGIC)) {
       Present (ResultRecord, "TITLE", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty())
	 Present (ResultRecord, "SUBJECT", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty())
         Present (ResultRecord, "HEADLINE", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty())
         Present (ResultRecord, "DESCRIPTION", RecordSyntax, StringBuffer);

       if (StringBuffer->IsEmpty() && ! GetResourcePath(ResultRecord, StringBuffer))
	 DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
     }
     else DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
   }


   ~IBDOC_ODT_J() { }
private:
   STRING desc;
   STRING default_filter;
};


IBDOC_ODT_J::IBDOC_ODT_J(PIDBOBJ DbParent, const STRING& Name) : FILTER2MEMODOC(DbParent, Name)
{

  STRING jar  = Getoption("JAR", default_jar);
  STRING java = Getoption("JAVA", default_java);
 
  SetFilter(java);
  default_filter =  java;


  if (tagRegistry)
    tagRegistry->ProfileWriteString("Fields", Doctype+"-Body", "Content");

  STRING options = "-jar " + jar;
  STRING args = GetArgs(); 
  if (!args.IsEmpty()) options << " " << args;
  SetArgs( options ) ;
  desc.form("\
%s. Uses an external filter defined in a Java JAR.\nOptions:\n\
   JAVA    Specifies the Java (JVM) to use (Default '%s') \n\
   JAR     Specifies the jar to use (Default '%s')\n\
           Default execution: '%s %s <filename>'\n\
for default Brief record (headline) uses (in order as found) TITLE, SUBJECT, HEADLINE, DESCRIPTION else path\n\n",	   
	   myDescription, java.c_str(), jar.c_str(),
	   java.c_str(), options.c_str());
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_ODT_J *  __plugin_odt_j_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_ODT_J (parent, Name);
  }
  int          __plugin_odt_j_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_odt_j_query (void) { return myDescription; }
}

