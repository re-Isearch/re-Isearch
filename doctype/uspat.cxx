/*@@@
File:		uspat.cc
Version:	1.00
Description:	Class USPAT - Greenbook-style US Patent text
Author:		Jim Fullton, Jim.Fullton@cnidr.org
@@@*/
#include "doctype.hxx"
#include <ctype.h>

/* add miscellaneous PATO defs here.  clean up later */

#define PATO_MAXSTR 256

typedef struct tagPATO_FIELD {
	char ID[6];
	char Data[PATO_MAXSTR];
	struct tagPATO_FIELD* Next;
} PATO_FIELD;

typedef struct tagPATO_GROUP {
	char ID[6];
	PATO_FIELD* Fields;
	struct tagPATO_GROUP* Next;
} PATO_GROUP;

typedef struct tagPATO_PATENT {
	PATO_GROUP* Groups;
} PATO_PATENT;

typedef struct tagPATO_BUFFER {
	int Index;
	char* Data;
	int Size;
} PATO_BUFFER;

typedef struct tagPATO_OUTPUT {
	int Type; /* 0:stdout, 1:file, 2:buffer */
	FILE* fp;
	PATO_BUFFER* Buffer;
} PATO_OUTPUT;


class USPAT : public DOCTYPE {
public:
	char pato_HostName[256];
	char pato_DBName[256];
	//char RecBuffer[2000000];
	PATO_PATENT *Patent;
	USPAT(IDBOBJ* DbParent, const STRING& Name);
        const char *Description(PSTRLIST List) const;
	void AddFieldDefs();
	void ParseRecords(const RECORD& FileRecord);
	void ParseFields(RECORD* NewRecord)  ;
	void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
		const STRING &Rs, STRING* StringBuffer);
	
	~USPAT();
private:
	
	void pato_Cleanup(char* s) const;
	char* pato_item(char* S, int X, char* Item, char Delim)const ;
	int pato_itemCount(char* S, char Delim) const;
	char* RmTrailBlanks(char* S)const ;
	PATO_PATENT* pato_ReadPatent(char* Buffer, int PatentSize)const ;
	char* pato_Get_TI(PATO_PATENT* Patent, char* S, int SSize)const ;
	void CleanTextString(PSTRING StringBuffer)const ;
	PATO_GROUP* pato_FindGroup(PATO_PATENT* Patent, const char* Group)const ;
	PATO_FIELD* pato_FindField(PATO_GROUP* Group, const char* Field)const ;
	char* pato_Get_PN(PATO_PATENT* Patent, char* S, int SSize)const ;
	void pato_DisposePatent(PATO_PATENT* Patent) const;
	void pato_OutputBuffer(PATO_OUTPUT* Output, PATO_BUFFER* Buffer)const ;
	void pato_InitBuffer(PATO_BUFFER* Buffer, int Size) const;
	void pato_FreeBuffer(PATO_BUFFER* Buffer) const;
	char* pato_GetField(PATO_GROUP* Group, const char* Field, char* S) const;
	//void InsertField(PDFT pdft,char* Field, int Start, int End) const;
	void AddCommas(char* S1, char* S2) const;
	void Semi_to_Comma(char* S1) const;
	void pato_Code(char* S, int Code, int Size, int Flag) const;
	void pato_ElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		PSTRING ElementSet, char* Format) const;
	void pato_TextElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* Element) const;
	void pato_HtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* Element) const;
  	void pato_TextHtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* Element) const;
	void PatentPath(char* fn, char* path)const ;
	void space(char* S, int length, PATO_OUTPUT* Output) const;
	void pato_Write(PATO_OUTPUT* Output, const char* S) const;
	void pato_HtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* ElementSet)const ; 
   	void pato_TextHtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* ElementSet)const ; 
	void pato_TextElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
		char* ElementSet)const ;
	void pato_ClassFixup(char* S)const ;
	int pato_WriteBuffer(PATO_BUFFER* Buffer, const char* S)const ;
	int pato_GoodClass(char* Sub)const ;
	void pato_html_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group)const ;
	void pato_text_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group)const ;
	void pato_html_FigLinks(char* Data, char* Buffer)const ;
	void pato_html_Special(char* Data, char* Buffer)const ;
	void pato_html_CloseFormat(PATO_OUTPUT* Output, char* Format)const ;
	void pato_ExpandDate(char* S, char* Buffer) const;
	void pato_html_Continue(PATO_OUTPUT* Output, PATO_FIELD* Field) const;
	char* pato_GetMonth(int x, char* S) const;
	int pato_PA(char* ID)const ;
	void pato_html_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
		int MaxNumNames)const ;
	void pato_text_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart)const ;
	void pato_NameCount(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
			    int MaxNumNames) const;
};

typedef USPAT* PUSPAT;

class GB {
	STRLIST c_field_name_list;
	int32_t 	c_field_name_count,
		c_length;
	char 	*c_record;
	int GetFieldName(const int32_t Start, STRING *Name, int32_t *NextFieldStart);
	void InsertField(PIDBOBJ Db, PDFT pdft, const STRING& Field, int Start, 
		int End) const;
public:
	GB(char *Record, const int32_t Length);
	~GB();

	PDFT BuildDft(PIDBOBJ Db, STRLIST& FieldNames, STRLIST& Hierarchies);
};


//
// You can have this doctype parse any field from greenbook by
// adding the desired field name to this list.  If the field is
// hierarchical, you must include the full path of the field
// name using '-' as a separator.  See the INVT-NAM, INVT-CTY and
// INVT-STA field names below.  Only one level of hierarchy is
// supported.  These field names _MUST_ match the actual name of
// the field as found in the greenbook file.
//
// You will be able to Present based on any of these field names
// as ElementSetNames
//
#define FIELD_NAMES "PATN-TTL,PATN-APN,PATN-APD,PATN-WKU,INVT-NAM,INVT-CTY,INVT-STA,INVT-CNT,ASSG-NAM,ASSG-CTY,ASSG-STA,ASSG-CNT,CLAS-OCL,CLAS-XCL,CLAS-ICL,UREF-*,OREF-*,FREF-*,LREP-*,ABST-*,DCLM-*,BSUM-*,CLMS-*,DETD-*,DRWD-*"

#define HIERARCHIES "PATN,INVT,ASSG,CLAS,UREF,OREF,FREF,LREP,ABST,DCLM,BSUM,CLMS,DETD,DRWD"

static const char *MyDescription = "US Patents (Green Book)";

// Stubs for dynamic loading
extern "C" {
  USPAT *     __plugin_uspat_create (IDBOBJ * parent, const STRING& Name) { return new USPAT (parent, Name); }
  int          __plugin_uspat_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_uspat_query (void) { return MyDescription; }
}

USPAT::USPAT(PIDBOBJ DbParent, const STRING& Name) : DOCTYPE(DbParent, Name) { }

const char *USPAT::Description(PSTRLIST List) const
{
  if (List)
    {
      List->AddEntry ("USPAT");
      DOCTYPE::Description(List);
    }
  return MyDescription;
}


void USPAT::AddFieldDefs()  
{
  DOCTYPE::AddFieldDefs();
}


void USPAT::ParseRecords(const RECORD& FileRecord) 
{
  DOCTYPE::ParseRecords(FileRecord);		
}


void USPAT::ParseFields(PRECORD NewRecord) 
{
  FILE* fp;
  STRING fn;
  GPTYPE RecStart, RecEnd, RecLength, ActualLength;
  PDFT pdft;
  static int xx=0;
	
  NewRecord->GetFullFileName(&fn);
  fp = fopen(fn, "rb");
  if (!fp) {
    perror("Failed to open database file");
    return;
  }

  RecStart = NewRecord->GetRecordStart();
  RecEnd = NewRecord->GetRecordEnd();
  if (RecEnd == 0) {
    fseek(fp, 0, 2);
    RecStart = 0;
    RecEnd = ftell(fp) - 1;
  }
  fseek(fp, RecStart, 0);
  RecLength = RecEnd - RecStart;

  char *RecBuffer;
  RecBuffer = new char[RecLength + 1];
  ActualLength = fread(RecBuffer, 1, RecLength, fp);
  RecBuffer[ActualLength]='\0';
  fclose(fp);

  // Debugging
  ++xx;
  if(xx%100==0)
    printf("%d records\n",xx);
	
  GB *gb;
  gb = new GB(RecBuffer, ActualLength);

  STRLIST FieldNames;
  STRLIST Hierarchies;
  FieldNames.Split(",", FIELD_NAMES);
  Hierarchies.Split(",", HIERARCHIES);
  pdft = gb->BuildDft(Db, FieldNames, Hierarchies);
  /*
    cout << "Document " << xx << " completed." << endl;
  */
  NewRecord->SetDft(*pdft);
  //cout << "DFT set." << endl;
  delete pdft;
  delete gb;

  delete [] RecBuffer;
}


/*
###################################################################
#
#	MODULE:		Present
#	AUTHOR:		Jim Fullton
#	DATE:		9/22/95
#	TYPE:		Public
#	COMMENTS:	General public Present method.  Calls
#	low-level, private present functions for each element set.
#	When PRS is added to the Present function, it will be
#	passed through to the low-level functions.   
#
#
###################################################################
*/
void 
USPAT::Present(const RESULT& ResultRecord, const STRING& ElementSet, 
	const STRING &RecordSyntax, PSTRING StringBuffer) 
{

  // Following case added by NRN to allow Greenbook to be returned for
  // debugging purposes.
  
  if (RecordSyntax == "DEBUG") {
    DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax,
		     StringBuffer);
    return;
  }
  
  
  char* RecBuffer;
  int RecLength;
  PATO_PATENT *ThePatent;
  PATO_BUFFER Buffer;
  PATO_OUTPUT Output;
  char* pcPRS=NULL;
  STRING Before, After;
  
  // these next two lines will go away when DOCTYPE::Present
  // is modified to support Preferred Record Syntax
  
  PSTRING PreferredRecordSyntax= new STRING;
  
  *PreferredRecordSyntax=RecordSyntax;
  pcPRS=PreferredRecordSyntax->NewCString();
  // ================
  
  *StringBuffer = "";
  if(PreferredRecordSyntax->Equals(SutrsRecordSyntax)){
    Before="***";
    After="***";
  }else{
    Before="<B><I>";
    After="</I></B>";
  }
  if(ElementSet.Equals("OF")){
    Before="";
    After="";
  }
  ResultRecord.GetHighlightedRecord(Before, After, StringBuffer);
  // ResultRecord.GetRecordData(StringBuffer);
  // first, load the patent, for all present operations
  
  
  RecBuffer=StringBuffer->NewCString();
  RecLength=StringBuffer->GetLength();
  
  ThePatent = pato_ReadPatent(RecBuffer, RecLength);
  pato_InitBuffer(&Buffer, RecLength+20000);
  pato_OutputBuffer(&Output, &Buffer);
   
  if(ElementSet.Equals("B")){
    char PN[64],TTL[2048],PN2[64];
    char MyString[4096];
    
    pato_Get_TI(ThePatent,TTL,2048);
    pato_Get_PN(ThePatent,PN,64);
    AddCommas(PN,PN2);
    if(PreferredRecordSyntax->Equals(SutrsRecordSyntax))
      sprintf(MyString,"(%s) %s",PN2,TTL);
    else
      sprintf(MyString,"(<B>%s</B>) %s",PN2,TTL);
    *StringBuffer=MyString;
  }else if(ElementSet.Equals("PN")){
    char PN[64];
    pato_Get_PN(ThePatent,PN,64);
    *StringBuffer=PN;
  }else{				// handle all other element sets
    
    STRING xx;
    STRING MyEs=ElementSet;
    xx="F";
    
    if((ElementSet.Equals("OF")) || (ElementSet.Equals("FT")))
      pato_ElementSet(&Output, ThePatent, &xx, pcPRS);
    else
      pato_ElementSet(&Output, ThePatent, &MyEs, pcPRS);
    *StringBuffer=Buffer.Data;
  }
  pato_FreeBuffer(&Buffer);
  pato_DisposePatent(ThePatent);
  delete [] pcPRS;
  delete [] RecBuffer ;
  
}


USPAT::~USPAT() {
}

/*
   ###################################################################
   #
   #	MODULE:		CleanTextString
   #	AUTHOR:		Jim Fullton
   #	DATE:		8/22/95
   #	TYPE:		Private
   #	COMMENTS:	Clean assorted grodiness from a text STRING.
   #	Converts \n\r and \t into space, then compresses multiple
   #	sequential spaces into a single space.
   #
   #
   #
   ###################################################################
   */
