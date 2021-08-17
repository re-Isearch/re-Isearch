/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:		index.cxx
Version:	$Revision: 1.1 $
Description:	Class NUMERICFLDMGR
Author:		Jim Fullton, CNIDR
@@@*/

#include <stdlib.h>
#include <string.h>

#include "gdt.h"
#include "defs.hxx"
#include "string.hxx"
#include "nfield.hxx"
#include "nlist.hxx"
#include "vlist.hxx"
#include "attr.hxx"
#include "attrlist.hxx"
#include "mdtrec.hxx"
#include "mdt.hxx"
#include "dfd.hxx"
#include "dfdt.hxx"
#include "fc.hxx"
#include "fct.hxx"
#include "df.hxx"
#include "dft.hxx"
#include "result.hxx"
#include "strlist.hxx"
#include "record.hxx"
#include "idbobj.hxx"
#include "iresult.hxx"
#include "opobj.hxx"
#include "operand.hxx"
#include "rset.hxx"
#include "irset.hxx"
#include "opstack.hxx"
#include "squery.hxx"
#include "dtreg.hxx"
#include "nfldmgr.hxx"



NUMERICFLDMGR::NUMERICFLDMGR()
{
  NumFields = 0;
		
}
NUMERICFLDMGR::~NUMERICFLDMGR()
{
  if(NumFields>0)
    delete [] fields;
}

INT NUMERICFLDMGR::Find(INT Attribute, INT4 Relation, FLOAT Key)
{
  INT FieldIndex;
  INT4 Position=0;

  FieldIndex=LocateFieldByAttribute(Attribute);
  if(FieldIndex==-1)		// no such field
    return(0);
//  Position=fields[FieldIndex].Find(Key,Relation);
  if(Position==-1)
    return(0);
  else
    return(1);
}



/* reads field definition file and loads fields */

INT NUMERICFLDMGR::LoadFields(PCHR dbName)
{
  CHR FullName[256];
  CHR Input[256];
  CHR TypeString[128];
  INT Attribute;
  FILE *fp;
  INT counter=0;
  
  sprintf(FullName,"%s.fdf",dbName); // make definition file name
  fp=fopen(FullName,"rb");
  if(fp==NULL)
    return(0);			// no fields
  while(fgets(Input,256,fp)!=NULL){
    PCHR p;
    p=strchr(Input,'\n');
    if(p)
      *p='\0';			// zap newline
    p=strchr(Input,'#');
    if(p)
      *p='\0';			// zap comments
    if(!strlen(Input))
      continue;
    ++counter;
  }

  fields=new NUMERICLIST[counter];
  rewind(fp);
  while(fgets(Input,256,fp)!=NULL){
    PCHR p;
    p=strchr(Input,'\n');
    if(p)
      *p='\0';			// zap newline
    p=strchr(Input,'#');
    if(p)
      *p='\0';			// zap comments
    if(!strlen(Input))
      continue;
    sscanf(Input,"%d %s",&Attribute,TypeString);
    // 62	TEXT
    // 12	NUMERIC or whatever
    // etc

    FIELDTYPE FieldType (TypeString);
    
    if(FieldType.IsText()){
      continue;
    }else{
      // make the field
      CHR FieldFile[256];
      
      sprintf(FieldFile,"%s.%d",dbName,Attribute);
      fields[NumFields].SetFileName(FieldFile);
//      fields[NumFields].LoadTable();
      if(fields[NumFields].GetCount()==0)
	continue;
      fields[NumFields].SetAttribute(Attribute);
      fields[NumFields].Sort();
      ++NumFields;
      
      
    }
  }	
  fclose(fp);
  
  return(NumFields);
}

/* returns -1 if no field for this attribute, field index if matched */

INT NUMERICFLDMGR::LocateFieldByAttribute(INT Attribute)
{
  INT i,hit=0;
  
  for(i=0; i<NumFields; i++){
    if(Attribute==fields[i].GetAttribute()){
      hit=1;
      break;
    }
  }
  if(hit==0)
    return(-1);
  else{
    return(i);
  }
}


void NUMERICFLDMGR::GetResult(INT use, PIRSET s, PIDBOBJ Parent)
{
  INT i,w;
  INT4 gp;
  IRESULT iresult;
  //fields[i].ResetHitPosition();
  i=LocateFieldByAttribute(use);
//  printf("GetResult: Found field %d by attribute\n",i);
  while((gp=fields[i].GetNextHitPosition())!=-1){
    w = Parent->GetMainMdt()->LookupByGp(gp);
    iresult.SetMdtIndex(w);
    iresult.SetHitCount(1);
    iresult.SetScore(0);
    s->AddEntry(iresult, 1);
  }
}