void 
USPAT::CleanTextString(PSTRING StringBuffer) const
{
  
  int i,j,Length;
  char *ptr,*Text;
  
  Text=StringBuffer->NewCString();
  
  // convert tabs, newlines and linefeeds to spaces
  
  while((ptr=strpbrk(Text,"\n\r\t"))!=NULL)
    *ptr=' ';
  Length=strlen(Text);
  
  // squash multiple spaces
  
  for(i=0,j=0; i<Length; i++){
    if(Text[i]==' ' && Text[i+1]==' ')
      continue;
    Text[j++]=Text[i];
  }
  Text[j]='\0';
  *StringBuffer=Text;
  delete [] Text;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_Get_TI
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Public
   #	COMMENTS:	Extract the title of the loaded patent into
   #		        a string.
   #
   #
   ###################################################################
   */
char* 
USPAT::pato_Get_TI(PATO_PATENT* Patent, char* S, int SSize) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  char* RetValue = NULL;
  Group = pato_FindGroup(Patent, "PATN");
  if (Group != NULL) {
    Field = pato_FindField(Group, "TTL");
    if (Field != NULL) {
      strncpy(S, Field->Data, SSize-1);
      S[SSize-1] = '\0';
      while ( (Field->Next != NULL) &&
	     (Field->Next->ID[0] == '\0') ) {
	Field = Field->Next;
	strncat(S, Field->Data, SSize - strlen(S) - 1);
	S[SSize-1] = '\0';
      }
      strncat(S, "\n", SSize - strlen(S) - 1);
      S[SSize-1] = '\0';
      RetValue = S;
    }
  }
  return RetValue;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_ReadPatent
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Public
   #	COMMENTS:	Load a US Patent into internal patent
   #			management structures.
   #
   #
   ###################################################################
   */
PATO_PATENT* 
USPAT::pato_ReadPatent(char* Buffer, int PatentSize) const
{
  PATO_PATENT* Patent;
  PATO_GROUP* CurrentGroup;
  char Si[256];
  int Done = 0;
  int Pos = 0;
  int On = 0,j,tLen;
  Patent = NULL;
  CurrentGroup = NULL;
  while ( (Pos < PatentSize) && (!Done) ) {
    
    /* strncpy(Si, Buffer+Pos, 80);  for real greenbook */
    
    
    for(j=0; j<256; j++)
      Si[j]='\0';
    memccpy(Si,Buffer+Pos,(int)'\n',200);
    tLen=strlen(Si);
    
    if (On) {
      RmTrailBlanks(Si);
      if (strlen(Si) <= 5) {
	/* Logical group */
	PATO_GROUP* G;
	G = (PATO_GROUP*)calloc(1, sizeof(PATO_GROUP));
	if (Patent->Groups == NULL) {
	  Patent->Groups = G;
	} else {
	  PATO_GROUP* F;
	  F = Patent->Groups;
	  while (F->Next != NULL)
	    F = F->Next;
	  F->Next = G;
	}
	CurrentGroup = G;
	strcpy(G->ID, Si);
      } else {
	/* Field */
	if (CurrentGroup != NULL) {
	  PATO_FIELD* F;
	  F = (PATO_FIELD*)calloc(1, sizeof(PATO_FIELD));
	  if (CurrentGroup->Fields == NULL) {
	    CurrentGroup->Fields = F;
	  } else {
	    PATO_FIELD* E;
	    E = CurrentGroup->Fields;
	    while (E->Next != NULL)
	      E = E->Next;
	    E->Next = F;
	  }
	  if (strlen(Si) < 5) {
	    strcpy(F->ID, Si);
	    F->Data[0] = '\0';
	  } else {
	    strcpy(F->Data, Si+5);
	    Si[5] = '\0';
	    RmTrailBlanks(Si);
	    strcpy(F->ID, Si);
	  }
	}
      }
      //  Pos += 80;
      /* for fake greenbook  Pos+=tLen;*/
      Pos+=tLen;
    } else {			/* On == 0 */
      if (strncmp(Si, "PATN", 4) == 0) {
	On = 1;
	Patent = (PATO_PATENT*)calloc(1,sizeof(PATO_PATENT));
      }else
	Pos+=strlen(Si);
    }
    
  }
  return Patent;
}


/*
   ###################################################################
   #
   #	MODULE: 	RmTrailBlanks
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Remove blank padding from each patent
   #
   #
   #
   ###################################################################
   */
char* 
USPAT::RmTrailBlanks(char* S) const
{
  char* P;
  
  P=strchr(S,'\n');
  if(P)
    *P='\0';
  P = S + strlen(S) - 1;
  while ( (P >= S) && (*P == ' ') )
    *P-- = '\0';
  return S;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_itemCount
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Count the patent items in a string
   #
   #
   #
   ###################################################################
   */
int 
USPAT::pato_itemCount(char* S, char Delim) const
{
  char* NS;
  int x, c;
  
  if (S == NULL)
    return 0;
  NS = strdup(S);
  if (NS == NULL)
    return 0;
  pato_Cleanup(NS);
  if (NS[0] == '\0')
    return 0;
  c = 1;
  for (x = 0; x < strlen(NS); x++)
    c += (NS[x] == Delim);
  return c;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_Item
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		
   #	TYPE:		Public
   #	COMMENTS:	
   #
   #
   #
   ###################################################################
   */
char* 
USPAT::pato_item(char* S, int X, char* Item, char Delim) const
{
  char* NS;
  char* p;
  char* NSp;
  int c;
  
  if (Item == NULL)
    return NULL;
  Item[0] = '\0';
  if (S == NULL)
    return Item;
  if (X < 1)
    return Item;
  NS = strdup(S);
  if (NS == NULL)
    return Item;
  pato_Cleanup(NS);
  if (NS[0] == '\0')
    return Item;
  c = X - 1;
  NSp = NS;
  while ( (c > 0) && ((p=strchr(NSp, Delim)) != NULL) ) {
    NSp = p + 1;
    c--;
  }
  if (c > 0)
    return Item;
  if ((p=strchr(NSp, Delim)) != NULL)
    *p = '\0';
  strcpy(Item, NSp);
  free(NS);
  return Item;
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_Cleanup
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Strip leading and trailing spaces from
   #			a string
   #
   #
   ###################################################################
   */
void 
USPAT::pato_Cleanup(char* s) const
{
  char* p = s;
  while (*p == ' ')
    p++;
  strcpy(s, p);
  p = s + strlen(s) - 1;
  while ( (p >= s) && (*p == ' ') )
    *p-- = '\0';
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_FindField
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Find a patent field
   #			
   #
   #
   ###################################################################
   */
PATO_FIELD* USPAT::pato_FindField(PATO_GROUP* Group, const char* Field) const
{
  PATO_FIELD* P;
  int Found;
  
  P = Group->Fields;
  Found = 0;
  while ( (P != NULL) && (Found == 0) ) {
    if (strcmp(P->ID, Field) == 0)
      Found = 1;
    else
      P = P->Next;
  }
  return P;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_FindGroup
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Find a patent group
   #			
   #
   #
   ###################################################################
   */
PATO_GROUP* 
USPAT::pato_FindGroup(PATO_PATENT* Patent, const char* Group) const
{
  PATO_GROUP* P;
  int Found;
  P = Patent->Groups;
  Found = 0;
  while ( (P != NULL) && (Found == 0) ) {
    if (strcmp(P->ID, Group) == 0)
      Found = 1;
    else
      P = P->Next;
  }
  return P;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_DisposePatent
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Destroy a patent object
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_DisposePatent(PATO_PATENT* Patent) const
{
  PATO_GROUP* Group;
  PATO_GROUP* NextGroup;
  PATO_FIELD* Field;
  PATO_FIELD* NextField;
  
  if (Patent != NULL) {
    Group = Patent->Groups;
    while (Group != NULL) {
      Field = Group->Fields;
      while (Field != NULL) {
	NextField = Field->Next;
	free(Field);
	Field = NextField;
      }
      NextGroup = Group->Next;
      free(Group);
      Group = NextGroup;
    }
    free(Patent);
    Patent = NULL;
  }
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_Code
   #	AUTHOR:		Glenn MacStravic
   #	DATE:		2/8/96
   #	TYPE:		Private
   #	COMMENTS:	Get string corresponding to code
   #
   ###################################################################
   */
void 
USPAT::pato_Code(char* S, int Code, int Size, int Flag) const
{
  switch (Code) {
  case 1:
    strncpy(S, ", Pat. No. ", Size-1);
    break;
  case 3:
    strncpy(S, ", abandoned", Size-1);
    break;
  case 4:
    strncpy(S, ", now defensive publication no. ", Size-1);
  case 50:
    strncpy(S, "reissue of: ", Size-1);
    break;
  case 51:
    strncpy(S, "which is a reissue of: ", Size-1);
    break;
  case 52:
    strncpy(S, "which is a reissue of: ", Size-1);
    break;
  case 71:
    strncpy(S, "continuation of (including streamline cont.) Ser. No.", Size-1);
    break;
  case 72:
    strncpy(S, "continuation-in-part of Ser No. ", Size-1);
    break;
  case 73:
    strncpy(S, "a substitute for Ser No. ", Size-1);
    break;
  case 74:
    strncpy(S, "division of Ser No. ", Size-1);
    break;
  case 75:
    strncpy(S, ", and a continuation-in-part of Ser No. ", Size-1);
    break;
    
    // codes 75 through 81 are missing from the Greenbook information.  I deduced 75 by
    // looking at an actual patent and comparing it to our data. - gem
    
  case 81:
    if (Flag == 2) 
      strncpy(S, "is a continuation of Ser. No ", Size-1);
    else
      strncpy(S, ", which is a continuation of Ser. No. ", Size-1);
    break;
  case 82:
    if (Flag == 2)
      strncpy(S, "is a continuation-in-part of Ser. No. ", Size-1);
    else
      strncpy(S, ", which is a continuation-in-part of Ser. No. ", Size-1);
    break;
    
    // 83 is missing too!! - gem
    
  case 84:
    if (Flag == 2)
      strncpy(S, "is a division of Ser. No. ", Size-1);
    else
      strncpy(S, ", which is a division of Ser. No ", Size-1);
    break;
    
    // so's 85 - gem
    
  case 86:
    strncpy(S, ", said Ser. No. ", Size-1);
    break;
    
    // and 87 & 88 - gem
    
  case 89:
    strncpy(S, ", Ser. No ", Size-1);
    break;
  case 90:
    strncpy(S, " and Ser No. ", Size-1);
    break;
  case 91:
    strncpy(S, " and a continuation of Ser. No. ", Size-1);
    break;
  case 92:
    strncpy(S, ", each Ser. No. ", Size-1);
    break;
  default:
    strncpy(S, " ", Size-1);
  }
  
  S[Size-1] = '\0';
  
  if (Flag == 1) 
    S[0] = toupper(S[0]);
  
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_Get_PN
   #	AUTHOR:		Nassib Nassar, Jim Fullton, Glenn MacStravic
   #	DATE:		2/8/96
   #	TYPE:		Private
   #	COMMENTS:	Get Unformatted Patent Number
   #
   #
   ###################################################################
   */
char* 
USPAT::pato_Get_PN(PATO_PATENT* Patent, char* S, int SSize) const
{
  PATO_GROUP* Group;
  int x;
  char PS[4000], PPrefix[PATO_MAXSTR], Original[PATO_MAXSTR];
  
  Group = pato_FindGroup(Patent, "PATN");
  if(Group==NULL ){
    strcpy(S," ");
    return(S);
  }
  pato_GetField(Group, "WKU", PS);
  strcpy(Original, PS);
  PS[8] = '\0';
  strcpy(PPrefix, PS);
  x = 0;
  while ( (!isdigit(PPrefix[x])) && (PPrefix[x] != '\0') )
    x++;
  PPrefix[x] = '\0';
  if (PPrefix[0] != '\0')
    strcat(PPrefix, " ");
  strcpy(PS, PS + x);
  while (PS[0] == '0')
    strcpy(PS, PS+1);
  strncpy(S, PPrefix, SSize-1);
  S[SSize-1] = '\0';
  strncat(S, PS, SSize-1);
  S[SSize-1] = '\0';
  
  return S;
}				


/* ###################################################################
   #
   #	MODULE: 	pato_OutputBuffer
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_OutputBuffer(PATO_OUTPUT* Output, PATO_BUFFER* Buffer) const
{
  Output->Type = 2;
  Output->Buffer = Buffer;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_InitBuffer
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_InitBuffer(PATO_BUFFER* Buffer, int Size) const
{
  Buffer->Data = (char*)malloc(Size);
  Buffer->Size = Size;
  Buffer->Index = 0;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_FreeBuffer
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_FreeBuffer(PATO_BUFFER* Buffer) const
{
  free(Buffer->Data);
  Buffer->Data = NULL;
  Buffer->Size = 0;
  Buffer->Index = 0;
}


/*
   ###################################################################
   #
   #	MODULE: 	pato_GetField
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	
   #			
   #
   #
   ###################################################################
   */
char* 
USPAT::pato_GetField(PATO_GROUP* Group, const char* Field, char* S) const
{
  PATO_FIELD* F;
  
  F = pato_FindField(Group, Field);
  if (F == NULL)
    S[0] = '\0';
  else
    strcpy(S, F->Data);
  return S;
}


/*
   ###################################################################
   #
   #	MODULE: 	InsertField
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		9/23/95
   #	TYPE:		Private
   #	COMMENTS:	
   #			
   #
   #
   ###################################################################
   */

/*
   void USPAT::InsertField(PDFT pdft, char* Field, int Start, int End) const
   {
   STRING FieldName;
   FC fc;
   FCT *pfct;
   DF df;
   DFD dfd;
   
   FieldName = Field;
   dfd.SetFieldName(FieldName);
   Db->DfdtAddEntry(dfd);
   fc.SetFieldStart(Start);
   fc.SetFieldEnd(End);
   pfct = new FCT();
   pfct->AddEntry(fc);
   df.SetFct(*pfct);
   df.SetFieldName(FieldName);
   pdft->AddEntry(df);
   delete pfct;
   }
   */
/*
   ###################################################################
   #
   #	MODULE: 	AddCommas
   #	AUTHOR:		Nassib Nassar, Jim Fullton, Glenn MacStravic
   #	DATE:		2/23/96
   #	TYPE:		Private
   #	COMMENTS:	Add commas to a number.  5123456 becomes
   #			5,123,456 & D 346932 = D 346,932
   #
   #
   ###################################################################
   */

void 
USPAT::AddCommas(char* S1, char* S2) const
{
  int SL = strlen(S1);
  char S[40];
  char* p = S;
  int x;
  int y = 0;
  for (x=SL-1; x>=0; x--) {
    if ( isdigit(S1[x]) ) {
      if (y > 2) {
	y = 0;
	*(p++) = ',';
      }
      y++;
    }
    else y = 0;
    *(p++) = S1[x];
  }
  
  *p = '\0';
  
  p = S2;
  SL = strlen(S);
  for (x=SL-1; x>=0; x--) {
    *(p++) = S[x];
  }
  *p = '\0';
}

/*
   ###################################################################
   #
   #	MODULE: 	Semi_to_Comma
   #	AUTHOR:		Glenn MacStravic
   #	DATE:		2/8/96
   #	TYPE:		Private
   #	COMMENTS:	Convert ';' to ', '
   #
   ###################################################################
   */
void 
USPAT::Semi_to_Comma(char* S1) const
{
  char S[256];
  char* p = S;
  char* q = S1;
  
  while (*q != '\0') {
    if (*q == ';') {
      *(p++) = ',';
      *(p++) = ' ';
    }
    
    else *(p++) = *q;
    
    q++;
  }
  
  *p = '\0';
  strcpy(S1, S);
  
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_ElementSet
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Determine if caller asked for a valid
   #			element set, and call correct patent processing
   #                    function for requested PRS
   #
   ###################################################################
   */
void 
USPAT::pato_ElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
			    PSTRING ElementSet, char* Format) const
{
  char ES[256];
  char Element[80];
  char p[256];
  char* RealElementSet=p;
  ElementSet->GetCString(RealElementSet,256);
  int x, y;
  char RecordSyntax[256];
  *ES = '\0';
  *RecordSyntax = '\0';
  if (*RealElementSet == '*')
    RealElementSet++;
  x = pato_itemCount(RealElementSet, ',');
  for (y=1; y<=x; y++) {
    if (y != 1)
      strcat(ES, ",");
    pato_item(RealElementSet, y, Element, ',');
    
    // these are the supported element sets for patents
    
    if ( (strcmp(Element, "B") == 0) ||
	(strcmp(Element, "F") == 0) ||
	(strcmp(Element, "FRO") == 0) ||
	(strcmp(Element, "CIT") == 0) ||
	(strcmp(Element, "PN") == 0) ||
	(strcmp(Element, "TI") == 0) ||
	(strcmp(Element, "COMPACT") == 0) ||
	(strcmp(Element, "IMAGES") == 0) ||
	(strcmp(Element, "AB") == 0) ) {
      if (strcmp(Element, "B") == 0)
	strcat(ES, "PN,TI");
      else
	strcat(ES, Element);
    } else {
      strcat(ES, "B");
    }
  }
  
  if ( (strcmp(Format, "HTML") == 0) ||
      (strcmp(Format, HtmlRecordSyntax) == 0) || (strcmp(Format, "TEXTHTML") == 0)) {
    if (ElementSet->GetChr(1) == '*')
      strcpy(RecordSyntax, "SUTRS");
    else if (strcmp(Format, "TEXTHTML") == 0)
      strcpy(RecordSyntax, "TEXTHTML");
    else
      strcpy(RecordSyntax, "HTML");
  }
  
  if ( (strcmp(Format, "SUTRS") == 0) ||
      (strcmp(Format, SutrsRecordSyntax) == 0) ) {
    strcpy(RecordSyntax, "SUTRS");
  }
  if ( strcmp(RecordSyntax, "HTML") == 0 )
    pato_HtmlElementSet(Output, Patent, ES);
  else if ( strcmp(RecordSyntax, "TEXTHTML") == 0 )
    pato_TextHtmlElementSet(Output, Patent, ES);
  else
    pato_TextElementSet(Output, Patent, ES);
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_TextElement
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Process a request for a text (SUTRS) element
   #			
   #
   #
   ###################################################################
   */

void 
USPAT::pato_TextElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
			     char* Element) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  
  int x;
  char S[4000], PPrefix[PATO_MAXSTR], T[PATO_MAXSTR];
  char PatentNumber[40];
  
  Group = pato_FindGroup(Patent, "PATN");
  
  if ( (strcmp(Element, "F") == 0) ||
      (strcmp(Element, "PN") == 0) ||
      (strcmp(Element, "FRO") == 0)||
	(strcmp(Element, "AB")==0)  ) {
    pato_GetField(Group, "WKU", S);
    S[8] = '\0';
    strcpy(PPrefix, S);
    x = 0;
    while ( (!isdigit(PPrefix[x])) && (PPrefix[x] != '\0') )
      x++;
    PPrefix[x] = '\0';
    if (PPrefix[0] != '\0')
      strcat(PPrefix, " ");
    strcpy(S, S + x);
    while (S[0] == '0')
      strcpy(S, S+1);
    strcpy(PatentNumber, S);
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_Write(Output, "United States Patent ");
    pato_Write(Output, PPrefix);
    pato_Write(Output, PatentNumber);
    pato_Write(Output, "\n\n");
  }
  
  if ( (strcmp(Element, "PN") == 0) || (strcmp(Element, "FRO") == 0)|| 
      (strcmp(Element, "AB") == 0)  ) {
    pato_Write(Output, "Patent: ");
    if ( (*PPrefix != '\0') && (PPrefix[strlen(PPrefix)-1] == ' ') )
      PPrefix[strlen(PPrefix)-1] = '\0';
    pato_Write(Output, PPrefix);
    AddCommas(PatentNumber, S);
    pato_Write(Output, S);
    pato_Write(Output, "\n\n");
  }
  
  
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "SRC", S);
    x = atoi(S);
    pato_Write(Output, "Filing Period: ");
    switch (x) {
    case 2:
      pato_Write(Output, "Earlier than Jan. 1, 1948");
      break;
    case 3:
      pato_Write(Output, "Jan. 1, 1948 - Dec. 31, 1959");
      break;
    case 4:
      pato_Write(Output, "Jan. 1, 1960 - Dec. 31, 1969");
      break;
    case 5:
      pato_Write(Output, "Jan. 1, 1970 - Dec. 31, 1978");
      break;
    case 6:
      pato_Write(Output, "Jan. 1, 1979 - Dec. 31, 1986");
      break;
    case 7:
      pato_Write(Output, "Jan. 1, 1987 - Present");
      break;
    default:
      pato_Write(Output, "Unknown");
    }
    pato_Write(Output, "\n");
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "APN", S);
    S[6] = '\0';
    pato_Write(Output, "Application Number: ");
    pato_Write(Output, S);
    pato_Write(Output, "\n");
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "APT", S);
    x = atoi(S);
    pato_Write(Output, "Application Type: ");
    switch (x) {
    case 1:
      pato_Write(Output, "Invention (Utility) Patent");
      break;
    case 2:
      pato_Write(Output, "Reissue Patent");
      break;
    case 3:
      pato_Write(Output, "TVPP Application (if present)");
      break;
    case 4:
      pato_Write(Output, "Design Patent");
      break;
    case 5:
      pato_Write(Output, "Defensive Publication");
      break;
    case 6:
      pato_Write(Output, "Plant Patent");
      break;
    case 7:
      pato_Write(Output, "Statutory Invention Registration (SIR)");
      break;
    default:
      pato_Write(Output, "Unknown");
    }
    pato_Write(Output, "\n");
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "PBL", S);
    if (S[0] != '\0') {
      pato_Write(Output, "Publication Level: ");
      switch (S[0]) {
      case 'E':
	pato_Write(Output, "Reissue");
	break;
      case 'H':
	pato_Write(Output, "Defensive Publication");
	break;
      default:
	pato_Write(Output, "Unknown");
      }
      pato_Write(Output, "\n");
    }
  }
  
  /*
     if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
     pato_GetField(Group, "ART", S);
     pato_Write(Output, "Art Unit: ");
     pato_Write(Output, T);
     pato_Write(Output, "\n");
     }
     */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "APD", S);
    pato_Write(Output, "Application Filing Date: ");
    pato_ExpandDate(S, T);
    pato_Write(Output, T);
    pato_Write(Output, "\n");
  }
  
  Field = pato_FindField(Group, "TTL");
  if (Field != NULL) {
    if ( (strcmp(Element, "F") == 0) ||
	(strcmp(Element, "FRO") == 0) ) {
      pato_Write(Output, "Title of Invention: ");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "\n");
    }
    if ( strcmp(Element, "TI") == 0 ) {
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "ISD", S);
    pato_Write(Output, "Issue Date: ");
    pato_ExpandDate(S, T);
    pato_Write(Output, T);
    pato_Write(Output, "\n");
  }
  
  /* skipping ECL */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Field = pato_FindField(Group, "EXP");
    if (Field != NULL) {
      pato_Write(Output, "Primary Examiner: ");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Field = pato_FindField(Group, "EXA");
    if (Field != NULL) {
      pato_Write(Output, "Assistant Examiner: ");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "\n");
    }
  }
  
  /* skipping NDR, NFG */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "DCD", S);
    if (S[0] != '\0') {
      pato_Write(Output, "Disclaimer Date: ");
      pato_ExpandDate(S, T);
      pato_Write(Output, T);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    pato_GetField(Group, "TRM", S);
    if (S[0] != '\0') {
      pato_Write(Output, "Term of Patent: ");
      pato_Write(Output, S);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "INVT");
    if (Group != NULL) {
      pato_Write(Output, "Inventor:\n");
      pato_text_Names(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ASSG");
    if (Group != NULL) {
      pato_Write(Output, "Assignee:\n");
      pato_text_Names(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  /* skipping All PRIR, All REIS */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "CLAS");
    if (Group != NULL) {
      Field = pato_FindField(Group, "FSC");
      if (Field != NULL) {
	int SubCount = 0;
	char Class[40];
	*Class = '\0';
	pato_Write(Output, "Classification:\n");
	while ( (Field != NULL) &&
	       ( (strcmp(Field->ID, "FSC") == 0) ||
		(strcmp(Field->ID, "FSS") == 0) ) ) {
	  if (strcmp(Field->ID, "FSC") == 0) {
	    char* p = Field->Data;
	    while (*p == ' ')
	      p++;
	    strcpy(Class, p);
	    pato_Write(Output, "\nClass ");
	    pato_Write(Output, Class);
	    pato_Write(Output, ": Subclass ");
	    SubCount = 0;
	    Field = Field->Next;
	  } else {
	    int x, y;
	    char Sub[40], GSub[40];
	    do {
	      y = pato_itemCount(Field->Data, ';');
	      for (x=1; x<=y; x++) {
		pato_item(Field->Data,
			  x, Sub, ';');
		strcpy(GSub, Sub);
		if (pato_GoodClass(GSub))
		  {
		    SubCount++;
		    if (SubCount
			!= 1)
		      pato_Write(Output, ", ");
		    if (strcmp(
			       Class,
			       "43")
			== 0) {
		      pato_Write(Output, Sub);
		    } else {
		      pato_Write(Output, Sub);
		    }
		  }
	      }
	      Field = Field->Next;
	    } while ((Field != NULL) &&
		     (Field->ID[0] == '\0'));
	  }
	}
	pato_Write(Output, "\n");
      }
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    pato_Write(Output, "References:\n");
    Group = pato_FindGroup(Patent, "UREF");
    if (Group != NULL) {
      pato_Write(Output, "U.S. Patent Documents:\n");
      pato_Write(Output, "\n");
      while ( (Group != NULL) && (strcmp(Group->ID, "UREF") == 0) ) {
	pato_GetField(Group, "PNO", S);		
	pato_Write(Output, S);
	pato_Write(Output, " (");
	pato_GetField(Group, "ISD", S);
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	pato_Write(Output, "; ");
	pato_GetField(Group, "NAM", S);
	pato_Write(Output, S);
	pato_Write(Output, ")\n");
	/* skipping OCL, XCL, UCL */
	Group = Group->Next;
      }
      pato_Write(Output, "\n");
    }
  }
  
  /* skipping All FREF */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "OREF");
    if (Group != NULL) {
      pato_Write(Output, "Other References:\n");
      pato_text_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ABST");
    if (Group != NULL) {
      pato_Write(Output, "Abstract\n");
      pato_text_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) ) {
    Group = pato_FindGroup(Patent, "BSUM");
    if (Group != NULL) {
      pato_text_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) ) {
    Group = pato_FindGroup(Patent, "DETD");
    if (Group != NULL) {
      pato_text_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) ) {
    Group = pato_FindGroup(Patent, "CLMS");
    if (Group != NULL) {
      pato_Write(Output, "Claims\n");
      pato_text_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  
}				/* pato_TextElementSet() */

/*
   ###################################################################
   #
   #	MODULE: 	pato_HtmlElement
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Process request for an HTML element
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_HtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
			     char* Element) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  int x;
  int reissue_flag = 0;
  int icl_flag = 0;
  int ocl_flag = 0;
  int fsc_flag = 0;
  char S[4000], PPrefix[PATO_MAXSTR], T[PATO_MAXSTR];
  char PatentNumber[40],Temp[1024];
  
  Group = pato_FindGroup(Patent, "PATN");
  
  /*
     if ( (strcmp(Element, "F") == 0) ||
     (strcmp(Element, "PN") == 0) ||
     (strcmp(Element, "COMPACT") == 0) ||
     (strcmp(Element, "FRO") == 0) ) {
     */
  pato_GetField(Group, "WKU", S);
  S[8] = '\0';
  strcpy(PPrefix, S);
  x = 0;
  while ( (!isdigit(PPrefix[x])) && (PPrefix[x] != '\0') )
    x++;
  PPrefix[x] = '\0';
  if (PPrefix[0] != '\0')
    strcat(PPrefix, " ");
  strcpy(S, S + x);
  while (S[0] == '0')
    strcpy(S, S+1);
  AddCommas(S,PatentNumber);
  //  strcpy(PatentNumber, S);
  /*	} */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    
    
    Group = pato_FindGroup(Patent, "PATN");
    pato_Write(Output, "<TITLE>United States Patent ");
    pato_Write(Output, PPrefix);
    pato_Write(Output, PatentNumber);
    
    pato_Write(Output, "</TITLE>");
    pato_Write(Output,"<TABLE WIDTH=100%>");
    pato_Write(Output,"<TR>");
    pato_Write(Output,"<TD ALIGN=LEFT WIDTH=50%><B>United States Patent</B></TD> ");
    pato_Write(Output,"<TD ALIGN=RIGHT WIDTH=50%><B>");
    pato_Write(Output, PPrefix);
    pato_Write(Output, PatentNumber);
    pato_Write(Output, "</B></TD></TR>\n");
    
    if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
	(strcmp(Element, "CIT") == 0) ) {
      pato_GetField(Group, "ISD", S);
      
      pato_ExpandDate(S, T);
      pato_GetField(Group, "DCD", S);
      if ( S[0] != '\0' ) {
	strcpy(S, "* ");
	S[2] = '\0';
	strcat(S, T);
	S[strlen(S)] = '\0';
	strcpy(T, S);
      }
      
      pato_Write(Output,"<TR><TD ALIGN=LEFT WIDTH=50%>");
      Group = pato_FindGroup(Patent, "INVT");
      pato_NameCount(Output, Group, 10000);
      Group = pato_FindGroup(Patent, "PATN");
      pato_Write(Output,"</TD>");
      pato_Write(Output,"<TD ALIGN=RIGHT WIDTH=50%>");
      pato_Write(Output, "<B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B></TD></TR>\n");
    }
    pato_Write(Output,"</TABLE>");
    pato_Write(Output, "<HR>\n");
  }
  Field = pato_FindField(Group, "TTL");
  if (Field != NULL) {
    if ( (strcmp(Element, "F") == 0) ||
	(strcmp(Element, "FRO") == 0) ||
	(strcmp(Element, "CIT") == 0) ) {
      pato_Write(Output, "<B>");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "</B><BR><BR>\n");
    }
  }
  
  
  pato_Write(Output,"<TABLE>");
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "INVT");
    if (Group != NULL) {
      pato_Write(Output,"<TR>");
      pato_Write(Output, "<TD VALIGN=TOP>Inventors: </TD>"); 
      pato_Write(Output, "<TD VALIGN=TOP>");
      
      pato_html_Names(Output, Group, 10000);
      pato_Write(Output, "</TD>\n");
      pato_Write(Output,"</TR>\n");
    }
  }
  
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ASSG");
    if (Group != NULL) {
      pato_Write(Output,"<TR>");
      pato_Write(Output, "<TD VALIGN=TOP>Assignee: </TD>");
      pato_Write(Output, "<TD VALIGN=TOP>");
      pato_html_Names(Output, Group, 10000);
      pato_Write(Output, "</TD>");
      pato_Write(Output,"</TR>\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    if (Group != NULL) {
      pato_GetField(Group, "DCD", S);
      if ( S[0] != '\0') {
	pato_Write(Output, "<TR><TD VALIGN=TOP><b>[*]</b> Notice: </TD>");
	pato_Write(Output, 
		   "<TD VALIGN=TOP>The portion of the term of this patent subsequent to ");
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	pato_Write(Output, " has been disclaimed.</TD></TR>\n");
	
      }
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    
    pato_GetField(Group, "APN", S);
    S[6] = '\0';
    pato_Write(Output,"<TR>");
    pato_Write(Output, "<TD>Appl. No.: </TD>");
    AddCommas(S,Temp);
    pato_Write(Output, "<TD><B>");
    pato_Write(Output, Temp);
    pato_Write(Output, "</B></TD>");
    pato_Write(Output,"</TR>\n");
  }
  
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    pato_Write(Output,"<TR>");
    pato_GetField(Group, "APD", S);
    pato_Write(Output, "<TD>Filed: </TD>");
    pato_ExpandDate(S, T);
    pato_Write(Output, "<TD><B>");
    pato_Write(Output, T);
    pato_Write(Output, "</B></TD></TR>");
  }
  
  /* PCT goes here */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PCTA");
    if(Group!=NULL){
      
      pato_GetField(Group, "PD3",S);
      
      pato_ExpandDate(S,T);
      pato_Write(Output,"<TR>");
      pato_Write(Output,"<TD>PCT Filed: </TD>");
      pato_Write(Output,"<TD><B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B></TD></TR>\n");
      pato_GetField(Group,"PCN",S);
      
      pato_Write(Output,"<TR>");
      pato_Write(Output,"<TD>PCT No: </TD>");
      pato_Write(Output,"<TD><B>");
      pato_Write(Output,S);
      pato_Write(Output,"</B></TD></TR>\n");
      
      pato_GetField(Group,"PD1",S);
      pato_ExpandDate(S,T);
      pato_Write(Output,"<TR>");
      pato_Write(Output,"<TD>371 Date: </TD>");
      pato_Write(Output,"<TD><B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B></TD></TR>\n");
      
      Field=pato_FindField(Group,"PD2");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_ExpandDate(S,T);
	pato_Write(Output,"<TR>");
	pato_Write(Output,"<TD>102(e) Date: </TD>");
	pato_Write(Output,"<TD><B>");
	pato_Write(Output, T);
	pato_Write(Output, "</B></TD></TR>\n");
      }
      Field=pato_FindField(Group,"PCP");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_Write(Output,"<TR>");
	pato_Write(Output,"<TD>PCT Pub. No.: </TD>");
	pato_Write(Output,"<TD><B>");
	pato_Write(Output, S);
	pato_Write(Output, "</B></TD></TR>\n");
      }
      Field=pato_FindField(Group,"PCD");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_ExpandDate(S,T);
	pato_Write(Output,"<TR>");
	
	pato_Write(Output,"<TD>PCT Pub. Date: </TD>");
	pato_Write(Output,"<TD><B>");
	pato_Write(Output, T);
	pato_Write(Output, "</B></TD></TR>\n");
      }
      
    }
    
  }
  
  pato_Write(Output, "\n</TABLE>\n");
  
  
  /*  Reissue data, Tim & Glenn, 2/8/96  */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "REIS");
    if (Group != NULL) {
      char mess[80], apn[10], apd[10], pno[10], isd[10];
      int code;
      FILE* USPN;
      long FSize;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      reissue_flag = 1;
      
      pato_Write(Output, "<p><CENTER><b>Related U.S. Patent Documents</b></CENTER>\n");
      while ( (Group != NULL) && (strcmp(Group->ID, "REIS") == 0) ) {
	Field = pato_FindField(Group, "COD");
	if (Field != NULL) {
	  code=atoi(Field->Data);
	  pato_GetField(Group, "APN", apn);
	  pato_GetField(Group, "APD", apd);
	  pato_GetField(Group, "PNO", pno);
	  pato_GetField(Group, "ISD", isd);
	  
	  pato_Code(mess,code,80,1);
	  pato_Write(Output, mess);	// write reissue code string - gem
	  pato_Write(Output, "<CENTER><TABLE WIDTH=95%>\n");
	  pato_Write(Output, "\n");
	  
	  if ( pno[0] != '\0') {	// patent number cell with lookup
	    PRESULT PResult= new RESULT;
	    STRING PNString,TmpString;
	    bool hit;
	    char* p;
	    
	    strcpy(T,pno);
	    p = T+strlen(T);
	    while ( !isdigit(*p) ) *(p--) = '\0';
	    p = T;
	    while ( !isdigit(*p) || (*p == '0') ) p++;
	    PNString=p;
	    while ( pno[0] == '0') strcpy(pno,pno+1);
	    
	    /* if(DoLookup==true)
	       hit=PntLookup(USPN,FSize,PNString);
	       else*/
	    hit=false;
	    
	    pato_Write(Output,"<TR><TD Valign=top align=left width=30%>Patent No.:");
	    pato_Write(Output,"</TD><TD valign=top align=left width=30%>");
	    
	    
	    pato_Write(Output, "<B>");
	    if(hit==false) {
	      AddCommas(pno,T);
	      pato_Write(Output, T);
	    }
	    else{
	      //pato_Write(Output,"<a href=\"/cgi-bin/patfetch?PN+");
	      pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	      pato_Write(Output,p);
	      pato_Write(Output,"+F");
	      pato_Write(Output,"\">");
	      AddCommas(pno,T);
	      pato_Write(Output,T);
	      pato_Write(Output,"</a>");
	    }
	    delete PResult;
	    pato_Write(Output, "</B></TD><TD></TD></TR>\n");
	  }
	  
	  if ( isd[0] != '\0') {	// issue date cell
	    pato_Write(Output,"<TR><TD valign=top align=left width=30%>Issued:");
	    pato_Write(Output,"</TD><TD valign=top align=left width=30%>");
	    pato_ExpandDate(isd, T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B></TD><TD></TD></TR>\n");
	  }
	  
	  if ( apn[0] != '\0') {
	    pato_Write(Output,"<TR><TD valign=top align=left width=30%>Appl. No.:");
	    pato_Write(Output,"</TD><TD valign=top align=left width=30%>"); 
	    AddCommas(apn,T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B></TD><TD></TD></TR>\n");
	  }
	  
	  if ( apd[0] != '\0') {
	    pato_Write(Output,"<TR><TD Valign=top align=left width=30%>Filed:");
	    pato_Write(Output,"</TD><TD Valign=top align=left width=30%>"); 
	    pato_ExpandDate(apd, T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B></TD><TD></TD></TR>\n");
	  }
	  Group = Group->Next;		// goto next
	}
      }
      fclose(USPN);
      pato_Write(Output, "</TABLE></CENTER>\n");
    }
  }
  
  /* End Reissue */
  
  /* begin Related U.S. Application Data - gem */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    Group = pato_FindGroup(Patent, "RLAP");
    if (Group != NULL) {   
      FILE* USPN;
      long FSize;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      if (reissue_flag == 0 ) {
	pato_Write(Output, "<p><CENTER><B>Related U.S. Application Data</B></CENTER>\n");
	pato_Write(Output, "<CENTER><TABLE WIDTH=95%><TR><TD>");
      }
      else
	pato_Write(Output, "U.S. Applications:\n<CENTER><TABLE WIDTH=95%><TR><TD>");
      
      int Flag = 1;
      while ( (Group != NULL) && (strcmp(Group->ID, "RLAP") == 0) ) {
	char mess[80], apn[10], apd[10], psc[10], pno[10], isd[10];
	int code;
	
	Field = pato_FindField(Group, "COD");
	if (Field != NULL) {
	  code = atoi(Field->Data);
	  
	  pato_GetField(Group, "APN", apn);
	  pato_GetField(Group, "APD", apd);
	  pato_GetField(Group, "PSC", psc);
	  pato_GetField(Group, "PNO", pno);
	  pato_GetField(Group, "ISD", isd);
	  
	  pato_Code(mess, code, 80, Flag); // write relation code string
	  if ( (code == 86) || (code == 92) )
	    Flag = 2;
	  else
	    Flag = 0;
	  pato_Write(Output, mess);
	  
	  if ( apn[0] != '\0') {
	    AddCommas(apn, T);		// write application number
	    pato_Write(Output, T);
	  }
	  
	  if ( apd[0] != '\0' ) {
	    pato_ExpandDate(apd, T);	// write application date
	    pato_Write(Output, ", ");
	    pato_Write(Output, T);
	  }
	  
	  if ( psc[0] != '\0' ) {	// write status code string
	    code = atoi(psc);
	    pato_Code(mess, code, 80, 0);
	    pato_Write(Output, mess);
	  }
	  
	  if ( pno[0] != '\0' ) {	// write patent number
	    PRESULT PResult= new RESULT;
	    STRING PNString,TmpString;
	    bool hit;
	    char* p;
	    
	    strcpy(T,pno);
	    p = T+strlen(T);
	    while ( !isdigit(*p) ) *(p--) = '\0';
	    p = T;
	    while ( !isdigit(*p) || (*p == '0') ) p++;
	    PNString=p;
	    
	    /*  if(DoLookup==true)
		hit=PntLookup(USPN,FSize,PNString);
		else*/
	    hit=false;
	    if(hit==false) {
	      AddCommas(pno,T);
	      pato_Write(Output, T);
	    }
	    else{
	      //pato_Write(Output,"<a href=\"/cgi-bin/patfetch?PN+");
	      pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	      pato_Write(Output,p);
	      pato_Write(Output,"+F");
	      pato_Write(Output,"\">");
	      AddCommas(pno,T);
	      pato_Write(Output,T);
	      pato_Write(Output,"</a>");
	    }
	    delete PResult;
	  }
	  
	  if ( isd[0] != '\0' ) {	// write patent issue date
	    pato_ExpandDate(isd, T);
	    pato_Write(Output, ", ");
	    pato_Write(Output, T);
	  }
	  
	}
	
	Group = Group->Next;		// go to next RLAP group
      }
      fclose(USPN);
      pato_Write(Output, ".</TD></TR></TABLE></CENTER>\n");
    }
  }    
  
  
  /* end related application data */	    
  
  /* begin forieign priority here */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    char Code[20];
    Group = pato_FindGroup(Patent, "PRIR");
    if( Group != NULL ) {
      pato_Write(Output,"<P><CENTER><B>Foreign Application Priority Data</b></CENTER>");
      pato_Write(Output,"<CENTER><TABLE WIDTH=80%>");
      
      while ( (Group != NULL) && (strcmp(Group->ID, "PRIR") == 0) ) {
	Field = pato_FindField(Group, "APD");
	if(Field->Data){
	  strcpy(S,Field->Data);
	  pato_ExpandDate(S,T);		// date in T
	}
	
	Field = pato_FindField(Group, "CNT");
	if(Field->Data){
	  strcpy(Code,Field->Data);
	}
	
	if ( Code[2] == 'X') Code[2] = '\0';
	// write date/code pair
	char oBuf[256];
	sprintf(oBuf,"<TD WIDTH=100%%>%s [%s]</TD>",T,Code);
	pato_Write(Output,"<TR>");
	pato_Write(Output,oBuf);
	Field = pato_FindField(Group, "APN");
	if(Field->Data){
	  strcpy(oBuf,Field->Data);
	  pato_Write(Output,"<TD WIDTH=50% ALIGN=RIGHT>");
	  pato_Write(Output,oBuf);
	  pato_Write(Output,"</TD>");
	}
	pato_Write(Output,"</TR>\n");
	Group=Group->Next;
      }
      pato_Write(Output,"</TABLE></CENTER>");
    }
  }
  /* end foreign priority data */
  
  
  /* Intl Class */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    Group = pato_FindGroup(Patent, "CLAS");
    if (Group != NULL) {
      char xBuf[256],U[256];
      int bold=1;
      
      Field = pato_FindField(Group, "ICL");
      if (Field != NULL) {
	icl_flag = 1;
	pato_Write(Output,"<TABLE WIDTH=100%>");
	pato_Write(Output, "\n<TR><TD VALIGN=TOP WIDTH=50%><B>Intl. Cl.:</B></TD>");
	pato_Write(Output,"<TD ALIGN=RIGHT WIDTH=50%>");
	do{
	  strncpy(S,Field->Data,4);
	  S[4]='\0';
	  strncpy(T,&Field->Data[4],3);
	  T[3]='\0';
	  strcpy(U,&Field->Data[7]);
	  
	  if(bold)
	    pato_Write(Output,"<B>");
	  sprintf(xBuf,"%s %s/%s",S,T,U);
	  
	  pato_Write(Output,xBuf);
	  if(bold){
	    pato_Write(Output,"</b>");
	    bold=0;
	  }
	  
	  if ( !strcmp(Field->Next->ID, "ICL") )
	    pato_Write(Output, "; ");
	  
	}while((Field = Field->Next) && !strcmp(Field->ID, "ICL"));
	pato_Write(Output,"</TD></TR>\n");
      }
      
      /* U.S. Class */     
      
      Field = pato_FindField(Group, "OCL");
      if (Field != NULL) {
	char s[80], t[80], u[80], v[80], w[174];
	char* p;
	char* q;
	int x, y;
	
	if (icl_flag == 0)
	  pato_Write(Output,"\n<TABLE WIDTH=100%><TR>");
	else
	  pato_Write(Output,"\n<TR>");
	ocl_flag = 1;
	strcpy(v, Field->Data);
	p = v + strlen(v) - 1;
	while ( (p >= v) && (*p == ' ') ) // cut off trailing space from v - gem
	  *(p--) = '\0';
	
	strcpy(t, v);
	if ( t[6] == '\0')
	  sprintf(s, "%c%c%c", t[3], t[4], t[5]);
	else
	  sprintf(s, "%c%c%c.%c%c%c", t[3], t[4], t[5], t[6], t[7], t[8]);
	
	t[3] = '\0';
	x = 0;
	y = 0;
	while ( x <= 2) {		// eliminate all spaces from t
	  if ( t[x] == ' ')
	    x++;
	  else
	    u[y++] = t[x++];
	}
	u[y] = '\0';
	
	q = t;
	while (*q == ' ') q++;
	p = v;
	while (*p == ' ') p++;	// cut off initial spaces from v - gem
	
	pato_Write(Output, "<TD VALIGN=TOP WIDTH=50%><B>U.S. Cl.:</B></TD>");
	pato_Write(Output,"<TD ALIGN=RIGHT WIDTH=50%><B>");
	
	if (isdigit(u[0])) {		// No 'D' class files available yet
	  sprintf(w, "/CLASSES/%s/%s.html", u, u);
	  pato_Write(Output,"<a href=\"");
	  pato_Write(Output, w);
	  pato_Write(Output,"\">");
	  pato_Write(Output, q);
	  pato_Write(Output,"</a>");
	}
	else {
	  pato_Write(Output, q);
	}
	
	pato_Write(Output,"/");
	pato_ClassFixup(p);
	pato_Write(Output, s);
	pato_Write(Output, "</B>");
      }
      
      /* U.S. Cross-reference Classes */
      
      Field = pato_FindField(Group, "XCL");
      if (Field != NULL) {
	char s[80], t[80], u[80], v[80], w[174];
	char* p;
	char* q;
	int x, y;
	
	while ( (Field != NULL) && ( strcmp(Field->ID, "XCL") == 0 ) ) {
	  //
	  // walk down the remaingin XCL fields, outputting as we go - gem
	  //
	  strcpy(v, Field->Data);
	  p = v + strlen(v) - 1;
	  pato_Write(Output,"; ");
	  
	  while ( (p >= v) && (*p == ' ') ) // cut off trailing space from v - gem
	    *(p--) = '\0';
	  
	  strcpy(t, v);
	  
	  if ( t[6] == '\0' )
	    sprintf(s, "%c%c%c", t[3], t[4], t[5]);
	  else
	    sprintf(s, "%c%c%c.%c%c%c", t[3], t[4], t[5], t[6], t[7], t[8]);
	  
	  t[3] = '\0';
	  x = 0;
	  y = 0;
	  while ( x <= 2) {
	    if ( t[x] == ' ')
	      x++;
	    else
	      u[y++] = t[x++];
	  }
	  
	  u[y] = '\0';
	  
	  q = t;
	  while (*q == ' ') q++;
	  p = v;
	  while (*p == ' ') p++;	// cut off initial spaces from v - gem
	  
	  if ( isdigit(u[0]) ) {	// No 'D' class files available yet
	    sprintf(w, "/CLASSES/%s/%s.html", u, u);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, w);
	    pato_Write(Output,"\">");
	    pato_Write(Output, q);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, u);
	    pato_Write(Output, "</b>");
	  }
	  
	  pato_Write(Output,"/");
	  pato_ClassFixup(p);
	  pato_Write(Output, s);
	  Field = Field->Next;
	}
	pato_Write(Output, "</TD>");
	pato_Write(Output,"</TR>\n");
      }
      
      // begin FSC and FSS - gem
      
      Field = pato_FindField(Group, "FSC");
      if( Field != NULL ) {
	char t[80], u[80], v[256], w[174];
	char* p;
	int x, y;
	fsc_flag = 1;
	
	if ( (icl_flag == 0) && (ocl_flag == 0) )
	  pato_Write(Output, "\n<TABLE WIDTH=100%><TR><TD VALIGN=TOP WIDTH=50% ALIGN=LEFT>");
	else
	  pato_Write(Output, "<TR><TD VALIGN=TOP ALIGN=LEFT WIDTH=50%>");
	
	pato_Write(Output, "<B>Field of Search:</B></TD>");
	pato_Write(Output,"<TD VALIGN=TOP ALIGN=RIGHT WIDTH=50%>");
	
	while ( (Field != NULL) && (strcmp(Field->ID, "FSC") == 0) ) {
	  strcpy(t, Field->Data);
	  t[3]= '\0';
	  x = 0;
	  y = 0;
	  while ( x <= 2) {
	    if ( t[x] == ' ')
	      x++;
	    else
	      u[y++] = t[x++];
	  }
	  u[y] = '\0';
	  
	  p = t;
	  while (*p == ' ') p++;
	  if ( isdigit(u[0]) ) {	// No 'D' class files available yet
	    sprintf(w, "/CLASSES/%s/%s.html", u, u);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, w);
	    pato_Write(Output,"\">");
	    pato_Write(Output, p);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, p);
	    pato_Write(Output, "</b>");
	  }
	  pato_Write(Output, "/");
	  
	  Field = Field->Next;
	  if ( ( Field != NULL) && (strcmp(Field->ID, "FSS") == 0) ) {
	    strcpy(v, Field->Data);
	    p = v + strlen(v) -1;
	    while ( (p >= v) && (*p == ' ') ) // strip trailing space from v - gem
	      *(p--) = '\0';
	    Semi_to_Comma(v);
	    pato_Write(Output, v);
	    if ( Field->Next != NULL)
	      pato_Write(Output,"; ");
	  }
	  Field = Field->Next;
	}
	pato_Write(Output,"</TD></TR>\n");
      }
      
      if ( (icl_flag == 1) || (ocl_flag == 1) || (fsc_flag == 1) )
	pato_Write(Output,"</TABLE>\n");
    }
  }
  
  
  /* display References */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    
    pato_Write(Output, "<HR><B><CENTER>References Cited</CENTER></B><HR>\n");
    Group = pato_FindGroup(Patent, "UREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>U.S. Patent Documents</CENTER>\n");
      pato_Write(Output, "<CENTER><TABLE WIDTH=90%>");
      FILE* USPN;
      long FSize;
      int UREF_xcl_flag = 0;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      
      while ( (Group != NULL) && (strcmp(Group->ID, "UREF") == 0) ) {
	PRESULT PResult= new RESULT;
	STRING PNString,TmpString;
	bool hit;
	char* p;
	pato_Write(Output,"<TR>");
	pato_GetField(Group, "PNO", S);		
	/* add hyperlinking here */
	// S has raw patent number. Put in STRING and call KeyLookup.
	// if a result is returned, it exists.
	// We need to strip of prefixes from S for pntlookup to work - gem
	strcpy(T,S);
	p = T+strlen(T);
	while ( !isdigit(*p) ) *(p--) = '\0';
	p = T;
	while ( !isdigit(*p) ) p++;
	PNString=p;
	
	/* if(DoLookup==true)
	   hit=PntLookup(USPN,FSize,PNString);
	   else*/
	hit=false;
	
	pato_Write(Output,"<TD WIDTH=25%>");
	PResult->GetKey(&TmpString);
	if(hit==false) {
	  AddCommas(S,T);
	  pato_Write(Output, T);
	}
	else{
	  //pato_Write(Output,"<a href=\"/cgi-bin/patfetch?PN+");
	  pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	  pato_Write(Output,p);
	  pato_Write(Output,"+F");
	  pato_Write(Output,"\">");
	  AddCommas(S,T);
	  pato_Write(Output,T);
	  pato_Write(Output,"</a>");
	}
	delete PResult;
	pato_Write(Output,"</TD>");
	pato_Write(Output,"<TD WIDTH=25%>");
	pato_GetField(Group, "ISD", S);
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	
	pato_Write(Output,"</TD>");
	
	pato_Write(Output, "<TD WIDTH=25% ALIGN=LEFT>");
	
	pato_GetField(Group, "NAM", S);
	pato_Write(Output, S);
	pato_Write(Output, "</TD>");
	pato_Write(Output, "<TD WIDTH=25% ALIGN=RIGHT> ");
	Field=pato_FindField(Group, "OCL");
	
	if (Field == NULL) {
	  Field=pato_FindField(Group, "XCL");
	  if (Field != NULL)
	    UREF_xcl_flag = 1;
	}
	
	if (Field != NULL) {
	  char cl[10],scl[10],ex[10],bf[128],temp[10];
	  int x=0;
	  int y=0;
	  strcpy(S,Field->Data);
	  strncpy(cl,S,3);
	  cl[3]='\0';
	  while ( x <= 2) {
	    if ( cl[x] == ' ') 
	      x++;
	    else 
	      temp[y++] = cl[x++];
	  }
	  temp[y] = '\0';
	  
	  strncpy(scl,S+3,3);
	  scl[3]='\0';
	  strcpy(ex,S+6);
	  if (isdigit(temp[0])) {
	    sprintf(bf, "/CLASSES/%s/%s.html", temp, temp);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, bf);
	    pato_Write(Output,"\">");
	    pato_Write(Output, cl);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, cl);
	    pato_Write(Output, "</b>");
	  }
	  pato_Write(Output, "/");
	  
	  if(!strlen(ex))
	    sprintf(bf,"%s",scl);
	  else
	    sprintf(bf,"%s%s",scl,ex);
	  pato_Write(Output, bf);
	  
	}
	
	if (UREF_xcl_flag == 1) {
	  pato_Write(Output, " <b>X</b>");
	  UREF_xcl_flag = 0;
	}
	
	pato_Write(Output,"</TD>"); 
	pato_Write(Output,"</TR>\n");
	Group = Group->Next;
      }
      fclose(USPN);
      pato_Write(Output, "</TABLE></CENTER><P>");
    }
  }
  // end u.s. references.
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    
    Group = pato_FindGroup(Patent, "FREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>Foreign Patent Documents</CENTER>\n");
      pato_Write(Output, "<CENTER><TABLE WIDTH=90%>");
      while ( (Group != NULL) && (strcmp(Group->ID, "FREF") == 0) ) {
	pato_Write(Output,"<TR>");
	pato_GetField(Group, "PNO", S);		
	//AddCommas(S,T);
	pato_Write(Output,"<TD WIDTH=25% ALIGN>");
	pato_Write(Output, S);
	pato_Write(Output,"</TD>");
	pato_Write(Output,"<TD WIDTH=25%>");
	pato_GetField(Group, "ISD", S);
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	
	pato_Write(Output,"</TD>");
	
	pato_Write(Output, "<TD WIDTH=25% ALIGN=LEFT>");
	
	pato_GetField(Group, "CNT", S);
	if (S[2] == 'X') S[2] = '\0'; // eliminate 'X' from end of country codes.
	pato_Write(Output, S);
	pato_Write(Output, "</TD>");
	pato_Write(Output, "<TD WIDTH=25% ALIGN=RIGHT> ");
	Field=pato_FindField(Group, "OCL");
	if(Field!=NULL){
	  char cl[10],scl[10],ex[10],bf[32];
	  strcpy(S,Field->Data);
	  strncpy(cl,S,3);
	  cl[3]='\0';
	  strncpy(scl,S+3,3);
	  scl[3]='\0';
	  strcpy(ex,S+6);
	  if(!strlen(ex))
	    sprintf(bf,"%s/%s",cl,scl);
	  else
	    sprintf(bf,"%s/%s %s",cl,scl,ex);
	  pato_Write(Output,bf);
	}else
	  pato_Write(Output, " ");
	pato_Write(Output,"</TD></TR>\n"); 
	/* skipping OCL, XCL, UCL */
	Group = Group->Next;
      }
      pato_Write(Output, "</TABLE></CENTER><P>");
    }
  }
  // end foreign references  
  
  // begin other references
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "OREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>Other References</CENTER>\n<UL>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "</UL><BR>");
    }
  }
  // end other references
  // end references
  
  // begin Examiner Info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    Field = pato_FindField(Group, "EXP");
    if (Field != NULL) {
      pato_Write(Output, "<I>Primary Examiner: </I>");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "<BR>\n");
    }
    Field = pato_FindField(Group, "EXA");
    if (Field != NULL) {
      pato_Write(Output, "\n<I>Assistant Examiner: </I>");
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, "</B><BR>\n");
    }
  }
  // end examiner info
  
  // begin Legal Rep Info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "LREP");
    
    if (Group != NULL) {
      pato_Write(Output, "<I>Attorney, Agent or Firm: </I>");
      Field = pato_FindField(Group, "FRM");
      
      if (Field != NULL) {
	pato_Write(Output, Field->Data);
      }
      
      if ( (Field = pato_FindField(Group, "FR2") ) != NULL) {
	while (( Field != NULL ) && (strcmp(Field->ID, "FR2") == 0)) {
	  pato_Write(Output, Field->Data);
	  Field = Field->Next;
	  if (( Field != NULL) && (strcmp(Field->ID, "FR2") == 0))
	    pato_Write(Output, ", ");
	} 
      }
      
      if ( (Field = pato_FindField(Group, "ATT") ) != NULL) {
	while (( Field != NULL ) && (strcmp(Field->ID, "ATT") == 0)) {
	  pato_Write(Output, Field->Data);
	  Field = Field->Next;
	  if ((Field != NULL ) && (strcmp(Field->ID, "REG") == 0)) {
	    pato_Write(Output, "(");
	    pato_Write(Output, Field->Data);
	    pato_Write(Output, ")");
	    Field = Field->Next;
	  }
	  if ((Field != NULL) && strcmp(Field->ID, "ATT")) 
	    pato_Write(Output, ", ");
	}
      }
      
      if ( (Field = pato_FindField(Group, "AAT") ) != NULL) {
	pato_Write(Output, ", ");
	pato_Write(Output, Field->Data);
      }
      
      if ( (Field = pato_FindField(Group, "AGT") ) != NULL) {
	pato_Write(Output, Field->Data);
      }
      
      pato_Write(Output, "<BR>\n");
    }
  }
  // end legal rep info
  
  // begin abstract
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ABST");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Abstract</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end abstract
  
  // begin design claims
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "DCLM");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Design Claims");
      pato_Write(Output, "</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end design claims
  
  // begin claims and drawings info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    if(Group!=NULL){
      Field = pato_FindField(Group, "NCL");
      if (Field != NULL) {
	int i;
	char fBuf[256];
	
	i=atoi(Field->Data);
	if(i>1)
	  sprintf(fBuf,"%d Claims, ",i);
	else if(i==1)
	  sprintf(fBuf,"%d Claim, ",i);
	pato_Write(Output,"<CENTER><B>");
	pato_Write(Output, fBuf);
	pato_Write(Output, "</B>\n");
      }
    }else{
      char fBuf[256];
      sprintf(fBuf,"No Claims, ");
      pato_Write(Output,"<CENTER><B>");
      pato_Write(Output, fBuf);
      pato_Write(Output, "</B>\n");
    }
    Group = pato_FindGroup(Patent, "PATN");
    Field = pato_FindField(Group, "NFG");
    if (Field != NULL) {
      pato_Write(Output,"<B>");
      if(atoi(Field->Data)>0){
	pato_Write(Output, Field->Data);
	pato_Write(Output, " Drawing Figures");
      }else
	pato_Write(Output,"No Drawings");
      pato_Write(Output, "</B>\n");
      
    }else
      pato_Write(Output,"<B>No Drawings </B>");
    pato_Write(Output,"</CENTER>");
  }
  // end claims and drawings info
  
  // begin government interest
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "GOVT");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end government interest
  
  // begin brief summary
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "BSUM");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end brief summary
  
  // begin detailed description
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "DETD");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  // end detailed description
  
  // begin claims
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "CLMS");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Claims</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "</UL>\n");
    }
  }
  // end claims
  
}				/* pato_HtmlElementSet() */
/*
   ###################################################################
   #
   #	MODULE: 	pato_TextHtmlElement
   #	AUTHOR:		Glenn MacStravic
   #	DATE:		3/7/96
   #	TYPE:		Private
   #	COMMENTS:	Process request for an Text-only HTML element
   #
   ###################################################################
   */
void 
USPAT::pato_TextHtmlElement(PATO_OUTPUT* Output, PATO_PATENT* Patent,
				 char* Element) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  int x;
  int reissue_flag = 0;
  int icl_flag = 0;
  int ocl_flag = 0;
  int fsc_flag = 0;
  char S[4000], PPrefix[PATO_MAXSTR], T[PATO_MAXSTR];
  char PatentNumber[40],Temp[1024];
  
  Group = pato_FindGroup(Patent, "PATN");
  
  /*
     if ( (strcmp(Element, "F") == 0) ||
     (strcmp(Element, "PN") == 0) ||
     (strcmp(Element, "COMPACT") == 0) ||
     (strcmp(Element, "FRO") == 0) ) {
     */
  pato_GetField(Group, "WKU", S);
  S[8] = '\0';
  strcpy(PPrefix, S);
  x = 0;
  while ( (!isdigit(PPrefix[x])) && (PPrefix[x] != '\0') )
    x++;
  PPrefix[x] = '\0';
  if (PPrefix[0] != '\0')
    strcat(PPrefix, " ");
  strcpy(S, S + x);
  while (S[0] == '0')
    strcpy(S, S+1);
  AddCommas(S,PatentNumber);
  //  strcpy(PatentNumber, S);
  /*	} */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    
    Group = pato_FindGroup(Patent, "PATN");
    pato_Write(Output, "<TITLE>United States Patent ");
    pato_Write(Output, PPrefix);
    pato_Write(Output, PatentNumber);
    
    pato_Write(Output, "</TITLE>");
    pato_Write(Output,"<B>United States Patent</B> ");
    pato_Write(Output, PPrefix);
    pato_Write(Output, PatentNumber);
    pato_Write(Output, "<p>");
    
    if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
	(strcmp(Element, "CIT") == 0) ) {
      pato_GetField(Group, "ISD", S);
      
      pato_ExpandDate(S, T);
      pato_GetField(Group, "DCD", S);
      if ( S[0] != '\0' ) {
	strcpy(S, "* ");
	S[2] = '\0';
	strcat(S, T);
	S[strlen(S)] = '\0';
	strcpy(T, S);
      }
      
      Group = pato_FindGroup(Patent, "INVT");
      pato_NameCount(Output, Group, 10000);
      Group = pato_FindGroup(Patent, "PATN");
      pato_Write(Output, " <B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B><p>");
    }
    pato_Write(Output, "<HR>\n");
  }
  Field = pato_FindField(Group, "TTL");
  if (Field != NULL) {
    if ( (strcmp(Element, "F") == 0) ||
	(strcmp(Element, "FRO") == 0) ||
	(strcmp(Element, "CIT") == 0) ) {
      pato_Write(Output, "<B>");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "</B><BR><BR>\n");
    }
  }
  
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "INVT");
    if (Group != NULL) {
      pato_Write(Output, "Inventors: "); 
      
      pato_html_Names(Output, Group, 10000);
      pato_Write(Output, "<p>\n");
    }
  }
  
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ASSG");
    if (Group != NULL) {
      pato_Write(Output, "Assignee: ");
      pato_html_Names(Output, Group, 10000);
      pato_Write(Output,"<p>\n");
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    if (Group != NULL) {
      pato_GetField(Group, "DCD", S);
      if ( S[0] != '\0') {
	pato_Write(Output, "<b>[*]</b> Notice: ");
	pato_Write(Output, "The portion of the term of this patent subsequent to ");
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	pato_Write(Output, " has been disclaimed.<p>\n");
	
      }
    }
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    
    pato_GetField(Group, "APN", S);
    S[6] = '\0';
    pato_Write(Output, "Appl. No.: ");
    AddCommas(S,Temp);
    pato_Write(Output, "<B>");
    pato_Write(Output, Temp);
    pato_Write(Output, "</B>");
    pato_Write(Output,"<p>\n");
  }
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    pato_GetField(Group, "APD", S);
    pato_Write(Output, "Filed: ");
    pato_ExpandDate(S, T);
    pato_Write(Output, "<B>");
    pato_Write(Output, T);
    pato_Write(Output, "</B><p>\n");
  }
  
  /* PCT goes here */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PCTA");
    if(Group!=NULL){
      
      pato_GetField(Group, "PD3",S);
      
      pato_ExpandDate(S,T);
      pato_Write(Output,"PCT Filed: ");
      pato_Write(Output,"<B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B><p>\n");
      pato_GetField(Group,"PCN",S);
      
      pato_Write(Output,"PCT No: ");
      pato_Write(Output,"<TD><B>");
      pato_Write(Output,S);
      pato_Write(Output,"</B></TD></TR>\n");
      
      pato_GetField(Group,"PD1",S);
      pato_ExpandDate(S,T);
      pato_Write(Output,"<TR>");
      pato_Write(Output,"<TD>371 Date: </TD>");
      pato_Write(Output,"<TD><B>");
      pato_Write(Output, T);
      pato_Write(Output, "</B><p>\n");
      
      Field=pato_FindField(Group,"PD2");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_ExpandDate(S,T);
	pato_Write(Output,"102(e) Date: ");
	pato_Write(Output,"<B>");
	pato_Write(Output, T);
	pato_Write(Output, "</B><p>\n");
      }
      Field=pato_FindField(Group,"PCP");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_Write(Output,"PCT Pub. No.: ");
	pato_Write(Output,"<B>");
	pato_Write(Output, S);
	pato_Write(Output, "</B><p>\n");
      }
      Field=pato_FindField(Group,"PCD");
      if(Field->Data){
	strcpy(S,Field->Data);
	pato_ExpandDate(S,T);
	pato_Write(Output,"PCT Pub. Date: ");
	pato_Write(Output,"<B>");
	pato_Write(Output, T);
	pato_Write(Output, "</B><p>\n");
      }
    }
  }
  
  /*  Reissue data, Tim & Glenn, 2/8/96  */
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "REIS");
    if (Group != NULL) {
      char mess[80], apn[10], apd[10], pno[10], isd[10];
      int code;
      FILE* USPN;
      long FSize;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      reissue_flag = 1;
      
      pato_Write(Output, "<p><CENTER><b>Related U.S. Patent Documents</b></CENTER>\n");
      while ( (Group != NULL) && (strcmp(Group->ID, "REIS") == 0) ) {
	Field = pato_FindField(Group, "COD");
	if (Field != NULL) {
	  code=atoi(Field->Data);
	  pato_GetField(Group, "APN", apn);
	  pato_GetField(Group, "APD", apd);
	  pato_GetField(Group, "PNO", pno);
	  pato_GetField(Group, "ISD", isd);
	  
	  pato_Code(mess,code,80,1);
	  pato_Write(Output, mess);	// write reissue code string - gem
	  pato_Write(Output, "<CENTER><p>");
	  pato_Write(Output, "\n");
	  
	  if ( pno[0] != '\0') {	// patent number cell with lookup
	    PRESULT PResult= new RESULT;
	    STRING PNString,TmpString;
	    bool hit;
	    char* p;
	    
	    strcpy(T,pno);
	    p = T+strlen(T);
	    while ( !isdigit(*p) ) *(p--) = '\0';
	    p = T;
	    while ( !isdigit(*p) || (*p == '0') ) p++;
	    PNString=p;
	    while ( pno[0] == '0') strcpy(pno,pno+1);
	    
	    /* if(DoLookup==true)
	       hit=PntLookup(USPN,FSize,PNString);
	       else*/
	    hit=false;
	    
	    pato_Write(Output,"Patent No.: ");
	    pato_Write(Output, "<B>");
	    if(hit==false) {
	      AddCommas(pno,T);
	      pato_Write(Output, T);
	    }
	    else{
	      pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	      pato_Write(Output,p);
	      pato_Write(Output,"+F");
	      pato_Write(Output,"\">");
	      AddCommas(pno,T);
	      pato_Write(Output,T);
	      pato_Write(Output,"</a>");
	    }
	    delete PResult;
	    pato_Write(Output, "</B><p>\n");
	  }
	  
	  if ( isd[0] != '\0') {	// issue date cell
	    pato_Write(Output,"Issued: ");
	    pato_ExpandDate(isd, T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B><p>\n");
	  }
	  
	  if ( apn[0] != '\0') {
	    pato_Write(Output,"Appl. No.: ");
	    AddCommas(apn,T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B><p>\n");
	  }
	  
	  if ( apd[0] != '\0') {
	    pato_Write(Output,"Filed: ");
	    pato_ExpandDate(apd, T); 
	    pato_Write(Output, "<B>");
	    pato_Write(Output, T);
	    pato_Write(Output, "</B><p>\n");
	  }
	  Group = Group->Next;		// goto next
	}
      }
      fclose(USPN);
      pato_Write(Output, "</CENTER>\n");
    }
  }
  
  /* End Reissue */
  
  /* begin Related U.S. Application Data - gem */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    Group = pato_FindGroup(Patent, "RLAP");
    if (Group != NULL) {   
      FILE* USPN;
      long FSize;
      int Flag = 1;
      char mess[80], apn[10], apd[10], psc[10], pno[10], isd[10];
      int code;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      if (reissue_flag == 0 ) {
	pato_Write(Output, "<p><CENTER><B>Related U.S. Application Data</B></CENTER>\n");
	pato_Write(Output, "<CENTER><p>\n");
      }
      else
	pato_Write(Output, "U.S. Applications:\n<CENTER><p>\n");
      
      while ( (Group != NULL) && (strcmp(Group->ID, "RLAP") == 0) ) {
	Field = pato_FindField(Group, "COD");
	if (Field != NULL) {
	  code = atoi(Field->Data);
	  
	  pato_GetField(Group, "APN", apn);
	  pato_GetField(Group, "APD", apd);
	  pato_GetField(Group, "PSC", psc);
	  pato_GetField(Group, "PNO", pno);
	  pato_GetField(Group, "ISD", isd);
	  
	  pato_Code(mess, code, 80, Flag); // write relation code string
	  if ( (code == 86) || (code == 92) )
	    Flag = 2;
	  else
	    Flag = 0;
	  pato_Write(Output, mess);
	  
	  if ( apn[0] != '\0') {
	    AddCommas(apn, T);		// write application number
	    pato_Write(Output, T);
	  }
	  
	  if ( apd[0] != '\0' ) {
	    pato_ExpandDate(apd, T);	// write application date
	    pato_Write(Output, ", ");
	    pato_Write(Output, T);
	  }
	  
	  if ( psc[0] != '\0' ) {	// write status code string
	    code = atoi(psc);
	    pato_Code(mess, code, 80, 0);
	    pato_Write(Output, mess);
	  }
	  
	  if ( pno[0] != '\0' ) {	// write patent number
	    PRESULT PResult= new RESULT;
	    STRING PNString,TmpString;
	    bool hit;
	    char* p;
	    
	    strcpy(T,pno);
	    p = T+strlen(T);
	    while ( !isdigit(*p) ) *(p--) = '\0';
	    p = T;
	    while ( !isdigit(*p) || (*p == '0') ) p++;
	    PNString=p;
	    
	    /*  if(DoLookup==true)
		hit=PntLookup(USPN,FSize,PNString);
		else*/
	    hit=false;
	    if(hit==false) {
	      AddCommas(pno,T);
	      pato_Write(Output, T);
	    }
	    else{
	      pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	      pato_Write(Output,p);
	      pato_Write(Output,"+F");
	      pato_Write(Output,"\">");
	      AddCommas(pno,T);
	      pato_Write(Output,T);
	      pato_Write(Output,"</a>");
	    }
	    delete PResult;
	  }
	  
	  if ( isd[0] != '\0' ) {	// write patent issue date
	    pato_ExpandDate(isd, T);
	    pato_Write(Output, ", ");
	    pato_Write(Output, T);
	  }
	  
	}
	
	Group = Group->Next;		// go to next RLAP group
      }
      fclose(USPN);
      pato_Write(Output, ".</CENTER><p>\n");
    }
  }    
  
  
  /* end related application data */	    
  
  /* begin forieign priority here */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    char Code[20];
    Group = pato_FindGroup(Patent, "PRIR");
    if( Group != NULL ) {
      pato_Write(Output,"<P><CENTER><B>Foreign Application Priority Data</b></CENTER>\n");
      pato_Write(Output,"<CENTER><p>\n");
      
      while ( (Group != NULL) && (strcmp(Group->ID, "PRIR") == 0) ) {
	Field = pato_FindField(Group, "APD");
	if(Field->Data){
	  strcpy(S,Field->Data);
	  pato_ExpandDate(S,T);		// date in T
	}
	
	Field = pato_FindField(Group, "CNT");
	if(Field->Data){
	  strcpy(Code,Field->Data);
	}
	
	if ( Code[2] == 'X') {
	  Code[2] = '\0';
	}
	// write date/code pair
	char oBuf[256];
	sprintf(oBuf, " %s [%s] ", T, Code);
	pato_Write(Output, oBuf);
	Field = pato_FindField(Group, "APN");
	if(Field->Data) {
	  strcpy(oBuf,Field->Data);
	  pato_Write(Output,oBuf);
	}
	pato_Write(Output,"<p>\n");
	Group=Group->Next;
      }
      pato_Write(Output,"</CENTER><p>\n");
    }
  }
  
  /* end foreign priority data */
  
  
  /* Intl Class */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ||
      (strcmp(Element, "CIT") == 0) ) {
    Group = pato_FindGroup(Patent, "CLAS");
    if (Group != NULL) {
      char xBuf[256],U[256];
      int bold=1;
      
      Field = pato_FindField(Group, "ICL");
      if (Field != NULL) {
	icl_flag = 1;
	pato_Write(Output, "\n<p><B>Intl. Cl.:</B>");
	do{
	  strncpy(S,Field->Data,4);
	  S[4]='\0';
	  strncpy(T,&Field->Data[4],3);
	  T[3]='\0';
	  strcpy(U,&Field->Data[7]);
	  
	  if(bold)
	    pato_Write(Output,"<B>");
	  sprintf(xBuf,"%s %s/%s",S,T,U);
	  
	  pato_Write(Output,xBuf);
	  if(bold){
	    pato_Write(Output,"</b>");
	    bold=0;
	  }
	  
	  if ( !strcmp(Field->Next->ID, "ICL") )
	    pato_Write(Output, "; ");
	  
	}while((Field = Field->Next) && !strcmp(Field->ID, "ICL"));
	pato_Write(Output,"<p>\n");
      }
      
      /* U.S. Class */     
      
      Field = pato_FindField(Group, "OCL");
      if (Field != NULL) {
	char s[80], t[80], u[80], v[80], w[175];
	char* p;
	char* q;
	int x, y;
	
	if (icl_flag == 0)
	  pato_Write(Output,"<p>");
	ocl_flag = 1;
	strcpy(v, Field->Data);
	p = v + strlen(v) - 1;
	while ( (p >= v) && (*p == ' ') ) // cut off trailing space from v - gem
	  *(p--) = '\0';
	
	strcpy(t, v);
	if ( t[6] == '\0')
	  sprintf(s, "%c%c%c", t[3], t[4], t[5]);
	else
	  sprintf(s, "%c%c%c.%c%c%c", t[3], t[4], t[5], t[6], t[7], t[8]);
	
	t[3] = '\0';
	x = 0;
	y = 0;
	while ( x <= 2) {		// eliminate all spaces from t
	  if ( t[x] == ' ')
	    x++;
	  else
	    u[y++] = t[x++];
	}
	u[y] = '\0';
	
	q = t;
	while (*q == ' ') q++;
	p = v;
	while (*p == ' ') p++;	// cut off initial spaces from v - gem
	
	pato_Write(Output, "<B>U.S. Cl.:</B> <B>");
	
	if (isdigit(u[0])) {		// No 'D' class files available yet
	  sprintf(w, "/CLASSES/%s/%s.html", u, u);
	  pato_Write(Output,"<a href=\"");
	  pato_Write(Output, w);
	  pato_Write(Output,"\">");
	  pato_Write(Output, q);
	  pato_Write(Output,"</a>");
	}
	else {
	  pato_Write(Output, q);
	}
	
	pato_Write(Output,"/");
	pato_ClassFixup(p);
	pato_Write(Output, s);
	pato_Write(Output, "</B>");
      }
      
      /* U.S. Cross-reference Classes */
      
      Field = pato_FindField(Group, "XCL");
      if (Field != NULL) {
	char s[80], t[80], u[80], v[80], w[174];
	char* p;
	char* q;
	int x, y;
	
	while ( (Field != NULL) && ( strcmp(Field->ID, "XCL") == 0 ) ) {
	  //
	  // walk down the remaingin XCL fields, outputting as we go - gem
	  //
	  strcpy(v, Field->Data);
	  p = v + strlen(v) - 1;
	  pato_Write(Output,"; ");
	  
	  while ( (p >= v) && (*p == ' ') ) // cut off trailing space from v - gem
	    *(p--) = '\0';
	  
	  strcpy(t, v);
	  
	  if ( t[6] == '\0' )
	    sprintf(s, "%c%c%c", t[3], t[4], t[5]);
	  else
	    sprintf(s, "%c%c%c.%c%c%c", t[3], t[4], t[5], t[6], t[7], t[8]);
	  
	  t[3] = '\0';
	  x = 0;
	  y = 0;
	  while ( x <= 2) {
	    if ( t[x] == ' ')
	      x++;
	    else
	      u[y++] = t[x++];
	  }
	  
	  u[y] = '\0';
	  
	  q = t;
	  while (*q == ' ') q++;
	  p = v;
	  while (*p == ' ') p++;	// cut off initial spaces from v - gem
	  
	  if ( isdigit(u[0]) ) {	// No 'D' class files available yet
	    sprintf(w, "/CLASSES/%s/%s.html", u, u);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, w);
	    pato_Write(Output,"\">");
	    pato_Write(Output, q);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, u);
	    pato_Write(Output, "</b>");
	  }
	  
	  pato_Write(Output,"/");
	  pato_ClassFixup(p);
	  pato_Write(Output, s);
	  Field = Field->Next;
	}
	pato_Write(Output,"<p>\n");
      }
      
      // begin FSC and FSS - gem
      
      Field = pato_FindField(Group, "FSC");
      if( Field != NULL ) {
	char t[80], u[80], v[256], w[174];
	char* p;
	int x, y;
	fsc_flag = 1;
	
	if ( (icl_flag == 0) && (ocl_flag == 0) )
	  pato_Write(Output, "<p>");
	pato_Write(Output, "<B>Field of Search:</B> ");
	
	while ( (Field != NULL) && (strcmp(Field->ID, "FSC") == 0) ) {
	  strcpy(t, Field->Data);
	  t[3]= '\0';
	  x = 0;
	  y = 0;
	  while ( x <= 2) {
	    if ( t[x] == ' ')
	      x++;
	    else
	      u[y++] = t[x++];
	  }
	  u[y] = '\0';
	  
	  p = t;
	  while (*p == ' ') p++;
	  if ( isdigit(u[0]) ) {	// No 'D' class files available yet
	    sprintf(w, "/CLASSES/%s/%s.html", u, u);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, w);
	    pato_Write(Output,"\">");
	    pato_Write(Output, p);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, p);
	    pato_Write(Output, "</b>");
	  }
	  pato_Write(Output, "/");
	  
	  Field = Field->Next;
	  if ( ( Field != NULL) && (strcmp(Field->ID, "FSS") == 0) ) {
	    strcpy(v, Field->Data);
	    p = v + strlen(v) -1;
	    while ( (p >= v) && (*p == ' ') ) // strip trailing space from v - gem
	      *(p--) = '\0';
	    Semi_to_Comma(v);
	    pato_Write(Output, v);
	    if ( Field->Next != NULL)
	      pato_Write(Output,"; ");
	  }
	  Field = Field->Next;
	}
	pato_Write(Output,"<p>\n");
      }
    }
  }
  
  
  /* display References */
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    
    pato_Write(Output, "<HR><B><CENTER>References Cited</CENTER></B><HR>\n");
    Group = pato_FindGroup(Patent, "UREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>U.S. Patent Documents</CENTER>\n");
      pato_Write(Output, "<PRE>\n");
      FILE* USPN;
      long FSize;
      STRING PNString,TmpString;
      bool hit;
      char* p;
      int UREF_xcl_flag = 0;
      int x = 0;
      bool DoLookup=false;
      USPN=fopen("/pto7/USPN","rb");
      if(USPN){
	DoLookup=true;
	FSize=GetFileSize(USPN);
      }
      
      while ( (Group != NULL) && (strcmp(Group->ID, "UREF") == 0) ) {
	PRESULT PResult= new RESULT;
	pato_Write(Output,"<p>\n");
	pato_GetField(Group, "PNO", S);		
	/* add hyperlinking here */
	// S has raw patent number. Put in STRING and call KeyLookup.
	// if a result is returned, it exists.
	// We need to strip of prefixes from S for pntlookup to work - gem
	strcpy(T,S);
	p = T+strlen(T);
	while ( !isdigit(*p) ) *(p--) = '\0';
	p = T;
	while ( !isdigit(*p) ) p++;
	PNString=p;
	
	/*  if(DoLookup==true)
	    hit=PntLookup(USPN,FSize,PNString);
	    else*/
	hit=false;
	
	PResult->GetKey(&TmpString);
	if(hit==false) {
	  AddCommas(S,T);
	  pato_Write(Output, T);
	  pato_Write(Output, " ");
	}
	else{
	  pato_Write(Output,"<a href=\"/cgi-bin/patbib_fetch?/pto7+PN+");
	  pato_Write(Output,p);
	  pato_Write(Output,"+F");
	  pato_Write(Output,"\">");
	  AddCommas(S,T);
	  pato_Write(Output,T);
	  pato_Write(Output,"</a> ");
	}
	delete PResult;
	space(T,12,Output);
	pato_GetField(Group, "ISD", S);
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	space(T,11,Output);
	
	pato_GetField(Group, "NAM", S);
	pato_Write(Output, S);
	
	Field=pato_FindField(Group, "OCL");
	
	if (Field == NULL) {
	  Field=pato_FindField(Group, "XCL");
	  if (Field != NULL)
	    UREF_xcl_flag = 1;
	}
	
	if (Field != NULL) {
	  char cl[10],scl[10],ex[10],bf[128],temp[10];
	  int x=0;
	  int y=0;
	  strcpy(T,Field->Data);
	  strncpy(cl,T,3);
	  cl[3]='\0';
	  while ( x <= 2) {
	    if ( cl[x] == ' ') 
	      x++;
	    else 
	      temp[y++] = cl[x++];
	  }
	  temp[y] = '\0';
	  
	  strncpy(scl,T+3,3);
	  scl[3]='\0';
	  strcpy(ex,T+6);
	  space(S,20,Output);
	  if (isdigit(temp[0])) {
	    sprintf(bf, "/CLASSES/%s/%s.html", temp, temp);
	    pato_Write(Output,"<a href=\"");
	    pato_Write(Output, bf);
	    pato_Write(Output,"\">");
	    pato_Write(Output, cl);
	    pato_Write(Output,"</a>");
	  }
	  else {
	    pato_Write(Output, "<b>");
	    pato_Write(Output, cl);
	    pato_Write(Output, "</b>");
	  }
	  pato_Write(Output, "/");
	  
	  if(!strlen(ex))
	    sprintf(bf,"%s",scl);
	  else
	    sprintf(bf,"%s%s",scl,ex);
	  pato_Write(Output, bf);
	  
	}
	
	if (UREF_xcl_flag == 1) {
	  pato_Write(Output, " <b>X</b>");
	  UREF_xcl_flag = 0;
	}
	Group = Group->Next;
      }
      fclose(USPN);
      pato_Write(Output, "</PRE><P>\n");
    }
  }
  // end u.s. references.
  
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    
    Group = pato_FindGroup(Patent, "FREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>Foreign Patent Documents</CENTER>\n");
      pato_Write(Output, "<CENTER>");
      while ( (Group != NULL) && (strcmp(Group->ID, "FREF") == 0) ) {
	pato_Write(Output,"<p>\n");
	pato_GetField(Group, "PNO", S);		
	//AddCommas(S,T);
	pato_Write(Output, S);
	pato_Write(Output," ");
	pato_GetField(Group, "ISD", S);
	pato_ExpandDate(S, T);
	pato_Write(Output, T);
	pato_Write(Output," ");
	pato_GetField(Group, "CNT", S);
	if (S[2] == 'X') S[2] = '\0'; // eliminate 'X' from end of country codes.
	pato_Write(Output, S);
	pato_Write(Output, " ");
	
	Field=pato_FindField(Group, "OCL");
	if(Field!=NULL){
	  char cl[10],scl[10],ex[10],bf[32 /* 20 */];
	  strcpy(S,Field->Data);
	  strncpy(cl,S,3);
	  cl[3]='\0';
	  strncpy(scl,S+3,3);
	  scl[3]='\0';
	  strcpy(ex,S+6);
	  if(!strlen(ex))
	    sprintf(bf,"%s/%s",cl,scl);
	  else
	    sprintf(bf,"%s/%s %s",cl,scl,ex);
	  pato_Write(Output,bf);
	}else
	  pato_Write(Output, " ");
	/* skipping OCL, XCL, UCL */
	Group = Group->Next;
      }
      pato_Write(Output, "</CENTER><P>\n");
    }
  }
  // end foreign references  
  
  // begin other references
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "CIT") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "OREF");
    if (Group != NULL) {
      pato_Write(Output, "<CENTER>Other References</CENTER>\n<UL>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "</UL><BR>");
    }
  }
  // end other references
  // end references
  
  // begin Examiner Info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    Field = pato_FindField(Group, "EXP");
    if (Field != NULL) {
      pato_Write(Output, "<I>Primary Examiner: </I>");
      pato_Write(Output, Field->Data);
      pato_Write(Output, "\n");
      pato_html_Continue(Output, Field);
      pato_Write(Output, "<BR>\n");
    }
    Field = pato_FindField(Group, "EXA");
    if (Field != NULL) {
      pato_Write(Output, "\n<I>Assistant Examiner: </I>");
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, "</B><BR>\n");
    }
  }
  // end examiner info
  
  // begin Legal Rep Info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "LREP");
    
    if (Group != NULL) {
      pato_Write(Output, "<I>Attorney, Agent or Firm: </I>");
      Field = pato_FindField(Group, "FRM");
      
      if (Field != NULL) {
	pato_Write(Output, Field->Data);
      }
      
      if ( (Field = pato_FindField(Group, "FR2") ) != NULL) {
	while (( Field != NULL ) && (strcmp(Field->ID, "FR2") == 0)) {
	  pato_Write(Output, Field->Data);
	  Field = Field->Next;
	  if (( Field != NULL) && (strcmp(Field->ID, "FR2") == 0))
	    pato_Write(Output, ", ");
	} 
      }
      
      if ( (Field = pato_FindField(Group, "ATT") ) != NULL) {
	while (( Field != NULL ) && (strcmp(Field->ID, "ATT") == 0)) {
	  pato_Write(Output, Field->Data);
	  Field = Field->Next;
	  if ((Field != NULL ) && (strcmp(Field->ID, "REG") == 0)) {
	    pato_Write(Output, "(");
	    pato_Write(Output, Field->Data);
	    pato_Write(Output, ")");
	    Field = Field->Next;
	  }
	  if ((Field != NULL) && strcmp(Field->ID, "ATT")) 
	    pato_Write(Output, ", ");
	}
      }
      
      if ( (Field = pato_FindField(Group, "AAT") ) != NULL) {
	pato_Write(Output, ", ");
	pato_Write(Output, Field->Data);
      }
      
      if ( (Field = pato_FindField(Group, "AGT") ) != NULL) {
	pato_Write(Output, Field->Data);
      }
      
      pato_Write(Output, "<BR>\n");
    }
  }
  // end legal rep info
  
  // begin abstract
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "ABST");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Abstract</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end abstract
  
  // begin design claims
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "DCLM");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Design Claims");
      pato_Write(Output, "</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end design claims
  
  // begin claims and drawings info
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "PATN");
    if(Group!=NULL){
      Field = pato_FindField(Group, "NCL");
      if (Field != NULL) {
	int i;
	char fBuf[256];
	
	i=atoi(Field->Data);
	if(i>1)
	  sprintf(fBuf,"%d Claims, ",i);
	else if(i==1)
	  sprintf(fBuf,"%d Claim, ",i);
	pato_Write(Output,"<CENTER><B>");
	pato_Write(Output, fBuf);
	pato_Write(Output, "</B>\n");
      }
    }else{
      char fBuf[256];
      sprintf(fBuf,"No Claims, ");
      pato_Write(Output,"<CENTER><B>");
      pato_Write(Output, fBuf);
      pato_Write(Output, "</B>\n");
    }
    Group = pato_FindGroup(Patent, "PATN");
    Field = pato_FindField(Group, "NFG");
    if (Field != NULL) {
      pato_Write(Output,"<B>");
      if(atoi(Field->Data)>0){
	pato_Write(Output, Field->Data);
	pato_Write(Output, " Drawing Figures");
      }else
	pato_Write(Output,"No Drawings");
      pato_Write(Output, "</B>\n");
      
    }else
      pato_Write(Output,"<B>No Drawings </B>");
    pato_Write(Output,"</CENTER>");
  }
  // end claims and drawings info
  
  // begin government interest
  if ( (strcmp(Element, "F") == 0) || (strcmp(Element, "AB") == 0) ||
      (strcmp(Element, "FRO") == 0) ) {
    Group = pato_FindGroup(Patent, "GOVT");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end government interest
  
  // begin brief summary
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "BSUM");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "<P>\n");
    }
  }
  // end brief summary
  
  // begin detailed description
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "DETD");
    if (Group != NULL) {
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "\n");
    }
  }
  // end detailed description
  
  // begin claims
  if ( strcmp(Element, "F") == 0 ) {
    Group = pato_FindGroup(Patent, "CLMS");
    if (Group != NULL) {
      pato_Write(Output, "<HR><B><CENTER>Claims</CENTER></B><HR>\n");
      pato_html_AllPA(Output, Group);
      pato_Write(Output, "</UL>\n");
    }
  }
  // end claims
}

/*
   ###################################################################
   #
   #	MODULE: 	space
   #	AUTHOR:		Glenn MacStravic
   #	DATE:		3/7/96
   #	TYPE:		Private
   #	COMMENTS:	Write # of space based on strlen
   #			
   ###################################################################
   */

void 
USPAT::space(char* S, int length, PATO_OUTPUT* Output) const 
{
  int x;
  int SL = length - strlen(S);
  
  for (x = 0; x < SL; x++) 
    pato_Write(Output, " ");
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_write
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Write data to output stream
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_Write(PATO_OUTPUT* Output, const char* S) const
{
  switch (Output->Type) {
  case 0:
    printf("%s", S);
    break;
  case 1:
    fprintf(Output->fp, "%s", S);
    break;
  case 2:
    pato_WriteBuffer(Output->Buffer, S);
    break;
  default:
    printf("Error in Output Specification!\n");
  }
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_HtmlElementSet
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Generate HTML output for a specified element set
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_HtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
				char* ElementSet)  const
{
  int x, y;
  char Element[256];
  if ( strcmp(ElementSet, "COMPACT") != 0 )
    pato_Write(Output, "<HEAD>\n<BODY>\n<HR>\n");
  x = pato_itemCount(ElementSet, ',');
  for (y=1; y<=x; y++) {
    pato_item(ElementSet, y, Element, ',');
    pato_HtmlElement(Output, Patent, Element);
  }
  if ( strcmp(ElementSet, "COMPACT") != 0 )
    pato_Write(Output, "\n</BODY>\n</HEAD>\n");
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_TextHtmlElementSet
   #	AUTHOR:		Glenn MacStravic
   #	DATE:		3/7/95
   #	TYPE:		Private
   #	COMMENTS:	Generate Text HTML output for a specified element set
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_TextHtmlElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
				    char* ElementSet)  const
{
  int x, y;
  char Element[256];
  if ( strcmp(ElementSet, "COMPACT") != 0 )
    pato_Write(Output, "<HEAD>\n<BODY>\n<HR>\n");
  x = pato_itemCount(ElementSet, ',');
  for (y=1; y<=x; y++) {
    pato_item(ElementSet, y, Element, ',');
    pato_TextHtmlElement(Output, Patent, Element);
  }
  if ( strcmp(ElementSet, "COMPACT") != 0 )
    pato_Write(Output, "\n</BODY>\n</HEAD>\n");
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_TextElementSet
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Generate SUTRS output for a specified element
   #			set.
   #
   #
   ###################################################################
   */

void 
USPAT::pato_TextElementSet(PATO_OUTPUT* Output, PATO_PATENT* Patent,
				char* ElementSet) const
{
  int x, y;
  char Element[256];
  x = pato_itemCount(ElementSet, ',');
  for (y=1; y<=x; y++) {
    pato_item(ElementSet, y, Element, ',');
    pato_TextElement(Output, Patent, Element);
  }
}
/*
   ###################################################################
   #
   #	MODULE: 	PatentPath
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Generate a proper patent path
   #			
   #
   #
   ###################################################################
   */

void 
USPAT::PatentPath(char* fn, char* path) const
{
  char t[256], d1[256], d2[256], d3[256], buf[766];
  if (fn[2] == 'T') {
    strcpy(path, fn);
  } else {
    sprintf(t, "00%s", fn+2);
    sprintf(d1, "%c%c%c/", t[0], t[1], t[2]);
    sprintf(d2, "%c%c%c/", t[3], t[4], t[5]);
    sprintf(d3, "%c%c%c/", t[6], t[7], t[8]);
    sprintf(buf, "%s%s%s", d1, d2, d3);
    sprintf(path, "%s%s", buf, fn);
  }
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_ClassFixup
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Clean up a greenbook class/subclass field
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_ClassFixup(char* S) const
{
  int x;
  char T[256];
  char* P;
  if (strchr(S, '.') == NULL)
    strcat(S, ".");
  strcat(S, "000");
  P = strchr(S, '.');
  *(P+4) = '\0';
  strcpy(T, S);
  P = strchr(T, '.');
  *S = '\0';
  for (x=1; x<=(3-(P-T)); x++)
    strcat(S, "0");
  strcat(S, T);
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_WriteBuffer
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Write a string to a buffer
   #			
   #
   #
   ###################################################################
   */
int 
USPAT::pato_WriteBuffer(PATO_BUFFER* Buffer, const char* S) const
{
  int Z;
  int x, y;
  int Index, Size;
  char* Data;
  Index = Buffer->Index;
  Data = Buffer->Data;
  Size = Buffer->Size;
  x = Size - Index - 1;
  y = strlen(S);
  if (x > y)
    Z = y;
  else
    Z = x;
  memcpy(Data + Index, S, Z);
  *(Data + Index + Z) = '\0';
  Buffer->Index += Z;
  return Z;
}

int 
USPAT::pato_GoodClass(char* Sub) const
{
  int Good = 1;
  char* p = Sub;
  char ch;
  while ( (Good == 1) && ((ch=*p) != '\0') ) {
    if ( (ch != '.') && (!isdigit(ch)) )
      Good = 0;
    p++;
  }
  if (Good == 1) {
    if (strchr(Sub, '.') == NULL)
      strcat(Sub, ".");
    p = strchr(Sub, '.');
    if (p[1] == '\0')
      strcat(Sub, "0");
    if (p[2] == '\0')
      strcat(Sub, "0");
  }
  return Good;
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_AllPA
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Format all paragraphs in a patent (paragraphs
   #			are designated by tags like PAR, PA1, etc)
   #
   #
   ###################################################################
   */

void 
USPAT::pato_html_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group) const
{
  PATO_FIELD* Field;
  char Format[6];
  char StartCode[22];
  char Buffer2[4000];
  int Good;
  int x, y;
  if (Group == NULL)
    return;
  Format[0] = '\0';
  Field = Group->Fields;
  while (Field != NULL) {
    if (Field->ID[0] != '\0') {
      StartCode[0] = '\0';
      Good = 0;
      if ( (Field->ID[0] == 'P') && (Field->ID[1] = 'A') ) {
	switch (Field->ID[2]) {
	case 'C':
	case 'R':
	case 'L':
	case '0':
	  Good = 1;
	  break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	  Good = 1;
	  x = atoi(Field->ID+2);
	  StartCode[0] = '\0';
	  for (y=1; y<=x; y++)
	    strcat(StartCode, "<UL>");
	  break;
	}
      }
      if ( (strcmp(Field->ID, "STM") == 0) ||
	  (strcmp(Field->ID, "FNT") == 0) )
	Good = 1;
      if (strcmp(Field->ID, "TBL") == 0) {
	Good = 1;
	strcpy(StartCode, "<PRE WIDTH=\"80\"><UL>\n");
      }
      if (strcmp(Field->ID, "EQU") == 0) {
	Good = 1;
	strcpy(StartCode, "<PRE WIDTH=\"80\"><UL>\n");
      }
      pato_html_CloseFormat(Output, Format);
      pato_Write(Output, StartCode);
      if (Good == 1) {
	strcpy(Format, Field->ID);
	pato_Write(Output, "<P>");
      } else {
	Format[0] = '\0';
      }
    }
    if (Format[0] != '\0') {
      Good = 0;
      /*
	 pato_html_FigLinks(Field->Data, Buffer1);
	 */
      pato_html_Special(Field->Data, Buffer2);
      
      // strcpy(Buffer2, Field->Data);
      if ( (strcmp(Format, "STM") == 0) ||
	  (strcmp(Format, "FNT") == 0) ) {
	pato_Write(Output, "<I>");
	pato_Write(Output, Buffer2);
	pato_Write(Output, "</I>\n");
	Good = 1;
      }
      if (strcmp(Format, "PAC") == 0) {
	pato_Write(Output, "<HR><B><CENTER>");
	pato_Write(Output, Buffer2);
	pato_Write(Output, "</CENTER></B><HR>\n");
	Good = 1;
      }
      if (Good == 0) {
	pato_Write(Output, Buffer2);
	pato_Write(Output, "\n");
      }
    }
    Field = Field->Next;
  }
  pato_html_CloseFormat(Output, Format);
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_text_AllPA
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Format all PA paragraphs as plain text
   #			This can be made quite elaborate.
   #
   #
   ###################################################################
   */
void 
USPAT::pato_text_AllPA(PATO_OUTPUT* Output, PATO_GROUP* Group) const
{
  PATO_FIELD* Field;
  char Format[6];
  char StartCode[20];
  char  Buffer2[4000];
  int Good;
  int x;
  if (Group == NULL)
    return;
  Format[0] = '\0';
  Field = Group->Fields;
  while (Field != NULL) {
    if (Field->ID[0] != '\0') {
      StartCode[0] = '\0';
      Good = 0;
      if ( (Field->ID[0] == 'P') && (Field->ID[1] = 'A') ) {
	switch (Field->ID[2]) {
	case 'C':
	case 'R':
	case 'L':
	case '0':
	  Good = 1;
	  break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	  Good = 1;
	  x = atoi(Field->ID+2);
	  StartCode[0] = '\0';
	  /*
	     for (y=1; y<=x; y++)
	     strcat(StartCode, "");
	     */
	  break;
	}
      }
      if ( (strcmp(Field->ID, "STM") == 0) ||
	  (strcmp(Field->ID, "FNT") == 0) )
	Good = 1;
      if (strcmp(Field->ID, "TBL") == 0) {
	Good = 1;
	strcpy(StartCode, "\n");
      }
      if (strcmp(Field->ID, "EQU") == 0) {
	Good = 1;
	strcpy(StartCode, "\n");
      }
      /*			pato_html_CloseFormat(Output, Format);*/
      pato_Write(Output, StartCode);
      if (Good == 1) {
	strcpy(Format, Field->ID);
	pato_Write(Output, "\n");
      } else {
	Format[0] = '\0';
      }
    }
    if (Format[0] != '\0') {
      Good = 0;
      /*
	 pato_html_FigLinks(Field->Data, Buffer1);
	 pato_html_Special(Buffer1, Buffer2);
	 */
      strcpy(Buffer2, Field->Data);
      if ( (strcmp(Format, "STM") == 0) ||
	  (strcmp(Format, "FNT") == 0) ) {
	pato_Write(Output, Buffer2);
	pato_Write(Output, "\n");
	Good = 1;
      }
      if (strcmp(Format, "PAC") == 0) {
	pato_Write(Output, Buffer2);
	pato_Write(Output, "\n");
	Good = 1;
      }
      if (Good == 0) {
	pato_Write(Output, Buffer2);
	pato_Write(Output, "\n");
      }
    }
    Field = Field->Next;
  }
  /*	pato_html_CloseFormat(Output, Format);*/
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_FigLinks
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Link a reference to a figure to a list of
   #			figures, or the figure itself.  Not used yet
   #
   #
   ###################################################################
   */
void 
USPAT::pato_html_FigLinks(char* Data, char* Buffer) const
{
  char* p = Data;
  char* f;
  char temp;
  char fig[6];
  *Buffer = '\0';
  while ( (f=strstr(p, "FIG")) != NULL ) {
    temp = *f;
    *f = '\0';
    strcat(Buffer, p);
    *f = temp;
    if ( (f[3] == '.') || ( (f[3] == 'S') && (f[4] == '.') ) ) {
      if (f[3] == '.')
	strcpy(fig, "FIG.");
      else
	strcpy(fig, "FIGS.");
      sprintf(Buffer, "<A HREF=\"http: //%s/pto/figs.html\">",
	      pato_HostName);
      strcat(Buffer, fig);
      strcat(Buffer, "</A>");
      p = f + strlen(fig);
    } else {
      strcat(Buffer, "FIG");
      p = f + 3;
    }
  }
  strcat(Buffer, p);
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_Special
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Format .xxx. tags in a patent to show the 
   #			actual symbol
   #
   #
   ###################################################################
   */
void 
USPAT::pato_html_Special(char* Data, char* Buffer) const
{
  char* p = Data;
  char* f;
  int sp;
  char temp;
  *Buffer = '\0';
  while ( (f=strchr(p, '.')) != NULL ) {
    temp = *f;
    *f = '\0';
    strcat(Buffer, p);
    *f = temp;
    sp = 0;
    if ( strncmp(f, ".degree.", 8) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I> deg. </I>");
      p = f + 8;
    }
    if ( strncmp(f, ".parallel.", 10) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I> parallel to </I>");
      p = f + 10;
    }
    if ( strncmp(f, ".perp.", 6) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I> perpendicular to </I>");
      p = f + 6;
    }
    if ( strncmp(f, ".chi.", 5) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I>chi</I>");
      p = f + 5;
    }
    if ( strncmp(f, ".ltoreq.", 8) == 0 ) {
      sp = 1;
      strcat(Buffer, "<=");
      p = f + 8;
    }
    if ( strncmp(f, ".gtoreq.", 8) == 0 ) {
      sp = 1;
      strcat(Buffer, ">=");
      p = f + 8;
    }
    if ( strncmp(f, ".theta.", 7) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I>theta</I>");
      p = f + 7;
    }
    if ( strncmp(f, ".times.", 7) == 0 ) {
      sp = 1;
      strcat(Buffer, "*");
      p = f + 7;
    }
    if ( strncmp(f, ".mu.", 4) == 0 ) {
      sp = 1;
      strcat(Buffer, "<I>mu</I>");
      p = f + 4;
    }
    if ( strncmp(f, ".div.", 5) == 0 ) {
      sp = 1;
      strcat(Buffer, "/");
      p = f + 5;
    }
    if ( strncmp(f, ".+-.", 4) == 0 ) {
      sp = 1;
      strcat(Buffer, "+/-");
      p = f + 4;
    }
    if ( strncmp(f, ".sup.", 5) == 0 ) {
      char* e = f + 5;
      sp = 1;
      while ( (*e != ' ') && (*e != ',') && (*e != '.') &&
	     (*e != ')') && (*e != '\0') )
	e++;
      temp = *e;
      *e = '\0';
      strcat(Buffer, "(^<I>");
      strcat(Buffer, f + 5);
      strcat(Buffer, "</I>)");
      *e = temp;
      p = e;
    }
    if ( strncmp(f, ".sub.", 5) == 0 ) {
      char* e = f + 5;
      sp = 1;
      while ( (*e != ' ') && (*e != ',') && (*e != '.') &&
	     (*e != ')') && (*e != '\0') )
	e++;
      temp = *e;
      *e = '\0';
      strcat(Buffer, "(<I>");
      strcat(Buffer, f + 5);
      strcat(Buffer, "</I>)");
      *e = temp;
      p = e;
    }
    if ( (strncmp(f, ".sbsb.", 6) == 0) ||
	(strncmp(f, ".sbsp.", 6) == 0) ) {
      char* e = f + 6;
      sp = 1;
      while ( (*e != ' ') && (*e != ',') && (*e != '.') &&
	     (*e != ')') && (*e != '\0') )
	e++;
      temp = *e;
      *e = '\0';
      strcat(Buffer, "(<I>");
      strcat(Buffer, f + 6);
      strcat(Buffer, "</I>)");
      *e = temp;
      p = e;
    }
    if ( (strncmp(f, ".spsp.", 6) == 0) ||
	(strncmp(f, ".spsb.", 6) == 0) ) {
      char* e = f + 6;
      sp = 1;
      while ( (*e != ' ') && (*e != ',') && (*e != '.') &&
	     (*e != ')') && (*e != '\0') )
	e++;
      temp = *e;
      *e = '\0';
      strcat(Buffer, "(^<I>");
      strcat(Buffer, f + 6);
      strcat(Buffer, "</I>)");
      *e = temp;
      p = e;
    }
    if (!sp) {
      strcat(Buffer, ".");
      p = f + 1;
    }
  }
  strcat(Buffer, p);
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_CloseFormat
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Add an html formatting end tag
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_html_CloseFormat(PATO_OUTPUT* Output, char* Format) const
{
  int x, y;
  if ( (Format[0] == 'P') &&
      (Format[1] == 'A') ) {
    switch (Format[2]) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
      x = atoi(Format+2);
      for (y=1; y<=x; y++)
	pato_Write(Output, "</UL>");
      pato_Write(Output, "\n");
    }
  }
  if ( (strcmp(Format, "TBL") == 0) ||
      (strcmp(Format, "EQU") == 0) )
    pato_Write(Output, "</UL></PRE>\n");
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_ExpandDate
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Convert a greenbook date into a human-readable
   #			date
   #
   #
   ###################################################################
   */

void 
USPAT::pato_ExpandDate(char* S, char* Buffer) const
{
  int x;
  char T[PATO_MAXSTR];
  sprintf(T, "%c%c", S[4], S[5]);
  x = atoi(T);
  pato_GetMonth(x, T);
  strcpy(Buffer, T);
  sprintf(T, "%c%c", S[6], S[7]);
  x = atoi(T);
  if (x == 0)
    sprintf(T, ", ");
  else
    sprintf(T, " %i, ", x);
  strcat(Buffer, T);
  S[4] = '\0';
  strcat(Buffer, S);
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_Continue
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Continue writing HTML when text breaks across
   #			patent fields (PA!, etc)
   #
   #
   ###################################################################
   */

void 
USPAT::pato_html_Continue(PATO_OUTPUT* Output, PATO_FIELD* Field) const
{
  while ( (Field->Next != NULL) && (Field->Next->ID[0] == '\0') ) {
    Field = Field->Next;
    pato_Write(Output, Field->Data);
    pato_Write(Output, "\n");
  }
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_GetMonth
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Convert numeric month into month name
   #			
   #
   #
   ###################################################################
   */

char* 
USPAT::pato_GetMonth(int x, char* S)  const
{
  switch (x) {
  case 1:			strcpy(S, "Jan.");	break;
			      case 2:			strcpy(S, "Feb.");	break;
			      case 3:			strcpy(S, "Mar.");	break;
			      case 4:			strcpy(S, "Apr.");	break;
			      case 5:			strcpy(S, "May");	break;
			      case 6:			strcpy(S, "Jun.");	break;
			      case 7:			strcpy(S, "Jul.");	break;
			      case 8:			strcpy(S, "Aug.");	break;
			      case 9:			strcpy(S, "Sept.");	break;
			      case 10:		strcpy(S, "Oct.");	break;
			      case 11:		strcpy(S, "Nov.");	break;
			      case 12:		strcpy(S, "Dec.");	break;
			      default:		strcpy(S, "???");
			      }
  return S;
}

/*
   ###################################################################
   #
   #	MODULE: 	pato_PA
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Return TRUE if ID is a paragraph formatting
   #			tag
   #
   #
   ###################################################################
   */
int 
USPAT::pato_PA(char* ID) const
{
  /* there are better ways to do this (faster) (no kidding) */
  return ((strcmp(ID, "PAR") == 0) ||
	  (strcmp(ID, "PAC") == 0) ||
	  (strcmp(ID, "PAL") == 0) ||
	  (strcmp(ID, "PA0") == 0) ||
	  (strcmp(ID, "PA1") == 0) ||
	  (strcmp(ID, "PA2") == 0) ||
	  (strcmp(ID, "PA3") == 0) ||
	  (strcmp(ID, "PA4") == 0) ||
	  (strcmp(ID, "PA5") == 0) ||
	  (strcmp(ID, "FNT") == 0) ||
	  (strcmp(ID, "") == 0) );
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_html_Names
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Process name fields (inventor, assignee, etc
   #			have multipart fields) for proper HTML display
   #
   #
   ###################################################################
   */
void 
USPAT::pato_html_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
			    int MaxNumNames) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  int c;
  int CityFlag = 0;
  int StateFlag = 0;
  int ZipFlag = 0;
  char GID[6], CNT[6];
  Group = GroupStart;
  if (Group == NULL)
    return;
  strcpy(GID, Group->ID);
  c = 1;
  while ( (Group != NULL) && (strcmp(Group->ID, GID) == 0) &&
	 (c <= MaxNumNames) ) {
    Field = pato_FindField(Group, "NAM");
    if (Field != NULL) {
      pato_Write(Output, "<B>");
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, "</B>\n");
    }
    pato_Write(Output, "(");
    Field = pato_FindField(Group, "STR");
    if (Field != NULL) {
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, ", \n");
    }
    Field = pato_FindField(Group, "CTY");
    if (Field != NULL) {
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      CityFlag = 1;
    }
    Field = pato_FindField(Group, "STA");
    if (Field != NULL) {
      StateFlag = 1;
      if (CityFlag == 1) 
	pato_Write(Output, ", ");
      pato_Write(Output, Field->Data);
    }
    Field = pato_FindField(Group, "ZIP");
    if (Field != NULL) {
      ZipFlag = 1;
      pato_Write(Output, "  ");
      pato_Write(Output, Field->Data);
    }
    Field = pato_FindField(Group, "CNT");
    if (Field != NULL) {
      strcpy(CNT, Field->Data);
      
      if ( (CityFlag == 1) || (StateFlag == 1) || (ZipFlag == 1) )
	pato_Write(Output, ", ");
      if ( CNT[2] == 'X') CNT[2] = '\0';
      pato_Write(Output, CNT);
    }
    pato_Write(Output, ")");
    Field = pato_FindField(Group, "ITX");
    if (Field != NULL) {
      pato_Write(Output, ", \n");
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, "\n");
    }
    
    Group = Group->Next;
    if( strcmp(Group->ID, GID)==0)
      pato_Write(Output, ";\n");
    else
      pato_Write(Output, ".\n");
    c++;
  }
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_NameCount
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		10/3/95
   #	TYPE:		Private
   #	COMMENTS:	Count Inventor Names and write header tag
   #			
   #
   #
   ###################################################################
   */
void 
USPAT::pato_NameCount(PATO_OUTPUT* Output, PATO_GROUP* GroupStart,
			   int MaxNumNames) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  char Tag[256];
  
  int c;
  char  GID[6],*p;
  Group = GroupStart;
  if (Group == NULL)
    return;
  strcpy(GID, Group->ID);
  c = 1;
  strcpy(Tag,"<b>");
  while ( (Group != NULL) && (strcmp(Group->ID, GID) == 0) &&
	 (c <= MaxNumNames) ) {
    Field = pato_FindField(Group, "NAM");
    if (Field != NULL) {
      if(c==1)
	strcat(Tag,Field->Data);
      
    }
    Group = Group->Next;
    
    c++;
  }
  p=strchr(Tag,';');
  if(p)
    *p='\0';
  if(c>2)
    strcat(Tag,", et. al.");
  strcat(Tag,"</B>");
  pato_Write(Output,Tag);
  
}
/*
   ###################################################################
   #
   #	MODULE: 	pato_text_Names
   #	AUTHOR:		Nassib Nassar, Jim Fullton
   #	DATE:		8/30/95
   #	TYPE:		Private
   #	COMMENTS:	Process name fields for proper SUTRS display
   #			
   #
   #
   ###################################################################
   */

void 
USPAT::pato_text_Names(PATO_OUTPUT* Output, PATO_GROUP* GroupStart) const
{
  PATO_GROUP* Group;
  PATO_FIELD* Field;
  char GID[6], CNT[6];
  Group = GroupStart;
  if (Group == NULL)
    return;
  strcpy(GID, Group->ID);
  while ( (Group != NULL) && (strcmp(Group->ID, GID) == 0) ) {
    Field = pato_FindField(Group, "NAM");
    if (Field != NULL) {
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
    }
    pato_Write(Output, " (");
    Field = pato_FindField(Group, "STR");
    if (Field != NULL) {
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
      pato_Write(Output, ", ");
    }
    Field = pato_FindField(Group, "CTY");
    if (Field != NULL) {
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
    }
    Field = pato_FindField(Group, "STA");
    if (Field != NULL) {
      pato_Write(Output, ", ");
      pato_Write(Output, Field->Data);
    }
    Field = pato_FindField(Group, "ZIP");
    if (Field != NULL) {
      pato_Write(Output, "  ");
      pato_Write(Output, Field->Data);
    }
    Field = pato_FindField(Group, "CNT");
    if (Field != NULL) {
      strcpy(CNT, Field->Data);
      pato_Write(Output, ", ");
      if (CNT[2] == 'X') CNT[2] = '\0'; // knock 'X' off country codes.
      pato_Write(Output, CNT);
    }
    pato_Write(Output, ")");
    Field = pato_FindField(Group, "ITX");
    if (Field != NULL) {
      pato_Write(Output, ", ");
      pato_Write(Output, Field->Data);
      pato_html_Continue(Output, Field);
    }
    Field = pato_FindField(Group, "COD");
    if (Field != NULL) {
      pato_Write(Output, "; Assignee type: ");
      switch (Field->Data[1]) {
      case '2':
	pato_Write(Output,
		   "U.S. Company or Corporation");
	break;
      case '3':
	pato_Write(Output,
		   "Foreign Company or Corporation");
	break;
      case '4':
	pato_Write(Output,
		   "U.S. Individual");
	break;
      case '5':
	pato_Write(Output,
		   "Foreign Individual");
	break;
      case '6':
	pato_Write(Output,
		   "U.S. Government");
	break;
      case '7':
	pato_Write(Output,
		   "Foreign Government");
	break;
      case '8':
	pato_Write(Output,
		   "County Government");
	break;
      case '9':
	pato_Write(Output,
		   "U.S. State Government");
	break;
      default:
	pato_Write(Output,
		   "Unknown");
      }
      if (Field->Data[0] == '1')
	pato_Write(Output, " (part interest)");
    }
    pato_Write(Output, ".\n");
    Group = Group->Next;
  }
}


GB::GB(char *Record, const int32_t Length)
{
  c_length = Length;
  c_record = Record;
}

GB::~GB()
{
}

//
// Does all the real work of parsing the record and building a DFT
//
// Re-written By Glenn MacStravic and Kevin Gamiel, 2/22/96
//
PDFT GB::BuildDft(PIDBOBJ Db, STRLIST& FieldNameList, STRLIST& HierarchiesList)
{
  //
  // Build the new DFT to be filled and returned
  //
  PDFT pdft;
  pdft = new DFT();
  
  STRING	Name, Entry;
  int32_t		Start=0,
  NextFieldStart;
  bool 	InH;
  STRING 		H;
  
  int Done=0;
  InH = false;
  Done = GetFieldName(Start, &Name, &NextFieldStart);
  while (!Done) {
    // Name is field name
    // Start is starting position
    // (NextFieldStart - 1) is ending position
    
    if(InH) {
      STRING t, u;
      t = H;
      u = H;
      u.Cat("-*");
      t.Cat("-");
      t.Cat(Name);
      //
      // Is this a subfield of the current hierarchy?
      //
      if(FieldNameList.SearchCase(t) > 0) {
	//char *field_name;
	//field_name = t.NewCString();
	InsertField(Db, pdft, t, Start,
		    NextFieldStart-1);
	//delete [] field_name;
      } else if (FieldNameList.SearchCase(Name) > 0) {
	// //cout << "Falling out of h b/c we found field " << Name << endl;
	// If this is a non-hierarchical field we're looking at, treat as 
	// special case (pass through while() loop without loading new field
	
	//cout << "Non-hierchical field detected...exiting " << H;
	//cout << " Hierarchy" << endl;
	InH = false;
	continue;
      }
      else if (HierarchiesList.SearchCase(Name) > 0) {
	// If this is a new hierarchy, continue the loop without
	// grabbing the next field
	//cout << "New Hierachy detected...exiting " << H;
	//cout << " Hierachy." << endl;
	InH = false;
	continue;
      } else if (FieldNameList.SearchCase(u) > 0) {
	char *field_name;
	//field_name= u.NewCString();
	InsertField(Db, pdft, u, Start,
		    NextFieldStart-1);
	//delete [] field_name;
      }
    }
    
    else {			// Not in a hieracrchy
      //
      // Is this a field we're looking for?
      //
      if(FieldNameList.SearchCase(Name) > 0) {
	char *field_name;
	//field_name = Name.NewCString();
	//cout << "Inserting Field: " << field_name << endl;
	InsertField(Db, pdft, Name, Start, 
		    NextFieldStart-1);
	//delete [] field_name;
      }
      //
      // Is this the top of a hierarchy?
      //
      if (HierarchiesList.SearchCase(Name) > 0) {
	InH = true;
	//cout << "Starting Hierarchy "<< Name << endl;
	H = Name;
      }
    }
    
    //cout << "Found field " << Name << "[" << Start;
    //cout << "," << NextFieldStart-1 << "]" << endl;
    //cin.ignore();
    
    Start = NextFieldStart;
    Done=GetFieldName(Start, &Name, &NextFieldStart);
  }
  //cout << "Done!" << endl;
  return pdft;
}

//
// 	Start points to first character of a field name in c_record.
// 	Name will contain the name of the field on return.
// 	NextFieldStart will contain the position of the next field in c_record.
//		If no more fields exist, this value will be 0.
//
int GB::GetFieldName(const int32_t Start, STRING *Name, int32_t *NextFieldStart)
{
  char tmp[5];
  memcpy(tmp, c_record+Start, 4);
  tmp[4] = '\0';
  *Name = tmp;
  STRINGINDEX t;
  t = Name->Search(' ');
  if(t > 0)
    Name->EraseAfter(t-1);
  
  int 	Done=0;
  char 	*ColumnStartPtr=c_record + Start, *ptr;
  do {
    ptr = strchr(ColumnStartPtr, '\n');
    if(ptr) {
      ColumnStartPtr = ptr;
      ++ColumnStartPtr;
    } else {
      Done=1;
      *NextFieldStart = strlen(c_record);
    }
    
  } while((!Done) && ((ColumnStartPtr-c_record)<c_length) && 
	  (*ColumnStartPtr==' '));
  if(!Done)
    *NextFieldStart = ColumnStartPtr - c_record;
  return(Done);
}

// Inserts Field into DFD - GEM 2/21/96

void GB::InsertField(PIDBOBJ Db, PDFT pdft, const STRING& Field, int Start, int End) const
{
  STRING FieldName;
  FC fc;
  FCT *pfct;
  DF df;
  DFD dfd;
  STRING temp = Field;
  
  temp.UpperCase();
  
  //cout << "Inserting field " << Field << " [" << Start << ",";
  //cout << End << "]" << endl; 
  
  switch (temp.GetChr(1)) {
    
  case 'A':
    if(temp == "ABST-*")
      FieldName="ABST";
    else if(temp == "ASSG-NAM")
      FieldName="AN";
    else if(temp == "ASSG-CTY")
      FieldName="AC";
    else if(temp == "ASSG-STA")
      FieldName="AS";
    else if(temp == "ASSG-CNT")
      FieldName="ACN";
    break;
    
  case 'B':
    if(temp == "BSUM-*")
      FieldName="BSUM";
    break;
    
  case 'C':
    if(temp == "CLAS-OCL")
      FieldName="OCL";
    else if(temp == "CLAS-XCL")
      FieldName="OCL";
    else if(temp == "CLAS-ICL")
      FieldName="ICL";
    else if(temp == "CLMS-*")
      FieldName="CLMS";
    break;
    
  case 'D':
    if(temp == "DCLM-*")
      FieldName="ABST";
    else if(temp == "DETD-*")
      FieldName="DETD";
    else if(temp == "DRWD-*")
      FieldName="DRWD";
    break;
    
  case 'F':
    if(temp == "FREF-*")
      FieldName="REF";
    break;
    
  case 'I':
    if(temp == "INVT-NAM")
      FieldName="IN";
    else if(temp == "INVT-CTY")
      FieldName="IC";
    else if(temp == "INVT-STA")
      FieldName="IS";
    else if(temp == "INVT-CNT")
      FieldName="ICN";
    break;
    
  case 'L':
    if(temp == "LREP-*")
      FieldName="LREP";
    
  case 'O':
    if(temp == "OREF-*")
      FieldName="REF";
    break;
    
  case 'P':
    if(temp == "PATN-WKU")
      FieldName="PN";
    else if(temp == "PATN-TTL")
      FieldName="TTL";
    else if(temp == "PATN-APN")
      FieldName="APN";
    else if(temp == "PATN-APD")
      FieldName="APD";
    break;
    
  case 'U':
    if(temp == "UREF-*")
      FieldName="REF";
    break;
    
  default:
    FieldName = Field;
  }
  
  dfd.SetFieldName(FieldName);
  Db->DfdtAddEntry(dfd);
  fc.SetFieldStart(Start);
  fc.SetFieldEnd(End);
  pfct = new FCT();
  pfct->AddEntry(fc);
  df.SetFct(*pfct);
  df.SetFieldName(FieldName);
  pdft->AddEntry(df);
  delete pfct;
}

